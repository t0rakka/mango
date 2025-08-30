/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#include <limits>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <mango/core/bits.hpp>
#include <mango/math/math.hpp>

namespace mango::math
{

    static constexpr float epsilon = std::numeric_limits<float>::epsilon();

    // ------------------------------------------------------------------------
    // Matrix3x3
    // ------------------------------------------------------------------------

    const Matrix3x3& Matrix3x3::operator = (float s)
    {
        m[0] = float32x3(s, 0, 0);
        m[1] = float32x3(0, s, 0);
        m[2] = float32x3(0, 0, s);
        return *this;
    }

    const Matrix3x3& Matrix3x3::operator = (const float* ptr)
    {
        m[0][0] = ptr[0];
        m[0][1] = ptr[1];
        m[0][2] = ptr[2];
        m[1][0] = ptr[3];
        m[1][1] = ptr[4];
        m[1][2] = ptr[5];
        m[2][0] = ptr[6];
        m[2][1] = ptr[7];
        m[2][2] = ptr[8];
        return *this;
    }

    const Matrix3x3& Matrix3x3::operator = (const Quaternion& q)
    {
        const float x2 = q.x * 2.0f;
        const float y2 = q.y * 2.0f;
        const float z2 = q.z * 2.0f;

        const float wx = q.w * x2;
        const float wy = q.w * y2;
        const float wz = q.w * z2;

        const float xx = q.x * x2;
        const float xy = q.x * y2;
        const float xz = q.x * z2;

        const float yy = q.y * y2;
        const float yz = q.y * z2;
        const float zz = q.z * z2;

        m[0] = float32x3(1.0f - yy - zz, xy + wz, xz - wy);
        m[1] = float32x3(xy - wz, 1.0f - xx - zz, yz + wx);
        m[2] = float32x3(xz + wy, yz - wx, 1.0f - xx - yy);

        return *this;
    }

    const Matrix3x3& Matrix3x3::operator = (const AngleAxis& a)
    {
        float length2 = square(a.axis);
        if (length2 < epsilon)
        {
            *this = 1.0f; // set identity
            return *this;
        }

        float s = std::sin(a.angle);
        float c = std::cos(a.angle);
        float k = 1.0f - c;

        float length = 1.0f / std::sqrt(length2);

        float x = a.axis.x * length;
        float y = a.axis.y * length;
        float z = a.axis.z * length;

        float xk = x * k;
        float yk = y * k;
        float zk = z * k;

        float xy = x * yk;
        float yz = y * zk;
        float zx = z * xk;
        float xs = x * s;
        float ys = y * s;
        float zs = z * s;

        m[0] = float32x3(x * xk + c, xy + zs, zx - ys);
        m[1] = float32x3(xy - zs, y * yk + c, yz + xs);
        m[2] = float32x3(zx + ys, yz - xs, z * zk + c);

        return *this;
    }

    const Matrix3x3& Matrix3x3::operator = (const EulerAngles& euler)
    {
        // use vectorized sin / cos
        const float32x4 v = float32x4(euler.x, euler.y, euler.z, 0.0f);
        const float32x4 s = sin(v);
        const float32x4 c = cos(v);

        const float sx = s.x;
        const float sy = s.y;
        const float sz = s.z;
        const float cx = c.x;
        const float cy = c.y;
        const float cz = c.z;
        const float sysx = sy * sx;
        const float sycx = sy * cx;

        m[0] = float32x3(cz * cy, sz * cy, -sy);
        m[1] = float32x3(cz * sysx - sz * cx, sz * sysx + cz * cx, cy * sx);
        m[2] = float32x3(cz * sycx + sz * sx, sz * sycx - cz * sx, cy * cx);

        return *this;
    }

    float Matrix3x3::determinant2x2() const
    {
        return m[0][0] * m[1][1] - m[1][0] * m[0][1];
    }

    float Matrix3x3::determinant3x3() const
    {
        return m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
               m[0][1] * (m[1][0] * m[2][2] - m[2][0] * m[1][2]) +
               m[0][2] * (m[1][0] * m[2][1] - m[2][0] * m[1][1]);
    }

    Matrix3x3 Matrix3x3::identity()
    {
        return Matrix3x3(1.0f);
    }

    Matrix3x3 Matrix3x3::scale(float s)
    {
        Matrix3x3 m;
        m[0] = float32x3(s, 0, 0);
        m[1] = float32x3(0, s, 0);
        m[2] = float32x3(0, 0, s);
        return m;
    }

    Matrix3x3 Matrix3x3::scale(float x, float y, float z)
    {
        Matrix3x3 m;
        m[0] = float32x3(x, 0, 0);
        m[1] = float32x3(0, y, 0);
        m[2] = float32x3(0, 0, z);
        return m;
    }

    Matrix3x3 Matrix3x3::rotate(float angle, const float32x3& axis)
    {
        const Matrix3x3 m = AngleAxis(angle, axis);
        return m;
    }

    Matrix3x3 Matrix3x3::rotateX(float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        Matrix3x3 m;
        m[0] = float32x3(1, 0, 0);
        m[1] = float32x3(0, c, s);
        m[2] = float32x3(0,-s, c);
        return m;
    }

    Matrix3x3 Matrix3x3::rotateY(float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        Matrix3x3 m;
        m[0] = float32x3(c, 0,-s);
        m[1] = float32x3(0, 1, 0);
        m[2] = float32x3(s, 0, c);
        return m;
     }

    Matrix3x3 Matrix3x3::rotateZ(float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        Matrix3x3 m;
        m[0] = float32x3( c, s, 0);
        m[1] = float32x3(-s, c, 0);
        m[2] = float32x3( 0, 0, 1);
        return m;
    }

    Matrix3x3 Matrix3x3::rotateXYZ(float x, float y, float z)
    {
        return Matrix3x3(EulerAngles(x, y, z));
    }

    Matrix3x3 scale(const Matrix3x3& input, float s)
    {
        const float32x3 v = float32x3(s, s, s);

        Matrix3x3 m;
        m[0] = input[0] * v;
        m[1] = input[1] * v;
        m[2] = input[2] * v;
        return m;
    }

    Matrix3x3 scale(const Matrix3x3& input, float x, float y, float z)
    {
        const float32x3 v = float32x3(x, y, z);

        Matrix3x3 m;
        m[0] = input[0] * v;
        m[1] = input[1] * v;
        m[2] = input[2] * v;
        return m;
    }

    Matrix3x3 rotate(const Matrix3x3& input, float angle, const float32x3& axis)
    {
        const Matrix3x3 temp = AngleAxis(angle, axis);
        return input * temp;
    }

