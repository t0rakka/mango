/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int32, 16>
    {
        using VectorType = simd::int32x16;
        using ScalarType = int32;
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

        const ScalarType* data() const
        {
            return reinterpret_cast<const ScalarType *>(component);
        }

        explicit Vector() {}
        ~Vector() {}

        Vector(int32 s)
            : m(simd::int32x16_set1(s))
        {
        }

        Vector(int32 s0, int32 s1, int32 s2, int32 s3, int32 s4, int32 s5, int32 s6, int32 s7, int s8,
            int s9, int s10, int s11, int s12, int s13, int s14, int s15)
            : m(simd::int32x16_set16(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15))
        {
        }

        Vector(simd::int32x16 v)
            : m(v)
        {
        }

        Vector& operator = (simd::int32x16 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (int32 s)
        {
            m = simd::int32x16_set1(s);
            return *this;
        }

        operator simd::int32x16 () const
        {
            return m;
        }

#ifdef int512_is_hardware_vector
        operator simd::int32x16::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const int32x16 operator + (int32x16 v)
    {
        return v;
    }

    static inline int32x16 operator - (int32x16 v)
    {
        return simd::sub(simd::int32x16_zero(), v);
    }

    static inline int32x16& operator += (int32x16& a, int32x16 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline int32x16& operator -= (int32x16& a, int32x16 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline int32x16 operator + (int32x16 a, int32x16 b)
    {
        return simd::add(a, b);
    }

    static inline int32x16 operator - (int32x16 a, int32x16 b)
    {
        return simd::sub(a, b);
    }

    static inline int32x16 nand(int32x16 a, int32x16 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline int32x16 operator & (int32x16 a, int32x16 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline int32x16 operator | (int32x16 a, int32x16 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline int32x16 operator ^ (int32x16 a, int32x16 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline int32x16 min(int32x16 a, int32x16 b)
    {
        return simd::min(a, b);
    }

    static inline int32x16 max(int32x16 a, int32x16 b)
    {
        return simd::max(a, b);
    }

    static inline mask32x16 operator > (int32x16 a, int32x16 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask32x16 operator < (int32x16 a, int32x16 b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask32x16 operator == (int32x16 a, int32x16 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline int32x16 select(mask32x16 mask, int32x16 a, int32x16 b)
    {
        return simd::select(mask, a, b);
    }

    static inline int32x16 operator << (int32x16 a, int b)
    {
        return simd::sll(a, b);
    }

    static inline int32x16 operator >> (int32x16 a, int b)
    {
        return simd::sra(a, b);
    }

    static inline int32x16 operator << (int32x16 a, uint32x16 b)
    {
        return simd::sll(a, b);
    }

    static inline int32x16 operator >> (int32x16 a, uint32x16 b)
    {
        return simd::sra(a, b);
    }

} // namespace mango
