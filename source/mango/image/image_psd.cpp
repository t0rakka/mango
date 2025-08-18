/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/compress.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;
    using namespace mango::math;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    // Specification:
    // https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/

    enum class ColorMode
    {
        BITMAP  = 0,
        GRAYSCALE = 1,
        INDEXED = 2,
        RGB = 3,
        CMYK = 4,
        MULTICHANNEL = 7,
        DUOTONE = 8,
        LAB = 9
    };

    enum class Compression
    {
        RAW = 0,
        RLE = 1,
        ZIP = 2,
        ZIP_PRED = 3
    };

    struct PackBits
    {
        std::vector<u32> offsets;
        std::vector<u32> sizes;

        PackBits(int channels, int height)
            : offsets(channels)
            , sizes(channels * height)
        {
        }

        const u8* parse(const u8* p, int channels, int height, int version)
        {
            int offset = 0;
            for (int channel = 0; channel < channels; ++channel)
            {
                offsets[channel] = offset;

                if (version == 1)
                {
                    for (int y = 0; y < height; ++y)
                    {
                        u32 bytes = bigEndian::uload16(p);
                        p += 2;
                        sizes[channel * height + y] = bytes;
                        offset += bytes;
                    }
                }
                else
                {
                    for (int y = 0; y < height; ++y)
                    {
                        u32 bytes = bigEndian::uload32(p);
                        p += 4;
                        sizes[channel * height + y] = bytes;
                        offset += bytes;
                    }
                }
            }

            return p;
        }
    };

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;

        const u8* m_palette = nullptr;

        int m_version;
        ColorMode m_color_mode;
        Compression m_compression;
        int m_channels;
        int m_bits;

        ConstMemory m_icc_profile;

        Interface(ConstMemory memory)
        {
            BigEndianConstPointer p = memory;

            u32 magic = p.read32();
            if (magic != 0x38425053)
            {
                header.setError("[ImageDecoder.PSD] Incorrect identifier.");
                return;
            }

            m_version = p.read16();
            if (m_version > 2)
            {
                header.setError("[ImageDecoder.PSD] Incorrect version ({}).", m_version);
                return;
            }

            int width_max = 30000;
            int height_max = 30000;

            if (m_version == 2)
            {
                width_max = 300000;
                height_max = 300000;
            }

            p += 6;

            m_channels = p.read16();
            if (m_channels < 1 || m_channels > 56)
            {
                header.setError("[ImageDecoder.PSD] Incorrect number of channels ({}).", m_channels);
                return;
            }

            int height = p.read32();
            int width = p.read32();
            if (width < 1 || width > width_max || height < 1 || height > height_max)
            {
                header.setError("[ImageDecoder.PSD] Incorrect image dimensions ({} x {}).", width, height);
                return;
            }

            Format format;

            m_bits = p.read16();
            switch (m_bits)
            {
                case 1:
                case 8:
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    break;
                case 16:
                    format = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
                    break;
                case 32:
                    format = Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32);
                    break;
                default:
                    header.setError("[ImageDecoder.PSD] Incorrect number of bits ({}).", m_bits);
                    return;
            }

            m_color_mode = ColorMode(p.read16());
            switch (m_color_mode)
            {
                case ColorMode::BITMAP:
                case ColorMode::GRAYSCALE:
                case ColorMode::INDEXED:
                case ColorMode::RGB:
                case ColorMode::CMYK:
                case ColorMode::MULTICHANNEL:
                case ColorMode::DUOTONE:
                case ColorMode::LAB:
                    break;
                default:
                    header.setError("[ImageDecoder.PSD] Incorrect color mode ({}).", int(m_color_mode));
                    return;
            }

            // ColorMode data
            int colormode_size = p.read32();

            if (m_color_mode == ColorMode::INDEXED)
            {
                if (colormode_size == 768)
                {
                    m_palette = p;
                }
                else
                {
                    header.setError("[ImageDecoder.PSD] Incorrect palette size ({}).", colormode_size);
                    return;
                }
            }

            p += colormode_size;

            // ImageResource data
            ConstMemory image_resource_data(p + 4, p.read32());
            p += image_resource_data.size;

            // Layer and Mask data
            ConstMemory layer_data(p + 4, p.read32());
            p += layer_data.size;

            m_compression = Compression(p.read16());
            switch (m_compression)
            {
                case Compression::RAW:
                case Compression::RLE:
                    break;
                case Compression::ZIP:
                case Compression::ZIP_PRED:
                    // MANGO TODO: need psd files with these compressions
                    header.setError("[ImageDecoder.PSD] Unsupported compression ({}).", int(m_compression));
                    return;
                default:
                    header.setError("[ImageDecoder.PSD] Incorrect compression ({}).", int(m_compression));
                    return;
            }

            const u8* end = memory.end();
            if (p >= end)
            {
                header.setError("[ImageDecoder.PSD] Out of data.");
                return;
            }

            m_memory = ConstMemory(p, end - p);

            printLine(Print::Info, "[psd]\n");
            printLine(Print::Info, "  Version:     {}", m_version);
            printLine(Print::Info, "  Image:       {} x {}", width, height);
            printLine(Print::Info, "  Channels:    {} ({} bits)", m_channels, m_bits);
            printLine(Print::Info, "  ColorMode:   {}", int(m_color_mode));
            printLine(Print::Info, "  Compression: {}", int(m_compression));

            parse_resources(image_resource_data);

            header.width   = width;
            header.height  = height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
            header.format  = format;
            header.compression = TextureCompression::NONE;

            icc = m_icc_profile;
        }

        ~Interface()
        {
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(face);
            MANGO_UNREFERENCED(depth);
            return ConstMemory();
        }

        std::string parse_string(BigEndianConstPointer& p, int alignment)
        {
            std::string str;

            u8 length = *p++;
            if (length)
            {
                const char* s = p.cast<const char>();
                str = std::string(s, s + length);
            }

            p += length;
            p += (0 - (length + 1)) & (alignment - 1);

            return str;
        }

        void parse_resources(ConstMemory resources)
        {
            BigEndianConstPointer p = resources.address;
            const u8* end = resources.end();

            printLine(Print::Info, "  [ImageResourceBlocks]");

            while (p < end)
            {
                u32 signature = p.read32();
                if (signature != 0x3842494d)
                {
                    return;
                }

                u16 id = p.read16();
                std::string c_name = parse_string(p, 2);
                u32 length = p.read32();

                bool supported = true;

                if (id == 1005)
                {
                    printLine(Print::Info, "    Resolution Info");
                }
                else if (id == 1039)
                {
                    printLine(Print::Info, "    ICC Profile");
                    m_icc_profile = ConstMemory(p, length);
                }
                else if (id == 1047)
                {
                    printLine(Print::Info, "    TransparencyIndex");
                    // MANGO TODO: need psd file that uses this feature
                }
                else if (id == 1058 || id == 1059)
                {
                    printLine(Print::Info, "    EXIF");
                }
                else if (id == 1060)
                {
                    printLine(Print::Info, "    XMP");
                }
                else
                {
                    supported = false;
                }

                p += length;

                if (supported)
                {
                    printLine(Print::Info, "      {} bytes", length);
                }
            }
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(face);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(options);

            ImageDecodeStatus status;

            const u8* p = m_memory.address;

            int width = header.width;
            int height = header.height;
            int channels = std::min(4, m_channels);

            int bytes_per_scan = div_ceil(width * m_bits, 8);
            int bytes_per_channel = height * bytes_per_scan;

            //printLine(Print::Info, "  available: {} bytes", m_memory.size);
            //printLine(Print::Info, "  request:   {} bytes", channels * bytes_per_channel);

            Bitmap temp(width, height, header.format);
            Buffer buffer(channels * bytes_per_scan);

            switch (m_compression)
            {
                case Compression::RAW:
                {
                    for (int y = 0; y < height; ++y)
                    {
                        for (int channel = 0; channel < channels; ++channel)
                        {
                            const u8* src = p + channel * bytes_per_channel + y * bytes_per_scan;
                            u8* dst = buffer + channel * bytes_per_scan;
                            std::memcpy(dst, src, bytes_per_scan);
                        }

#ifdef MANGO_LITTLE_ENDIAN
                        byteswap(buffer, m_bits);
#endif
                        resolve(temp.image + y * temp.stride, buffer, width, channels);
                    }

                    break;
                }

                case Compression::RLE:
                {
                    PackBits packbits(m_channels, height);
                    p = packbits.parse(p, m_channels, height, m_version);

                    for (int y = 0; y < height; ++y)
                    {
                        for (int channel = 0; channel < channels; ++channel)
                        {
                            u32 bytes = packbits.sizes[channel * height + y];
                            const u8* src = p + packbits.offsets[channel];
                            packbits.offsets[channel] += bytes;

                            u8* dst = buffer + channel * bytes_per_scan;

                            Memory output(dst, bytes_per_scan);
                            ConstMemory input(src, bytes);

                            bool result = packbits_decompress(output, input);
                            if (!result)
                            {
                                status.setError("[ImageDecoder.PSD] packbits decompression failed.");
                                return status;
                            }
                        }

#ifdef MANGO_LITTLE_ENDIAN
                        byteswap(buffer, m_bits);
#endif
                        resolve(temp.image + y * temp.stride, buffer, width, channels);
                    }

                    break;
                }

                case Compression::ZIP:
                {
                    // MANGO TODO: need example files
                    break;
                }

                case Compression::ZIP_PRED:
                {
                    // MANGO TODO: need example files
                    break;
                }
            }

            dest.blit(0, 0, temp);

            return status;
        }

        void resolve(u8* dest, const u8* src, int width, int channels)
        {
            switch (m_color_mode)
            {
                case ColorMode::BITMAP:
                    resolveBitmap(dest, src, width, channels);
                    break;
                case ColorMode::GRAYSCALE:
                    resolveGrayscale(dest, src, width, channels);
                    break;
                case ColorMode::INDEXED:
                    resolveIndexed(dest, src, width, channels);
                    break;
                case ColorMode::RGB:
                    resolveRGB(dest, src, width, channels);
                    break;
                case ColorMode::CMYK:
                    resolveCMYK(dest, src, width, channels);
                    break;
                case ColorMode::MULTICHANNEL:
                    resolveGrayscale(dest, src, width, channels);
                    break;
                case ColorMode::DUOTONE:
                    resolveGrayscale(dest, src, width, channels);
                    break;
                case ColorMode::LAB:
                    resolveLab(dest, src, width, channels);
                    break;
            }
        }

        void resolveBitmap(u8* dest, const u8* src, int width, int channels)
        {
            MANGO_UNREFERENCED(channels);

            u8 mask = 0;
            u8 data = 0;

            for (int x = 0; x < width; ++x)
            {
                if (!mask)
                {
                    mask = 0x80;
                    data = *src++;
                }

                u8 value = 0xff + !!(data & mask);
                mask >>= 1;

                dest[0] = value;
                dest[1] = value;
                dest[2] = value;
                dest[3] = 0xff;
                dest += 4;
            }
        }

        void resolveGrayscale(u8* dest, const u8* src, int width, int channels)
        {
            MANGO_UNREFERENCED(channels);

            switch (m_bits)
            {
                case 8:
                    pack_grayscale<u8>(dest, src, width, 0xff);
                    break;
                case 16:
                    pack_grayscale<u16>(dest, src, width, 0xffff);
                    break;
                case 32:
                    pack_grayscale<float>(dest, src, width, 1.0f);
                    break;
            }
        }

        void resolveIndexed(u8* dest, const u8* src, int width, int channels)
        {
            MANGO_UNREFERENCED(channels);

            if (m_bits == 8 && m_palette)
            {
                for (int i = 0; i < width; ++i)
                {
                    u8 index = *src++;
                    dest[0] = m_palette[index + 256 * 0];
                    dest[1] = m_palette[index + 256 * 1];
                    dest[2] = m_palette[index + 256 * 2];
                    dest[3] = 0xff;
                    dest += 4;
                }
            }
        }

        void resolveRGB(u8* dest, const u8* src, int width, int channels)
        {
            switch (m_bits)
            {
                case 8:
                    if (channels == 4)
                        pack_rgba<u8>(dest, src, width);
                    else
                        pack_rgb<u8>(dest, src, width, 0xff);
                    break;
                case 16:
                    if (channels == 4)
                        pack_rgba<u16>(dest, src, width);
                    else
                        pack_rgb<u16>(dest, src, width, 0xffff);
                    break;
                case 32:
                    if (channels == 4)
                        pack_rgba<float>(dest, src, width);
                    else
                        pack_rgb<float>(dest, src, width, 1.0f);
                    break;
            }
        }

        void resolveCMYK(u8* dest, const u8* src, int width, int channels)
        {
            MANGO_UNREFERENCED(channels);

            // MANGO TODO: 16, 32 bits
            // MANGO TODO: different nr of channels
            if (m_bits == 8)
            {
                for (int i = 0; i < width; ++i)
                {
                    int c = src[width * 0];
                    int m = src[width * 1];
                    int y = src[width * 2];
                    int k = src[width * 3];
                    dest[0] = (c * k) / 255;
                    dest[1] = (m * k) / 255;
                    dest[2] = (y * k) / 255;
                    dest[3] = 0xff;
                    ++src;
                    dest += 4;
                }
            }
        }

        void resolveLab(u8* dest, const u8* src, int width, int channels)
        {
            MANGO_UNREFERENCED(channels);

            if (m_bits == 8)
            {
                for (int i = 0; i < width; ++i)
                {
                    float L = src[width * 0 + i];
                    float A = src[width * 1 + i];
                    float B = src[width * 2 + i];

                    L = (L * 100.0f) / 255.0f;
                    A = A - 128;
                    B = B - 128;

                    uint32x4 color = convert<uint32x4>(lab_to_rgb(L, A, B) * 255.0f);
                    ustore32(dest + i * 4, color.pack());
                }
            }
            else if (m_bits == 16)
            {
                const u16* s = reinterpret_cast<const u16*>(src);
                u16* d = reinterpret_cast<u16*>(dest);

                for (int i = 0; i < width; ++i)
                {
                    float L = s[width * 0 + i];
                    float A = s[width * 1 + i];
                    float B = s[width * 2 + i];

                    L = (L * 100.0f) / 65535.0f;
                    A = (A - 32768) / 257.0f;
                    B = (B - 32768) / 257.0f;

                    uint32x4 color = convert<uint32x4>(lab_to_rgb(L, A, B) * 65535.0f);
                    store_low(d + i * 4, simd::narrow(color, color));
                }
            }
        }

        float32x4 lab_to_rgb(float L, float A, float B)
        {
            float y = (L + 16.0f) / 116.0f;
            float x = A / 500.0f + y;
            float z = y - B / 200.0f;

            x = 0.95047f * ((x * x * x > 0.008856f) ? x * x * x : (x - 16/116.0f) / 7.787f);
            y = 1.00000f * ((y * y * y > 0.008856f) ? y * y * y : (y - 16/116.0f) / 7.787f);
            z = 1.08883f * ((z * z * z > 0.008856f) ? z * z * z : (z - 16/116.0f) / 7.787f);

            float r = x *  3.2406f + y * -1.5372f + z * -0.4986f;
            float g = x * -0.9689f + y *  1.8758f + z *  0.0415f;
            float b = x *  0.0557f + y * -0.2040f + z *  1.0570f;
            float a = 1.0f;

            float32x4 color(r, g, b, a);
            color = linear_to_srgb(color);

            return color;
        }

        template <typename T>
        void pack_grayscale(u8* image, const u8* buffer, int width, T alpha) const
        {
            const T* src = reinterpret_cast<const T*>(buffer);
            T* dest = reinterpret_cast<T*>(image);

            for (int i = 0; i < width; ++i)
            {
                T s = *src++;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = alpha;
                dest += 4;
            }
        }

        template <typename T>
        void pack_rgb(u8* image, const u8* buffer, int width, T alpha) const
        {
            const T* src = reinterpret_cast<const T*>(buffer);
            T* dest = reinterpret_cast<T*>(image);

            for (int i = 0; i < width; ++i)
            {
                dest[0] = src[width * 0];
                dest[1] = src[width * 1];
                dest[2] = src[width * 2];
                dest[3] = alpha;
                ++src;
                dest += 4;
            }
        }

        template <typename T>
        void pack_rgba(u8* image, const u8* buffer, int width) const
        {
            const T* src = reinterpret_cast<const T*>(buffer);
            T* dest = reinterpret_cast<T*>(image);

            for (int i = 0; i < width; ++i)
            {
                dest[0] = src[width * 0];
                dest[1] = src[width * 1];
                dest[2] = src[width * 2];
                dest[3] = src[width * 3];
                ++src;
                dest += 4;
            }
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

    void registerImageCodecPSD()
    {
        registerImageDecoder(createInterface, ".psd");
        registerImageDecoder(createInterface, ".psb");
    }

} // namespace mango::image
