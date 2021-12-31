/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango::math
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
            : m(simd::s32x8_set(s))
        {
        }

        Vector(s32 s0, s32 s1, s32 s2, s32 s3, s32 s4, s32 s5, s32 s6, s32 s7)
            : m(simd::s32x8_set(s0, s1, s2, s3, s4, s5, s6, s7))
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

        Vector(const Vector& v) = default;

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
            m = simd::s32x8_set(s);
            return *this;
        }

        operator simd::s32x8 () const
        {
            return m;
        }

#ifdef simd_int256_is_hardware_vector
        operator simd::s32x8::vector () const
        {
            return m.data;
        }
#endif

        static Vector ascend()
        {
            return Vector(0, 1, 2, 3, 4, 5, 6, 7);
        }
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    MATH_SIMD_UNSIGNED_INTEGER_OPERATORS(s32, 8);
    MATH_SIMD_SIGNED_INTEGER_OPERATORS(s32, 8);

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    MATH_SIMD_INTEGER_FUNCTIONS(s32, 8, mask32x8);
    MATH_SIMD_SATURATING_INTEGER_FUNCTIONS(s32, 8, mask32x8);

    static inline Vector<s32, 8> abs(Vector<s32, 8> a)
    {
        return simd::abs(a);
    }

    static inline Vector<s32, 8> abs(Vector<s32, 8> a, mask32x8 mask)
    {
        return simd::abs(a, mask);
    }

    static inline Vector<s32, 8> abs(Vector<s32, 8> a, mask32x8 mask, Vector<s32, 8> value)
    {
        return simd::abs(a, mask, value);
    }

    static inline Vector<s32, 8> hadd(Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::hadd(a, b);
    }

    static inline Vector<s32, 8> hsub(Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::hsub(a, b);
    }

    // ------------------------------------------------------------------
    // bitwise operators
    // ------------------------------------------------------------------

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

    static inline Vector<s32, 8> operator ~ (Vector<s32, 8> a)
    {
        return simd::bitwise_not(a);
    }

    // ------------------------------------------------------------------
    // compare / select
    // ------------------------------------------------------------------

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

    static inline mask32x8 operator >= (Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask32x8 operator <= (Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::compare_le(b, a);
    }

    static inline mask32x8 operator != (Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<s32, 8> select(mask32x8 mask, Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::select(mask, a, b);
    }

    // ------------------------------------------------------------------
    // shift
    // ------------------------------------------------------------------

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

} // namespace mango::math
