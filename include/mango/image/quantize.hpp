/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "format.hpp"
#include "surface.hpp"

namespace mango {
namespace image {

    struct ColorQuantizeOptions
    {
        float quality = 0.90f;
        bool dithering = true;
    };

    struct ColorQuantizer
    {
        Palette palette;

        void quantize(const Surface& dest, const Surface& source, const ColorQuantizeOptions& options);
    };

} // namespace image
} // namespace mango
