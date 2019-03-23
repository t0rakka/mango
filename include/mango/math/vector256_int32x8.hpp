/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<s32, 8>
    {
        using VectorType = simd::s32x8;
        using ScalarType = s32;
        enum { VectorSize = 8 };

        VectorType m;

        ScalarType& operator [] (size_t index)
        {
            assert(index < VectorSize);
            return data()[index];
        }

        ScalarType operator [] (size_t index) const
        {
            assert(index < VectorSize);
            return data()[index];
        }

        ScalarType* data()
        {
            return reinterpret_cast<ScalarType *>(this);
        }

        const ScalarType* data() const
        {
            return reinterpret_cast<const ScalarType *>(this);
        }

        explicit Vector() {}

        Vector(s32 s)
            : m(simd::s32x8_set1(s))
        {
        }

        Vector(s32 s0, s32 s1, s32 s2, s32 s3, s32 s4, s32 s5, s32 s6, s32 s7)
            : m(simd::s32x8_set8(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::s32x8 v)
            : m(v)
        {
        }

        template <typename T, int I>
        Vector& operator = (const ScalarAccessor<ScalarType, T, I>& accessor)
        {
            *this = ScalarType(accessor);
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
            m = v.m;
            return *this;
        }

        Vector& operator = (simd::s32x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (s32 s)
        {
            m = simd::s32x8_set1(s);
            return *this;
        }

        operator simd::s32x8 () const
        {
            return m;
        }

#ifdef int256_is_hardware_vector
        operator simd::s32x8::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const Vector<s32, 8> operator + (Vector<s32, 8> v)
    {
        return v;
    }

    static inline Vector<s32, 8> operator - (Vector<s32, 8> v)
    {
        return simd::sub(simd::s32x8_zero(), v);
    }

    static inline Vector<s32, 8>& operator += (Vector<s32, 8>& a, Vector<s32, 8> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<s32, 8>& operator -= (Vector<s32, 8>& a, Vector<s32, 8> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<s32, 8> operator + (Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<s32, 8> operator - (Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<s32, 8> nand(Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<s32, 8> operator & (Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<s32, 8> operator | (Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<s32, 8> operator ^ (Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<s32, 8> adds(Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<s32, 8> subs(Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<s32, 8> min(Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<s32, 8> max(Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::max(a, b);
    }

    static inline mask32x8 operator > (Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask32x8 operator < (Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask32x8 operator == (Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<s32, 8> select(mask32x8 mask, Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<s32, 8> operator << (Vector<s32, 8> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<s32, 8> operator >> (Vector<s32, 8> a, int b)
    {
        return simd::sra(a, b);
    }

    static inline Vector<s32, 8> operator << (Vector<s32, 8> a, Vector<u32, 8> b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<s32, 8> operator >> (Vector<s32, 8> a, Vector<u32, 8> b)
    {
        return simd::sra(a, b);
    }

} // namespace mango
