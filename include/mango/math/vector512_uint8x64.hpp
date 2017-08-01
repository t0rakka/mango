/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint8, 64> : VectorBase<uint8, 64>
    {
        using VectorType = simd::uint8x64;

        simd::uint8x64 m;

        explicit Vector() = default;

        explicit Vector(uint8 s)
        : m(simd::uint8x64_set1(s))
        {
        }

        Vector(simd::uint8x64 v)
        : m(v)
        {
        }

        Vector& operator = (simd::uint8x64 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (uint8 s)
        {
            m = simd::uint8x64_set1(s);
            return *this;
        }

        operator simd::uint8x64 () const
        {
            return m;
        }

        operator simd::uint8x64 ()
        {
            return m;
        }
    };

    static inline const uint8x64 operator + (uint8x64 v)
    {
        return v;
    }

    static inline uint8x64 operator - (uint8x64 v)
    {
        return simd::sub(simd::uint8x64_zero(), v);
    }

    static inline uint8x64& operator += (uint8x64& a, uint8x64 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline uint8x64& operator += (uint8x64& a, uint8 b)
    {
        a = simd::add(a, simd::uint8x64_set1(b));
        return a;
    }

    static inline uint8x64& operator -= (uint8x64& a, uint8x64 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline uint8x64& operator -= (uint8x64& a, uint8 b)
    {
        a = simd::sub(a, simd::uint8x64_set1(b));
        return a;
    }

    static inline uint8x64 operator + (uint8x64 a, uint8x64 b)
    {
        return simd::add(a, b);
    }

    static inline uint8x64 operator + (uint8x64 a, uint8 b)
    {
        return simd::add(a, simd::uint8x64_set1(b));
    }

    static inline uint8x64 operator + (uint8 a, uint8x64 b)
    {
        return simd::add(simd::uint8x64_set1(a), b);
    }

    static inline uint8x64 operator - (uint8x64 a, uint8x64 b)
    {
        return simd::sub(a, b);
    }

    static inline uint8x64 operator - (uint8x64 a, uint8 b)
    {
        return simd::sub(a, simd::uint8x64_set1(b));
    }

    static inline uint8x64 operator - (uint8 a, uint8x64 b)
    {
        return simd::sub(simd::uint8x64_set1(a), b);
    }

    static inline uint8x64 nand(uint8x64 a, uint8x64 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline uint8x64 operator & (uint8x64 a, uint8x64 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline uint8x64 operator | (uint8x64 a, uint8x64 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline uint8x64 operator ^ (uint8x64 a, uint8x64 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline uint8x64 adds(uint8x64 a, uint8x64 b)
    {
        return simd::adds(a, b);
    }

    static inline uint8x64 subs(uint8x64 a, uint8x64 b)
    {
        return simd::subs(a, b);
    }

    static inline uint8x64 min(uint8x64 a, uint8x64 b)
    {
        return simd::min(a, b);
    }

    static inline uint8x64 max(uint8x64 a, uint8x64 b)
    {
        return simd::max(a, b);
    }

    static inline mask8x64 operator > (uint8x64 a, uint8x64 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask8x64 operator < (uint8x64 a, uint8x64 b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask8x64 operator == (uint8x64 a, uint8x64 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline uint8x64 select(mask8x64 mask, uint8x64 a, uint8x64 b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
