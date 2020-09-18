/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>
#include <mango/math/matrix.hpp>

namespace mango
{

    struct AngleAxis;

    // ------------------------------------------------------------------
    // Quaternion
    // ------------------------------------------------------------------

    struct Quaternion
    {
        Vector<float, 3> xyz;
        float w;

        explicit Quaternion()
            : w(0.0f)
        {
        }

        explicit Quaternion(float x, float y, float z, float w)
            : xyz(x, y, z)
            , w(w)
        {
        }

        Quaternion(const Quaternion& q)
            : xyz(q.xyz)
            , w(q.w)
        {
        }

        Quaternion(const Vector<float, 3>& v, float w)
            : xyz(v)
            , w(w)
        {
        }

        Quaternion(const Vector<float, 4>& v)
        {
            xyz = v.xyz;
            w = v.w;
        }

        explicit Quaternion(const Matrix<float, 4, 4>& m)
        {
            *this = m;
        }

        explicit Quaternion(const AngleAxis& a)
        {
            *this = a;
        }

        ~Quaternion()
        {
        }

        const Quaternion& operator = (const Quaternion& q)
        {
            xyz = q.xyz;
            w = q.w;
            return *this;
        }

        const Quaternion& operator = (const Matrix<float, 4, 4>& m);
        const Quaternion& operator = (const AngleAxis& a);

        const Quaternion& operator += (const Quaternion& q)
        {
            xyz += q.xyz;
            w += q.w;
            return *this;
        }

        const Quaternion& operator -= (const Quaternion& q)
        {
            xyz -= q.xyz;
            w -= q.w;
            return *this;
        }

        const Quaternion& operator *= (const Quaternion& q)
        {
            float s = w * q.w - dot(xyz, q.xyz);
            *this = Quaternion(w * q.xyz + xyz * q.w + cross(xyz, q.xyz), s);
            return *this;
        }

        const Quaternion& operator *= (float s)
        {
            xyz *= s;
            w *= s;
            return *this;
        }

        Quaternion operator + () const
        {
            return *this;
        }

        Quaternion operator - () const
        {
    		return Quaternion(-xyz, -w);
        }

        operator Vector<float, 4> () const
        {
            return Vector<float, 4>(xyz, w);
        }

        static Quaternion identity();
        static Quaternion rotateXYZ(float xangle, float yangle, float zangle);
        static Quaternion rotate(const Vector<float, 3>& from, const Vector<float, 3>& to);
    };

    // ------------------------------------------------------------------
    // AngleAxis
    // ------------------------------------------------------------------

    struct AngleAxis
    {
        float angle;
        Vector<float, 3> axis;

        AngleAxis() = default;

        AngleAxis(float Angle, const Vector<float, 3>& Axis)
            : angle(Angle)
            , axis(Axis)
        {
        }

        AngleAxis(const float4x4& m);
        AngleAxis(const Quaternion& q);
        ~AngleAxis() = default;
    };

    // ------------------------------------------------------------------
    // Quaternion operators
    // ------------------------------------------------------------------

    static inline Quaternion operator + (const Quaternion& a, const Quaternion& b)
    {
		return Quaternion(a.xyz + b.xyz, a.w + b.w);
    }

    static inline Quaternion operator - (const Quaternion& a, const Quaternion& b)
    {
		return Quaternion(a.xyz - b.xyz, a.w - b.w);
    }

    static inline Quaternion operator * (const Quaternion& q, float s)
    {
		return Quaternion(q.xyz * s, q.w * s);
    }

    static inline Quaternion operator * (float s, const Quaternion& q)
    {
		return Quaternion(q.xyz * s, q.w * s);
    }

    static inline Quaternion operator * (const Quaternion& a, const Quaternion& b)
    {
		const Vector<float, 3> xyz = a.w * b.xyz + b.w * a.xyz + cross(a.xyz, b.xyz);
		const float w = a.w * b.w - dot(a.xyz, b.xyz);
        return Quaternion(xyz, w);
    }

    static inline Vector<float, 3> operator * (const Vector<float, 3>& v3, const Quaternion& q)
    {
        const Vector<float, 3> n = cross(q.xyz, v3);
        return v3 + 2.0f * (q.w * n + cross(q.xyz, n));
    }

    // ------------------------------------------------------------------
    // Quaternion functions
    // ------------------------------------------------------------------

    static inline float dot(const Quaternion& a, const Quaternion& b)
    {
		return dot(a.xyz, b.xyz) + a.w * b.w;
    }

    static inline float norm(const Quaternion& q)
    {
        return square(q.xyz) + q.w * q.w;
    }

    static inline float square(const Quaternion& q)
    {
        return square(q.xyz) + q.w * q.w;
    }

    static inline float mod(const Quaternion& q)
    {
        return float(std::sqrt(norm(q)));
    }

    static inline Quaternion negate(const Quaternion& q)
    {
        const float s = -1.0f / mod(q);
        return Quaternion(q.xyz * s, q.w * -s);
    }

    static inline Quaternion inverse(const Quaternion& q)
    {
        const float s = -1.0f / norm(q);
        return Quaternion(q.xyz * s, q.w * -s);
    }

    static inline Quaternion conjugate(const Quaternion& q)
    {
        return Quaternion(-q.xyz, q.w);
    }

    Quaternion log(const Quaternion& q);
    Quaternion exp(const Quaternion& q);
    Quaternion pow(const Quaternion& q, float p);
    Quaternion normalize(const Quaternion& q);
    Quaternion lndif(const Quaternion& a, const Quaternion& b);
    Quaternion lerp(const Quaternion& a, const Quaternion& b, float time);
    Quaternion slerp(const Quaternion& a, const Quaternion& b, float time);
    Quaternion slerp(const Quaternion& a, const Quaternion& b, int spin, float time);
    Quaternion squad(const Quaternion& p, const Quaternion& a, const Quaternion& b, const Quaternion& q, float time);

    // ------------------------------------------------------------------
    // types
    // ------------------------------------------------------------------

    using quat = Quaternion;

} // namespace mango
