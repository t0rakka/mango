/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/exception.hpp>
#include <mango/image/image.hpp>

#define ID "[ImageDecoder.TGA] "

namespace
{
    using namespace mango;

	// ------------------------------------------------------------
	// header
	// ------------------------------------------------------------

    enum
    {
        TYPE_RAW_PALETTE   = 1,
        TYPE_RAW_RGB       = 2,
        TYPE_RAW_BW        = 3,
        TYPE_RLE_PALETTE   = 9,
        TYPE_RLE_RGB       = 10,
        TYPE_RLE_BW        = 11,
    };

    struct HeaderTGA
    {
        u8   idfield_length;
        u8   colormap_type;
        u8   data_type;
        u16  colormap_origin;
        u16  colormap_length;
        u8   colormap_bits;
        u16  image_origin_x;
        u16  image_origin_y;
        u16  image_width;
        u16  image_height;
        u8   pixel_size;
        u8   descriptor;

        void read(LittleEndianPointer& p)
        {
            idfield_length   = p.read8();
            colormap_type    = p.read8();
            data_type        = p.read8();
            colormap_origin  = p.read16();
            colormap_length  = p.read16();
            colormap_bits    = p.read8();
            image_origin_x   = p.read16();
            image_origin_y   = p.read16();
            image_width      = p.read16();
            image_height     = p.read16();
            pixel_size       = p.read8();
            descriptor       = p.read8();

            switch (data_type)
            {
                case TYPE_RAW_PALETTE:
                case TYPE_RAW_RGB:
                case TYPE_RAW_BW:
                case TYPE_RLE_PALETTE:
                case TYPE_RLE_RGB:
                case TYPE_RLE_BW:
                    break;
                default:
                    MANGO_EXCEPTION(ID"Invalid data type (%d).", data_type);
            }

            switch (pixel_size)
            {
                case 8:
                case 16:
                case 24:
                case 32:
                    break;
                default:
                    MANGO_EXCEPTION(ID"Invalid pixel size (%d).", pixel_size);
            }

            if (colormap_type > 1)
            {
                MANGO_EXCEPTION(ID"Invalid colormap type (%d).", colormap_type);
            }

            if (data_type == TYPE_RAW_PALETTE || data_type == TYPE_RLE_PALETTE)
            {
                // palette
                if (colormap_length > 256)
                {
                    MANGO_EXCEPTION(ID"Invalid colormap length.", colormap_length);
                }

                if (colormap_bits != 16 && colormap_bits != 24)
                {
                    MANGO_EXCEPTION(ID"Invalid colormap size.", colormap_bits);
                }
            }

            // skip idfield
            p += idfield_length;
        }

        void write(Stream& file)
        {
            LittleEndianStream s(file);

            s.write8(idfield_length);
            s.write8(colormap_type);
            s.write8(data_type);
            s.write16(colormap_origin);
            s.write16(colormap_length);
            s.write8(colormap_bits);
            s.write16(image_origin_x);
            s.write16(image_origin_y);
            s.write16(image_width);
            s.write16(image_height);
            s.write8(pixel_size);
            s.write8(descriptor);
        }

        bool isPalette() const
        {
            return (data_type & 3) == 1;
        }

        bool isRLE() const
        {
            return (data_type & 8) != 0;
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
                        // expand palette to 32 bits
                        format = FORMAT_B8G8R8A8;
                    }
                    else
                    {
                        // keep grayscale at 8 bits
                        format = FORMAT_L8;
                    }
                    break;

                case 16:
                    format = FORMAT_B5G5R5X1;
                    break;

                case 24:
                    format = FORMAT_B8G8R8;
                    break;

                case 32:
                    format = FORMAT_B8G8R8A8;
                    break;
            }

