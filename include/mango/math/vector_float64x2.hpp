/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        using VectorType = simd::f64x2;
        using ScalarType = double;
        enum { VectorSize = 2 };

        union
        {
            simd::f64x2 m;

            ScalarAccessor<double, simd::f64x2, 0> x;
            ScalarAccessor<double, simd::f64x2, 1> y;

            ShuffleAccessor2<double, simd::f64x2, 0, 0> xx;
            ShuffleAccessor2<double, simd::f64x2, 0, 1> xy;
            ShuffleAccessor2<double, simd::f64x2, 1, 0> yx;
            ShuffleAccessor2<double, simd::f64x2, 1, 1> yy;
        };

        ScalarType& operator [] (size_t index)
        {
            assert(index < VectorSize);
            return data()[index];
        }

        ScalarType operator [] (size_t index) const
        {
            assert(index < VectorSize);
            return data()[index];
        }

        ScalarType* data()
        {
            return reinterpret_cast<ScalarType *>(this);
        }

        const ScalarType* data() const
        {
            return reinterpret_cast<const ScalarType *>(this);
        }

        explicit Vector() {}

        Vector(double s)
            : m(simd::f64x2_set1(s))
        {
        }

        explicit Vector(double x, double y)
            : m(simd::f64x2_set2(x, y))
        {
        }

        Vector(simd::f64x2 v)
            : m(v)
        {
        }

        template <int X, int Y>
        Vector(const ShuffleAccessor2<double, simd::f64x2, X, Y>& p)
        {
            m = p;
        }

        template <int X, int Y>
        Vector& operator = (const ShuffleAccessor2<double, simd::f64x2, X, Y>& p)
        {
            m = p;
            return *this;
        }

        template <typename T, int I>
        Vector& operator = (const ScalarAccessor<ScalarType, T, I>& accessor)
        {
            *this = ScalarType(accessor);
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
            m = v.m;
            return *this;
        }

        Vector& operator = (simd::f64x2 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (double s)
        {
            m = simd::f64x2_set1(s);
            return *this;
        }

        operator simd::f64x2 () const
        {
            return m;
        }

#ifdef float128_is_hardware_vector
        operator simd::f64x2::vector () const
        {
            return m.data;
        }
#endif
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline const Vector<double, 2> operator + (Vector<double, 2> v)
    {
        return v;
    }

    static inline Vector<double, 2> operator - (Vector<double, 2> v)
    {
        return simd::neg(v);
    }

    static inline Vector<double, 2>& operator += (Vector<double, 2>& a, Vector<double, 2> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<double, 2>& operator -= (Vector<double, 2>& a, Vector<double, 2> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<double, 2>& operator *= (Vector<double, 2>& a, Vector<double, 2> b)
    {
        a = simd::mul(a, b);
        return a;
    }

    template <typename VT, int I>
    static inline Vector<double, 2>& operator /= (Vector<double, 2>& a, ScalarAccessor<double, VT, I> b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<double, 2>& operator /= (Vector<double, 2>& a, Vector<double, 2> b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<double, 2>& operator /= (Vector<double, 2>& a, double b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<double, 2> operator + (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<double, 2> operator - (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<double, 2> operator * (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::mul(a, b);
    }

    template <typename VT, int I>
    static inline Vector<double, 2> operator / (Vector<double, 2> a, ScalarAccessor<double, VT, I> b)
    {
        return simd::div(a, b);
    }

    static inline Vector<double, 2> operator / (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::div(a, b);
    }

    static inline Vector<double, 2> operator / (Vector<double, 2> a, double b)
    {
        return simd::div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<double, 2> abs(Vector<double, 2> a)
    {
        return simd::abs(a);
    }

    static inline double square(Vector<double, 2> a)
    {
        return simd::square(a);
    }

    static inline double length(Vector<double, 2> a)
    {
        return simd::length(a);
    }

    static inline Vector<double, 2> normalize(Vector<double, 2> a)
    {
        return simd::normalize(a);
    }

    static inline Vector<double, 2> sqrt(Vector<double, 2> a)
    {
        return simd::sqrt(a);
    }

    static inline Vector<double, 2> rsqrt(Vector<double, 2> a)
    {
        return simd::rsqrt(a);
    }

    static inline Vector<double, 2> rcp(Vector<double, 2> a)
    {
        return simd::rcp(a);
    }

    static inline Vector<double, 2> round(Vector<double, 2> a)
    {
        return simd::round(a);
    }

    static inline Vector<double, 2> trunc(Vector<double, 2> a)
    {
        return simd::trunc(a);
    }

    static inline Vector<double, 2> floor(Vector<double, 2> a)
    {
        return simd::floor(a);
    }

    static inline Vector<double, 2> ceil(Vector<double, 2> a)
    {
        return simd::ceil(a);
    }

    static inline Vector<double, 2> fract(Vector<double, 2> a)
    {
        return simd::fract(a);
    }

    static inline double dot(Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::dot2(a, b);
    }

    static inline Vector<double, 2> min(Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<double, 2> max(Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<double, 2> clamp(Vector<double, 2> a, Vector<double, 2> amin, Vector<double, 2> amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline Vector<double, 2> madd(Vector<double, 2> a, Vector<double, 2> b, Vector<double, 2> c)
    {
        return simd::madd(a, b, c);
    }

    static inline Vector<double, 2> msub(Vector<double, 2> a, Vector<double, 2> b, Vector<double, 2> c)
    {
        return simd::msub(a, b, c);
    }

    static inline Vector<double, 2> lerp(Vector<double, 2> a, Vector<double, 2> b, double factor)
    {
        return a + (b - a) * factor;
    }

    static inline Vector<double, 2> lerp(Vector<double, 2> a, Vector<double, 2> b, Vector<double, 2> factor)
    {
        return a + (b - a) * factor;
    }

    template <int x, int y>
    static inline Vector<double, 2> shuffle(Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::shuffle<x, y>(a, b);
    }

    // ------------------------------------------------------------------
	// bitwise operators
    // ------------------------------------------------------------------

    static inline Vector<double, 2> nand(Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<double, 2> operator & (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<double, 2> operator | (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<double, 2> operator ^ (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<double, 2> operator ~ (Vector<double, 2> a)
    {
        return simd::bitwise_not(a);
    }

    // ------------------------------------------------------------------
    // compare / select
    // ------------------------------------------------------------------

    static inline mask64x2 operator > (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask64x2 operator >= (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask64x2 operator < (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask64x2 operator <= (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask64x2 operator == (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask64x2 operator != (Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<double, 2> select(mask64x2 mask, Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
