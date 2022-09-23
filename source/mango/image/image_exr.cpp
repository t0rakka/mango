/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    /*
    struct Interface : ImageDecoderInterface
    {
        Interface(ConstMemory memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
        }

        ConstMemory memory(int level, int depth, int face) override
        {
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }
    */

} // namespace

namespace mango::image
{

    void registerImageCodecEXR()
    {
        // TODO
        //registerImageDecoder(createInterface, ".exr");
    }

} // namespace mango::image
