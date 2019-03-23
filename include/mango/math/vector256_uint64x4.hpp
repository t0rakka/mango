/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<u64, 4>
    {
        using VectorType = simd::u64x4;
        using ScalarType = u64;
        enum { VectorSize = 4 };

        union
        {
            simd::u64x4 m;

            ScalarAccessor<u64, simd::u64x4, 0> x;
            ScalarAccessor<u64, simd::u64x4, 1> y;
            ScalarAccessor<u64, simd::u64x4, 2> z;
            ScalarAccessor<u64, simd::u64x4, 3> w;

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

        Vector(u64 s)
            : m(simd::u64x4_set1(s))
        {
        }

        explicit Vector(u64 x, u64 y, u64 z, u64 w)
            : m(simd::u64x4_set4(x, y, z, w))
        {
        }

        Vector(simd::u64x4 v)
            : m(v)
        {
        }

        Vector& operator = (const Vector& v)
        {
            m = v.m;
            return *this;
        }

        Vector& operator = (simd::u64x4 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (u64 s)
        {
            m = simd::u64x4_set1(s);
            return *this;
        }

        operator simd::u64x4 () const
        {
            return m;
        }

#ifdef int256_is_hardware_vector
        operator simd::u64x4::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const Vector<u64, 4> operator + (Vector<u64, 4> v)
    {
        return v;
    }

    static inline Vector<u64, 4> operator - (Vector<u64, 4> v)
    {
        return simd::sub(simd::u64x4_zero(), v);
    }

    static inline Vector<u64, 4>& operator += (Vector<u64, 4>& a, Vector<u64, 4> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<u64, 4>& operator -= (Vector<u64, 4>& a, Vector<u64, 4> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<u64, 4> operator + (Vector<u64, 4> a, Vector<u64, 4> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<u64, 4> operator - (Vector<u64, 4> a, Vector<u64, 4> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<u64, 4> nand(Vector<u64, 4> a, Vector<u64, 4> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<u64, 4> operator & (Vector<u64, 4> a, Vector<u64, 4> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<u64, 4> operator | (Vector<u64, 4> a, Vector<u64, 4> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<u64, 4> operator ^ (Vector<u64, 4> a, Vector<u64, 4> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<u64, 4> select(mask64x4 mask, Vector<u64, 4> a, Vector<u64, 4> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<u64, 4> operator << (Vector<u64, 4> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<u64, 4> operator >> (Vector<u64, 4> a, int b)
    {
        return simd::srl(a, b);
    }

} // namespace mango
