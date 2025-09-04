/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango::math
{

    // ------------------------------------------------------------------
    // sRGB <-> linear conversion functions
    // ------------------------------------------------------------------

    //
    // http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
    //

    template <typename T>
    static inline
    T linear_to_srgb(T linear)
    {
        linear = clamp(linear, 0.0f, 1.0f);
        T s1 = sqrt(linear);
        T s2 = sqrt(s1);
        T s3 = sqrt(s2);
        return 0.662002687f * s1 + 0.684122060f * s2 - 0.323583601f * s3 - 0.0225411470f * linear;
    }

    template <typename T>
    static inline
    T srgb_to_linear(T s)
    {
        return s * (s * (s * 0.305306011f + 0.682171111f) + 0.012522878f);
    }

    // ------------------------------------------------------------------
    // sRGB <-> linear conversion lookup tables
    // ------------------------------------------------------------------

    const u8* get_linear_to_srgb_table();
    const u8* get_srgb_to_linear_table();

} // namespace mango::math
