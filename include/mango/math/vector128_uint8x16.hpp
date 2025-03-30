/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango::math
{

    template <>
    struct Vector<u8, 16>
    {
        using VectorType = simd::u8x16;
        using ScalarType = u8;
        enum { VectorSize = 16 };

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

        Vector(u8 s)
            : m(simd::u8x16_set(s))
        {
        }

        Vector(u8 s0, u8 s1, u8 s2, u8 s3, u8 s4, u8 s5, u8 s6, u8 s7, u8 s8, u8 s9, u8 s10, u8 s11, u8 s12, u8 s13, u8 s14, u8 s15)
            : m(simd::u8x16_set(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15))
        {
        }

        Vector(simd::u8x16 v)
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

        Vector& operator = (simd::u8x16 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (u8 s)
        {
            m = simd::u8x16_set(s);
            return *this;
        }

        operator simd::u8x16 () const
        {
            return m;
        }

        operator const auto& () const
        {
            return m.data;
        }

        static Vector uload(const void* source)
        {
            return simd::u8x16_uload(source);
        }


        static void ustore(void* dest, Vector v)
        {
            simd::u8x16_ustore(dest, v);
        }

        static Vector ascend()
        {
            return Vector(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
        }
    };

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    template <>
    inline Vector<u8, 16> load_low<u8, 16>(const u8 *source) noexcept
    {
        return simd::u8x16_load_low(source);
    }

    static inline void store_low(u8 *dest, Vector<u8, 16> v) noexcept
    {
        simd::u8x16_store_low(dest, v);
    }

} // namespace mango::math
