/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<s64, 8>
    {
        using VectorType = simd::s64x8;
        using ScalarType = s64;
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

        Vector(s64 s)
            : m(simd::s64x8_set1(s))
        {
        }

        explicit Vector(s64 s0, s64 s1, s64 s2, s64 s3, s64 s4, s64 s5, s64 s6, s64 s7)
            : m(simd::s64x8_set8(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::s64x8 v)
            : m(v)
        {
        }

        Vector& operator = (const Vector& v)
        {
            m = v.m;
            return *this;
        }

        Vector& operator = (simd::s64x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (s64 s)
        {
            m = simd::s64x8_set1(s);
            return *this;
        }

        operator simd::s64x8 () const
        {
            return m;
        }

#ifdef int512_is_hardware_vector
        operator simd::s64x8::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const Vector<s64, 8> operator + (Vector<s64, 8> v)
    {
        return v;
    }

    static inline Vector<s64, 8> operator - (Vector<s64, 8> v)
    {
        return simd::sub(simd::s64x8_zero(), v);
    }

    static inline Vector<s64, 8>& operator += (Vector<s64, 8>& a, Vector<s64, 8> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<s64, 8>& operator -= (Vector<s64, 8>& a, Vector<s64, 8> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<s64, 8> operator + (Vector<s64, 8> a, Vector<s64, 8> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<s64, 8> operator - (Vector<s64, 8> a, Vector<s64, 8> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<s64, 8> nand(Vector<s64, 8> a, Vector<s64, 8> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<s64, 8> operator & (Vector<s64, 8> a, Vector<s64, 8> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<s64, 8> operator | (Vector<s64, 8> a, Vector<s64, 8> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<s64, 8> operator ^ (Vector<s64, 8> a, Vector<s64, 8> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<s64, 8> select(mask64x8 mask, Vector<s64, 8> a, Vector<s64, 8> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<s64, 8> operator << (Vector<s64, 8> a, int b)
    {
        return simd::sll(a, b);
    }

} // namespace mango
