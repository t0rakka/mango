/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint8, 16> : VectorBase<uint8, 16>
    {
        simd::uint8x16 xyzw;

        explicit Vector() = default;

        explicit Vector(uint8 s)
        : xyzw(simd::uint8x16_set1(s))
        {
        }

        Vector(simd::uint8x16 v)
        : xyzw(v)
        {
        }

        Vector& operator = (simd::uint8x16 v)
        {
            xyzw = v;
            return *this;
        }

        Vector& operator = (uint8 s)
        {
            xyzw = simd::uint8x16_set1(s);
            return *this;
        }

        operator simd::uint8x16 () const
        {
            return xyzw;
        }

        operator simd::uint8x16 ()
        {
            return xyzw;
        }
    };

    static inline const Vector<uint8, 16> operator + (Vector<uint8, 16> v)
    {
        return v;
    }

    static inline Vector<uint8, 16> operator - (Vector<uint8, 16> v)
    {
        return simd::sub(simd::uint8x16_zero(), v);
    }

    static inline Vector<uint8, 16>& operator += (Vector<uint8, 16>& a, Vector<uint8, 16> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<uint8, 16>& operator += (Vector<uint8, 16>& a, uint8 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<uint8, 16>& operator -= (Vector<uint8, 16>& a, Vector<uint8, 16> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint8, 16>& operator -= (Vector<uint8, 16>& a, uint8 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint8, 16> operator + (Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint8, 16> operator + (Vector<uint8, 16> a, uint8 b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint8, 16> operator + (uint8 a, Vector<uint8, 16> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint8, 16> operator - (Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint8, 16> operator - (Vector<uint8, 16> a, uint8 b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint8, 16> operator - (uint8 a, Vector<uint8, 16> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint8, 16> nand(Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<uint8, 16> operator & (Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<uint8, 16> operator | (Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<uint8, 16> operator ^ (Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<uint8, 16> operator ~ (Vector<uint8, 16> a)
    {
        return simd::bitwise_not(a);
    }

    static inline Vector<uint8, 16> adds(Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<uint8, 16> subs(Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<uint8, 16> min(Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<uint8, 16> max(Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<uint8, 16> clamp(Vector<uint8, 16> a, Vector<uint8, 16> amin, Vector<uint8, 16> amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline Vector<uint8, 16> operator > (Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline Vector<uint8, 16> operator >= (Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline Vector<uint8, 16> operator < (Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline Vector<uint8, 16> operator <= (Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::compare_le(a, b);
    }

    static inline Vector<uint8, 16> operator == (Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<uint8, 16> operator != (Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<uint8, 16> select(Vector<uint8, 16> mask, Vector<uint8, 16> a, Vector<uint8, 16> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
