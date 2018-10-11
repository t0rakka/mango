/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int64, 2>
    {
        using VectorType = simd::int64x2;
        using ScalarType = int64;
        enum { VectorSize = 2 };

        union
        {
            simd::int64x2 m;

            ScalarAccessor<int64, simd::int64x2, 0> x;
            ScalarAccessor<int64, simd::int64x2, 1> y;

            ShuffleAccessor2<int64, simd::int64x2, 0, 0> xx;
            ShuffleAccessor2<int64, simd::int64x2, 0, 1> xy;
            ShuffleAccessor2<int64, simd::int64x2, 1, 0> yx;
            ShuffleAccessor2<int64, simd::int64x2, 1, 1> yy;

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
            : m(simd::int64x2_set1(s))
        {
        }

        explicit Vector(int64 x, int64 y)
            : m(simd::int64x2_set2(x, y))
        {
        }

        Vector(simd::int64x2 v)
            : m(v)
        {
        }

        template <int X, int Y>
        Vector(const ShuffleAccessor2<int64, simd::int64x2, X, Y>& p)
        {
            m = p;
        }

        template <int X, int Y>
        Vector& operator = (const ShuffleAccessor2<int64, simd::int64x2, X, Y>& p)
        {
            m = p;
            return *this;
        }

        Vector& operator = (simd::int64x2 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (int64 s)
        {
            m = simd::int64x2_set1(s);
            return *this;
        }

        operator simd::int64x2 () const
        {
            return m;
        }

#ifdef int128_is_hardware_vector
        operator simd::int64x2::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const Vector<int64, 2> operator + (Vector<int64, 2> v)
    {
        return v;
    }

    static inline Vector<int64, 2> operator - (Vector<int64, 2> v)
    {
        return simd::sub(simd::int64x2_zero(), v);
    }

    static inline Vector<int64, 2>& operator += (Vector<int64, 2>& a, Vector<int64, 2> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<int64, 2>& operator -= (Vector<int64, 2>& a, Vector<int64, 2> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<int64, 2> operator + (Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int64, 2> operator - (Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int64, 2> nand(Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<int64, 2> operator & (Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<int64, 2> operator | (Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<int64, 2> operator ^ (Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<int64, 2> select(mask64x2 mask, Vector<int64, 2> a, Vector<int64, 2> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<int64, 2> operator << (Vector<int64, 2> a, int b)
    {
        return simd::sll(a, b);
    }

} // namespace mango
