/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/matrix.hpp>

namespace mango::math
{

    struct Quaternion;
    struct AngleAxis;
    struct EulerAngles;

    // ------------------------------------------------------------------
    // Matrix<float, 3, 3>
    // ------------------------------------------------------------------

    /*
    offsets: [memory layout]

        [0 1 2]
        [3 4 5]
        [6 7 8]

    indices: [operator (y,x)]

        [(0,0) (0,1) (0,2)]
        [(1,0) (1,1) (1,2)]
        [(2,0) (2,1) (2,2)]

    scaling: [sx, sy, sz]

        [sx -- --]
        [-- sy --]
        [-- -- sz]

    rotation: [axis vectors]

        [xx xy xz]  <- x-axis
        [yx yy yz]  <- y-axis
        [zx zy zz]  <- z-axis
    */

    template <>
    struct Matrix<float, 3, 3>
    {
        using ScalarType = float;
        using VectorType = Vector<float, 3>;

        VectorType m[3];

        explicit Matrix()
        {
        }

        explicit Matrix(float scale)
        {
            *this = scale;
        }

        explicit Matrix(const float* v)
        {
            *this = v;
        }

        Matrix(const Matrix& v)
        {
            m[0] = v.m[0];
            m[1] = v.m[1];
            m[2] = v.m[2];
        }

        explicit Matrix(const Vector<float, 3>& v0,
                        const Vector<float, 3>& v1,
                        const Vector<float, 3>& v2)
        {
            m[0] = v0;
            m[1] = v1;
            m[2] = v2;
        }

        explicit Matrix(float s0, float s1, float s2,
                        float s3, float s4, float s5,
                        float s6, float s7, float s8)
        {
            m[0] = Vector<float, 3>(s0, s1, s2);
            m[1] = Vector<float, 3>(s3, s4, s5);
            m[2] = Vector<float, 3>(s6, s7, s8);
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
            assert(y < 3);
            return m[y];
        }

        VectorType& operator [] (size_t y)
        {
            assert(y < 3);
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

        // functions

        Matrix(const Quaternion& rotation)
        {
            *this = rotation;
        }

        Matrix(const AngleAxis& rotation)
        {
            *this = rotation;
        }

        Matrix(const EulerAngles& rotation)
        {
            *this = rotation;
        }

        ~Matrix()
        {
        }

        const Matrix3x3& operator = (const Matrix3x3& v)
        {
            m[0] = v.m[0];
            m[1] = v.m[1];
            m[2] = v.m[2];
            return *this;
        }

        const Matrix3x3& operator = (float scale);
        const Matrix3x3& operator = (const float* ptr);
        const Matrix3x3& operator = (const Quaternion& rotation);
        const Matrix3x3& operator = (const AngleAxis& rotation);
        const Matrix3x3& operator = (const EulerAngles& rotation);

        float determinant2x2() const;
        float determinant3x3() const;

        static Matrix3x3 identity();
        static Matrix3x3 scale(float s);
        static Matrix3x3 scale(float x, float y, float z);
        static Matrix3x3 rotate(float angle, const float32x3& axis);
        static Matrix3x3 rotateX(float angle);
        static Matrix3x3 rotateY(float angle);
        static Matrix3x3 rotateZ(float angle);
        static Matrix3x3 rotateXYZ(float x, float y, float z);
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline
    Vector<float, 3> operator * (const Vector<float, 3>& v, const Matrix3x3& m)
    {
        float x = v[0] * m(0, 0) + v[1] * m(1, 0) + v[2] * m(2, 0);
        float y = v[0] * m(0, 1) + v[1] * m(1, 1) + v[2] * m(2, 1);
        float z = v[0] * m(0, 2) + v[1] * m(1, 2) + v[2] * m(2, 2);
        return Vector<float, 3>(x, y, z);
    }

    static inline
    Matrix3x3 operator * (const Matrix3x3& a, const Matrix3x3& b)
    {
        return Matrix3x3(
            a[0] * b,
            a[1] * b,
            a[2] * b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    Matrix3x3 scale(const Matrix3x3& matrix, float s);
    Matrix3x3 scale(const Matrix3x3& matrix, float x, float y, float z);
    Matrix3x3 rotate(const Matrix3x3& matrix, float angle, const float32x3& axis);
    Matrix3x3 rotateX(const Matrix3x3& matrix, float angle);
    Matrix3x3 rotateY(const Matrix3x3& matrix, float angle);
    Matrix3x3 rotateZ(const Matrix3x3& matrix, float angle);
    Matrix3x3 rotateXYZ(const Matrix3x3& matrix, float x, float y, float z);
    Matrix3x3 normalize(const Matrix3x3& matrix);

    static inline
    Matrix3x3 transpose(const Matrix3x3& m)
    {
        return Matrix3x3(
            m(0, 0), m(1, 0), m(2, 0),
            m(0, 1), m(1, 1), m(2, 1),
            m(0, 2), m(1, 2), m(2, 2));
    }

    static inline
    Matrix3x3 inverse(const Matrix3x3& m)
    {
        float s = m.determinant3x3();
        if (s)
        {
            s = 1.0f / s;
        }

        const float m00 = (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2)) * s;
        const float m01 = (m(2, 0) * m(1, 2) - m(1, 0) * m(2, 2)) * s;
        const float m02 = (m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1)) * s;
        const float m10 = (m(2, 1) * m(0, 2) - m(0, 1) * m(2, 2)) * s;
        const float m11 = (m(0, 0) * m(2, 2) - m(2, 0) * m(0, 2)) * s;
        const float m12 = (m(2, 0) * m(0, 1) - m(0, 0) * m(2, 1)) * s;
        const float m20 = (m(0, 1) * m(1, 2) - m(1, 1) * m(0, 2)) * s;
        const float m21 = (m(1, 0) * m(0, 2) - m(0, 0) * m(1, 2)) * s;
        const float m22 = (m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1)) * s;

        return Matrix3x3(
            m00, m01, m02,
            m10, m11, m12,
            m20, m21, m22);
    }

} // namespace mango::math
