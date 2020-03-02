/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "format.hpp"
#include "surface.hpp"

namespace mango {
namespace image {

    struct ColorQuantizer
    {
        struct ColorQuantizerContext* context;

        // Initialize quantization network and generate palette (from the colors in source surface)
        ColorQuantizer(const Surface& source, float quality = 0.90f);
        ~ColorQuantizer();

        // Get generated palette
        Palette getPalette() const;

        // Quantize ANY image with the quantization network (the original color image is recommended)
        void quantize(const Surface& dest, const Surface& source, bool dithering = true);
    };

} // namespace image
} // namespace mango
