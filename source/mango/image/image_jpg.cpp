/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
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
            ImageHeader header;

            header.width   = m_parser.header.width;
            header.height  = m_parser.header.height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = false;
            header.format  = m_parser.header.format;
            header.compression = TextureCompression::NONE;

            return header;
        }

        Exif exif() override
        {
            if (m_parser.exif_memory.address)
            {
                return Exif(m_parser.exif_memory);
            }

            return Exif();
        }

        void decode(Surface& dest, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(palette);
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            jpeg::Status s = m_parser.decode(dest);
            MANGO_UNREFERENCED_PARAMETER(s);
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
        jpeg::encodeImage(stream, surface, quality);
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
