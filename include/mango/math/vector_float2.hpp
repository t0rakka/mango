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
        struct Permute2
        {
            simd::float32x2 m;

			operator Vector<float, 2> () const
			{
                return simd::shuffle<X, Y>(m);
			}
        };

        union
        {
            simd::float32x2 m;

            ScalarAccessor<simd::float32x2, float, 0> x;
            ScalarAccessor<simd::float32x2, float, 1> y;

            Permute2<0, 0> xx;
            Permute2<1, 0> yx;
            Permute2<0, 1> xy;
            Permute2<1, 1> yy;
        };

        explicit Vector() = default;

        explicit Vector(float s)
        : m(simd::float32x2_set1(s))
        {
        }

        explicit Vector(int s)
        : m(simd::float32x2_set1(float(s)))
        {
        }

        explicit Vector(float x, float y)
        : m(simd::float32x2_set2(x, y))
        {
        }

        Vector(simd::float32x2 v)
        : m(v)
        {
        }

        template <int X, int Y>
        Vector(const Permute2<X, Y>& p)
        {
            m = p;
        }

        template <int X, int Y>
        Vector& operator = (const Permute2<X, Y>& p)
        {
            m = p;
            return *this;
        }

        Vector& operator = (simd::float32x2 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (float s)
        {
            m = simd::float32x2_set1(s);
            return *this;
        }

        operator simd::float32x2 () const
        {
            return m;
        }

        operator simd::float32x2 ()
        {
            return m;
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
        return simd::neg(v);
    }

    static inline Vector<float, 2>& operator += (Vector<float, 2>& a, const Vector<float, 2>& b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<float, 2>& operator -= (Vector<float, 2>& a, const Vector<float, 2>& b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<float, 2>& operator *= (Vector<float, 2>& a, const Vector<float, 2>& b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline Vector<float, 2>& operator *= (Vector<float, 2>& a, float b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline Vector<float, 2>& operator /= (Vector<float, 2>& a, const Vector<float, 2>& b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<float, 2>& operator /= (Vector<float, 2>& a, float b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<float, 2> operator + (const Vector<float, 2>& a, const Vector<float, 2>& b)
    {
        return simd::add(a, b);
    }

    template <int X, int Y, int A, int B>
    static inline Vector<float, 2> operator + (const Vector<float, 2>::Permute2<X,Y>& a, const Vector<float, 2>::Permute2<A,B>& b)
    {
        return simd::add(a, b);
    }

    static inline Vector<float, 2> operator - (const Vector<float, 2>& a, const Vector<float, 2>& b)
    {
        return simd::sub(a, b);
    }

    template <int X, int Y, int A, int B>
    static inline Vector<float, 2> operator - (const Vector<float, 2>::Permute2<X,Y>& a, const Vector<float, 2>::Permute2<A,B>& b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<float, 2> operator * (const Vector<float, 2>& a, const Vector<float, 2>& b)
    {
        return simd::mul(a, b);
    }

    static inline Vector<float, 2> operator * (const Vector<float, 2>& a, float b)
    {
        return simd::mul(a, b);
    }

    static inline Vector<float, 2> operator * (float a, const Vector<float, 2>& b)
    {
        return simd::mul(a, b);
    }

    template <int X, int Y, int A, int B>
    static inline Vector<float, 2> operator * (const Vector<float, 2>::Permute2<X,Y>& a, const Vector<float, 2>::Permute2<A,B>& b)
    {
        return simd::mul(a, b);
    }

    static inline Vector<float, 2> operator / (const Vector<float, 2>& a, const Vector<float, 2>& b)
    {
        return simd::div(a, b);
    }

    static inline Vector<float, 2> operator / (const Vector<float, 2>& a, float b)
    {
        return simd::div(a, b);
    }

    template <int X, int Y, int A, int B>
    static inline Vector<float, 2> operator / (const Vector<float, 2>::Permute2<X,Y>& a, const Vector<float, 2>::Permute2<A,B>& b)
    {
        return simd::div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<float, 2> clamp(const Vector<float, 2>& a, const Vector<float, 2>& amin, const Vector<float, 2>& amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline Vector<float, 2> madd(const Vector<float, 2>& a, const Vector<float, 2>& b, const Vector<float, 2>& c)
    {
        return simd::madd(a, b, c);
    }

    static inline Vector<float, 2> lerp(const Vector<float, 2>& a, const Vector<float, 2>& b, float factor)
    {
        return a + (b - a) * factor;
    }

    static inline Vector<float, 2> lerp(const Vector<float, 2>& a, const Vector<float, 2>& b, const Vector<float, 2>& factor)
    {
        return a + (b - a) * factor;
    }

} // namespace mango
