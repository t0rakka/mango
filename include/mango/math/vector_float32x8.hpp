/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"
#include "vector_float32x4.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // Vector<float, 8>
    // ------------------------------------------------------------------

    template <>
    struct Vector<float, 8> : VectorBase<float, 8>
    {
        using VectorType = simd::float32x8;

        union
        {
            simd::float32x8 m;
            LowAccessor<Vector<float, 4>, simd::float32x8> low;
            HighAccessor<Vector<float, 4>, simd::float32x8> high;
        };

        explicit Vector() = default;

        Vector(float s)
        : m(simd::float32x8_set1(s))
        {
        }

        explicit Vector(float s0, float s1, float s2, float s3, float s4, float s5, float s6, float s7)
        : m(simd::float32x8_set8(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        explicit Vector(const Vector<float, 4>& v0, const Vector<float, 4>& v1)
        : m(simd::combine(v0, v1))
        {
        }

        Vector(simd::float32x8 v)
        : m(v)
        {
        }

        Vector& operator = (simd::float32x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (float s)
        {
            m = simd::float32x8_set1(s);
            return *this;
        }

        operator simd::float32x8 () const
        {
            return m;
        }

        operator simd::float32x8 ()
        {
            return m;
        }
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline float8 operator + (float8 v)
    {
        return v;
    }

    static inline float8 operator - (float8 v)
    {
        return simd::neg(v);
    }

    static inline float8& operator += (float8& a, float8 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline float8& operator -= (float8& a, float8 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline float8& operator *= (float8& a, float8 b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline float8& operator /= (float8& a, float8 b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline float8& operator /= (float8& a, float b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline float8 operator + (float8 a, float8 b)
    {
        return simd::add(a, b);
    }

    static inline float8 operator - (float8 a, float8 b)
    {
        return simd::sub(a, b);
    }

    static inline float8 operator * (float8 a, float8 b)
    {
        return simd::mul(a, b);
    }

    static inline float8 operator / (float8 a, float8 b)
    {
        return simd::div(a, b);
    }

    static inline float8 operator / (float8 a, float b)
    {
        return simd::div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

#define MAKE_VECTOR_FUNCTION1(Name, SimdName) \
    static inline float8 Name(float8 a) { \
        return SimdName(a); \
    }

#define MAKE_VECTOR_FUNCTION2(Name, SimdName) \
    static inline float8 Name(float8 a, float8 b) { \
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
#endif
    MAKE_VECTOR_FUNCTION1(sign, simd::sign)
    MAKE_VECTOR_FUNCTION1(radians, simd::radians)
    MAKE_VECTOR_FUNCTION1(degrees, simd::degrees)
    MAKE_VECTOR_FUNCTION1(sqrt, simd::sqrt)
    MAKE_VECTOR_FUNCTION1(rsqrt, simd::rsqrt)

    MAKE_VECTOR_FUNCTION2(min, simd::min)
    MAKE_VECTOR_FUNCTION2(max, simd::max)
    MAKE_VECTOR_FUNCTION2(mod, simd::mod)
#if 0
    MAKE_VECTOR_FUNCTION2(pow, simd::pow)
    MAKE_VECTOR_FUNCTION2(atan2, simd::atan2)
#endif

#undef MAKE_VECTOR_FUNCTION1
#undef MAKE_VECTOR_FUNCTION2

    static inline float8 clamp(float8 a, float8 amin, float8 amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline float8 madd(float8 a, float8 b, float8 c)
    {
        return simd::madd(a, b, c);
    }

    static inline float8 msub(float8 a, float8 b, float8 c)
    {
        return simd::msub(a, b, c);
    }

    static inline float8 lerp(float8 a, float8 b, float factor)
    {
        return a + (b - a) * factor;
    }

    static inline float8 lerp(float8 a, float8 b, float8 factor)
    {
        return a + (b - a) * factor;
    }

    // ------------------------------------------------------------------
	// bitwise operators
    // ------------------------------------------------------------------

    static inline float8 nand(float8 a, float8 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline float8 operator & (float8 a, float8 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline float8 operator | (float8 a, float8 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline float8 operator ^ (float8 a, float8 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline float8 operator ~ (float8 a)
    {
        return simd::bitwise_not(a);
    }

    // ------------------------------------------------------------------
    // compare / select
    // ------------------------------------------------------------------

    static inline mask32x8 operator > (float8 a, float8 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask32x8 operator >= (float8 a, float8 b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask32x8 operator < (float8 a, float8 b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask32x8 operator <= (float8 a, float8 b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask32x8 operator == (float8 a, float8 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask32x8 operator != (float8 a, float8 b)
    {
        return simd::compare_neq(a, b);
    }

    static inline float8 select(mask32x8 mask, float8 a, float8 b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
