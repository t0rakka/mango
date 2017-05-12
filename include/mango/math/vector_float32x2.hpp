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
        using vector_type = simd::float32x2;
        union
        {
            simd::float32x2 xy;

            ScalarAccessor<float, simd::float32x2, 0> x;
            ScalarAccessor<float, simd::float32x2, 1> y;

            Permute2<float, simd::float32x2, 0, 0> xx;
            Permute2<float, simd::float32x2, 1, 0> yx;
            Permute2<float, simd::float32x2, 1, 1> yy;
        };

        explicit Vector() = default;

        explicit Vector(float s)
        : xy(simd::float32x2_set1(s))
        {
        }

        explicit Vector(int s)
        : xy(simd::float32x2_set1(float(s)))
        {
        }

        explicit Vector(float x, float y)
        : xy(simd::float32x2_set2(x, y))
        {
        }

        Vector(simd::float32x2 v)
        : xy(v)
        {
        }

        template <int X, int Y>
        Vector(const Permute2<float, simd::float32x2, X, Y>& p)
        {
            xy = p;
        }

        template <int X, int Y>
        Vector& operator = (const Permute2<float, simd::float32x2, X, Y>& p)
        {
            xy = p;
            return *this;
        }

        Vector& operator = (simd::float32x2 v)
        {
            xy = v;
            return *this;
        }

        Vector& operator = (float s)
        {
            xy = simd::float32x2_set1(s);
            return *this;
        }

        operator simd::float32x2 () const
        {
            return xy;
        }

        operator simd::float32x2 ()
        {
            return xy;
        }
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline const float2 operator + (float2 v)
    {
        return v;
    }

    static inline float2 operator - (float2 v)
    {
        return simd::neg(v);
    }

    static inline float2& operator += (float2& a, float2 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline float2& operator += (float2& a, float b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline float2& operator -= (float2& a, float2 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline float2& operator -= (float2& a, float b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline float2& operator *= (float2& a, float2 b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline float2& operator *= (float2& a, float b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline float2& operator /= (float2& a, float2 b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline float2& operator /= (float2& a, float b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline float2 operator + (float2 a, float2 b)
    {
        return simd::add(a, b);
    }

    static inline float2 operator + (float2 a, float b)
    {
        return simd::add(a, b);
    }

    static inline float2 operator + (float a, float2 b)
    {
        return simd::add(a, b);
    }

    static inline float2 operator - (float2 a, float2 b)
    {
        return simd::sub(a, b);
    }

    static inline float2 operator - (float2 a, float b)
    {
        return simd::sub(a, b);
    }

    static inline float2 operator - (float a, float2 b)
    {
        return simd::sub(a, b);
    }

    static inline float2 operator * (float2 a, float2 b)
    {
        return simd::mul(a, b);
    }

    static inline float2 operator * (float2 a, float b)
    {
        return simd::mul(a, b);
    }

    static inline float2 operator * (float a, float2 b)
    {
        return simd::mul(a, b);
    }

    static inline float2 operator / (float2 a, float2 b)
    {
        return simd::div(a, b);
    }

    static inline float2 operator / (float2 a, float b)
    {
        return simd::div(a, b);
    }

    static inline float2 operator / (float a, float2 b)
    {
        return simd::div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline float2 clamp(float2 a, float2 amin, float2 amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline float2 madd(float2 a, float2 b, float2 c)
    {
        return simd::madd(a, b, c);
    }

    static inline float2 lerp(float2 a, float2 b, float factor)
    {
        return a + (b - a) * factor;
    }

    static inline float2 lerp(float2 a, float2 b, float2 factor)
    {
        return a + (b - a) * factor;
    }

    // ------------------------------------------------------------------
	// bitwise operators
    // ------------------------------------------------------------------

    static inline float2 nand(float2 a, float2 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline float2 operator & (float2 a, float2 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline float2 operator | (float2 a, float2 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline float2 operator ^ (float2 a, float2 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline float2 operator ~ (float2 a)
    {
        return simd::bitwise_not(a);
    }

    // ------------------------------------------------------------------
    // compare / select
    // ------------------------------------------------------------------

    static inline float2 operator > (float2 a, float2 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline float2 operator >= (float2 a, float2 b)
    {
        return simd::compare_ge(a, b);
    }

    static inline float2 operator < (float2 a, float2 b)
    {
        return simd::compare_lt(a, b);
    }

    static inline float2 operator <= (float2 a, float2 b)
    {
        return simd::compare_le(a, b);
    }

    static inline float2 operator == (float2 a, float2 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline float2 operator != (float2 a, float2 b)
    {
        return simd::compare_neq(a, b);
    }

    static inline float2 select(float2 mask, float2 a, float2 b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
