/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
//#define MANGO_ENABLE_DEBUG_PRINT

#include <cmath>
#include <cctype>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#define ID "[ImageDecoder.PNM] "

namespace
{
    using namespace mango;

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
        for (;;)
        {
            if (p >= end)
            {
                MANGO_EXCEPTION(ID"Decoding error (out of data).");
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
        for (;;)
        {
            if (p >= end)
            {
                MANGO_EXCEPTION(ID"Decoding error (out of data).");
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

		Format format;
		bool is_ascii;
        bool is_float;
        const char* data;
        u8 invert;

        HeaderPNM(Memory memory)
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

                channels = 3;
                maxvalue = 255;
                is_float = true;
            }
            else if (!std::strncmp(p, "P7\n", 3))
            {
                char type[100];

                p = nextLine(p, end);
                if (std::sscanf(p, "WIDTH %i", &width) < 1)
                    MANGO_EXCEPTION(ID"Incorrect width");

                p = nextLine(p, end);
                if (std::sscanf(p, "HEIGHT %i", &height) < 1)
                    MANGO_EXCEPTION(ID"Incorrect height");

                p = nextLine(p, end);
                if (std::sscanf(p, "DEPTH %i", &channels) < 1)
                    MANGO_EXCEPTION(ID"Incorrect depth");

                p = nextLine(p, end);
                if (std::sscanf(p, "MAXVAL %i", &maxvalue) < 1)
                    MANGO_EXCEPTION(ID"Incorrect maxval");

                p = nextLine(p, end);
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
                if (std::strncmp(p, "ENDHDR", 6))
                    MANGO_EXCEPTION(ID"Incorrect endhdr");
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
                    MANGO_EXCEPTION(ID"Incorrect header magic (%s)", p);
                }

                p += 3; // skip header magic
                p = nextValue(width, p, end);
                p = nextValue(height, p, end);

                if (!maxvalue)
                {
                    p = nextValue(maxvalue, p, end);
                }
            }

            debugPrint("  image: %d x %d, channels: %d\n", width, height, channels);
            debugPrint("  maxvalue: %d\n", maxvalue);

            if (maxvalue < 1 || maxvalue > 65535)
                MANGO_EXCEPTION(ID"Incorrect maxvalue");

            if (is_float)
            {
                switch (channels)
                {
                    case 1: format = Format(32, Format::FLOAT32, ColorRGBA(32, 32, 32, 0), ColorRGBA(0, 0, 0, 0)); break;
                    case 3: format = Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32, 0); break;
                    default:
                        MANGO_EXCEPTION(ID"Incorrect number of channels");
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
                        MANGO_EXCEPTION(ID"Incorrect number of channels");
                }
            }

            data = p;
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        Memory m_memory;
        HeaderPNM m_header;

        Interface(Memory memory)
            : m_memory(memory)
            , m_header(memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            ImageHeader header;

            header.width   = m_header.width;
            header.height  = m_header.height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = false;
            header.format  = m_header.format;
            header.compression = TextureCompression::NONE;

            return header;
        }

        bool decode_matching(Surface& dest)
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

        ImageDecodeStatus decode(Surface& dest, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(palette);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            status.direct = dest.format == m_header.format &&
                            dest.width >= m_header.width &&
                            dest.height >= m_header.height;

            if (status.direct)
            {
                decode_matching(dest);
            }
            else
            {
                Bitmap temp(m_header.width, m_header.height, m_header.format);
                decode_matching(temp);
                dest.blit(0, 0, temp);
            }

            status.success = true;
            return status;
        }
    };

    ImageDecoderInterface* createInterface(Memory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango
{

    void registerImageDecoderPNM()
    {
        registerImageDecoder(createInterface, ".pbm");
        registerImageDecoder(createInterface, ".pgm");
        registerImageDecoder(createInterface, ".ppm");
        registerImageDecoder(createInterface, ".pam");
        registerImageDecoder(createInterface, ".pfm");
    }

} // namespace mango
