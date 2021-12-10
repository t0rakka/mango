/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <mango/core/thread.hpp>
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // QOI codec
    // ------------------------------------------------------------

    constexpr u32 QOI_HEADER_MAGIC = u32_mask('q', 'o', 'i', 'f');
    constexpr u32 QOI_HEADER_SIZE = 14;

    enum : u8
    {
        QOI_SRGB = 0x00,
        QOI_SRGB_LINEAR_ALPHA = 0x01,
        QOI_LINEAR = 0x0f
    };

    #define QOI_INDEX   0x00 // 00xxxxxx
    #define QOI_RUN_8   0x40 // 010xxxxx
    #define QOI_RUN_16  0x60 // 011xxxxx
    #define QOI_DIFF_8  0x80 // 10xxxxxx
    #define QOI_DIFF_16 0xc0 // 110xxxxx
    #define QOI_DIFF_24 0xe0 // 1110xxxx
    #define QOI_COLOR   0xf0 // 1111xxxx

    #define QOI_UPDATE  0x80 // 1xxxxxxx

    #define QOI_MASK_2  0xc0 // 11000000
    #define QOI_MASK_3  0xe0 // 11100000
    #define QOI_MASK_4  0xf0 // 11110000

    #define QOI_COLOR_HASH(C) (C.r ^ C.g ^ C.b ^ C.a)
    #define QOI_PADDING 4

    static inline
    bool is_diff(int r, int g, int b, int a)
    {
        return r > -16 && r < 17 &&
            g > -16 && g < 17 && 
            b > -16 && b < 17 && 
            a > -16 && a < 17;
    }

    static inline
    bool is_diff8(int r, int g, int b, int a)
    {
        return a == 0 &&
            r > -2 && r < 3 &&
            g > -2 && g < 3 && 
            b > -2 && b < 3;
    }

    static inline
    bool is_diff16(int r, int g, int b, int a)
    {
        return a == 0 &&
            r > -16 && r < 17 && 
            g >  -8 && g <  9 && 
            b >  -8 && b <  9;
    }

    u8* qoi_encode(const u8* image, size_t stride, int width, int height, size_t* out_len)
    {
        if (image == NULL || out_len == NULL ||
            width <= 0 || width >= (1 << 16) ||
            height <= 0 || height >= (1 << 16))
        {
            return nullptr;
        }

        constexpr int channels = 4;

        int max_size = width * height * (channels + 1) + QOI_PADDING;
        u8* bytes = new u8[max_size];
        if (!bytes)
        {
            return nullptr;
        }

        int p = 0;

        Color index[64] = { 0 };

        int run = 0;
        Color prev(0, 0, 0, 255);
        Color color = prev;

        for (int y = 0; y < height; ++y)
        {
            bool is_last_scanline = (y == height - 1);
            int xend = is_last_scanline ? (width - 1) : (1 << 30);

            const Color* src = reinterpret_cast<const Color*>(image);

            for (int x = 0; x < width; ++x)
            {
                color = src[x];

                if (color == prev)
                {
                    run++;
                }

                bool is_last_pixel = (x == xend);

                if (run > 0 && (run == 0x2020 || color != prev || is_last_pixel))
                {
                    if (run < 33)
                    {
                        run -= 1;
                        bytes[p++] = QOI_RUN_8 | run;
                    }
                    else
                    {
                        run -= 33;
                        bytes[p++] = QOI_RUN_16 | run >> 8;
                        bytes[p++] = run;
                    }
                    run = 0;
                }

                if (color != prev)
                {
                    int index_pos = QOI_COLOR_HASH(color) % 64;

                    if (index[index_pos] == color)
                    {
                        bytes[p++] = QOI_INDEX | index_pos;
                    }
                    else
                    {
                        index[index_pos] = color;

                        int r = color.r - prev.r;
                        int g = color.g - prev.g;
                        int b = color.b - prev.b;
                        int a = color.a - prev.a;

                        if (is_diff(r, g, b, a))
                        {
                            if (is_diff8(r, g, b, a))
                            {
                                bytes[p++] = QOI_DIFF_8 | ((r + 1) << 4) | (g + 1) << 2 | (b + 1);
                            }
                            else if (is_diff16(r, g, b, a))
                            {
                                bytes[p++] = QOI_DIFF_16 | (r + 15);
                                bytes[p++] = ((g + 7) << 4) | (b + 7);
                            }
                            else
                            {
                                bytes[p++] = QOI_DIFF_24 | ((r + 15) >> 1);
                                bytes[p++] = ((r + 15) << 7) | ((g + 15) << 2) | ((b + 15) >> 3);
                                bytes[p++] = ((b + 15) << 5) | (a + 15);
                            }
                        }
                        else
                        {
                            int p0 = p;
                            bytes[p++] = QOI_COLOR;

                            int mask = 0;
                            if (r)
                            {
                                mask |= 8;
                                bytes[p++] = color.r;
                            }
                            if (g)
                            {
                                mask |= 4;
                                bytes[p++] = color.g;
                            }
                            if (b)
                            {
                                mask |= 2;
                                bytes[p++] = color.b;
                            }
                            if (a)
                            {
                                mask |= 1;
                                bytes[p++] = color.a;
                            }

                            bytes[p0] |= mask;
                        }
                    }
                }

                prev = color;
            }

            image += stride;
        }

        for (int i = 0; i < QOI_PADDING; i++)
        {
            bytes[p++] = 0;
        }

        *out_len = p;
        return bytes;
    }

    void qoi_decode(u8* image, const u8* data, size_t size, int width, int height, size_t stride)
    {
        Color color(0, 0, 0, 255);
        Color index[64] = { 0 };

        //const u8* end = data + size;
        int run = 0;

        for (int y = 0; y < height; ++y)
        {
            Color* dest = reinterpret_cast<Color*>(image);
            Color* xend = dest + width;

            for ( ; dest < xend; )
            {
                if (run > 0)
                {
                    --run;
                    *dest++ = color;
                }
                else
                {
                    u32 b1 = *data++;

                    if ((b1 & QOI_MASK_2) == QOI_INDEX)
                    {
                        color = index[b1 ^ QOI_INDEX];
                    }
                    else if ((b1 & QOI_MASK_3) == QOI_RUN_8)
                    {
                        run = (b1 & 0x1f);
                    }
                    else if ((b1 & QOI_MASK_3) == QOI_RUN_16)
                    {
                        u32 b2 = *data++;
                        run = (((b1 & 0x1f) << 8) | (b2)) + 32;
                    }
                    else if ((b1 & QOI_MASK_2) == QOI_DIFF_8)
                    {
                        color.r += ((b1 >> 4) & 0x03) - 1;
                        color.g += ((b1 >> 2) & 0x03) - 1;
                        color.b += ((b1 >> 0) & 0x03) - 1;
                    }
                    else if ((b1 & QOI_MASK_3) == QOI_DIFF_16)
                    {
                        u32 b2 = *data++;
                        color.r += (b1 & 0x1f) - 15;
                        color.g += (b2 >> 4) - 7;
                        color.b += (b2 & 0x0f) - 7;
                    }
                    else if ((b1 & QOI_MASK_4) == QOI_DIFF_24)
                    {
                        u32 b = (b1 << 16) | (data[0] << 8) | data[1];
                        data += 2;
                        color.r += ((b >> 15) & 0x1f) - 15;
                        color.g += ((b >> 10) & 0x1f) - 15;
                        color.b += ((b >>  5) & 0x1f) - 15;
                        color.a += ((b >>  0) & 0x1f) - 15;
                    }
                    else if ((b1 & QOI_MASK_4) == QOI_COLOR)
                    {
                        if (b1 & 8) { color.r = *data++; }
                        if (b1 & 4) { color.g = *data++; }
                        if (b1 & 2) { color.b = *data++; }
                        if (b1 & 1) { color.a = *data++; }
                    }

                    if (b1 & QOI_UPDATE)
                    {
                        index[QOI_COLOR_HASH(color) % 64] = color;
                    }

                    *dest++ = color;
                }
            }

            image += stride;
        }
    }

    // ------------------------------------------------------------
    // ImageDecoderQOI
    // ------------------------------------------------------------

    struct ImageDecoderQOI : ImageDecoderInterface
    {
        ConstMemory m_memory;
        ImageHeader m_header;

        ImageDecoderQOI(ConstMemory memory)
        {
            if (memory.size < QOI_HEADER_SIZE)
            {
                m_header.setError("[ImageDecoder.QOI] Not enough data.");
                return;
            }

            BigEndianConstPointer p = memory;

            // read header
            u32 magic = p.read32();
            u32 width = p.read32();
            u32 height = p.read32();
            int channels = p.read8();
            int colorspace = p.read8();

            m_memory = ConstMemory(p, memory.size - QOI_HEADER_SIZE);

            if (magic != QOI_HEADER_MAGIC)
            {
                m_header.setError("[ImageDecoder.QOI] Incorrect identifier.");
                return;
            }

            if (channels < 3 || channels > 4)
            {
                m_header.setError("[ImageDecoder.QOI] Incorrect number of channels.");
                return;
            }

            switch (colorspace)
            {
                case QOI_SRGB:
                case QOI_SRGB_LINEAR_ALPHA:
                case QOI_LINEAR:
                    break;
                default:
                    m_header.setError("[ImageDecoder.QOI] Incorrect colorspace.");
                    return;
            }

            m_header.width   = width;
            m_header.height  = height;
            m_header.depth   = 0;
            m_header.levels  = 0;
            m_header.faces   = 0;
            m_header.palette = false;
            m_header.format  = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            m_header.compression = TextureCompression::NONE;
        }

        ~ImageDecoderQOI()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!m_header.success)
            {
                status.setError(m_header.info);
                return status;
            }

            if (dest.format == m_header.format &&
                dest.width >= m_header.width &&
                dest.height >= m_header.height)
            {
                qoi_decode(dest.image, m_memory.address, m_memory.size, m_header.width, m_header.height, dest.stride);
                status.direct = true;
            }
            else
            {
                Bitmap temp(m_header.width, m_header.height, m_header.format);
                qoi_decode(temp.image, m_memory.address, m_memory.size, m_header.width, m_header.height, temp.stride);
                dest.blit(0, 0, temp);
            }

            return status;
        }
    };

    ImageDecoderInterface* create_decoder_qoi(ConstMemory memory)
    {
        ImageDecoderInterface* x = new ImageDecoderQOI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // encode_qoi
    // ------------------------------------------------------------

    ImageEncodeStatus encode_qoi(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

        u8* buffer = nullptr;
        size_t size = 0;

        if (surface.format == format)
        {
            buffer = qoi_encode(surface.image, surface.stride, surface.width, surface.height, &size);
        }
        else
        {
            Bitmap temp(surface.width, surface.height, format);
            temp.blit(0, 0, surface);
            buffer = qoi_encode(temp.image, temp.stride, temp.width, temp.height, &size);
        }

        ImageEncodeStatus status;

        if (!buffer || !size)
        {
            delete[] buffer;
            status.setError("[ImageDecoder.QOI] Encoding failed.");
            return status;
        }

        BigEndianStream s = stream;

        // write header
        s.write32(QOI_HEADER_MAGIC);
        s.write32(surface.width);
        s.write32(surface.height);
        s.write8(4);
        s.write8(QOI_SRGB_LINEAR_ALPHA);

        // write encoded image
        s.write(buffer, u64(size));

        delete[] buffer;
    
        return status;
    }

    // ------------------------------------------------------------
    // ImageDecoderTOI
    // ------------------------------------------------------------

    constexpr u32 TOI_HEADER_MAGIC = u32_mask('t', 'o', 'i', 'f');
    constexpr u32 TOI_HEADER_SIZE = 16;

    struct ImageDecoderTOI : ImageDecoderInterface
    {
        ConstMemory m_memory;
        ImageHeader m_header;
        u32 m_xtile;
        u32 m_ytile;

        ImageDecoderTOI(ConstMemory memory)
        {
            if (memory.size < TOI_HEADER_SIZE)
            {
                m_header.setError("[ImageDecoder.TOI] Not enough data.");
                return;
            }

            LittleEndianConstPointer p = memory;

            // read header
            u32 magic = p.read32();
            u32 width = p.read32();
            u32 height = p.read32();
            u32 xtile = p.read16();
            u32 ytile = p.read16();

            m_memory = ConstMemory(p, memory.size - TOI_HEADER_SIZE);

            if (magic != TOI_HEADER_MAGIC)
            {
                m_header.setError("[ImageDecoder.TOI] Incorrect identifier.");
                return;
            }

            m_xtile = xtile;
            m_ytile = ytile;

            m_header.width   = width;
            m_header.height  = height;
            m_header.depth   = 0;
            m_header.levels  = 0;
            m_header.faces   = 0;
            m_header.palette = false;
            m_header.format  = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            m_header.compression = TextureCompression::NONE;
        }

        ~ImageDecoderTOI()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

            if (dest.format != format ||
                dest.width != m_header.width ||
                dest.height != m_header.height)
            {
                Bitmap temp(m_header.width, m_header.height, format);
                decodeTiles(temp);
                dest.blit(0, 0, temp);
            }
            else
            {
                decodeTiles(dest);
                status.direct = true;
            }

            return status;
        }

        void decodeTiles(const Surface& dest)
        {
            const int xs = ceil_div(m_header.width, m_xtile);
            const int ys = ceil_div(m_header.height, m_ytile);

            ConcurrentQueue q;

            LittleEndianConstPointer p = m_memory.address;

            for (int y = 0; y < ys; ++y)
            {
                for (int x = 0; x < xs; ++x)
                {
                    Surface rect(dest, x * m_xtile, y * m_ytile, m_xtile, m_ytile);

                    u32 size = p.read32();
                    ConstMemory memory(p, size);
                    p += size;

                    q.enqueue([rect, memory]
                    {
                        int w = rect.width;
                        int h = rect.height;
                        qoi_decode(rect.image, memory.address, memory.size, w, h, rect.stride);
                    });
                }
            }

            q.wait();
        }
    };

    ImageDecoderInterface* create_decoder_toi(ConstMemory memory)
    {
        ImageDecoderInterface* x = new ImageDecoderTOI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // encode_toi
    // ------------------------------------------------------------

    ImageEncodeStatus encode_toi(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

        const int xtile = 64;
        const int ytile = 64;

        const int width = surface.width;
        const int height = surface.height;
        const int xs = ceil_div(width, xtile);
        const int ys = ceil_div(height, ytile);

        Surface source = surface;
        std::unique_ptr<u8[]> temp;

        Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

        if (surface.format != format ||
            surface.width != width ||
            surface.height != height)
        {
            temp.reset(new u8[width * height * 4]);
            source = Surface(width, height, format, width * 4, temp.get());
            source.blit(0, 0, surface);
        }

        std::vector<Memory> encode_memory(xs * ys);

        ConcurrentQueue q;

        for (int y = 0; y < ys; ++y)
        {
            for (int x = 0; x < xs; ++x)
            {
                Surface rect(source, x * xtile, y * ytile, xtile, ytile);
                int idx = y * xs + x;

                q.enqueue([rect, idx, &encode_memory]
                {
                    size_t length;
                    u8* encode_ptr = qoi_encode(rect.image, rect.stride, rect.width, rect.height, &length);
                    encode_memory[idx] = Memory(encode_ptr, length);
                });
            }
        }

        q.wait();

        LittleEndianStream s = stream;

        // write header
        s.write32(TOI_HEADER_MAGIC);
        s.write32(width);
        s.write32(height);
        s.write16(xtile);
        s.write16(ytile);

        for (auto memory : encode_memory)
        {
            s.write32(memory.size);
            s.write(memory.address, memory.size);
            delete[] memory.address;
        }

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageDecoderQOI()
    {
        registerImageDecoder(create_decoder_qoi, ".qoi");
        registerImageDecoder(create_decoder_toi, ".toi");
        registerImageEncoder(encode_qoi, ".qoi");
        registerImageEncoder(encode_toi, ".toi");
    }

} // namespace mango::image
