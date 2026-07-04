/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
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

    struct Interface : ImageDecodeInterface
    {
        jpeg::Parser m_parser;

        Interface(ConstMemory memory)
            : m_parser(this, memory)
        {
            // lossless must decode in scan order; band callbacks are not used
            async = !m_parser.isLossless();
            header = m_parser.header;
            icc = m_parser.icc_buffer;
            exif = m_parser.exif_memory;

            // JPEG is sRGB by convention (the default). When an ICC profile is embedded it
            // defines the color space exactly, so defer to it. The exception is UltraHDR:
            // the gain map path outputs linear fp16 in explicit primaries, and the embedded
            // ICC describes the original SDR base, not our linearized result - keep our signalling.
            if (icc.size && header.color.transfer != TransferFunction::Linear)
            {
                header.color.primaries = ColorPrimaries::Unspecified;
                header.color.transfer = TransferFunction::Unspecified;
            }
        }

        ~Interface()
        {
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status = m_parser.decode(dest, options);

            if (m_parser.cmykIccApplied())
            {
                icc = ConstMemory();
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
        ImageEncodeStatus status = jpeg::encodeImage(stream, surface, options);
        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecJPG()
    {
        registerImageDecoder(createInterface, ".jpg");
        registerImageDecoder(createInterface, ".jpeg");
        registerImageDecoder(createInterface, ".jfif");
        registerImageDecoder(createInterface, ".mpo");
        registerImageDecoder(createInterface, ".jpe");
        registerImageDecoder(createInterface, ".jps");
        registerImageEncoder(imageEncode, ".jpg");
        registerImageEncoder(imageEncode, ".jpeg");
    }

} // namespace mango::image