    Matrix3x3 rotateX(const Matrix3x3& input, float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        Matrix3x3 m;

        for (int i = 0; i < 4; ++i)
        {
            const float x = input[i][0];
            const float y = input[i][1];
            const float z = input[i][2];
            m[i][0] = x;
            m[i][1] = y * c - z * s;
            m[i][2] = z * c + y * s;
        }

        return m;
    }

    Matrix3x3 rotateY(const Matrix3x3& input, float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        Matrix3x3 m;

        for (int i = 0; i < 4; ++i)
        {
            const float x = input[i][0];
            const float y = input[i][1];
            const float z = input[i][2];
            m[i][0] = x * c + z * s;
            m[i][1] = y;
            m[i][2] = z * c - x * s;
        }

        return m;
    }

    Matrix3x3 rotateZ(const Matrix3x3& input, float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        Matrix3x3 m;

        for (int i = 0; i < 4; ++i)
        {
            const float x = input[i][0];
            const float y = input[i][1];
            const float z = input[i][2];
            m[i][0] = x * c - y * s;
            m[i][1] = y * c + x * s;
            m[i][2] = z;
        }

        return m;
    }

    Matrix3x3 rotateXYZ(const Matrix3x3& input, float x, float y, float z)
    {
        const Matrix3x3 temp = Matrix3x3::rotateXYZ(x, y, z);
        return input * temp;
    }

    Matrix3x3 normalize(const Matrix3x3& input)
    {
        float32x3 x = input[0];
        float32x3 y = input[1];
        float32x3 z = input[2];
        x = normalize(x);
        y = normalize(y - x * dot(x, y));
        z = cross(x, y);
        return Matrix3x3(x, y, z);
    }

    // ------------------------------------------------------------------------
    // Matrix4x4
    // ------------------------------------------------------------------------

    const Matrix4x4& Matrix4x4::operator = (float s)
    {
        m[0] = float32x4(s, 0, 0, 0);
        m[1] = float32x4(0, s, 0, 0);
        m[2] = float32x4(0, 0, s, 0);
        m[3] = float32x4(0, 0, 0, 1);
        return *this;
    }

    const Matrix4x4& Matrix4x4::operator = (const float* ptr)
    {
        m[0] = simd::f32x4_uload(ptr + 0);
        m[1] = simd::f32x4_uload(ptr + 4);
        m[2] = simd::f32x4_uload(ptr + 8);
        m[3] = simd::f32x4_uload(ptr + 12);
        return *this;
    }

    const Matrix4x4& Matrix4x4::operator = (const Quaternion& q)
    {
        const float x2 = q.x * 2.0f;
        const float y2 = q.y * 2.0f;
        const float z2 = q.z * 2.0f;

        const float wx = q.w * x2;
        const float wy = q.w * y2;
        const float wz = q.w * z2;

        const float xx = q.x * x2;
        const float xy = q.x * y2;
        const float xz = q.x * z2;

        const float yy = q.y * y2;
        const float yz = q.y * z2;
        const float zz = q.z * z2;

        m[0] = float32x4(1.0f - yy - zz, xy + wz, xz - wy, 0.0f);
        m[1] = float32x4(xy - wz, 1.0f - xx - zz, yz + wx, 0.0f);
        m[2] = float32x4(xz + wy, yz - wx, 1.0f - xx - yy, 0.0f);
        m[3] = float32x4(0.0f, 0.0f, 0.0f, 1.0f);

        return *this;
    }

    const Matrix4x4& Matrix4x4::operator = (const AngleAxis& a)
    {
        float length2 = square(a.axis);
        if (length2 < epsilon)
        {
            *this = 1.0f; // set identity
            return *this;
        }

        float s = std::sin(a.angle);
        float c = std::cos(a.angle);
        float k = 1.0f - c;

        float length = 1.0f / std::sqrt(length2);

        float x = a.axis.x * length;
        float y = a.axis.y * length;
        float z = a.axis.z * length;

        float xk = x * k;
        float yk = y * k;
        float zk = z * k;

        float xy = x * yk;
        float yz = y * zk;
        float zx = z * xk;
        float xs = x * s;
        float ys = y * s;
        float zs = z * s;

        m[0] = float32x4(x * xk + c, xy + zs, zx - ys, 0.0f);
        m[1] = float32x4(xy - zs, y * yk + c, yz + xs, 0.0f);
        m[2] = float32x4(zx + ys, yz - xs, z * zk + c, 0.0f);
        m[3] = float32x4(0.0f, 0.0f, 0.0f, 1.0f);

        return *this;
    }

    const Matrix4x4& Matrix4x4::operator = (const EulerAngles& euler)
    {
        const float32x4 v = float32x4(euler.x, euler.y, euler.z, 0.0f);
        const float32x4 s = sin(v);
        const float32x4 c = cos(v);

        const float sx = s.x;
        const float sy = s.y;
        const float sz = s.z;
        const float cx = c.x;
        const float cy = c.y;
        const float cz = c.z;
        const float sysx = sy * sx;
        const float sycx = sy * cx;

        m[0] = float32x4(cz * cy, sz * cy, -sy, 0);
        m[1] = float32x4(cz * sysx - sz * cx, sz * sysx + cz * cx, cy * sx, 0);
        m[2] = float32x4(cz * sycx + sz * sx, sz * sycx - cz * sx, cy * cx, 0);
        m[3] = float32x4(0, 0, 0, 1);

        return *this;
    }

    bool Matrix4x4::isAffine() const
    {
        float32x4 c = column<3>();
        return all_of(c == float32x4(0, 0, 0, 1));
    }

    float Matrix4x4::determinant2x2() const
    {
        return m[0][0] * m[1][1] - m[1][0] * m[0][1];
    }

    float Matrix4x4::determinant3x3() const
    {
        return m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
               m[0][1] * (m[1][0] * m[2][2] - m[2][0] * m[1][2]) +
               m[0][2] * (m[1][0] * m[2][1] - m[2][0] * m[1][1]);
    }

