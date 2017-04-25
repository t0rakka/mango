/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint64, 4> : VectorBase<uint64, 4>
    {
        union
        {
            simd::uint64x4 xyzw;

            ScalarAccessor<uint64, simd::uint64x4, 0> x;
            ScalarAccessor<uint64, simd::uint64x4, 1> y;
            ScalarAccessor<uint64, simd::uint64x4, 2> z;
            ScalarAccessor<uint64, simd::uint64x4, 3> w;
        };

        explicit Vector() = default;

        explicit Vector(uint64 s)
        : xyzw(simd::uint64x4_set1(s))
        {
        }

        explicit Vector(uint64 x, uint64 y, uint64 z, uint64 w)
        : xyzw(simd::uint64x4_set4(x, y, z, w))
        {
        }

        Vector(simd::uint64x4 v)
        : xyzw(v)
        {
        }

        Vector& operator = (simd::uint64x4 v)
        {
            xyzw = v;
            return *this;
        }

        Vector& operator = (uint64 s)
        {
            xyzw = simd::uint64x4_set1(s);
            return *this;
        }

        operator simd::uint64x4 () const
        {
            return xyzw;
        }

        operator simd::uint64x4 ()
        {
            return xyzw;
        }
    };

    static inline const Vector<uint64, 4> operator + (Vector<uint64, 4> v)
    {
        return v;
    }

    static inline Vector<uint64, 4> operator - (Vector<uint64, 4> v)
    {
        return simd::sub(simd::uint64x4_zero(), v);
    }

    static inline Vector<uint64, 4>& operator += (Vector<uint64, 4>& a, Vector<uint64, 4> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<uint64, 4>& operator += (Vector<uint64, 4>& a, uint64 b)
    {
        a = simd::add(a, simd::uint64x4_set1(b));
        return a;
    }

    static inline Vector<uint64, 4>& operator -= (Vector<uint64, 4>& a, Vector<uint64, 4> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint64, 4>& operator -= (Vector<uint64, 4>& a, uint64 b)
    {
        a = simd::sub(a, simd::uint64x4_set1(b));
        return a;
    }

    static inline Vector<uint64, 4> operator + (Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint64, 4> operator + (Vector<uint64, 4> a, uint64 b)
    {
        return simd::add(a, simd::uint64x4_set1(b));
    }

    static inline Vector<uint64, 4> operator + (uint64 a, Vector<uint64, 4> b)
    {
        return simd::add(simd::uint64x4_set1(a), b);
    }

    static inline Vector<uint64, 4> operator - (Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint64, 4> operator - (Vector<uint64, 4> a, uint64 b)
    {
        return simd::sub(a, simd::uint64x4_set1(b));
    }

    static inline Vector<uint64, 4> operator - (uint64 a, Vector<uint64, 4> b)
    {
        return simd::sub(simd::uint64x4_set1(a), b);
    }

    static inline Vector<uint64, 4> nand(Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<uint64, 4> operator & (Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<uint64, 4> operator | (Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<uint64, 4> operator ^ (Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<uint64, 4> select(Vector<uint64, 4> mask, Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<uint64, 4> operator << (Vector<uint64, 4> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<uint64, 4> operator >> (Vector<uint64, 4> a, int b)
    {
        return simd::srl(a, b);
    }

} // namespace mango
