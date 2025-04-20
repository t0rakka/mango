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
            std::array<double, 3> component;

            // generate 2 component accessors
#define VECTOR3_SHUFFLE2(A, B, NAME) \
            ShuffleAccessor<Vector<double, 2>, std::array<double, 3>, A, B> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR3_SHUFFLE2

            // generate 3 component accessors
#define VECTOR3_SHUFFLE3(A, B, C, NAME) \
            ShuffleAccessor<Vector<double, 3>, std::array<double, 3>, A, B, C> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR3_SHUFFLE3
        };

        ScalarType& operator [] (size_t index)
        {
            assert(index < VectorSize);
            return component[index];
        }

        ScalarType operator [] (size_t index) const
        {
            assert(index < VectorSize);
            return component[index];
        }

        ScalarType* data()
        {
            return component.data();
        }

        const ScalarType* data() const
        {
            return component.data();
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
