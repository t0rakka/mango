/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>
#include <mango/math/vector_float32x2.hpp>
#include <mango/math/vector_float32x3.hpp>

namespace mango::math
{

    // ------------------------------------------------------------------
    // Vector<float, 4>
    // ------------------------------------------------------------------

    template <>
    struct Vector<float, 4>
    {
        using VectorType = simd::f32x4;
        using ScalarType = float;
        enum { VectorSize = 4 };

        union
        {
            simd::f32x4 m {};

            LowAccessor<Vector<float, 2>, simd::f32x4> low;
            HighAccessor<Vector<float, 2>, simd::f32x4> high;

            ScalarAccessor<float, simd::f32x4, 0> x;
            ScalarAccessor<float, simd::f32x4, 1> y;
            ScalarAccessor<float, simd::f32x4, 2> z;
            ScalarAccessor<float, simd::f32x4, 3> w;

            // generate 2 component accessors
#define VECTOR4_SHUFFLE2(A, B, NAME) \
            ShuffleAccessor<Vector<float, 4>, simd::f32x4, A, B> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR4_SHUFFLE2

            // generate 3 component accessors
#define VECTOR4_SHUFFLE3(A, B, C, NAME) \
            ShuffleAccessor<Vector<float, 4>, simd::f32x4, A, B, C> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR4_SHUFFLE3

            // generate 4 component accessors
#define VECTOR4_SHUFFLE4(A, B, C, D, NAME) \
            ShuffleAccessor<Vector<float, 4>, simd::f32x4, A, B, C, D> NAME
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

        Vector(float s)
            : m(simd::f32x4_set(s))
        {
        }

        explicit Vector(float x, float y, float z, float w)
            : m(simd::f32x4_set(x, y, z, w))
        {
        }

        explicit Vector(const Vector<float, 2>& v, float z, float w)
            : m(simd::f32x4_set(v.x, v.y, z, w))
        {
        }

        explicit Vector(float x, float y, const Vector<float, 2>& v)
            : m(simd::f32x4_set(x, y, v.x, v.y))
        {
        }

        explicit Vector(float x, const Vector<float, 2>& v, float w)
            :m(simd::f32x4_set(x, v.x, v.y, w))
        {
        }

        explicit Vector(const Vector<float, 2>& xy, const Vector<float, 2>& zw)
            : m(simd::f32x4_set(xy.x, xy.y, zw.x, zw.y))
        {
        }

        explicit Vector(const Vector<float, 3>& v, float w)
            : m(simd::f32x4_set(v.x, v.y, v.z, w))
        {
        }

        explicit Vector(float x, const Vector<float, 3>& v)
            : m(simd::f32x4_set(x, v.x, v.y, v.z))
        {
        }

        Vector(simd::f32x4 v)
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

        Vector& operator = (simd::f32x4 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (float s)
        {
            m = simd::f32x4_set(s);
            return *this;
        }

        operator simd::f32x4 () const
        {
            return m;
        }

        operator const auto& () const
        {
            return m.data;
        }

        u32 pack() const
        {
            const simd::s32x4 temp = simd::convert<simd::s32x4>(m);
            return simd::pack(temp);
        }

        static Vector unpack(u32 a)
        {
            const simd::s32x4 temp = simd::unpack(a);
            return simd::convert<simd::f32x4>(temp);
        }

        static Vector uload(const void* source)
        {
            return simd::f32x4_uload(source);
        }

        static void ustore(void* dest, Vector v)
        {
            simd::f32x4_ustore(dest, v);
        }

        static Vector ascend()
        {
            return Vector(0.0f, 1.0f, 2.0f, 3.0f);
        }
    };

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline
    float dot(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::dot4(a, b);
    }

    static inline
    Vector<float, 4> cross(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::cross3(a, b);
    }

    static inline
    Vector<float, 4> hmin(Vector<float, 4> v)
    {
        return simd::hmin(v);
    }

    static inline
    Vector<float, 4> hmax(Vector<float, 4> v)
    {
        return simd::hmax(v);
    }

    template <int x, int y, int z, int w>
    static inline
    Vector<float, 4> shuffle(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::shuffle<x, y, z, w>(a, b);
    }

    static inline
    Vector<float, 4> movelh(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::movelh(a, b);
    }

    static inline
    Vector<float, 4> movehl(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::movehl(a, b);
    }

    // ------------------------------------------------------------------
    // trigonometric functions
    // ------------------------------------------------------------------

    Vector<float, 4> sin(Vector<float, 4> a);
    Vector<float, 4> cos(Vector<float, 4> a);
    Vector<float, 4> tan(Vector<float, 4> a);
    Vector<float, 4> asin(Vector<float, 4> a);
    Vector<float, 4> acos(Vector<float, 4> a);
    Vector<float, 4> atan(Vector<float, 4> a);
    Vector<float, 4> atan2(Vector<float, 4> a, Vector<float, 4> b);
    Vector<float, 4> exp(Vector<float, 4> a);
    Vector<float, 4> exp2(Vector<float, 4> a);
    Vector<float, 4> log(Vector<float, 4> a);
    Vector<float, 4> log2(Vector<float, 4> a);
    Vector<float, 4> pow(Vector<float, 4> a, Vector<float, 4> b);

} // namespace mango::math
