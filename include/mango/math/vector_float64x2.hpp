/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // Vector<double, 2>
    // ------------------------------------------------------------------

    template <>
    struct Vector<double, 2>
    {
        using VectorType = simd::float64x2;
        using ScalarType = double;
        enum { VectorSize = 2 };

        union
        {
            simd::float64x2 xy;

            ScalarAccessor<double, simd::float64x2, 0> x;
            ScalarAccessor<double, simd::float64x2, 1> y;

            Permute2<double, simd::float64x2, 0, 0> xx;
            Permute2<double, simd::float64x2, 1, 0> yx;
            Permute2<double, simd::float64x2, 1, 1> yy;

            DeAggregate<ScalarType> component[VectorSize];
        };

        ScalarType& operator [] (size_t index)
        {
            assert(index < VectorSize);
            return component[index].data;
        }

        ScalarType operator [] (size_t index) const
        {
            assert(index < VectorSize);
            return component[index].data;
        }

        explicit Vector() {}
        ~Vector() {}

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

    static inline const float64x2 operator + (float64x2 v)
    {
        return v;
    }

    static inline float64x2 operator - (float64x2 v)
    {
        return simd::neg(v);
    }

    static inline float64x2& operator += (float64x2& a, float64x2 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline float64x2& operator -= (float64x2& a, float64x2 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline float64x2& operator *= (float64x2& a, float64x2 b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline float64x2& operator /= (float64x2& a, float64x2 b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline float64x2& operator /= (float64x2& a, double b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline float64x2 operator + (float64x2 a, float64x2 b)
    {
        return simd::add(a, b);
    }

    static inline float64x2 operator - (float64x2 a, float64x2 b)
    {
        return simd::sub(a, b);
    }

    static inline float64x2 operator * (float64x2 a, float64x2 b)
    {
        return simd::mul(a, b);
    }

    static inline float64x2 operator / (float64x2 a, float64x2 b)
    {
        return simd::div(a, b);
    }

    static inline float64x2 operator / (float64x2 a, double b)
    {
        return simd::div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline float64x2 abs(float64x2 a)
    {
        return simd::abs(a);
    }

    static inline float64x2 sqrt(float64x2 a)
    {
        return simd::sqrt(a);
    }

    static inline float64x2 rsqrt(float64x2 a)
    {
        return simd::rsqrt(a);
    }

    static inline float64x2 rcp(float64x2 a)
    {
        return simd::rcp(a);
    }

    static inline float64x2 round(float64x2 a)
    {
        return simd::round(a);
    }

    static inline float64x2 trunc(float64x2 a)
    {
        return simd::trunc(a);
    }

    static inline float64x2 floor(float64x2 a)
    {
        return simd::floor(a);
    }

    static inline float64x2 ceil(float64x2 a)
    {
        return simd::ceil(a);
    }

    static inline float64x2 fract(float64x2 a)
    {
        return simd::fract(a);
    }

    static inline float64x2 dot(float64x2 a, float64x2 b)
    {
        return simd::dot2(a, b);
    }

    static inline float64x2 min(float64x2 a, float64x2 b)
    {
        return simd::min(a, b);
    }

    static inline float64x2 max(float64x2 a, float64x2 b)
    {
        return simd::min(a, b);
    }

    static inline float64x2 clamp(float64x2 a, float64x2 amin, float64x2 amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline float64x2 madd(float64x2 a, float64x2 b, float64x2 c)
    {
        return simd::madd(a, b, c);
    }

    static inline float64x2 msub(float64x2 a, float64x2 b, float64x2 c)
    {
        return simd::msub(a, b, c);
    }

    static inline float64x2 lerp(float64x2 a, float64x2 b, double factor)
    {
        return a + (b - a) * factor;
    }

    static inline float64x2 lerp(float64x2 a, float64x2 b, float64x2 factor)
    {
        return a + (b - a) * factor;
    }

    // ------------------------------------------------------------------
	// bitwise operators
    // ------------------------------------------------------------------

    static inline float64x2 nand(float64x2 a, float64x2 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline float64x2 operator & (float64x2 a, float64x2 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline float64x2 operator | (float64x2 a, float64x2 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline float64x2 operator ^ (float64x2 a, float64x2 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline float64x2 operator ~ (float64x2 a)
    {
        return simd::bitwise_not(a);
    }

    // ------------------------------------------------------------------
    // compare / select
    // ------------------------------------------------------------------

    static inline mask64x2 operator > (float64x2 a, float64x2 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask64x2 operator >= (float64x2 a, float64x2 b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask64x2 operator < (float64x2 a, float64x2 b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask64x2 operator <= (float64x2 a, float64x2 b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask64x2 operator == (float64x2 a, float64x2 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask64x2 operator != (float64x2 a, float64x2 b)
    {
        return simd::compare_neq(a, b);
    }

    static inline float64x2 select(mask64x2 mask, float64x2 a, float64x2 b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
