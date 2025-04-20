/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango::math
{

    template <>
    struct Vector<s64, 2>
    {
        using VectorType = simd::s64x2;
        using ScalarType = s64;
        enum { VectorSize = 2 };

        union
        {
            simd::s64x2 m {};

            ScalarAccessor<s64, simd::s64x2, 0> x;
            ScalarAccessor<s64, simd::s64x2, 1> y;

            // generate 2 component accessors
#define VECTOR2_SHUFFLE2(A, B, NAME) \
            ShuffleAccessor<Vector<s64, 2>, simd::s64x2, A, B> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR2_SHUFFLE2
        };

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

        Vector(s64 s)
            : m(simd::s64x2_set(s))
        {
        }

        explicit Vector(s64 x, s64 y)
            : m(simd::s64x2_set(x, y))
        {
        }

        Vector(simd::s64x2 v)
            : m(v)
        {
        }

        Vector(const Vector& v) = default;

        Vector& operator = (const Vector& v)
        {
            m = v.m;
            return *this;
        }

        Vector& operator = (simd::s64x2 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (s64 s)
        {
            m = simd::s64x2_set(s);
            return *this;
        }

        operator simd::s64x2 () const
        {
            return m;
        }

        operator const auto& () const
        {
            return m.data;
        }

        static Vector uload(const void* source)
        {
            return simd::s64x2_uload(source);
        }

        static void ustore(void* dest, Vector v)
        {
            simd::s64x2_ustore(dest, v);
        }

        static Vector ascend()
        {
            return Vector(0, 1);
        }
    };

    // ------------------------------------------------------------------
    // shift
    // ------------------------------------------------------------------

    static inline Vector<s64, 2> operator << (Vector<s64, 2> a, int b)
    {
        return simd::sll(a, b);
    }

} // namespace mango::math