    float Matrix4x4::determinant4x4() const
    {
        float s0 = m[1][1] * (m[2][2] * m[3][3] - m[3][2] * m[2][3]) -
                   m[1][2] * (m[2][1] * m[3][3] - m[3][1] * m[2][3]) +
                   m[1][3] * (m[2][1] * m[3][2] - m[3][1] * m[2][2]);
        float s1 = m[1][0] * (m[2][2] * m[3][3] - m[3][2] * m[2][3]) -
                   m[1][2] * (m[2][0] * m[3][3] - m[3][0] * m[2][3]) +
                   m[1][3] * (m[2][0] * m[3][2] - m[3][0] * m[2][2]);
        float s2 = m[1][0] * (m[2][1] * m[3][3] - m[3][1] * m[2][3]) -
                   m[1][1] * (m[2][0] * m[3][3] - m[3][0] * m[2][3]) +
                   m[1][3] * (m[2][0] * m[3][1] - m[3][0] * m[2][1]);
        float s3 = m[1][0] * (m[2][1] * m[3][2] - m[3][1] * m[2][2]) -
                   m[1][1] * (m[2][0] * m[3][2] - m[3][0] * m[2][2]) +
                   m[1][1] * (m[2][0] * m[3][1] - m[3][0] * m[2][1]);
        return m[0][0] * s0 - m[0][1] * s1 + m[0][2] * s2 - m[0][3] * s3;
    }

    Matrix4x4 Matrix4x4::identity()
    {
        return Matrix4x4(1.0f);
    }

    Matrix4x4 Matrix4x4::translate(float x, float y, float z)
    {
        Matrix4x4 m;
        m[0] = float32x4(1, 0, 0, 0);
        m[1] = float32x4(0, 1, 0, 0);
        m[2] = float32x4(0, 0, 1, 0);
        m[3] = float32x4(x, y, z, 1);
        return m;
    }

    Matrix4x4 Matrix4x4::scale(float s)
    {
        Matrix4x4 m;
        m[0] = float32x4(s, 0, 0, 0);
        m[1] = float32x4(0, s, 0, 0);
        m[2] = float32x4(0, 0, s, 0);
        m[3] = float32x4(0, 0, 0, 1);
        return m;
    }

    Matrix4x4 Matrix4x4::scale(float x, float y, float z)
    {
        Matrix4x4 m;
        m[0] = float32x4(x, 0, 0, 0);
        m[1] = float32x4(0, y, 0, 0);
        m[2] = float32x4(0, 0, z, 0);
        m[3] = float32x4(0, 0, 0, 1);
        return m;
    }

    Matrix4x4 Matrix4x4::rotate(float angle, const float32x3& axis)
    {
        const Matrix4x4 m = AngleAxis(angle, axis);
        return m;
    }

    Matrix4x4 Matrix4x4::rotateX(float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        Matrix4x4 m;
        m[0] = float32x4(1, 0, 0, 0);
        m[1] = float32x4(0, c, s, 0);
        m[2] = float32x4(0,-s, c, 0);
        m[3] = float32x4(0, 0, 0, 1);
        return m;
    }

    Matrix4x4 Matrix4x4::rotateY(float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        Matrix4x4 m;
        m[0] = float32x4(c, 0,-s, 0);
        m[1] = float32x4(0, 1, 0, 0);
        m[2] = float32x4(s, 0, c, 0);
        m[3] = float32x4(0, 0, 0, 1);
        return m;
     }

    Matrix4x4 Matrix4x4::rotateZ(float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        Matrix4x4 m;
        m[0] = float32x4( c, s, 0, 0);
        m[1] = float32x4(-s, c, 0, 0);
        m[2] = float32x4( 0, 0, 1, 0);
        m[3] = float32x4( 0, 0, 0, 1);
        return m;
    }

    Matrix4x4 Matrix4x4::rotateXYZ(float x, float y, float z)
    {
        return Matrix4x4(EulerAngles(x, y, z));
    }

    Matrix4x4 Matrix4x4::lookat(const float32x3& target, const float32x3& viewer, const float32x3& up)
    {
        const float32x3 zaxis = normalize(target - viewer);
        const float32x3 xaxis = normalize(cross(up, zaxis));
        const float32x3 yaxis = cross(zaxis, xaxis);

        Matrix4x4 m;
        m[0] = float32x4(xaxis.x, yaxis.x, zaxis.x, 0);
        m[1] = float32x4(xaxis.y, yaxis.y, zaxis.y, 0);
        m[2] = float32x4(xaxis.z, yaxis.z, zaxis.z, 0);
        m[3] = float32x4(-dot(xaxis, viewer), -dot(yaxis, viewer), -dot(zaxis, viewer), 1.0f);
        return m;
    }

    Matrix4x4 Matrix4x4::orthoGL(float left, float right, float bottom, float top, float znear, float zfar)
    {
        float x = 2.0f / (right - left);
        float y = 2.0f / (top - bottom);
        float z = -2.0f / (zfar - znear);
        float a = -(left + right) / (right - left);
        float b = -(bottom + top) / (top - bottom);
        float c = -(znear + zfar) / (zfar - znear);

        Matrix4x4 m;
        m[0] = float32x4(x, 0, 0, 0);
        m[1] = float32x4(0, y, 0, 0);
        m[2] = float32x4(0, 0, z, 0);
        m[3] = float32x4(a, b, c, 1);
        return m;
    }

    Matrix4x4 Matrix4x4::frustumGL(float left, float right, float bottom, float top, float znear, float zfar)
    {
        float a = (right + left) / (right - left);
        float b = (top + bottom) / (top - bottom);
        float c = -(zfar + znear) / (zfar - znear);
        float d = -(2.0f * znear * zfar) / (zfar - znear);
        float x = (2.0f * znear) / (right - left);
        float y = (2.0f * znear) / (top - bottom);
        float z = -1.0f;

        Matrix4x4 m;
        m[0] = float32x4(x, 0, 0, 0);
        m[1] = float32x4(0, y, 0, 0);
        m[2] = float32x4(a, b, c, z);
        m[3] = float32x4(0, 0, d, 0);
        return m;
    }

    Matrix4x4 Matrix4x4::perspectiveGL(float xfov, float yfov, float znear, float zfar)
    {
        float x = znear * std::tan(xfov * 0.5f);
        float y = znear * std::tan(yfov * 0.5f);
        return frustumGL(-x, x, -y, y, znear, zfar);
    }

    Matrix4x4 Matrix4x4::orthoVK(float left, float right, float bottom, float top, float znear, float zfar)
    {
        float x = 2.0f / (right - left);
        float y = 2.0f / (bottom - top);
        float z = 1.0f / (znear - zfar);
        float a = (right + left) / (left - right);
        float b = (top + bottom) / (top - bottom);
        float c = (zfar + znear) / (zfar - znear) * -0.5f;

        Matrix4x4 m;
        m[0] = float32x4(x, 0, 0, 0);
        m[1] = float32x4(0, y, 0, 0);
        m[2] = float32x4(0, 0, z, z);
        m[3] = float32x4(a, b, c, c + 1.0f);
        return m;
    }

