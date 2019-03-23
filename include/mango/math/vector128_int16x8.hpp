/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<s16, 8>
    {
        using VectorType = simd::s16x8;
        using ScalarType = s16;
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

        Vector(s16 s)
            : m(simd::s16x8_set1(s))
        {
        }

        Vector(s16 s0, s16 s1, s16 s2, s16 s3, s16 s4, s16 s5, s16 s6, s16 s7)
            : m(simd::s16x8_set8(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::s16x8 v)
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

        Vector& operator = (simd::s16x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (s16 s)
        {
            m = simd::s16x8_set1(s);
            return *this;
        }

        operator simd::s16x8 () const
        {
            return m;
        }

#ifdef int128_is_hardware_vector
        operator simd::s16x8::vector () const
        {
            return m.data;
        }
#endif
    };

    template <>
    inline Vector<s16, 8> load_low<s16, 8>(const s16 *source)
    {
        return simd::s16x8_load_low(source);
    }

    static inline void store_low(s16 *dest, Vector<s16, 8> v)
    {
        simd::s16x8_store_low(dest, v);
    }

    static inline const Vector<s16, 8> operator + (Vector<s16, 8> v)
    {
        return v;
    }

    static inline Vector<s16, 8> operator - (Vector<s16, 8> v)
    {
        return simd::sub(simd::s16x8_zero(), v);
    }

    static inline Vector<s16, 8>& operator += (Vector<s16, 8>& a, Vector<s16, 8> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<s16, 8>& operator -= (Vector<s16, 8>& a, Vector<s16, 8> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<s16, 8> operator + (Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<s16, 8> operator - (Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<s16, 8> nand(Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<s16, 8> operator & (Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<s16, 8> operator | (Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<s16, 8> operator ^ (Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<s16, 8> operator ~ (Vector<s16, 8> a)
    {
        return simd::bitwise_not(a);
    }

    static inline Vector<s16, 8> abs(Vector<s16, 8> a)
    {
        return simd::abs(a);
    }

    static inline Vector<s16, 8> adds(Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<s16, 8> subs(Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<s16, 8> min(Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<s16, 8> max(Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<s16, 8> clamp(Vector<s16, 8> a, Vector<s16, 8> amin, Vector<s16, 8> amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline mask16x8 operator > (Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask16x8 operator >= (Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask16x8 operator < (Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask16x8 operator <= (Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask16x8 operator == (Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask16x8 operator != (Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<s16, 8> select(mask16x8 mask, Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<s16, 8> operator << (Vector<s16, 8> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<s16, 8> operator >> (Vector<s16, 8> a, int b)
    {
        return simd::sra(a, b);
    }

} // namespace mango
