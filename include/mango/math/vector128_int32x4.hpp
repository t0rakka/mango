/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango::math
{

    template <>
    struct Vector<s32, 4>
    {
        using VectorType = simd::s32x4;
        using ScalarType = s32;
        enum { VectorSize = 4 };

        union
        {
            simd::s32x4 m {};

            ScalarAccessor<s32, simd::s32x4, 0> x;
            ScalarAccessor<s32, simd::s32x4, 1> y;
            ScalarAccessor<s32, simd::s32x4, 2> z;
            ScalarAccessor<s32, simd::s32x4, 3> w;

            // generate 2 component accessors
#define VECTOR4_SHUFFLE2(A, B, NAME) \
            ShuffleAccessor<Vector<s32, 2>, simd::s32x4, A, B> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR4_SHUFFLE2

            // generate 3 component accessors
#define VECTOR4_SHUFFLE3(A, B, C, NAME) \
            ShuffleAccessor<Vector<s32, 3>, simd::s32x4, A, B, C> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR4_SHUFFLE3

            // generate 4 component accessors
#define VECTOR4_SHUFFLE4(A, B, C, D, NAME) \
            ShuffleAccessor<Vector<s32, 4>, simd::s32x4, A, B, C, D> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR4_SHUFFLE4
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

        Vector(s32 s)
            : m(simd::s32x4_set(s))
        {
        }

        explicit Vector(s32 x, s32 y, s32 z, s32 w)
            : m(simd::s32x4_set(x, y, z, w))
        {
        }

        Vector(simd::s32x4 v)
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

        Vector& operator = (simd::s32x4 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (s32 s)
        {
            m = simd::s32x4_set(s);
            return *this;
        }

        operator simd::s32x4 () const
        {
            return m;
        }

        operator const auto& () const
        {
            return m.data;
        }

        u32 pack() const
        {
            return simd::pack(m);
        }

        static Vector unpack(u32 a)
        {
            return simd::unpack(a);
        }

        static Vector uload(const void* source)
        {
            return simd::s32x4_uload(source);
        }

        static void ustore(void* dest, Vector v)
        {
            simd::s32x4_ustore(dest, v);
        }

        static Vector ascend()
        {
            return Vector(0, 1, 2, 3);
        }
    };

    template <int x, int y, int z, int w>
    static inline Vector<s32, 4> shuffle(Vector<s32, 4> v)
    {
        return simd::shuffle<x, y, z, w>(v);
    }

    template <>
    inline Vector<s32, 4> load_low<s32, 4>(const s32 *source) noexcept
    {
        return simd::s32x4_load_low(source);
    }

    static inline void store_low(s32 *dest, Vector<s32, 4> v) noexcept
    {
        simd::s32x4_store_low(dest, v);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<s32, 4> hadd(Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::hadd(a, b);
    }

    static inline Vector<s32, 4> hsub(Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::hsub(a, b);
    }

    static inline Vector<s32, 4> mullo(Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::mullo(a, b);
    }

    // ------------------------------------------------------------------
    // shift
    // ------------------------------------------------------------------

    static inline Vector<s32, 4> operator << (Vector<s32, 4> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<s32, 4> operator >> (Vector<s32, 4> a, int b)
    {
        return simd::sra(a, b);
    }

    static inline Vector<s32, 4> operator << (Vector<s32, 4> a, Vector<u32, 4> b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<s32, 4> operator >> (Vector<s32, 4> a, Vector<u32, 4> b)
    {
        return simd::sra(a, b);
    }

} // namespace mango::math