            return format;
        }
    };

	// ------------------------------------------------------------
	// tga code
	// ------------------------------------------------------------

    void decompressRLE(u8* temp, u8* p, int width, int height, int bpp)
    {
        int x = 0;
        int y = 0;
        u8* buffer = temp;

        for (; y < height;)
        {
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
                const int left = width - x;
                const int size = std::min(count, left);

                if (sample & 0x80)
                {
                    // repeat color
                    for (int i = 0; i < size; ++i)
                    {
                        for (int j = 0; j < bpp; ++j)
                        {
                            buffer[j] = color[j];
                        }
                        buffer += bpp;
                    }
                }
                else
                {
                    int bytes = size * bpp;
                    std::memcpy(buffer, color, bytes);
                    p += bytes;
                    buffer += bytes;
                    color += bytes;
                }

                count -= size;
                x += size;

                if (x >= width)
                {
                    ++y;
                    x = 0;
                    temp += width * bpp;
                    buffer = temp;
                }
            }
        }
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        HeaderTGA m_header;
        u8* m_pointer;

        Interface(Memory memory)
        {
            LittleEndianPointer p = memory.address;
            m_header.read(p);
            m_pointer = p;
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            ImageHeader header;
            header.width   = m_header.image_width;
            header.height  = m_header.image_height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = m_header.isPalette();
            header.format  = m_header.getFormat();
            header.compression = TextureCompression::NONE;
            return header;
        }

        void decode(Surface& surface, Palette* ptr_palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(ptr_palette);
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            LittleEndianPointer p = m_pointer;

            Palette palette;

            switch (m_header.data_type)
            {
                case TYPE_RAW_BW:
                case TYPE_RLE_BW:
                {
                    // grayscale
                    break;
                }

                case TYPE_RAW_PALETTE:
                case TYPE_RLE_PALETTE:
                {
                    // read palette
                    palette.size = u32(m_header.colormap_length);

                    if (m_header.colormap_bits == 16)
                    {
                        for (u32 i = 0; i < palette.size; ++i)
                        {
                            u16 color = uload16le(p);
                            u32 r = (color >> 0) & 0x1f;
                            u32 g = (color >> 5) & 0x1f;
                            u32 b = (color >> 10) & 0x1f;
                            r = (r * 255) / 31;
                            g = (g * 255) / 31;
                            b = (b * 255) / 31;
                            palette[i] = ColorBGRA(r, g, b, 0xff);
                            p += 2;
                        }
                    }
                    else if (m_header.colormap_bits == 24)
                    {
                        for (u32 i = 0; i < palette.size; ++i)
                        {
                            palette[i] = ColorBGRA(p[2], p[1], p[0], 0xff);
                            p += 3;
                        }
                    }
                    break;
                }

                case TYPE_RAW_RGB:
                case TYPE_RLE_RGB:
                {
                    p += m_header.colormap_length * ((m_header.colormap_bits + 1) >> 3);
                    break;
                }
            }

            Format format = m_header.getFormat();

            const int width = m_header.image_width;
            const int height = m_header.image_height;
            const int bpp = m_header.getBytesPerPixel();

            Surface dest(surface, 0, 0, width, height);

            if (m_header.descriptor & 0x20)
            {
                dest.image = surface.image;
                dest.stride = surface.stride;
            }
            else
            {
                // flip the image upside down
                dest.image = surface.image + (height - 1) * surface.stride;
                dest.stride = -surface.stride;
            }

            u8* temp = nullptr;
            u8* data = p;

            if (m_header.isRLE())
            {
                temp = new u8[width * height * bpp];
                decompressRLE(temp, p, width, height, bpp);
                data = temp;
            }

            switch (m_header.data_type)
            {
                case TYPE_RAW_BW:
                case TYPE_RLE_BW:
                case TYPE_RAW_RGB:
                case TYPE_RLE_RGB:
                {
                    dest.blit(0, 0, Surface(width, height, format, width * bpp, data));
                    break;
                }

                case TYPE_RAW_PALETTE:
                case TYPE_RLE_PALETTE:
                {
                    if (ptr_palette)
                    {
                        *ptr_palette = palette;
                        dest.blit(0, 0, Surface(width, height, FORMAT_L8, width, data));
                    }
                    else
                    {
                        Bitmap bitmap(width, height, FORMAT_B8G8R8A8);
                        for (int y = 0; y < height; ++y)
                        {
                            ColorBGRA* d = bitmap.address<ColorBGRA>(0, y);
                            u8* s = data + y * width;
                            for (int x = 0; x < width; ++x)
                            {
                                d[x] = palette[s[x]];
                            }
                        }
                        dest.blit(0, 0, bitmap);
                    }
                    break;
                }
            }

            delete[] temp;
        }
    };

    ImageDecoderInterface* createInterface(Memory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    void imageEncode(Stream& stream, const Surface& surface, float quality)
    {
        MANGO_UNREFERENCED_PARAMETER(quality);

        // configure output
        const bool isalpha = surface.format.alpha();
        const Format format = isalpha ? FORMAT_B8G8R8A8 : FORMAT_B8G8R8;
        const int width = surface.width;
        const int height = surface.height;

        // configure header
        HeaderTGA header;

        header.idfield_length   = 0;
        header.colormap_type    = 0;
        header.data_type        = 2;
        header.colormap_origin  = 0;
        header.colormap_length  = 0;
        header.colormap_bits    = 0;
        header.image_origin_x   = 0;
        header.image_origin_y   = 0;
        header.image_width      = static_cast<u16>(width);
        header.image_height     = static_cast<u16>(height);
        header.pixel_size       = static_cast<u8>(format.bits);
        header.descriptor       = 0x20 | (isalpha ? 8 : 0);

        // write header
        header.write(stream);

        // write image
        if (format != surface.format)
        {
            Bitmap temp(width, height, format);
            temp.blit(0, 0, surface);
            stream.write(temp.image, width * height * format.bytes());
        }
        else
        {
            u8* image = surface.image;
            const int bytesPerLine = width * format.bytes();

            for (int y = 0; y < height; ++y)
            {
                stream.write(image, bytesPerLine);
                image += surface.stride;
            }
        }
    }

} // namespace

namespace mango
{

    void registerImageDecoderTGA()
    {
        registerImageDecoder(createInterface, ".tga");
        registerImageEncoder(imageEncode, ".tga");
    }

} // namespace mango
