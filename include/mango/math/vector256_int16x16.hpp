/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<s16, 16>
    {
        using VectorType = simd::s16x16;
        using ScalarType = s16;
        enum { VectorSize = 16 };

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

        Vector(s16 s)
            : m(simd::s16x16_set1(s))
        {
        }

        Vector(simd::s16x16 v)
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

        Vector& operator = (simd::s16x16 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (s16 s)
        {
            m = simd::s16x16_set1(s);
            return *this;
        }

        operator simd::s16x16 () const
        {
            return m;
        }

#ifdef int256_is_hardware_vector
        operator simd::s16x16::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const Vector<s16, 16> operator + (Vector<s16, 16> v)
    {
        return v;
    }

    static inline Vector<s16, 16> operator - (Vector<s16, 16> v)
    {
        return simd::sub(simd::s16x16_zero(), v);
    }

    static inline Vector<s16, 16>& operator += (Vector<s16, 16>& a, Vector<s16, 16> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<s16, 16>& operator -= (Vector<s16, 16>& a, Vector<s16, 16> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<s16, 16> operator + (Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<s16, 16> operator - (Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<s16, 16> nand(Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<s16, 16> operator & (Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<s16, 16> operator | (Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<s16, 16> operator ^ (Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<s16, 16> adds(Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<s16, 16> subs(Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<s16, 16> min(Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<s16, 16> max(Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::max(a, b);
    }

    static inline mask16x16 operator > (Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask16x16 operator < (Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask16x16 operator == (Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<s16, 16> select(mask16x16 mask, Vector<s16, 16> a, Vector<s16, 16> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<s16, 16> operator << (Vector<s16, 16> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<s16, 16> operator >> (Vector<s16, 16> a, int b)
    {
        return simd::sra(a, b);
    }

} // namespace mango
