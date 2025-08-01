/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/string.hpp>
#include <mango/image/image.hpp>

// Specification:
// https://www.fileformat.info/format/tga/egff.htm

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // header
    // ------------------------------------------------------------

    enum : u8
    {
        IMAGETYPE_NONE            = 0,
        IMAGETYPE_PALETTE         = 1,
        IMAGETYPE_TRUECOLOR       = 2,
        IMAGETYPE_MONOCHROME      = 3,
        IMAGETYPE_RLE_PALETTE     = 9,
        IMAGETYPE_RLE_TRUECOLOR   = 10,
        IMAGETYPE_RLE_MONOCHROME  = 11,
    };

    enum : u8
    {
        DESCRIPTOR_OVERLAY   = 0x04,
        DESCRIPTOR_ALPHA     = 0x08,
        DESCRIPTOR_ORIGIN_X  = 0x10,
        DESCRIPTOR_ORIGIN_Y  = 0x20,
    };

    struct HeaderTGA
    {
        u8   id_length;
        u8   colormap_type; // 0 - no palette, 1 - palette
        u8   image_type;
        u16  colormap_origin;
        u16  colormap_length;
        u8   colormap_bits;
        u16  xoffset;
        u16  yoffset;
        u16  width;
        u16  height;
        u8   pixel_size;
        u8   descriptor;

        std::string error;

        const u8* parse(LittleEndianConstPointer p)
        {
            id_length        = p.read8();
            colormap_type    = p.read8();
            image_type       = p.read8();
            colormap_origin  = p.read16();
            colormap_length  = p.read16();
            colormap_bits    = p.read8();
            xoffset          = p.read16();
            yoffset          = p.read16();
            width            = p.read16();
            height           = p.read16();
            pixel_size       = p.read8();
            descriptor       = p.read8();

            printLine(Print::Info, "  dimensions:    {} x {}", width, height);
            printLine(Print::Info, "  image_type:    {}", image_type);
            printLine(Print::Info, "  pixel_size:    {}", pixel_size);
            printLine(Print::Info, "  colormap_type: {}", colormap_type);
            printLine(Print::Info, "  colormap_bits: {}", colormap_bits);
            printLine(Print::Info, "  colormap:      [{}, {}]", colormap_origin, colormap_origin + colormap_length);
            printLine(Print::Info, "  descriptor:    {:#x}", descriptor);
            printLine(Print::Info, "  id_length:     {}", id_length);

            switch (image_type)
            {
                case IMAGETYPE_NONE:
                case IMAGETYPE_PALETTE:
                case IMAGETYPE_TRUECOLOR:
                case IMAGETYPE_MONOCHROME:
                case IMAGETYPE_RLE_PALETTE:
                case IMAGETYPE_RLE_TRUECOLOR:
                case IMAGETYPE_RLE_MONOCHROME:
                    break;
                default:
                    error = fmt::format("[ImageDecoder.TGA] Invalid data type ({}).", image_type);
                    return nullptr;
            }

            switch (pixel_size)
            {
                case 8:
                case 16:
                case 24:
                case 32:
                    break;
                default:
                    error = fmt::format("[ImageDecoder.TGA] Invalid pixel size ({}).", pixel_size);
                    return nullptr;
            }

            if (!colormap_type)
            {
                // no colormap
            }
            else
            {
                if (isPalette())
                {
                    // palette
                    if (colormap_origin + colormap_length > 256)
                    {
                        error = fmt::format("[ImageDecoder.TGA] Invalid colormap (origin: {}, length: {}).", colormap_origin, colormap_length);
                        return nullptr;
                    }

                    if (colormap_bits != 15 &&
                        colormap_bits != 16 &&
                        colormap_bits != 24 &&
                        colormap_bits != 32)
                    {
                        error = fmt::format("[ImageDecoder.TGA] Invalid colormap bits ({}).", colormap_bits);
                        return nullptr;
                    }
                }
            }

            /*
            if (width > 512 || height > 482)
            {
                error = fmt::format("[ImageDecoder.TGA] Incorrect image dimensions: {} x {} (maximum: 512 x 482).", width, height);
                return nullptr;
            }
            */

            // skip idfield
            p += id_length;

            return p;
        }

        void write(Stream& file)
        {
            LittleEndianStream s(file);

            s.write8(id_length);
            s.write8(colormap_type);
            s.write8(image_type);
            s.write16(colormap_origin);
            s.write16(colormap_length);
            s.write8(colormap_bits);
            s.write16(xoffset);
            s.write16(yoffset);
            s.write16(width);
            s.write16(height);
            s.write8(pixel_size);
            s.write8(descriptor);
        }

        bool isPalette() const
        {
            return (image_type & 3) == 1;
        }

        bool isRLE() const
        {
            return (image_type & 8) != 0;
        }

        int getBytesPerPixel() const
        {
            return pixel_size >> 3;
        }

        Format getFormat() const
        {
            Format format;

            switch (pixel_size)
            {
                case 8:
                    if (isPalette())
                    {
                        format = IndexedFormat(8);
                    }
                    else
                    {
                        format = LuminanceFormat(8, Format::UNORM, 8,  0);
                    }
                    break;

                case 16:
                    format = (descriptor & DESCRIPTOR_ALPHA) ? LuminanceFormat(16, Format::UNORM, 8, 8)
                                                             : Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 0);
                    break;

                case 24:
                    format = Format(24, Format::UNORM, Format::BGR, 8, 8, 8);
                    break;

                case 32:
                    format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
                    break;
            }

            return format;
        }
    };

    bool tga_decompress_RLE(u8* dest, const u8* p, const u8* end, int width, int height, int bpp)
    {
        int x = 0;
        int y = 0;
        u8* scan = dest;

        for ( ; y < height; )
        {
            if (p >= end)
            {
                return false;
            }

            u8 sample = *p++;
            int count = (sample & 0x7f) + 1;
            const u8* color = p;

            if (sample & 0x80)
            {
                p += bpp;
            }

            for ( ; count > 0; )
            {
                // clip to right edge
                const int size = std::min(count, width - x);
                if (!size)
                {
                    return false;
                }

                if (sample & 0x80)
                {
                    if (color + bpp > end)
                    {
                        return false;
                    }

                    // repeat color
                    for (int i = 0; i < size; ++i)
                    {
                        for (int j = 0; j < bpp; ++j)
                        {
                            scan[j] = color[j]; // <-- read
                        }
                        scan += bpp;
                    }
                }
                else
                {
                    int bytes = size * bpp;
                    if (color + bytes > end)
                    {
                        return false;
                    }

                    std::memcpy(scan, color, bytes);
                    p += bytes;
                    scan += bytes;
                    color += bytes;
                }

                count -= size;
                x += size;

                if (x >= width)
                {
                    ++y;
                    x = 0;
                    dest += width * bpp;
                    scan = dest;
                }
            }
        }

        return true;
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        HeaderTGA m_targa_header;
        const u8* m_pointer;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            m_pointer = m_targa_header.parse(memory.address);
            if (m_pointer)
            {
                header.width   = m_targa_header.width;
                header.height  = m_targa_header.height;
                header.depth   = 0;
                header.levels  = 0;
                header.faces   = 0;
                header.format  = m_targa_header.getFormat();
                header.compression = TextureCompression::NONE;
            }
            else
            {
                header.setError(m_targa_header.error);
            }
        }

        ~Interface()
        {
        }

        ImageDecodeStatus decode(const Surface& surface, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            LittleEndianConstPointer p = m_pointer;

            Palette palette;

            switch (m_targa_header.image_type)
            {
                case IMAGETYPE_NONE:
                {
                    status.setError("[ImageDecoder.TGA] No image data.");
                    return status;
                }

                case IMAGETYPE_MONOCHROME:
                case IMAGETYPE_RLE_MONOCHROME:
                {
                    // grayscale
                    break;
                }

                case IMAGETYPE_PALETTE:
                case IMAGETYPE_RLE_PALETTE:
                {
                    // read palette
                    u32 origin = u32(m_targa_header.colormap_origin);
                    u32 length = u32(m_targa_header.colormap_length);

                    palette.size = origin + length;

                    if (m_targa_header.colormap_bits == 15 || m_targa_header.colormap_bits == 16)
                    {
                        for (u32 i = 0; i < length; ++i)
                        {
                            u16 color = littleEndian::uload16(p);
                            u32 r = (color >> 0) & 0x1f;
                            u32 g = (color >> 5) & 0x1f;
                            u32 b = (color >> 10) & 0x1f;
                            r = (r * 255) / 31;
                            g = (g * 255) / 31;
                            b = (b * 255) / 31;
                            palette[origin + i] = Color(r, g, b, 0xff);
                            p += 2;
                        }
                    }
                    else if (m_targa_header.colormap_bits == 24)
                    {
                        for (u32 i = 0; i < length; ++i)
                        {
                            palette[origin + i] = Color(p[2], p[1], p[0], 0xff);
                            p += 3;
                        }
                    }
                    else if (m_targa_header.colormap_bits == 32)
                    {
                        for (u32 i = 0; i < length; ++i)
                        {
                            palette[origin + i] = Color(p[2], p[1], p[0], p[3]);
                            p += 4;
                        }
                    }
                    break;
                }

                case IMAGETYPE_TRUECOLOR:
                case IMAGETYPE_RLE_TRUECOLOR:
                {
                    p += m_targa_header.colormap_length * ((m_targa_header.colormap_bits + 1) >> 3);
                    break;
                }
            }

            const int width = m_targa_header.width;
            const int height = m_targa_header.height;
            const int bpp = m_targa_header.getBytesPerPixel();
            const bool yflip = (m_targa_header.descriptor & DESCRIPTOR_ORIGIN_Y) == 0;
            const bool xflip = (m_targa_header.descriptor & DESCRIPTOR_ORIGIN_X) != 0;

            Format format = m_targa_header.getFormat();
            DecodeTargetBitmap target(surface, width, height, format, palette, yflip);

            const u8* data = p;
            const u8* end = m_memory.end();

            const size_t bytes_per_scan = width * bpp;
            const size_t bytes_per_image = height * bytes_per_scan;

            if (m_targa_header.isRLE())
            {
                Buffer buffer(bytes_per_image);
                if (!tga_decompress_RLE(buffer, p, end, width, height, bpp))
                {
                    status.setError("[ImageDecoder.TGA] RLE decoding error.");
                    return status;
                }

                target.blit(0, 0, Surface(width, height, format, bytes_per_scan, buffer));
            }
            else
            {
                target.blit(0, 0, Surface(width, height, format, bytes_per_scan, data));
            }

            if (xflip)
            {
                // image origin is at the right
                target.xflip();
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

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

        // configure output
        const bool isalpha = surface.format.isAlpha();
        const Format format = isalpha ? Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8)
                                      : Format(24, Format::UNORM, Format::BGR, 8, 8, 8);
        const int width = surface.width;
        const int height = surface.height;

        // configure header
        HeaderTGA header;

        header.id_length        = 0;
        header.colormap_type    = 0;
        header.image_type       = IMAGETYPE_TRUECOLOR;
        header.colormap_origin  = 0;
        header.colormap_length  = 0;
        header.colormap_bits    = 0;
        header.xoffset          = 0;
        header.yoffset          = 0;
        header.width            = u16(width);
        header.height           = u16(height);
        header.pixel_size       = u8(format.bits);
        header.descriptor       = DESCRIPTOR_ORIGIN_Y | (isalpha ? DESCRIPTOR_ALPHA : 0);

        // write header
        header.write(stream);

        // convert to correct format when required
        TemporaryBitmap temp(surface, format);

        // write image
        const int bytesPerLine = width * format.bytes();

        for (int y = 0; y < height; ++y)
        {
            u8* image = temp.address(0, y);
            stream.write(image, bytesPerLine);
        }

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecTGA()
    {
        registerImageDecoder(createInterface, ".tga");
        registerImageEncoder(imageEncode, ".tga");
    }

} // namespace mango::image
