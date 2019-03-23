/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<u32, 8>
    {
        using VectorType = simd::u32x8;
        using ScalarType = u32;
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

        Vector(u32 s)
            : m(simd::u32x8_set1(s))
        {
        }

        Vector(u32 s0, u32 s1, u32 s2, u32 s3, u32 s4, u32 s5, u32 s6, u32 s7)
            : m(simd::u32x8_set8(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::u32x8 v)
            : m(v)
        {
        }

        Vector& operator = (const Vector& v)
        {
            m = v.m;
            return *this;
        }

        Vector& operator = (simd::u32x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (u32 s)
        {
            m = simd::u32x8_set1(s);
            return *this;
        }

        operator simd::u32x8 () const
        {
            return m;
        }

#ifdef int256_is_hardware_vector
        operator simd::u32x8::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const Vector<u32, 8> operator + (Vector<u32, 8> v)
    {
        return v;
    }

    static inline Vector<u32, 8> operator - (Vector<u32, 8> v)
    {
        return simd::sub(simd::u32x8_zero(), v);
    }

    static inline Vector<u32, 8>& operator += (Vector<u32, 8>& a, Vector<u32, 8> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<u32, 8>& operator -= (Vector<u32, 8>& a, Vector<u32, 8> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<u32, 8> operator + (Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<u32, 8> operator - (Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<u32, 8> nand(Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<u32, 8> operator & (Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<u32, 8> operator | (Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<u32, 8> operator ^ (Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<u32, 8> adds(Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<u32, 8> subs(Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<u32, 8> min(Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<u32, 8> max(Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::max(a, b);
    }

    static inline mask32x8 operator > (Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask32x8 operator < (Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask32x8 operator == (Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<u32, 8> select(mask32x8 mask, Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<u32, 8> operator << (Vector<u32, 8> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<u32, 8> operator >> (Vector<u32, 8> a, int b)
    {
        return simd::srl(a, b);
    }

    static inline Vector<u32, 8> operator << (Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<u32, 8> operator >> (Vector<u32, 8> a, Vector<u32, 8> b)
    {
        return simd::srl(a, b);
    }

} // namespace mango
