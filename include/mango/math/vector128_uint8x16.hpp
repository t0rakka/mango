/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<u8, 16>
    {
        using VectorType = simd::u8x16;
        using ScalarType = u8;
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

        Vector(u8 s)
            : m(simd::u8x16_set1(s))
        {
        }

        Vector(u8 s0, u8 s1, u8 s2, u8 s3, u8 s4, u8 s5, u8 s6, u8 s7, u8 s8, u8 s9, u8 s10, u8 s11, u8 s12, u8 s13, u8 s14, u8 s15)
            : m(simd::u8x16_set16(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15))
        {
        }

        Vector(simd::u8x16 v)
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

        Vector& operator = (simd::u8x16 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (u8 s)
        {
            m = simd::u8x16_set1(s);
            return *this;
        }

        operator simd::u8x16 () const
        {
            return m;
        }

#ifdef int128_is_hardware_vector
        operator simd::u8x16::vector () const
        {
            return m.data;
        }
#endif
    };

    template <>
    inline Vector<u8, 16> load_low<u8, 16>(const u8 *source)
    {
        return simd::u8x16_load_low(source);
    }

    static inline void store_low(u8 *dest, Vector<u8, 16> v)
    {
        simd::u8x16_store_low(dest, v);
    }

    static inline const Vector<u8, 16> operator + (Vector<u8, 16> v)
    {
        return v;
    }

    static inline Vector<u8, 16> operator - (Vector<u8, 16> v)
    {
        return simd::sub(simd::u8x16_zero(), v);
    }

    static inline Vector<u8, 16>& operator += (Vector<u8, 16>& a, Vector<u8, 16> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<u8, 16>& operator -= (Vector<u8, 16>& a, Vector<u8, 16> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<u8, 16> operator + (Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<u8, 16> operator - (Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<u8, 16> nand(Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<u8, 16> operator & (Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<u8, 16> operator | (Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<u8, 16> operator ^ (Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<u8, 16> operator ~ (Vector<u8, 16> a)
    {
        return simd::bitwise_not(a);
    }

    static inline Vector<u8, 16> adds(Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<u8, 16> subs(Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<u8, 16> min(Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<u8, 16> max(Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<u8, 16> clamp(Vector<u8, 16> a, Vector<u8, 16> amin, Vector<u8, 16> amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline mask8x16 operator > (Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask8x16 operator >= (Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask8x16 operator < (Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask8x16 operator <= (Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask8x16 operator == (Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask8x16 operator != (Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<u8, 16> select(mask8x16 mask, Vector<u8, 16> a, Vector<u8, 16> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
