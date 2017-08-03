/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cassert>
#include "vector.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // MatrixBase
    // ------------------------------------------------------------------

    template <typename ScalarType, int Width, int Height>
    struct MatrixBase
    {
        using VectorType = Vector<ScalarType, Width>;

        operator const float* () const
        {
            return reinterpret_cast<const float*>(this);
        }

        operator float* ()
        {
            return reinterpret_cast<float*>(this);
        }

        const VectorType& operator [] (int y) const
        {
            assert(y >= 0 && y < Height);
            return reinterpret_cast<const VectorType *>(this)[y];
        }

        VectorType& operator [] (int y)
        {
            assert(y >= 0 && y < Height);
            return reinterpret_cast<VectorType *>(this)[y];
        }

        float operator () (int y, int x) const
        {
            assert(x >= 0 && x < Width);
            assert(y >= 0 && y < Height);
            return reinterpret_cast<const float *>(this)[y * Width + x];
        }

        float& operator () (int y, int x)
        {
            assert(x >= 0 && x < Width);
            assert(y >= 0 && y < Height);
            return reinterpret_cast<float *>(this)[y * Width + x];
        }
    };

    // ------------------------------------------------------------------
    // Matrix
    // ------------------------------------------------------------------

    template <typename ScalarType, int Width, int Height>
    struct Matrix : MatrixBase<ScalarType, Width, Height>
    {
        using VectorType = Vector<ScalarType, Width>;

        VectorType m[Height];

        explicit Matrix()
        {
        }

        ~Matrix()
        {
        }
    };

    // ------------------------------------------------------------------
    // typedefs
    // ------------------------------------------------------------------

    typedef Matrix<float, 3, 3> float3x3;
    typedef Matrix<float, 4, 4> float4x4;

} // namespace mango

#include "matrix_float4x4.hpp"

