/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int32, 8> : VectorBase<int32, 8>
    {
        using vector_type = simd::int32x8;
        simd::int32x8 m;

        explicit Vector() = default;

        explicit Vector(int32 s)
        : m(simd::int32x8_set1(s))
        {
        }

        Vector(simd::int32x8 v)
        : m(v)
        {
        }

        Vector& operator = (simd::int32x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (int32 s)
        {
            m = simd::int32x8_set1(s);
            return *this;
        }

        operator simd::int32x8 () const
        {
            return m;
        }

        operator simd::int32x8 ()
        {
            return m;
        }
    };

    static inline const Vector<int32, 8> operator + (Vector<int32, 8> v)
    {
        return v;
    }

    static inline Vector<int32, 8> operator - (Vector<int32, 8> v)
    {
        return simd::sub(simd::int32x8_zero(), v);
    }

    static inline Vector<int32, 8>& operator += (Vector<int32, 8>& a, Vector<int32, 8> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<int32, 8>& operator += (Vector<int32, 8>& a, int32 b)
    {
        a = simd::add(a, simd::int32x8_set1(b));
        return a;
    }

    static inline Vector<int32, 8>& operator -= (Vector<int32, 8>& a, Vector<int32, 8> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<int32, 8>& operator -= (Vector<int32, 8>& a, int32 b)
    {
        a = simd::sub(a, simd::int32x8_set1(b));
        return a;
    }

    static inline Vector<int32, 8> operator + (Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int32, 8> operator + (Vector<int32, 8> a, int32 b)
    {
        return simd::add(a, simd::int32x8_set1(b));
    }

    static inline Vector<int32, 8> operator + (int32 a, Vector<int32, 8> b)
    {
        return simd::add(simd::int32x8_set1(a), b);
    }

    static inline Vector<int32, 8> operator - (Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int32, 8> operator - (Vector<int32, 8> a, int32 b)
    {
        return simd::sub(a, simd::int32x8_set1(b));
    }

    static inline Vector<int32, 8> operator - (int32 a, Vector<int32, 8> b)
    {
        return simd::sub(simd::int32x8_set1(a), b);
    }

    static inline Vector<int32, 8> nand(Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<int32, 8> operator & (Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<int32, 8> operator | (Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<int32, 8> operator ^ (Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<int32, 8> adds(Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<int32, 8> subs(Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<int32, 8> min(Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<int32, 8> max(Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<int32, 8> operator > (Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline Vector<int32, 8> operator == (Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<int32, 8> select(Vector<int32, 8> mask, Vector<int32, 8> a, Vector<int32, 8> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<int32, 8> operator << (Vector<int32, 8> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<int32, 8> operator >> (Vector<int32, 8> a, int b)
    {
        return simd::sra(a, b);
    }

} // namespace mango
