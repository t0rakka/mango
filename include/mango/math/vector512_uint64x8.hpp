/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint64, 8>
    {
        using VectorType = simd::uint64x8;
        using ScalarType = uint64;
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

        const ScalarType* data() const
        {
            return reinterpret_cast<const ScalarType *>(component);
        }

        explicit Vector() {}
        ~Vector() {}

        Vector(uint64 s)
            : m(simd::uint64x8_set1(s))
        {
        }

        explicit Vector(uint64 s0, uint64 s1, uint64 s2, uint64 s3, uint64 s4, uint64 s5, uint64 s6, uint64 s7)
            : m(simd::uint64x8_set8(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::uint64x8 v)
            : m(v)
        {
        }

        Vector& operator = (simd::uint64x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (uint64 s)
        {
            m = simd::uint64x8_set1(s);
            return *this;
        }

        operator simd::uint64x8 () const
        {
            return m;
        }

#ifdef int512_is_hardware_vector
        operator simd::uint64x8::vector () const
        {
            return m.data;
        }
#endif
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

    static inline uint64x8& operator -= (uint64x8& a, uint64x8 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline uint64x8 operator + (uint64x8 a, uint64x8 b)
    {
        return simd::add(a, b);
    }

    static inline uint64x8 operator - (uint64x8 a, uint64x8 b)
    {
        return simd::sub(a, b);
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
