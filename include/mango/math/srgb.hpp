/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango::math
{

    // ------------------------------------------------------------------
    // sRGB <-> linear conversion functions
    // ------------------------------------------------------------------

    float linear_to_srgb(float linear);
    float srgb_to_linear(float srgb);

    float32x4 linear_to_srgb(float32x4 linear);
    float32x4 srgb_to_linear(float32x4 srgb);

} // namespace mango::math
