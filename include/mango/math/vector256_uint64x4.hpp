/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint64, 4>
    {
        using VectorType = simd::uint64x4;
        using ScalarType = uint64;
        enum { VectorSize = 4 };

        union
        {
            simd::uint64x4 m;

            ScalarAccessor<uint64, simd::uint64x4, 0> x;
            ScalarAccessor<uint64, simd::uint64x4, 1> y;
            ScalarAccessor<uint64, simd::uint64x4, 2> z;
            ScalarAccessor<uint64, simd::uint64x4, 3> w;

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

        Vector(uint64 s)
            : m(simd::uint64x4_set1(s))
        {
        }

        explicit Vector(uint64 x, uint64 y, uint64 z, uint64 w)
            : m(simd::uint64x4_set4(x, y, z, w))
        {
        }

        Vector(simd::uint64x4 v)
            : m(v)
        {
        }

        Vector& operator = (simd::uint64x4 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (uint64 s)
        {
            m = simd::uint64x4_set1(s);
            return *this;
        }

        operator simd::uint64x4 () const
        {
            return m;
        }

#ifdef int256_is_hardware_vector
        operator simd::uint64x4::vector () const
        {
            return m.data;
        }
#endif
    };

    static inline const Vector<uint64, 4> operator + (Vector<uint64, 4> v)
    {
        return v;
    }

    static inline Vector<uint64, 4> operator - (Vector<uint64, 4> v)
    {
        return simd::sub(simd::uint64x4_zero(), v);
    }

    static inline Vector<uint64, 4>& operator += (Vector<uint64, 4>& a, Vector<uint64, 4> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<uint64, 4>& operator -= (Vector<uint64, 4>& a, Vector<uint64, 4> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint64, 4> operator + (Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint64, 4> operator - (Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint64, 4> nand(Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<uint64, 4> operator & (Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<uint64, 4> operator | (Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<uint64, 4> operator ^ (Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<uint64, 4> select(mask64x4 mask, Vector<uint64, 4> a, Vector<uint64, 4> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<uint64, 4> operator << (Vector<uint64, 4> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<uint64, 4> operator >> (Vector<uint64, 4> a, int b)
    {
        return simd::srl(a, b);
    }

} // namespace mango
