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
        using VectorType = simd::float64x2;

        union
        {
            simd::float64x2 xy;

            ScalarAccessor<double, simd::float64x2, 0> x;
            ScalarAccessor<double, simd::float64x2, 1> y;

            Permute2<double, simd::float64x2, 0, 0> xx;
            Permute2<double, simd::float64x2, 1, 0> yx;
            Permute2<double, simd::float64x2, 1, 1> yy;
        };

        explicit Vector() = default;

        Vector(double s)
        : xy(simd::float64x2_set1(s))
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
        Vector(const Permute2<double, simd::float64x2, X, Y>& p)
        {
            xy = p;
        }

        template <int X, int Y>
        Vector& operator = (const Permute2<double, simd::float64x2, X, Y>& p)
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

    static inline const double2 operator + (double2 v)
    {
        return v;
    }

    static inline double2 operator - (double2 v)
    {
        return simd::neg(v);
    }

    static inline double2& operator += (double2& a, double2 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline double2& operator -= (double2& a, double2 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline double2& operator *= (double2& a, double2 b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline double2& operator /= (double2& a, double2 b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline double2& operator /= (double2& a, double b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline double2 operator + (double2 a, double2 b)
    {
        return simd::add(a, b);
    }

    static inline double2 operator - (double2 a, double2 b)
    {
        return simd::sub(a, b);
    }

    static inline double2 operator * (double2 a, double2 b)
    {
        return simd::mul(a, b);
    }

    static inline double2 operator / (double2 a, double2 b)
    {
        return simd::div(a, b);
    }

    static inline double2 operator / (double2 a, double b)
    {
        return simd::div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline double2 clamp(double2 a, double2 amin, double2 amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline double2 madd(double2 a, double2 b, double2 c)
    {
        return simd::madd(a, b, c);
    }

    static inline double2 lerp(double2 a, double2 b, double factor)
    {
        return a + (b - a) * factor;
    }

    static inline double2 lerp(double2 a, double2 b, double2 factor)
    {
        return a + (b - a) * factor;
    }

    // ------------------------------------------------------------------
	// bitwise operators
    // ------------------------------------------------------------------

    static inline double2 nand(double2 a, double2 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline double2 operator & (double2 a, double2 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline double2 operator | (double2 a, double2 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline double2 operator ^ (double2 a, double2 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline double2 operator ~ (double2 a)
    {
        return simd::bitwise_not(a);
    }

    // ------------------------------------------------------------------
    // compare / select
    // ------------------------------------------------------------------

    static inline mask64x2 operator > (double2 a, double2 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask64x2 operator >= (double2 a, double2 b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask64x2 operator < (double2 a, double2 b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask64x2 operator <= (double2 a, double2 b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask64x2 operator == (double2 a, double2 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask64x2 operator != (double2 a, double2 b)
    {
        return simd::compare_neq(a, b);
    }

    static inline double2 select(mask64x2 mask, double2 a, double2 b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
