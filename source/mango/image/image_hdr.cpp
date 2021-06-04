/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // tokenizer
    // ------------------------------------------------------------

    std::string readline(const u8*& data, const u8* end)
    {
        const u8* p = data;

        int endsize = 1;

        // scan for endline
        for ( ; data < end; )
        {
            u8 v = *p++;

            // Unix ("\n")
            if (v == '\n')
                break;

            // MacOS ("\r")
            if (v == '\r')
            {
                // Windows ("\r\n")
                if (*p == '\n')
                {
                    ++endsize;
                    ++p;
                }

                break;
            }
        }

        int size = int(p - data) - endsize;
        std::string msg(reinterpret_cast<const char*>(data), size);

        data = p;

        return msg;
    }

    inline bool whitespace(char v)
    {
        return v == ' ' || v == '\t' || v == '=';
    }

    void insert_token(std::vector<std::string>& tokens, const char* text, int size)
    {
        if (size > 0)
        {
            std::string msg(text, size);
            tokens.push_back(msg);
        }
    }

    std::vector<std::string> tokenize(const std::string& line)
    {
        std::vector<std::string> tokens;

        const char* p = line.c_str();
        const char* endline = p + line.length();

        for ( ; p < endline;)
        {
            // skip whitespaces
            for ( ;; ++p)
            {
                char v = *p;
                if (!v) return tokens;
                if (!whitespace(v)) break;
            }

            const char* begin = p;

            // seek next whitespace
            for ( ;; ++p)
            {
                char v = *p;

                if (!v)
                {
                    int size = int(p - begin);
                    insert_token(tokens, begin, size);
                    return tokens;
                }

                if (whitespace(v))
                    break;
            }

            int size = int(p - begin);
            insert_token(tokens, begin, size);
        }

        return tokens;
    }

    // ------------------------------------------------------------
    // encoder
    // ------------------------------------------------------------

    struct rgbe
    {
        u8 r, g, b, e;
    };

    /*
    rgbe create_rgbe(float r, float g, float b)
    {
        float maxf = r > g ? r : g;
        maxf = maxf > b ? maxf : b;
    
        rgbe color;

        if (maxf <= 1e-32f)
        {
            color.r = 0;
            color.g = 0;
            color.b = 0;
            color.e = 0;
        }
        else
        {
            int exponent;
            std::frexpf(maxf, &exponent);
            float scale = std::ldexpf(1.0f, 8 - exponent);

            color.r = u8(r * scale);
            color.g = u8(g * scale);
            color.b = u8(b * scale);
            color.e = u8(exponent + 128);
        }

        return color;
    }
    */

    // ------------------------------------------------------------
    // decoder
    // ------------------------------------------------------------

    void write_rgbe(float* buffer, rgbe color)
    {
        float scale = color.e ? std::ldexp(1.0f, color.e - 136) : 0;
        buffer[0] = color.r * scale;
        buffer[1] = color.g * scale;
        buffer[2] = color.b * scale;
        buffer[3] = 1.0f;
    }

    struct HeaderRAD
    {
        enum rad_format
        {
            rad_rle_rgbe,
            rad_unsupported
        } format = rad_unsupported;

        int width = 0;
        int height = 0;
        float exposure = 1.0f;
        bool xflip = false;
        bool yflip = false;

        ImageHeader header;

        const u8* parse(ConstMemory memory)
        {
            const u8* data = memory.address;
            const u8* end = memory.address + memory.size;

            std::string id = readline(data, end);
            if (id != "#?RADIANCE")
            {
                header.setError("[ImageDecoder.HDR] Incorrect radiance header.");
                return nullptr;
            }

            for ( ;; )
            {
                std::string ln = readline(data, end);
                if (ln.empty())
                    break;

                std::vector<std::string> tokens = tokenize(ln);

                if (tokens[0] == "FORMAT")
                {
                    if (tokens.size() != 2)
                    {
                        header.setError("[ImageDecoder.HDR] Incorrect radiance header (format).");
                        return nullptr;
                    }

                    if (tokens[1] == "32-bit_rle_rgbe")
                        format = rad_rle_rgbe;
                }
                else if (tokens[1] == "EXPOSURE")
                {
                    if (tokens.size() != 2)
                    {
                        header.setError("[ImageDecoder.HDR] Incorrect radiance header (exposure).");
                        return nullptr;
                    }

                    exposure = float(std::atof(tokens[1].c_str()));
                }
            }

            if (format == rad_unsupported)
            {
                header.setError("[ImageDecoder.HDR] Incorrect or unsupported format.");
                return nullptr;
            }

            std::string dims = readline(data, end);
            std::vector<std::string> tokens = tokenize(dims);

            if (tokens.size() != 4)
            {
                header.setError("[ImageDecoder.HDR] Incorrect radiance header (dimensions).");
                return nullptr;
            }

            for (int i = 0; i < 2; ++i)
            {
                int index = i * 2;
                if (tokens[index] == "+Y")
                {
                    yflip = false;
                    height = std::atoi(tokens[index + 1].c_str());
                }
                else if (tokens[index] == "-Y")
                {
                    yflip = true;
                    height = std::atoi(tokens[index + 1].c_str());
                }
                else if (tokens[index] == "+X")
                {
                    xflip = false;
                    width = std::atoi(tokens[index + 1].c_str());
                }
                else if (tokens[index] == "-X")
                {
                    xflip = true;
                    width = std::atoi(tokens[index + 1].c_str());
                }
                else
                {
                    header.setError("[ImageDecoder.HDR] Incorrect radiance header (dimensions).");
                    return nullptr;
                }
            }

            if (!width || !height)
            {
                header.setError("[ImageDecoder.HDR] Incorrect radiance header (dimensions).");
                return nullptr;
            }

            header.width   = width;
            header.height  = height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
            header.palette = false;
            header.format  = Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32);
            header.compression = TextureCompression::NONE;

            return data;
        }
    };

    void hdr_decode(ImageDecodeStatus& status, const Surface& surface, const u8* data)
    {
        Buffer buffer(surface.width * 4);

        for (int y = 0; y < surface.height; ++y)
        {
            if (data[0] != 2 || data[1] != 2 || data[2] & 0x80)
            {
                status.setError("[ImageDecoder.HDR] Incorrect rle_rgbe stream (wrong header).");
                return;
            }

            if (((data[2] << 8) | data[3]) != surface.width)
            {
                status.setError("[ImageDecoder.HDR] Incorrect rle_rgbe stream (wrong scan).");
                return;
            }

            data += 4;

            for (int channel = 0; channel < 4; ++channel)
            {
                u8* dest = buffer + surface.width * channel;
                u8* end = dest + surface.width;

                while (dest < end)
                {
                    int count = *data++;

                    if (count > 128)
                    {
                        count -= 128;
                        if (!count || dest + count > end)
                        {
                            status.setError("[ImageDecoder.HDR] Incorrect rle_rgbe stream (rle count).");
                            return;
                        }

                        u8 value = *data++;
                        std::memset(dest, value, count);
                        dest += count;
                    }
                    else
                    {
                        if (!count || dest + count > end)
                        {
                            status.setError("[ImageDecoder.HDR] Incorrect rle_rgbe stream (rle count).");
                            return;
                        }

                        std::memcpy(dest, data, count);
                        dest += count;
                        data += count;
                    }
                }
            }

            float* image = surface.address<float>(0, y);

            for (int x = 0; x < surface.width; ++x)
            {
                rgbe color;
                color.r = buffer[x + surface.width * 0];
                color.g = buffer[x + surface.width * 1];
                color.b = buffer[x + surface.width * 2];
                color.e = buffer[x + surface.width * 3];

                write_rgbe(image, color);
                image += 4;
            }
        }
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        HeaderRAD m_rad_header;
        const u8* m_data;

        Interface(ConstMemory memory)
        {
            m_data = m_rad_header.parse(memory);
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_rad_header.header;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            const ImageHeader& header = m_rad_header.header;
            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            status.direct = dest.format == header.format &&
                            dest.width == header.width &&
                            dest.height == header.height;

            if (status.direct)
            {
                hdr_decode(status, dest, m_data);
            }
            else
            {
                Bitmap temp(header.width, header.height, header.format);
                hdr_decode(status, temp, m_data);
                if (status)
                {
                    dest.blit(0, 0, temp);
                }
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

    void registerImageDecoderHDR()
    {
        registerImageDecoder(createInterface, ".hdr");
    }

} // namespace mango::image
