/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#include <limits>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <mango/core/bits.hpp>
#include <mango/math/math.hpp>

namespace
{

    constexpr float epsilon = std::numeric_limits<float>::epsilon();
    constexpr float pi = float(mango::math::pi);

} // namespace

namespace mango
{

    // ------------------------------------------------------------------------
    // float4x4
    // ------------------------------------------------------------------------

    const float4x4& float4x4::operator = (float s)
    {
        const float32x4 zero(0);
        m[0] = simd::set_component<0>(zero, s);
        m[1] = simd::set_component<1>(zero, s);
        m[2] = simd::set_component<2>(zero, s);
        m[3] = simd::set_component<3>(zero, 1.0f);
        return *this;
    }

    const float4x4& float4x4::operator = (const float* v)
    {
        m[0] = float32x4(v[0], v[1], v[2], v[3]);
        m[1] = float32x4(v[4], v[5], v[6], v[7]);
        m[2] = float32x4(v[8], v[9], v[10], v[11]);
        m[3] = float32x4(v[12], v[13], v[14], v[15]);
        return *this;
    }

    const float4x4& float4x4::operator = (const quat& q)
    {
        const float32x3 s = q.xyz * 2.0f;
        const float32x3 w = s * q.w;
        const float32x3 x = s * q.xyz.x;
        const float yy = q.xyz.y * s.y;
        const float yz = q.xyz.y * s.z;
        const float zz = q.xyz.z * s.z;

        m[0] = float32x4(1.0f - yy - zz, x.y + w.z, x.z - w.y, 0.0f);
        m[1] = float32x4(x.y - w.z, 1.0f - x.x - zz, yz + w.x, 0.0f);
        m[2] = float32x4(x.z + w.y, yz - w.x, 1.0f - x.x - yy, 0.0f);
        m[3] = float32x4(0.0f, 0.0f, 0.0f, 1.0f);

        return *this;
    }

    const float4x4& float4x4::operator = (const AngleAxis& a)
    {
        const float length2 = square(a.axis);
        if (length2 < epsilon)
        {
            *this = 1.0f; // set identity
            return *this;
        }

        const float length = 1.0f / std::sqrt(length2);

        const float x = a.axis.x * length;
        const float y = a.axis.y * length;
        const float z = a.axis.z * length;

        const float s = std::sin(a.angle);
        const float c = std::cos(a.angle);
        const float i = 1.0f - c;

        const float xx = x * x * i + c;
        const float yy = y * y * i + c;
        const float zz = z * z * i + c;
        const float xy = x * y * i;
        const float yz = y * z * i;
        const float zx = z * x * i;
        const float xs = x * s;
        const float ys = y * s;
        const float zs = z * s;

        m[0] = float32x4(xx, xy + zs, zx - ys, 0.0f);
        m[1] = float32x4(xy - zs, yy, yz + xs, 0.0f);
        m[2] = float32x4(zx + ys, yz - xs, zz, 0.0f);
        m[3] = float32x4(0.0f, 0.0f, 0.0f, 1.0f);

        return *this;
    }

    bool float4x4::isAffine() const
    {
        const float* p = *this;
        return p[ 3] == 0.0f &&
               p[ 7] == 0.0f &&
               p[11] == 0.0f &&
               p[15] == 1.0f;
    }

    float float4x4::determinant() const
    {
        const float* p = *this;
        const float a = p[0] * p[5] * p[10] + p[1] * p[6] * p[ 8] + p[2] * p[4] * p[9];
        const float b = p[2] * p[5] * p[ 8] + p[1] * p[4] * p[10] + p[0] * p[6] * p[9];
        return a - b;
    }

    void float4x4::setIdentity()
    {
        const float32x4 zero(0);
        m[0] = simd::set_component<0>(zero, 1.0f);
        m[1] = simd::set_component<1>(zero, 1.0f);
        m[2] = simd::set_component<2>(zero, 1.0f);
        m[3] = simd::set_component<3>(zero, 1.0f);
    }

    void float4x4::translate(float x, float y, float z)
    {
        const float32x4 v = float32x4(x, y, z, 0.0f);
        m[0] = madd(m[0], m[0].wwww, v);
        m[1] = madd(m[1], m[1].wwww, v);
        m[2] = madd(m[2], m[2].wwww, v);
        m[3] = madd(m[3], m[3].wwww, v);
    }

