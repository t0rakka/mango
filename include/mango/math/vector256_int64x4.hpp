/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int64, 4>
    {
        using VectorType = simd::int64x4;
        using ScalarType = int64;
        enum { VectorSize = 4 };

        union
        {
            simd::int64x4 m;

            ScalarAccessor<int64, simd::int64x4, 0> x;
            ScalarAccessor<int64, simd::int64x4, 1> y;
            ScalarAccessor<int64, simd::int64x4, 2> z;
            ScalarAccessor<int64, simd::int64x4, 3> w;

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

        Vector(int64 s)
            : m(simd::int64x4_set1(s))
        {
        }

        explicit Vector(int64 x, int64 y, int64 z, int64 w)
            : m(simd::int64x4_set4(x, y, z, w))
        {
        }

        Vector(simd::int64x4 v)
            : m(v)
        {
        }

        Vector& operator = (simd::int64x4 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (int64 s)
        {
            m = simd::int64x4_set1(s);
            return *this;
        }

        operator simd::int64x4 () const
        {
            return m;
        }

#ifdef int256_is_hardware_vector
        operator simd::int64x4::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const Vector<int64, 4> operator + (Vector<int64, 4> v)
    {
        return v;
    }

    static inline Vector<int64, 4> operator - (Vector<int64, 4> v)
    {
        return simd::sub(simd::int64x4_zero(), v);
    }

    static inline Vector<int64, 4>& operator += (Vector<int64, 4>& a, Vector<int64, 4> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<int64, 4>& operator -= (Vector<int64, 4>& a, Vector<int64, 4> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<int64, 4> operator + (Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int64, 4> operator - (Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int64, 4> nand(Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<int64, 4> operator & (Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<int64, 4> operator | (Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<int64, 4> operator ^ (Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<int64, 4> select(mask64x4 mask, Vector<int64, 4> a, Vector<int64, 4> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<int64, 4> operator << (Vector<int64, 4> a, int b)
    {
        return simd::sll(a, b);
    }

} // namespace mango
