/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint64, 8> : VectorBase<uint64, 8>
    {
        using VectorType = simd::uint64x8;

        simd::uint64x8 xyzw;

        explicit Vector() = default;

        explicit Vector(uint64 s)
        : xyzw(simd::uint64x8_set1(s))
        {
        }

        explicit Vector(uint64 s0, uint64 s1, uint64 s2, uint64 s3, uint64 s4, uint64 s5, uint64 s6, uint64 s7)
        : xyzw(simd::uint64x8_set8(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::uint64x8 v)
        : xyzw(v)
        {
        }

        Vector& operator = (simd::uint64x8 v)
        {
            xyzw = v;
            return *this;
        }

        Vector& operator = (uint64 s)
        {
            xyzw = simd::uint64x8_set1(s);
            return *this;
        }

        operator simd::uint64x8 () const
        {
            return xyzw;
        }

        operator simd::uint64x8 ()
        {
            return xyzw;
        }
    };

    static inline const uint64x8 operator + (uint64x8 v)
    {
        return v;
    }

    static inline uint64x8 operator - (uint64x8 v)
    {
        return simd::sub(simd::uint64x8_zero(), v);
    }

    static inline uint64x8& operator += (uint64x8& a, uint64x8 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline uint64x8& operator += (uint64x8& a, uint64 b)
    {
        a = simd::add(a, simd::uint64x8_set1(b));
        return a;
    }

    static inline uint64x8& operator -= (uint64x8& a, uint64x8 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline uint64x8& operator -= (uint64x8& a, uint64 b)
    {
        a = simd::sub(a, simd::uint64x8_set1(b));
        return a;
    }

    static inline uint64x8 operator + (uint64x8 a, uint64x8 b)
    {
        return simd::add(a, b);
    }

    static inline uint64x8 operator + (uint64x8 a, uint64 b)
    {
        return simd::add(a, simd::uint64x8_set1(b));
    }

    static inline uint64x8 operator + (uint64 a, uint64x8 b)
    {
        return simd::add(simd::uint64x8_set1(a), b);
    }

    static inline uint64x8 operator - (uint64x8 a, uint64x8 b)
    {
        return simd::sub(a, b);
    }

    static inline uint64x8 operator - (uint64x8 a, uint64 b)
    {
        return simd::sub(a, simd::uint64x8_set1(b));
    }

    static inline uint64x8 operator - (uint64 a, uint64x8 b)
    {
        return simd::sub(simd::uint64x8_set1(a), b);
    }

    static inline uint64x8 nand(uint64x8 a, uint64x8 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline uint64x8 operator & (uint64x8 a, uint64x8 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline uint64x8 operator | (uint64x8 a, uint64x8 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline uint64x8 operator ^ (uint64x8 a, uint64x8 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline uint64x8 select(mask64x8 mask, uint64x8 a, uint64x8 b)
    {
        return simd::select(mask, a, b);
    }

    static inline uint64x8 operator << (uint64x8 a, int b)
    {
        return simd::sll(a, b);
    }

    static inline uint64x8 operator >> (uint64x8 a, int b)
    {
        return simd::srl(a, b);
    }

} // namespace mango