    Matrix4x4 Matrix4x4::frustumVK(float left, float right, float bottom, float top, float znear, float zfar)
    {
        float a = (right + left) / (right - left);
        float b = (top + bottom) / (bottom - top);
        float c = (zfar + znear) / (znear - zfar) * 0.5f;
        float d = (2.0f * znear * zfar) / (znear - zfar) * 0.5f;
        float x = (2.0f * znear) / (right - left);
        float y = (2.0f * znear) / (bottom - top);

        Matrix4x4 m;
        m[0] = float32x4(x, 0, 0, 0);
        m[1] = float32x4(0, y, 0, 0);
        m[2] = float32x4(a, b, c, c - 1.0f);
        m[3] = float32x4(0, 0, d, d);
        return m;
    }

    Matrix4x4 Matrix4x4::perspectiveVK(float xfov, float yfov, float znear, float zfar)
    {
        float x = znear * std::tan(xfov * 0.5f);
        float y = znear * std::tan(yfov * 0.5f);
        return frustumVK(-x, x, -y, y, znear, zfar);
    }

    Matrix4x4 Matrix4x4::orthoD3D(float left, float right, float bottom, float top, float znear, float zfar)
    {
        const float x = 1.0f / (right - left);
        const float y = 1.0f / (top - bottom);
        const float z = 1.0f / (zfar - znear);

        const float w = 2.0f * x;
        const float h = 2.0f * y;
        const float a = -x * (left + right);
        const float b = -y * (bottom + top);
        const float c = -z * znear;

        Matrix4x4 m;
        m[0] = float32x4(w, 0, 0, 0);
        m[1] = float32x4(0, h, 0, 0);
        m[2] = float32x4(0, 0, z, 0);
        m[3] = float32x4(a, b, c, 1);
        return m;
    }

    Matrix4x4 Matrix4x4::frustumD3D(float left, float right, float bottom, float top, float znear, float zfar)
    {
        const float x = 1.0f / (right - left);
        const float y = 1.0f / (top - bottom);
        const float z = 1.0f / (zfar - znear);

        const float w = x * znear * 2.0f;
        const float h = y * znear * 2.0f;
        const float a = -x * (left + right);
        const float b = -y * (bottom + top);
        const float c = z * zfar;
        const float d = z * zfar * -znear;

        Matrix4x4 m;
        m[0] = float32x4(w, 0, 0, 0);
        m[1] = float32x4(0, h, 0, 0);
        m[2] = float32x4(a, b, c, 1);
        m[3] = float32x4(0, 0, d, 0);
        return m;
    }

    Matrix4x4 Matrix4x4::perspectiveD3D(float xfov, float yfov, float znear, float zfar)
    {
        const float w = 1.0f / std::tan(xfov * 0.5f);
        const float h = 1.0f / std::tan(yfov * 0.5f);
        const float a = zfar / (zfar - znear);
        const float b = -a * znear;

        Matrix4x4 m;
        m[0] = float32x4(w, 0, 0, 0);
        m[1] = float32x4(0, h, 0, 0);
        m[2] = float32x4(0, 0, a, 1);
        m[3] = float32x4(0, 0, b, 0);
        return m;
    }

    Matrix4x4 translate(const Matrix4x4& input, float x, float y, float z)
    {
        const float32x4 v = float32x4(x, y, z, 0.0f);

        Matrix4x4 m;
        m[0] = madd(input[0], input[0].wwww, v);
        m[1] = madd(input[1], input[1].wwww, v);
        m[2] = madd(input[2], input[2].wwww, v);
        m[3] = madd(input[3], input[3].wwww, v);
        return m;
    }

    Matrix4x4 scale(const Matrix4x4& input, float s)
    {
        const float32x4 v = float32x4(s, s, s, 1.0f);

        Matrix4x4 m;
        m[0] = input[0] * v;
        m[1] = input[1] * v;
        m[2] = input[2] * v;
        m[3] = input[3] * v;
        return m;
    }

    Matrix4x4 scale(const Matrix4x4& input, float x, float y, float z)
    {
        const float32x4 v = float32x4(x, y, z, 1.0f);

        Matrix4x4 m;
        m[0] = input[0] * v;
        m[1] = input[1] * v;
        m[2] = input[2] * v;
        m[3] = input[3] * v;
        return m;
    }

    Matrix4x4 rotate(const Matrix4x4& input, float angle, const float32x3& axis)
    {
        const Matrix4x4 temp = AngleAxis(angle, axis);
        return input * temp;
    }

    Matrix4x4 rotateX(const Matrix4x4& input, float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        Matrix4x4 m;

        for (int i = 0; i < 4; ++i)
        {
            const float x = input[i][0];
            const float y = input[i][1];
            const float z = input[i][2];
            const float w = input[i][3];
            m[i][0] = x;
            m[i][1] = y * c - z * s;
            m[i][2] = z * c + y * s;
            m[i][3] = w;
        }

        return m;
    }

    Matrix4x4 rotateY(const Matrix4x4& input, float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        Matrix4x4 m;

        for (int i = 0; i < 4; ++i)
        {
            const float x = input[i][0];
            const float y = input[i][1];
            const float z = input[i][2];
            const float w = input[i][3];
            m[i][0] = x * c + z * s;
            m[i][1] = y;
            m[i][2] = z * c - x * s;
            m[i][3] = w;
        }

        return m;
    }

    Matrix4x4 rotateZ(const Matrix4x4& input, float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        Matrix4x4 m;

        for (int i = 0; i < 4; ++i)
        {
            const float x = input[i][0];
            const float y = input[i][1];
            const float z = input[i][2];
            const float w = input[i][3];
            m[i][0] = x * c - y * s;
            m[i][1] = y * c + x * s;
            m[i][2] = z;
            m[i][3] = w;
        }

        return m;
    }

    Matrix4x4 rotateXYZ(const Matrix4x4& input, float x, float y, float z)
    {
        const Matrix4x4 temp = Matrix4x4::rotateXYZ(x, y, z);
        return input * temp;
    }

    Matrix4x4 normalize(const Matrix4x4& input)
    {
        float32x4 x = input[0];
        float32x4 y = input[1];
        float32x4 z = input[2];
        x = normalize(x);
        y = normalize(y - x * dot(x, y));
        z = cross(x, y);
        return Matrix4x4(x, y, z, input[3]);
    }

