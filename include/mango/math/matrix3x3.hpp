/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
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
    struct Matrix<float, 3, 3> : MatrixBase<float, 3, 3>
    {
        float32x3 m[3];

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

        explicit Matrix(
            const Vector<float, 3>& v0,
            const Vector<float, 3>& v1,
            const Vector<float, 3>& v2)
        {
            m[0] = v0;
            m[1] = v1;
            m[2] = v2;
        }

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

        operator float32x3* ()
        {
            return m;
        }

        operator const float32x3* () const
        {
            return m;
        }

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
        Matrix3x3 result;
        result[0] = a[0] * b;
        result[1] = a[1] * b;
        result[2] = a[2] * b;
        result[3] = a[3] * b;
        return result;
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    Matrix4x4 scale(const Matrix4x4& matrix, float s);
    Matrix4x4 scale(const Matrix4x4& matrix, float x, float y, float z);
    Matrix4x4 rotate(const Matrix4x4& matrix, float angle, const float32x3& axis);
    Matrix4x4 rotateX(const Matrix4x4& matrix, float angle);
    Matrix4x4 rotateY(const Matrix4x4& matrix, float angle);
    Matrix4x4 rotateZ(const Matrix4x4& matrix, float angle);
    Matrix4x4 rotateXYZ(const Matrix4x4& matrix, float x, float y, float z);
    Matrix4x4 normalize(const Matrix4x4& matrix);

} // namespace mango::math
