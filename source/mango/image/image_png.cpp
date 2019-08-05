/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
//#define MANGO_ENABLE_DEBUG_PRINT

#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>

#define ID "[ImageDecoder.PNG] "
#define FILTER_BYTE 1

// https://www.w3.org/TR/2003/REC-PNG-20031110/
// https://wiki.mozilla.org/APNG_Specification

// TODO: discard modes (requires us to keep a copy of main image)
// TODO: check that animations starting with "IDAT" and "fdAT" work correctly
// TODO: SIMD blending (not critical)

// ------------------------------------------------------------
// miniz
// ------------------------------------------------------------

#include "../../external/miniz/miniz.h"
#undef crc32 // fix miniz pollution

// ------------------------------------------------------------
// stb zlib
// ------------------------------------------------------------

#define STBI_NO_STDIO
#define STBI_NO_JPEG
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_PIC
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_PNM
//#define STBI_NO_PNG
//#define STBI_NO_HDR

#define STBI_SUPPORT_ZLIB

#define STBI_MALLOC(sz)           malloc(sz)
#define STBI_REALLOC(p,newsz)     realloc(p,newsz)
#define STBI_FREE(p)              free(p)

#define STB_IMAGE_IMPLEMENTATION

#include "../../external/stb/stb_image.h"

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // Filter
    // ------------------------------------------------------------

    enum FilterType : u8
    {
        FILTER_NONE    = 0,
        FILTER_SUB     = 1,
        FILTER_UP      = 2,
        FILTER_AVERAGE = 3,
        FILTER_PAETH   = 4,
    };

    typedef void (*FilterFunc)(u8* scan, const u8* prev, int bytes, int bpp);

    void filter_sub(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED_PARAMETER(prev);

        for (int x = bpp; x < bytes; ++x)
        {
            scan[x] += scan[x - bpp];
        }
    }

    void filter_up(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED_PARAMETER(bpp);

        for (int x = 0; x < bytes; ++x)
        {
            scan[x] += prev[x];
        }
    }

    void filter_average(u8* scan, const u8* prev, int bytes, int bpp)
    {
        for (int x = 0; x < bpp; ++x)
        {
            scan[x] += prev[x] / 2;
        }

        for (int x = bpp; x < bytes; ++x)
        {
            scan[x] += (prev[x] + scan[x - bpp]) / 2;
        }
    }

    void filter_paeth(u8* scan, const u8* prev, int bytes, int bpp)
    {
        for (int x = 0; x < bpp; ++x)
        {
            scan[x] += prev[x];
        }

        for (int x = bpp; x < bytes; ++x)
        {
            int c = prev[x - bpp];
            int a = scan[x - bpp];
            int b = prev[x];

            int p = b - c;
            int q = a - c;
            int pa = std::abs(p);
            int pb = std::abs(q);
            int pc = std::abs(p + q);

            if (pb < pa)
            {
                pa = pb;
                a = b;
            }

            if (pc < pa)
            {
                a = c;
            }

            scan[x] += u8(a);
        }
    }

    void filter_paeth_8bit(u8* scan, const u8* prev, int bytes, int bpp)
    {
        int c = prev[0];
        int a = (scan[0] + c) & 0xff;
        scan[0] = u8(a);

        for (int x = 1; x < bytes; ++x)
        {
            int b = prev[x];

            int p = b - c;
            int q = a - c;
            int pa = abs(p);
            int pb = abs(q);
            int pc = abs(p + q);

            if (pb < pa)
            {
                pa = pb;
                a = b;
            }

            if (pc < pa)
            {
                a = c;
            }

            c = b;
            a = (a + scan[x]) & 0xff;
            scan[x] = u8(a);
        }
    }

#if defined(MANGO_ENABLE_SSE2)

    // -----------------------------------------------------------------------------------
    // SSE2 Filters
    // -----------------------------------------------------------------------------------

    inline int8x16 load4(const void* p)
    {
        u32 temp = uload32(p);
        return int8x16(_mm_cvtsi32_si128(temp));
    }

    inline void store4(void* p, __m128i v)
    {
        u32 temp = _mm_cvtsi128_si32(v);
        ustore32(p, temp);
    }

    inline int8x16 load3(const void* p)
    {
        u32 temp = 0;
        std::memcpy(&temp, p, 3);
        return int8x16(_mm_cvtsi32_si128(temp));
    }

    inline void store3(void* p, __m128i v)
    {
        u32 temp = _mm_cvtsi128_si32(v);
        std::memcpy(p, &temp, 3);
    }

    inline __m128i average_sse2(__m128i a, __m128i b, __m128i d)
    {
        __m128i avg = _mm_avg_epu8(a, b);
        avg = _mm_sub_epi8(avg, _mm_and_si128(_mm_xor_si128(a, b), _mm_set1_epi8(1)));
        d = _mm_add_epi8(d, avg);
        return d;
    }

    inline int16x8 nearest_sse2(u8* scan, const u8* prev, __m128i zero, int16x8 a, int16x8 b, int16x8 c, int16x8 d)
    {
        int16x8 pa = b - c;
        int16x8 pb = a - c;
        int16x8 pc = pa + pb;
        pa = abs(pa);
        pb = abs(pb);
        pc = abs(pc);
        int16x8 smallest = min(pc, min(pa, pb));
        int16x8 nearest = select(smallest == pa, a,
                          select(smallest == pb, b, c));
        d = _mm_add_epi8(d, nearest);
        return d;
    }

    void filter_sub_24bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED_PARAMETER(prev);

        __m128i d = _mm_setzero_si128();

        for (int x = 0; x < bytes; x += 3)
        {
            d = _mm_add_epi8(load3(scan + x), d);
            store3(scan + x, d);
        }
    }

    void filter_sub_32bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED_PARAMETER(prev);

        __m128i d = _mm_setzero_si128();

        for (int x = 0; x < bytes; x += 4)
        {
            d = _mm_add_epi8(load4(scan + x), d);
            store4(scan + x, d);
        }
    }

    void filter_average_24bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        __m128i d = _mm_setzero_si128();

        for (int x = 0; x < bytes; x += 3)
        {
            d = average_sse2(d, load3(prev + x), load3(scan + x));
            store3(scan + x, d);
        }
    }

    void filter_average_32bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        __m128i d = _mm_setzero_si128();

        for (int x = 0; x < bytes; x += 4)
        {
            d = average_sse2(d, load4(prev + x), load4(scan + x));
            store4(scan + x, d);
        }
    }

    void filter_paeth_24bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        int8x16 zero = 0;
        int16x8 b = 0;
        int16x8 d = 0;

        for (int x = 0; x < bytes; x += 3)
        {
            int16x8 c = b;
            int16x8 a = d;
            b = _mm_unpacklo_epi8(load3(prev + x), zero);
            d = _mm_unpacklo_epi8(load3(scan + x), zero);
            d = nearest_sse2(scan, prev, zero, a, b, c, d);
            store3(scan + x, _mm_packus_epi16(d, d));
        }
    }

    void filter_paeth_32bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        int8x16 zero = 0;
        int16x8 b = 0;
        int16x8 d = 0;

        for (int x = 0; x < bytes; x += 4)
        {
            int16x8 c = b;
            int16x8 a = d;
            b = _mm_unpacklo_epi8(load4(prev + x), zero);
            d = _mm_unpacklo_epi8(load4(scan + x), zero);
            d = nearest_sse2(scan, prev, zero, a, b, c, d);
            store4(scan + x, _mm_packus_epi16(d, d));
        }
    }

#endif // MANGO_ENABLE_SSE2

#if defined(MANGO_ENABLE_NEON__todo)

    // -----------------------------------------------------------------------------------
    // NEON Filters
    // -----------------------------------------------------------------------------------

    void filter_up_neon(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED_PARAMETER(bpp);

        for (int x = 0; x < bytes; x += 16)
        {
            uint8x16_t a = vld1q_u8(scan + x);
            uint8x16_t b = vld1q_u8(prev + x);
            vst1q_u8(scan + x, vaddq_u8(a, b));
        }
    }

#endif // MANGO_ENABLE_NEON

    struct FilterDispatcher
    {
        FilterFunc sub = filter_sub;
        FilterFunc up = filter_up;
        FilterFunc average = filter_average;
        FilterFunc paeth = filter_paeth;

        FilterDispatcher(int bpp)
        {
            switch (bpp)
            {
                case 1:
                    paeth = filter_paeth_8bit;
                    break;
                case 3:
#if defined(MANGO_ENABLE_SSE2)
                    sub = filter_sub_24bit_sse2;
                    average = filter_average_24bit_sse2;
                    paeth = filter_paeth_24bit_sse2;
#endif
#if defined(MANGO_ENABLE_NEON__todo)
#endif

                    break;
                case 4:
#if defined(MANGO_ENABLE_SSE2)
                    sub = filter_sub_32bit_sse2;
                    average = filter_average_32bit_sse2;
                    paeth = filter_paeth_32bit_sse2;
#endif
#if defined(MANGO_ENABLE_NEON__todo)
#endif
                    break;
            }

#if defined(MANGO_ENABLE_NEON__todo)
            up = filter_up_neon;
#endif
        }

        void call(FilterType method, u8* scan, const u8* prev, int bytes, int bpp)
        {
            //printf("%d", method);
            switch (method)
            {
                case FILTER_NONE:
                default:
                    break;

                case FILTER_SUB:
                    sub(scan, prev, bytes, bpp);
                    break;

                case FILTER_UP:
                    up(scan, prev, bytes, bpp);
                    break;

                case FILTER_AVERAGE:
                    average(scan, prev, bytes, bpp);
                    break;

                case FILTER_PAETH:
                    paeth(scan, prev, bytes, bpp);
                    break;
            }
        }
    };

    // ------------------------------------------------------------
    // AdamInterleave
    // ------------------------------------------------------------

    struct AdamInterleave
    {
        int xorig;
        int yorig;
        int xspc;
        int yspc;
        int w;
        int h;

        AdamInterleave(u32 pass, u32 width, u32 height)
        {
            assert(pass >=0 && pass < 7);

            const u32 orig = 0x01020400 >> (pass * 4);
            const u32 spc = 0x01122333 >> (pass * 4);

            xorig = (orig >> 4) & 15;
            yorig = (orig >> 0) & 15;
            xspc = (spc >> 4) & 15;
            yspc = (spc >> 0) & 15;
            w = (width - xorig + (1 << xspc) - 1) >> xspc;
            h = (height - yorig + (1 << yspc) - 1) >> yspc;
        }
    };

    // ------------------------------------------------------------
    // ParserPNG
    // ------------------------------------------------------------

    enum ColorType
    {
        COLOR_TYPE_I       = 0,
        COLOR_TYPE_RGB     = 2,
        COLOR_TYPE_PALETTE = 3,
        COLOR_TYPE_IA      = 4,
        COLOR_TYPE_RGBA    = 6,
    };

    struct Chromaticity
    {
        float2 white;
        float2 red;
        float2 green;
        float2 blue;
    };

    struct Frame
    {
        enum Dispose : u8
        {
            NONE       = 0,
            BACKGROUND = 1,
            PREVIOUS   = 2,
        };

        enum Blend : u8
        {
            SOURCE = 0,
            OVER   = 1,
        };

        u32 sequence_number;
        u32 width;
        u32 height;
        u32 xoffset;
        u32 yoffset;
        u16 delay_num;
        u16 delay_den;
        u8 dispose;
        u8 blend;

        void read(BigEndianConstPointer p)
        {
            sequence_number = p.read32();
            width = p.read32();
            height = p.read32();
            xoffset = p.read32();
            yoffset = p.read32();
            delay_num = p.read16();
            delay_den = p.read16();
            dispose = p.read8();
            blend = p.read8();

            debugPrint("  Sequence: %d\n", sequence_number);
            debugPrint("  Frame: %d x %d (%d, %d)\n", width, height, xoffset, yoffset);
            debugPrint("  Time: %d / %d\n", delay_num, delay_den);
            debugPrint("  Dispose: %d\n", dispose);
            debugPrint("  Blend: %d\n", blend);
        }
    };

    class ParserPNG
    {
    protected:
        Memory m_memory;

        const u8* m_pointer = nullptr;
        const u8* m_end = nullptr;
        const char* m_error = nullptr;

        Buffer m_compressed;

        // IHDR
        int m_width;
        int m_height;
        int m_bit_depth;
        int m_color_type;
        int m_compression;
        int m_filter;
        int m_interlace;

        int m_channels;

        // PLTE
        Palette m_palette;

        // tRNS
        bool m_transparent_enable = false;
        u16 m_transparent_sample[3];
        ColorBGRA m_transparent_color;

        // cHRM
        Chromaticity m_chromaticity;

        // gAMA
        float m_gamma = 0.0f;

        // sBIT
        u8 m_scale_bits[4];

        // sRGB
        u8 m_srgb_render_intent = -1;

        // acTL
        u32 m_number_of_frames = 0;
        u32 m_repeat_count = 0;

        // fcTL
        Frame m_frame;
        const u8* m_first_frame = nullptr;

        void setError(const char* error);

        void read_IHDR(BigEndianConstPointer p, u32 size);
        void read_IDAT(BigEndianConstPointer p, u32 size);
        void read_PLTE(BigEndianConstPointer p, u32 size);
        void read_tRNS(BigEndianConstPointer p, u32 size);
        void read_cHRM(BigEndianConstPointer p, u32 size);
        void read_gAMA(BigEndianConstPointer p, u32 size);
        void read_sBIT(BigEndianConstPointer p, u32 size);
        void read_sRGB(BigEndianConstPointer p, u32 size);
        void read_acTL(BigEndianConstPointer p, u32 size);
        void read_fcTL(BigEndianConstPointer p, u32 size);
        void read_fdAT(BigEndianConstPointer p, u32 size);

        void parse();
        void filter(u8* buffer, int bytes, int height);
        void deinterlace1to4(u8* output, int width, int height, int stride, u8* buffer);
        void deinterlace8to16(u8* output, int width, int height, int stride, u8* buffer);

        void process_i1to4   (u8* dest, int width, int height, int stride, const u8* src);
        void process_i8      (u8* dest, int width, int height, int stride, const u8* src);
        void process_rgb8    (u8* dest, int width, int height, int stride, const u8* src);
        void process_pal1to4 (u8* dest, int width, int height, int stride, const u8* src, Palette* palette);
        void process_pal8    (u8* dest, int width, int height, int stride, const u8* src, Palette* palette);
        void process_ia8     (u8* dest, int width, int height, int stride, const u8* src);
        void process_rgba8   (u8* dest, int width, int height, int stride, const u8* src);
        void process_i16     (u8* dest, int width, int height, int stride, const u8* src);
        void process_rgb16   (u8* dest, int width, int height, int stride, const u8* src);
        void process_ia16    (u8* dest, int width, int height, int stride, const u8* src);
        void process_rgba16  (u8* dest, int width, int height, int stride, const u8* src);

        void process(u8* dest, int width, int height, int stride, u8* buffer, Palette* palette);

        void blend_ia8      (u8* dest, const u8* src, int width);
        void blend_ia16     (u8* dest, const u8* src, int width);
        void blend_bgra8    (u8* dest, const u8* src, int width);
        void blend_rgba16   (u8* dest, const u8* src, int width);
        void blend_indexed  (u8* dest, const u8* src, int width);

        void blend(Surface& d, Surface& s, Palette* palette);

        int getBytesPerLine(int width) const
        {
            return m_channels * ((m_bit_depth * width + 7) / 8);
        }

        int getImageBufferSize(int width, int height) const;

    public:
        ParserPNG(Memory memory);
        ~ParserPNG();

        const char* getError() const;

        ImageHeader header() const;
        const char* decode(Surface& dest, Palette* palette);
    };

    // ------------------------------------------------------------
    // ParserPNG
    // ------------------------------------------------------------

    ParserPNG::ParserPNG(Memory memory)
        : m_memory(memory)
        , m_end(memory.address + memory.size)
    {
        BigEndianConstPointer p = memory.address;

        // read header magic
        const u64 magic = p.read64();
        if (magic != 0x89504e470d0a1a0a)
        {
            setError("Incorrect header magic.");
            return;
        }

        // read first chunk; it must be IHDR
        const u32 size = p.read32();
        const u32 id = p.read32();
        if (id != u32_mask_rev('I', 'H', 'D', 'R'))
        {
            setError("Incorrect file; the IHDR chunk must come first.");
            return;
        }

        read_IHDR(p, size);
        p += size; // skip chunk data
        p += sizeof(u32); // skip crc

        // keep track of parsing position
        m_pointer = p;
    }

    ParserPNG::~ParserPNG()
    {
    }

    void ParserPNG::setError(const char* error)
    {
        if (!m_error)
        {
            m_error = error;
        }
    }

    const char* ParserPNG::getError() const
    {
        return m_error;
    }

    void ParserPNG::read_IHDR(BigEndianConstPointer p, u32 size)
    {
        debugPrint("[\"IHDR\"] %d bytes\n", size);

        if (size != 13)
        {
            setError("Incorrect IHDR chunk size.");
            return;
        }

        m_width = p.read32();
        m_height = p.read32();
        m_bit_depth = p.read8();
        m_color_type = p.read8();
        m_compression = p.read8();
        m_filter = p.read8();
        m_interlace = p.read8();

        if (m_width > 0x8000 || m_height > 0x8000)
        {
            setError("Too large image.");
            return;
        }

        if (!u32_is_power_of_two(m_bit_depth))
        {
            setError("Incorrect bit depth.");
            return;
        }

        // look-ahead into the chunks to see if we have transparency information
        p += 4; // skip crc
        for (; p < m_end - 8;)
        {
            const u32 size = p.read32();
            const u32 id = p.read32();
            switch (id)
            {
                case u32_mask_rev('t', 'R', 'N', 'S'):
                    m_transparent_enable = true;
                    break;
            }
            p += (size + 4);
        }

        // bit-depth range defaults
        int minBits = 3; // log2(8) bits
        int maxBits = 4; // log2(16) bits

        switch (m_color_type)
        {
            case COLOR_TYPE_I:
                // supported: 1, 2, 4, 8, 16 bits
                minBits = 0;
                m_channels = 1;
                break;
            case COLOR_TYPE_RGB:
                // supported: 8, 16 bits
                m_channels = 3;
                break;
            case COLOR_TYPE_PALETTE:
                // supported: 1, 2, 4, 8 bits
                minBits = 0;
                maxBits = 3;
                m_channels = 1;
                break;
            case COLOR_TYPE_IA:
                // supported: 8, 16 bits
                m_channels = 2;
                break;
            case COLOR_TYPE_RGBA:
                // supported: 8, 16 bits
                m_channels = 4;
                break;
            default:
                setError("Incorrect color type.");
                return;
        }

        const int log2bits = u32_log2(m_bit_depth);
        if (log2bits < minBits || log2bits > maxBits)
        {
            setError("Unsupported bit depth for color type.");
            return;
        }

        // load default scaling values (override from sBIT chunk)
        for (int i = 0; i < m_channels; ++i)
        {
            m_scale_bits[i] = m_bit_depth;
        }

        debugPrint("  Image: (%d x %d), %d bits\n", m_width, m_height, m_bit_depth);
        debugPrint("  Color:       %d\n", m_color_type);
        debugPrint("  Compression: %d\n", m_compression);
        debugPrint("  Filter:      %d\n", m_filter);
        debugPrint("  Interlace:   %d\n", m_interlace);
    }

    void ParserPNG::read_IDAT(BigEndianConstPointer p, u32 size)
    {
        m_compressed.append(p, size);
    }

    void ParserPNG::read_PLTE(BigEndianConstPointer p, u32 size)
    {
        if (size % 3)
        {
            setError("Incorrect palette size.");
            return;
        }

        if (size > 768)
        {
            setError("Too large palette.");
            return;
        }

        m_palette.size = size / 3;
        for (u32 i = 0; i < m_palette.size; ++i)
        {
            m_palette[i] = ColorBGRA(p[0], p[1], p[2], 0xff);
            p += 3;
        }
    }

    void ParserPNG::read_tRNS(BigEndianConstPointer p, u32 size)
    {
        if (m_color_type == COLOR_TYPE_I)
        {
            if (size != 2)
            {
                setError("Incorrect tRNS chunk size.");
                return;
            }

            m_transparent_enable = true;
            m_transparent_sample[0] = p.read16();
        }
        else if (m_color_type == COLOR_TYPE_RGB)
        {
            if (size != 6)
            {
                setError("Incorrect tRNS chunk size.");
                return;
            }

            m_transparent_enable = true;
            m_transparent_sample[0] = p.read16();
            m_transparent_sample[1] = p.read16();
            m_transparent_sample[2] = p.read16();
            m_transparent_color = ColorBGRA(m_transparent_sample[0] & 0xff,
                                            m_transparent_sample[1] & 0xff,
                                            m_transparent_sample[2] & 0xff, 0xff);
        }
        else if (m_color_type == COLOR_TYPE_PALETTE)
        {
            if (m_palette.size < u32(size))
            {
                setError("Incorrect alpha palette size.");
                return;
            }

            for (u32 i = 0; i < size; ++i)
            {
                m_palette[i].a = p[i];
            }
        }
        else
        {
            setError("Incorrect color type for alpha palette.");
        }
    }

    void ParserPNG::read_cHRM(BigEndianConstPointer p, u32 size)
    {
        if (size != 32)
        {
            setError("Incorrect cHRM chunk size.");
            return;
        }

        const float scale = 100000.0f;
        m_chromaticity.white.x = p.read32() / scale;
        m_chromaticity.white.y = p.read32() / scale;
        m_chromaticity.red.x   = p.read32() / scale;
        m_chromaticity.red.y   = p.read32() / scale;
        m_chromaticity.green.x = p.read32() / scale;
        m_chromaticity.green.y = p.read32() / scale;
        m_chromaticity.blue.x  = p.read32() / scale;
        m_chromaticity.blue.y  = p.read32() / scale;
    }

    void ParserPNG::read_gAMA(BigEndianConstPointer p, u32 size)
    {
        if (size != 4)
        {
            setError("Incorrect gAMA chunk size.");
            return;
        }

        m_gamma = p.read32() / 100000.0f;
    }

    void ParserPNG::read_sBIT(BigEndianConstPointer p, u32 size)
    {
        if (size > 4)
        {
            setError("Incorrect sBIT chunk size.");
            return;
        }

        for (u32 i = 0; i < size; ++i)
        {
            m_scale_bits[i] = p[i];
        }
    }

    void ParserPNG::read_sRGB(BigEndianConstPointer p, u32 size)
    {
        if (size != 1)
        {
            setError("Incorrect sRGB chunk size.");
            return;
        }

        m_srgb_render_intent = p[0];
    }

    void ParserPNG::read_acTL(BigEndianConstPointer p, u32 size)
    {
        if (size != 8)
        {
            setError("Incorrect acTL chunk size.");
            return;
        }

        m_number_of_frames = p.read32();
        m_repeat_count = p.read32();

        debugPrint("  Frames: %d\n", m_number_of_frames);
        debugPrint("  Repeat: %d %s\n", m_repeat_count, m_repeat_count ? "" : "(infinite)");
    }

    void ParserPNG::read_fcTL(BigEndianConstPointer p, u32 size)
    {
        if (size != 26)
        {
            setError("Incorrect fcTL chunk size.");
            return;
        }

        if (!m_first_frame)
        {
            m_first_frame = p - 8;
        }

        m_frame.read(p);
    }

    void ParserPNG::read_fdAT(BigEndianConstPointer p, u32 size)
    {
        u32 sequence_number = p.read32();
        size -= 4;

        debugPrint("  Sequence: %d\n", sequence_number);
        MANGO_UNREFERENCED_PARAMETER(sequence_number);

        m_compressed.append(p, size);
    }

    ImageHeader ParserPNG::header() const
    {
        ImageHeader header;

        if (!m_error)
        {
            header.width   = m_width;
            header.height  = m_height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = false;
            header.compression = TextureCompression::NONE;

            // force alpha channel on when transparency is enabled
            int color_type = m_color_type;
            if (m_transparent_enable && color_type != COLOR_TYPE_PALETTE)
            {
                color_type |= 4;
            }

            // select decoding format
            switch (color_type)
            {
                case COLOR_TYPE_I:
                    header.format = m_bit_depth <= 8 ?
                        LuminanceFormat(8, Format::UNORM, 8, 0) :
                        LuminanceFormat(16, Format::UNORM, 16, 0);
                    break;

                case COLOR_TYPE_IA:
                    header.format = m_bit_depth <= 8 ?
                        LuminanceFormat(16, Format::UNORM, 8, 8) :
                        LuminanceFormat(32, Format::UNORM, 16, 16);
                    break;

                case COLOR_TYPE_PALETTE:
                    header.palette = true;
                    header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
                    break;

                case COLOR_TYPE_RGB:
                case COLOR_TYPE_RGBA:
                    header.format = m_bit_depth <= 8 ?
                        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8) :
                        Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16); // NOTE: RGBA is not an error here!
                    break;
            }
        }

        return header;
    }

    void ParserPNG::parse()
    {
        BigEndianConstPointer p = m_pointer;

        for ( ; p < m_end - 8; )
        {
            const u32 size = p.read32();
            const u32 id = p.read32();

            const u8* ptr_next_chunk = p + size;
            ptr_next_chunk += 4; // skip crc

            debugPrint("[\"%c%c%c%c\"] %d bytes\n", (id >> 24), (id >> 16), (id >> 8), (id >> 0), size);

            // check that we won't read past end of file
            if (p + size + 4 > m_end)
            {
                setError("Corrupted file.\n");
                return;
            }

            switch (id)
            {
                case u32_mask_rev('I', 'H', 'D', 'R'):
                    setError("File can only have one IHDR chunk.");
                    break;

                case u32_mask_rev('P', 'L', 'T', 'E'):
                    read_PLTE(p, size);
                    break;

                case u32_mask_rev('t', 'R', 'N', 'S'):
                    read_tRNS(p, size);
                    break;

                case u32_mask_rev('g', 'A', 'M', 'A'):
                    read_gAMA(p, size);
                    break;

                case u32_mask_rev('s', 'B', 'I', 'T'):
                    read_sBIT(p, size);
                    break;

                case u32_mask_rev('s', 'R', 'G', 'B'):
                    read_sRGB(p, size);
                    break;

                case u32_mask_rev('c', 'H', 'R', 'M'):
                    read_cHRM(p, size);
                    break;

                case u32_mask_rev('I', 'D', 'A', 'T'):
                    read_IDAT(p, size);
                    if (m_number_of_frames > 0)
                    {
                        m_pointer = ptr_next_chunk;
                        return;
                    }
                    break;

                case u32_mask_rev('a', 'c', 'T', 'L'):
                    read_acTL(p, size);
                    break;

                case u32_mask_rev('f', 'c', 'T', 'L'):
                    read_fcTL(p, size);
                    break;

                case u32_mask_rev('f', 'd', 'A', 'T'):
                    read_fdAT(p, size);
                    m_pointer = ptr_next_chunk;
                    return;

                case u32_mask_rev('p', 'H', 'Y', 's'):
                case u32_mask_rev('b', 'K', 'G', 'D'):
                case u32_mask_rev('z', 'T', 'X', 't'):
                case u32_mask_rev('t', 'E', 'X', 't'):
                case u32_mask_rev('t', 'I', 'M', 'E'):
                case u32_mask_rev('i', 'T', 'X', 't'):
                    // NOTE: ignoring these chunks
                    break;

                case u32_mask_rev('I', 'E', 'N', 'D'):
                    if (m_number_of_frames > 0)
                    {
                        // reset current pointer to first animation frame (for looping)
                        m_pointer = m_first_frame;
                    }
                    else
                    {
                        // terminate parsing
                        m_pointer = m_end;
                    }
                    return;

                default:
                    debugPrint("UNKNOWN CHUNK: [\"%c%c%c%c\"] %d bytes\n", (id >> 24), (id >> 16), (id >> 8), (id >> 0), size);
                    break;
            }

            if (m_error)
            {
                // error occured while reading chunks
                return;
            }

            p = ptr_next_chunk;
        }
    }

    void ParserPNG::filter(u8* buffer, int bytes, int height)
    {
        // zero scanline
        std::vector<u8> zeros(bytes, 0);
        const u8* prev = zeros.data();

        const int bpp = (m_bit_depth < 8) ? 1 : m_channels * m_bit_depth / 8;
        if (bpp > 8)
            return;

        FilterDispatcher dispatcher(bpp);

        u8* s = buffer;

        for (int y = 0; y < height; ++y)
        {
            FilterType method = FilterType(*s++);
            dispatcher.call(method, s, prev, bytes, bpp);
            prev = s;
            s += bytes;
        }
    }

    void ParserPNG::deinterlace1to4(u8* output, int width, int height, int stride, u8* buffer)
    {
        u8* p = buffer;

        const int samples = 8 / m_bit_depth;
        const int mask = samples - 1;
        const int shift = u32_log2(samples);
        const int valueShift = u32_log2(m_bit_depth);
        const int valueMask = (1 << m_bit_depth) - 1;

        for (int pass = 0; pass < 7; ++pass)
        {
            AdamInterleave adam(pass, width, height);
            debugPrint("  pass: %d (%d x %d)\n", pass, adam.w, adam.h);

            const int bw = FILTER_BYTE + ((adam.w + mask) >> shift);
            filter(p, bw - FILTER_BYTE, adam.h);

            if (adam.w && adam.h)
            {
                for (int y = 0; y < adam.h; ++y)
                {
                    const int yoffset = (y << adam.yspc) + adam.yorig;
                    u8* dest = output + yoffset * stride + FILTER_BYTE;
                    u8* src = p + y * bw + FILTER_BYTE;

                    for (int x = 0; x < adam.w; ++x)
                    {
                        const int xoffset = (x << adam.xspc) + adam.xorig;
                        u8 v = src[x >> shift];
                        int a = (mask - (x & mask)) << valueShift;
                        int b = (mask - (xoffset & mask)) << valueShift;
                        v = ((v >> a) & valueMask) << b;
                        dest[xoffset >> shift] |= v;
                    }
                }

                // next pass
                p += bw * adam.h;
            }
        }
    }

    void ParserPNG::deinterlace8to16(u8* output, int width, int height, int stride, u8* buffer)
    {
        u8* p = buffer;
        const int size = getBytesPerLine(width) / width;

        for (int pass = 0; pass < 7; ++pass)
        {
            AdamInterleave adam(pass, width, height);
            debugPrint("  pass: %d (%d x %d)\n", pass, adam.w, adam.h);

            const int bw = FILTER_BYTE + adam.w * size;
            filter(p, bw - FILTER_BYTE, adam.h);

            if (adam.w && adam.h)
            {
                const int ps = adam.w * size + FILTER_BYTE;

                for (int y = 0; y < adam.h; ++y)
                {
                    const int yoffset = (y << adam.yspc) + adam.yorig;
                    u8* dest = output + yoffset * stride + FILTER_BYTE;
                    u8* src = p + y * ps + FILTER_BYTE;

                    dest += adam.xorig * size;
                    const int xmax = (adam.w * size) << adam.xspc;
                    const int xstep = size << adam.xspc;

                    for (int x = 0; x < xmax; x += xstep)
                    {
                        std::memcpy(dest + x, src, size);
                        src += size;
                    }
                }

                // next pass
                p += bw * adam.h;
            }
        }
    }

    void ParserPNG::process_i1to4(u8* dest, int width, int height, int stride, const u8* src)
    {
        const int bits = m_bit_depth;

        const int maxValue = (1 << bits) - 1;
        const int scale = (255 / maxValue) * 0x010101;
        const u32 mask = (1 << bits) - 1;

        for (int y = 0; y < height; ++y)
        {
            u8* d = dest;
            dest += stride;
            ++src; // skip filter byte

            u32 data = 0;
            int offset = -1;

            for (int x = 0; x < width; ++x)
            {
                if (offset < 0)
                {
                    offset = 8 - bits;
                    data = *src++;
                }
                int value = (data >> offset) & mask;
                *d++ = value * scale;
                if (m_transparent_enable)
                {
                    *d++ = value == m_transparent_sample[0] ? 0 : 0xff;
                }
                offset -= bits;
            }
        }
    }

    void ParserPNG::process_i8(u8* dest, int width, int height, int stride, const u8* src)
    {
        if (m_transparent_enable)
        {
            for (int y = 0; y < height; ++y)
            {
                u16* d = reinterpret_cast<u16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    const u16 alpha = m_transparent_sample[0] == src[x] ? 0 : 0xff00;
                    d[x] = alpha | src[x];
                }
                src += width;
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                ++src; // skip filter byte
                std::memcpy(dest, src, width);
                src += width;
                dest += stride;
            }
        }
    }

    void ParserPNG::process_rgb8(u8* dest, int width, int height, int stride, const u8* src)
    {
        if (m_transparent_enable)
        {
            for (int y = 0; y < height; ++y)
            {
                u32* d = reinterpret_cast<u32*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    ColorBGRA color(src[0], src[1], src[2], 0xff);
                    if (color == m_transparent_color)
                    {
                        color.a = 0;
                    }
                    d[x] = color;
                    src += 3;
                }
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                u32* d = reinterpret_cast<u32*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    d[x] = ColorBGRA(src[0], src[1], src[2], 0xff);
                    src += 3;
                }
            }
        }
    }

    void ParserPNG::process_pal1to4(u8* dest, int width, int height, int stride, const u8* src, Palette* ptr_palette)
    {
        const int bits = m_bit_depth;

        const u32 mask = (1 << bits) - 1;

        if (ptr_palette)
        {
            *ptr_palette = m_palette;

            for (int y = 0; y < height; ++y)
            {
                u8* d = reinterpret_cast<u8*>(dest);
                dest += stride;
                ++src; // skip filter byte

                u32 data = 0;
                int offset = -1;

                for (int x = 0; x < width; ++x)
                {
                    if (offset < 0)
                    {
                        offset = 8 - bits;
                        data = *src++;
                    }
                    *d++ = (data >> offset) & mask;
                    offset -= bits;
                }
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                u32* d = reinterpret_cast<u32*>(dest);
                dest += stride;
                ++src; // skip filter byte

                u32 data = 0;
                int offset = -1;

                for (int x = 0; x < width; ++x)
                {
                    if (offset < 0)
                    {
                        offset = 8 - bits;
                        data = *src++;
                    }
                    d[x] = m_palette[(data >> offset) & mask];
                    offset -= bits;
                }
            }
        }
    }

    void ParserPNG::process_pal8(u8* dest, int width, int height, int stride, const u8* src, Palette* ptr_palette)
    {
        if (ptr_palette)
        {
            *ptr_palette = m_palette;

            for (int y = 0; y < height; ++y)
            {
                ++src; // skip filter byte
                std::memcpy(dest, src, width);
                src += width;
                dest += stride;
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                u32* d = reinterpret_cast<u32*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    d[x] = m_palette[*src++];
                }
            }
        }
    }

    void ParserPNG::process_ia8(u8* dest, int width, int height, int stride, const u8* src)
    {
        for (int y = 0; y < height; ++y)
        {
            u16* d = reinterpret_cast<u16*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                *d++ = (src[1] << 8) | src[0];
                src += 2;
            }
        }
    }

    void ParserPNG::process_rgba8(u8* dest, int width, int height, int stride, const u8* src)
    {
        for (int y = 0; y < height; ++y)
        {
            u32* d = reinterpret_cast<u32*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                *d++ = ColorBGRA(src[0], src[1], src[2], src[3]);
                src += 4;
            }
        }
    }

    void ParserPNG::process_i16(u8* dest, int width, int height, int stride, const u8* src)
    {
        if (m_transparent_enable)
        {
            for (int y = 0; y < height; ++y)
            {
                u16* d = reinterpret_cast<u16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    u16 gray = (src[0] << 8) | src[1];
                    u16 alpha = m_transparent_sample[0] == src[0] ? 0 : 0xffff;
                    d[0] = gray;
                    d[1] = alpha;
                    d += 2;
                    src += 2;
                }
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                u16* d = reinterpret_cast<u16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    *d++ = (src[0] << 8) | src[1];
                    src += 2;
                }
            }
        }
    }

    void ParserPNG::process_rgb16(u8* dest, int width, int height, int stride, const u8* src)
    {
        if (m_transparent_enable)
        {
            for (int y = 0; y < height; ++y)
            {
                u16* d = reinterpret_cast<u16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    u16 red   = (src[0] << 8) | src[1];
                    u16 green = (src[2] << 8) | src[3];
                    u16 blue  = (src[4] << 8) | src[5];
                    u16 alpha = 0xffff;
                    if (m_transparent_sample[0] == red &&
                        m_transparent_sample[1] == green &&
                        m_transparent_sample[1] == blue) alpha = 0;
                    d[0] = red;
                    d[1] = green;
                    d[2] = blue;
                    d[3] = alpha;
                    d += 4;
                    src += 6;
                }
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                u16* d = reinterpret_cast<u16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    u16 red   = (src[0] << 8) | src[1];
                    u16 green = (src[2] << 8) | src[3];
                    u16 blue  = (src[4] << 8) | src[5];
                    u16 alpha = 0xffff;
                    d[0] = red;
                    d[1] = green;
                    d[2] = blue;
                    d[3] = alpha;
                    d += 4;
                    src += 6;
                }
            }
        }
    }

    void ParserPNG::process_ia16(u8* dest, int width, int height, int stride, const u8* src)
    {
        for (int y = 0; y < height; ++y)
        {
            u16* d = reinterpret_cast<u16*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                u16 gray  = (src[0] << 8) | src[1];
                u16 alpha = (src[2] << 8) | src[3];
                d[0] = gray;
                d[1] = alpha;
                d += 2;
                src += 4;
            }
        }
    }

    void ParserPNG::process_rgba16(u8* dest, int width, int height, int stride, const u8* src)
    {
        for (int y = 0; y < height; ++y)
        {
            u16* d = reinterpret_cast<u16*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                u16 red   = (src[0] << 8) | src[1];
                u16 green = (src[2] << 8) | src[3];
                u16 blue  = (src[4] << 8) | src[5];
                u16 alpha = (src[6] << 8) | src[7];
                d[0] = blue;
                d[1] = green;
                d[2] = red;
                d[3] = alpha;
                d += 4;
                src += 8;
            }
        }
    }

    void ParserPNG::process(u8* image, int width, int height, int stride, u8* buffer, Palette* ptr_palette)
    {
        Buffer temp;

        if (m_interlace)
        {
            const int stride = FILTER_BYTE + getBytesPerLine(width);

            temp.resize(height * stride);
            std::memset(temp, 0, height * stride);

            // deinterlace does filter for each pass
            if (m_bit_depth < 8)
                deinterlace1to4(temp, width, height, stride, buffer);
            else
                deinterlace8to16(temp, width, height, stride, buffer);

            // use de-interlaced temp buffer as processing source
            buffer = temp;
        }
        else
        {
            // filter the whole image in single pass
            filter(buffer, getBytesPerLine(width), height);
        }

        if (m_error)
        {
            return;
        }

        if (m_color_type == COLOR_TYPE_I)
        {
            if (m_bit_depth < 8)
                process_i1to4(image, width, height, stride, buffer);
            else if (m_bit_depth == 8)
                process_i8(image, width, height, stride, buffer);
            else
                process_i16(image, width, height, stride, buffer);
        }
        else if (m_color_type == COLOR_TYPE_RGB)
        {
            if (m_bit_depth == 8)
                process_rgb8(image, width, height, stride, buffer);
            else
                process_rgb16(image, width, height, stride, buffer);
        }
        else if (m_color_type == COLOR_TYPE_PALETTE)
        {
            if (m_bit_depth < 8)
                process_pal1to4(image, width, height, stride, buffer, ptr_palette);
            else
                process_pal8(image, width, height, stride, buffer, ptr_palette);
        }
        else if (m_color_type == COLOR_TYPE_IA)
        {
            if (m_bit_depth == 8)
                process_ia8(image, width, height, stride, buffer);
            else
                process_ia16(image, width, height, stride, buffer);
        }
        else if (m_color_type == COLOR_TYPE_RGBA)
        {
            if (m_bit_depth == 8)
                process_rgba8(image, width, height, stride, buffer);
            else
                process_rgba16(image, width, height, stride, buffer);
        }
    }

    void ParserPNG::blend_ia8(u8* dest, const u8* src, int width)
    {
        for (int x = 0; x < width; ++x)
        {
            u8 alpha = src[1];
            u8 invalpha = 255 - alpha;
            dest[0] = src[0] + ((dest[0] - src[0]) * invalpha) / 255;
            dest[1] = alpha;
            src += 2;
            dest += 2;
        }
    }

    void ParserPNG::blend_ia16(u8* dest8, const u8* src8, int width)
    {
        u16* dest = reinterpret_cast<u16*>(dest8);
        const u16* src = reinterpret_cast<const u16*>(src8);
        for (int x = 0; x < width; ++x)
        {
            u16 alpha = src[1];
            u16 invalpha = 255 - alpha;
            dest[0] = src[0] + ((dest[0] - src[0]) * invalpha) / 255;
            dest[1] = alpha;
            src += 2;
            dest += 2;
        }
    }

    void ParserPNG::blend_bgra8(u8* dest, const u8* src, int width)
    {
        for (int x = 0; x < width; ++x)
        {
            u8 alpha = src[3];
            u8 invalpha = 255 - alpha;
            dest[0] = src[0] + ((dest[0] - src[0]) * invalpha) / 255;
            dest[1] = src[1] + ((dest[1] - src[1]) * invalpha) / 255;
            dest[2] = src[2] + ((dest[2] - src[2]) * invalpha) / 255;
            dest[3] = alpha;
            src += 4;
            dest += 4;
        }
    }

    void ParserPNG::blend_rgba16(u8* dest8, const u8* src8, int width)
    {
        u16* dest = reinterpret_cast<u16*>(dest8);
        const u16* src = reinterpret_cast<const u16*>(src8);
        for (int x = 0; x < width; ++x)
        {
            u16 alpha = src[3];
            u16 invalpha = 255 - alpha;
            dest[0] = src[0] + ((dest[0] - src[0]) * invalpha) / 65535;
            dest[1] = src[1] + ((dest[1] - src[1]) * invalpha) / 65535;
            dest[2] = src[2] + ((dest[2] - src[2]) * invalpha) / 65535;
            dest[3] = alpha;
            src += 4;
            dest += 4;
        }
    }

    void ParserPNG::blend_indexed(u8* dest, const u8* src, int width)
    {
        for (int x = 0; x < width; ++x)
        {
            u8 sample = src[x];
            ColorBGRA color = m_palette[sample];
            if (color.a)
            {
                dest[x] = sample;
            }
        }
    }

    void ParserPNG::blend(Surface& d, Surface& s, Palette* palette)
    {
        int width = s.width;
        int height = s.height;
        if (m_frame.blend == Frame::SOURCE || !s.format.isAlpha())
        {
            d.blit(0, 0, s);
        }
        else if (m_frame.blend == Frame::OVER)
        {
            for (int y = 0; y < height; ++y)
            {
                const u8* src = s.address(0, y);
                u8* dest = d.address(0, y);

                if (palette)
                {
                    // palette extract mode: blend with the main image palette
                    blend_indexed(dest, src, width);
                }
                else
                {
                    switch (s.format.bits)
                    {
                        case 8:
                            blend_ia8(dest, src, width);
                            break;
                        case 16:
                            blend_ia16(dest, src, width);
                            break;
                        case 32:
                            blend_bgra8(dest, src, width);
                            break;
                        case 64:
                            blend_rgba16(dest, src, width);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }

    int ParserPNG::getImageBufferSize(int width, int height) const
    {
        int buffer_size = 0;

        if (m_interlace)
        {
            // NOTE: brute-force loop to resolve memory consumption
            for (int pass = 0; pass < 7; ++pass)
            {
                AdamInterleave adam(pass, width, height);
                if (adam.w && adam.h)
                {
                    buffer_size += (FILTER_BYTE + getBytesPerLine(adam.w)) * adam.h;
                }
            }
        }
        else
        {
            buffer_size = (FILTER_BYTE + getBytesPerLine(width)) * height;
        }

        return buffer_size;
    }

    const char* ParserPNG::decode(Surface& dest, Palette* ptr_palette)
    {
        m_compressed.reset();

        parse();

        if (!m_compressed.size())
        {
            return m_error;
        }

        // default: main image from "IHDR" chunk
        int width = m_width;
        int height = m_height;
        int stride = dest.stride;
        u8* image = dest.image;

        std::unique_ptr<u8[]> framebuffer;

        // override with animation frame
        if (m_number_of_frames > 0)
        {
            width = m_frame.width;
            height = m_frame.height;
            stride = width * dest.format.bytes();

            // decode frame into temporary buffer (for composition)
            image = new u8[stride * height];
            framebuffer.reset(image);
        }

        // decompression
        // - use STB for small buffers
        // - use MINIZ for large buffers

        int buffer_size = getImageBufferSize(width, height);

        if (m_compressed.size() <= 128 * 1024)
        {
            Memory mem = m_compressed;
            int raw_len = buffer_size;
            u8 *buffer = (u8*) stbi_zlib_decode_malloc_guesssize_headerflag(
                reinterpret_cast<const char *>(mem.address),
                int(mem.size),
                raw_len,
                &raw_len,
                1);
            if (buffer)
            {
                debugPrint("  # total_out: %d \n", raw_len);

                // process image
                process(image, width, height, stride, buffer, ptr_palette);
                STBI_FREE(buffer);
            }
        }
        else
        {
            // allocate output buffer
            debugPrint("  buffer bytes: %d\n", buffer_size);
            Buffer buffer(buffer_size);

            // decompress stream
            mz_stream stream;
            int status;
            memset(&stream, 0, sizeof(stream));

            stream.next_in   = m_compressed;
            stream.avail_in  = (unsigned int)m_compressed.size();
            stream.next_out  = buffer;
            stream.avail_out = (unsigned int)buffer_size;

            status = mz_inflateInit(&stream);
            if (status != MZ_OK)
            {
                // TODO: error
            }

            status = mz_inflate(&stream, MZ_FINISH);
            if (status != MZ_STREAM_END)
            {
                // TODO: error
            }

            debugPrint("  # total_out: %d \n", int(stream.total_out));
            status = mz_inflateEnd(&stream);

            // process image
            process(image, width, height, stride, buffer, ptr_palette);
        }

        if (m_number_of_frames > 0)
        {
            Surface d(dest, m_frame.xoffset, m_frame.yoffset, width, height);
            Surface s(width, height, dest.format, stride, image);
            blend(d, s, ptr_palette);
        }

        return m_error;
    }

    // ------------------------------------------------------------
    // writePNG()
    // ------------------------------------------------------------

    void writeChunk(Stream& stream, u32 chunkid, Memory memory)
    {
        BigEndianStream s(stream);

        u8 temp[4];
        ustore32be(temp, chunkid);
        u32 crc = crc32(0, Memory(temp, 4));
        crc = crc32(crc, memory);

        s.write32(u32(memory.size));
        s.write32(chunkid);
        s.write(memory);
        s.write32(crc);
    }

    void write_IHDR(Stream& stream, const Surface& surface, u8 color_bits, ColorType color_type)
    {
        MemoryStream buffer;
        BigEndianStream s(buffer);

        s.write32(surface.width);
        s.write32(surface.height);
        s.write8(color_bits);
        s.write8(color_type);
        s.write8(0); // compression
        s.write8(0); // filter
        s.write8(0); // interlace

        writeChunk(stream, u32_mask_rev('I', 'H', 'D', 'R'), buffer);
    }

    void write_IDAT(Stream& stream, const Surface& surface)
    {
        const int bytesPerLine = surface.width * surface.format.bytes();
        const int bytes = (FILTER_BYTE + bytesPerLine) * surface.height;

        z_stream z = { 0 };
        deflateInit(&z, -1);

        z.avail_out = (unsigned int)deflateBound(&z, bytes);
        Buffer buffer(z.avail_out);

        z.next_out = buffer;

        for (int y = 0; y < surface.height; ++y)
        {
            bool last_scan = (y == surface.height - 1);

            // compress filler byte
            u8 zero = 0;
            z.avail_in = 1;
            z.next_in = &zero;
            deflate(&z, Z_NO_FLUSH);

            // compress scanline
            z.avail_in = bytesPerLine;
            z.next_in = surface.address<u8>(0, y);
            deflate(&z, last_scan ? Z_FINISH : Z_NO_FLUSH);
        }

        // compressed size includes chunkID
        const size_t compressed_size = int(z.next_out - buffer);

        deflateEnd(&z);

        // write chunkdID + compressed data
        writeChunk(stream, u32_mask_rev('I', 'D', 'A', 'T'), Memory(buffer, compressed_size));
    }

    void writePNG(Stream& stream, const Surface& surface, u8 color_bits, ColorType color_type)
    {
        static const u8 magic[] =
        {
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
        };

        BigEndianStream s(stream);

        // write magic
        s.write(magic, 8);

        write_IHDR(stream, surface, color_bits, color_type);
        write_IDAT(stream, surface);

        // write IEND
        s.write32(0);
        s.write32(0x49454e44);
        s.write32(0xae426082);
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ParserPNG m_parser;
        ImageHeader m_header;

        Interface(Memory memory)
            : m_parser(memory)
        {
            m_header = m_parser.header();

            const char* error = m_parser.getError();
            if (error)
            {
                debugPrint("HEADER ERROR: %s\n", error);
            }
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        void decode(Surface& dest, Palette* ptr_palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            if (m_parser.getError())
            {
                // any internal errors in previous decoding / parsin passes
                // and we're out of here
                return;
            }

            const char* error = nullptr;

            if (dest.format == m_header.format &&
                dest.width >= m_header.width &&
                dest.height >= m_header.height &&
                !ptr_palette)
            {
                // direct decoding
                error = m_parser.decode(dest, nullptr);
            }
            else
            {
                if (ptr_palette && m_header.palette)
                {
                    // direct decoding with palette
                    error = m_parser.decode(dest, ptr_palette);
                }
                else
                {
                    // indirect
                    Bitmap temp(m_header.width, m_header.height, m_header.format);
                    error = m_parser.decode(temp, nullptr);
                    dest.blit(0, 0, temp);
                }
            }

            if (error)
            {
                debugPrint("DECODE ERROR: %s\n", error);
            }
        }
    };

    ImageDecoderInterface* createInterface(Memory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    void imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED_PARAMETER(options);

        // defaults
        u8 color_bits = 8;
        ColorType color_type = COLOR_TYPE_RGBA;
        Format format;

        // select png format
        if (surface.format.isLuminance())
        {
            if (surface.format.isAlpha())
            {
                color_type = COLOR_TYPE_IA;

                if (surface.format.size[0] > 8)
                {
                    color_bits = 16;
                    format = LuminanceFormat(32, Format::UNORM, 16, 16);
                }
                else
                {
                    color_bits = 8;
                    format = LuminanceFormat(16, Format::UNORM, 8, 8);
                }
            }
            else
            {
                color_type = COLOR_TYPE_I;

                if (surface.format.size[0] > 8)
                {
                    color_bits = 16;
                    format = LuminanceFormat(16, Format::UNORM, 16, 0);
                }
                else
                {
                    color_bits = 8;
                    format = LuminanceFormat(8, Format::UNORM, 8, 0);
                }
            }
        }
        else
        {
            // always encode alpha in non-luminance formats
            color_type = COLOR_TYPE_RGBA;

            if (surface.format.size[0] > 8)
            {
                color_bits = 16;
                format = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
            }
            else
            {
                color_bits = 8;
                format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
        }

        if (surface.format == format)
        {
            writePNG(stream, surface, color_bits, color_type);
        }
        else
        {
            Bitmap temp(surface.width, surface.height, format);
            temp.blit(0, 0, surface);
            writePNG(stream, temp, color_bits, color_type);
        }
    }

} // namespace

namespace mango
{

    void registerImageDecoderPNG()
    {
        registerImageDecoder(createInterface, ".png");
        registerImageEncoder(imageEncode, ".png");
    }

} // namespace mango
