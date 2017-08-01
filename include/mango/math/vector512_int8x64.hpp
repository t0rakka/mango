/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int8, 64> : VectorBase<int8, 64>
    {
        using VectorType = simd::int8x64;

        simd::int8x64 m;

        explicit Vector() = default;

        explicit Vector(int8 s)
        : m(simd::int8x64_set1(s))
        {
        }

        Vector(simd::int8x64 v)
        : m(v)
        {
        }

        Vector& operator = (simd::int8x64 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (int8 s)
        {
            m = simd::int8x64_set1(s);
            return *this;
        }

        operator simd::int8x64 () const
        {
            return m;
        }

        operator simd::int8x64 ()
        {
            return m;
        }
    };

    static inline const int8x64 operator + (int8x64 v)
    {
        return v;
    }

    static inline int8x64 operator - (int8x64 v)
    {
        return simd::sub(simd::int8x64_zero(), v);
    }

    static inline int8x64& operator += (int8x64& a, int8x64 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline int8x64& operator += (int8x64& a, int8 b)
    {
        a = simd::add(a, simd::int8x64_set1(b));
        return a;
    }

    static inline int8x64& operator -= (int8x64& a, int8x64 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline int8x64& operator -= (int8x64& a, int8 b)
    {
        a = simd::sub(a, simd::int8x64_set1(b));
        return a;
    }

    static inline int8x64 operator + (int8x64 a, int8x64 b)
    {
        return simd::add(a, b);
    }

    static inline int8x64 operator + (int8x64 a, int8 b)
    {
        return simd::add(a, simd::int8x64_set1(b));
    }

    static inline int8x64 operator + (int8 a, int8x64 b)
    {
        return simd::add(simd::int8x64_set1(a), b);
    }

    static inline int8x64 operator - (int8x64 a, int8x64 b)
    {
        return simd::sub(a, b);
    }

    static inline int8x64 operator - (int8x64 a, int8 b)
    {
        return simd::sub(a, simd::int8x64_set1(b));
    }

    static inline int8x64 operator - (int8 a, int8x64 b)
    {
        return simd::sub(simd::int8x64_set1(a), b);
    }

    static inline int8x64 nand(int8x64 a, int8x64 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline int8x64 operator & (int8x64 a, int8x64 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline int8x64 operator | (int8x64 a, int8x64 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline int8x64 operator ^ (int8x64 a, int8x64 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline int8x64 adds(int8x64 a, int8x64 b)
    {
        return simd::adds(a, b);
    }

    static inline int8x64 subs(int8x64 a, int8x64 b)
    {
        return simd::subs(a, b);
    }

    static inline int8x64 min(int8x64 a, int8x64 b)
    {
        return simd::min(a, b);
    }

    static inline int8x64 max(int8x64 a, int8x64 b)
    {
        return simd::max(a, b);
    }

    static inline mask8x64 operator > (int8x64 a, int8x64 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask8x64 operator < (int8x64 a, int8x64 b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask8x64 operator == (int8x64 a, int8x64 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline int8x64 select(mask8x64 mask, int8x64 a, int8x64 b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
