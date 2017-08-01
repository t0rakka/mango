/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int64, 8> : VectorBase<int64, 8>
    {
        using VectorType = simd::int64x8;

        simd::int64x8 xyzw;

        explicit Vector() = default;

        explicit Vector(int64 s)
        : xyzw(simd::int64x8_set1(s))
        {
        }

        explicit Vector(int64 s0, int64 s1, int64 s2, int64 s3, int64 s4, int64 s5, int64 s6, int64 s7)
        : xyzw(simd::int64x8_set8(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::int64x8 v)
        : xyzw(v)
        {
        }

        Vector& operator = (simd::int64x8 v)
        {
            xyzw = v;
            return *this;
        }

        Vector& operator = (int64 s)
        {
            xyzw = simd::int64x8_set1(s);
            return *this;
        }

        operator simd::int64x8 () const
        {
            return xyzw;
        }

        operator simd::int64x8 ()
        {
            return xyzw;
        }
    };

    static inline const int64x8 operator + (int64x8 v)
    {
        return v;
    }

    static inline int64x8 operator - (int64x8 v)
    {
        return simd::sub(simd::int64x8_zero(), v);
    }

    static inline int64x8& operator += (int64x8& a, int64x8 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline int64x8& operator += (int64x8& a, int64 b)
    {
        a = simd::add(a, simd::int64x8_set1(b));
        return a;
    }

    static inline int64x8& operator -= (int64x8& a, int64x8 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline int64x8& operator -= (int64x8& a, int64 b)
    {
        a = simd::sub(a, simd::int64x8_set1(b));
        return a;
    }

    static inline int64x8 operator + (int64x8 a, int64x8 b)
    {
        return simd::add(a, b);
    }

    static inline int64x8 operator + (int64x8 a, int64 b)
    {
        return simd::add(a, simd::int64x8_set1(b));
    }

    static inline int64x8 operator + (int64 a, int64x8 b)
    {
        return simd::add(simd::int64x8_set1(a), b);
    }

    static inline int64x8 operator - (int64x8 a, int64x8 b)
    {
        return simd::sub(a, b);
    }

    static inline int64x8 operator - (int64x8 a, int64 b)
    {
        return simd::sub(a, simd::int64x8_set1(b));
    }

    static inline int64x8 operator - (int64 a, int64x8 b)
    {
        return simd::sub(simd::int64x8_set1(a), b);
    }

    static inline int64x8 nand(int64x8 a, int64x8 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline int64x8 operator & (int64x8 a, int64x8 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline int64x8 operator | (int64x8 a, int64x8 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline int64x8 operator ^ (int64x8 a, int64x8 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline int64x8 select(mask64x8 mask, int64x8 a, int64x8 b)
    {
        return simd::select(mask, a, b);
    }

    static inline int64x8 operator << (int64x8 a, int b)
    {
        return simd::sll(a, b);
    }

} // namespace mango
