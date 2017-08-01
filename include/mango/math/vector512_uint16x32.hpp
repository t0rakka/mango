/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint16, 32> : VectorBase<uint16, 32>
    {
        using VectorType = simd::uint16x32;

        simd::uint16x32 m;

        explicit Vector() = default;

        explicit Vector(uint16 s)
        : m(simd::uint16x32_set1(s))
        {
        }

        Vector(simd::uint16x32 v)
        : m(v)
        {
        }

        Vector& operator = (simd::uint16x32 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (uint16 s)
        {
            m = simd::uint16x32_set1(s);
            return *this;
        }

        operator simd::uint16x32 () const
        {
            return m;
        }

        operator simd::uint16x32 ()
        {
            return m;
        }
    };

    static inline const uint16x32 operator + (uint16x32 v)
    {
        return v;
    }

    static inline uint16x32 operator - (uint16x32 v)
    {
        return simd::sub(simd::uint16x32_zero(), v);
    }

    static inline uint16x32& operator += (uint16x32& a, uint16x32 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline uint16x32& operator += (uint16x32& a, uint16 b)
    {
        a = simd::add(a, simd::uint16x32_set1(b));
        return a;
    }

    static inline uint16x32& operator -= (uint16x32& a, uint16x32 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline uint16x32& operator -= (uint16x32& a, uint16 b)
    {
        a = simd::sub(a, simd::uint16x32_set1(b));
        return a;
    }

    static inline uint16x32 operator + (uint16x32 a, uint16x32 b)
    {
        return simd::add(a, b);
    }

    static inline uint16x32 operator + (uint16x32 a, uint16 b)
    {
        return simd::add(a, simd::uint16x32_set1(b));
    }

    static inline uint16x32 operator + (uint16 a, uint16x32 b)
    {
        return simd::add(simd::uint16x32_set1(a), b);
    }

    static inline uint16x32 operator - (uint16x32 a, uint16x32 b)
    {
        return simd::sub(a, b);
    }

    static inline uint16x32 operator - (uint16x32 a, uint16 b)
    {
        return simd::sub(a, simd::uint16x32_set1(b));
    }

    static inline uint16x32 operator - (uint16 a, uint16x32 b)
    {
        return simd::sub(simd::uint16x32_set1(a), b);
    }

    static inline uint16x32 nand(uint16x32 a, uint16x32 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline uint16x32 operator & (uint16x32 a, uint16x32 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline uint16x32 operator | (uint16x32 a, uint16x32 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline uint16x32 operator ^ (uint16x32 a, uint16x32 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline uint16x32 adds(uint16x32 a, uint16x32 b)
    {
        return simd::adds(a, b);
    }

    static inline uint16x32 subs(uint16x32 a, uint16x32 b)
    {
        return simd::subs(a, b);
    }

    static inline uint16x32 min(uint16x32 a, uint16x32 b)
    {
        return simd::min(a, b);
    }

    static inline uint16x32 max(uint16x32 a, uint16x32 b)
    {
        return simd::max(a, b);
    }

    static inline mask16x32 operator > (uint16x32 a, uint16x32 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask16x32 operator < (uint16x32 a, uint16x32 b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask16x32 operator == (uint16x32 a, uint16x32 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline uint16x32 select(mask16x32 mask, uint16x32 a, uint16x32 b)
    {
        return simd::select(mask, a, b);
    }

    static inline uint16x32 operator << (uint16x32 a, int b)
    {
        return simd::sll(a, b);
    }

    static inline uint16x32 operator >> (uint16x32 a, int b)
    {
        return simd::srl(a, b);
    }

} // namespace mango
