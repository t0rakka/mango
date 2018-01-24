/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint32, 16>
    {
        using VectorType = simd::uint32x16;
        using ScalarType = uint32;
        enum { VectorSize = 16 };

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
            : m(simd::uint32x16_set1(s))
        {
        }

        Vector(uint32 s0, uint32 s1, uint32 s2, uint32 s3, uint32 s4, uint32 s5, uint32 s6, uint32 s7,
            uint32 s8, uint32 s9, uint32 s10, uint32 s11, uint32 s12, uint32 s13, uint32 s14, uint32 s15)
            : m(simd::uint32x16_set16(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15))
        {
        }

        Vector(simd::uint32x16 v)
            : m(v)
        {
        }

        Vector& operator = (simd::uint32x16 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (uint32 s)
        {
            m = simd::uint32x16_set1(s);
            return *this;
        }

        operator simd::uint32x16 () const
        {
            return m;
        }

        operator simd::uint32x16 ()
        {
            return m;
        }
    };

    static inline const uint32x16 operator + (uint32x16 v)
    {
        return v;
    }

    static inline uint32x16 operator - (uint32x16 v)
    {
        return simd::sub(simd::uint32x16_zero(), v);
    }

    static inline uint32x16& operator += (uint32x16& a, uint32x16 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline uint32x16& operator -= (uint32x16& a, uint32x16 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline uint32x16 operator + (uint32x16 a, uint32x16 b)
    {
        return simd::add(a, b);
    }

    static inline uint32x16 operator - (uint32x16 a, uint32x16 b)
    {
        return simd::sub(a, b);
    }

    static inline uint32x16 nand(uint32x16 a, uint32x16 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline uint32x16 operator & (uint32x16 a, uint32x16 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline uint32x16 operator | (uint32x16 a, uint32x16 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline uint32x16 operator ^ (uint32x16 a, uint32x16 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline uint32x16 min(uint32x16 a, uint32x16 b)
    {
        return simd::min(a, b);
    }

    static inline uint32x16 max(uint32x16 a, uint32x16 b)
    {
        return simd::max(a, b);
    }

    static inline mask32x16 operator > (uint32x16 a, uint32x16 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask32x16 operator < (uint32x16 a, uint32x16 b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask32x16 operator == (uint32x16 a, uint32x16 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline uint32x16 select(mask32x16 mask, uint32x16 a, uint32x16 b)
    {
        return simd::select(mask, a, b);
    }

    static inline uint32x16 operator << (uint32x16 a, int b)
    {
        return simd::sll(a, b);
    }

    static inline uint32x16 operator >> (uint32x16 a, int b)
    {
        return simd::srl(a, b);
    }

    static inline uint32x16 operator << (uint32x16 a, uint32x16 b)
    {
        return simd::sll(a, b);
    }

    static inline uint32x16 operator >> (uint32x16 a, uint32x16 b)
    {
        return simd::srl(a, b);
    }

} // namespace mango