    void float4x4::translate(const float32x3& t)
    {
        const float32x4 v = float32x4(t.x, t.y, t.z, 0.0f);
        m[0] = madd(m[0], m[0].wwww, v);
        m[1] = madd(m[1], m[1].wwww, v);
        m[2] = madd(m[2], m[2].wwww, v);
        m[3] = madd(m[3], m[3].wwww, v);
    }

    void float4x4::scale(float s)
    {
        const float32x4 zero(0);
        m[0] = simd::set_component<0>(zero, s);
        m[1] = simd::set_component<1>(zero, s);
        m[2] = simd::set_component<2>(zero, s);
        m[3] = simd::set_component<3>(zero, 1.0f);
    }

    void float4x4::scale(float x, float y, float z)
    {
        const float32x4 zero(0);
        m[0] = simd::set_component<0>(zero, x);
        m[1] = simd::set_component<1>(zero, y);
        m[2] = simd::set_component<2>(zero, z);
        m[3] = simd::set_component<3>(zero, 1.0f);
    }

    void float4x4::scale(const float32x3& s)
    {
        const float32x4 v = float32x4(s.x, s.y, s.z, 1.0f);
        m[0] = m[0] * v;
        m[1] = m[1] * v;
        m[2] = m[2] * v;
        m[3] = m[3] * v;
    }

#if defined(MANGO_ENABLE_SIMD)

    void float4x4::rotate(float angle, const float32x3& axis)
    {
        const float4x4 temp = AngleAxis(angle, axis);
        *this = *this * temp;
    }

#else

    void float4x4::rotate(float angle, const float32x3& axis)
    {
        const float4x4 temp = AngleAxis(angle, axis);

        const float m00 = temp(0, 0);
        const float m01 = temp(0, 1);
        const float m02 = temp(0, 2);
        const float m10 = temp(1, 0);
        const float m11 = temp(1, 1);
        const float m12 = temp(1, 2);
        const float m20 = temp(2, 0);
        const float m21 = temp(2, 1);
        const float m22 = temp(2, 2);

        float* p = *this;

        // multiply_by_3x3
        for (int i = 0; i < 4; ++i)
        {
            const float x = p[0];
            const float y = p[1];
            const float z = p[2];
            p[0] = x * m00 + y * m10 + z * m20;
            p[1] = x * m01 + y * m11 + z * m21;
            p[2] = x * m02 + y * m12 + z * m22;
            p += 4;
        }
    }

#endif

    void float4x4::rotateX(float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        float* p = *this;
        
        for (int i = 0; i < 4; ++i)
        {
            const float y = p[1];
            const float z = p[2];
            p[1] = y * c - z * s;
            p[2] = z * c + y * s;
            p += 4;
        }
    }

    void float4x4::rotateY(float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        float* p = *this;
        
        for (int i = 0; i < 4; ++i)
        {
            const float x = p[0];
            const float z = p[2];
            p[0] = x * c + z * s;
            p[2] = z * c - x * s;
            p += 4;
        }
    }

    void float4x4::rotateZ(float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        float* p = *this;

        for (int i = 0; i < 4; ++i)
        {
            const float x = p[0];
            const float y = p[1];
            p[0] = x * c - y * s;
            p[1] = y * c + x * s;
            p += 4;
        }
    }

#if defined(MANGO_ENABLE_SIMD)

    void float4x4::rotateXYZ(float xangle, float yangle, float zangle)
    {
        const float4x4 temp = matrix::rotateXYZ(xangle, yangle, zangle);
        *this = *this * temp;
    }

    float4x4 normalize(const float4x4& m)
    {
        float32x4 x = m[0];
        float32x4 y = m[1];
        float32x4 z = m[2];
        x = normalize(x);
        y = normalize(y - x * dot(x, y));
        z = cross(x, y);
        return float4x4(x, y, z, m[3]);
    }

#else

