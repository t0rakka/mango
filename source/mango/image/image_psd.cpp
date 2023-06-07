/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/core/buffer.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

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

    struct Interface : ImageDecoderInterface
    {
        ImageHeader m_header;
        ConstMemory m_memory;

        const u8* m_palette = nullptr;

        int m_version;
        ColorMode m_color_mode;
        Compression m_compression;
        int m_channels;
        int m_bits;

        Interface(ConstMemory memory)
        {
            BigEndianConstPointer p = memory;

            u32 magic = p.read32();
            if (magic != 0x38425053)
            {
                m_header.setError("[ImageDecoder.PSD] Incorrect identifier.");
                return;
            }

            m_version = p.read16();
            if (m_version > 2)
            {
                m_header.setError("[ImageDecoder.PSD] Incorrect version (%d).", m_version);
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
                m_header.setError("[ImageDecoder.PSD] Incorrect number of channels (%d).", m_channels);
                return;
            }

            int height = p.read32();
            int width = p.read32();
            if (width < 1 || width > width_max || height < 1 || height > height_max)
            {
                m_header.setError("[ImageDecoder.PSD] Incorrect image dimensions (%d x %d).", width, height);
                return;
            }

            Format format;

            m_bits = p.read16();
            switch (m_bits)
            {
                case 8:
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    break;
                case 16:
                    format = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
                    break;
                case 32:
                    format = Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32);
                    break;
                case 1:
                    // TODO
                    m_header.setError("[ImageDecoder.PSD] Unsupported number of bits (%d).", m_bits);
                    return;
                default:
                    m_header.setError("[ImageDecoder.PSD] Incorrect number of bits (%d).", m_bits);
                    return;
            }

            m_color_mode = ColorMode(p.read16());
            switch (m_color_mode)
            {
                case ColorMode::INDEXED:
                case ColorMode::GRAYSCALE:
                case ColorMode::RGB:
                case ColorMode::CMYK:
                    break;
                case ColorMode::BITMAP:
                case ColorMode::MULTICHANNEL:
                case ColorMode::DUOTONE:
                case ColorMode::LAB:
                    m_header.setError("[ImageDecoder.PSD] Unsupported color mode (%d).", m_color_mode);
                    return;
                default:
                    m_header.setError("[ImageDecoder.PSD] Incorrect color mode (%d).", m_color_mode);
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
                    m_header.setError("[ImageDecoder.PSD] Incorrect palette size (%d).", colormode_size);
                    return;
                }
            }

            p += colormode_size;

            // ImageResource data
            int resource_size = p.read32();
            p += resource_size;

            // Layer and Mask data
            int layer_size = p.read32();
            p += layer_size;

            m_compression = Compression(p.read16());
            switch (m_compression)
            {
                case Compression::RAW:
                case Compression::RLE:
                    break;
                case Compression::ZIP:
                case Compression::ZIP_PRED:
                    // TODO
                    m_header.setError("[ImageDecoder.PSD] Unsupported compression (%d).", m_compression);
                    return;
                default:
                    m_header.setError("[ImageDecoder.PSD] Incorrect compression (%d).", m_compression);
                    return;
            }

            const u8* end = memory.address + memory.size;
            if (p >= end)
            {
                m_header.setError("[ImageDecoder.PSD] Out of data.");
                return;
            }

            debugPrint("[psd]\n");
            debugPrint("  Version:     %d\n", m_version);
            debugPrint("  Image:       %d x %d\n", width, height);
            debugPrint("  Channels:    %d (%d bits)\n", m_channels, m_bits);
            debugPrint("  ColorMode:   %d\n", m_color_mode);
            debugPrint("  Compression: %d\n", m_compression);

            m_memory = ConstMemory(p, end - p);

            m_header.width   = width;
            m_header.height  = height;
            m_header.depth   = 0;
            m_header.levels  = 0;
            m_header.faces   = 0;
            m_header.palette = false;
            m_header.format  = format;
            m_header.compression = TextureCompression::NONE;
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            return ConstMemory();
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            ImageDecodeStatus status;

            BigEndianConstPointer p = m_memory.address;

            int width = m_header.width;
            int height = m_header.height;
            int pixels = width * height;
            int channels = std::min(4, m_channels);

            int bytes_per_pixel = div_ceil(m_bits, 8);
            int bytes_per_channel = pixels * bytes_per_pixel;

            Buffer buffer(channels * bytes_per_channel);

            switch (m_compression)
            {
                case Compression::RAW:
                {
                    switch (m_bits)
                    {
                        case 1:
                            // TODO
                            break;

                        case 8:
                            std::memcpy(buffer, p, channels * bytes_per_channel);
                            break;

                        case 16:
                        {
                            u16* dest = reinterpret_cast<u16*>(buffer.data());
                            int count = pixels * channels;
                            for (int i = 0; i < count; ++i)
                            {
                                dest[i] = p.read16();
                            }
                            break;
                        }

                        case 32:
                        {
                            u32* dest = reinterpret_cast<u32*>(buffer.data());
                            int count = pixels * channels;
                            for (int i = 0; i < count; ++i)
                            {
                                dest[i] = p.read32();
                            }
                            break;
                        }
                    }

                    break;
                }

                case Compression::RLE:
                {
                    // number of bytes per scanline are stored in array before RLE data
                    // version 1 files use u16 to store the length, version 2 files use u32
                    int scan_header_size = m_version == 1 ? sizeof(u16) : sizeof(u32);
                    p += height * m_channels * scan_header_size;

                    for (int channel = 0; channel < channels; ++channel)
                    {
                        u8* dest = buffer + channel * bytes_per_channel;
                        p = decode_runlength(dest, p, bytes_per_channel);

                        if (m_bits == 16)
                        {
                            u16* ptr = reinterpret_cast<u16*>(dest);
                            for (int i = 0; i < pixels; ++i)
                            {
                                ptr[i] = byteswap(ptr[i]);
                            }
                        }
                        else if (m_bits == 32)
                        {
                            u32* ptr = reinterpret_cast<u32*>(dest);
                            for (int i = 0; i < pixels; ++i)
                            {
                                ptr[i] = byteswap(ptr[i]);
                            }
                        }
                    }

                    break;
                }

                case Compression::ZIP:
                {
                    // TODO
                    break;
                }

                case Compression::ZIP_PRED:
                {
                    // TODO
                    break;
                }
            }

            Bitmap temp(width, height, m_header.format);
            resolve(temp.image, buffer, pixels, channels, pixels);

            dest.blit(0, 0, temp);

            return status;
        }

        void resolve(u8* dest, const u8* src, int stride, int channels, int count)
        {
            if (m_color_mode == ColorMode::INDEXED)
            {
                if (m_bits == 8 && m_palette)
                {
                    for (int i = 0; i < count; ++i)
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
            else if (m_color_mode == ColorMode::GRAYSCALE)
            {
                switch (m_bits)
                {
                    case 1:
                        // TODO
                        break;
                    case 8:
                        pack_grayscale<u8>(dest, src, count, 0xff);
                        break;
                    case 16:
                        pack_grayscale<u16>(dest, src, count, 0xffff);
                        break;
                    case 32:
                        pack_grayscale<float>(dest, src, count, 1.0f);
                        break;
                }
            }
            else if (m_color_mode == ColorMode::RGB)
            {
                switch (m_bits)
                {
                    case 1:
                        // TODO
                        break;
                    case 8:
                        if (channels == 4)
                            pack_rgba<u8>(dest, src, count);
                        else
                            pack_rgb<u8>(dest, src, count, 0xff);
                        break;
                    case 16:
                        if (channels == 4)
                            pack_rgba<u16>(dest, src, count);
                        else
                            pack_rgb<u16>(dest, src, count, 0xffff);
                        break;
                    case 32:
                        if (channels == 4)
                            pack_rgba<float>(dest, src, count);
                        else
                            pack_rgb<float>(dest, src, count, 1.0f);
                        break;
                }
            }
            else if (m_color_mode == ColorMode::CMYK)
            {
                // TODO: 1, 16, 32 bits
                // TODO: different nr of channels
                if (m_bits == 8)
                {
                    for (int i = 0; i < count; ++i)
                    {
                        int c = src[stride * 0];
                        int m = src[stride * 1];
                        int y = src[stride * 2];
                        int k = src[stride * 3];
                        dest[0] = (c * k) / 255;
                        dest[1] = (m * k) / 255;
                        dest[2] = (y * k) / 255;
                        dest[3] = 0xff;
                        ++src;
                        dest += 4;
                    }
                }
            }
        }

        template <typename T>
        void pack_grayscale(u8* image, const u8* buffer, int pixels, T alpha) const
        {
            const T* src = reinterpret_cast<const T*>(buffer);
            T* dest = reinterpret_cast<T*>(image);

            for (int i = 0; i < pixels; ++i)
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
        void pack_rgb(u8* image, const u8* buffer, int pixels, T alpha) const
        {
            const T* src = reinterpret_cast<const T*>(buffer);
            T* dest = reinterpret_cast<T*>(image);

            for (int i = 0; i < pixels; ++i)
            {
                dest[0] = src[pixels * 0];
                dest[1] = src[pixels * 1];
                dest[2] = src[pixels * 2];
                dest[3] = alpha;
                ++src;
                dest += 4;
            }
        }

        template <typename T>
        void pack_rgba(u8* image, const u8* buffer, int pixels) const
        {
            const T* src = reinterpret_cast<const T*>(buffer);
            T* dest = reinterpret_cast<T*>(image);

            for (int i = 0; i < pixels; ++i)
            {
                dest[0] = src[pixels * 0];
                dest[1] = src[pixels * 1];
                dest[2] = src[pixels * 2];
                dest[3] = src[pixels * 3];
                ++src;
                dest += 4;
            }
        }

        // decoding

        const u8* decode_runlength(u8* output, const u8* input, int count) const
        {
            while (count > 0)
            {
                int code = s8(*input++);

                if (code == 128)
                {
                    continue;
                }

                if (code >= 0)
                {
                    int length = 1 + code;
                    count -= length;

                    std::memcpy(output, input, length);
                    output += length;
                    input += length;
                }
                else
                {
                    int length = 1 - code;
                    count -= length;

                    u8 value = *input++;
                    std::memset(output, value, length);
                    output += length;
                }
            }

            return input;
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

    void registerImageCodecPSD()
    {
        registerImageDecoder(createInterface, ".psd");
        registerImageDecoder(createInterface, ".psb");
    }

} // namespace mango::image
