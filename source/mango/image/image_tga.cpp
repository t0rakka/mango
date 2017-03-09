/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/exception.hpp>
#include <mango/image/image.hpp>

#define ID "ImageStream.TGA: "

namespace
{
    using namespace mango;

	// ------------------------------------------------------------
	// header
	// ------------------------------------------------------------

    struct HeaderTGA
    {
        uint8   idfield_length;
        uint8   colormap_type;
        uint8   data_type;
        uint16  colormap_origin;
        uint16  colormap_length;
        uint8   colormap_bits;
        uint16  image_origin_x;
        uint16  image_origin_y;
        uint16  image_width;
        uint16  image_height;
        uint8   pixel_size;
        uint8   descriptor;

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
                case 1:
                case 2:
                case 3:
                case 9:
                case 10:
                    break;
                default:
                    MANGO_EXCEPTION(ID"Invalid data type.");
            }

            switch (pixel_size)
            {
                case 8:
                case 16:
                case 24:
                case 32:
                    break;
                default:
                    MANGO_EXCEPTION(ID"Invalid pixel size.");
            }

            if (colormap_type > 1)
            {
                MANGO_EXCEPTION(ID"Invalid colormap type.");
            }

            if (data_type == 1 || data_type == 9)
            {
                // palette
                if (colormap_bits != 24 || colormap_length > 256)
                {
                    MANGO_EXCEPTION(ID"Invalid colormap size.");
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

        Format getFormat() const
        {
            Format format;

            switch (pixel_size)
            {
                case 8:
                    if (data_type == 3)
                    {
                        // expand grayscale to 32 bits
                        format = FORMAT_L8;
                    }
                    else
                    {
                        // expand palette to 32 bits
                        format = FORMAT_B8G8R8A8;
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

    void writeRGB(uint8* buffer, const uint8* color, int depth, int size)
    {
        for (int i = 0; i < size; ++i)
        {
            switch (depth)
            {
                // fall-through
                case 4: buffer[3] = color[3];
                case 3: buffer[2] = color[2];
                case 2: buffer[1] = color[1];
                case 1: buffer[0] = color[0];
            }

            buffer += depth;
        }
    }

    void writePAL(uint8* buffer, const uint8* p, const uint8* palette, int depth, int size)
    {
        for (int i = 0; i < size; ++i)
        {
            const uint8* color = palette + p[i] * depth;

            switch (depth)
            {
                // fall-through
                case 4: buffer[3] = color[3];
                case 3: buffer[2] = color[2];
                case 2: buffer[1] = color[1];
                case 1: buffer[0] = color[0];
            }

            buffer += depth;
        }
    }

    void readImageRAW(Surface& dest, Surface& src, const uint8* palette)
    {
        if (palette)
        {
            const int depth = dest.format.bytes();

            for (int y = 0; y < src.height; ++y)
            {
                writePAL(dest.image, src.image, palette, depth, src.width);
                src.image += src.width;
                dest.image += dest.stride;
            }
        }
        else
        {
            dest.blit(0, 0, src);
        }
    }

	void readImageRLE(const Surface& dest, const Surface& src, const uint8* palette)
	{
        uint8* buffer = dest.image;
        uint8* p = src.image;
        int x = 0;
        int y = 0;

        Blitter blitter(dest.format, src.format);
        uint8* temp = NULL;

        BlitRect rect;
        rect.destImage = dest.image;
        rect.destStride = dest.stride;
        rect.srcImage = src.image;
        rect.srcStride = src.stride;
        rect.width = dest.width;
        rect.height = 1;

        int depth;
        int sample_depth;

        if (palette)
        {
            depth = dest.format.bytes();
            sample_depth = 1;
        }
        else
        {
            depth = src.format.bytes();
            sample_depth = depth;

            if (dest.format != src.format)
            {
                temp = new uint8[src.width * depth];
                buffer = temp;
                rect.srcImage = temp;
            }
        }

        for (; y < src.height;)
        {
            uint8 sample = *p++;
            int count = (sample & 0x7f) + 1;

            const uint8 *color = palette ? palette + p[0] * depth : p;

            if (sample & 0x80)
            {
                p += sample_depth;
            }

            for ( ; count > 0; )
            {
                // clip to right edge
                const int left = src.width - x;
                const int size = std::min(count, left);

                if (sample & 0x80)
                {
                    writeRGB(buffer, color, depth, size);
                }
                else
                {
                    if (palette)
                    {
                        writePAL(buffer, p, palette, depth, size);
                    }
                    else
                    {
                        std::memcpy(buffer, p, size * depth);
                    }

                    p += size * sample_depth;
                }

                buffer += size * depth;

                count -= size;
                x += size;

                if (x >= src.width)
                {
                    ++y;
                    x = 0;
                    if (temp)
                    {
                        // NOTE: the conversion could be asynchronous but tga format is not high on priorities
                        blitter.convert(rect);
                        rect.destImage += rect.destStride;
                        buffer = temp;
                    }
                    else
                    {
                        rect.destImage += rect.destStride;
                        buffer = rect.destImage;
                    }
                }
            }
        }

        delete [] temp;
	}

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        HeaderTGA m_header;
        uint8* m_pointer;

        Interface(const Memory& memory)
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
            header.width  = m_header.image_width;
            header.height = m_header.image_height;
            header.depth  = 0;
            header.levels = 0;
            header.faces  = 0;
            header.format = m_header.getFormat();
            header.compression = TextureCompression::NONE;
            return header;
        }

        void decode(Surface& surface, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            LittleEndianPointer p = m_pointer;

            // read palette
            uint8 paletteImage[1024];
            uint8* palette = nullptr;

            switch (m_header.data_type)
            {
                case 3:
                {
                    // grayscale
                    break;
                }

                case 1:
                case 9:
                {
                    // rgb palette
                    int paletteSize = int(m_header.colormap_length);

                    // convert 24 bit palette into target surface format using blitter
                    Surface srcPalette(paletteSize, 1, FORMAT_B8G8R8, 0, p);
                    Surface destPalette(paletteSize, 1, surface.format, 0, paletteImage);
                    destPalette.blit(0, 0, srcPalette);

                    palette = paletteImage;
                    p += paletteSize * 3;
                    break;
                }

                case 2:
                case 10:
                {
                    p += m_header.colormap_length * ((m_header.colormap_bits + 1) >> 3);
                    break;
                }
            }

            const int width = m_header.image_width;
            const int height = m_header.image_height;
            Format format = m_header.getFormat();

            Surface src(width, height, format, width * format.bytes(), p);
            Surface dest(surface, 0, 0, width, height);

            if (m_header.descriptor & 0x20)
            {
                dest.image = surface.image;
                dest.stride = surface.stride;
            }
            else
            {
                dest.image = surface.image + (height - 1) * surface.stride;
                dest.stride = -surface.stride;
            }

            // decode image
            switch (m_header.data_type)
            {
                case 1:
                case 2:
                case 3:
                    // linear memory layout can use the blitter to read the image
                    readImageRAW(dest, src, palette);
                    break;

                case 9:
                case 10:
                    // run-length encoded layout requires custom routine
                    readImageRLE(dest, src, palette);
                    break;
            }
        }
    };

    ImageDecoderInterface* createInterface(const Memory& memory)
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
        header.image_width      = static_cast<uint16>(width);
        header.image_height     = static_cast<uint16>(height);
        header.pixel_size       = static_cast<uint8>(format.bits());
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
            uint8* image = surface.image;
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

    void registerTGA()
    {
        registerImageDecoder(createInterface, "tga");
        registerImageEncoder(imageEncode, "tga");
    }

} // namespace mango
