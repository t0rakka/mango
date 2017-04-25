/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint16, 8> : VectorBase<uint16, 8>
    {
        simd::uint16x8 m;

        explicit Vector() = default;

        explicit Vector(uint16 s)
        : m(simd::uint16x8_set1(s))
        {
        }

        Vector(simd::uint16x8 v)
        : m(v)
        {
        }

        Vector& operator = (simd::uint16x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (uint16 s)
        {
            m = simd::uint16x8_set1(s);
            return *this;
        }

        operator simd::uint16x8 () const
        {
            return m;
        }

        operator simd::uint16x8 ()
        {
            return m;
        }
    };

    static inline const Vector<uint16, 8> operator + (Vector<uint16, 8> v)
    {
        return v;
    }

    static inline Vector<uint16, 8> operator - (Vector<uint16, 8> v)
    {
        return simd::sub(simd::uint16x8_zero(), v);
    }

    static inline Vector<uint16, 8>& operator += (Vector<uint16, 8>& a, Vector<uint16, 8> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<uint16, 8>& operator += (Vector<uint16, 8>& a, uint16 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<uint16, 8>& operator -= (Vector<uint16, 8>& a, Vector<uint16, 8> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint16, 8>& operator -= (Vector<uint16, 8>& a, uint16 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint16, 8> operator + (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint16, 8> operator + (Vector<uint16, 8> a, uint16 b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint16, 8> operator + (uint16 a, Vector<uint16, 8> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint16, 8> operator - (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint16, 8> operator - (Vector<uint16, 8> a, uint16 b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint16, 8> operator - (uint16 a, Vector<uint16, 8> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint16, 8> nand(Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<uint16, 8> operator & (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<uint16, 8> operator | (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<uint16, 8> operator ^ (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<uint16, 8> operator ~ (Vector<uint16, 8> a)
    {
        return simd::bitwise_not(a);
    }

    static inline Vector<uint16, 8> adds(Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<uint16, 8> subs(Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<uint16, 8> min(Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<uint16, 8> max(Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<uint16, 8> clamp(Vector<uint16, 8> a, Vector<uint16, 8> amin, Vector<uint16, 8> amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline Vector<uint16, 8> operator > (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline Vector<uint16, 8> operator >= (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline Vector<uint16, 8> operator < (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline Vector<uint16, 8> operator <= (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::compare_le(a, b);
    }

    static inline Vector<uint16, 8> operator == (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<uint16, 8> operator != (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<uint16, 8> select(Vector<uint16, 8> mask, Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
