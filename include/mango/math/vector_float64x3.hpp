/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <array>
#include <mango/math/vector.hpp>
#include <mango/math/vector_float64x2.hpp>

namespace mango::math
{

    // ------------------------------------------------------------------
    // Vector<double, 3>
    // ------------------------------------------------------------------

    template <>
    struct Vector<double, 3>
    {
        using VectorType = void;
        using ScalarType = double;
        enum { VectorSize = 3 };

        union
        {
            struct { double x, y, z; };

            // generate 2 component accessors
#define VECTOR3_SHUFFLE_ACCESSOR2(A, B, NAME) \
            ShuffleAccessor<Vector<double, 2>, std::array<double, 3>, A, B> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR3_SHUFFLE_ACCESSOR2

            // generate 3 component accessors
#define VECTOR3_SHUFFLE_ACCESSOR3(A, B, C, NAME) \
            ShuffleAccessor<Vector<double, 3>, std::array<double, 3>, A, B, C> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR3_SHUFFLE_ACCESSOR3
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
        {
            x = s;
            y = s;
            z = s;
        }

        explicit Vector(double s0, double s1, double s2)
        {
            x = s0;
            y = s1;
            z = s2;
        }

        explicit Vector(const Vector<double, 2>& v, double s)
        {
            x = v.x;
            y = v.y;
            z = s;
        }

        explicit Vector(double s, const Vector<double, 2>& v)
        {
            x = s;
            y = v.x;
            z = v.y;
        }

        Vector(const Vector& v)
        {
            x = v.x;
            y = v.y;
            z = v.z;
        }

#if 0
        template <int X, int Y, int Z>
        Vector(const ShuffleAccessor3<X, Y, Z>& p)
        {
            const double* v = p.v;
            x = v[X];
            y = v[Y];
            z = v[Z];
        }

        template <int X, int Y, int Z>
        Vector& operator = (const ShuffleAccessor3<X, Y, Z>& p)
        {
            const double* v = p.v;
            x = v[X];
            y = v[Y];
            z = v[Z];
            return *this;
        }
#endif

        Vector& operator = (double s)
        {
            x = s;
            y = s;
            z = s;
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
            x = v.x;
            y = v.y;
            z = v.z;
            return *this;
        }

        static Vector uload(const void* source)
        {
            Vector v;
            std::memcpy(v.data(), source, 12);
            return v;
        }

        static void ustore(void* dest, Vector v)
        {
            std::memcpy(dest, v.data(), 12);
        }

        static Vector ascend()
        {
            return Vector(0.0f, 1.0f, 2.0f);
        }
    };

} // namespace mango::math
