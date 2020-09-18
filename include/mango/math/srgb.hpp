/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango
{

    // ------------------------------------------------------------------
    // linear / non-linear (sRGB) conversion functions
    // ------------------------------------------------------------------

    // The conversion functions do NOT consider w-component to contain alpha;
    // every component is converted between linear and non-linear sRGB.
    // The inputs and outputs are normalized.

    float linear_to_srgb(float linear);
    float srgb_to_linear(float srgb);

    float32x4 linear_to_srgb(float32x4 linear);
    float32x4 srgb_to_linear(float32x4 srgb);

    // ------------------------------------------------------------------
    // sRGB
    // ------------------------------------------------------------------

    // sRGB implements non-linear color container where each color component
    // is stored as 8 bit UNORM sRGB. 
    // The alpha component is always stored as a linear value.

    struct sRGB
    {
        u32 color;

        sRGB() = default;

        sRGB(u32 srgb)
            : color(srgb)
        {
        }

        sRGB(float32x4 linear)
        {
            *this = linear;
        }

        sRGB& operator = (float32x4 linear)
        {
            float32x4 srgb = linear_to_srgb(linear) * 255.0f;
            srgb.w = linear.w * 255.0f; // pass-through linear alpha
            color = srgb.pack();
            return *this;
        }

        sRGB& operator = (u32 srgb)
        {
            color = srgb;
            return *this;
        }

        operator u32 () const
        {
            return color;
        }

        operator float32x4 () const
        {
            float32x4 srgb;
            srgb.unpack(color);
            float32x4 linear = srgb_to_linear(srgb / 255.0f);
            linear.w = float(color >> 24) / 255.0f; // pass-through linear alpha
            return linear;
        }
    };

} // namespace mango
