/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
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
    struct Vector<float, 8>
    {
        using VectorType = simd::float32x8;
        using ScalarType = float;
        enum { VectorSize = 8 };

        union
        {
            simd::float32x8 m;
            LowAccessor<Vector<float, 4>, simd::float32x8> low;
            HighAccessor<Vector<float, 4>, simd::float32x8> high;
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

#ifdef float256_is_hardware_vector
        operator simd::float32x8::vector () const
        {
            return m.data;
        }
#endif
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

    static inline float8 abs(float8 a)
    {
        return simd::abs(a);
    }

    static inline float8 round(float8 a)
    {
        return simd::round(a);
    }

    static inline float8 floor(float8 a)
    {
        return simd::floor(a);
    }

    static inline float8 ceil(float8 a)
    {
        return simd::ceil(a);
    }

    static inline float8 trunc(float8 a)
    {
        return simd::trunc(a);
    }

    static inline float8 fract(float8 a)
    {
        return simd::fract(a);
    }

    static inline float8 sign(float8 a)
    {
        return simd::sign(a);
    }

    static inline float8 radians(float8 a)
    {
        return simd::radians(a);
    }

    static inline float8 degrees(float8 a)
    {
        return simd::degrees(a);
    }

    static inline float8 sqrt(float8 a)
    {
        return simd::sqrt(a);
    }

    static inline float8 rsqrt(float8 a)
    {
        return simd::rsqrt(a);
    }

    static inline float8 rcp(float8 a)
    {
        return simd::rcp(a);
    }

    static inline float8 min(float8 a, float8 b)
    {
        return simd::min(a, b);
    }

    static inline float8 max(float8 a, float8 b)
    {
        return simd::max(a, b);
    }

    static inline float8 mod(float8 a, float8 b)
    {
        return simd::mod(a, b);
    }

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
    // trigonometric functions
    // ------------------------------------------------------------------

    float32x8 sin(float32x8 a);
    float32x8 cos(float32x8 a);
    float32x8 tan(float32x8 a);
    float32x8 exp(float32x8 a);
    float32x8 exp2(float32x8 a);
    float32x8 log(float32x8 a);
    float32x8 log2(float32x8 a);
    float32x8 asin(float32x8 a);
    float32x8 acos(float32x8 a);
    float32x8 atan(float32x8 a);
    float32x8 atan2(float32x8 a, float32x8 b);
    float32x8 pow(float32x8 a, float32x8 b);

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
