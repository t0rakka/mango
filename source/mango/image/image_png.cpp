/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
//#define MANGO_ENABLE_DEBUG_PRINT

#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>

#ifdef MANGO_ENABLE_IMAGE_PNG

// https://www.w3.org/TR/2003/REC-PNG-20031110/
// https://wiki.mozilla.org/APNG_Specification

// TODO: discard modes (requires us to keep a copy of main image)
// TODO: check that animations starting with "IDAT" and "fdAT" work correctly
// TODO: SIMD blending (not critical)

// TODO: process at the same time we are inflating
// TODO: SIMD color conversions

// ------------------------------------------------------------
// libdeflate
// ------------------------------------------------------------

#include "../../external/libdeflate/libdeflate.h"

namespace
{
    using namespace mango;

    constexpr int PNG_SIMD_PADDING = 16;
    constexpr int PNG_FILTER_BYTE = 1;
    constexpr u64 PNG_HEADER_MAGIC = 0x89504e470d0a1a0a;

    enum ColorType
    {
        COLOR_TYPE_I       = 0,
        COLOR_TYPE_RGB     = 2,
        COLOR_TYPE_PALETTE = 3,
        COLOR_TYPE_IA      = 4,
        COLOR_TYPE_RGBA    = 6,
    };

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

    using FilterFunc = void (*)(u8* scan, const u8* prev, int bytes, int bpp);

    void filter_sub(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(prev);

        for (int x = bpp; x < bytes; ++x)
        {
            scan[x] += scan[x - bpp];
        }
    }

    void filter_up(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

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

    // helper functions

    inline __m128i load4(const void* p)
    {
        u32 temp = uload32(p);
        return _mm_cvtsi32_si128(temp);
    }

    inline void store4(void* p, __m128i v)
    {
        u32 temp = _mm_cvtsi128_si32(v);
        ustore32(p, temp);
    }

    inline __m128i load3(const void* p)
    {
#if 1
        // NOTE: we have PNG_SIMD_PADDING for overflow guardband
        u32 temp = uload32(p);
#else
        u32 temp = 0;
        std::memcpy(&temp, p, 3);
#endif
        return _mm_cvtsi32_si128(temp);
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

    // filters

    void filter_sub_24bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(prev);
        MANGO_UNREFERENCED(bpp);

        __m128i d = _mm_setzero_si128();

        for (int x = 0; x < bytes; x += 3)
        {
            d = _mm_add_epi8(load3(scan + x), d);
            store3(scan + x, d);
        }
    }

    void filter_sub_32bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(prev);
        MANGO_UNREFERENCED(bpp);

        __m128i d = _mm_setzero_si128();

        for (int x = 0; x < bytes; x += 4)
        {
            d = _mm_add_epi8(load4(scan + x), d);
            store4(scan + x, d);
        }
    }

    void filter_up_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

        while (bytes >= 16)
        {
            __m128i a = _mm_loadu_si128(reinterpret_cast<__m128i*>(scan));
            __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(prev));
            a = _mm_add_epi8(a, b);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(scan), a);
            scan += 16;
            prev += 16;
            bytes -= 16;
        }

        for (int x = 0; x < bytes; ++x)
        {
            scan[x] += prev[x];
        }
    }

    void filter_average_24bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

        __m128i d = _mm_setzero_si128();

        for (int x = 0; x < bytes; x += 3)
        {
            d = average_sse2(d, load3(prev + x), load3(scan + x));
            store3(scan + x, d);
        }
    }

    void filter_average_32bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

        __m128i d = _mm_setzero_si128();

        for (int x = 0; x < bytes; x += 4)
        {
            d = average_sse2(d, load4(prev + x), load4(scan + x));
            store4(scan + x, d);
        }
    }

    void filter_paeth_24bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

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
        MANGO_UNREFERENCED(bpp);

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
        MANGO_UNREFERENCED(bpp);

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
        int bpp = 0;

        FilterDispatcher(int bpp)
            : bpp(bpp)
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

#if defined(MANGO_ENABLE_SSE2)
            up = filter_up_sse2;
#endif
#if defined(MANGO_ENABLE_NEON__todo)
            up = filter_up_neon;
