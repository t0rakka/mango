/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>
#include <mango/math/vector_float32x4.hpp>

namespace mango::math
{

    // ------------------------------------------------------------------
    // Vector<float, 8>
    // ------------------------------------------------------------------

    template <>
    struct Vector<float, 8>
    {
        using VectorType = simd::f32x8;
        using ScalarType = float;
        enum { VectorSize = 8 };

        union
        {
            simd::f32x8 m;
            LowAccessor<Vector<float, 4>, simd::f32x8> low;
            HighAccessor<Vector<float, 4>, simd::f32x8> high;
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
            : m(simd::f32x8_set(s))
        {
        }

        explicit Vector(float s0, float s1, float s2, float s3, float s4, float s5, float s6, float s7)
            : m(simd::f32x8_set(s0, s1, s2, s3, s4, s5, s6, s7))
        {
        }

        explicit Vector(const Vector<float, 4>& v0, const Vector<float, 4>& v1)
            : m(simd::combine(v0, v1))
        {
        }

        Vector(simd::f32x8 v)
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

        Vector& operator = (simd::f32x8 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (float s)
        {
            m = simd::f32x8_set(s);
            return *this;
        }

        operator simd::f32x8 () const
        {
            return m;
        }

        operator const auto& () const
        {
            return m.data;
        }

        static Vector uload(const void* source)
        {
            return simd::f32x8_uload(source);
        }

        static void ustore(void* dest, Vector v)
        {
            simd::f32x8_ustore(dest, v);
        }

        static Vector ascend()
        {
            return Vector(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);
        }
    };

    // ------------------------------------------------------------------
    // trigonometric functions
    // ------------------------------------------------------------------

    Vector<float, 8> sin(Vector<float, 8> a);
    Vector<float, 8> cos(Vector<float, 8> a);
    Vector<float, 8> tan(Vector<float, 8> a);
    Vector<float, 8> asin(Vector<float, 8> a);
    Vector<float, 8> acos(Vector<float, 8> a);
    Vector<float, 8> atan(Vector<float, 8> a);
    Vector<float, 8> atan2(Vector<float, 8> a, Vector<float, 8> b);
    Vector<float, 8> exp(Vector<float, 8> a);
    Vector<float, 8> exp2(Vector<float, 8> a);
    Vector<float, 8> log(Vector<float, 8> a);
    Vector<float, 8> log2(Vector<float, 8> a);
    Vector<float, 8> pow(Vector<float, 8> a, Vector<float, 8> b);

} // namespace mango::math
