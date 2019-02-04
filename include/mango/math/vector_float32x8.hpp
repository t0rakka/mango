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

        const ScalarType* data() const
        {
            return reinterpret_cast<const ScalarType *>(component);
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

    static inline Vector<float, 8> operator + (Vector<float, 8> v)
    {
        return v;
    }

    static inline Vector<float, 8> operator - (Vector<float, 8> v)
    {
        return simd::neg(v);
    }

    static inline Vector<float, 8>& operator += (Vector<float, 8>& a, Vector<float, 8> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<float, 8>& operator -= (Vector<float, 8>& a, Vector<float, 8> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<float, 8>& operator *= (Vector<float, 8>& a, Vector<float, 8> b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline Vector<float, 8>& operator /= (Vector<float, 8>& a, Vector<float, 8> b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<float, 8>& operator /= (Vector<float, 8>& a, float b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<float, 8> operator + (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<float, 8> operator - (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<float, 8> operator * (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::mul(a, b);
    }

    static inline Vector<float, 8> operator / (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::div(a, b);
    }

    static inline Vector<float, 8> operator / (Vector<float, 8> a, float b)
    {
        return simd::div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<float, 8> abs(Vector<float, 8> a)
    {
        return simd::abs(a);
    }

    static inline Vector<float, 8> round(Vector<float, 8> a)
    {
        return simd::round(a);
    }

    static inline Vector<float, 8> floor(Vector<float, 8> a)
    {
        return simd::floor(a);
    }

    static inline Vector<float, 8> ceil(Vector<float, 8> a)
    {
        return simd::ceil(a);
    }

    static inline Vector<float, 8> trunc(Vector<float, 8> a)
    {
        return simd::trunc(a);
    }

    static inline Vector<float, 8> fract(Vector<float, 8> a)
    {
        return simd::fract(a);
    }

    static inline Vector<float, 8> sign(Vector<float, 8> a)
    {
        return simd::sign(a);
    }

    static inline Vector<float, 8> radians(Vector<float, 8> a)
    {
        return simd::radians(a);
    }

    static inline Vector<float, 8> degrees(Vector<float, 8> a)
    {
        return simd::degrees(a);
    }

    static inline Vector<float, 8> sqrt(Vector<float, 8> a)
    {
        return simd::sqrt(a);
    }

    static inline Vector<float, 8> rsqrt(Vector<float, 8> a)
    {
        return simd::rsqrt(a);
    }

    static inline Vector<float, 8> rcp(Vector<float, 8> a)
    {
        return simd::rcp(a);
    }

    static inline Vector<float, 8> min(Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<float, 8> max(Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<float, 8> mod(Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::mod(a, b);
    }

    static inline Vector<float, 8> clamp(Vector<float, 8> a, Vector<float, 8> amin, Vector<float, 8> amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline Vector<float, 8> madd(Vector<float, 8> a, Vector<float, 8> b, Vector<float, 8> c)
    {
        return simd::madd(a, b, c);
    }

    static inline Vector<float, 8> msub(Vector<float, 8> a, Vector<float, 8> b, Vector<float, 8> c)
    {
        return simd::msub(a, b, c);
    }

    static inline Vector<float, 8> lerp(Vector<float, 8> a, Vector<float, 8> b, float factor)
    {
        return a + (b - a) * factor;
    }

    static inline Vector<float, 8> lerp(Vector<float, 8> a, Vector<float, 8> b, Vector<float, 8> factor)
    {
        return a + (b - a) * factor;
    }

    // ------------------------------------------------------------------
    // trigonometric functions
    // ------------------------------------------------------------------

    Vector<float, 8> sin(Vector<float, 8> a);
    Vector<float, 8> cos(Vector<float, 8> a);
    Vector<float, 8> tan(Vector<float, 8> a);
    Vector<float, 8> exp(Vector<float, 8> a);
    Vector<float, 8> exp2(Vector<float, 8> a);
    Vector<float, 8> log(Vector<float, 8> a);
    Vector<float, 8> log2(Vector<float, 8> a);
    Vector<float, 8> asin(Vector<float, 8> a);
    Vector<float, 8> acos(Vector<float, 8> a);
    Vector<float, 8> atan(Vector<float, 8> a);
    Vector<float, 8> atan2(Vector<float, 8> a, Vector<float, 8> b);
    Vector<float, 8> pow(Vector<float, 8> a, Vector<float, 8> b);

    // ------------------------------------------------------------------
	// bitwise operators
    // ------------------------------------------------------------------

    static inline Vector<float, 8> nand(Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<float, 8> operator & (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<float, 8> operator | (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<float, 8> operator ^ (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<float, 8> operator ~ (Vector<float, 8> a)
    {
        return simd::bitwise_not(a);
    }

    // ------------------------------------------------------------------
    // compare / select
    // ------------------------------------------------------------------

    static inline mask32x8 operator > (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask32x8 operator >= (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask32x8 operator < (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask32x8 operator <= (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask32x8 operator == (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask32x8 operator != (Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<float, 8> select(mask32x8 mask, Vector<float, 8> a, Vector<float, 8> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
