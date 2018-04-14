/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint16, 8>
    {
        using VectorType = simd::uint16x8;
        using ScalarType = uint16;
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

        Vector(uint16 s)
            : m(simd::uint16x8_set1(s))
        {
        }

        Vector(uint16 s0, uint16 s1, uint16 s2, uint16 s3, uint16 s4, uint16 s5, uint16 s6, uint16 s7)
            : m(simd::uint16x8_set8(s0, s1, s2, s3, s4, s5, s6, s7))
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

    template <>
    inline Vector<uint16, 8> load_low<uint16, 8>(const uint16 *source)
    {
        return simd::uint16x8_load_low(source);
    }

    static inline void store_low(uint16 *dest, Vector<uint16, 8> v)
    {
        simd::uint16x8_store_low(dest, v);
    }

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

    static inline Vector<uint16, 8>& operator -= (Vector<uint16, 8>& a, Vector<uint16, 8> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint16, 8> operator + (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint16, 8> operator - (Vector<uint16, 8> a, Vector<uint16, 8> b)
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

    static inline mask16x8 operator > (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask16x8 operator >= (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask16x8 operator < (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask16x8 operator <= (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask16x8 operator == (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask16x8 operator != (Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<uint16, 8> select(mask16x8 mask, Vector<uint16, 8> a, Vector<uint16, 8> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<uint16, 8> operator << (Vector<uint16, 8> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<uint16, 8> operator >> (Vector<uint16, 8> a, int b)
    {
        return simd::srl(a, b);
    }

} // namespace mango
