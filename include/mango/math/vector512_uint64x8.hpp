/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango::math
{

    template <>
    struct Vector<u64, 8>
    {
        using VectorType = simd::u64x8;
        using ScalarType = u64;
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

        Vector(u64 s)
            : m(simd::u64x8_set(s))
        {
        }

        explicit Vector(u64 s0, u64 s1, u64 s2, u64 s3, u64 s4, u64 s5, u64 s6, u64 s7)
            : m(simd::u64x8_set(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::u64x8 v)
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

        Vector& operator = (simd::u64x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (u64 s)
        {
            m = simd::u64x8_set(s);
            return *this;
        }

        operator simd::u64x8 () const
        {
            return m;
        }

        operator const auto& () const
        {
            return m.data;
        }

        static Vector uload(const void* source)
        {
            return simd::u64x8_uload(source);
        }

        static void ustore(void* dest, Vector v)
        {
            simd::u64x8_ustore(dest, v);
        }

        static Vector ascend()
        {
            return Vector(0, 1, 2, 3, 4, 5, 6, 7);
        }
    };

    // ------------------------------------------------------------------
    // shift
    // ------------------------------------------------------------------

    static inline Vector<u64, 8> operator << (Vector<u64, 8> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<u64, 8> operator >> (Vector<u64, 8> a, int b)
    {
        return simd::srl(a, b);
    }

} // namespace mango::math
