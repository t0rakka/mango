/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"
#include "vector_float32x8.hpp"

namespace mango
{

    template <>
    struct Vector<float, 16> : VectorBase<float, 16>
    {
        using VectorType = simd::float32x16;

        union
        {
            simd::float32x16 m;
            LowAccessor<Vector<float, 8>, simd::float32x16> low;
            HighAccessor<Vector<float, 8>, simd::float32x16> high;
        };

        explicit Vector() = default;

        explicit Vector(float s)
        : m(simd::float32x16_set1(s))
        {
        }

        explicit Vector(int s)
        : m(simd::float32x16_set1(float(s)))
        {
        }

        explicit Vector(float s0, float s1, float s2, float s3, float s4, float s5, float s6, float s7,
            float s8, float s9, float s10, float s11, float s12, float s13, float s14, float s15)
        : m(simd::float32x16_set16(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15))
        {
        }

        Vector(simd::float32x16 v)
        : m(v)
        {
        }

        Vector& operator = (simd::float32x16 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (float s)
        {
            m = simd::float32x16_set1(s);
            return *this;
        }

        operator simd::float32x16 () const
        {
            return m;
        }

        operator simd::float32x16 ()
        {
            return m;
        }
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline float32x16 operator + (float32x16 v)
    {
        return v;
    }

    static inline float32x16 operator - (float32x16 v)
    {
        return simd::neg(v);
    }

    static inline float32x16& operator += (float32x16& a, float32x16 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline float32x16& operator += (float32x16& a, float b)
    {
        a = simd::add(a, simd::float32x16_set1(b));
        return a;
    }

    static inline float32x16& operator -= (float32x16& a, float32x16 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline float32x16& operator -= (float32x16& a, float b)
    {
        a = simd::sub(a, simd::float32x16_set1(b));
        return a;
    }

    static inline float32x16& operator *= (float32x16& a, float32x16 b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline float32x16& operator *= (float32x16& a, float b)
    {
        a = simd::mul(a, simd::float32x16_set1(b));
        return a;
    }

    static inline float32x16& operator /= (float32x16& a, float32x16 b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline float32x16& operator /= (float32x16& a, float b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline float32x16 operator + (float32x16 a, float32x16 b)
    {
        return simd::add(a, b);
    }

    static inline float32x16 operator + (float32x16 a, float b)
    {
        return simd::add(a, simd::float32x16_set1(b));
    }

    static inline float32x16 operator + (float a, float32x16 b)
    {
        return simd::add(simd::float32x16_set1(a), b);
    }

    static inline float32x16 operator - (float32x16 a, float32x16 b)
    {
        return simd::sub(a, b);
    }

    static inline float32x16 operator - (float32x16 a, float b)
    {
        return simd::sub(a, simd::float32x16_set1(b));
    }

    static inline float32x16 operator - (float a, float32x16 b)
    {
        return simd::sub(simd::float32x16_set1(a), b);
    }

    static inline float32x16 operator * (float32x16 a, float32x16 b)
    {
        return simd::mul(a, b);
    }

    static inline float32x16 operator * (float32x16 a, float b)
    {
        return simd::mul(a, simd::float32x16_set1(b));
    }

    static inline float32x16 operator * (float a, float32x16 b)
    {
        return simd::mul(simd::float32x16_set1(a), b);
    }

    static inline float32x16 operator / (float32x16 a, float32x16 b)
    {
        return simd::div(a, b);
    }

    static inline float32x16 operator / (float32x16 a, float b)
    {
        return simd::div(a, b);
    }

    static inline float32x16 operator / (float a, float32x16 b)
    {
        return simd::div(simd::float32x16_set1(a), b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

#define MAKE_VECTOR_FUNCTION1(Name, SimdName) \
    static inline float32x16 Name(float32x16 a) { \
        return SimdName(a); \
    }

#define MAKE_VECTOR_FUNCTION2(Name, SimdName) \
    static inline float32x16 Name(float32x16 a, float32x16 b) { \
        return SimdName(a, b); \
    }

    MAKE_VECTOR_FUNCTION1(abs, simd::abs)
    MAKE_VECTOR_FUNCTION1(round, simd::round)
    MAKE_VECTOR_FUNCTION1(floor, simd::floor)
    MAKE_VECTOR_FUNCTION1(ceil, simd::ceil)
    MAKE_VECTOR_FUNCTION1(trunc, simd::trunc)
    MAKE_VECTOR_FUNCTION1(fract, simd::fract)
#if 0
    MAKE_VECTOR_FUNCTION1(sin, simd::sin)
    MAKE_VECTOR_FUNCTION1(cos, simd::cos)
    MAKE_VECTOR_FUNCTION1(tan, simd::tan)
    MAKE_VECTOR_FUNCTION1(asin, simd::asin)
    MAKE_VECTOR_FUNCTION1(acos, simd::acos)
    MAKE_VECTOR_FUNCTION1(atan, simd::atan)
    MAKE_VECTOR_FUNCTION1(exp, simd::exp)
    MAKE_VECTOR_FUNCTION1(log, simd::log)
    MAKE_VECTOR_FUNCTION1(exp2, simd::exp2)
    MAKE_VECTOR_FUNCTION1(log2, simd::log2)
    MAKE_VECTOR_FUNCTION1(sign, simd::sign)
    MAKE_VECTOR_FUNCTION1(radians, simd::radians)
    MAKE_VECTOR_FUNCTION1(degrees, simd::degrees)
#endif
    MAKE_VECTOR_FUNCTION1(sqrt, simd::sqrt)
    MAKE_VECTOR_FUNCTION1(rsqrt, simd::rsqrt)

    MAKE_VECTOR_FUNCTION2(min, simd::min)
    MAKE_VECTOR_FUNCTION2(max, simd::max)
#if 0
    MAKE_VECTOR_FUNCTION2(mod, simd::mod)
    MAKE_VECTOR_FUNCTION2(pow, simd::pow)
    MAKE_VECTOR_FUNCTION2(atan2, simd::atan2)
#endif

#undef MAKE_VECTOR_FUNCTION1
#undef MAKE_VECTOR_FUNCTION2

    static inline float32x16 clamp(float32x16 a, float32x16 amin, float32x16 amax)
    {
        return simd::max(amin, simd::min(amax, a));
    }

    static inline float32x16 madd(float32x16 a, float32x16 b, float32x16 c)
    {
        return simd::madd(a, b, c);
    }

    static inline float32x16 msub(float32x16 a, float32x16 b, float32x16 c)
    {
        return simd::msub(a, b, c);
    }

    static inline float32x16 lerp(float32x16 a, float32x16 b, float factor)
    {
        return a + (b - a) * factor;
    }

    static inline float32x16 lerp(float32x16 a, float32x16 b, float32x16 factor)
    {
        return a + (b - a) * factor;
    }

    // ------------------------------------------------------------------
	// bitwise operators
    // ------------------------------------------------------------------

    static inline float32x16 nand(float32x16 a, float32x16 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline float32x16 operator & (float32x16 a, float32x16 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline float32x16 operator | (float32x16 a, float32x16 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline float32x16 operator ^ (float32x16 a, float32x16 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline float32x16 operator ~ (float32x16 a)
    {
        return simd::bitwise_not(a);
    }

    // ------------------------------------------------------------------
    // compare / select
    // ------------------------------------------------------------------

    static inline mask32x16 operator > (float32x16 a, float32x16 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask32x16 operator >= (float32x16 a, float32x16 b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask32x16 operator < (float32x16 a, float32x16 b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask32x16 operator <= (float32x16 a, float32x16 b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask32x16 operator == (float32x16 a, float32x16 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask32x16 operator != (float32x16 a, float32x16 b)
    {
        return simd::compare_neq(a, b);
    }

    static inline float32x16 select(mask32x16 mask, float32x16 a, float32x16 b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
