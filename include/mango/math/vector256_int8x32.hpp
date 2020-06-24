/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<s8, 32>
    {
        using VectorType = simd::s8x32;
        using ScalarType = s8;
        enum { VectorSize = 32 };

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

        Vector(s8 s)
            : m(simd::s8x32_set(s))
        {
        }

        Vector(simd::s8x32 v)
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

        Vector& operator = (simd::s8x32 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (s8 s)
        {
            m = simd::s8x32_set(s);
            return *this;
        }

        operator simd::s8x32 () const
        {
            return m;
        }

#ifdef int256_is_hardware_vector
        operator simd::s8x32::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const Vector<s8, 32> operator + (Vector<s8, 32> v)
    {
        return v;
    }

    static inline Vector<s8, 32> operator - (Vector<s8, 32> v)
    {
        return simd::neg(v);
    }

    static inline Vector<s8, 32>& operator += (Vector<s8, 32>& a, Vector<s8, 32> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<s8, 32>& operator -= (Vector<s8, 32>& a, Vector<s8, 32> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<s8, 32> operator + (Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<s8, 32> operator - (Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<s8, 32> unpacklo(Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::unpacklo(a, b);
    }

    static inline Vector<s8, 32> unpackhi(Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::unpackhi(a, b);
    }

    static inline Vector<s8, 32> abs(Vector<s8, 32> a)
    {
        return simd::abs(a);
    }

    static inline Vector<s8, 32> abs(Vector<s8, 32> a, mask8x32 mask)
    {
        return simd::abs(a, mask);
    }

    static inline Vector<s8, 32> abs(Vector<s8, 32> a, mask8x32 mask, Vector<s8, 32> value)
    {
        return simd::abs(a, mask, value);
    }

    static inline Vector<s8, 32> add(Vector<s8, 32> a, Vector<s8, 32> b, mask8x32 mask)
    {
        return simd::add(a, b, mask);
    }

    static inline Vector<s8, 32> add(Vector<s8, 32> a, Vector<s8, 32> b, mask8x32 mask, Vector<s8, 32> value)
    {
        return simd::add(a, b, mask, value);
    }

    static inline Vector<s8, 32> sub(Vector<s8, 32> a, Vector<s8, 32> b, mask8x32 mask)
    {
        return simd::sub(a, b, mask);
    }

    static inline Vector<s8, 32> sub(Vector<s8, 32> a, Vector<s8, 32> b, mask8x32 mask, Vector<s8, 32> value)
    {
        return simd::sub(a, b, mask, value);
    }

    static inline Vector<s8, 32> adds(Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<s8, 32> adds(Vector<s8, 32> a, Vector<s8, 32> b, mask8x32 mask)
    {
        return simd::adds(a, b, mask);
    }

    static inline Vector<s8, 32> adds(Vector<s8, 32> a, Vector<s8, 32> b, mask8x32 mask, Vector<s8, 32> value)
    {
        return simd::adds(a, b, mask, value);
    }

    static inline Vector<s8, 32> subs(Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<s8, 32> subs(Vector<s8, 32> a, Vector<s8, 32> b, mask8x32 mask)
    {
        return simd::subs(a, b, mask);
    }

    static inline Vector<s8, 32> subs(Vector<s8, 32> a, Vector<s8, 32> b, mask8x32 mask, Vector<s8, 32> value)
    {
        return simd::subs(a, b, mask, value);
    }

    static inline Vector<s8, 32> min(Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<s8, 32> min(Vector<s8, 32> a, Vector<s8, 32> b, mask8x32 mask)
    {
        return simd::min(a, b, mask);
    }

    static inline Vector<s8, 32> min(Vector<s8, 32> a, Vector<s8, 32> b, mask8x32 mask, Vector<s8, 32> value)
    {
        return simd::min(a, b, mask, value);
    }

    static inline Vector<s8, 32> max(Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<s8, 32> max(Vector<s8, 32> a, Vector<s8, 32> b, mask8x32 mask)
    {
        return simd::max(a, b, mask);
    }

    static inline Vector<s8, 32> max(Vector<s8, 32> a, Vector<s8, 32> b, mask8x32 mask, Vector<s8, 32> value)
    {
        return simd::max(a, b, mask, value);
    }

    static inline Vector<s8, 32> clamp(Vector<s8, 32> a, Vector<s8, 32> low, Vector<s8, 32> high)
    {
        return simd::clamp(a, low, high);
    }

    // ------------------------------------------------------------------
	// bitwise operators
    // ------------------------------------------------------------------

    static inline Vector<s8, 32> nand(Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<s8, 32> operator & (Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<s8, 32> operator | (Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<s8, 32> operator ^ (Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<s8, 32> operator ~ (Vector<s8, 32> a)
    {
        return simd::bitwise_not(a);
    }

    // ------------------------------------------------------------------
	// compare / select
    // ------------------------------------------------------------------

    static inline mask8x32 operator > (Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask8x32 operator < (Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask8x32 operator == (Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask8x32 operator >= (Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask8x32 operator <= (Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::compare_le(b, a);
    }

    static inline mask8x32 operator != (Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<s8, 32> select(mask8x32 mask, Vector<s8, 32> a, Vector<s8, 32> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
