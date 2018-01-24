/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint32, 8>
    {
        using VectorType = simd::uint32x8;
        using ScalarType = uint32;
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

        Vector(uint32 s)
            : m(simd::uint32x8_set1(s))
        {
        }

        Vector(uint32 s0, uint32 s1, uint32 s2, uint32 s3, uint32 s4, uint32 s5, uint32 s6, uint32 s7)
            : m(simd::uint32x8_set8(s0, s1, s2, s3, s4, s5, s6, s7))
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

    static inline Vector<uint32, 8>& operator -= (Vector<uint32, 8>& a, Vector<uint32, 8> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint32, 8> operator + (Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint32, 8> operator - (Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::sub(a, b);
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

    static inline mask32x8 operator > (Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask32x8 operator < (Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask32x8 operator == (Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<uint32, 8> select(mask32x8 mask, Vector<uint32, 8> a, Vector<uint32, 8> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<uint32, 8> operator << (Vector<uint32, 8> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<uint32, 8> operator >> (Vector<uint32, 8> a, int b)
    {
        return simd::srl(a, b);
    }

    static inline Vector<uint32, 8> operator << (Vector<uint32, 8> a, uint32x8 b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<uint32, 8> operator >> (Vector<uint32, 8> a, uint32x8 b)
    {
        return simd::srl(a, b);
    }

} // namespace mango