    void float4x4::rotateXYZ(float xangle, float yangle, float zangle)
    {
        const float32x4 v = float4(xangle, yangle, zangle, 0.0f);
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

        const float m00 = cz * cy;
        const float m01 = sz * cy;
        const float m02 = -sy;
        const float m10 = cz * sysx - sz * cx;
        const float m11 = sz * sysx + cz * cx;
        const float m12 = cy * sx;
        const float m20 = cz * sycx + sz * sx;
        const float m21 = sz * sycx - cz * sx;
        const float m22 = cy * cx;

        float* p = *this;

        // multiply_by_3x3
        for (int i = 0; i < 4; ++i)
        {
            const float x = p[0];
            const float y = p[1];
            const float z = p[2];
            p[0] = x * m00 + y * m10 + z * m20;
            p[1] = x * m01 + y * m11 + z * m21;
            p[2] = x * m02 + y * m12 + z * m22;
            p += 4;
        }
    }

    float4x4 normalize(const float4x4& _m)
    {
        const float* m = _m;

        float32x3 x(m[0], m[1], m[2]);
        float32x3 y(m[4], m[5], m[6]);
        float32x3 z(m[8], m[9], m[10]);

        x = normalize(x);
        y = normalize(y - x * dot(x, y));
        z = cross(x, y);

        float4x4 result;

        result[0] = float4(x, 0.0f);
        result[1] = float4(y, 0.0f);
        result[2] = float4(z, 0.0f);
        result[3] = float4(m[12], m[13], m[14], 1.0f);

        return result;
    }

#endif

    float4x4 mirror(const float4x4& M, const float32x4& plane)
    {
        const float* m = M;

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
        
        float4x4 result;
        
        result[0] = float4(xaxis.x, xaxis.y, xaxis.z, 0.0f);
        result[1] = float4(yaxis.x, yaxis.y, yaxis.z, 0.0f);
        result[2] = float4(zaxis.x, zaxis.y, zaxis.z, 0.0f);
        result[3] = float4(pos.x, pos.y, pos.z, 1.0f);
        
        return result;
    }

    float4x4 affineInverse(const float4x4& M)
    {
        const float* m = M;
        
        float s = 1.0f / M.determinant();
        
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

        float4x4 result;

        result[0] = float4(m00, m01, m02, m[ 3]);
        result[1] = float4(m10, m11, m12, m[ 7]);
        result[2] = float4(m20, m21, m22, m[11]);
        result[3] = float4(m30, m31, m32, m[15]);

        return result;
    }

    float4x4 adjoint(const float4x4& M)
    {
        const float* m = M;

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

        float4x4 result;

        result[0] = float4(m00, m01, m02, m[3]);
        result[1] = float4(m10, m11, m12, m[7]);
        result[2] = float4(m20, m21, m22, m[11]);
        result[3] = float4(m30, m31, m32, m[15]);

        return result;
    }

namespace matrix {

    float4x4 identity()
    {
        return float4x4(1.0f);
    }

    float4x4 translate(float x, float y, float z)
    {
        float4x4 m;
        const float32x4 zero(0);
        m[0] = simd::set_component<0>(zero, 1.0f);
        m[1] = simd::set_component<1>(zero, 1.0f);
        m[2] = simd::set_component<2>(zero, 1.0f);
        m[3] = float32x4(x, y, z, 1.0f);
        return m;
    }

    float4x4 translate(const float32x3& translation)
    {
        float4x4 m;
        const float32x4 zero(0);
        m[0] = simd::set_component<0>(zero, 1.0f);
        m[1] = simd::set_component<1>(zero, 1.0f);
        m[2] = simd::set_component<2>(zero, 1.0f);
        m[3] = float32x4(translation.x, translation.y, translation.z, 1.0f);
        return m;
    }

    float4x4 scale(float s)
    {
        float4x4 m;
        const float32x4 zero(0);
        m[0] = simd::set_component<0>(zero, s);
        m[1] = simd::set_component<1>(zero, s);
        m[2] = simd::set_component<2>(zero, s);
        m[3] = simd::set_component<3>(zero, 1.0f);
        return m;
    }

    float4x4 scale(float x, float y, float z)
    {
        float4x4 m;
        const float32x4 zero(0);
        m[0] = simd::set_component<0>(zero, x);
        m[1] = simd::set_component<1>(zero, y);
        m[2] = simd::set_component<2>(zero, z);
        m[3] = simd::set_component<3>(zero, 1.0f);
        return m;
    }

