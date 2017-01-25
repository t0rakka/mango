/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>

#define MINIZ_HEADER_FILE_ONLY
#include "../../miniz/miniz.cpp"

#define ID "ImageStream.PNG: "
#define FILTER_BYTE 1
//#define PNG_ENABLE_PRINT

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // print()
    // ------------------------------------------------------------

#ifdef PNG_ENABLE_PRINT
    #define print(...) printf(__VA_ARGS__)
#else
    #define print(...)
#endif

    // ------------------------------------------------------------
    // PaethPredictor()
    // ------------------------------------------------------------

    inline uint8 PaethPredictor(uint8 a, uint8 b, uint8 c)
    {
        const int x = b - c;
        const int y = a - c;
        const int pa = abs(x);
        const int pb = abs(y);
        const int pc = abs(x + y);
        int pred;
        if (pa <= pb && pa <= pc)
            pred = a;
        else if (pb <= pc)
            pred = b;
        else
            pred = c;
        return pred;
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

        AdamInterleave(uint32 pass, uint32 width, uint32 height)
        {
            assert(pass >=0 && pass < 7);

            const uint32 orig = 0x01020400 >> (pass * 4);
            const uint32 spc = 0x01122333 >> (pass * 4);

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
        COLOR_TYPE_I = 0,
        COLOR_TYPE_RGB = 2,
        COLOR_TYPE_PALETTE = 3,
        COLOR_TYPE_IA = 4,
        COLOR_TYPE_RGBA = 6,
    };

    struct Chromaticity
    {
        float2 white;
        float2 red;
        float2 green;
        float2 blue;
    };

    class ParserPNG
    {
    protected:
        const Memory& m_memory;

        const uint8* m_pointer = NULL;
        const uint8* m_end = NULL;
        const char* m_error = NULL;

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
        int m_bytes_per_line;

        // PLTE
        uint32 m_palette[256];
        int m_palette_size = 0;

        // tRNS
        bool m_transparent_enable = false;
        uint16 m_transparent_sample[3];
        uint32 m_transparent_rgba;

        // cHRM
        Chromaticity m_chromaticity;

        // gAMA
        float m_gamma = 0.0f;

        // sBIT
        uint8 m_scale_bits[4];

        // sRGB
        uint8 m_srgb_render_intent = -1;

        void setError(const char* error);

        void read_IHDR(BigEndianConstPointer p, uint32 size);
        void read_IDAT(BigEndianConstPointer p, uint32 size);
        void read_PLTE(BigEndianConstPointer p, uint32 size);
        void read_tRNS(BigEndianConstPointer p, uint32 size);
        void read_cHRM(BigEndianConstPointer p, uint32 size);
        void read_gAMA(BigEndianConstPointer p, uint32 size);
        void read_sBIT(BigEndianConstPointer p, uint32 size);
        void read_sRGB(BigEndianConstPointer p, uint32 size);

        void parse();
        void filter(uint8* buffer, int bytes, int height);
        uint8* deinterlace1to4(uint8* buffer);
        uint8* deinterlace8to16(uint8* buffer);

        void process_i1to4(uint8* dest, int stride, const uint8* src);
        void process_i8(uint8* dest, int stride, const uint8* src);
        void process_rgb8(uint8* dest, int stride, const uint8* src);
        void process_pal1to4(uint8* dest, int stride, const uint8* src);
        void process_pal8(uint8* dest, int stride, const uint8* src);
        void process_ia8(uint8* dest, int stride, const uint8* src);
        void process_rgba8(uint8* dest, int stride, const uint8* src);
        void process_i16(uint8* dest, int stride, const uint8* src);
        void process_rgb16(uint8* dest, int stride, const uint8* src);
        void process_ia16(uint8* dest, int stride, const uint8* src);
        void process_rgba16(uint8* dest, int stride, const uint8* src);

        uint8* process(uint8* image, int stride, uint8* src);

    public:
        ParserPNG(const Memory& memory);
        ~ParserPNG();

        const char* getError() const;

        ImageHeader header() const;
        const char* decode(Surface& dest);
    };

    // ------------------------------------------------------------
    // ParserPNG
    // ------------------------------------------------------------

    ParserPNG::ParserPNG(const Memory& memory)
    : m_memory(memory), m_end(memory.address + memory.size)
    {
        BigEndianConstPointer p = memory.address;

        // read header magic
        const uint64 magic = p.read64();
        if (magic != 0x89504e470d0a1a0a)
        {
            setError("Incorrect header magic.");
            return;
        }

        // read first chunk; it must be IHDR
        const uint32 size = p.read32();
        const uint32 id = p.read32();
        if (id != makeReverseFourCC('I', 'H', 'D', 'R'))
        {
            setError("Incorrect file; the IHDR chunk must come first.");
            return;
        }

        read_IHDR(p, size);
        p += size; // skip chunk data
        p += sizeof(uint32); // skip crc

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

    void ParserPNG::read_IHDR(BigEndianConstPointer p, uint32 size)
    {
        print("[\"IHDR\"] %d bytes\n", size);

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
            const uint32 size = p.read32();
            const uint32 id = p.read32();
            switch (id)
            {
                case makeReverseFourCC('t', 'R', 'N', 'S'):
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

        m_bytes_per_line = m_channels * ((m_bit_depth * m_width + 7) / 8);

        print("  Image: (%d x %d), %d bits\n", m_width, m_height, m_bit_depth);
        print("  Color:       %d\n", m_color_type);
        print("  Compression: %d\n", m_compression);
        print("  Filter:      %d\n", m_filter);
        print("  Interlace:   %d\n", m_interlace);
    }

    void ParserPNG::read_IDAT(BigEndianConstPointer p, uint32 size)
    {
        m_compressed.write(p, size);
    }

    void ParserPNG::read_PLTE(BigEndianConstPointer p, uint32 size)
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

        m_palette_size = size / 3;
        for (int i = 0; i < m_palette_size; ++i)
        {
            m_palette[i] = PackedColor(p[0], p[1], p[2], 0xff);
            p += 3;
        }
    }

    void ParserPNG::read_tRNS(BigEndianConstPointer p, uint32 size)
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
            m_transparent_rgba = PackedColor(m_transparent_sample[0] & 0xff,
                                             m_transparent_sample[1] & 0xff,
                                             m_transparent_sample[2] & 0xff, 0xff);
        }
        else if (m_color_type == COLOR_TYPE_PALETTE)
        {
            if (m_palette_size < int(size))
            {
                setError("Incorrect alpha palette size.");
                return;
            }

            for (uint32 i = 0; i < size; ++i)
            {
                uint32 alpha = p[i];
                uint32 color = m_palette[i] & 0x00ffffff;
                m_palette[i] = color | (alpha << 24);
            }
        }
        else
        {
            setError("Incorrect color type for alpha palette.");
        }
    }

    void ParserPNG::read_cHRM(BigEndianConstPointer p, uint32 size)
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

    void ParserPNG::read_gAMA(BigEndianConstPointer p, uint32 size)
    {
        if (size != 4)
        {
            setError("Incorrect gAMA chunk size.");
            return;
        }

        m_gamma = p.read32() / 100000.0f;
    }

    void ParserPNG::read_sBIT(BigEndianConstPointer p, uint32 size)
    {
        if (size > 4)
        {
            setError("Incorrect sBIT chunk size.");
            return;
        }

        for (uint32 i = 0; i < size; ++i)
        {
            m_scale_bits[i] = p[i];
        }
    }

    void ParserPNG::read_sRGB(BigEndianConstPointer p, uint32 size)
    {
        if (size != 1)
        {
            setError("Incorrect sRGB chunk size.");
            return;
        }

        m_srgb_render_intent = p[0];
    }

    ImageHeader ParserPNG::header() const
    {
        ImageHeader header;

        if (!m_error)
        {
            header.width  = m_width;
            header.height = m_height;
            header.depth  = 0;
            header.levels = 0;
            header.faces  = 0;
            header.compression = TextureCompression::NONE;

            // force alpha channel on when transparency is enabled
            int type = m_color_type;
            if (m_transparent_enable && type != COLOR_TYPE_PALETTE)
                type |= 4;

            // select decoding format
            switch (m_color_type)
            {
                case COLOR_TYPE_I:
                    header.format = m_bit_depth <= 8 ?
                        Format(8, 0xff, 0xff, 0xff, 0) :
                        Format(16, 0xffff, 0xffff, 0xffff, 0);
                    break;

                case COLOR_TYPE_IA:
                    header.format = m_bit_depth <= 8 ?
                        Format(16, 0xff, 0xff, 0xff, 0xff00) :
                        Format(32, 0xffff, 0xffff, 0xffff, 0xffff0000);
                    break;

                case COLOR_TYPE_PALETTE:
                    header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    break;

                case COLOR_TYPE_RGB:
                case COLOR_TYPE_RGBA:
                    header.format = m_bit_depth <= 8 ?
                        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8) :
                        Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
                    break;
            }
        }

        return header;
    }

    void ParserPNG::parse()
    {
        BigEndianConstPointer p = m_pointer;

        for (; p < m_end - 8;)
        {
            const uint32 size = p.read32();
            const uint32 id = p.read32();
            print("[\"%c%c%c%c\"] %d bytes\n", (id >> 24), (id >> 16), (id >> 8), (id >> 0), size);

            // check that we won't read past end of file
            if (p + size + 4 > m_end)
            {
                setError("Corrupted file.\n");
                return;
            }

            switch (id)
            {
                case makeReverseFourCC('I', 'H', 'D', 'R'):
                    setError("File can only have one IHDR chunk.");
                    break;

                case makeReverseFourCC('P', 'L', 'T', 'E'):
                    read_PLTE(p, size);
                    break;

                case makeReverseFourCC('t', 'R', 'N', 'S'):
                    read_tRNS(p, size);
                    break;

                case makeReverseFourCC('g', 'A', 'M', 'A'):
                    read_gAMA(p, size);
                    break;

                case makeReverseFourCC('s', 'B', 'I', 'T'):
                    read_sBIT(p, size);
                    break;

                case makeReverseFourCC('s', 'R', 'G', 'B'):
                    read_sRGB(p, size);
                    break;

                case makeReverseFourCC('c', 'H', 'R', 'M'):
                    read_cHRM(p, size);
                    break;

                case makeReverseFourCC('I', 'D', 'A', 'T'):
                    read_IDAT(p, size);
                    break;

                case makeReverseFourCC('p', 'H', 'Y', 's'):
                case makeReverseFourCC('b', 'K', 'G', 'D'):
                case makeReverseFourCC('z', 'T', 'X', 't'):
                case makeReverseFourCC('t', 'E', 'X', 't'):
                case makeReverseFourCC('t', 'I', 'M', 'E'):
                    // NOTE: ignoring these chunks
                    break;

                case makeReverseFourCC('I', 'E', 'N', 'D'):
                    // terminate parsing (required for files with junk after the IEND marker)
                    p = m_end; 
                    break;

                default:
                    print("UNKNOWN CHUNK: [\"%c%c%c%c\"] %d bytes\n", (id >> 24), (id >> 16), (id >> 8), (id >> 0), size);
                    break;
            }

            if (m_error)
            {
                // error occured while reading chunks
                return;
            }

            p += size;
            p += 4; // skip CRC
        }
    }

    void ParserPNG::filter(uint8* buffer, int bytes, int height)
    {
        // zero scanline
        uint8* zero = new uint8[bytes];
        if (!zero)
        {
            setError("Memory allocation failed.");
            return;
        }
        std::memset(zero, 0, bytes);
        const uint8* p = zero;

        uint8 prev[16];

        const int size = (m_bit_depth < 8) ? 1 : m_channels * m_bit_depth / 8;
        if (size > 8)
            return;

        uint8* s = buffer;

        for (int y = 0; y < height; ++y)
        {
            int method = *s++;
            uint8* buf = s;

            switch (method)
            {
                case 0:
                    break;

                case 1:
                    std::memset(prev, 0, 8);
                    for (int x = 0; x < bytes; x += size)
                    {
                        for (int i = 0; i < size; ++i)
                        {
                            const uint8 value = buf[i] + prev[i];
                            prev[i] = value;
                            buf[i] = value;
                        }
                        buf += size;
                    }
                    break;

                case 2:
                    for (int x = 0; x < bytes; ++x)
                    {
                        const uint8 value = buf[x] + p[x];
                        buf[x] = value;
                    }
                    break;

                case 3:
                    std::memset(prev, 0, 8);
                    for (int x = 0; x < bytes; x += size)
                    {
                        for (int i = 0; i < size; ++i)
                        {
                            const uint8 value = buf[i] + ((prev[i] + p[i]) >> 1);
                            prev[i] = value;
                            buf[i] = value;
                        }
                        p += size;
                        buf += size;
                    }
                    break;

                case 4:
                    std::memset(prev, 0, 16);
                    for (int x = 0; x < bytes; x += size)
                    {
                        for (int i = 0; i < size; ++i)
                        {
                            uint8 A = prev[i + 0];
                            uint8 C = prev[i + 8];
                            uint8 B = p[i];
                            uint8 value = buf[i] + PaethPredictor(A, B, C);
                            buf[i] = value;
                            prev[i + 0] = value;
                            prev[i + 8] = B;
                        }
                        p += size;
                        buf += size;
                    }
                    break;
            }

            p = s;
            s += bytes;
        }

        delete[] zero;
    }

    uint8* ParserPNG::deinterlace1to4(uint8* buffer)
    {
        const int stride = FILTER_BYTE + m_bytes_per_line;
        uint8* temp = new uint8[m_height * stride];
        if (!temp)
        {
            setError("Memory allocation failed.");
            return buffer;
        }
        std::memset(temp, 0, m_height * stride);

        uint8* p = buffer;

        const int samples = 8 / m_bit_depth;
        const int mask = samples - 1;
        const int shift = u32_log2(samples);
        const int valueShift = u32_log2(m_bit_depth);
        const int valueMask = (1 << m_bit_depth) - 1;

        for (int pass = 0; pass < 7; ++pass)
        {
            AdamInterleave adam(pass, m_width, m_height);
            print("  pass: %d (%d x %d)\n", pass, adam.w, adam.h);

            const int bw = FILTER_BYTE + ((adam.w + mask) >> shift);
            filter(p, bw - FILTER_BYTE, adam.h);

            if (adam.w && adam.h)
            {
                for (int y = 0; y < adam.h; ++y)
                {
                    const int yoffset = (y << adam.yspc) + adam.yorig;
                    uint8* dest = temp + yoffset * stride + FILTER_BYTE;
                    uint8* src = p + y * bw + FILTER_BYTE;

                    for (int x = 0; x < adam.w; ++x)
                    {
                        const int xoffset = (x << adam.xspc) + adam.xorig;
                        uint8 v = src[x >> shift];
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

        // migrate to deinterleaved temp buffer
        delete[] buffer;
        return temp;
    }

    uint8* ParserPNG::deinterlace8to16(uint8* buffer)
    {
        const int stride = FILTER_BYTE + m_bytes_per_line;
        uint8* temp = new uint8[m_height * stride];
        if (!temp)
        {
            setError("Memory allocation failed.");
            return buffer;
        }

        uint8* p = buffer;
        const int size = m_bytes_per_line / m_width;

        for (int pass = 0; pass < 7; ++pass)
        {
            AdamInterleave adam(pass, m_width, m_height);
            print("  pass: %d (%d x %d)\n", pass, adam.w, adam.h);

            const int bw = FILTER_BYTE + adam.w * size;
            filter(p, bw - FILTER_BYTE, adam.h);

            if (adam.w && adam.h)
            {
                const int ps = adam.w * size + FILTER_BYTE;

                for (int y = 0; y < adam.h; ++y)
                {
                    const int yoffset = (y << adam.yspc) + adam.yorig;
                    uint8* dest = temp + yoffset * stride + FILTER_BYTE;
                    uint8* src = p + y * ps + FILTER_BYTE;

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

        // migrate to deinterleaved temp buffer
        delete[] buffer;
        return temp;
    }

    void ParserPNG::process_i1to4(uint8* dest, int stride, const uint8* src)
    {
        const int width = m_width;
        const int height = m_height;
        const int bits = m_bit_depth;

        const int maxValue = (1 << bits) - 1;
        const int scale = (255 / maxValue) * 0x010101;
        const uint32 mask = (1 << bits) - 1;

        for (int y = 0; y < height; ++y)
        {
            uint8* d = dest;
            dest += stride;
            ++src; // skip filter byte

            uint32 data = 0;
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
                offset -= bits;
            }
        }
    }

    void ParserPNG::process_i8(uint8* dest, int stride, const uint8* src)
    {
        const int width = m_width;
        const int height = m_height;

        if (m_transparent_enable)
        {
            for (int y = 0; y < height; ++y)
            {
                uint16* d = reinterpret_cast<uint16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    const uint16 alpha = m_transparent_sample[0] == src[x] ? 0 : 0xff00;
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

    void ParserPNG::process_rgb8(uint8* dest, int stride, const uint8* src)
    {
        const int width = m_width;
        const int height = m_height;

        if (m_transparent_enable)
        {
            for (int y = 0; y < height; ++y)
            {
                uint32* d = reinterpret_cast<uint32*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    PackedColor color(src[0], src[1], src[2], 0xff);
                    if (uint32(color) == m_transparent_rgba)
                        color[3] = 0;
                    *d++ = color;
                    src += 3;
                }
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                uint32* d = reinterpret_cast<uint32*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    *d++ = PackedColor(src[0], src[1], src[2], 0xff);
                    src += 3;
                }
            }
        }
    }

    void ParserPNG::process_pal1to4(uint8* dest, int stride, const uint8* src)
    {
        const int width = m_width;
        const int height = m_height;
        const int bits = m_bit_depth;
        const uint32* palette = m_palette;

        const uint32 mask = (1 << bits) - 1;

        for (int y = 0; y < height; ++y)
        {
            uint32* d = reinterpret_cast<uint32*>(dest);
            dest += stride;
            ++src; // skip filter byte

            uint32 data = 0;
            int offset = -1;

            for (int x = 0; x < width; ++x)
            {
                if (offset < 0)
                {
                    offset = 8 - bits;
                    data = *src++;
                }
                *d++ = palette[(data >> offset) & mask];
                offset -= bits;
            }
        }
    }

    void ParserPNG::process_pal8(uint8* dest, int stride, const uint8* src)
    {
        const int width = m_width;
        const int height = m_height;
        const uint32* palette = m_palette;

        for (int y = 0; y < height; ++y)
        {
            uint32* d = reinterpret_cast<uint32*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                *d++ = palette[*src++];
            }
        }
    }

    void ParserPNG::process_ia8(uint8* dest, int stride, const uint8* src)
    {
        const int width = m_width;
        const int height = m_height;

        for (int y = 0; y < height; ++y)
        {
            uint16* d = reinterpret_cast<uint16*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                *d++ = (src[1] << 8) | src[0];
                src += 2;
            }
        }
    }

    void ParserPNG::process_rgba8(uint8* dest, int stride, const uint8* src)
    {
        const int width = m_width;
        const int height = m_height;

        for (int y = 0; y < height; ++y)
        {
            uint32* d = reinterpret_cast<uint32*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                *d++ = PackedColor(src[0], src[1], src[2], src[3]);
                src += 4;
            }
        }
    }

    void ParserPNG::process_i16(uint8* dest, int stride, const uint8* src)
    {
        const int width = m_width;
        const int height = m_height;

        if (m_transparent_enable)
        {
            for (int y = 0; y < height; ++y)
            {
                uint16* d = reinterpret_cast<uint16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    uint16 gray = (src[0] << 8) | src[1];
                    uint16 alpha = m_transparent_sample[0] == src[0] ? 0 : 0xffff;
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
                uint16* d = reinterpret_cast<uint16*>(dest);
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

    void ParserPNG::process_rgb16(uint8* dest, int stride, const uint8* src)
    {
        const int width = m_width;
        const int height = m_height;

        if (m_transparent_enable)
        {
            for (int y = 0; y < height; ++y)
            {
                uint16* d = reinterpret_cast<uint16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    uint16 red   = (src[0] << 8) | src[1];
                    uint16 green = (src[2] << 8) | src[3];
                    uint16 blue  = (src[4] << 8) | src[5];
                    uint16 alpha = 0xffff;
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
                uint16* d = reinterpret_cast<uint16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    uint16 red   = (src[0] << 8) | src[1];
                    uint16 green = (src[2] << 8) | src[3];
                    uint16 blue  = (src[4] << 8) | src[5];
                    uint16 alpha = 0xffff;
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

    void ParserPNG::process_ia16(uint8* dest, int stride, const uint8* src)
    {
        const int width = m_width;
        const int height = m_height;

        for (int y = 0; y < height; ++y)
        {
            uint16* d = reinterpret_cast<uint16*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                uint16 gray  = (src[0] << 8) | src[1];
                uint16 alpha = (src[2] << 8) | src[3];
                d[0] = gray;
                d[1] = alpha;
                d += 2;
                src += 4;
            }
        }
    }

    void ParserPNG::process_rgba16(uint8* dest, int stride, const uint8* src)
    {
        const int width = m_width;
        const int height = m_height;

        for (int y = 0; y < height; ++y)
        {
            uint16* d = reinterpret_cast<uint16*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                uint16 red   = (src[0] << 8) | src[1];
                uint16 green = (src[2] << 8) | src[3];
                uint16 blue  = (src[4] << 8) | src[5];
                uint16 alpha = (src[6] << 8) | src[7];
                d[0] = red;
                d[1] = green;
                d[2] = blue;
                d[3] = alpha;
                d += 4;
                src += 8;
            }
        }
    }

    uint8* ParserPNG::process(uint8* image, int stride, uint8* buffer)
    {
        if (m_interlace)
        {
            // deinterlace does filter for each pass
            if (m_bit_depth < 8)
                buffer = deinterlace1to4(buffer);
            else
                buffer = deinterlace8to16(buffer);
        }
        else
        {
            // filter the whole image in single pass
            filter(buffer, m_bytes_per_line, m_height);
        }

        if (m_error)
            return buffer;

        if (m_color_type == COLOR_TYPE_I)
        {
            if (m_bit_depth < 8)
                process_i1to4(image, stride, buffer);
            else if (m_bit_depth == 8)
                process_i8(image, stride, buffer);
            else
                process_i16(image, stride, buffer);
        }
        else if (m_color_type == COLOR_TYPE_RGB)
        {
            if (m_bit_depth == 8)
                process_rgb8(image, stride, buffer);
            else
                process_rgb16(image, stride, buffer);
        }
        else if (m_color_type == COLOR_TYPE_PALETTE)
        {
            if (m_bit_depth < 8)
                process_pal1to4(image, stride, buffer);
            else
                process_pal8(image, stride, buffer);
        }
        else if (m_color_type == COLOR_TYPE_IA)
        {
            if (m_bit_depth == 8)
                process_ia8(image, stride, buffer);
            else
                process_ia16(image, stride, buffer);
        }
        else if (m_color_type == COLOR_TYPE_RGBA)
        {
            if (m_bit_depth == 8)
                process_rgba8(image, stride, buffer);
            else
                process_rgba16(image, stride, buffer);
        }

        return buffer;
    }

    const char* ParserPNG::decode(Surface& dest)
    {
        if (!m_error)
        {
            parse();

            int buffer_size = 0;
            
            // compute output buffer size
            if (m_interlace)
            {
                // NOTE: brute-force loop to resolve memory consumption
                for (int pass = 0; pass < 7; ++pass)
                {
                    AdamInterleave adam(pass, m_width, m_height);
                    if (adam.w && adam.h)
                    {
                        const int bytesPerLine = FILTER_BYTE + m_channels * ((adam.w * m_bit_depth + 7) / 8);
                        buffer_size += bytesPerLine * adam.h;
                    }
                }
            }
            else
            {
                buffer_size = (FILTER_BYTE + m_bytes_per_line) * m_height;
            }
            
            // allocate output buffer
            print("  buffer bytes: %d\n", buffer_size);
            uint8* buffer = new uint8[buffer_size];
            if (!buffer)
            {
                setError("Memory allocation failed.");
                return m_error;
            }

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
                // error
            }

            status = mz_inflate(&stream, MZ_FINISH);
            if (status != MZ_STREAM_END)
            {
                // error
            }

            print("  # total_out: %d \n", int(stream.total_out));
            status = mz_inflateEnd(&stream);

            // process image
            buffer = process(dest.image, dest.stride, buffer);
            delete[] buffer;
        }

        return m_error;
    }

    // ------------------------------------------------------------
    // writePNG()
    // ------------------------------------------------------------

    void writeChunk(Stream& stream, const uint8* buffer, size_t size)
    {
        BigEndianStream s(stream);

        const uint32 chunk_size = static_cast<uint32>(size - 4);
        const uint32 chunk_crc = crc32(0, buffer, size);

        s.write32(chunk_size);
        s.write(buffer, size);
        s.write32(chunk_crc);
    }

    void write_IHDR(Stream& stream, const Surface& surface)
    {
        Buffer buffer;
        BigEndianStream s(buffer);

        s.write32(makeReverseFourCC('I', 'H', 'D', 'R'));

        s.write32(surface.width);
        s.write32(surface.height);
        s.write8(8); // color bits
        s.write8(6); // color type
        s.write8(0); // compression
        s.write8(0); // filter
        s.write8(0); // interlace

        writeChunk(stream, buffer, static_cast<size_t>(buffer.size()));
    }

    void write_IDAT(Stream& stream, const Surface& surface)
    {
        const int bytesPerLine = surface.width * sizeof(uint32);
        const int bytes = (FILTER_BYTE + bytesPerLine) * surface.height;

        z_stream z = { 0 };
        deflateInit(&z, -1);

        z.avail_out = (unsigned int)deflateBound(&z, bytes);
        Buffer buffer(4 + z.avail_out);

        BigEndianStream s(buffer);
        s.write32(makeReverseFourCC('I', 'D', 'A', 'T'));

        z.next_out = buffer + 4;

        for (int y = 0; y < surface.height; ++y)
        {
            bool last_scan = (y == surface.height - 1);

            // compress filler byte
            uint8 zero = 0;
            z.avail_in = 1;
            z.next_in = &zero;
            deflate(&z, Z_NO_FLUSH);

            // compress scanline
            z.avail_in = bytesPerLine;
            z.next_in = surface.address<uint8>(0, y);
            deflate(&z, last_scan ? Z_FINISH : Z_NO_FLUSH);
        }

        // compressed size includes chunkID
        const size_t compressed_size = int(z.next_out - buffer);

        deflateEnd(&z);

        // write chunkdID + compressed data
        writeChunk(stream, buffer, compressed_size);
    }

    void writePNG(Stream& stream, const Surface& surface)
    {
        static const uint8 magic[] =
        {
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
        };

        BigEndianStream s(stream);

        // write magic
        s.write(magic, 8);

        write_IHDR(stream, surface);
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

        Interface(const Memory& memory)
        : m_parser(memory)
        {
            m_header = m_parser.header();

            const char* error = m_parser.getError();
            if (error)
            {
                print("HEADER ERROR: %s\n", error);
            }
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        void decode(Surface& dest, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            const char* error = NULL;

            if (dest.format == m_header.format &&
                dest.width >= m_header.width &&
                dest.height >= m_header.height)
            {
                // direct decoding
                error = m_parser.decode(dest);
            }
            else
            {
                // indirect
                Bitmap temp(m_header.width, m_header.height, m_header.format);
                error = m_parser.decode(temp);
                dest.blit(0, 0, temp);
            }

            if (error)
            {
                print("DECODE ERROR: %s\n", error);
            }
        }
    };

    ImageDecoderInterface* createInterface(const Memory& memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    void imageEncode(Stream& stream, const Surface& surface, float quality)
    {
        MANGO_UNREFERENCED_PARAMETER(quality);

        // keep it simple: only write 32 bit rgba
        if (surface.format == FORMAT_R8G8B8A8)
        {
            writePNG(stream, surface);
        }
        else
        {
            Bitmap temp(surface.width, surface.height, FORMAT_R8G8B8A8);
            temp.blit(0, 0, surface);
            writePNG(stream, temp);
        }
    }

} // namespace

namespace mango
{

    void registerPNG()
    {
        registerImageDecoder(createInterface, "png");
        registerImageEncoder(imageEncode, "png");
    }

} // namespace mango
