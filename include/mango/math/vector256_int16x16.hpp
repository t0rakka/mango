/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int16, 16> : VectorBase<int16, 16>
    {
        using VectorType = simd::int16x16;

        simd::int16x16 m;

        explicit Vector() = default;

        Vector(int16 s)
        : m(simd::int16x16_set1(s))
        {
        }

        Vector(simd::int16x16 v)
        : m(v)
        {
        }

        Vector& operator = (simd::int16x16 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (int16 s)
        {
            m = simd::int16x16_set1(s);
            return *this;
        }

        operator simd::int16x16 () const
        {
            return m;
        }

        operator simd::int16x16 ()
        {
            return m;
        }
    };

    static inline const Vector<int16, 16> operator + (Vector<int16, 16> v)
    {
        return v;
    }

    static inline Vector<int16, 16> operator - (Vector<int16, 16> v)
    {
        return simd::sub(simd::int16x16_zero(), v);
    }

    static inline Vector<int16, 16>& operator += (Vector<int16, 16>& a, Vector<int16, 16> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<int16, 16>& operator -= (Vector<int16, 16>& a, Vector<int16, 16> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<int16, 16> operator + (Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int16, 16> operator - (Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int16, 16> nand(Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<int16, 16> operator & (Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<int16, 16> operator | (Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<int16, 16> operator ^ (Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<int16, 16> adds(Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<int16, 16> subs(Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<int16, 16> min(Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<int16, 16> max(Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::max(a, b);
    }

    static inline mask16x16 operator > (Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask16x16 operator < (Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask16x16 operator == (Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<int16, 16> select(mask16x16 mask, Vector<int16, 16> a, Vector<int16, 16> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<int16, 16> operator << (Vector<int16, 16> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<int16, 16> operator >> (Vector<int16, 16> a, int b)
    {
        return simd::sra(a, b);
    }

} // namespace mango