    float4x4 scale(const float32x3& s)
    {
        float4x4 m;
        const float32x4 zero(0);
        m[0] = simd::set_component<0>(zero, s.x);
        m[1] = simd::set_component<1>(zero, s.y);
        m[2] = simd::set_component<2>(zero, s.z);
        m[3] = simd::set_component<3>(zero, 1.0f);
        return m;
    }

    float4x4 rotate(float angle, const float32x3& axis)
    {
        const float4x4 temp = AngleAxis(angle, axis);
        return temp;
    }

    float4x4 rotateX(float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        return float4x4
        {
            float4(1, 0, 0, 0),
            float4(0, c, s, 0),
            float4(0,-s, c, 0),
            float4(0, 0, 0, 1)
        };
    }

    float4x4 rotateY(float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        return float4x4
        {
            float4(c, 0,-s, 0),
            float4(0, 1, 0, 0),
            float4(s, 0, c, 0),
            float4(0, 0, 0, 1)
        };
     }

    float4x4 rotateZ(float angle)
    {
        const float s = std::sin(angle);
        const float c = std::cos(angle);

        return float4x4
        {
            float4( c, s, 0, 0),
            float4(-s, c, 0, 0),
            float4( 0, 0, 1, 0),
            float4( 0, 0, 0, 1)
        };
    }

    float4x4 rotateXYZ(float x, float y, float z)
    {
        const float32x4 v = float4(x, y, z, 0.0f);
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

        const float m00 = cz * cy;
        const float m01 = sz * cy;
        const float m02 = -sy;
        const float m10 = cz * sysx - sz * cx;
        const float m11 = sz * sysx + cz * cx;
        const float m12 = cy * sx;
        const float m20 = cz * sycx + sz * sx;
        const float m21 = sz * sycx - cz * sx;
        const float m22 = cy * cx;

        return float4x4
        {
            float4(m00, m01, m02, 0),
            float4(m10, m11, m12, 0),
            float4(m20, m21, m22, 0),
            float4(0, 0, 0, 1)
        };
    }

    float4x4 lookat(const float32x3& target, const float32x3& viewer, const float32x3& up)
    {
        const float32x3 zaxis = normalize(target - viewer);
        const float32x3 xaxis = normalize(cross(up, zaxis));
        const float32x3 yaxis = cross(zaxis, xaxis);

        return float4x4
        {
            float4(xaxis.x, yaxis.x, zaxis.x, 0),
            float4(xaxis.y, yaxis.y, zaxis.y, 0),
            float4(xaxis.z, yaxis.z, zaxis.z, 0),
            float4(-dot(xaxis, viewer), -dot(yaxis, viewer), -dot(zaxis, viewer), 1.0f)
        };
    }

} // namespace matrix

namespace opengl {

    float4x4 ortho(float left, float right, float bottom, float top, float znear, float zfar)
    {
        float x = 2.0f / (right - left);
        float y = 2.0f / (top - bottom);
        float z = -2.0f / (zfar - znear);
        float a = -(left + right) / (right - left);
        float b = -(bottom + top) / (top - bottom);
        float c = -(znear + zfar) / (zfar - znear);

        return float4x4
        {
            float4(x, 0, 0, 0),
            float4(0, y, 0, 0),
            float4(0, 0, z, 0),
            float4(a, b, c, 1)
        };
    }

    float4x4 frustum(float left, float right, float bottom, float top, float znear, float zfar)
    {
        float a = (right + left) / (right - left);
        float b = (top + bottom) / (top - bottom);
        float c = -(zfar + znear) / (zfar - znear);
        float d = -(2.0f * znear * zfar) / (zfar - znear);
        float x = (2.0f * znear) / (right - left);
        float y = (2.0f * znear) / (top - bottom);
        float z = -1.0f;

        return float4x4
        {
            float4(x, 0, 0, 0),
            float4(0, y, 0, 0),
            float4(a, b, c, z),
            float4(0, 0, d, 0)
        };
    }

    float4x4 perspective(float xfov, float yfov, float znear, float zfar)
    {
        float x = znear * std::tan(xfov * 0.5f);
        float y = znear * std::tan(yfov * 0.5f);
        return frustum(-x, x, -y, y, znear, zfar);
    }

