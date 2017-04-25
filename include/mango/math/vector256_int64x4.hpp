/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int64, 4> : VectorBase<int64, 4>
    {
        union
        {
            simd::int64x4 xyzw;

            ScalarAccessor<int64, simd::int64x4, 0> x;
            ScalarAccessor<int64, simd::int64x4, 1> y;
            ScalarAccessor<int64, simd::int64x4, 2> z;
            ScalarAccessor<int64, simd::int64x4, 3> w;
        };

        explicit Vector() = default;

        explicit Vector(int64 s)
        : xyzw(simd::int64x4_set1(s))
        {
        }

        explicit Vector(int64 x, int64 y, int64 z, int64 w)
        : xyzw(simd::int64x4_set4(x, y, z, w))
        {
        }

        Vector(simd::int64x4 v)
        : xyzw(v)
        {
        }

        Vector& operator = (simd::int64x4 v)
        {
            xyzw = v;
            return *this;
        }

        Vector& operator = (int64 s)
        {
            xyzw = simd::int64x4_set1(s);
            return *this;
        }

        operator simd::int64x4 () const
        {
            return xyzw;
        }

        operator simd::int64x4 ()
        {
            return xyzw;
        }
    };

    static inline const Vector<int64, 4> operator + (Vector<int64, 4> v)
    {
        return v;
    }

    static inline Vector<int64, 4> operator - (Vector<int64, 4> v)
    {
        return simd::sub(simd::int64x4_zero(), v);
    }

    static inline Vector<int64, 4>& operator += (Vector<int64, 4>& a, Vector<int64, 4> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<int64, 4>& operator += (Vector<int64, 4>& a, int64 b)
    {
        a = simd::add(a, simd::int64x4_set1(b));
        return a;
    }

    static inline Vector<int64, 4>& operator -= (Vector<int64, 4>& a, Vector<int64, 4> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<int64, 4>& operator -= (Vector<int64, 4>& a, int64 b)
    {
        a = simd::sub(a, simd::int64x4_set1(b));
        return a;
    }

    static inline Vector<int64, 4> operator + (Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int64, 4> operator + (Vector<int64, 4> a, int64 b)
    {
        return simd::add(a, simd::int64x4_set1(b));
    }

    static inline Vector<int64, 4> operator + (int64 a, Vector<int64, 4> b)
    {
        return simd::add(simd::int64x4_set1(a), b);
    }

    static inline Vector<int64, 4> operator - (Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int64, 4> operator - (Vector<int64, 4> a, int64 b)
    {
        return simd::sub(a, simd::int64x4_set1(b));
    }

    static inline Vector<int64, 4> operator - (int64 a, Vector<int64, 4> b)
    {
        return simd::sub(simd::int64x4_set1(a), b);
    }

    static inline Vector<int64, 4> nand(Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<int64, 4> operator & (Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<int64, 4> operator | (Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<int64, 4> operator ^ (Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<int64, 4> select(Vector<int64, 4> mask, Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
