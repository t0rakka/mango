/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int16, 8>
    {
        using VectorType = simd::int16x8;
        using ScalarType = int16;
        enum { VectorSize = 8 };

        union
        {
            VectorType m;
            DeAggregate<ScalarType> component[VectorSize];
        };

        ScalarType& operator [] (size_t index)
        {
            assert(index < VectorSize);
            return component[index].data;
        }

        ScalarType operator [] (size_t index) const
        {
            assert(index < VectorSize);
            return component[index].data;
        }

        explicit Vector() {}
        ~Vector() {}

        Vector(int16 s)
            : m(simd::int16x8_set1(s))
        {
        }

        Vector(int16 s0, int16 s1, int16 s2, int16 s3, int16 s4, int16 s5, int16 s6, int16 s7)
            : m(simd::int16x8_set8(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::int16x8 v)
            : m(v)
        {
        }

        Vector& operator = (simd::int16x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (int16 s)
        {
            m = simd::int16x8_set1(s);
            return *this;
        }

        operator simd::int16x8 () const
        {
            return m;
        }

        operator simd::int16x8 ()
        {
            return m;
        }
    };

    static inline const Vector<int16, 8> operator + (Vector<int16, 8> v)
    {
        return v;
    }

    static inline Vector<int16, 8> operator - (Vector<int16, 8> v)
    {
        return simd::sub(simd::int16x8_zero(), v);
    }

    static inline Vector<int16, 8>& operator += (Vector<int16, 8>& a, Vector<int16, 8> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<int16, 8>& operator -= (Vector<int16, 8>& a, Vector<int16, 8> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<int16, 8> operator + (Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int16, 8> operator - (Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int16, 8> nand(Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<int16, 8> operator & (Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<int16, 8> operator | (Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<int16, 8> operator ^ (Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<int16, 8> operator ~ (Vector<int16, 8> a)
    {
        return simd::bitwise_not(a);
    }

    static inline Vector<int16, 8> abs(Vector<int16, 8> a)
    {
        return simd::abs(a);
    }

    static inline Vector<int16, 8> adds(Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<int16, 8> subs(Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<int16, 8> min(Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<int16, 8> max(Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<int16, 8> clamp(Vector<int16, 8> a, Vector<int16, 8> amin, Vector<int16, 8> amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline mask16x8 operator > (Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask16x8 operator >= (Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask16x8 operator < (Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask16x8 operator <= (Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask16x8 operator == (Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask16x8 operator != (Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<int16, 8> select(mask16x8 mask, Vector<int16, 8> a, Vector<int16, 8> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<int16, 8> operator << (Vector<int16, 8> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<int16, 8> operator >> (Vector<int16, 8> a, int b)
    {
        return simd::sra(a, b);
    }

} // namespace mango
