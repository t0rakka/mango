/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint64, 2> : VectorBase<uint64, 2>
    {
        using VectorType = simd::uint64x2;

        union
        {
            simd::uint64x2 xy;

            ScalarAccessor<uint64, simd::uint64x2, 0> x;
            ScalarAccessor<uint64, simd::uint64x2, 1> y;

            Permute2<uint64, simd::uint64x2, 0, 0> xx;
            Permute2<uint64, simd::uint64x2, 1, 0> yx;
            Permute2<uint64, simd::uint64x2, 1, 1> yy;
        };

        explicit Vector() = default;

        explicit Vector(uint64 s)
        : xy(simd::uint64x2_set1(s))
        {
        }

        explicit Vector(uint64 x, uint64 y)
        : xy(simd::uint64x2_set2(x, y))
        {
        }

        Vector(simd::uint64x2 v)
        : xy(v)
        {
        }

        template <int X, int Y>
        Vector(const Permute2<uint64, simd::uint64x2, X, Y>& p)
        {
            xy = p;
        }

        template <int X, int Y>
        Vector& operator = (const Permute2<uint64, simd::uint64x2, X, Y>& p)
        {
            xy = p;
            return *this;
        }

        Vector& operator = (simd::uint64x2 v)
        {
            xy = v;
            return *this;
        }

        Vector& operator = (uint64 s)
        {
            xy = simd::uint64x2_set1(s);
            return *this;
        }

        operator simd::uint64x2 () const
        {
            return xy;
        }

        operator simd::uint64x2 ()
        {
            return xy;
        }
    };

    static inline const Vector<uint64, 2> operator + (Vector<uint64, 2> v)
    {
        return v;
    }

    static inline Vector<uint64, 2> operator - (Vector<uint64, 2> v)
    {
        return simd::sub(simd::uint64x2_zero(), v);
    }

    static inline Vector<uint64, 2>& operator += (Vector<uint64, 2>& a, Vector<uint64, 2> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<uint64, 2>& operator += (Vector<uint64, 2>& a, uint64 b)
    {
        a = simd::add(a, simd::uint64x2_set1(b));
        return a;
    }

    static inline Vector<uint64, 2>& operator -= (Vector<uint64, 2>& a, Vector<uint64, 2> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint64, 2>& operator -= (Vector<uint64, 2>& a, uint64 b)
    {
        a = simd::sub(a, simd::uint64x2_set1(b));
        return a;
    }

    static inline Vector<uint64, 2> operator + (Vector<uint64, 2> a, Vector<uint64, 2> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint64, 2> operator + (Vector<uint64, 2> a, uint64 b)
    {
        return simd::add(a, simd::uint64x2_set1(b));
    }

    static inline Vector<uint64, 2> operator + (uint64 a, Vector<uint64, 2> b)
    {
        return simd::add(simd::uint64x2_set1(a), b);
    }

    static inline Vector<uint64, 2> operator - (Vector<uint64, 2> a, Vector<uint64, 2> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint64, 2> operator - (Vector<uint64, 2> a, uint64 b)
    {
        return simd::sub(a, simd::uint64x2_set1(b));
    }

    static inline Vector<uint64, 2> operator - (uint64 a, Vector<uint64, 2> b)
    {
        return simd::sub(simd::uint64x2_set1(a), b);
    }

    static inline Vector<uint64, 2> nand(Vector<uint64, 2> a, Vector<uint64, 2> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<uint64, 2> operator & (Vector<uint64, 2> a, Vector<uint64, 2> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<uint64, 2> operator | (Vector<uint64, 2> a, Vector<uint64, 2> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<uint64, 2> operator ^ (Vector<uint64, 2> a, Vector<uint64, 2> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<uint64, 2> select(mask64x2 mask, Vector<uint64, 2> a, Vector<uint64, 2> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<uint64, 2> operator << (Vector<uint64, 2> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<uint64, 2> operator >> (Vector<uint64, 2> a, int b)
    {
        return simd::srl(a, b);
    }

} // namespace mango
