/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // sRGB
    // ------------------------------------------------------------------

    float srgb_encode(float n); // linear to sRGB
    float srgb_decode(float s); // sRGB to linear

    float4 srgb_encode(float4 n); // linear to sRGB
    float4 srgb_decode(float4 s); // sRGB to linear

} // namespace mango
