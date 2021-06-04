/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
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
            if (std::isdigit(c) || c == '-')
                break;

            if (c == '#')
                p = skipComment(p, end);
            else
                ++p;
        }

        char* n;
        value = int(std::strtol(p, &n, 10));
        return n + 1;
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
            const char* end = reinterpret_cast<const char *>(memory.address + memory.size);

            debugPrint("[Header: %c%c]\n", p[0], p[1]);

            if (!std::strncmp(p, "Pf\n", 3))
            {
                p = nextLine(p, end);
                p = nextValue(width, p, end);
                p = nextValue(height, p, end);
                p = nextValue(endian, p, end);
                p = nextLine(p, end);

                if (!p)
                {
                    header.setError("Decoding error (out of data).");
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
                    header.setError("Decoding error (out of data).");
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
                    header.setError("Decoding error (out of data).");
                    return;
                }

                if (std::sscanf(p, "WIDTH %i", &width) < 1)
                {
                    header.setError("[ImageDecoder.PNM] Incorrect width");
                    return;
                }

                p = nextLine(p, end);
                if (!p)
                {
                    header.setError("Decoding error (out of data).");
                    return;
                }

                if (std::sscanf(p, "HEIGHT %i", &height) < 1)
                {
                    header.setError("[ImageDecoder.PNM] Incorrect height");
                    return;
                }

                p = nextLine(p, end);
                if (!p)
                {
                    header.setError("Decoding error (out of data).");
                    return;
                }

                if (std::sscanf(p, "DEPTH %i", &channels) < 1)
                {
                    header.setError("[ImageDecoder.PNM] Incorrect depth");
                    return;
                }

                p = nextLine(p, end);
                if (!p)
                {
                    header.setError("Decoding error (out of data).");
                    return;
                }

                if (std::sscanf(p, "MAXVAL %i", &maxvalue) < 1)
                {
                    header.setError("[ImageDecoder.PNM] Incorrect maxval");
                    return;
                }

                p = nextLine(p, end);
                if (!p)
                {
                    header.setError("Decoding error (out of data).");
                    return;
                }

                if (std::sscanf(p, "TUPLTYPE %s", type) > 0)
                {
                    debugPrint("  tupltype: %s\n", type);
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
                    header.setError("Decoding error (out of data).");
                    return;
                }

                if (std::strncmp(p, "ENDHDR", 6))
                {
                    header.setError("[ImageDecoder.PNM] Incorrect endhdr");
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
                    header.setError("[ImageDecoder.PNM] Incorrect header magic (%s)", p);
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
                    header.setError("Decoding error (out of data).");
                    return;
                }
            }

            debugPrint("  image: %d x %d, channels: %d\n", width, height, channels);
            debugPrint("  maxvalue: %d\n", maxvalue);

            if (maxvalue < 1 || maxvalue > 65535)
            {
                header.setError("[ImageDecoder.PNM] Incorrect maxvalue");
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
                        header.setError("[ImageDecoder.PNM] Incorrect number of channels");
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
                        header.setError("[ImageDecoder.PNM] Incorrect number of channels");
                        return;
                }
            }

            data = p;

            header.width   = width;
            header.height  = height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = false;
            header.format  = format;
            header.compression = TextureCompression::NONE;
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ConstMemory m_memory;
        HeaderPNM m_header;

        Interface(ConstMemory memory)
            : m_memory(memory)
            , m_header(memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header.header;
        }

        bool decode_matching(const Surface& dest)
        {
            const char* p = m_header.data;
            const char* end = reinterpret_cast<const char *>(m_memory.address + m_memory.size);

            const int xcount = m_header.width * m_header.channels;
            int maxvalue = m_header.maxvalue;
            u8 invert = m_header.invert;

            if (m_header.is_ascii)
            {
                for (int y = 0; y < m_header.height; ++y)
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

                        image[x] = u8(value * 255 / maxvalue) ^ invert;
                    }
                }
            }
            else if (m_header.is_float)
            {
                if (m_header.endian < 0)
                {
                    LittleEndianConstPointer ptr = p;

                    for (int y = 0; y < m_header.height; ++y)
                    {
                        float* image = dest.address<float>(0, m_header.height - 1 - y);

                        for (int x = 0; x < xcount; ++x)
                        {
                            image[x] = ptr.read32f();
                        }
                    }
                }
                else
                {
                    BigEndianConstPointer ptr = p;

                    for (int y = 0; y < m_header.height; ++y)
                    {
                        float* image = dest.address<float>(0, m_header.height - 1 - y);

                        for (int x = 0; x < xcount; ++x)
                        {
                            image[x] = ptr.read32f();
                        }
                    }
                }
            }
            else
            {
                if (m_header.invert)
                {
                    for (int y = 0; y < m_header.height; ++y)
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
                else if (m_header.maxvalue <= 255)
                {
                    for (int y = 0; y < m_header.height; ++y)
                    {
                        u8* image = dest.address<u8>(0, y);
                        std::memcpy(image, p, xcount);
                        p += xcount;
                    }
                }
                else
                {
                    BigEndianConstPointer e = p;

                    for (int y = 0; y < m_header.height; ++y)
                    {
                        u8* image = dest.address<u8>(0, y);

                        for (int x = 0; x < xcount; ++x)
                        {
                            int value = e.read16();
                            image[x] = u8(value * 255 / m_header.maxvalue);
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

			const ImageHeader& header = m_header.header;
            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            status.direct = dest.format == header.format &&
                            dest.width >= header.width &&
                            dest.height >= header.height;

            if (status.direct)
            {
                status.success = decode_matching(dest);
            }
            else
            {
                Bitmap temp(header.width, header.height, header.format);
                status.success = decode_matching(temp);
                dest.blit(0, 0, temp);
            }

            return status;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageDecoderPNM()
    {
        registerImageDecoder(createInterface, ".pbm");
        registerImageDecoder(createInterface, ".pgm");
        registerImageDecoder(createInterface, ".ppm");
        registerImageDecoder(createInterface, ".pam");
        registerImageDecoder(createInterface, ".pfm");
    }

} // namespace mango::image
