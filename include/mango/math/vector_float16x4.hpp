/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"
#include "vector_float32x4.hpp"

namespace mango
{

    template <>
    struct Vector<float16, 4>
    {
        using VectorType = simd::float16x4;
        using ScalarType = float16;
        enum { VectorSize = 4 };

        union
        {
            simd::float16x4 m;
            DeAggregate<ScalarType> component[VectorSize];
        };

        ScalarType& operator [] (size_t index)
        {
            assert(index < VectorSize);
            return component[index].data;
        }

        ScalarType operator [] (size_t index) const
        {
            assert(index < VectorSize);
            return component[index].data;
        }

        explicit Vector() {}
        ~Vector() {}

        explicit Vector(const Vector<float, 4>& v)
            : m(simd::convert<simd::float16x4>(v.m))
        {
        }

        Vector(simd::float32x4 v)
            : m(simd::convert<simd::float16x4>(v))
        {
        }

        Vector(simd::float16x4 v)
            : m(v)
        {
        }

        Vector& operator = (const Vector<float, 4>& v)
        {
            m = simd::convert<simd::float16x4>(v.m);
            return *this;
        }

        Vector& operator = (simd::float32x4 v)
        {
            m = simd::convert<simd::float16x4>(v);
            return *this;
        }

        Vector& operator = (simd::float16x4 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
            m = v.m;
            return *this;
        }

        operator Vector<float, 4> () const
        {
            return Vector<float, 4>(simd::convert<simd::float32x4>(m));
        }

        operator const simd::float16x4& () const
        {
            return m;
        }

        operator simd::float16x4& ()
        {
            return m;
        }
    };

} // namespace mango