    Matrix4x4 mirror(const Matrix4x4& input, const float32x4& plane)
    {
        const float* m = input;

        // components
        float32x3 xaxis(m[0], m[1], m[2]);
        float32x3 yaxis(m[4], m[5], m[6]);
        float32x3 zaxis(m[8], m[9], m[10]);
        float32x3 trans(m[12], m[13], m[14]);

        float32x3 normal = plane.xyz;
        float32x3 normal2 = normal * -2.0f;
        float dist = plane.w;

        // mirror translation
        float32x3 pos = trans + normal2 * (dot(trans, normal) - dist);

        // mirror x rotation
        xaxis += trans;
        xaxis += normal2 * (dot(xaxis, normal) - dist);
        xaxis -= pos;

        // mirror y rotation
        yaxis += trans;
        yaxis += normal2 * (dot(yaxis, normal) - dist);
        yaxis -= pos;

        // mirror z rotation
        zaxis += trans;
        zaxis += normal2 * (dot(zaxis, normal) - dist);
        zaxis -= pos;

        Matrix4x4 result;
        result[0] = float32x4(xaxis.x, xaxis.y, xaxis.z, 0.0f);
        result[1] = float32x4(yaxis.x, yaxis.y, yaxis.z, 0.0f);
        result[2] = float32x4(zaxis.x, zaxis.y, zaxis.z, 0.0f);
        result[3] = float32x4(pos.x, pos.y, pos.z, 1.0f);
        return result;
    }

    Matrix4x4 affineInverse(const Matrix4x4& input)
    {
        const float* m = input;

        float s = input.determinant3x3();
        if (s)
        {
            s = 1.0f / s;
        }

        float m00 = (m[ 5] * m[10] - m[ 6] * m[ 9]) * s;
        float m01 = (m[ 9] * m[ 2] - m[10] * m[ 1]) * s;
        float m02 = (m[ 1] * m[ 6] - m[ 2] * m[ 5]) * s;
        float m10 = (m[ 6] * m[ 8] - m[ 4] * m[10]) * s;
        float m11 = (m[10] * m[ 0] - m[ 8] * m[ 2]) * s;
        float m12 = (m[ 2] * m[ 4] - m[ 0] * m[ 6]) * s;
        float m20 = (m[ 4] * m[ 9] - m[ 5] * m[ 8]) * s;
        float m21 = (m[ 8] * m[ 1] - m[ 9] * m[ 0]) * s;
        float m22 = (m[ 0] * m[ 5] - m[ 1] * m[ 4]) * s;
        float m30 = -(m00 * m[12] + m10 * m[13] + m20 * m[14]);
        float m31 = -(m01 * m[12] + m11 * m[13] + m21 * m[14]);
        float m32 = -(m02 * m[12] + m12 * m[13] + m22 * m[14]);

        Matrix4x4 result;
        result[0] = float32x4(m00, m01, m02, m[ 3]);
        result[1] = float32x4(m10, m11, m12, m[ 7]);
        result[2] = float32x4(m20, m21, m22, m[11]);
        result[3] = float32x4(m30, m31, m32, m[15]);
        return result;
    }

    Matrix4x4 adjoint(const Matrix4x4& input)
    {
        const float* m = input;

        float m00 = m[5] * m[10] - m[6] * m[9];
        float m01 = m[9] * m[2] - m[10] * m[1];
        float m02 = m[1] * m[6] - m[2] * m[5];
        float m10 = m[6] * m[8] - m[4] * m[10];
        float m11 = m[10] * m[0] - m[8] * m[2];
        float m12 = m[2] * m[4] - m[0] * m[6];
        float m20 = m[4] * m[9] - m[5] * m[8];
        float m21 = m[8] * m[1] - m[9] * m[0];
        float m22 = m[0] * m[5] - m[1] * m[4];
        float m30 = -(m[0] * m[12] + m[4] * m[13] + m[8] * m[14]);
        float m31 = -(m[1] * m[12] + m[5] * m[13] + m[9] * m[14]);
        float m32 = -(m[2] * m[12] + m[6] * m[13] + m[10] * m[14]);

        Matrix4x4 result;
        result[0] = float32x4(m00, m01, m02, m[3]);
        result[1] = float32x4(m10, m11, m12, m[7]);
        result[2] = float32x4(m20, m21, m22, m[11]);
        result[3] = float32x4(m30, m31, m32, m[15]);
        return result;
    }

    Matrix4x4 obliqueGL(const Matrix4x4& proj, const float32x4& nearclip)
    {
        float32x4 s = sign(nearclip);
        float xsign = s.x;
        float ysign = s.y;

        float32x4 q((xsign - proj(2,0)) / proj(0,0),
                    (ysign - proj(2,1)) / proj(1,1),
                    -1.0f, (1.0f + proj(2,2)) / proj(3,2));

        float32x4 c = nearclip * (2.0f / dot(nearclip, q));
        c += float32x4(0.0f, 0.0f, 1.0f, 0.0f);

        Matrix4x4 p = proj;
        p[0][2] = c.x;
        p[1][2] = c.y;
        p[2][2] = c.z;
        p[3][2] = c.w;
        return p;
    }

    Matrix4x4 obliqueVK(const Matrix4x4& proj, const float32x4& nearclip)
    {
        // conversion from GL to VK matrix format
        const Matrix4x4 to_vk
        {
            float32x4(1.0f, 0.0f, 0.0f, 0.0f),
            float32x4(0.0f,-1.0f, 0.0f, 0.0f),
            float32x4(0.0f, 0.0f, 0.5f, 0.5f),
            float32x4(0.0f, 0.0f, 0.0f, 1.0f)
        };

        // inverse of to_vk matrix
        const Matrix4x4 from_vk
        {
            float32x4(1.0f, 0.0f, 0.0f, 0.0f),
            float32x4(0.0f,-1.0f, 0.0f, 0.0f),
            float32x4(0.0f, 0.0f, 2.0f,-1.0f),
            float32x4(0.0f, 0.0f, 0.0f, 1.0f)
        };

        // NOTE: using the existing OpenGL function requires a round-trip to it's matrix format. :(
        Matrix4x4 p = obliqueGL(proj * from_vk, nearclip);
        return p * to_vk;
    }

    Matrix4x4 obliqueD3D(const Matrix4x4& proj, const float32x4& nearclip)
    {
        float32x4 clip = nearclip;
        float32x4 s = sign(clip);
        float xsign = s.x;
        float ysign = s.y;

        float32x4 q((xsign - proj(2,0)) / proj(0,0),
                    (ysign - proj(2,1)) / proj(1,1),
                    1.0f,
                    (1.0f - proj(2,2)) / proj(3,2));
        float32x4 c = clip / dot(clip, q);

        Matrix4x4 p = proj;
        p[0][2] = c.x;
        p[1][2] = c.y;
        p[2][2] = c.z;
        p[3][2] = c.w;
        return p;
    }