    float4x4 oblique(const float4x4& proj, const float32x4& nearclip)
    {
        float32x4 s = sign(nearclip);
        float xsign = s.x;
        float ysign = s.y;

        float32x4 q((xsign - proj(2,0)) / proj(0,0),
                    (ysign - proj(2,1)) / proj(1,1),
                    -1.0f, (1.0f + proj(2,2)) / proj(3,2));

        float32x4 c = nearclip * (float4(2.0f) / dot(nearclip, q));
        c += float4(0.0f, 0.0f, 1.0f, 0.0f);

        float4x4 p = proj;

        p(0, 2) = c.x;
        p(1, 2) = c.y;
        p(2, 2) = c.z;
        p(3, 2) = c.w;

        return p;
    }

} // namespace opengl

namespace vulkan {

    float4x4 ortho(float left, float right, float bottom, float top, float znear, float zfar)
    {
        float x = 2.0f / (right - left);
        float y = 2.0f / (bottom - top);
        float z = 1.0f / (znear - zfar);
        float a = (right + left) / (left - right);
        float b = (top + bottom) / (top - bottom);
        float c = (zfar + znear) / (zfar - znear) * -0.5f;

        return float4x4
        {
            float4(x, 0, 0, 0),
            float4(0, y, 0, 0),
            float4(0, 0, z, z),
            float4(a, b, c, c + 1.0f)
        };
    }

    float4x4 frustum(float left, float right, float bottom, float top, float znear, float zfar)
    {
        float a = (right + left) / (right - left);
        float b = (top + bottom) / (bottom - top);
        float c = (zfar + znear) / (znear - zfar) * 0.5f;
        float d = (2.0f * znear * zfar) / (znear - zfar) * 0.5f;
        float x = (2.0f * znear) / (right - left);
        float y = (2.0f * znear) / (bottom - top);

        return float4x4
        {
            float4(x, 0, 0, 0),
            float4(0, y, 0, 0),
            float4(a, b, c, c - 1.0f),
            float4(0, 0, d, d)
        };
    }

    float4x4 perspective(float xfov, float yfov, float znear, float zfar)
    {
        float x = znear * std::tan(xfov * 0.5f);
        float y = znear * std::tan(yfov * 0.5f);
        return frustum(-x, x, -y, y, znear, zfar);
    }

    float4x4 oblique(const float4x4& proj, const float32x4& nearclip)
    {
        // conversion from GL to VK matrix format
        const float4x4 to_vk
        {
            float4(1.0f, 0.0f, 0.0f, 0.0f),
            float4(0.0f,-1.0f, 0.0f, 0.0f),
            float4(0.0f, 0.0f, 0.5f, 0.5f),
            float4(0.0f, 0.0f, 0.0f, 1.0f)
        };

        // inverse of to_vk matrix
        const float4x4 from_vk
        {
            float4(1.0f, 0.0f, 0.0f, 0.0f),
            float4(0.0f,-1.0f, 0.0f, 0.0f),
            float4(0.0f, 0.0f, 2.0f,-1.0f),
            float4(0.0f, 0.0f, 0.0f, 1.0f)
        };

        // NOTE: using the existing OpenGL function requires a round-trip to it's matrix format. :(
        float4x4 p = opengl::oblique(proj * from_vk, nearclip);
        return p * to_vk;
    }

} // namespace vulkan

namespace directx {

    // left-handed

    float4x4 ortho(float left, float right, float bottom, float top, float znear, float zfar)
    {
        const float x = 1.0f / (right - left);
        const float y = 1.0f / (top - bottom);
        const float z = 1.0f / (zfar - znear);

        const float w = 2.0f * x;
        const float h = 2.0f * y;
        const float a = -x * (left + right);
        const float b = -y * (bottom + top);
        const float c = -z * znear;

        return float4x4
        {
            float4(w, 0, 0, 0),
            float4(0, h, 0, 0),
            float4(0, 0, z, 0),
            float4(a, b, c, 1)
        };
    }

    float4x4 frustum(float left, float right, float bottom, float top, float znear, float zfar)
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

        return float4x4
        {
            float4(w, 0, 0, 0),
            float4(0, h, 0, 0),
            float4(a, b, c, 1),
            float4(0, 0, d, 0)
        };
    }

