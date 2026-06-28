/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
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
            // The SGI header is 512 bytes and pixel data begins at offset 512; reject any
            // file too small to contain the header before reading it.
            if (memory.size < 512)
            {
                header.setError("[ImageDecoder.SGI] Incorrect file size.");
                return;
            }

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
                header.setError("[ImageDecoder.SGI] Incorrect channel / colormap.");
                return;
            }

            // a zero dimension yields an empty surface and no decodable body
            if (!xsize || !ysize)
            {
                header.setError("[ImageDecoder.SGI] Invalid image dimensions ({} x {}).", xsize, ysize);
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
        HeaderSGI m_sgi_header;

        Interface(ConstMemory memory)
            : m_memory(memory)
            , m_sgi_header(memory)
        {
            header = m_sgi_header.header;
        }

        ~Interface()
        {
        }

        bool decode_uncompressed(const Surface& s)
        {
            int width = m_sgi_header.xsize;
            int height = m_sgi_header.ysize;
            int channels = m_sgi_header.zsize;

            // All channel planes are stored contiguously after the 512-byte header.
            const size_t needed = size_t(width) * size_t(height) * size_t(channels);
            if (m_memory.size < 512 + needed)
            {
                return false;
            }

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

            return true;
        }

        bool decode_rle(const Surface& s)
        {
            const int width = m_sgi_header.xsize;
            const int height = m_sgi_header.ysize;
            const int channels = m_sgi_header.zsize;

            const u8* data = m_memory.address;

            // The RLE tables hold 'num' 32-bit offsets followed by 'num' 32-bit lengths,
            // both indexed by (channel * height + row), located right after the header.
            const size_t num = size_t(height) * size_t(channels);
            if (m_memory.size < 512 + num * 8)
            {
                return false;
            }

            BigEndianConstPointer p = data + 512;

            std::vector<u32> offsets(num);
            std::vector<u32> sizes(num);

            for (size_t i = 0; i < num; ++i)
            {
                offsets[i] = p.read32();
            }

            for (size_t i = 0; i < num; ++i)
            {
                sizes[i] = p.read32();
            }

            for (int channel = 0; channel < channels; ++channel)
            {
                for (int y = 0; y < height; ++y)
                {
                    int scanline = (height - 1) - y; // mirror vertically
                    u8* dest = s.address<u8>(0, scanline) + channel;

                    const size_t index = size_t(y) + size_t(channel) * size_t(height);
                    const u32 offset = offsets[index];
                    const u32 size = sizes[index];

                    // The offset/length pair is taken from the file; validate that the row's
                    // RLE data lies entirely within the buffer before dereferencing it.
                    if (offset > m_memory.size || size > m_memory.size - offset)
                    {
                        return false;
                    }

                    const u8* src = data + offset;
                    const u8* end = src + size;

                    int x = 0;

                    while (src < end && x < width)
                    {
                        u8 pixel = *src++;
                        int count = pixel & 0x7f;
                        if (!count)
                            break;

                        // never write past the end of the destination scanline
                        if (count > width - x)
                        {
                            count = width - x;
                        }

                        if (pixel & 0x80)
                        {
                            // literal run: consumes 'count' source bytes
                            if (src + count > end)
                            {
                                return false;
                            }

                            while (count--)
                            {
                                *dest = *src++;
                                dest += channels;
                                ++x;
                            }
                        }
                        else
                        {
                            // replicate run: consumes a single source byte
                            if (src >= end)
                            {
                                return false;
                            }

                            pixel = *src++;
                            while (count--)
                            {
                                *dest = pixel;
                                dest += channels;
                                ++x;
                            }
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

            DecodeTargetBitmap target(dest, m_sgi_header.xsize, m_sgi_header.ysize, header.format);

            bool ok = false;

            switch (m_sgi_header.encoding)
            {
                case 0:
                    ok = decode_uncompressed(target);
                    break;
                case 1:
                    ok = decode_rle(target);
                    break;
                default:
                    status.setError("[ImageDecoder.SGI] Unsupported encoding ({}).", m_sgi_header.encoding);
                    return status;
            }

            if (!ok)
            {
                status.setError("[ImageDecoder.SGI] Truncated or corrupt image data.");
                return status;
            }

            target.resolve();
            status.direct = target.isDirect();

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

    void registerImageCodecSGI()
    {
        registerImageDecoder(createInterface, ".rgb");
        registerImageDecoder(createInterface, ".rgba");
        registerImageDecoder(createInterface, ".bw");
        registerImageDecoder(createInterface, ".sgi");
    }

} // namespace mango::image