    // ------------------------------------------------------------------------
    // EulerAngles
    // ------------------------------------------------------------------------

    EulerAngles::EulerAngles(const Quaternion& q)
    {
        // x-axis
        float sr_cp = 2.0f * (q.w * q.x + q.y * q.z);
        float cr_cp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
        x = std::atan2(sr_cp, cr_cp);

        // y-axis
        float sp = 2.0f * (q.w * q.y - q.z * q.x);
        if (std::abs(sp) >= 1.0f)
        {
            // use 90 degrees if out of range
            y = std::copysign(float(pi) / 2.0f, sp);
        }
        else
        {
            y = std::asin(sp);
        }

        // z-axis
        float sy_cp = 2.0f * (q.w * q.z + q.x * q.y);
        float cy_cp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
        z = std::atan2(sy_cp, cy_cp);
    }

    // ------------------------------------------------------------------------
    // AngleAxis
    // ------------------------------------------------------------------------

    AngleAxis::AngleAxis(const Matrix3x3& m)
    {
        axis = float32x3(m[1][2] - m[2][1], m[2][0] - m[0][2], m[0][1] - m[1][0]);
        const float s = square(axis) * 0.5f;
        const float c = (m[0][0] + m[1][1] + m[2][2] - 1.0f) * 0.5f;
        angle = std::atan2(s, c);
    }

    AngleAxis::AngleAxis(const Matrix4x4& m)
    {
        axis = float32x3(m[1][2] - m[2][1], m[2][0] - m[0][2], m[0][1] - m[1][0]);
        const float s = square(axis) * 0.5f;
        const float c = (m[0][0] + m[1][1] + m[2][2] - 1.0f) * 0.5f;
        angle = std::atan2(s, c);
    }

    AngleAxis::AngleAxis(const Quaternion& q)
    {
        angle = std::acos(q.w) * 2.0f;
        axis = float32x3(q.x, q.y, q.z) / std::sqrt(1.0f - q.w * q.w);
    }

    // ------------------------------------------------------------------------
    // Quaternion
    // ------------------------------------------------------------------------

    template <int Width, int Height>
    static inline
    Quaternion matrixToQuaternion(const Matrix<float, Width, Height>& m)
    {
        const float m00 = m(0, 0);
        const float m01 = m(0, 1);
        const float m02 = m(0, 2);
        const float m10 = m(1, 0);
        const float m11 = m(1, 1);
        const float m12 = m(1, 2);
        const float m20 = m(2, 0);
        const float m21 = m(2, 1);
        const float m22 = m(2, 2);

        float s;
        Quaternion q;

        if (m22 < 0)
        {
            if (m00 > m11)
            {
                s = 1.0f + m00 - m11 - m22;
                q = Quaternion(s, m01 + m10, m20 + m02, m12 - m21);
            }
            else
            {
                s = 1.0f - m00 + m11 - m22;
                q = Quaternion(m01 + m10, s, m12 + m21, m20 - m02);
            }
        }
        else
        {
            if (m00 < -m11)
            {
                s = 1.0f - m00 - m11 + m22;
                q = Quaternion(m20 + m02, m12 + m21, s, m01 - m10);
            }
            else
            {
                s = 1.0f + m00 + m11 + m22;
                q = Quaternion(m12 - m21, m20 - m02, m01 - m10, s);
            }
        }

        return q * float(0.5f / std::sqrt(s));
    }

    const Quaternion& Quaternion::operator = (const Matrix<float, 3, 3>& m)
    {
        *this = matrixToQuaternion(m);
        return *this;
    }

    const Quaternion& Quaternion::operator = (const Matrix<float, 4, 4>& m)
    {
        *this = matrixToQuaternion(m);
        return *this;
    }

    const Quaternion& Quaternion::operator = (const AngleAxis& a)
    {
        const float theta = a.angle * 0.5f;
        const float s = std::sin(theta) / length(a.axis);
        const float c = std::cos(theta);

        x = a.axis.x * s;
        y = a.axis.y * s;
        z = a.axis.z * s;
        w = c;
        return *this;
    }

    const Quaternion& Quaternion::operator = (const EulerAngles& euler)
    {
        const float sx = std::sin(euler.x * 0.5f);
        const float cx = std::cos(euler.x * 0.5f);
        const float sy = std::sin(euler.y * 0.5f);
        const float cy = std::cos(euler.y * 0.5f);
        const float sz = std::sin(euler.z * 0.5f);
        const float cz = std::cos(euler.z * 0.5f);

        x = cz * sx * cy - sz * cx * sy;
        y = cz * cx * sy + sz * sx * cy;
        z = sz * cx * cy - cz * sx * sy;
        w = cz * cx * cy + sz * sx * sy;
        return *this;
    }

    Quaternion Quaternion::identity()
    {
        return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    }

    Quaternion Quaternion::rotateX(float angle)
    {
        float s = std::sin(angle * 0.5f);
        float c = std::cos(angle * 0.5f);
        return Quaternion(s, 0, 0, c);
    }

    Quaternion Quaternion::rotateY(float angle)
    {
        float s = std::sin(angle * 0.5f);
        float c = std::cos(angle * 0.5f);
        return Quaternion(0, s, 0, c);
    }

    Quaternion Quaternion::rotateZ(float angle)
    {
        float s = std::sin(angle * 0.5f);
        float c = std::cos(angle * 0.5f);
        return Quaternion(0, 0, s, c);
    }

    Quaternion Quaternion::rotateXYZ(float xangle, float yangle, float zangle)
    {
        return Quaternion(EulerAngles(xangle, yangle, zangle));
    }

    Quaternion Quaternion::rotate(const EulerAngles& euler)
    {
        return Quaternion(euler);
    }

    Quaternion Quaternion::rotate(const float32x3& from, const float32x3& to)
    {
        const float32x3 h = normalize(from + to);
        const float32x3 xyz = cross(from, h);
        const float w = dot(from, h);
        return Quaternion(xyz, w);
    }

    Quaternion log(const Quaternion& q)
    {
        float s = q.w ? std::atan2(std::sqrt(square(q)), q.w) : float(pi) * 2.0f;
        return Quaternion(q.x * s, q.y * s, q.z * s, 0.0f);
    }

    Quaternion exp(const Quaternion& q)
    {
        float s = std::sqrt(square(q));
        const float c = std::cos(s);
        s = (s > epsilon * 100.0f) ? std::sin(s) / s : 1.0f;
        return Quaternion(q.x * s, q.y * s, q.z * s, c);
    }

