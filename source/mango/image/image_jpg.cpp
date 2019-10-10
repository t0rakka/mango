/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#ifdef MANGO_ENABLE_IMAGE_JPG

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

        Memory icc() override
        {
            return m_parser.icc_buffer;
        }

        Memory exif() override
        {
            return m_parser.exif_memory;
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

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status = jpeg::encodeImage(stream, surface, options.quality);
        return status;
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

#endif // MANGO_ENABLE_IMAGE_JPG
