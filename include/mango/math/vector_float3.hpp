/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"
#include "vector_float2.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // Vector<float, 3>
    // ------------------------------------------------------------------

    template <>
    struct Vector<float, 3> : VectorBase<float, 3>
    {
        template <int X, int Y>
        struct Permute2
        {
			float v[3];

			operator Vector<float, 2> () const
			{
				return Vector<float, 2>(v[X], v[Y]);
			}
        };

        template <int X, int Y, int Z>
        struct Permute3
        {
			float v[3];

			operator Vector<float, 3> () const
			{
				return Vector<float, 3>(v[X], v[Y], v[Z]);
			}
        };

        union
        {
            struct { float x, y, z; };
			Permute2<0, 0> xx;
			Permute2<1, 0> yx;
			Permute2<2, 0> zx;
			Permute2<0, 1> xy;
			Permute2<1, 1> yy;
			Permute2<2, 1> zy;
			Permute2<0, 2> xz;
			Permute2<1, 2> yz;
			Permute2<2, 2> zz;
            Permute3<0, 0, 0> xxx;
            Permute3<1, 0, 0> yxx;
            Permute3<2, 0, 0> zxx;
            Permute3<0, 1, 0> xyx;
            Permute3<1, 1, 0> yyx;
            Permute3<2, 1, 0> zyx;
            Permute3<0, 2, 0> xzx;
            Permute3<1, 2, 0> yzx;
            Permute3<2, 2, 0> zzx;
            Permute3<0, 0, 1> xxy;
            Permute3<1, 0, 1> yxy;
            Permute3<2, 0, 1> zxy;
            Permute3<0, 1, 1> xyy;
            Permute3<1, 1, 1> yyy;
            Permute3<2, 1, 1> zyy;
            Permute3<0, 2, 1> xzy;
            Permute3<1, 2, 1> yzy;
            Permute3<2, 2, 1> zzy;
            Permute3<0, 0, 2> xxz;
            Permute3<1, 0, 2> yxz;
            Permute3<2, 0, 2> zxz;
            Permute3<0, 1, 2> xyz;
            Permute3<1, 1, 2> yyz;
            Permute3<2, 1, 2> zyz;
            Permute3<0, 2, 2> xzz;
            Permute3<1, 2, 2> yzz;
            Permute3<2, 2, 2> zzz;
        };

        explicit Vector()
        {
        }

        explicit Vector(float s)
        {
			x = s;
			y = s;
			z = s;
        }

        explicit Vector(int s)
        {
            const float f = float(s);
			x = f;
			y = f;
			x = f;
        }

        explicit Vector(float s0, float s1, float s2)
        {
			x = s0;
			y = s1;
			z = s2;
        }

		explicit Vector(const Vector<float, 2>& v, float s)
		{
			x = v.x;
			y = v.y;
			z = s;
		}

		explicit Vector(float s, const Vector<float, 2>& v)
		{
			x = s;
			y = v.x;
			z = v.y;
		}

        Vector(const Vector& v)
        {
			x = v.x;
			y = v.y;
			z = v.z;
        }

        template <int X, int Y, int Z>
        Vector(const Permute3<X, Y, Z>& p)
        {
			const float* v = p.v;
			x = v[X];
			y = v[Y];
			z = v[Z];
        }

        ~Vector()
        {
        }

        template <int X, int Y, int Z>
        Vector& operator = (const Permute3<X, Y, Z>& p)
        {
			const float* v = p.v;
			x = v[X];
			y = v[Y];
			z = v[Z];
            return *this;
        }

        Vector& operator = (float s)
        {
			x = s;
			y = s;
			z = s;
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
			x = v.x;
			y = v.y;
			z = v.z;
            return *this;
        }
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline const Vector<float, 3>& operator + (const Vector<float, 3>& v)
    {
        return v;
    }

    static inline Vector<float, 3> operator - (const Vector<float, 3>& v)
    {
        return Vector<float, 3>(-v.x, -v.y, -v.z);
    }

    static inline Vector<float, 3>& operator += (Vector<float, 3>& a, const Vector<float, 3>& b)
    {
        a.x += b.x;
        a.y += b.y;
        a.z += b.z;
        return a;
    }

    static inline Vector<float, 3>& operator -= (Vector<float, 3>& a, const Vector<float, 3>& b)
    {
        a.x -= b.x;
        a.y -= b.y;
        a.z -= b.z;
        return a;
    }

    static inline Vector<float, 3>& operator *= (Vector<float, 3>& a, const Vector<float, 3>& b)
    {
        a.x *= b.x;
        a.y *= b.y;
        a.z *= b.z;
        return a;
    }

    static inline Vector<float, 3>& operator *= (Vector<float, 3>& a, float b)
    {
        a.x *= b;
        a.y *= b;
        a.z *= b;
        return a;
    }

    static inline Vector<float, 3>& operator /= (Vector<float, 3>& a, const Vector<float, 3>& b)
    {
        a.x /= b.x;
        a.y /= b.y;
        a.z /= b.z;
        return a;
    }

    static inline Vector<float, 3>& operator /= (Vector<float, 3>& a, float b)
    {
        b = 1.0f / b;
        a.x *= b;
        a.y *= b;
        a.z *= b;
        return a;
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<float, 3> clamp(const Vector<float, 3>& a, const Vector<float, 3>& amin, const Vector<float, 3>& amax)
    {
        const float x = std::max(amin.x, std::min(amax.x, a.x));
        const float y = std::max(amin.y, std::min(amax.y, a.y));
        const float z = std::max(amin.z, std::min(amax.z, a.z));
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> madd(const Vector<float, 3>& a, const Vector<float, 3>& b, const Vector<float, 3>& c)
    {
        const float x = a.x + b.x * c.x;
        const float y = a.y + b.y * c.y;
        const float z = a.z + b.z * c.z;
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> lerp(const Vector<float, 3>& a, const Vector<float, 3>& b, float factor)
    {
        const float x = a.x + (b.x - a.x) * factor;
        const float y = a.y + (b.y - a.y) * factor;
        const float z = a.z + (b.z - a.z) * factor;
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> lerp(const Vector<float, 3>& a, const Vector<float, 3>& b, const Vector<float, 3>& factor)
    {
        const float x = a.x + (b.x - a.x) * factor.x;
        const float y = a.y + (b.y - a.y) * factor.y;
        const float z = a.z + (b.z - a.z) * factor.z;
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> hmin(const Vector<float, 3>& v)
    {
        const float s = std::min(std::min(v.x, v.y), v.z);
        return Vector<float, 3>(s);
    }

    static inline Vector<float, 3> hmax(const Vector<float, 3>& v)
    {
        const float s = std::max(std::max(v.x, v.y), v.z);
        return Vector<float, 3>(s);
    }

} // namespace mango
