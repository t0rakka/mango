/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango::math
{

    template <>
    struct Vector<u8, 32>
    {
        using VectorType = simd::u8x32;
        using ScalarType = u8;
        enum { VectorSize = 32 };

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
            : m(simd::u8x32_set(s))
        {
        }

        Vector(u8 s00, u8 s01, u8 s02, u8 s03, u8 s04, u8 s05, u8 s06, u8 s07,
               u8 s08, u8 s09, u8 s10, u8 s11, u8 s12, u8 s13, u8 s14, u8 s15,
               u8 s16, u8 s17, u8 s18, u8 s19, u8 s20, u8 s21, u8 s22, u8 s23,
               u8 s24, u8 s25, u8 s26, u8 s27, u8 s28, u8 s29, u8 s30, u8 s31)
            : m(simd::u8x32_set(s00, s01, s02, s03, s04, s05, s06, s07, s08, s09, s10, s11, s12, s13, s14, s15,
                                s16, s17, s18, s19, s20, s21, s22, s23, s24, s25, s26, s27, s28, s29, s30, s31))
        {
        }

        Vector(simd::u8x32 v)
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

        Vector& operator = (simd::u8x32 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (u8 s)
        {
            m = simd::u8x32_set(s);
            return *this;
        }

        operator simd::u8x32 () const
        {
            return m;
        }

        operator const auto& () const
        {
            return m.data;
        }

        static Vector uload(const void* source)
        {
            return simd::u8x32_uload(source);
        }

        static void ustore(void* dest, Vector v)
        {
            simd::u8x32_ustore(dest, v);
        }

        static Vector ascend()
        {
            return Vector(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31);
        }
    };

} // namespace mango::math
