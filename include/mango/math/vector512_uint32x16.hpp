/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<u32, 16>
    {
        using VectorType = simd::uint32x16;
        using ScalarType = u32;
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

        Vector(u32 s)
            : m(simd::uint32x16_set1(s))
        {
        }

        Vector(u32 s0, u32 s1, u32 s2, u32 s3, u32 s4, u32 s5, u32 s6, u32 s7,
            u32 s8, u32 s9, u32 s10, u32 s11, u32 s12, u32 s13, u32 s14, u32 s15)
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

        Vector& operator = (u32 s)
        {
            m = simd::uint32x16_set1(s);
            return *this;
        }

        operator simd::uint32x16 () const
        {
            return m;
        }

#ifdef int512_is_hardware_vector
        operator simd::uint32x16::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const Vector<u32, 16> operator + (Vector<u32, 16> v)
    {
        return v;
    }

    static inline Vector<u32, 16> operator - (Vector<u32, 16> v)
    {
        return simd::sub(simd::uint32x16_zero(), v);
    }

    static inline Vector<u32, 16>& operator += (Vector<u32, 16>& a, Vector<u32, 16> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<u32, 16>& operator -= (Vector<u32, 16>& a, Vector<u32, 16> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<u32, 16> operator + (Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<u32, 16> operator - (Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<u32, 16> nand(Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<u32, 16> operator & (Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<u32, 16> operator | (Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<u32, 16> operator ^ (Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<u32, 16> min(Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<u32, 16> max(Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::max(a, b);
    }

    static inline mask32x16 operator > (Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask32x16 operator < (Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask32x16 operator == (Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<u32, 16> select(mask32x16 mask, Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<u32, 16> operator << (Vector<u32, 16> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<u32, 16> operator >> (Vector<u32, 16> a, int b)
    {
        return simd::srl(a, b);
    }

    static inline Vector<u32, 16> operator << (Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<u32, 16> operator >> (Vector<u32, 16> a, Vector<u32, 16> b)
    {
        return simd::srl(a, b);
    }

} // namespace mango
