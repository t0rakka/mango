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
    // specializations
    // ------------------------------------------------------------------

    template <>
    struct Matrix<float, 2, 2> : MatrixBase<float, 2, 2>
    {
        float32x2 m[2];

        explicit Matrix()
        {
        }

        explicit Matrix(float s0, float s1, float s2, float s3)
        {
            m[0] = float32x2(s0, s1);
            m[1] = float32x2(s2, s3);
        }

        ~Matrix()
        {
        }

        operator float32x2* ()
        {
            return m;
        }

        operator const float32x2* () const
        {
            return m;
        }
    };

    template <>
    struct Matrix<float, 3, 3> : MatrixBase<float, 3, 3>
    {
        float32x3 m[3];

        explicit Matrix()
        {
        }

        explicit Matrix(float s0, float s1, float s2, float s3, float s4, float s5, float s6, float s7, float s8)
        {
            m[0] = float32x3(s0, s1, s2);
            m[1] = float32x3(s3, s4, s5);
            m[2] = float32x3(s6, s7, s8);
        }

        ~Matrix()
        {
        }

        operator float32x3* ()
        {
            return m;
        }

        operator const float32x3* () const
        {
            return m;
        }
    };

    static inline Matrix<float, 2, 2> operator * (const Matrix<float, 2, 2>& m, float s)
    {
        Matrix<float, 2, 2> result;
        result[0] = m[0] * s;
        result[1] = m[1] * s;
        return result;
    }

    static inline Vector<float, 2> operator * (const Vector<float, 2>& v, const Matrix<float, 2, 2>& m)
    {
        float x = v[0] * m(0, 0) + v[1] * m(1, 0);
        float y = v[0] * m(0, 1) + v[1] * m(1, 1);
        return Vector<float, 2>(x, y);
    }

    static inline Matrix<float, 3, 3> operator * (const Matrix<float, 3, 3>& m, float s)
    {
        Matrix<float, 3, 3> result;
        result[0] = m[0] * s;
        result[1] = m[1] * s;
        result[2] = m[2] * s;
        return result;
    }

    static inline Vector<float, 3> operator * (const Vector<float, 3>& v, const Matrix<float, 3, 3>& m)
    {
        float x = v[0] * m(0, 0) + v[1] * m(1, 0) + v[2] * m(2, 0);
        float y = v[0] * m(0, 1) + v[1] * m(1, 1) + v[2] * m(2, 1);
        float z = v[0] * m(0, 2) + v[1] * m(1, 2) + v[2] * m(2, 2);
        return Vector<float, 3>(x, y, z);
    }

    // ------------------------------------------------------------------
    // typedefs
    // ------------------------------------------------------------------

    using float2x2 = Matrix<float, 2, 2>;
    using float3x3 = Matrix<float, 3, 3>;
    using float4x4 = Matrix<float, 4, 4>;

} // namespace mango

#include "matrix_float4x4.hpp"
