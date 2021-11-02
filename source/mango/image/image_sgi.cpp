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
    // .sgi parser
    // ------------------------------------------------------------

    struct HeaderSGI
    {
        u8 encoding;
        u8 bpc;
        u16 dimension;
        u16 xsize;
        u16 ysize;
        u16 zsize;
        u32 colormap;

        ImageHeader header;

        HeaderSGI(ConstMemory memory)
        {
            BigEndianConstPointer p = memory.address;

            u16 magic = p.read16();
            if (magic != 474)
            {
                header.setError("[ImageDecoder.SGI] Incorrect header magic.");
                return;
            }

            encoding  = p.read8();  // 0: UNCOMPRESSED, 1: RLE
            bpc       = p.read8();  // bytes per pixel channel (1, 2)
            dimension = p.read16(); // number of dimensions (1, 2, 3)
            xsize     = p.read16(); // width
            ysize     = p.read16(); // height
            zsize     = p.read16(); // number of channels
            p += 2; // minimum pixel value
            p += 2; // maximum pixel value
            p += 84;
            colormap  = p.read32(); // 0: NORMAL, 1: DITHERED, 2: SCREEN, 3: COLORMAP

            if (bpc != 1 || colormap != 0)
            {
                header.setError("[ImageDecoder.SGI] Incorrect channel/colormap.");
                return;
            }

            // select color format
            Format format;
            switch (zsize)
            {
                case 1:
                    format = LuminanceFormat(8, Format::UNORM, 8, 0);
                    break;
                case 2:
                    format = LuminanceFormat(16, Format::UNORM, 8, 8);
                    break;
                case 3:
                    format = Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0);
                    break;
                case 4:
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    break;
                default:
                    header.setError("[ImageDecoder.SGI] Incorrect color format.");
                    return;
            }

            header.width   = xsize;
            header.height  = ysize;
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
        HeaderSGI m_sgi_header;

        Interface(ConstMemory memory)
            : m_memory(memory)
            , m_sgi_header(memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_sgi_header.header;
        }

        void decode_uncompressed(const Surface& s)
        {
            int width = m_sgi_header.xsize;
            int height = m_sgi_header.ysize;
            int channels = m_sgi_header.zsize;

            const u8* data = m_memory.address + 512;
            
            for (int channel = 0; channel < channels; ++channel)
            {
                for (int y = 0; y < height; ++y)
                {
                    int scanline = (height - 1) - y; // mirror vertically
                    u8* dest = s.address<u8>(0, scanline) + channel;

                    int offset = (y + channel * height) * width;
                    const u8* src = data + offset;

                    for (int x = 0; x < width; ++x)
                    {
                        *dest = src[x];
                        dest += channels;
                    }
                }
            }
        }

        void decode_rle(const Surface& s)
        {
            int height = m_sgi_header.ysize;
            int channels = m_sgi_header.zsize;

            const u8* data = m_memory.address;

            BigEndianConstPointer p = data + 512;

            // read RLE offset table
            int num = height * channels;

            std::vector<u32> offsets(num);
            std::vector<s32> sizes(num);

            for (int i = 0; i < num; ++i)
            {
                offsets[i] = p.read32();
            }

            for (int i = 0; i < num; ++i)
            {
                sizes[i] = p.read32();
            }

            for (int channel = 0; channel < channels; ++channel)
            {
                for (int y = 0; y < height; ++y)
                {
                    int scanline = (height - 1) - y; // mirror vertically
                    u8* dest = s.address<u8>(0, scanline) + channel;

                    int index = y + channel * height;
                    int offset = offsets[index];
                    const u8* src = data + offset;
                    const u8* end = src + sizes[index];

                    for (; src < end;)
                    {
                        u8 pixel = *src++;
                        int count = pixel & 0x7f;
                        if (!count)
                            break;

                        if (pixel & 0x80)
                        {
                            while (count--)
                            {
                                *dest = *src++;
                                dest += channels;
                            }
                        }
                        else
                        {
                            pixel = *src++;
                            while (count--)
                            {
                                *dest = pixel;
                                dest += channels;
                            }
                        }
                    }
                }
            }
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

			const ImageHeader& header = m_sgi_header.header;
            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            status.direct = dest.format == header.format &&
                            dest.width >= m_sgi_header.xsize &&
                            dest.height >= m_sgi_header.ysize;

            if (status.direct)
            {
                switch (m_sgi_header.encoding)
                {
                    case 0:
                        decode_uncompressed(dest);
                        break;
                    case 1:
                        decode_rle(dest);
                        break;
                }
            }
            else
            {
                Bitmap temp(m_sgi_header.xsize, m_sgi_header.ysize, header.format);
                switch (m_sgi_header.encoding)
                {
                    case 0:
                        decode_uncompressed(temp);
                        break;
                    case 1:
                        decode_rle(temp);
                        break;
                }
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

    void registerImageDecoderSGI()
    {
        registerImageDecoder(createInterface, ".rgb");
        registerImageDecoder(createInterface, ".rgba");
        registerImageDecoder(createInterface, ".bw");
        registerImageDecoder(createInterface, ".sgi");
    }

} // namespace mango::image
