/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"
#include "vector_float64x2.hpp"

namespace mango
{

    template <>
    struct Vector<double, 8> : VectorBase<double, 8>
    {
        using VectorType = simd::float64x8;

        union
        {
            simd::float64x8 xyzw;
            LowAccessor<Vector<double, 4>, simd::float64x8> low;
            HighAccessor<Vector<double, 4>, simd::float64x8> high;
        };

        explicit Vector()
        {
        }

        Vector(double s)
        : xyzw(simd::float64x8_set1(s))
        {
        }

        explicit Vector(double s0, double s1, double s2, double s3, double s4, double s5, double s6, double s7)
        : xyzw(simd::float64x8_set8(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::float64x8 v)
        : xyzw(v)
        {
        }

        Vector& operator = (simd::float64x8 v)
        {
            xyzw = v;
            return *this;
        }

        Vector& operator = (double s)
        {
            xyzw = simd::float64x8_set1(s);
            return *this;
        }

        operator simd::float64x8 () const
        {
            return xyzw;
        }

        operator simd::float64x8 ()
        {
            return xyzw;
        }
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline float64x8 operator + (float64x8 v)
    {
        return v;
    }

    static inline float64x8 operator - (float64x8 v)
    {
        return simd::neg(v);
    }

    static inline float64x8& operator += (float64x8& a, float64x8 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline float64x8& operator -= (float64x8& a, float64x8 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline float64x8& operator *= (float64x8& a, float64x8 b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline float64x8& operator /= (float64x8& a, float64x8 b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline float64x8& operator /= (float64x8& a, double b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline float64x8 operator + (float64x8 a, float64x8 b)
    {
        return simd::add(a, b);
    }

    static inline float64x8 operator - (float64x8 a, float64x8 b)
    {
        return simd::sub(a, b);
    }

    static inline float64x8 operator * (float64x8 a, float64x8 b)
    {
        return simd::mul(a, b);
    }

    static inline float64x8 operator / (float64x8 a, float64x8 b)
    {
        return simd::div(a, b);
    }

    static inline float64x8 operator / (float64x8 a, double b)
    {
        return simd::div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline float64x8 abs(float64x8 a)
    {
        return simd::abs(a);
    }

    static inline float64x8 round(float64x8 a)
    {
        return simd::round(a);
    }

    static inline float64x8 floor(float64x8 a)
    {
        return simd::floor(a);
    }

    static inline float64x8 ceil(float64x8 a)
    {
        return simd::ceil(a);
    }

    static inline float64x8 trunc(float64x8 a)
    {
        return simd::trunc(a);
    }

    static inline float64x8 fract(float64x8 a)
    {
        return simd::fract(a);
    }

    static inline float64x8 sqrt(float64x8 a)
    {
        return simd::sqrt(a);
    }

    static inline float64x8 rsqrt(float64x8 a)
    {
        return simd::rsqrt(a);
    }

    static inline float64x8 rcp(float64x8 a)
    {
        return simd::rcp(a);
    }

    static inline float64x8 min(float64x8 a, float64x8 b)
    {
        return simd::min(a, b);
    }

    static inline float64x8 max(float64x8 a, float64x8 b)
    {
        return simd::max(a, b);
    }

    static inline float64x8 clamp(float64x8 a, float64x8 amin, float64x8 amax)
    {
        return simd::max(amin, simd::min(amax, a));
    }

    static inline float64x8 madd(float64x8 a, float64x8 b, float64x8 c)
    {
        return simd::madd(a, b, c);
    }

    static inline float64x8 msub(float64x8 a, float64x8 b, float64x8 c)
    {
        return simd::msub(a, b, c);
    }

    static inline float64x8 lerp(float64x8 a, float64x8 b, double factor)
    {
        return a + (b - a) * factor;
    }

    static inline float64x8 lerp(float64x8 a, float64x8 b, float64x8 factor)
    {
        return a + (b - a) * factor;
    }

    // ------------------------------------------------------------------
	// bitwise operators
    // ------------------------------------------------------------------

    static inline float64x8 nand(float64x8 a, float64x8 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline float64x8 operator & (float64x8 a, float64x8 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline float64x8 operator | (float64x8 a, float64x8 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline float64x8 operator ^ (float64x8 a, float64x8 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline float64x8 operator ~ (float64x8 a)
    {
        return simd::bitwise_not(a);
    }

    // ------------------------------------------------------------------
	// compare / select
    // ------------------------------------------------------------------

    static inline mask64x8 operator > (float64x8 a, float64x8 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask64x8 operator >= (float64x8 a, float64x8 b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask64x8 operator < (float64x8 a, float64x8 b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask64x8 operator <= (float64x8 a, float64x8 b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask64x8 operator == (float64x8 a, float64x8 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask64x8 operator != (float64x8 a, float64x8 b)
    {
        return simd::compare_neq(a, b);
    }

    static inline float64x8 select(mask64x8 mask, float64x8 a, float64x8 b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
