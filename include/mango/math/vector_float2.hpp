/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        struct Permute
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
            Permute<0, 0> xx;
            Permute<1, 0> yx;
            Permute<0, 1> xy;
            Permute<1, 1> yy;
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
        Vector(const Permute<X, Y>& p)
        {
			const float* v = p.v;
			x = v[X];
			y = v[Y];
        }

        ~Vector()
        {
        }

        template <int X, int Y>
        Vector& operator = (const Permute<X, Y>& p)
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

    static inline const Vector<float, 2>& operator + (const Vector<float, 2>& v)
    {
        return v;
    }

    static inline Vector<float, 2> operator - (const Vector<float, 2>& v)
    {
		return Vector<float, 2>(-v.x, -v.y);
    }

    static inline Vector<float, 2>& operator += (Vector<float, 2>& a, const Vector<float, 2>& b)
    {
		a.x += b.x;
		a.y += b.y;
        return a;
    }

    static inline Vector<float, 2>& operator -= (Vector<float, 2>& a, const Vector<float, 2>& b)
    {
		a.x -= b.x;
		a.y -= b.y;
        return a;
    }

    static inline Vector<float, 2>& operator *= (Vector<float, 2>& a, const Vector<float, 2>& b)
    {
		a.x *= b.x;
		a.y *= b.y;
        return a;
    }

    static inline Vector<float, 2>& operator *= (Vector<float, 2>& a, float b)
    {
		a.x *= b;
		a.y *= b;
        return a;
    }

    static inline Vector<float, 2>& operator /= (Vector<float, 2>& a, const Vector<float, 2>& b)
    {
		a.x /= b.x;
		a.y /= b.y;
        return a;
    }

    static inline Vector<float, 2>& operator /= (Vector<float, 2>& a, float b)
    {
		b = 1.0f / b;
		a.x *= b;
		a.y *= b;
        return a;
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<float, 2> clamp(const Vector<float, 2>& a, const Vector<float, 2>& amin, const Vector<float, 2>& amax)
    {
		const float x = std::max(amin.x, std::min(amax.x, a.x));
		const float y = std::max(amin.y, std::min(amax.y, a.y));
        return Vector<float, 2>(x, y);
    }

    static inline Vector<float, 2> madd(const Vector<float, 2>& a, const Vector<float, 2>& b, const Vector<float, 2>& c)
    {
		const float x = a.x + b.x * c.x;
		const float y = a.y + b.y * c.y;
        return Vector<float, 2>(x, y);
    }

    static inline Vector<float, 2> lerp(const Vector<float, 2>& a, const Vector<float, 2>& b, float factor)
    {
		const float x = a.x + (b.x - a.x) * factor;
		const float y = a.y + (b.y - a.y) * factor;
        return Vector<float, 2>(x, y);
    }

    static inline Vector<float, 2> lerp(const Vector<float, 2>& a, const Vector<float, 2>& b, const Vector<float, 2>& factor)
    {
		const float x = a.x + (b.x - a.x) * factor.x;
		const float y = a.y + (b.y - a.y) * factor.y;
        return Vector<float, 2>(x, y);
    }

    static inline Vector<float, 2> hmin(const Vector<float, 2>& v)
    {
        const float s = std::min(v.x, v.y);
        return Vector<float, 2>(s);
    }

    static inline Vector<float, 2> hmax(const Vector<float, 2>& v)
    {
        const float s = std::max(v.x, v.y);
        return Vector<float, 2>(s);
    }

} // namespace mango
