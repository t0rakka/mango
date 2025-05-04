/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <array>
#include <mango/math/vector.hpp>
#include <mango/math/vector_float32x2.hpp>

namespace mango::math
{

    // ------------------------------------------------------------------
    // Vector<float, 3>
    // ------------------------------------------------------------------

    template <>
    struct Vector<float, 3>
    {
        using VectorType = void;
        using ScalarType = float;
        enum { VectorSize = 3 };

        union
        {
            struct { float x, y, z; };
            std::array<float, 3> component;

            // generate 2 component accessors
#define VECTOR3_SHUFFLE2(A, B, NAME) \
            ShuffleAccessor<float, std::array<float, 3>, A, B> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR3_SHUFFLE2

            // generate 3 component accessors
#define VECTOR3_SHUFFLE3(A, B, C, NAME) \
            ShuffleAccessor<float, std::array<float, 3>, A, B, C> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR3_SHUFFLE3

            // generate 4 component accessors
#define VECTOR3_SHUFFLE4(A, B, C, D, NAME) \
            ShuffleAccessor<float, std::array<float, 3>, A, B, C, D> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR3_SHUFFLE4
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

        Vector(float s)
        {
            x = s;
            y = s;
            z = s;
        }

        explicit Vector(float s0, float s1, float s2)
        {
            x = s0;
            y = s1;
            z = s2;
        }

        explicit Vector(const Vector<float, 2>& v, float s)
        {
            x = v.x;
            y = v.y;
            z = s;
        }

        explicit Vector(float s, const Vector<float, 2>& v)
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

        Vector& operator = (float s)
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
