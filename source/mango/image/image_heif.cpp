/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_HEIF)

#include <libheif/heif.h>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ImageHeader m_header;

        Interface(ConstMemory memory)
        {
            // TODO
            int width = 100;
            int height = 100;
            Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

            m_header.width   = width;
            m_header.height  = height;
            m_header.depth   = 0;
            m_header.levels  = 0;
            m_header.faces   = 0;
            m_header.palette = false;
            m_header.format  = format;
            m_header.compression = TextureCompression::NONE;
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ConstMemory icc() override
        {
            return ConstMemory();
        }

        ConstMemory exif() override
        {
            return ConstMemory();
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            // TODO
            ImageDecodeStatus status;
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

    ImageEncodeStatus imageEncode(Stream& output, const Surface& surface, const ImageEncodeOptions& options)
    {
        // TODO
        ImageEncodeStatus status;
        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecHEIF()
    {
        // TODO
        MANGO_UNREFERENCED(createInterface);
        MANGO_UNREFERENCED(imageEncode);
        /*
        registerImageDecoder(createInterface, ".xxx");
        registerImageEncoder(imageEncode, ".xxx");
        */
    }

} // namespace mango::image

#endif // defined(MANGO_ENABLE_HEIF)
