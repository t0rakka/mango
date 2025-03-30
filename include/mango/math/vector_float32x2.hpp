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

        template <int X, int Y>
        struct ShuffleAccessor2
        {
            float v[2];

            operator Vector<float, 2> () const
            {
                return Vector<float, 2>(v[X], v[Y]);
            }
        };

        union
        {
            struct { float x, y; };

            // generate 2 component accessors
#define VECTOR2_SHUFFLE_ACCESSOR2(A, B, NAME) \
            ShuffleAccessor<Vector<float, 2>, std::array<float, 2>, A, B> NAME
            #include <mango/math/accessor.hpp>
#undef VECTOR2_SHUFFLE_ACCESSOR2
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

#if 0
        template <int X, int Y>
        Vector(const ShuffleAccessor2<X, Y>& p)
        {
            const float* v = p.v;
            x = v[X];
            y = v[Y];
        }

        template <int X, int Y>
        Vector& operator = (const ShuffleAccessor2<X, Y>& p)
        {
            const float* v = p.v;
            x = v[X];
            y = v[Y];
            return *this;
        }
#endif

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
