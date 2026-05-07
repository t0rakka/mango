/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstddef>
#include <cassert>
#include <mango/math/vector.hpp>

namespace mango::math
{

    // ------------------------------------------------------------------
    // Matrix
    // ------------------------------------------------------------------

    template <typename ScalarType, size_t Width, size_t Height>
    struct Matrix
    {
        using VectorType = Vector<ScalarType, Width>;

        VectorType m[Height];

        explicit Matrix()
        {
        }

        ~Matrix()
        {
        }

        // accessors

        operator const ScalarType* () const
        {
            return m[0].data();
        }

        operator ScalarType* ()
        {
            return m[0].data();
        }

        operator VectorType* ()
        {
            return m;
        }

        operator const VectorType* () const
        {
            return m;
        }

        const ScalarType* data() const
        {
            return m[0].data();
        }

        ScalarType* data()
        {
            return m[0].data();
        }

        const VectorType& operator [] (size_t y) const
        {
            assert(y < Height);
            return m[y];
        }

        VectorType& operator [] (size_t y)
        {
            assert(y < Height);
            return m[y];
        }

        ScalarType operator () (size_t y, size_t x) const
        {
            assert(x < Width);
            assert(y < Height);
            return m[y][x];
        }

        ScalarType& operator () (size_t y, size_t x)
        {
            assert(x < Width);
            assert(y < Height);
            return m[y][x];
        }
    };

    // ------------------------------------------------------------------
    // 2x2
    // ------------------------------------------------------------------

    template <typename ScalarType>
    struct Matrix<ScalarType, 2, 2>
    {
        using VectorType = Vector<ScalarType, 2>;

        VectorType m[2];

        explicit Matrix()
        {
        }

        explicit Matrix(VectorType v0, VectorType v1)
            : m { v0, v1 }
        {
        }

        explicit Matrix(ScalarType s)
            : m { VectorType(s, 0), VectorType(0, s) }
        {
        }

        explicit Matrix(ScalarType s0, ScalarType s1, ScalarType s2, ScalarType s3)
            : m { VectorType(s0, s1), VectorType(s2, s3) }
        {
        }

        ~Matrix()
        {
        }

        // accessors

        operator const ScalarType* () const
        {
            return m[0].data();
        }

        operator ScalarType* ()
        {
            return m[0].data();
        }

        operator VectorType* ()
        {
            return m;
        }

        operator const VectorType* () const
        {
            return m;
        }

        const ScalarType* data() const
        {
            return m[0].data();
        }

        ScalarType* data()
        {
            return m[0].data();
        }

        const VectorType& operator [] (size_t y) const
        {
            assert(y < 2);
            return m[y];
        }

        VectorType& operator [] (size_t y)
        {
            assert(y < 2);
            return m[y];
        }

        ScalarType operator () (size_t y, size_t x) const
        {
            assert(x < 2);
            assert(y < 2);
            return m[y][x];
        }

        ScalarType& operator () (size_t y, size_t x)
        {
            assert(x < 2);
            assert(y < 2);
            return m[y][x];
        }
    };

    template <typename ScalarType>
    static inline
    Matrix<ScalarType, 2, 2> operator * (const Matrix<ScalarType, 2, 2>& m, ScalarType s)
    {
        return Matrix<ScalarType, 2, 2>(
            m[0] * s,
            m[1] * s);
    }

    template <typename ScalarType>
    static inline
    Vector<ScalarType, 2> operator * (const Vector<ScalarType, 2>& v, const Matrix<ScalarType, 2, 2>& m)
    {
        return Vector<ScalarType, 2>(
            v[0] * m(0, 0) + v[1] * m(1, 0),
            v[0] * m(0, 1) + v[1] * m(1, 1));
    }

    // ------------------------------------------------------------------
    // 3x3
    // ------------------------------------------------------------------

    template <typename ScalarType>
    struct Matrix<ScalarType, 3, 3>
    {
        using VectorType = Vector<ScalarType, 3>;

        VectorType m[3];

        explicit Matrix()
        {
        }

        explicit Matrix(VectorType v0, VectorType v1, VectorType v2)
            : m { v0, v1, v2 }
        {
        }

        explicit Matrix(ScalarType s)
            : m { VectorType(s, 0, 0), VectorType(0, s, 0), VectorType(0, 0, s) }
        {
        }

        explicit Matrix(ScalarType s0, ScalarType s1, ScalarType s2,
                        ScalarType s3, ScalarType s4, ScalarType s5,
                        ScalarType s6, ScalarType s7, ScalarType s8)
            : m { VectorType(s0, s1, s2), VectorType(s3, s4, s5), VectorType(s6, s7, s8) }
        {
        }

        ~Matrix()
        {
        }

        // accessors

        operator const ScalarType* () const
        {
            return m[0].data();
        }

        operator ScalarType* ()
        {
            return m[0].data();
        }

        operator VectorType* ()
        {
            return m;
        }

        operator const VectorType* () const
        {
            return m;
        }

        const ScalarType* data() const
        {
            return m[0].data();
        }

        ScalarType* data()
        {
            return m[0].data();
        }

        const VectorType& operator [] (size_t y) const
        {
            assert(y < Height);
            return m[y];
        }

        VectorType& operator [] (size_t y)
        {
            assert(y < Height);
            return m[y];
        }

        ScalarType operator () (size_t y, size_t x) const
        {
            assert(x < 3);
            assert(y < 3);
            return m[y][x];
        }

        ScalarType& operator () (size_t y, size_t x)
        {
            assert(x < 3);
            assert(y < 3);
            return m[y][x];
        }
    };

    template <typename ScalarType>
    static inline
    Matrix<ScalarType, 3, 3> operator * (const Matrix<ScalarType, 3, 3>& m, ScalarType s)
    {
        return Matrix<ScalarType, 3, 3>(
            m[0] * s,
            m[1] * s,
            m[2] * s);
    }

    template <typename ScalarType>
    static inline
    Vector<ScalarType, 3> operator * (const Vector<ScalarType, 3>& v, const Matrix<ScalarType, 3, 3>& m)
    {
        return Vector<ScalarType, 3>(
            v[0] * m(0, 0) + v[1] * m(1, 0) + v[2] * m(2, 0),
            v[0] * m(0, 1) + v[1] * m(1, 1) + v[2] * m(2, 1),
            v[0] * m(0, 2) + v[1] * m(1, 2) + v[2] * m(2, 2));
    }

    // ------------------------------------------------------------------
    // types
    // ------------------------------------------------------------------

    using Matrix2x2 = Matrix<float, 2, 2>;
    using Matrix3x3 = Matrix<float, 3, 3>;
    using Matrix4x4 = Matrix<float, 4, 4>;

} // namespace mango::math

#include <mango/math/matrix3x3.hpp>
#include <mango/math/matrix4x4.hpp>
