/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <array>
#include <mango/math/vector.hpp>

namespace mango::math
{

    // ------------------------------------------------------------------
    // Vector<float, 2>
    // ------------------------------------------------------------------

    template <>
    struct Vector<float, 2>
    {
        using VectorType = void;
        using ScalarType = float;
        enum { VectorSize = 2 };

        union
        {
            struct { float x, y; };
            std::array<float, 2> component;

            // generate 2 component accessors
#define VECTOR2_SHUFFLE2(A, B, NAME) \
            ShuffleAccessor<Vector<float, 2>, std::array<float, 2>, A, B> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR2_SHUFFLE2

            /*
            // generate 3 component accessors
#define VECTOR2_SHUFFLE3(A, B, C, NAME) \
            ShuffleAccessor<Vector<float, 3>, std::array<float, 2>, A, B, C> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR2_SHUFFLE3

            // generate 4 component accessors
#define VECTOR2_SHUFFLE4(A, B, C, D, NAME) \
            ShuffleAccessor<Vector<float, 4>, std::array<float, 2>, A, B, C, D> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR2_SHUFFLE4
            */
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
        }

        explicit Vector(float s0, float s1)
        {
            x = s0;
            y = s1;
        }

        Vector(const Vector& v)
        {
            x = v.x;
            y = v.y;
        }

        Vector& operator = (float s)
        {
            x = s;
            y = s;
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
            x = v.x;
            y = v.y;
            return *this;
        }

        static Vector uload(const void* source)
        {
            Vector v;
            std::memcpy(v.data(), source, 8);
            return v;
        }

        static void ustore(void* dest, Vector v)
        {
            std::memcpy(dest, v.data(), 8);
        }

        static Vector ascend()
        {
            return Vector(0.0f, 1.0f);
        }
    };

} // namespace mango::math