#endif
        }

        void operator () (u8* scan, const u8* prev, int bytes)
        {
            FilterType method = FilterType(scan[0]);
            //printf("%d", method);

            // skip filter byte
            ++scan;
            ++prev;
            --bytes;

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
    // color conversion
    // ------------------------------------------------------------

    struct ColorState
    {
        using Function = void (*)(const ColorState& state, int width, u8* dst, const u8* src);

        Function func = nullptr;
        int bits;

        // tRNS
        bool transparent_enable = false;
        u16 transparent_sample[3];
        ColorRGBA transparent_color;

        ColorBGRA* palette;
    };

    void process_pal1to4(const ColorState& state, int width, u8* dst, const u8* src)
    {
        ColorBGRA* palette = state.palette;

        const int bits = state.bits;
        const u32 mask = (1 << bits) - 1;

        if (palette)
        {
            u32* dest = reinterpret_cast<u32*>(dst);

            u32 data = 0;
            int offset = -1;

            for (int x = 0; x < width; ++x)
            {
                if (offset < 0)
                {
                    offset = 8 - bits;
                    data = *src++;
                }

                dest[x] = palette[(data >> offset) & mask];
                offset -= bits;
            }
        }
        else
        {
            u8* dest = dst;

            u32 data = 0;
            int offset = -1;

            for (int x = 0; x < width; ++x)
            {
                if (offset < 0)
                {
                    offset = 8 - bits;
                    data = *src++;
                }

                *dest++ = (data >> offset) & mask;
                offset -= bits;
            }
        }
    }

    void process_pal8(const ColorState& state, int width, u8* dst, const u8* src)
    {
        ColorBGRA* palette = state.palette;

        if (palette)
        {
            u32* dest = reinterpret_cast<u32*>(dst);

            for (int x = 0; x < width; ++x)
            {
                dest[x] = palette[src[x]];
            }
        }
        else
        {
            std::memcpy(dst, src, width);
        }
    }

    void process_i1to4(const ColorState& state, int width, u8* dst, const u8* src)
    {
        u8* dest = dst;

        const bool transparent_enable = state.transparent_enable;
        const u16 transparent_sample = state.transparent_sample[0];

        const int bits = state.bits;
        const int maxValue = (1 << bits) - 1;
        const int scale = (255 / maxValue) * 0x010101;
        const u32 mask = (1 << bits) - 1;

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
            *dest++ = value * scale;

            if (transparent_enable)
            {
                *dest++ = (value == transparent_sample) ? 0 : 0xff;
            }

            offset -= bits;
        }
    }

    void process_i8(const ColorState& state, int width, u8* dst, const u8* src)
    {
        u16* dest = reinterpret_cast<u16*>(dst);

        if (state.transparent_enable)
        {
            const u16 transparent_sample = state.transparent_sample[0];

            for (int x = 0; x < width; ++x)
            {
                const u16 alpha = transparent_sample == src[x] ? 0 : 0xff00;
                dest[x] = alpha | src[x];
            }
        }
        else
        {
            std::memcpy(dest, src, width);
        }
    }

    void process_i16(const ColorState& state, int width, u8* dst, const u8* src)
    {
        u16* dest = reinterpret_cast<u16*>(dst);

        if (state.transparent_enable)
        {
            const u16 transparent_sample = state.transparent_sample[0];

            for (int x = 0; x < width; ++x)
            {
                u16 gray = uload16be(src);
                u16 alpha = (transparent_sample == gray) ? 0 : 0xffff;
                dest[0] = gray;
                dest[1] = alpha;
                dest += 2;
                src += 2;
            }
        }
        else
        {
            for (int x = 0; x < width; ++x)
            {
                dest[x] = uload16be(src);
                src += 2;
            }
        }
    }

    void process_ia8(const ColorState& state, int width, u8* dst, const u8* src)
    {
        u16* dest = reinterpret_cast<u16*>(dst);

        for (int x = 0; x < width; ++x)
        {
            dest[x] = uload16be(src);
            src += 2;
        }
    }

    void process_ia16(const ColorState& state, int width, u8* dst, const u8* src)
    {
        u16* dest = reinterpret_cast<u16*>(dst);

        for (int x = 0; x < width; ++x)
        {
            dest[0] = uload16be(src + 0);
            dest[1] = uload16be(src + 2);
            dest += 2;
            src += 4;
        }
    }

    void process_rgb8(const ColorState& state, int width, u8* dst, const u8* src)
    {
        u32* dest = reinterpret_cast<u32*>(dst);

        if (state.transparent_enable)
        {
            const ColorRGBA transparent_color = state.transparent_color;

            for (int x = 0; x < width; ++x)
            {
                ColorRGBA color(src[0], src[1], src[2], 0xff);
                if (color == transparent_color)
                {
                    color.a = 0;
                }
                dest[x] = color;
                src += 3;
            }
        }
        else
        {
#if 1
            while (width >= 4)
            {
                u32 v0 = uload32(src + 0); // r1 b0 g0 r0
                u32 v1 = uload32(src + 4); // g2 r2 b1 g1
                u32 v2 = uload32(src + 8); // b3 g3 r3 b2
                dest[0] = 0xff000000 | v0;
                dest[1] = 0xff000000 | (v0 >> 24) | (v1 << 8);
                dest[2] = 0xff000000 | (v1 >> 16) | (v2 << 16);
                dest[3] = 0xff000000 | v2 >> 8;
                src += 12;
                dest += 4;
                width -= 4;
            }
#endif
            for (int x = 0; x < width; ++x)
            {
                dest[x] = ColorRGBA(src[0], src[1], src[2], 0xff);
                src += 3;
            }
        }
    }

    void process_rgb16(const ColorState& state, int width, u8* dst, const u8* src)
    {
        u16* dest = reinterpret_cast<u16*>(dst);

        if (state.transparent_enable)
        {
            const u16 transparent_sample0 = state.transparent_sample[0];
            const u16 transparent_sample1 = state.transparent_sample[1];
            const u16 transparent_sample2 = state.transparent_sample[2];

            for (int x = 0; x < width; ++x)
            {
                u16 red   = uload16be(src + 0);
                u16 green = uload16be(src + 2);
                u16 blue  = uload16be(src + 4);
                u16 alpha = 0xffff;
                if (transparent_sample0 == red &&
                    transparent_sample1 == green &&
                    transparent_sample2 == blue) alpha = 0;
                dest[0] = red;
                dest[1] = green;
                dest[2] = blue;
                dest[3] = alpha;
                dest += 4;
                src += 6;
            }
        }
        else
        {
            for (int x = 0; x < width; ++x)
            {
                dest[0] = uload16be(src + 0);
                dest[1] = uload16be(src + 2);
                dest[2] = uload16be(src + 4);
                dest[3] = 0xffff;
                dest += 4;
                src += 6;
            }
        }
    }

    void process_rgba8(const ColorState& state, int width, u8* dst, const u8* src)
    {
        std::memcpy(dst, src, width * 4);
    }

    void process_rgba16(const ColorState& state, int width, u8* dst, const u8* src)
    {
        u16* dest = reinterpret_cast<u16*>(dst);

        for (int x = 0; x < width; ++x)
        {
            dest[0] = uload16be(src + 0);
            dest[1] = uload16be(src + 2);
            dest[2] = uload16be(src + 4);
            dest[3] = uload16be(src + 6);
            dest += 4;
            src += 8;
        }
    }

    ColorState::Function getColorFunction(int color_type, int bit_depth)
    {
        ColorState::Function function = nullptr;

        if (color_type == COLOR_TYPE_PALETTE)
        {
            if (bit_depth < 8)
                function = process_pal1to4;
            else
                function = process_pal8;
        }
        else if (color_type == COLOR_TYPE_I)
        {
            if (bit_depth < 8)
                function = process_i1to4;
            else if (bit_depth == 8)
                function = process_i8;
            else
                function = process_i16;
        }
        else if (color_type == COLOR_TYPE_IA)
        {
            if (bit_depth == 8)
                function = process_ia8;
            else
                function = process_ia16;
        }
        else if (color_type == COLOR_TYPE_RGB)
        {
            if (bit_depth == 8)
                function = process_rgb8;
            else
                function = process_rgb16;
        }
        else if (color_type == COLOR_TYPE_RGBA)
        {
            if (bit_depth == 8)
                function = process_rgba8;
            else
                function = process_rgba16;
        }

        return function;
    }

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
        ConstMemory m_memory;
        ImageHeader m_header;

        const u8* m_pointer = nullptr;
        const u8* m_end = nullptr;
        const char* m_error = nullptr;

        Buffer m_compressed;

        ColorState m_color_state;

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
        u32 m_current_frame_index = 0;
        u32 m_next_frame_index = 0;

        // fcTL
        Frame m_frame;
        const u8* m_first_frame = nullptr;

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

        void blend_ia8      (u8* dest, const u8* src, int width);
        void blend_ia16     (u8* dest, const u8* src, int width);
        void blend_bgra8    (u8* dest, const u8* src, int width);
        void blend_rgba16   (u8* dest, const u8* src, int width);
        void blend_indexed  (u8* dest, const u8* src, int width);

        void deinterlace1to4(u8* output, int width, int height, int stride, u8* buffer);
        void deinterlace8to16(u8* output, int width, int height, int stride, u8* buffer);
        void filter(u8* buffer, int bytes, int height);
        void process(u8* dest, int width, int height, int stride, u8* buffer);

        void blend(Surface& d, Surface& s, Palette* palette);

        int getBytesPerLine(int width) const
        {
            return m_channels * ((m_bit_depth * width + 7) / 8);
        }

        void setError(const std::string& error)
        {
            m_header.info = "[ImageDecoder.PNG] ";
            m_header.info += error;
            m_header.success = false;
        }

        int getImageBufferSize(int width, int height) const;

    public:
        ParserPNG(ConstMemory memory);
        ~ParserPNG();

        const ImageHeader& getHeader();
        ImageDecodeStatus decode(Surface& dest, Palette* palette);
    };

    // ------------------------------------------------------------
    // ParserPNG
    // ------------------------------------------------------------

    ParserPNG::ParserPNG(ConstMemory memory)
        : m_memory(memory)
        , m_end(memory.address + memory.size)
    {
        BigEndianConstPointer p = memory.address;

        // read header magic
        const u64 magic = p.read64();
        if (magic != PNG_HEADER_MAGIC)
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

    const ImageHeader& ParserPNG::getHeader()
    {
        if (m_header.success)
        {
            m_header.width   = m_width;
            m_header.height  = m_height;
            m_header.depth   = 0;
            m_header.levels  = 0;
            m_header.faces   = 0;
			m_header.palette = false;
            m_header.compression = TextureCompression::NONE;

            // force alpha channel on when transparency is enabled
            int color_type = m_color_type;
            if (m_color_state.transparent_enable && color_type != COLOR_TYPE_PALETTE)
            {
                color_type |= 4;
            }

            // select decoding format
            int bits = m_bit_depth <= 8 ? 8 : 16;
            switch (color_type)
            {
                case COLOR_TYPE_I:
                    m_header.format = LuminanceFormat(bits, Format::UNORM, bits, 0);
                    break;

                case COLOR_TYPE_IA:
                    m_header.format = LuminanceFormat(bits * 2, Format::UNORM, bits, bits);
                    break;

                case COLOR_TYPE_PALETTE:
                    // NOTE: palette formats decode to BGRA (same format as the palette)
                    m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
                    m_header.palette = true;
                    break;

                case COLOR_TYPE_RGB:
                case COLOR_TYPE_RGBA:
                    m_header.format = Format(bits * 4, Format::UNORM, Format::RGBA, bits, bits, bits, bits);
                    break;
            }
        }

        return m_header;
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
                    m_color_state.transparent_enable = true;
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

        m_color_state.bits = m_bit_depth;
        m_color_state.func = getColorFunction(m_color_type, m_bit_depth);

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

            m_color_state.transparent_enable = true;
            m_color_state.transparent_sample[0] = p.read16();
        }
        else if (m_color_type == COLOR_TYPE_RGB)
        {
            if (size != 6)
            {
                setError("Incorrect tRNS chunk size.");
                return;
            }

            m_color_state.transparent_enable = true;
            m_color_state.transparent_sample[0] = p.read16();
            m_color_state.transparent_sample[1] = p.read16();
            m_color_state.transparent_sample[2] = p.read16();
            m_color_state.transparent_color = ColorRGBA(m_color_state.transparent_sample[0] & 0xff,
                                                        m_color_state.transparent_sample[1] & 0xff,
                                                        m_color_state.transparent_sample[2] & 0xff, 0xff);
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
        MANGO_UNREFERENCED(sequence_number);

        m_compressed.append(p, size);
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
                        ptr_next_chunk = m_first_frame;
                    }
                    else
                    {
                        // terminate parsing
                        m_pointer = m_end;
                        return;
                    }
                    break;

                default:
                    debugPrint("  # UNKNOWN: [\"%c%c%c%c\"] %d bytes\n", (id >> 24), (id >> 16), (id >> 8), (id >> 0), size);
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
                    buffer_size += (PNG_FILTER_BYTE + getBytesPerLine(adam.w)) * adam.h;
                }
            }
        }
        else
        {
            buffer_size = (PNG_FILTER_BYTE + getBytesPerLine(width)) * height;
        }

        return buffer_size;
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

            const int bw = PNG_FILTER_BYTE + ((adam.w + mask) >> shift);
            filter(p, bw, adam.h);

            if (adam.w && adam.h)
            {
                for (int y = 0; y < adam.h; ++y)
                {
                    const int yoffset = (y << adam.yspc) + adam.yorig;
                    u8* dest = output + yoffset * stride + PNG_FILTER_BYTE;
                    u8* src = p + y * bw + PNG_FILTER_BYTE;

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

            const int bw = PNG_FILTER_BYTE + adam.w * size;
            filter(p, bw, adam.h);

            if (adam.w && adam.h)
            {
                const int ps = adam.w * size + PNG_FILTER_BYTE;

                for (int y = 0; y < adam.h; ++y)
                {
                    const int yoffset = (y << adam.yspc) + adam.yorig;
                    u8* dest = output + yoffset * stride + PNG_FILTER_BYTE;
                    u8* src = p + y * ps + PNG_FILTER_BYTE;

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

    void ParserPNG::filter(u8* buffer, int bytes, int height)
    {
        const int bpp = (m_bit_depth < 8) ? 1 : m_channels * m_bit_depth / 8;
        if (bpp > 8)
            return;

        FilterDispatcher dispatcher(bpp);

        // zero scanline
        std::vector<u8> zeros(bytes, 0);
        const u8* prev = zeros.data();

        for (int y = 0; y < height; ++y)
        {
            dispatcher(buffer, prev, bytes);
            prev = buffer;
            buffer += bytes;
        }
    }

    void ParserPNG::process(u8* image, int width, int height, int stride, u8* buffer)
    {
        if (m_error)
        {
            return;
        }

        int bytes_per_line = getBytesPerLine(width) + PNG_FILTER_BYTE;

        Buffer temp;

        if (m_interlace)
        {
            temp.resize(height * bytes_per_line);
            std::memset(temp, 0, height * bytes_per_line);

            // deinterlace does filter for each pass
            if (m_bit_depth < 8)
                deinterlace1to4(temp, width, height, bytes_per_line, buffer);
            else
                deinterlace8to16(temp, width, height, bytes_per_line, buffer);

            // use de-interlaced temp buffer as processing source
            buffer = temp;

            // color conversion
            for (int y = 0; y < height; ++y)
            {
                m_color_state.func(m_color_state, width, image, buffer + 1);
                image += stride;
                buffer += bytes_per_line;
            }
        }
        else
        {
            const int bpp = (m_bit_depth < 8) ? 1 : m_channels * m_bit_depth / 8;
            if (bpp > 8)
                return;

            FilterDispatcher dispatcher(bpp);

            // zero scanline
            std::vector<u8> zeros(bytes_per_line, 0);
            const u8* prev = zeros.data();

            for (int y = 0; y < height; ++y)
            {
                // filtering
                dispatcher(buffer, prev, bytes_per_line);

                // color conversion
                m_color_state.func(m_color_state, width, image, buffer + PNG_FILTER_BYTE);
                image += stride;

                prev = buffer;
                buffer += bytes_per_line;
            }
        }
    }

    ImageDecodeStatus ParserPNG::decode(Surface& dest, Palette* ptr_palette)
    {
        ImageDecodeStatus status;

        m_compressed.reset();

        parse();

        if (!m_compressed.size())
        {
            setError("No compressed data.");
            return status;
        }

        if (!m_header.success)
        {
            status.setError(m_header.info);
            return status;
        }

        if (ptr_palette)
        {
            // caller requests palette; give it and decode u8 indices
            *ptr_palette = m_palette;
            m_color_state.palette = nullptr;
        }
        else
        {
            // caller doesn't want palette; lookup RGBA colors from palette
            m_color_state.palette = m_palette.color;
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

            // compute frame indices (for external users)
            m_current_frame_index = m_next_frame_index++;
            if (m_next_frame_index >= m_number_of_frames)
            {
                m_next_frame_index = 0;
            }
        }

        int buffer_size = getImageBufferSize(width, height);

        // allocate output buffer
        debugPrint("  buffer bytes: %d\n", buffer_size);
        Buffer buffer(buffer_size + PNG_SIMD_PADDING);

        libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();

        Memory memory = m_compressed;

        size_t bytes_out = 0;
        libdeflate_result result = libdeflate_zlib_decompress(decompressor,
            memory.address, memory.size,
            buffer, buffer.size(),
            &bytes_out);
        (void) result; // xxx

        debugPrint("  # total_out:  %d\n", int(bytes_out));

        // process image
        process(image, width, height, stride, buffer);

        libdeflate_free_decompressor(decompressor);

        if (m_number_of_frames > 0)
        {
            Surface d(dest, m_frame.xoffset, m_frame.yoffset, width, height);
            Surface s(width, height, dest.format, stride, image);
            blend(d, s, ptr_palette);
        }

        status.current_frame_index = m_current_frame_index;
        status.next_frame_index = m_next_frame_index;

        return status;
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

    void write_IDAT(Stream& stream, const Surface& surface, int level)
    {
        // create data to compress
        Buffer temp;

        u8* image = surface.image;
        int bytes_per_scan = surface.width * surface.format.bytes();

        for (int y = 0; y < surface.height; ++y)
        {
            u8 filter = 0;
            temp.append(&filter, 1);
            temp.append(image, bytes_per_scan);
            image += surface.stride;
        }

        // compress
        level = clamp(level, 1, 12);
        libdeflate_compressor* compressor = libdeflate_alloc_compressor(level);

        size_t bound = libdeflate_zlib_compress_bound(compressor, temp.size());
        Buffer buffer(bound);

        size_t bytes_out = libdeflate_zlib_compress(compressor, temp, temp.size(), buffer, bound);

        libdeflate_free_compressor(compressor);

        // write chunkdID + compressed data
        writeChunk(stream, u32_mask_rev('I', 'D', 'A', 'T'), Memory(buffer, bytes_out));
    }

    void writePNG(Stream& stream, const Surface& surface, u8 color_bits, ColorType color_type, int level)
    {
        BigEndianStream s(stream);

        // write magic
        s.write64(PNG_HEADER_MAGIC);

        write_IHDR(stream, surface, color_bits, color_type);
        write_IDAT(stream, surface, level);

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

        Interface(ConstMemory memory)
            : m_parser(memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_parser.getHeader();
        }

        ImageDecodeStatus decode(Surface& dest, Palette* ptr_palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            const ImageHeader& header = m_parser.getHeader();
            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            bool direct = dest.format == header.format &&
                          dest.width >= header.width &&
                          dest.height >= header.height &&
                          !ptr_palette;

            if (direct)
            {
                // direct decoding
                status = m_parser.decode(dest, nullptr);
            }
            else
            {
                if (ptr_palette && header.palette)
                {
                    // direct decoding with palette
                    status = m_parser.decode(dest, ptr_palette);
                    direct = true;
                }
                else
                {
                    // indirect
                    Bitmap temp(header.width, header.height, header.format);
                    status = m_parser.decode(temp, nullptr);
                    dest.blit(0, 0, temp);
                }
            }

            status.direct = direct;

            return status;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

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
            writePNG(stream, surface, color_bits, color_type, options.compression);
        }
        else
        {
            Bitmap temp(surface, format);
            writePNG(stream, temp, color_bits, color_type, options.compression);
        }

        return status;
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

#endif // MANGO_ENABLE_IMAGE_PNG
