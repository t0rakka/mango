/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<s64, 4>
    {
        using VectorType = simd::s64x4;
        using ScalarType = s64;
        enum { VectorSize = 4 };

        union
        {
            simd::s64x4 m;

            ScalarAccessor<s64, simd::s64x4, 0> x;
            ScalarAccessor<s64, simd::s64x4, 1> y;
            ScalarAccessor<s64, simd::s64x4, 2> z;
            ScalarAccessor<s64, simd::s64x4, 3> w;

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
            : m(simd::s64x4_set1(s))
        {
        }

        explicit Vector(s64 x, s64 y, s64 z, s64 w)
            : m(simd::s64x4_set4(x, y, z, w))
        {
        }

        Vector(simd::s64x4 v)
            : m(v)
        {
        }

        Vector& operator = (simd::s64x4 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (s64 s)
        {
            m = simd::s64x4_set1(s);
            return *this;
        }

        operator simd::s64x4 () const
        {
            return m;
        }

#ifdef int256_is_hardware_vector
        operator simd::s64x4::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const Vector<s64, 4> operator + (Vector<s64, 4> v)
    {
        return v;
    }

    static inline Vector<s64, 4> operator - (Vector<s64, 4> v)
    {
        return simd::sub(simd::s64x4_zero(), v);
    }

    static inline Vector<s64, 4>& operator += (Vector<s64, 4>& a, Vector<s64, 4> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<s64, 4>& operator -= (Vector<s64, 4>& a, Vector<s64, 4> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<s64, 4> operator + (Vector<s64, 4> a, Vector<s64, 4> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<s64, 4> operator - (Vector<s64, 4> a, Vector<s64, 4> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<s64, 4> nand(Vector<s64, 4> a, Vector<s64, 4> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<s64, 4> operator & (Vector<s64, 4> a, Vector<s64, 4> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<s64, 4> operator | (Vector<s64, 4> a, Vector<s64, 4> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<s64, 4> operator ^ (Vector<s64, 4> a, Vector<s64, 4> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<s64, 4> select(mask64x4 mask, Vector<s64, 4> a, Vector<s64, 4> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<s64, 4> operator << (Vector<s64, 4> a, int b)
    {
        return simd::sll(a, b);
    }

} // namespace mango