    Quaternion pow(const Quaternion& q, float p)
    {
        float s = square(q);
        const float c = std::cos(s * p);
        s = s ? std::sin(s * p) / s : 1.0f;
        return Quaternion(q.x * s, q.y * s, q.z * s, c);
    }

    Quaternion normalize(const Quaternion& q)
    {
        float s = norm(q);
        if (s)
        {
            s = 1.0f / std::sqrt(s);
        }
        return q * s;
    }

    Quaternion lndif(const Quaternion& a, const Quaternion& b)
    {
        Quaternion p = inverse(a) * b;
        const float length = std::sqrt(square(p));
        const float scale = norm(a);
        float s = scale ? std::atan2(length, scale) : float(pi) * 2.0f;
        if (length) s /= length;

        return Quaternion(p.x * s, p.y * s, p.z * s, 0.0f);
    }

    Quaternion lerp(const Quaternion& a, const Quaternion& b, float time)
    {
        float x = lerp(a.x, b.x, time);
        float y = lerp(a.y, b.y, time);
        float z = lerp(a.z, b.z, time);
		float w = a.w + (b.w - a.w) * time;
		return Quaternion(x, y, z, w);
    }

    Quaternion slerp(const Quaternion& a, const Quaternion& b, float time)
    {
        const float cosom = dot(a, b);

        if ((1.0f + cosom) > epsilon)
        {
            float sp;
            float sq;

            if ((1.0f - cosom) > epsilon)
            {
                const float omega = std::acos(cosom);
                const float sinom = 1.0f / std::sin(omega);

                sp = std::sin((1.0f - time) * omega) * sinom;
                sq = std::sin(time * omega) * sinom;
            }
            else
            {
                sp = 1.0f - time;
                sq = time;
            }

            return a * sp + b * sq;
        }
        else
        {
            const float halfpi = float(pi * 0.5);
            const float sp = std::sin((1.0f - time) * halfpi);
            const float sq = std::sin(time * halfpi);

            return Quaternion(a.x * sp - a.y * sq,
                              a.y * sp + a.x * sq,
                              a.z * sp - a.w * sq,
                              a.z);
        }
    }

    Quaternion slerp(const Quaternion& a, const Quaternion& b, int spin, float time)
    {
        float bflip = 1.0f;
        float tcos = dot(a, b);

        if (tcos < 0)
        {
            tcos = -tcos;
            bflip = -1;
        }

        float beta;
        float alpha;

        if ((1.0f - tcos) < epsilon * 100.0f)
        {
            // linear interpolate
            beta = 1.0f - time;
            alpha = time * bflip;
        }
        else
        {
            const float theta = std::acos(tcos);
            const float phi   = theta + spin * float(pi);
            const float tsin  = std::sin(theta);
            beta  = std::sin(theta - time * phi) / tsin;
            alpha = std::sin(time * phi) / tsin * bflip;
        }

        return a * beta + b * alpha;
    }

    Quaternion squad(const Quaternion& p, const Quaternion& a, const Quaternion& b, const Quaternion& q, float time)
    {
        Quaternion qa = slerp(p, q, 0, time);
        Quaternion qb = slerp(a, b, 0, time);
        return slerp(qa, qb, 0, 2.0f * time * (1.0f - time));
    }

    // ------------------------------------------------------------
    // linear/sRGB conversion tables
    // ------------------------------------------------------------

    // linear to sRGB table
    static
    const u8 encode_srgb_table [] =
    {
        0x00, 0x0c, 0x15, 0x1c, 0x21, 0x26, 0x2a, 0x2e, 0x31, 0x34, 0x37, 0x3a, 0x3d, 0x3f, 0x42, 0x44,
        0x46, 0x49, 0x4b, 0x4d, 0x4f, 0x51, 0x52, 0x54, 0x56, 0x58, 0x59, 0x5b, 0x5d, 0x5e, 0x60, 0x61,
        0x63, 0x64, 0x66, 0x67, 0x68, 0x6a, 0x6b, 0x6c, 0x6e, 0x6f, 0x70, 0x72, 0x73, 0x74, 0x75, 0x76,
        0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
        0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
        0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa1, 0xa2, 0xa3, 0xa4,
        0xa5, 0xa5, 0xa6, 0xa7, 0xa8, 0xa8, 0xa9, 0xaa, 0xab, 0xab, 0xac, 0xad, 0xae, 0xae, 0xaf, 0xb0,
        0xb0, 0xb1, 0xb2, 0xb3, 0xb3, 0xb4, 0xb5, 0xb5, 0xb6, 0xb7, 0xb7, 0xb8, 0xb9, 0xb9, 0xba, 0xbb,
        0xbb, 0xbc, 0xbd, 0xbd, 0xbe, 0xbf, 0xbf, 0xc0, 0xc1, 0xc1, 0xc2, 0xc2, 0xc3, 0xc4, 0xc4, 0xc5,
        0xc6, 0xc6, 0xc7, 0xc7, 0xc8, 0xc9, 0xc9, 0xca, 0xca, 0xcb, 0xcc, 0xcc, 0xcd, 0xcd, 0xce, 0xce,
        0xcf, 0xd0, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2, 0xd3, 0xd4, 0xd4, 0xd5, 0xd5, 0xd6, 0xd6, 0xd7, 0xd7,
        0xd8, 0xd9, 0xd9, 0xda, 0xda, 0xdb, 0xdb, 0xdc, 0xdc, 0xdd, 0xdd, 0xde, 0xde, 0xdf, 0xdf, 0xe0,
        0xe1, 0xe1, 0xe2, 0xe2, 0xe3, 0xe3, 0xe4, 0xe4, 0xe5, 0xe5, 0xe6, 0xe6, 0xe7, 0xe7, 0xe8, 0xe8,
        0xe9, 0xe9, 0xea, 0xea, 0xeb, 0xeb, 0xec, 0xec, 0xed, 0xed, 0xee, 0xee, 0xee, 0xef, 0xef, 0xf0,
        0xf0, 0xf1, 0xf1, 0xf2, 0xf2, 0xf3, 0xf3, 0xf4, 0xf4, 0xf5, 0xf5, 0xf6, 0xf6, 0xf6, 0xf7, 0xf7,
        0xf8, 0xf8, 0xf9, 0xf9, 0xfa, 0xfa, 0xfb, 0xfb, 0xfb, 0xfc, 0xfc, 0xfd, 0xfd, 0xfe, 0xfe, 0xff
    };

