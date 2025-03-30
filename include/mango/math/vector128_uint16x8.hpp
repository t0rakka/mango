/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango::math
{

    template <>
    struct Vector<u16, 8>
    {
        using VectorType = simd::u16x8;
        using ScalarType = u16;
        enum { VectorSize = 8 };

        VectorType m {};

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

        Vector(u16 s)
            : m(simd::u16x8_set(s))
        {
        }

        Vector(u16 s0, u16 s1, u16 s2, u16 s3, u16 s4, u16 s5, u16 s6, u16 s7)
            : m(simd::u16x8_set(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        Vector(simd::u16x8 v)
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

        Vector& operator = (simd::u16x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (u16 s)
        {
            m = simd::u16x8_set(s);
            return *this;
        }

        operator simd::u16x8 () const
        {
            return m;
        }

        operator const auto& () const
        {
            return m.data;
        }

        static Vector uload(const void* source)
        {
            return simd::u16x8_uload(source);
        }

        static void ustore(void* dest, Vector v)
        {
            simd::u16x8_ustore(dest, v);
        }

        static Vector ascend()
        {
            return Vector(0, 1, 2, 3, 4, 5, 6, 7);
        }
    };

    template <>
    inline Vector<u16, 8> load_low<u16, 8>(const u16 *source) noexcept
    {
        return simd::u16x8_load_low(source);
    }

    static inline void store_low(u16 *dest, Vector<u16, 8> v) noexcept
    {
        simd::u16x8_store_low(dest, v);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<u16, 8> mullo(Vector<u16, 8> a, Vector<u16, 8> b)
    {
        return simd::mullo(a, b);
    }

    // ------------------------------------------------------------------
    // shift
    // ------------------------------------------------------------------

    static inline Vector<u16, 8> operator << (Vector<u16, 8> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<u16, 8> operator >> (Vector<u16, 8> a, int b)
    {
        return simd::srl(a, b);
    }

} // namespace mango::math
