/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango::math
{

    template <>
    struct Vector<s16, 8>
    {
        using VectorType = simd::s16x8;
        using ScalarType = s16;
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

        Vector(s16 s)
            : m(simd::s16x8_set(s))
        {
        }

        Vector(s16 s0, s16 s1, s16 s2, s16 s3, s16 s4, s16 s5, s16 s6, s16 s7)
            : m(simd::s16x8_set(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::s16x8 v)
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

        Vector& operator = (simd::s16x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (s16 s)
        {
            m = simd::s16x8_set(s);
            return *this;
        }

        operator simd::s16x8 () const
        {
            return m;
        }

        operator const auto& () const
        {
            return m.data;
        }

        static Vector uload(const void* source)
        {
            return simd::s16x8_uload(source);
        }

        static void ustore(void* dest, Vector v)
        {
            simd::s16x8_ustore(dest, v);
        }

        static Vector ascend()
        {
            return Vector(0, 1, 2, 3, 4, 5, 6, 7);
        }
    };

    template <>
    inline Vector<s16, 8> load_low<s16, 8>(const s16 *source) noexcept
    {
        return simd::s16x8_load_low(source);
    }

    static inline void store_low(s16 *dest, Vector<s16, 8> v) noexcept
    {
        simd::s16x8_store_low(dest, v);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<s16, 8> hadd(Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::hadd(a, b);
    }

    static inline Vector<s16, 8> hsub(Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::hsub(a, b);
    }

    static inline Vector<s16, 8> hadds(Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::hadds(a, b);
    }

    static inline Vector<s16, 8> hsubs(Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::hsubs(a, b);
    }

    static inline Vector<s16, 8> mullo(Vector<s16, 8> a, Vector<s16, 8> b)
    {
        return simd::mullo(a, b);
    }

    // ------------------------------------------------------------------
    // shift
    // ------------------------------------------------------------------

    static inline Vector<s16, 8> operator << (Vector<s16, 8> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<s16, 8> operator >> (Vector<s16, 8> a, int b)
    {
        return simd::sra(a, b);
    }

} // namespace mango::math
