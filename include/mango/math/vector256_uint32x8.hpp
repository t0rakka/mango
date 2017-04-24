/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint32, 8> : simd::VectorBase<uint32, 8>
    {
        simd::uint32x8 m;

        explicit Vector() = default;

        explicit Vector(uint32 s)
            : m(simd::uint32x8_set1(s))
        {
        }

        Vector(simd::uint32x8 v)
            : m(v)
        {
        }

        Vector& operator = (simd::uint32x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (uint32 s)
        {
            m = simd::uint32x8_set1(s);
            return *this;
        }

        operator simd::uint32x8 () const
        {
            return m;
        }

        operator simd::uint32x8 ()
        {
            return m;
        }
    };

    static inline const Vector<uint32, 8> operator + (Vector<uint32, 8> v)
    {
        return v;
    }

    static inline Vector<uint32, 8> operator - (Vector<uint32, 8> v)
    {
        return simd::sub(simd::uint32x8_zero(), v);
    }

    static inline Vector<uint32, 8>& operator += (Vector<uint32, 8>& a, Vector<uint32, 8> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<uint32, 8>& operator += (Vector<uint32, 8>& a, uint32 b)
    {
        a = simd::add(a, simd::uint32x8_set1(b));
        return a;
    }

    static inline Vector<uint32, 8>& operator -= (Vector<uint32, 8>& a, Vector<uint32, 8> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint32, 8>& operator -= (Vector<uint32, 8>& a, uint32 b)
    {
        a = simd::sub(a, simd::uint32x8_set1(b));
        return a;
    }

    static inline Vector<uint32, 8> operator + (Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint32, 8> operator + (Vector<uint32, 8> a, uint32 b)
    {
        return simd::add(a, simd::uint32x8_set1(b));
    }

    static inline Vector<uint32, 8> operator + (uint32 a, Vector<uint32, 8> b)
    {
        return simd::add(simd::uint32x8_set1(a), b);
    }

    static inline Vector<uint32, 8> operator - (Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint32, 8> operator - (Vector<uint32, 8> a, uint32 b)
    {
        return simd::sub(a, simd::uint32x8_set1(b));
    }

    static inline Vector<uint32, 8> operator - (uint32 a, Vector<uint32, 8> b)
    {
        return simd::sub(simd::uint32x8_set1(a), b);
    }

    static inline Vector<uint32, 8> nand(Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<uint32, 8> operator & (Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<uint32, 8> operator | (Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<uint32, 8> operator ^ (Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<uint32, 8> adds(Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<uint32, 8> subs(Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<uint32, 8> min(Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<uint32, 8> max(Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<uint32, 8> operator > (Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline Vector<uint32, 8> operator == (Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<uint32, 8> select(Vector<uint32, 8> mask, Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
