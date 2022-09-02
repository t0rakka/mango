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

    static constexpr u32 QOI_HEADER_MAGIC = u32_mask('q', 'o', 'i', 'f');
    static constexpr u32 QOI_HEADER_SIZE = 14;

    enum : u8
    {
        QOI_SRGB = 0x00,
        QOI_LINEAR = 0x01
    };

    #define QOI_OP_INDEX  0x00 // 00xxxxxx
    #define QOI_OP_DIFF   0x40 // 01xxxxxx
    #define QOI_OP_LUMA   0x80 // 10xxxxxx
    #define QOI_OP_RUN    0xc0 // 11xxxxxx
    #define QOI_OP_RGB    0xfe // 11111110
    #define QOI_OP_RGBA   0xff // 11111111

    #define QOI_MASK_2    0xc0 // 11000000

    #define QOI_COLOR_HASH(C) (C.r * 3 + C.g * 5 + C.b * 7 + C.a * 11)
    #define QOI_PADDING 8

    static inline
    bool is_diff(int r, int g, int b)
    {
        return r > -3 && r < 2 &&
               g > -3 && g < 2 && 
               b > -3 && b < 2;
    }

    static inline
    bool is_luma(int r, int g, int b)
    {
        return r > -9  && r <  8 &&
               g > -33 && g < 32 && 
               b > -9  && b <  8;
    }

    u8* qoi_encode(const u8* image, size_t stride, int width, int height, size_t* out_len)
    {
        if (image == nullptr || out_len == nullptr ||
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
        int p_end = (width * height - 1) * channels;

        Color cache[64] = { 0 };

        int run = 0;
        Color prev(0, 0, 0, 255);
        Color color = prev;

        for (int y = 0; y < height; ++y)
        {
            const Color* src = reinterpret_cast<const Color*>(image);

            for (int x = 0; x < width; ++x)
            {
                color = src[x];

                if (color == prev)
                {
                    run++;
                    if (run == 62 || p == p_end)
                    {
                        bytes[p++] = QOI_OP_RUN | (run - 1);
                        run = 0;
                    }
                }
                else
                {
                    if (run > 0)
                    {
                        bytes[p++] = QOI_OP_RUN | (run - 1);
                        run = 0;
                    }

                    int index = QOI_COLOR_HASH(color) % 64;

                    if (cache[index] == color)
                    {
                        bytes[p++] = QOI_OP_INDEX | index;
                    }
                    else
                    {
                        cache[index] = color;

                        if (color.a == prev.a)
                        {
                            s8 vr = color.r - prev.r;
                            s8 vg = color.g - prev.g;
                            s8 vb = color.b - prev.b;

                            s8 vg_r = vr - vg;
                            s8 vg_b = vb - vg;

                            if (is_diff(vr, vg, vb))
                            {
                                bytes[p++] = QOI_OP_DIFF | (vr + 2) << 4 | (vg + 2) << 2 | (vb + 2);
                            }
                            else if (is_luma(vg_r, vg, vg_b))
                            {
                                bytes[p + 0] = QOI_OP_LUMA     | (vg   + 32);
                                bytes[p + 1] = (vg_r + 8) << 4 | (vg_b +  8);
                                p += 2;
                            }
                            else
                            {
                                bytes[p + 0] = QOI_OP_RGB;
                                bytes[p + 1] = color.r;
                                bytes[p + 2] = color.g;
                                bytes[p + 3] = color.b;
                                p += 4;
                            }
                        }
                        else
                        {
                            bytes[p + 0] = QOI_OP_RGBA;
                            bytes[p + 1] = color.r;
                            bytes[p + 2] = color.g;
                            bytes[p + 3] = color.b;
                            bytes[p + 4] = color.a;
                            p += 5;
                        }
                    }
                }

                prev = color;
            }

            image += stride;
        }

        // padding
        bytes[p + 0] = 0;
        bytes[p + 1] = 0;
        bytes[p + 2] = 0;
        bytes[p + 3] = 0;
        bytes[p + 4] = 0;
        bytes[p + 5] = 0;
        bytes[p + 6] = 0;
        bytes[p + 7] = 1;
        p += 8;

        *out_len = p;
        return bytes;
    }

    void qoi_decode(u8* image, const u8* data, size_t size, int width, int height, size_t stride)
    {
        Color color(0, 0, 0, 255);
        Color cache[64] = { 0 };

        const u8* end = data + size;
        int run = 0;

        for (int y = 0; y < height; ++y)
        {
            Color* dest = reinterpret_cast<Color*>(image);
            Color* xend = dest + width;

            for ( ; dest < xend; )
            {
                if (run > 0)
                {
                    run--;
                }
                else if (data < end)
                {
                    int b1 = *data++;

                    if (b1 == QOI_OP_RGB)
                    {
                        color.r = data[0];
                        color.g = data[1];
                        color.b = data[2];
                        data += 3;
                    }
                    else if (b1 == QOI_OP_RGBA)
                    {
                        color.r = data[0];
                        color.g = data[1];
                        color.b = data[2];
                        color.a = data[3];
                        data += 4;
                    }
                    else if ((b1 & QOI_MASK_2) == QOI_OP_INDEX)
                    {
                        color = cache[b1];
                    }
                    else if ((b1 & QOI_MASK_2) == QOI_OP_DIFF)
                    {
                        color.r += ((b1 >> 4) & 0x03) - 2;
                        color.g += ((b1 >> 2) & 0x03) - 2;
                        color.b += ( b1       & 0x03) - 2;
                    }
                    else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA)
                    {
                        int b2 = *data++;
                        int vg = (b1 & 0x3f) - 32;
                        color.r += vg - 8 + ((b2 >> 4) & 0x0f);
                        color.g += vg;
                        color.b += vg - 8 +  (b2       & 0x0f);
                    }
                    else if ((b1 & QOI_MASK_2) == QOI_OP_RUN)
                    {
                        run = (b1 & 0x3f);
                    }

                    cache[QOI_COLOR_HASH(color) % 64] = color;
                }

                *dest++ = color;
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
                    break;
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
        s.write8(QOI_SRGB);

        // write encoded image
        s.write(buffer, u64(size));

        delete[] buffer;
    
        return status;
    }

    // ------------------------------------------------------------
    // ImageDecoderTOI
    // ------------------------------------------------------------

    static constexpr u32 TOI_HEADER_MAGIC = u32_mask('t', 'o', 'i', 'f');
    static constexpr u32 TOI_HEADER_SIZE = 16;

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

        const int xtile = 128;
        const int ytile = 128;

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

        LittleEndianStream s = stream;

        // write header
        s.write32(TOI_HEADER_MAGIC);
        s.write32(width);
        s.write32(height);
        s.write16(xtile);
        s.write16(ytile);

        ConcurrentQueue q;
        TicketQueue tk;

        for (int y = 0; y < ys; ++y)
        {
            for (int x = 0; x < xs; ++x)
            {
                Surface rect(source, x * xtile, y * ytile, xtile, ytile);

                auto ticket = tk.acquire();

                q.enqueue([ticket, rect, &s]
                {
                    size_t length;
                    u8* encode_ptr = qoi_encode(rect.image, rect.stride, rect.width, rect.height, &length);

                    ticket.consume([=, &s]
                    {
                        s.write32(u32(length));
                        s.write(encode_ptr, length);
                        delete[] encode_ptr;
                    });
                });
            }
        }

        q.wait();
        tk.wait();

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
