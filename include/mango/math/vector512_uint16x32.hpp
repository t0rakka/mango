/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<u16, 32>
    {
        using VectorType = simd::uint16x32;
        using ScalarType = u16;
        enum { VectorSize = 32 };

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

        const ScalarType* data() const
        {
            return reinterpret_cast<const ScalarType *>(component);
        }

        explicit Vector() {}
        ~Vector() {}

        Vector(u16 s)
            : m(simd::uint16x32_set1(s))
        {
        }

        Vector(simd::uint16x32 v)
            : m(v)
        {
        }

        Vector& operator = (simd::uint16x32 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (u16 s)
        {
            m = simd::uint16x32_set1(s);
            return *this;
        }

        operator simd::uint16x32 () const
        {
            return m;
        }

#ifdef int512_is_hardware_vector
        operator simd::uint16x32::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const uint16x32 operator + (uint16x32 v)
    {
        return v;
    }

    static inline uint16x32 operator - (uint16x32 v)
    {
        return simd::sub(simd::uint16x32_zero(), v);
    }

    static inline uint16x32& operator += (uint16x32& a, uint16x32 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline uint16x32& operator -= (uint16x32& a, uint16x32 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline uint16x32 operator + (uint16x32 a, uint16x32 b)
    {
        return simd::add(a, b);
    }

    static inline uint16x32 operator - (uint16x32 a, uint16x32 b)
    {
        return simd::sub(a, b);
    }

    static inline uint16x32 nand(uint16x32 a, uint16x32 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline uint16x32 operator & (uint16x32 a, uint16x32 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline uint16x32 operator | (uint16x32 a, uint16x32 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline uint16x32 operator ^ (uint16x32 a, uint16x32 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline uint16x32 adds(uint16x32 a, uint16x32 b)
    {
        return simd::adds(a, b);
    }

    static inline uint16x32 subs(uint16x32 a, uint16x32 b)
    {
        return simd::subs(a, b);
    }

    static inline uint16x32 min(uint16x32 a, uint16x32 b)
    {
        return simd::min(a, b);
    }

    static inline uint16x32 max(uint16x32 a, uint16x32 b)
    {
        return simd::max(a, b);
    }

    static inline mask16x32 operator > (uint16x32 a, uint16x32 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask16x32 operator < (uint16x32 a, uint16x32 b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask16x32 operator == (uint16x32 a, uint16x32 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline uint16x32 select(mask16x32 mask, uint16x32 a, uint16x32 b)
    {
        return simd::select(mask, a, b);
    }

    static inline uint16x32 operator << (uint16x32 a, int b)
    {
        return simd::sll(a, b);
    }

    static inline uint16x32 operator >> (uint16x32 a, int b)
    {
        return simd::srl(a, b);
    }

} // namespace mango
