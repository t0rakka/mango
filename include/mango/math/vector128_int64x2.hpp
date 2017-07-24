/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int64, 2> : VectorBase<int64, 2>
    {
        using VectorType = simd::int64x2;
        using Mask = simd::int64x2::mask;

        union
        {
            simd::int64x2 xy;

            ScalarAccessor<int64, simd::int64x2, 0> x;
            ScalarAccessor<int64, simd::int64x2, 1> y;

            Permute2<int64, simd::int64x2, 0, 0> xx;
            Permute2<int64, simd::int64x2, 1, 0> yx;
            Permute2<int64, simd::int64x2, 1, 1> yy;
        };

        explicit Vector() = default;

        explicit Vector(int64 s)
        : xy(simd::int64x2_set1(s))
        {
        }

        explicit Vector(int64 x, int64 y)
        : xy(simd::int64x2_set2(x, y))
        {
        }

        Vector(simd::int64x2 v)
        : xy(v)
        {
        }

        template <int X, int Y>
        Vector(const Permute2<int64, simd::int64x2, X, Y>& p)
        {
            xy = p;
        }

        template <int X, int Y>
        Vector& operator = (const Permute2<int64, simd::int64x2, X, Y>& p)
        {
            xy = p;
            return *this;
        }

        Vector& operator = (simd::int64x2 v)
        {
            xy = v;
            return *this;
        }

        Vector& operator = (int64 s)
        {
            xy = simd::int64x2_set1(s);
            return *this;
        }

        operator simd::int64x2 () const
        {
            return xy;
        }

        operator simd::int64x2 ()
        {
            return xy;
        }
    };

    static inline const Vector<int64, 2> operator + (Vector<int64, 2> v)
    {
        return v;
    }

    static inline Vector<int64, 2> operator - (Vector<int64, 2> v)
    {
        return simd::sub(simd::int64x2_zero(), v);
    }

    static inline Vector<int64, 2>& operator += (Vector<int64, 2>& a, Vector<int64, 2> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<int64, 2>& operator += (Vector<int64, 2>& a, int64 b)
    {
        a = simd::add(a, simd::int64x2_set1(b));
        return a;
    }

    static inline Vector<int64, 2>& operator -= (Vector<int64, 2>& a, Vector<int64, 2> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<int64, 2>& operator -= (Vector<int64, 2>& a, int64 b)
    {
        a = simd::sub(a, simd::int64x2_set1(b));
        return a;
    }

    static inline Vector<int64, 2> operator + (Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int64, 2> operator + (Vector<int64, 2> a, int64 b)
    {
        return simd::add(a, simd::int64x2_set1(b));
    }

    static inline Vector<int64, 2> operator + (int64 a, Vector<int64, 2> b)
    {
        return simd::add(simd::int64x2_set1(a), b);
    }

    static inline Vector<int64, 2> operator - (Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int64, 2> operator - (Vector<int64, 2> a, int64 b)
    {
        return simd::sub(a, simd::int64x2_set1(b));
    }

    static inline Vector<int64, 2> operator - (int64 a, Vector<int64, 2> b)
    {
        return simd::sub(simd::int64x2_set1(a), b);
    }

    static inline Vector<int64, 2> nand(Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<int64, 2> operator & (Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<int64, 2> operator | (Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<int64, 2> operator ^ (Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<int64, 2> select(Vector<int64, 2>::Mask mask, Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<int64, 2> operator << (Vector<int64, 2> a, int b)
    {
        return simd::sll(a, b);
    }

} // namespace mango
