/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int8, 16> : VectorBase<int8, 16>
    {
        using VectorType = simd::int8x16;

        simd::int8x16 m;

        explicit Vector() = default;

        explicit Vector(int8 s)
        : m(simd::int8x16_set1(s))
        {
        }

        Vector(simd::int8x16 v)
        : m(v)
        {
        }

        Vector& operator = (simd::int8x16 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (int8 s)
        {
            m = simd::int8x16_set1(s);
            return *this;
        }

        operator simd::int8x16 () const
        {
            return m;
        }

        operator simd::int8x16 ()
        {
            return m;
        }
    };

    static inline const Vector<int8, 16> operator + (Vector<int8, 16> v)
    {
        return v;
    }

    static inline Vector<int8, 16> operator - (Vector<int8, 16> v)
    {
        return simd::sub(simd::int8x16_zero(), v);
    }

    static inline Vector<int8, 16>& operator += (Vector<int8, 16>& a, Vector<int8, 16> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<int8, 16>& operator += (Vector<int8, 16>& a, int8 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<int8, 16>& operator -= (Vector<int8, 16>& a, Vector<int8, 16> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<int8, 16>& operator -= (Vector<int8, 16>& a, int8 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<int8, 16> operator + (Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int8, 16> operator + (Vector<int8, 16> a, int8 b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int8, 16> operator + (int8 a, Vector<int8, 16> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int8, 16> operator - (Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int8, 16> operator - (Vector<int8, 16> a, int8 b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int8, 16> operator - (int8 a, Vector<int8, 16> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int8, 16> nand(Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<int8, 16> operator & (Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<int8, 16> operator | (Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<int8, 16> operator ^ (Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<int8, 16> operator ~ (Vector<int8, 16> a)
    {
        return simd::bitwise_not(a);
    }

    static inline Vector<int8, 16> abs(Vector<int8, 16> a)
    {
        return simd::abs(a);
    }

    static inline Vector<int8, 16> adds(Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<int8, 16> subs(Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<int8, 16> min(Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<int8, 16> max(Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<int8, 16> clamp(Vector<int8, 16> a, Vector<int8, 16> amin, Vector<int8, 16> amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline mask8x16 operator > (Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask8x16 operator >= (Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask8x16 operator < (Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask8x16 operator <= (Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask8x16 operator == (Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask8x16 operator != (Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<int8, 16> select(mask8x16 mask, Vector<int8, 16> a, Vector<int8, 16> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
