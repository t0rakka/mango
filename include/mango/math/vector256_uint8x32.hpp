/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint8, 32> : simd::VectorBase<uint8, 32>
    {
        simd::uint8x32 m;

        explicit Vector() = default;

        explicit Vector(uint8 s)
            : m(simd::uint8x32_set1(s))
        {
        }

        Vector(simd::uint8x32 v)
            : m(v)
        {
        }

        Vector& operator = (simd::uint8x32 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (uint8 s)
        {
            m = simd::uint8x32_set1(s);
            return *this;
        }

        operator simd::uint8x32 () const
        {
            return m;
        }

        operator simd::uint8x32 ()
        {
            return m;
        }
    };

    static inline const Vector<uint8, 32> operator + (Vector<uint8, 32> v)
    {
        return v;
    }

    static inline Vector<uint8, 32> operator - (Vector<uint8, 32> v)
    {
        return simd::sub(simd::uint8x32_zero(), v);
    }

    static inline Vector<uint8, 32>& operator += (Vector<uint8, 32>& a, Vector<uint8, 32> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<uint8, 32>& operator += (Vector<uint8, 32>& a, uint8 b)
    {
        a = simd::add(a, simd::uint8x32_set1(b));
        return a;
    }

    static inline Vector<uint8, 32>& operator -= (Vector<uint8, 32>& a, Vector<uint8, 32> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint8, 32>& operator -= (Vector<uint8, 32>& a, uint8 b)
    {
        a = simd::sub(a, simd::uint8x32_set1(b));
        return a;
    }

    static inline Vector<uint8, 32> operator + (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint8, 32> operator + (Vector<uint8, 32> a, uint8 b)
    {
        return simd::add(a, simd::uint8x32_set1(b));
    }

    static inline Vector<uint8, 32> operator + (uint8 a, Vector<uint8, 32> b)
    {
        return simd::add(simd::uint8x32_set1(a), b);
    }

    static inline Vector<uint8, 32> operator - (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint8, 32> operator - (Vector<uint8, 32> a, uint8 b)
    {
        return simd::sub(a, simd::uint8x32_set1(b));
    }

    static inline Vector<uint8, 32> operator - (uint8 a, Vector<uint8, 32> b)
    {
        return simd::sub(simd::uint8x32_set1(a), b);
    }

    static inline Vector<uint8, 32> nand(Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<uint8, 32> operator & (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<uint8, 32> operator | (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<uint8, 32> operator ^ (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<uint8, 32> adds(Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<uint8, 32> subs(Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<uint8, 32> min(Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<uint8, 32> max(Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<uint8, 32> operator > (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline Vector<uint8, 32> operator == (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<uint8, 32> select(Vector<uint8, 32> mask, Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
