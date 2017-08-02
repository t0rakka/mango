/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "matrix.hpp"

namespace mango
{

    struct Quaternion;
    struct AngleAxis;

    // ------------------------------------------------------------------
    // Matrix<float, 4, 4>
    // ------------------------------------------------------------------

    /*
    offsets: (memory layout)

    [ 0  4  8 12]
    [ 1  5  9 13]
    [ 2  6 10 14]
    [ 3  7 11 15]

    indices: (operator (i,j))

    [(0,0) (1,0) (2,0) (3,0)]
    [(0,1) (1,1) (2,1) (3,1)]
    [(0,1) (1,2) (2,2) (3,2)]
    [(0,3) (1,3) (2,3) (3,3)]

    scaling: (sx, sy, sz)

    [sx -- -- --]
    [-- sy -- --]
    [-- -- sz --]
    [-- -- -- --]

    translation: (tx, ty, tz)

    [-- -- -- tx]
    [-- -- -- ty]
    [-- -- -- tz]
    [-- -- -- --]

    rotation: (axis vectors)

    [xx yx zx --]
    [xy yy zy --]
    [xz yz zz --]
    [-- -- -- --]
    */

    template <>
    struct Matrix<float, 4, 4> : MatrixBase<float, 4, 4>
    {
        float32x4 m[4];

        explicit Matrix()
        {
        }

        explicit Matrix(float s)
        {
            *this = s;
        }

        explicit Matrix(const float* v)
        {
            *this = v;
        }

        Matrix(const Matrix& other)
        {
            m[0] = other.m[0];
            m[1] = other.m[1];
            m[2] = other.m[2];
            m[3] = other.m[3];
        }
        
        explicit Matrix(
            const Vector<float, 4>& v0,
            const Vector<float, 4>& v1,
            const Vector<float, 4>& v2,
            const Vector<float, 4>& v3)
        {
            m[0] = v0;
            m[1] = v1;
            m[2] = v2;
            m[3] = v3;
        }

        Matrix(const AngleAxis& a)
        {
            *this = a;
        }

        ~Matrix()
        {
        }

        const Matrix& operator = (const Matrix& other)
        {
            m[0] = other.m[0];
            m[1] = other.m[1];
            m[2] = other.m[2];
            m[3] = other.m[3];
            return *this;
        }

        const Matrix& operator = (float s);
        const Matrix& operator = (const float* v);
        const Matrix& operator = (const Quaternion& q);
        const Matrix& operator = (const AngleAxis& a);

        operator float32x4* ()
        {
            return m;
        }

        operator const float32x4* () const
        {
            return m;
        }

        bool isAffine() const;
        float determinant() const;

        // set identity
        void identity();

        // modify current matrix
        void translate(float xtrans, float ytrans, float ztrans);
        void translate(const float3& trans);
        void scale(float scale);
        void scale(float xscale, float yscale, float zscale);
        void scale(const float3& scale);
        void rotate(float angle, const float3& axis);
        void rotateX(float angle);
        void rotateY(float angle);
        void rotateZ(float angle);
        void rotateXYZ(float xangle, float yangle, float zangle);
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline Vector<float, 3> operator * (const Vector<float, 3>& v, const Matrix<float, 4, 4>& m)
    {
        float x = v[0] * m(0, 0) + v[1] * m(1, 0) + v[2] * m(2, 0) + m(3, 0);
        float y = v[0] * m(0, 1) + v[1] * m(1, 1) + v[2] * m(2, 1) + m(3, 1);
        float z = v[0] * m(0, 2) + v[1] * m(1, 2) + v[2] * m(2, 2) + m(3, 2);
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 4> operator * (const Vector<float, 4>& v, const Matrix<float, 4, 4>& m)
    {
        float32x4 temp = m[0] * v.xxxx;
        temp = madd(temp, m[1], v.yyyy);
        temp = madd(temp, m[2], v.zzzz);
        temp = madd(temp, m[3], v.wwww);
        return temp;
    }

    static inline Matrix<float, 4, 4> operator * (const Matrix<float, 4, 4>& a, const Matrix<float, 4, 4>& b)
    {
        Matrix<float, 4, 4> result;
        result[0] = a[0] * b;
        result[1] = a[1] * b;
        result[2] = a[2] * b;
        result[3] = a[3] * b;
        return result;
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    float4x4 inverse(const float4x4& m);
    float4x4 inverseTranspose(const float4x4& m);
    float4x4 transpose(const float4x4& m);
    float4x4 normalize(const float4x4& m);
    float4x4 mirror(const float4x4& m, const float4& plane);
    float4x4 affineInverse(const float4x4& m);
    float4x4 adjoint(const float4x4& m);

    namespace matrix {
        float4x4 translate(float xtrans, float ytrans, float ztrans);
        float4x4 translate(const float3& trans);
        float4x4 scale(float scale);
        float4x4 scale(float xscale, float yscale, float zscale);
        float4x4 scale(const float3& scale);
        float4x4 rotate(float angle, const float3& axis);
        float4x4 rotateX(float angle);
        float4x4 rotateY(float angle);
        float4x4 rotateZ(float angle);
        float4x4 rotateXYZ(float xangle, float yangle, float zangle);
        float4x4 lookat(const float3& target, const float3& viewer, const float3& up);
    } // namespace matrix

    namespace opengl {
        float4x4 ortho(float left, float right, float bottom, float top, float znear, float zfar);
        float4x4 frustum(float left, float right, float bottom, float top, float znear, float zfar);
        float4x4 perspective(float xfov, float yfov, float znear, float zfar);
        float4x4 oblique(const float4x4& proj, const float4& nearclip);
    } // namespace opengl

    namespace vulkan {
        float4x4 ortho(float left, float right, float bottom, float top, float znear, float zfar);
        float4x4 frustum(float left, float right, float bottom, float top, float znear, float zfar);
        float4x4 perspective(float xfov, float yfov, float znear, float zfar);
        float4x4 oblique(const float4x4& proj, const float4& nearclip);
    } // namespace vulkan

    namespace directx {
        // left-handed
        float4x4 ortho(float left, float right, float bottom, float top, float znear, float zfar);
        float4x4 frustum(float left, float right, float bottom, float top, float znear, float zfar);
        float4x4 perspective(float xfov, float yfov, float znear, float zfar);
        float4x4 oblique(const float4x4& proj, const float4& nearclip);
    } // namespace directx

} // namespace mango
