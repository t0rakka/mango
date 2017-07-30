/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // Vector<float, 2>
    // ------------------------------------------------------------------

    template <>
    struct Vector<float, 2> : VectorBase<float, 2>
    {
        template <int X, int Y>
        struct Permute2
        {
			float v[2];

			operator Vector<float, 2> () const
			{
				return Vector<float, 2>(v[X], v[Y]);
			}
        };

        union
        {
            struct { float x, y; };

			Permute2<0, 0> xx;
			Permute2<1, 0> yx;
			Permute2<0, 1> xy;
			Permute2<1, 1> yy;
        };

        explicit Vector()
        {
        }

        explicit Vector(float s)
        {
			x = s;
			y = s;
        }

        explicit Vector(int s)
        {
            const float f = float(s);
			x = f;
			y = f;
        }

        explicit Vector(float s0, float s1)
        {
			x = s0;
			y = s1;
        }

        Vector(const Vector& v)
        {
			x = v.x;
			y = v.y;
        }

        template <int X, int Y>
        Vector(const Permute2<X, Y>& p)
        {
			const float* v = p.v;
			x = v[X];
			y = v[Y];
        }

        ~Vector()
        {
        }

        template <int X, int Y>
        Vector& operator = (const Permute2<X, Y>& p)
        {
			const float* v = p.v;
			x = v[X];
			y = v[Y];
            return *this;
        }

        Vector& operator = (float s)
        {
			x = s;
			y = s;
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
			x = v.x;
			y = v.y;
            return *this;
        }
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline const float2& operator + (const float2& v)
    {
        return v;
    }

    static inline float2 operator - (const float2& v)
    {
        return float2(-v.x, -v.y);
    }

    static inline float2& operator += (float2& a, const float2& b)
    {
        a.x += b.x;
        a.y += b.y;
        return a;
    }

    static inline float2& operator -= (float2& a, const float2& b)
    {
        a.x -= b.x;
        a.y -= b.y;
        return a;
    }

    static inline float2& operator *= (float2& a, const float2& b)
    {
        a.x *= b.x;
        a.y *= b.y;
        return a;
    }

    static inline float2& operator *= (float2& a, float b)
    {
        a.x *= b;
        a.y *= b;
        return a;
    }

    static inline float2& operator /= (float2& a, const float2& b)
    {
        a.x /= b.x;
        a.y /= b.y;
        return a;
    }

    static inline float2& operator /= (float2& a, float b)
    {
        b = 1.0f / b;
        a.x *= b;
        a.y *= b;
        return a;
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline float2 clamp(const float2& a, const float2& amin, const float2& amax)
    {
        const float x = std::max(amin.x, std::min(amax.x, a.x));
        const float y = std::max(amin.y, std::min(amax.y, a.y));
        return float2(x, y);
    }

    static inline float2 madd(const float2& a, const float2& b, const float2& c)
    {
        const float x = a.x + b.x * c.x;
        const float y = a.y + b.y * c.y;
        return float2(x, y);
    }

    static inline float2 lerp(const float2& a, const float2& b, float factor)
    {
        const float x = a.x + (b.x - a.x) * factor;
        const float y = a.y + (b.y - a.y) * factor;
        return float2(x, y);
    }

    static inline float2 lerp(const float2& a, const float2& b, const float2& factor)
    {
        const float x = a.x + (b.x - a.x) * factor.x;
        const float y = a.y + (b.y - a.y) * factor.y;
        return float2(x, y);
    }

    static inline float2 hmin(const float2& v)
    {
        const float s = std::min(v.x, v.y);
        return float2(s);
    }

    static inline float2 hmax(const float2& v)
    {
        const float s = std::max(v.x, v.y);
        return float2(s);
    }

} // namespace mango
