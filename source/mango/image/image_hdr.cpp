/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <string_view>
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

    std::string_view readline(const u8*& data, const u8* end)
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

        size_t size = size_t(p - data) - endsize;
        std::string_view msg(reinterpret_cast<const char*>(data), size);

        data = p;

        return msg;
    }

    inline
    bool whitespace(char v)
    {
        return v == ' ' || v == '\t' || v == '=';
    }

    void insert_token(std::vector<std::string_view>& tokens, const char* text, size_t size)
    {
        if (size > 0)
        {
            tokens.emplace_back(text, size);
        }
    }

    std::vector<std::string_view> tokenize(std::string_view line)
    {
        std::vector<std::string_view> tokens;

        const char* p = line.data();
        const char* endline = p + line.length();

        for ( ; p < endline;)
        {
            // skip whitespaces
            for ( ;; ++p)
            {
                if (p >= endline)
                    return tokens;

                if (!whitespace(*p))
                    break;
            }

            const char* begin = p;

            // seek next whitespace
            for ( ;; ++p)
            {
                if (p >= endline)
                    break;

                if (whitespace(*p))
                    break;
            }

            size_t size = size_t(p - begin);
            insert_token(tokens, begin, size);
        }

        return tokens;
    }

    // ------------------------------------------------------------
    // rgbe
    // ------------------------------------------------------------

    static constexpr
    u32 g_exponent_table [] =
    {
        0x00000000, 0x00004000, 0x00008000, 0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000, 
        0x00200000, 0x00400000, 0x00800000, 0x01000000, 0x01800000, 0x02000000, 0x02800000, 0x03000000, 
        0x03800000, 0x04000000, 0x04800000, 0x05000000, 0x05800000, 0x06000000, 0x06800000, 0x07000000, 
        0x07800000, 0x08000000, 0x08800000, 0x09000000, 0x09800000, 0x0a000000, 0x0a800000, 0x0b000000, 
        0x0b800000, 0x0c000000, 0x0c800000, 0x0d000000, 0x0d800000, 0x0e000000, 0x0e800000, 0x0f000000, 
        0x0f800000, 0x10000000, 0x10800000, 0x11000000, 0x11800000, 0x12000000, 0x12800000, 0x13000000, 
        0x13800000, 0x14000000, 0x14800000, 0x15000000, 0x15800000, 0x16000000, 0x16800000, 0x17000000, 
        0x17800000, 0x18000000, 0x18800000, 0x19000000, 0x19800000, 0x1a000000, 0x1a800000, 0x1b000000, 
        0x1b800000, 0x1c000000, 0x1c800000, 0x1d000000, 0x1d800000, 0x1e000000, 0x1e800000, 0x1f000000, 
        0x1f800000, 0x20000000, 0x20800000, 0x21000000, 0x21800000, 0x22000000, 0x22800000, 0x23000000, 
        0x23800000, 0x24000000, 0x24800000, 0x25000000, 0x25800000, 0x26000000, 0x26800000, 0x27000000, 
        0x27800000, 0x28000000, 0x28800000, 0x29000000, 0x29800000, 0x2a000000, 0x2a800000, 0x2b000000, 
        0x2b800000, 0x2c000000, 0x2c800000, 0x2d000000, 0x2d800000, 0x2e000000, 0x2e800000, 0x2f000000, 
        0x2f800000, 0x30000000, 0x30800000, 0x31000000, 0x31800000, 0x32000000, 0x32800000, 0x33000000, 
        0x33800000, 0x34000000, 0x34800000, 0x35000000, 0x35800000, 0x36000000, 0x36800000, 0x37000000, 
        0x37800000, 0x38000000, 0x38800000, 0x39000000, 0x39800000, 0x3a000000, 0x3a800000, 0x3b000000, 
        0x3b800000, 0x3c000000, 0x3c800000, 0x3d000000, 0x3d800000, 0x3e000000, 0x3e800000, 0x3f000000, 
        0x3f800000, 0x40000000, 0x40800000, 0x41000000, 0x41800000, 0x42000000, 0x42800000, 0x43000000, 
        0x43800000, 0x44000000, 0x44800000, 0x45000000, 0x45800000, 0x46000000, 0x46800000, 0x47000000, 
        0x47800000, 0x48000000, 0x48800000, 0x49000000, 0x49800000, 0x4a000000, 0x4a800000, 0x4b000000, 
        0x4b800000, 0x4c000000, 0x4c800000, 0x4d000000, 0x4d800000, 0x4e000000, 0x4e800000, 0x4f000000, 
        0x4f800000, 0x50000000, 0x50800000, 0x51000000, 0x51800000, 0x52000000, 0x52800000, 0x53000000, 
        0x53800000, 0x54000000, 0x54800000, 0x55000000, 0x55800000, 0x56000000, 0x56800000, 0x57000000, 
        0x57800000, 0x58000000, 0x58800000, 0x59000000, 0x59800000, 0x5a000000, 0x5a800000, 0x5b000000, 
        0x5b800000, 0x5c000000, 0x5c800000, 0x5d000000, 0x5d800000, 0x5e000000, 0x5e800000, 0x5f000000, 
        0x5f800000, 0x60000000, 0x60800000, 0x61000000, 0x61800000, 0x62000000, 0x62800000, 0x63000000, 
        0x63800000, 0x64000000, 0x64800000, 0x65000000, 0x65800000, 0x66000000, 0x66800000, 0x67000000, 
        0x67800000, 0x68000000, 0x68800000, 0x69000000, 0x69800000, 0x6a000000, 0x6a800000, 0x6b000000, 
        0x6b800000, 0x6c000000, 0x6c800000, 0x6d000000, 0x6d800000, 0x6e000000, 0x6e800000, 0x6f000000, 
        0x6f800000, 0x70000000, 0x70800000, 0x71000000, 0x71800000, 0x72000000, 0x72800000, 0x73000000, 
        0x73800000, 0x74000000, 0x74800000, 0x75000000, 0x75800000, 0x76000000, 0x76800000, 0x77000000, 
        0x77800000, 0x78000000, 0x78800000, 0x79000000, 0x79800000, 0x7a000000, 0x7a800000, 0x7b000000, 
    };

    static inline
    float getScale(u8 exponent)
    {
        const float* table = reinterpret_cast<const float*>(g_exponent_table);
        return table[exponent];
        //return std::ldexpf(1.0f, exponent - 136);
    }

    // ------------------------------------------------------------
    // encoder
    // ------------------------------------------------------------

    /*
    struct rgbe
    {
        u8 r, g, b, e;
    };

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
            const u8* end = memory.end();

            std::string_view id = readline(data, end);
            if (id != "#?RADIANCE")
            {
                header.setError("[ImageDecoder.HDR] Incorrect radiance header.");
                return nullptr;
            }

            for ( ;; )
            {
                std::string_view line = readline(data, end);
                if (line.empty())
                    break;

                std::vector<std::string_view> tokens = tokenize(line);

                if (tokens[0] == "FORMAT")
                {
                    if (tokens.size() != 2)
                    {
                        header.setError("[ImageDecoder.HDR] Incorrect radiance header (format).");
                        return nullptr;
                    }

                    if (tokens[1] == "32-bit_rle_rgbe")
                    {
                        format = rad_rle_rgbe;
                    }
                }
                else if (tokens[1] == "EXPOSURE")
                {
                    if (tokens.size() != 2)
                    {
                        header.setError("[ImageDecoder.HDR] Incorrect radiance header (exposure).");
                        return nullptr;
                    }

                    exposure = float(std::atof(tokens[1].data()));
                }
            }

            if (format == rad_unsupported)
            {
                header.setError("[ImageDecoder.HDR] Incorrect or unsupported format.");
                return nullptr;
            }

            std::string_view dims = readline(data, end);
            std::vector<std::string_view> tokens = tokenize(dims);

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
                    height = std::atoi(tokens[index + 1].data());
                }
                else if (tokens[index] == "-Y")
                {
                    yflip = true;
                    height = std::atoi(tokens[index + 1].data());
                }
                else if (tokens[index] == "+X")
                {
                    xflip = false;
                    width = std::atoi(tokens[index + 1].data());
                }
                else if (tokens[index] == "-X")
                {
                    xflip = true;
                    width = std::atoi(tokens[index + 1].data());
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
            header.format  = Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32, Format::LINEAR);
            header.linear  = true;
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
                status.setError("[ImageDecoder.HDR] Incorrect stream header ({:x} {:x} {:x}).", data[0], data[1], data[2]);
                return;
            }

            if (((data[2] << 8) | data[3]) != surface.width)
            {
                status.setError("[ImageDecoder.HDR] Incorrect scan.");
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
                            status.setError("[ImageDecoder.HDR] Incorrect rle count ({}).", count);
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
                            status.setError("[ImageDecoder.HDR] Incorrect rle count ({}).", count);
                            return;
                        }

                        std::memcpy(dest, data, count);
                        dest += count;
                        data += count;
                    }
                }
            }

            float* image = surface.address<float>(0, y);

            u8* r = buffer + surface.width * 0;
            u8* g = buffer + surface.width * 1;
            u8* b = buffer + surface.width * 2;
            u8* e = buffer + surface.width * 3;

            for (int x = 0; x < surface.width; ++x)
            {
                float scale = getScale(e[x]);
                image[0] = r[x] * scale;
                image[1] = g[x] * scale;
                image[2] = b[x] * scale;
                image[3] = 1.0f;
                image += 4;
            }
        }
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        HeaderRAD m_rad_header;
        const u8* m_data;

        Interface(ConstMemory memory)
        {
            m_data = m_rad_header.parse(memory);
            header = m_rad_header.header;
        }

        ~Interface()
        {
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

            status.direct = dest.format == header.format &&
                            dest.width >= header.width &&
                            dest.height >= header.height;

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

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecHDR()
    {
        registerImageDecoder(createInterface, ".hdr");
    }

} // namespace mango::image
