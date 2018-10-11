/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"
#include "vector_float32x8.hpp"

namespace mango
{

    template <>
    struct Vector<float, 16>
    {
        using VectorType = simd::float32x16;
        using ScalarType = float;
        enum { VectorSize = 16 };

        union
        {
            simd::float32x16 m;
            LowAccessor<Vector<float, 8>, simd::float32x16> low;
            HighAccessor<Vector<float, 8>, simd::float32x16> high;
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

        Vector(float s)
            : m(simd::float32x16_set1(s))
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

#ifdef float512_is_hardware_vector
        operator simd::float32x16::vector () const
        {
            return m.data;
        }
#endif
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

    static inline float32x16& operator -= (float32x16& a, float32x16 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline float32x16& operator *= (float32x16& a, float32x16 b)
    {
        a = simd::mul(a, b);
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

    static inline float32x16 operator - (float32x16 a, float32x16 b)
    {
        return simd::sub(a, b);
    }

    static inline float32x16 operator * (float32x16 a, float32x16 b)
    {
        return simd::mul(a, b);
    }

    static inline float32x16 operator / (float32x16 a, float32x16 b)
    {
        return simd::div(a, b);
    }

    static inline float32x16 operator / (float32x16 a, float b)
    {
        return simd::div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline float32x16 abs(float32x16 a)
    {
        return simd::abs(a);
    }

    static inline float32x16 round(float32x16 a)
    {
        return simd::round(a);
    }

    static inline float32x16 floor(float32x16 a)
    {
        return simd::floor(a);
    }

    static inline float32x16 ceil(float32x16 a)
    {
        return simd::ceil(a);
    }

    static inline float32x16 trunc(float32x16 a)
    {
        return simd::trunc(a);
    }

    static inline float32x16 fract(float32x16 a)
    {
        return simd::fract(a);
    }

    static inline float32x16 sqrt(float32x16 a)
    {
        return simd::sqrt(a);
    }

    static inline float32x16 rsqrt(float32x16 a)
    {
        return simd::rsqrt(a);
    }

    static inline float32x16 rcp(float32x16 a)
    {
        return simd::rcp(a);
    }

    static inline float32x16 min(float32x16 a, float32x16 b)
    {
        return simd::min(a, b);
    }

    static inline float32x16 max(float32x16 a, float32x16 b)
    {
        return simd::max(a, b);
    }

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
    // trigonometric functions
    // ------------------------------------------------------------------

    float32x16 sin(float32x16 a);
    float32x16 cos(float32x16 a);
    float32x16 tan(float32x16 a);
    float32x16 exp(float32x16 a);
    float32x16 exp2(float32x16 a);
    float32x16 log(float32x16 a);
    float32x16 log2(float32x16 a);
    float32x16 asin(float32x16 a);
    float32x16 acos(float32x16 a);
    float32x16 atan(float32x16 a);
    float32x16 atan2(float32x16 a, float32x16 b);
    float32x16 pow(float32x16 a, float32x16 b);

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
