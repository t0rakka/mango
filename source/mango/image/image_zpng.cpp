/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#include "../../external/zpng/zpng.h"

namespace
{
    using namespace mango;
    using namespace mango::image;

	// ------------------------------------------------------------
	// ImageDecoder
	// ------------------------------------------------------------

    struct zpng_header
    {
        u16le magic;
        u16le width;
        u16le height;
        u8 channels;
        u8 bytes_per_channel;
    };

    Format resolve_format(int channels, int bytes_per_channel)
    {
        Format format;

        // TODO: this format selection assumes that the format supports 1, 3 or 4 channels and 8 and 16 bits per channel
        // TODO: probably should switch with number of channels -and- bytes per channel separately
        int bytes_per_pixel = bytes_per_channel * channels;
        switch (bytes_per_pixel)
        {
            case 1:
                format = LuminanceFormat(8, Format::UNORM, 8, 0);
                break;
            case 3:
                format = Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0);
                break;
            case 4:
                format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                break;
            case 2:
                format = LuminanceFormat(16, Format::UNORM, 16, 0);
                break;
            case 6:
                format = Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0);
                break;
            case 8:
                format = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
                break;
            default:
                break;
        }

        return format;
    }

    struct Interface : ImageDecoderInterface
    {
        ZPNG_Buffer m_buffer;
        ImageHeader m_header;

        Interface(ConstMemory memory)
        {
            m_buffer.Data = const_cast<u8*>(memory.address);
            m_buffer.Bytes = static_cast<unsigned int>(memory.size);

            const zpng_header* zheader = reinterpret_cast<const zpng_header *>(memory.address);
            if (zheader->magic != 0xfbf8)
            {
                m_header.setError("[ImageDecoder.ZPNG] Incorrect identifier.");
            }
            else
            {
                m_header.width   = zheader->width;
                m_header.height  = zheader->height;
                m_header.depth   = 0;
                m_header.levels  = 0;
                m_header.faces   = 0;
                m_header.palette = false;
                m_header.format  = resolve_format(zheader->channels, zheader->bytes_per_channel);
                m_header.compression = TextureCompression::NONE;
            }
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!m_header.success)
            {
                status.setError(m_header.info);
                return status;
            }

            ZPNG_ImageData z = ZPNG_Decompress(m_buffer);
            if (z.Buffer.Data)
            {
                Format format = resolve_format(z.Channels, z.BytesPerChannel);

                /*
                Bitmap temp(z.WidthPixels, z.HeightPixels, format);

                u8* d = temp.image;
                u8* s = z.Buffer.Data;
                int bytes = temp.width * format.bytes();

                for (int y = 0; y < temp.height; ++y)
                {
                    std::memcpy(d, s, bytes);
                    d += temp.stride;
                    s += z.StrideBytes;
                }
                */

                Surface temp(z.WidthPixels, z.HeightPixels, format, z.StrideBytes, z.Buffer.Data);
                dest.blit(0, 0, temp);

                ZPNG_Free(&z.Buffer);
            }

            return status;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

        // TODO: optimize encoder
        Bitmap temp(surface.width, surface.height, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
        temp.blit(0, 0, surface);

        ZPNG_ImageData z;

        z.Buffer.Data = temp.image;
        z.Buffer.Bytes = 0;

        z.BytesPerChannel = 1;
        z.Channels = 4;
        z.WidthPixels = surface.width;
        z.HeightPixels = surface.height;
        z.StrideBytes = int(temp.stride);

        // compress image
        ZPNG_Buffer buf = ZPNG_Compress(&z);

        // write compressed bytes into the result stream
        stream.write(buf.Data, buf.Bytes);

        // free compressed image
        ZPNG_Free(&buf);

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageDecoderZPNG()
    {
        registerImageDecoder(createInterface, ".zpng");
        registerImageEncoder(imageEncode, ".zpng");
    }

} // namespace mango::image
