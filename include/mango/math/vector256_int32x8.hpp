/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango::math
{

    template <>
    struct Vector<s32, 8>
    {
        using VectorType = simd::s32x8;
        using ScalarType = s32;
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

        Vector(s32 s)
            : m(simd::s32x8_set(s))
        {
        }

        Vector(s32 s0, s32 s1, s32 s2, s32 s3, s32 s4, s32 s5, s32 s6, s32 s7)
            : m(simd::s32x8_set(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::s32x8 v)
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

        Vector& operator = (simd::s32x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (s32 s)
        {
            m = simd::s32x8_set(s);
            return *this;
        }

        operator simd::s32x8 () const
        {
            return m;
        }

        operator const auto& () const
        {
            return m.data;
        }

        static Vector uload(const void* source)
        {
            return simd::s32x8_uload(source);
        }

        static void ustore(void* dest, Vector v)
        {
            simd::s32x8_ustore(dest, v);
        }

        static Vector ascend()
        {
            return Vector(0, 1, 2, 3, 4, 5, 6, 7);
        }
    };

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<s32, 8> hadd(Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::hadd(a, b);
    }

    static inline Vector<s32, 8> hsub(Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::hsub(a, b);
    }

    static inline Vector<s32, 8> mullo(Vector<s32, 8> a, Vector<s32, 8> b)
    {
        return simd::mullo(a, b);
    }

    // ------------------------------------------------------------------
    // shift
    // ------------------------------------------------------------------

    static inline Vector<s32, 8> operator << (Vector<s32, 8> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<s32, 8> operator >> (Vector<s32, 8> a, int b)
    {
        return simd::sra(a, b);
    }

    static inline Vector<s32, 8> operator << (Vector<s32, 8> a, Vector<u32, 8> b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<s32, 8> operator >> (Vector<s32, 8> a, Vector<u32, 8> b)
    {
        return simd::sra(a, b);
    }

} // namespace mango::math
