/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"
#include "vector_float4.hpp"

namespace mango
{

    // half4 is a storage class and does not support any arithmetic operations.
    template <>
    struct Vector<half, 4> : VectorBase<half, 4>
    {
        simd4h m;

        explicit Vector()
        {
        }

        explicit Vector(const Vector<float, 4>& v)
        : m(simd4h_convert(v.m))
        {
        }

        Vector(const simd4h& v)
        : m(v)
        {
        }

        ~Vector()
        {
        }

        Vector& operator = (const simd4h& v)
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
            return Vector<float, 4>(simd4f_convert(m));
        }

        operator const simd4h& () const
        {
            return m;
        }

        operator simd4h& ()
        {
            return m;
        }
    };

} // namespace mango
