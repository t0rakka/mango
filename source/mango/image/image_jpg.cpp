/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#include "../jpeg/jpeg.hpp"

namespace
{
    using namespace mango;
    using namespace mango::image;

	// ------------------------------------------------------------
	// ImageDecoder
	// ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        jpeg::Parser m_parser;

        Interface(ConstMemory memory)
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

        ConstMemory icc() override
        {
            return Memory(m_parser.icc_buffer);
        }

        ConstMemory exif() override
        {
            return m_parser.exif_memory;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status = m_parser.decode(dest, options);
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
        ImageEncodeStatus status = jpeg::encodeImage(stream, surface, options);
        return status;
    }

} // namespace

namespace mango::image
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

} // namespace mango::image
