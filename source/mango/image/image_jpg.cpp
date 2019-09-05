/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include "../jpeg/jpeg.hpp"

#define ID "[ImageDecoder.JPG] "

namespace
{
    using namespace mango;

	// ------------------------------------------------------------
	// ImageDecoder
	// ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        jpeg::Parser m_parser;

        Interface(Memory memory)
            : m_parser(memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_parser.header;
        }

        Exif exif() override
        {
            if (m_parser.exif_memory.address)
            {
                return Exif(m_parser.exif_memory);
            }

            return Exif();
        }

        ImageDecodeStatus decode(Surface& dest, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(palette);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status = m_parser.decode(dest);
            return status;
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

    void imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        jpeg::encodeImage(stream, surface, options.quality);
    }

} // namespace

namespace mango
{

    void registerImageDecoderJPG()
    {
        registerImageDecoder(createInterface, ".jpg");
        registerImageDecoder(createInterface, ".jpeg");
        registerImageDecoder(createInterface, ".jfif");
        registerImageDecoder(createInterface, ".mpo");
        registerImageEncoder(imageEncode, ".jpg");
        registerImageEncoder(imageEncode, ".jpeg");
    }

} // namespace mango
