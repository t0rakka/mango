/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "format.hpp"
#include "surface.hpp"

namespace mango {
namespace image {

    struct ColorQuantizer
    {
        Palette palette;

        void quantize(const Surface& dest, const Surface& source, bool dithering, float quality);
    };

} // namespace image
} // namespace mango
