/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // spline interpolation
    // ------------------------------------------------------------------

    template <typename VectorType, typename ScalarType>
    inline VectorType bezier(VectorType a, VectorType b, VectorType c, VectorType d, ScalarType time)
    {
        ScalarType time2 = time * time;
        ScalarType time3 = time * time2;
        ScalarType tp1 = ScalarType(1.0) - time;
        ScalarType tp2 = tp1 * tp1;
        return a * tp2 * tp1 +
               b * ScalarType(3.0) * tp2 * time +
               c * ScalarType(3.0) * tp1 * time2 +
               d * time3;
    }

    template <typename VectorType, typename ScalarType>
    inline VectorType catmull(VectorType a, VectorType b, VectorType c, VectorType d, ScalarType time)
    {
        ScalarType time2 = time * time;
        ScalarType time3 = time * time2;
        VectorType a5 = a * ScalarType(-0.5);
        VectorType d5 = d * ScalarType(0.5);
        return VectorType(time3 * (a5 + (b - c) * ScalarType(1.5) + d5) +
                          time2 * (a - b * ScalarType(2.5) + c * ScalarType(2.0) - d5) +
                          time * (a5 + c * ScalarType(0.5)) + b);
    }

    template <typename VectorType, typename ScalarType>
    inline VectorType bicubic(VectorType a, VectorType b, VectorType c, VectorType d, ScalarType time)
    {
        ScalarType time2 = time * time;
        ScalarType time3 = time * time2;
        VectorType s1 = c - a;
        VectorType s2 = d - b;
        return time3 * ((b - c) * ScalarType(2.0) + s1 + s2) +
               time2 * ((c - b) * ScalarType(3.0) - s1 * ScalarType(2.0) - s2) +
               time * s1 + b;
    }

    template <typename VectorType, typename ScalarType>
    inline VectorType bspline(VectorType a, VectorType b, VectorType c, VectorType d, ScalarType time)
    {
        ScalarType time2 = time * time;
        ScalarType time3 = time * time2;
        VectorType c3 = c * ScalarType(3.0);
        VectorType a3 = a * ScalarType(3.0);
        return (time3 * (b * ScalarType(3.0) - c3 + d - a) +
                time2 * (a3 - b * ScalarType(6.0) + c3) +
                time * (c3 - a3) + (a + b * ScalarType(4.0) + c)) / ScalarType(6.0);
    }

} // namespace mango
