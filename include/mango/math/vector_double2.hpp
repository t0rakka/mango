/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // Vector<double, 2>
    // ------------------------------------------------------------------

    template <>
    struct Vector<double, 2> : VectorBase<double, 2>
    {
        union
        {
            simd::float64x2 xy;

            ScalarAccessor<simd::float64x2, double, 0> x;
            ScalarAccessor<simd::float64x2, double, 1> y;

            Permute2<simd::float64x2, 0, 0> xx;
            Permute2<simd::float64x2, 1, 0> yx;
            Permute2<simd::float64x2, 1, 1> yy;
        };

        explicit Vector() = default;

        explicit Vector(double s)
        : xy(simd::float64x2_set1(s))
        {
        }

        explicit Vector(int s)
        : xy(simd::float64x2_set1(double(s)))
        {
        }

        explicit Vector(double x, double y)
        : xy(simd::float64x2_set2(x, y))
        {
        }

        Vector(simd::float64x2 v)
        : xy(v)
        {
        }

        template <int X, int Y>
        Vector(const Permute2<simd::float64x2, X, Y>& p)
        {
            xy = p;
        }

        template <int X, int Y>
        Vector& operator = (const Permute2<simd::float64x2, X, Y>& p)
        {
            xy = p;
            return *this;
        }

        Vector& operator = (simd::float64x2 v)
        {
            xy = v;
            return *this;
        }

        Vector& operator = (double s)
        {
            xy = simd::float64x2_set1(s);
            return *this;
        }

        operator simd::float64x2 () const
        {
            return xy;
        }

        operator simd::float64x2 ()
        {
            return xy;
        }
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline const Vector<double, 2>& operator + (const Vector<double, 2>& v)
    {
        return v;
    }

    static inline Vector<double, 2> operator - (const Vector<double, 2>& v)
    {
        return simd::neg(v);
    }

    static inline Vector<double, 2>& operator += (Vector<double, 2>& a, const Vector<double, 2>& b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<double, 2>& operator -= (Vector<double, 2>& a, const Vector<double, 2>& b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<double, 2>& operator *= (Vector<double, 2>& a, const Vector<double, 2>& b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline Vector<double, 2>& operator *= (Vector<double, 2>& a, double b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline Vector<double, 2>& operator /= (Vector<double, 2>& a, const Vector<double, 2>& b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<double, 2>& operator /= (Vector<double, 2>& a, double b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<double, 2> operator + (const Vector<double, 2>& a, const Vector<double, 2>& b)
    {
        return simd::add(a, b);
    }

    template <int X, int Y, int A, int B>
    static inline Vector<double, 2> operator + (const Permute2<simd::float64x2, X,Y>& a, const Permute2<simd::float64x2, A,B>& b)
    {
        return simd::add(a, b);
    }

    static inline Vector<double, 2> operator - (const Vector<double, 2>& a, const Vector<double, 2>& b)
    {
        return simd::sub(a, b);
    }

    template <int X, int Y, int A, int B>
    static inline Vector<double, 2> operator - (const Permute2<simd::float64x2, X,Y>& a, const Permute2<simd::float64x2, A,B>& b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<double, 2> operator * (const Vector<double, 2>& a, const Vector<double, 2>& b)
    {
        return simd::mul(a, b);
    }

    static inline Vector<double, 2> operator * (const Vector<double, 2>& a, double b)
    {
        return simd::mul(a, b);
    }

    static inline Vector<double, 2> operator * (double a, const Vector<double, 2>& b)
    {
        return simd::mul(a, b);
    }

    template <int X, int Y, int A, int B>
    static inline Vector<double, 2> operator * (const Permute2<simd::float64x2, X,Y>& a, const Permute2<simd::float64x2, A,B>& b)
    {
        return simd::mul(a, b);
    }

    static inline Vector<double, 2> operator / (const Vector<double, 2>& a, const Vector<double, 2>& b)
    {
        return simd::div(a, b);
    }

    static inline Vector<double, 2> operator / (const Vector<double, 2>& a, double b)
    {
        return simd::div(a, b);
    }

    template <int X, int Y, int A, int B>
    static inline Vector<double, 2> operator / (const Permute2<simd::float64x2, X,Y>& a, const Permute2<simd::float64x2, A,B>& b)
    {
        return simd::div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<double, 2> clamp(const Vector<double, 2>& a, const Vector<double, 2>& amin, const Vector<double, 2>& amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline Vector<double, 2> madd(const Vector<double, 2>& a, const Vector<double, 2>& b, const Vector<double, 2>& c)
    {
        return simd::madd(a, b, c);
    }

    static inline Vector<double, 2> lerp(const Vector<double, 2>& a, const Vector<double, 2>& b, double factor)
    {
        return a + (b - a) * factor;
    }

    static inline Vector<double, 2> lerp(const Vector<double, 2>& a, const Vector<double, 2>& b, const Vector<double, 2>& factor)
    {
        return a + (b - a) * factor;
    }

} // namespace mango
