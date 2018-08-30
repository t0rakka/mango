/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#define ID "ImageStream.RGB: "

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        Interface(Memory memory)
        {
            // TODO
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            ImageHeader header;

            // TODO
            //header.width   = ;
            //header.height  = ;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = false;
            //header.format  = ;
            header.compression = TextureCompression::NONE;

            return header;
        }

        void decode(Surface& dest, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(palette);
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);
            // TODO
        }
    };

    ImageDecoderInterface* createInterface(Memory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango
{

    void registerImageDecoderSGI()
    {
        registerImageDecoder(createInterface, "rgb");
        registerImageDecoder(createInterface, "rgba");
        registerImageDecoder(createInterface, "bw");
        registerImageDecoder(createInterface, "sgi");
    }

} // namespace mango