    // sRGB to linear table
    static
    const u8 decode_srgb_table [] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
        0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b, 0x0c, 0x0c,
        0x0c, 0x0d, 0x0d, 0x0e, 0x0e, 0x0f, 0x0f, 0x10, 0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13, 0x14,
        0x14, 0x15, 0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 0x19, 0x19, 0x1a, 0x1a, 0x1b, 0x1c, 0x1c, 0x1d,
        0x1e, 0x1e, 0x1f, 0x20, 0x20, 0x21, 0x22, 0x22, 0x23, 0x24, 0x24, 0x25, 0x26, 0x27, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
        0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3d, 0x3e, 0x3f, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
        0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x5c, 0x5d, 0x5e, 0x5f, 0x61, 0x62, 0x63, 0x65, 0x66, 0x67, 0x69, 0x6a, 0x6b, 0x6d,
        0x6e, 0x70, 0x71, 0x72, 0x74, 0x75, 0x77, 0x78, 0x7a, 0x7b, 0x7d, 0x7e, 0x80, 0x81, 0x83, 0x84,
        0x86, 0x87, 0x89, 0x8a, 0x8c, 0x8e, 0x8f, 0x91, 0x93, 0x94, 0x96, 0x98, 0x99, 0x9b, 0x9d, 0x9e,
        0xa0, 0xa2, 0xa4, 0xa5, 0xa7, 0xa9, 0xab, 0xad, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xb9, 0xbb,
        0xbd, 0xbf, 0xc1, 0xc3, 0xc5, 0xc7, 0xc9, 0xcb, 0xcd, 0xcf, 0xd1, 0xd3, 0xd5, 0xd7, 0xd9, 0xdb,
        0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xed, 0xef, 0xf1, 0xf3, 0xf5, 0xf8, 0xfa, 0xfc, 0xff 
    };

    // ------------------------------------------------------------------------
    // sRGB
    // ------------------------------------------------------------------------

    const u8* get_linear_to_srgb_table()
    {
        return encode_srgb_table;
    }

    const u8* get_srgb_to_linear_table()
    {
        return decode_srgb_table;
    }

#if 0

    //
    // precise approximations
    //

    namespace detail
    {

        static inline
        float pow24(float v)
        {
            s32 i = reinterpret_bits<s32>(v);
            i = (i >> 2) + (i >> 4);
            i += (i >> 4);
            i += (i >> 8);
            i += 0x2a514d80;
            float s = reinterpret_bits<float>(i);
            s = 0.3332454f * (2.0f * s + v / (s * s));
            return s * sqrt(sqrt(s));
        }

        static inline
        float32x4 pow24(float32x4 v)
        {
            int32x4 i = reinterpret<int32x4>(v);
            i = (i >> 2) + (i >> 4);
            i += (i >> 4);
            i += (i >> 8);
            i += 0x2a514d80;
            float32x4 s = reinterpret<float32x4>(i);
            s = 0.3332454f * (2.0f * s + v / (s * s));
            return s * sqrt(sqrt(s));
        }

        static inline
        float32x8 pow24(float32x8 v)
        {
            int32x8 i = reinterpret<int32x8>(v);
            i = (i >> 2) + (i >> 4);
            i += (i >> 4);
            i += (i >> 8);
            i += 0x2a514d80;
            float32x8 s = reinterpret<float32x8>(i);
            s = 0.3332454f * (2.0f * s + v / (s * s));
            return s * sqrt(sqrt(s));
        }

        static inline
        float root5(float v)
        {
            s32 i = reinterpret_bits<s32>(v);
            s32 d = (i >> 2) - (i >> 4) + (i >> 6) - (i >> 8) + (i >> 10);
            i = 0x32c9af22 + d;
            float f = reinterpret_bits<float>(i);
            float s = f * f;
            f -= (f - v / (s * s)) * 0.2f;
            return f;
        }

        static inline
        float32x4 root5(float32x4 v)
        {
            int32x4 i = reinterpret<int32x4>(v);
            int32x4 d = (i >> 2) - (i >> 4) + (i >> 6) - (i >> 8) + (i >> 10);
            i = 0x32c9af22 + d;
            float32x4 f = reinterpret<float32x4>(i);
            float32x4 s = f * f;
            f -= (f - v / (s * s)) * 0.2f;
            return f;
        }

        static inline
        float32x8 root5(float32x8 v)
        {
            int32x8 i = reinterpret<int32x8>(v);
            int32x8 d = (i >> 2) - (i >> 4) + (i >> 6) - (i >> 8) + (i >> 10);
            i = 0x32c9af22 + d;
            float32x8 f = reinterpret<float32x8>(i);
            float32x8 s = f * f;
            f -= (f - v / (s * s)) * 0.2f;
            return f;
        }

    } // namespace detail

    float linear_to_srgb(float linear)
    {
        linear = clamp(linear, 0.0f, 1.0f);
        float srgb = (linear < 0.0031308f) ?
            12.92f * linear :
            1.055f * detail::pow24(linear) - 0.055f;
        return srgb;
    }

    float srgb_to_linear(float srgb)
    {
        float linear;
        if (srgb <= 0.04045f)
        {
            linear = srgb * (1.0f / 12.92f);
        }
        else
        {
            float s = (srgb * (1.f / 1.055f) + 0.055f / 1.055f);
            linear = (s * s) * detail::root5(s * s);
        }
        return linear;
    }

    float32x4 linear_to_srgb(float32x4 linear)
    {
        linear = clamp(linear, 0.0f, 1.0f);
        float32x4 a = linear * 12.92f;
        float32x4 b = 1.055f * detail::pow24(linear) - 0.055f;
        float32x4 srgb = select(linear < 0.0031308f, a, b);
        return srgb;
    }

    float32x4 srgb_to_linear(float32x4 srgb)
    {
        float32x4 a = srgb * (1.0f / 12.92f);
        float32x4 s = (srgb * (1.f / 1.055f) + 0.055f / 1.055f);
        float32x4 b = (s * s) * detail::root5(s * s);
        float32x4 linear = select(srgb <= 0.04045f, a, b);
        return linear;
    }

    float32x8 linear_to_srgb(float32x8 linear)
    {
        linear = clamp(linear, 0.0f, 1.0f);
        float32x8 a = linear * 12.92f;
        float32x8 b = 1.055f * detail::pow24(linear) - 0.055f;
        float32x8 srgb = select(linear < 0.0031308f, a, b);
        return srgb;
    }

    float32x8 srgb_to_linear(float32x8 srgb)
    {
        float32x8 a = srgb * (1.0f / 12.92f);
        float32x8 s = (srgb * (1.f / 1.055f) + 0.055f / 1.055f);
        float32x8 b = (s * s) * detail::root5(s * s);
        float32x8 linear = select(srgb <= 0.04045f, a, b);
        return linear;
    }

#endif

} // namespace mango::math
