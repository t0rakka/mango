/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>

namespace mango::math
{

    // ------------------------------------------------------------------
    // Vector<double, 2>
    // ------------------------------------------------------------------

    template <>
    struct Vector<double, 2>
    {
        using VectorType = simd::f64x2;
        using ScalarType = double;
        enum { VectorSize = 2 };

        union
        {
            simd::f64x2 m {};

            ScalarAccessor<double, simd::f64x2, 0> x;
            ScalarAccessor<double, simd::f64x2, 1> y;

            // generate 2 component accessors
#define VECTOR2_SHUFFLE2(A, B, NAME) \
            ShuffleAccessor<double, simd::f64x2, A, B> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR2_SHUFFLE2

            // generate 3 component accessors
#define VECTOR2_SHUFFLE3(A, B, C, NAME) \
            ShuffleAccessor<double, simd::f64x2, A, B, C> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR2_SHUFFLE3

            // generate 4 component accessors
#define VECTOR2_SHUFFLE4(A, B, C, D, NAME) \
            ShuffleAccessor<double, simd::f64x2, A, B, C, D> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR2_SHUFFLE4
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

        Vector(double s)
            : m(simd::f64x2_set(s))
        {
        }

        explicit Vector(double x, double y)
            : m(simd::f64x2_set(x, y))
        {
        }

        Vector(simd::f64x2 v)
            : m(v)
        {
        }

        Vector(const Vector& v) = default;

        Vector& operator = (const Vector& v)
        {
            m = v.m;
            return *this;
        }

        Vector& operator = (simd::f64x2 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (double s)
        {
            m = simd::f64x2_set(s);
            return *this;
        }

        operator simd::f64x2 () const
        {
            return m;
        }

        operator const auto& () const
        {
            return m.data;
        }

        static Vector uload(const void* source)
        {
            return simd::f64x2_uload(source);
        }

        static void ustore(void* dest, Vector v)
        {
            simd::f64x2_ustore(dest, v);
        }

        static Vector ascend()
        {
            return Vector(0.0, 1.0);
        }
    };

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline
    double dot(Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::dot2(a, b);
    }

    template <int x, int y>
    static inline
    Vector<double, 2> shuffle(Vector<double, 2> a, Vector<double, 2> b)
    {
        return simd::shuffle<x, y>(a, b);
    }

    // ------------------------------------------------------------------
    // trigonometric functions
    // ------------------------------------------------------------------

    Vector<double, 2> sin(Vector<double, 2> a);
    Vector<double, 2> cos(Vector<double, 2> a);
    Vector<double, 2> tan(Vector<double, 2> a);
    Vector<double, 2> asin(Vector<double, 2> a);
    Vector<double, 2> acos(Vector<double, 2> a);
    Vector<double, 2> atan(Vector<double, 2> a);
    Vector<double, 2> atan2(Vector<double, 2> a, Vector<double, 2> b);

} // namespace mango::math