    float4x4 perspective(float xfov, float yfov, float znear, float zfar)
    {
        const float w = 1.0f / std::tan(xfov * 0.5f);
        const float h = 1.0f / std::tan(yfov * 0.5f);
        const float a = zfar / (zfar - znear);
        const float b = -a * znear;

        return float4x4
        {
            float4(w, 0, 0, 0),
            float4(0, h, 0, 0),
            float4(0, 0, a, 1),
            float4(0, 0, b, 0)
        };
    }

    float4x4 oblique(const float4x4& proj, const float32x4& nearclip)
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

        float4x4 p = proj;

        p(0, 2) = c.x;
        p(1, 2) = c.y;
        p(2, 2) = c.z;
        p(3, 2) = c.w;

        return p;
    }

} // namespace directx

    // ------------------------------------------------------------------------
    // AngleAxis
    // ------------------------------------------------------------------------

    AngleAxis::AngleAxis(const float4x4& m)
    {
        axis = float3(m[1][2] - m[2][1], m[2][0] - m[0][2], m[0][1] - m[1][0]);
        const float s = square(axis) * 0.5f;
        const float c = (m[0][0] + m[1][1] + m[2][2] - 1.0f) * 0.5f;
        angle = std::atan2(s, c);
    }

    AngleAxis::AngleAxis(const Quaternion& q)
    {
        angle = std::acos(q.w) * 2.0f;
        axis = q.xyz / std::sqrt(1.0f - q.w * q.w);
    }

    // ------------------------------------------------------------------------
    // Quaternion
    // ------------------------------------------------------------------------

    const Quaternion& Quaternion::operator = (const Matrix<float, 4, 4>& m)
    {
#if 1
        const float m00 = m(0, 0);
        const float m11 = m(1, 1);
        const float m22 = m(2, 2);

        const float m01 = m(0, 1);
        const float m02 = m(0, 2);
        const float m10 = m(1, 0);
        const float m12 = m(1, 2);
        const float m20 = m(2, 0);
        const float m21 = m(2, 1);

        float t;
        Quaternion q;

        if (m22 < 0)
        {
            if (m00 > m11)
            {
                t = 1.0f + m00 - m11 - m22;
                q = Quaternion(t, m01 + m10, m20 + m02, m12 - m21);
            }
            else
            {
                t = 1.0f - m00 + m11 - m22;
                q = Quaternion(m01 + m10, t, m12 + m21, m20 - m02);
            }
        }
        else
        {
            if (m00 < -m11)
            {
                t = 1.0f - m00 - m11 + m22;
                q = Quaternion(m20 + m02, m12 + m21, t, m01 - m10);
            }
            else
            {
                t = 1.0f + m00 + m11 + m22;
                q = Quaternion(m12 - m21, m20 - m02, m01 - m10, t);
            }
        }

        *this = q * (0.5f / std::sqrt(t));
        return *this;
#endif
#if 0
        Quaternion q;

        const float det = m(0,0) + m(1,1) + m(2,2);
        if (det > 0)
        {
            float s = std::sqrt(det + 1.0f);
            q.w = s * 0.5f;
            s = 0.5f / s;
            q[0] = (m(1,2) - m(2,1)) * s;
            q[1] = (m(2,0) - m(0,2)) * s;
            q[2] = (m(0,1) - m(1,0)) * s;
        }
        else
        {
            int i = 0;
            if (m(1,1) > m(0,0)) i = 1;
            if (m(2,2) > m(i,i)) i = 2;

            int j = i + 1; if (j > 2) j = 0;
            int k = j + 1; if (k > 2) k = 0;

            float s = std::sqrt(m(i,i) - m(j,j) - m(k,k) + 1.0f);
            if (s < epsilon)
            {
				q.xyz = float3(0.0f, 0.0f, 0.0f);
                q.w = 1.0f;
            }
            else
            {
                float* v = q;
                v[i] = s * 0.5f;
                s = 0.5f / s;
                v[3] = (m(j,k) - m(k,j)) * s;
                v[j] = (m(i,j) + m(j,i)) * s;
                v[k] = (m(i,k) + m(k,i)) * s;
            }
        }

        *this = q;
        return *this;
#endif
    }

    const Quaternion& Quaternion::operator = (const AngleAxis& a)
    {
        const float theta = a.angle * 0.5f;
        const float s = std::sin(theta) / length(a.axis);
        const float c = std::cos(theta);
        xyz = a.axis * s;
        w = c;
        return *this;
    }

    Quaternion Quaternion::identity()
    {
        return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    }

    Quaternion Quaternion::rotateXYZ(float xangle, float yangle, float zangle)
    {
        const float32x4 v = float4(xangle, yangle, zangle, 0.0f) * 0.5f;
        const float32x4 s = sin(v);
        const float32x4 c = cos(v);
        const float sx = s.x;
        const float sy = s.y;
        const float sz = s.z;
        const float cx = c.x;
        const float cy = c.y;
        const float cz = c.z;
		const float cycz = cy * cz;
		const float sysz = sy * sz;
		const float sycz = sy * cz;
		const float cysz = cy * sz;
        const float x = cycz * cx - sysz * sx;
        const float y = sysz * cx + cycz * sx;
        const float z = sycz * cx + cysz * sx;
        const float w = cysz * cx - sycz * sx;
        return Quaternion(x, y, z, w);
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
        float s = q.w ? std::atan2(std::sqrt(square(q)), q.w) : pi * 2.0f;
        return Quaternion(q.xyz * s, 0.0f);
    }

    Quaternion exp(const Quaternion& q)
    {
        float s = std::sqrt(square(q));
        const float c = std::cos(s);
        s = (s > epsilon * 100.0f) ? std::sin(s) / s : 1.0f;
        return Quaternion(q.xyz * s, c);
    }

    Quaternion pow(const Quaternion& q, float p)
    {
        float s = square(q);
        const float c = std::cos(s * p);
        s = s ? std::sin(s * p) / s : 1.0f;
        return Quaternion(q.xyz * s, c);
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
        float s = scale ? std::atan2(length, scale) : pi * 2.0f;
        if (length) s /= length;

        return Quaternion(p.xyz * s, 0.0f);
    }

    Quaternion lerp(const Quaternion& a, const Quaternion& b, float time)
    {
		const float32x3 xyz = lerp(a.xyz, b.xyz, time);
		const float w = a.w + (b.w - a.w) * time;
		return Quaternion(xyz, w);
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
            const float halfpi = pi * 0.5f;
            const float sp = std::sin((1.0f - time) * halfpi);
            const float sq = std::sin(time * halfpi);

			// TODO: check the return value
            return Quaternion(a.xyz.x * sp - a.xyz.y * sq,
                              a.xyz.y * sp + a.xyz.x * sq,
                              a.xyz.z * sp - a.w * sq,
                              a.xyz.z);
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
            const float phi   = theta + spin * pi;
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

    // ------------------------------------------------------------------------
    // sRGB
    // ------------------------------------------------------------------------

    float linear_to_srgb(float n)
    {
        float s = clamp(n, 0.0f, 1.0f);
        s = (s < 0.0031308f) ? 12.92f * s : 1.055f * std::pow(s, 0.41666f) - 0.055f;
        return s;
    }

    float srgb_to_linear(float s)
    {
        return (s <= 0.04045f) ? s * (1.0f / 12.92f) : std::pow(((s + 0.055f) * (1.0f / 1.055f)), 2.4f);
    }

    static inline float32x4 pow24(float32x4 v)
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

    static inline float32x4 root5(float32x4 v)
    {
        int32x4 i = reinterpret<int32x4>(v);
        int32x4 d = (i >> 2) - (i >> 4) + (i >> 6) - (i >> 8) + (i >> 10);
        i = 0x32c9af22 + d;
        float32x4 f = reinterpret<float32x4>(i);
        float32x4 s = f * f;
        f -= (f - v / (s * s)) * 0.2f;
        return f;
    }

    float32x4 linear_to_srgb(float32x4 linear)
    {
        float32x4 a = linear * 12.92f;
        float32x4 b = 1.055f * pow24(linear) - 0.055f;
        float32x4 srgb = select(linear <= 0.0031308f, a, b);
        return srgb;
    }

    float32x4 srgb_to_linear(float32x4 srgb)
    {
        float32x4 a = srgb * (1.0f / 12.92f);
        float32x4 b = (srgb * (1.f / 1.055f) + 0.055f / 1.055f);
        float32x4 c = (b * b) * root5(b * b);
        float32x4 linear = select(srgb < 0.04045f, a, c);
        return linear;
    }

} // namespace mango
