/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint8, 32>
    {
        using VectorType = simd::uint8x32;
        using ScalarType = uint8;
        enum { VectorSize = 32 };

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

        explicit Vector() {}
        ~Vector() {}

        Vector(uint8 s)
            : m(simd::uint8x32_set1(s))
        {
        }

        Vector(simd::uint8x32 v)
            : m(v)
        {
        }

        Vector& operator = (simd::uint8x32 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (uint8 s)
        {
            m = simd::uint8x32_set1(s);
            return *this;
        }

        operator simd::uint8x32 () const
        {
            return m;
        }

        operator simd::uint8x32 ()
        {
            return m;
        }
    };

    static inline const Vector<uint8, 32> operator + (Vector<uint8, 32> v)
    {
        return v;
    }

    static inline Vector<uint8, 32> operator - (Vector<uint8, 32> v)
    {
        return simd::sub(simd::uint8x32_zero(), v);
    }

    static inline Vector<uint8, 32>& operator += (Vector<uint8, 32>& a, Vector<uint8, 32> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<uint8, 32>& operator -= (Vector<uint8, 32>& a, Vector<uint8, 32> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint8, 32> operator + (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint8, 32> operator - (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint8, 32> nand(Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<uint8, 32> operator & (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<uint8, 32> operator | (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<uint8, 32> operator ^ (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<uint8, 32> adds(Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<uint8, 32> subs(Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<uint8, 32> min(Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<uint8, 32> max(Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::max(a, b);
    }

    static inline mask8x32 operator > (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask8x32 operator < (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::compare_gt(b, a);
    }

    static inline mask8x32 operator == (Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<uint8, 32> select(mask8x32 mask, Vector<uint8, 32> a, Vector<uint8, 32> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango
