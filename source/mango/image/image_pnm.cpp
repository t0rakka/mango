/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <cctype>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    const char* skipComment(const char* p, const char* end)
    {
        for (;;)
        {
            if (p >= end)
                break;

            if (*p++ == '\n')
                break;
        }
        return p;
    }

    inline
    const char* nextValue(int& value, const char* p, const char* end)
    {
        if (!p)
        {
            return nullptr;
        }

        for (;;)
        {
            if (p >= end)
            {
                return nullptr;
            }

            char c = *p;
            if (std::isdigit((unsigned char)c) || c == '-')
                break;

            if (c == '#')
                p = skipComment(p, end);
            else
                ++p;
        }

        // Bounded integer parse: the input buffer is not guaranteed to be NUL terminated,
        // so std::strtol could read past the end. Parse digits explicitly against 'end'.
        bool negative = false;
        if (p < end && *p == '-')
        {
            negative = true;
            ++p;
        }

        long v = 0;
        while (p < end && std::isdigit((unsigned char)*p))
        {
            v = v * 10 + (*p - '0');
            if (v > 0x7fffffff)
                v = 0x7fffffff; // clamp, validated against dimensions later
            ++p;
        }

        value = int(negative ? -v : v);

        // skip the delimiter (matches the previous strtol-based behavior)
        return p + 1;
    }

    const char* nextLine(const char* p, const char* end)
    {
        if (!p)
        {
            return nullptr;
        }

        for (;;)
        {
            if (p >= end)
            {
                return nullptr;
            }

            char c = *p++;
            if (c == '\n')
                break;
        }
        return p;
    }

    // ------------------------------------------------------------
    // HeaderPNM
    // ------------------------------------------------------------

    struct HeaderPNM
    {
        int width;
        int height;
        int channels;
        int maxvalue;
        int endian;

        bool is_ascii;
        bool is_float;
        const char* data;
        u8 invert;

        ImageHeader header;

        void setDataError()
        {
            header.setError("Decoding error (out of data).");
        }

        HeaderPNM(ConstMemory memory)
            : width(0)
            , height(0)
            , channels(0)
            , maxvalue(0)
            , endian(0)
            , is_ascii(false)
            , is_float(false)
            , data(nullptr)
            , invert(0)
        {
            const char* p = reinterpret_cast<const char *>(memory.address);
            const char* end = reinterpret_cast<const char *>(memory.end());

            // need at least the 3-byte magic before touching the bytes below
            if (memory.size < 3)
            {
                header.setError("[ImageDecoder.PNM] Not enough data.");
                return;
            }

            printLine(Print::Info, "[Header: {:c}{:c}]", p[0], p[1]);

            if (!std::strncmp(p, "Pf\n", 3))
            {
                p = nextLine(p, end);
                p = nextValue(width, p, end);
                p = nextValue(height, p, end);
                p = nextValue(endian, p, end);
                p = nextLine(p, end);

                if (!p)
                {
                    setDataError();
                    return;
                }

                channels = 1;
                maxvalue = 255;
                is_float = true;
            }
            else if (!std::strncmp(p, "PF\n", 3))
            {
                p = nextLine(p, end);
                p = nextValue(width, p, end);
                p = nextValue(height, p, end);
                p = nextValue(endian, p, end);
                p = nextLine(p, end);

                if (!p)
                {
                    setDataError();
                    return;
                }

                channels = 3;
                maxvalue = 255;
                is_float = true;
            }
            else if (!std::strncmp(p, "P7\n", 3))
            {
                char type[100];

                p = nextLine(p, end);
                if (!p)
                {
                    setDataError();
                    return;
                }

                if (std::sscanf(p, "WIDTH %i", &width) < 1)
                {
                    header.setError("[ImageDecoder.PNM] Incorrect WIDTH.");
                    return;
                }

                p = nextLine(p, end);
                if (!p)
                {
                    setDataError();
                    return;
                }

                if (std::sscanf(p, "HEIGHT %i", &height) < 1)
                {
                    header.setError("[ImageDecoder.PNM] Incorrect HEIGHT.");
                    return;
                }

                p = nextLine(p, end);
                if (!p)
                {
                    setDataError();
                    return;
                }

                if (std::sscanf(p, "DEPTH %i", &channels) < 1)
                {
                    header.setError("[ImageDecoder.PNM] Incorrect DEPTH.");
                    return;
                }

                p = nextLine(p, end);
                if (!p)
                {
                    setDataError();
                    return;
                }

                if (std::sscanf(p, "MAXVAL %i", &maxvalue) < 1)
                {
                    header.setError("[ImageDecoder.PNM] Incorrect MAXVAL.");
                    return;
                }

                p = nextLine(p, end);
                if (!p)
                {
                    setDataError();
                    return;
                }

                if (std::sscanf(p, "TUPLTYPE %99s", type) > 0)
                {
                    printLine(Print::Info, "  tupltype: {}", type);
                    /*
                    if (!strncmp(type, "BLACKANDWHITE_ALPHA", strlen("BLACKANDWHITE_ALPHA")))
                    {
                        ++header.channels;
                    }
                    else if (!strncmp(type, "GRAYSCALE_ALPHA", strlen("GRAYSCALE_ALPHA")))
                    {
                        ++header.channels;
                    }
                    else if (!strncmp(type, "RGB_ALPHA", strlen("RGB_ALPHA")))
                    {
                        ++header.channels;
                    }
                    else
                    {
                        // custom type
                    }
                    */
                }

                p = nextLine(p, end);
                if (!p)
                {
                    setDataError();
                    return;
                }

                if (std::strncmp(p, "ENDHDR", 6))
                {
                    header.setError("[ImageDecoder.PNM] Incorrect ENDHDR.");
                    return;
                }
            }
            else
            {
                if (!std::strncmp(p, "P1", 2))
                {
                    is_ascii = true;
                    channels = 1;
                    maxvalue = 1;
                    invert = 0xff;
                }
                else if (!std::strncmp(p, "P2", 2))
                {
                    is_ascii = true;
                    channels = 1;
                }
                else if (!std::strncmp(p, "P3", 2))
                {
                    is_ascii = true;
                    channels = 3;
                }
                else if (!std::strncmp(p, "P4", 2))
                {
                    channels = 1;
                    maxvalue = 1;
                    invert = 0xff;
                }
                else if (!std::strncmp(p, "P5", 2))
                {
                    channels = 1;
                }
                else if (!std::strncmp(p, "P6", 2))
                {
                    channels = 3;
                }
                else
                {
                    header.setError("[ImageDecoder.PNM] Incorrect header magic ({}).", p);
                    return;
                }

                p += 3; // skip header magic
                p = nextValue(width, p, end);
                p = nextValue(height, p, end);

                if (!maxvalue)
                {
                    p = nextValue(maxvalue, p, end);
                }

                if (!p)
                {
                    setDataError();
                    return;
                }
            }

            printLine(Print::Info, "  image: {} x {}, channels: {}", width, height, channels);
            printLine(Print::Info, "  maxvalue: {}", maxvalue);

            // 0x100000 (1M) per dimension is far beyond any sane PNM and keeps width *
            // channels and the byte totals safely inside int / size_t range below.
            if (width < 1 || height < 1 || width > 0x100000 || height > 0x100000)
            {
                header.setError("[ImageDecoder.PNM] Incorrect image dimensions ({} x {}).", width, height);
                return;
            }

            if (maxvalue < 1 || maxvalue > 65535)
            {
                header.setError("[ImageDecoder.PNM] Incorrect maxvalue.");
                return;
            }

            Format format;

            if (is_float)
            {
                switch (channels)
                {
                    case 1: format = Format(32, Format::FLOAT32, Color(32, 32, 32, 0), Color(0, 0, 0, 0)); break;
                    case 3: format = Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32, 0); break;
                    default:
                        header.setError("[ImageDecoder.PNM] Incorrect number of channels.");
                        return;
                }
            }
            else
            {
                switch (channels)
                {
                    case 1: format = LuminanceFormat(8, Format::UNORM, 8, 0); break;
                    case 2: format = LuminanceFormat(16, Format::UNORM, 8, 8); break;
                    case 3: format = Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0); break;
                    case 4: format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8); break;
                    default:
                        header.setError("[ImageDecoder.PNM] Incorrect number of channels.");
                        return;
                }
            }

            data = p;

            header.width   = width;
            header.height  = height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
            header.format  = format;
            header.compression = TextureCompression::NONE;
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        HeaderPNM m_pnm_header;

        Interface(ConstMemory memory)
            : m_memory(memory)
            , m_pnm_header(memory)
        {
            header = m_pnm_header.header;
        }

        ~Interface()
        {
        }

        bool decode_matching(const Surface& dest)
        {
            const char* p = m_pnm_header.data;
            const char* end = reinterpret_cast<const char *>(m_memory.end());

            if (!p || p > end)
            {
                return false;
            }

            const int xcount = m_pnm_header.width * m_pnm_header.channels;
            const int height = m_pnm_header.height;
            int maxvalue = m_pnm_header.maxvalue;
            u8 invert = m_pnm_header.invert;

            // available raw bytes from the start of the pixel data
            const size_t avail = size_t(end - p);

            if (m_pnm_header.is_ascii)
            {
                for (int y = 0; y < height; ++y)
                {
                    u8* image = dest.address<u8>(0, y);

                    for (int x = 0; x < xcount; ++x)
                    {
                        int value;
                        p = nextValue(value, p, end);
                        if (!p)
                        {
                            return false;
                        }

                        // clamp to [0, maxvalue] so the scaling cannot overflow
                        if (value < 0)
                            value = 0;
                        else if (value > maxvalue)
                            value = maxvalue;

                        image[x] = u8(value * 255 / maxvalue) ^ invert;
                    }
                }
            }
            else if (m_pnm_header.is_float)
            {
                // float samples: xcount * height 32-bit values
                if (avail < size_t(xcount) * size_t(height) * 4)
                {
                    return false;
                }

                if (m_pnm_header.endian < 0)
                {
                    LittleEndianConstPointer ptr = p;

                    for (int y = 0; y < m_pnm_header.height; ++y)
                    {
                        float* image = dest.address<float>(0, m_pnm_header.height - 1 - y);

                        for (int x = 0; x < xcount; ++x)
                        {
                            image[x] = ptr.read32f();
                        }
                    }
                }
                else
                {
                    BigEndianConstPointer ptr = p;

                    for (int y = 0; y < m_pnm_header.height; ++y)
                    {
                        float* image = dest.address<float>(0, m_pnm_header.height - 1 - y);

                        for (int x = 0; x < xcount; ++x)
                        {
                            image[x] = ptr.read32f();
                        }
                    }
                }
            }
            else
            {
                if (m_pnm_header.invert)
                {
                    // 1-bit packed data, each scanline byte-aligned
                    const size_t bytes_per_scan = (size_t(xcount) + 7) / 8;
                    if (avail < bytes_per_scan * size_t(height))
                    {
                        return false;
                    }

                    for (int y = 0; y < height; ++y)
                    {
                        u8* image = dest.address<u8>(0, y);

                        // 1 bit data is byte aligned; reset data register
                        u8 data = 0;
                        int size = 0;

                        for (int x = 0; x < xcount; ++x)
                        {
                            if (!size)
                            {
                                // out of data; fetch a byte
                                data = *p++;
                                size = 8;
                            }

                            // extract one bit in reverse order (MSB-first)
                            --size;
                            u8 sample = (data >> size) & 1;

                            // invert the sample
                            // 0 --> 255
                            // 1 --> 0
                            image[x] = sample + 0xff;
                        }
                    }
                }
                else if (m_pnm_header.maxvalue <= 255)
                {
                    // one byte per sample
                    if (avail < size_t(xcount) * size_t(height))
                    {
                        return false;
                    }

                    for (int y = 0; y < height; ++y)
                    {
                        u8* image = dest.address<u8>(0, y);
                        std::memcpy(image, p, xcount);
                        p += xcount;
                    }
                }
                else
                {
                    // two bytes per sample
                    if (avail < size_t(xcount) * size_t(height) * 2)
                    {
                        return false;
                    }

                    BigEndianConstPointer e = p;

                    for (int y = 0; y < height; ++y)
                    {
                        u8* image = dest.address<u8>(0, y);

                        for (int x = 0; x < xcount; ++x)
                        {
                            int value = e.read16();
                            image[x] = u8(value * 255 / m_pnm_header.maxvalue);
                        }
                    }
                }
            }

            return true;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            DecodeTargetBitmap target(dest, header.width, header.height, header.format);

            status.success = decode_matching(target);
            status.direct = target.isDirect();

            if (status.success)
            {
                target.resolve();
            }

            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    ImageEncodeStatus imageEncodePFM(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        int width = surface.width;
        int height = surface.height;

        LittleEndianStream s(stream);

        std::string header = fmt::format("PF\n{} {}\n-1.0\n", width, height);
        s.write(header.c_str(), header.length());

        TemporaryBitmap temp(surface, Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32));

        for (int y = height - 1; y >= 0; --y)
        {
            float* scan = temp.address<float>(0, y);

            for (int x = 0; x < width; ++x)
            {
                s.write32f(scan[0]);
                s.write32f(scan[1]);
                s.write32f(scan[2]);
                scan += 4;
            }
        }

        ImageEncodeStatus status;
        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecPNM()
    {
        registerImageDecoder(createInterface, ".pbm");
        registerImageDecoder(createInterface, ".pgm");
        registerImageDecoder(createInterface, ".ppm");
        registerImageDecoder(createInterface, ".pam");
        registerImageDecoder(createInterface, ".pfm");

        registerImageEncoder(imageEncodePFM, ".pfm");
    }

} // namespace mango::image
