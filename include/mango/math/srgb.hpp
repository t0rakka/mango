/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // sRGB
    // ------------------------------------------------------------------

    float srgb_encode(float n);
    float srgb_decode(float s);
    float4 srgb_encode(float4 n);
    float4 srgb_decode(float4 s);
    float4 srgb_encode_packed(uint32 n);
    uint32 srgb_decode_packed(float4 s);
    
} // namespace mango
