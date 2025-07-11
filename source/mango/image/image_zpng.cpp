/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        littleEndian::u16 magic;
        littleEndian::u16 width;
        littleEndian::u16 height;
        u8 channels;
        u8 bytes_per_channel;
    };

    Format resolve_format(int channels, int bytes_per_channel)
    {
        Format format;

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

    struct Interface : ImageDecodeInterface
    {
        ZPNG_Buffer m_buffer;

        Interface(ConstMemory memory)
        {
            m_buffer.Data = const_cast<u8*>(memory.address);
            m_buffer.Bytes = static_cast<unsigned int>(memory.size);

            const zpng_header* zheader = reinterpret_cast<const zpng_header *>(memory.address);
            if (zheader->magic != 0xfbf8)
            {
                header.setError("[ImageDecoder.ZPNG] Incorrect identifier.");
            }
            else
            {
                header.width   = zheader->width;
                header.height  = zheader->height;
                header.depth   = 0;
                header.levels  = 0;
                header.faces   = 0;
                header.format  = resolve_format(zheader->channels, zheader->bytes_per_channel);
                header.compression = TextureCompression::NONE;

                if (!header.format.bits)
                {
                    header.setError("[ImageDecoder.ZPNG] Unsupported format.");
                }
            }
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

            ZPNG_ImageData z = ZPNG_Decompress(m_buffer);
            if (z.Buffer.Data)
            {
                Format format = resolve_format(z.Channels, z.BytesPerChannel);

                if (!format.bits)
                {
                    ZPNG_Free(&z.Buffer);
                    status.setError("[ImageDecoder.ZPNG] Unsupported format.");
                    return status;
                }

                Surface temp(z.WidthPixels, z.HeightPixels, format, z.StrideBytes, z.Buffer.Data);
                dest.blit(0, 0, temp);

                ZPNG_Free(&z.Buffer);
            }

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

        TemporaryBitmap temp(surface, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        ZPNG_ImageData z;

        z.Buffer.Data = temp.image;
        z.Buffer.Bytes = 0;

        z.BytesPerChannel = 1;
        z.Channels = 4;
        z.WidthPixels = temp.width;
        z.HeightPixels = temp.height;
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

    void registerImageCodecZPNG()
    {
        registerImageDecoder(createInterface, ".zpng");
        registerImageEncoder(imageEncode, ".zpng");
    }

} // namespace mango::image
