/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    // 128 bit gather

    static inline float32x4 gather4(const float* address, int32x4 offset)
    {
        return simd::gather4(address, offset);
    }

    static inline float64x2 gather2(const double* address, int32x4 offset)
    {
        return simd::gather2(address, offset);
    }

    static inline uint32x4 gather4(const uint32* address, int32x4 offset)
    {
        return simd::gather4(address, offset);
    }

    static inline int32x4 gather4(const int32* address, int32x4 offset)
    {
        return simd::gather4(address, offset);
    }

    static inline uint64x2 gather2(const uint64* address, int32x4 offset)
    {
        return simd::gather2(address, offset);
    }

    static inline int64x2 gather2(const int64* address, int32x4 offset)
    {
        return simd::gather2(address, offset);
    }

    // 256 bit gather

    static inline float32x8 gather8(const float* address, int32x8 offset)
    {
        return simd::gather8(address, offset);
    }

    static inline float64x4 gather4(const double* address, int32x4 offset)
    {
        return simd::gather4(address, offset);
    }

    static inline uint32x8 gather8(const uint32* address, int32x8 offset)
    {
        return simd::gather8(address, offset);
    }

    static inline int32x8 gather8(const int32* address, int32x8 offset)
    {
        return simd::gather8(address, offset);
    }

    static inline uint64x4 gather4(const uint64* address, int32x4 offset)
    {
        return simd::gather4(address, offset);
    }

    static inline int64x4 gather4(const int64* address, int32x4 offset)
    {
        return simd::gather4(address, offset);
    }

} // namespace mango
