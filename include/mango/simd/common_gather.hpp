/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // 128 bit gather

    static inline float32x4 gather4(const float* address, int32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        auto s2 = address[get_component<2>(offset)];
        auto s3 = address[get_component<3>(offset)];
        return float32x4_set4(s0, s1, s2, s3);
    }

    static inline float64x2 gather2(const double* address, int32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        return float64x2_set2(s0, s1);
    }

    static inline uint32x4 gather4(const uint32* address, int32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        auto s2 = address[get_component<2>(offset)];
        auto s3 = address[get_component<3>(offset)];
        return uint32x4_set4(s0, s1, s2, s3);
    }

    static inline int32x4 gather4(const int32* address, int32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        auto s2 = address[get_component<2>(offset)];
        auto s3 = address[get_component<3>(offset)];
        return int32x4_set4(s0, s1, s2, s3);
    }

    static inline uint64x2 gather2(const uint64* address, int32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        return uint64x2_set2(s0, s1);
    }

    static inline int64x2 gather2(const int64* address, int32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        return int64x2_set2(s0, s1);
    }

    // 256 bit gather

    static inline float32x8 gather8(const float* address, int32x8 offset)
    {
        float32x4 a = gather4(address, get_low(offset));
        float32x4 b = gather4(address, get_high(offset));
        return combine(a, b);
    }

    static inline float64x4 gather4(const double* address, int32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        auto s2 = address[get_component<2>(offset)];
        auto s3 = address[get_component<3>(offset)];
        return float64x4_set4(s0, s1, s2, s3);
    }

    static inline uint32x8 gather8(const uint32* address, int32x8 offset)
    {
        uint32x4 a = gather4(address, get_low(offset));
        uint32x4 b = gather4(address, get_high(offset));
        return combine(a, b);
    }

    static inline int32x8 gather8(const int32* address, int32x8 offset)
    {
        int32x4 a = gather4(address, get_low(offset));
        int32x4 b = gather4(address, get_high(offset));
        return combine(a, b);
    }

    static inline uint64x4 gather4(const uint64* address, int32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        auto s2 = address[get_component<2>(offset)];
        auto s3 = address[get_component<3>(offset)];
        return uint64x4_set4(s0, s1, s2, s3);
    }

    static inline int64x4 gather4(const int64* address, int32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        auto s2 = address[get_component<2>(offset)];
        auto s3 = address[get_component<3>(offset)];
        return int64x4_set4(s0, s1, s2, s3);
    }

    // 512 bit gather

    static inline float32x16 gather16(const float* address, int32x16 offset)
    {
        float32x16 result;
        result.lo = gather8(address, offset.lo);
        result.hi = gather8(address, offset.hi);
        return result;
    }

    static inline float64x8 gather8(const double* address, int32x8 offset)
    {
        float64x8 result;
        result.lo = gather4(address, offset.lo);
        result.hi = gather4(address, offset.hi);
        return result;
    }

    static inline uint32x16 gather16(const uint32* address, int32x16 offset)
    {
        uint32x16 result;
        result.lo = gather8(address, offset.lo);
        result.hi = gather8(address, offset.hi);
        return result;
    }

    static inline int32x16 gather16(const int32* address, int32x16 offset)
    {
        int32x16 result;
        result.lo = gather8(address, offset.lo);
        result.hi = gather8(address, offset.hi);
        return result;
    }

    static inline uint64x8 gather8(const uint64* address, int32x8 offset)
    {
        uint64x8 result;
        result.lo = gather4(address, offset.lo);
        result.hi = gather4(address, offset.hi);
        return result;
    }

    static inline int64x8 gather8(const int64* address, int32x8 offset)
    {
        int64x8 result;
        result.lo = gather4(address, offset.lo);
        result.hi = gather4(address, offset.hi);
        return result;
    }

    // 128 bit masked gather

    static inline float32x4 gather4(const float* address, int32x4 offset, float32x4 value, mask32x4 mask)
    {
        return select(mask, gather4(address, offset), value);
    }

    static inline float64x2 gather2(const double* address, int32x4 offset, float64x2 value, mask64x2 mask)
    {
        return select(mask, gather2(address, offset), value);
    }

    static inline uint32x4 gather4(const uint32* address, int32x4 offset, uint32x4 value, mask32x4 mask)
    {
        return select(mask, gather4(address, offset), value);
    }

    static inline int32x4 gather4(const int32* address, int32x4 offset, int32x4 value, mask32x4 mask)
    {
        return select(mask, gather4(address, offset), value);
    }

    static inline uint64x2 gather2(const uint64* address, int32x4 offset, uint64x2 value, mask64x2 mask)
    {
        return select(mask, gather2(address, offset), value);
    }

    static inline int64x2 gather2(const int64* address, int32x4 offset, int64x2 value, mask64x2 mask)
    {
        return select(mask, gather2(address, offset), value);
    }

    // 256 bit masked gather

    static inline float32x8 gather8(const float* address, int32x8 offset, float32x8 value, mask32x8 mask)
    {
        return select(mask, gather8(address, offset), value);
    }

    static inline float64x4 gather4(const double* address, int32x4 offset, float64x4 value, mask64x4 mask)
    {
        return select(mask, gather4(address, offset), value);
    }

    static inline uint32x8 gather8(const uint32* address, int32x8 offset, uint32x8 value, mask32x8 mask)
    {
        return select(mask, gather8(address, offset), value);
    }

    static inline int32x8 gather8(const int32* address, int32x8 offset, int32x8 value, mask32x8 mask)
    {
        return select(mask, gather8(address, offset), value);
    }

    static inline uint64x4 gather4(const uint64* address, int32x4 offset, uint64x4 value, mask64x4 mask)
    {
        return select(mask, gather4(address, offset), value);
    }

    static inline int64x4 gather4(const int64* address, int32x4 offset, int64x4 value, mask64x4 mask)
    {
        return select(mask, gather4(address, offset), value);
    }

    // 512 bit masked gather

    static inline float32x16 gather16(const float* address, int32x16 offset, float32x16 value, mask32x16 mask)
    {
        return select(mask, gather16(address, offset), value);
    }

    static inline float64x8 gather8(const double* address, int32x8 offset, float64x8 value, mask64x8 mask)
    {
        return select(mask, gather8(address, offset), value);
    }

    static inline uint32x16 gather16(const uint32* address, int32x16 offset, uint32x16 value, mask32x16 mask)
    {
        return select(mask, gather16(address, offset), value);
    }

    static inline int32x16 gather16(const int32* address, int32x16 offset, int32x16 value, mask32x16 mask)
    {
        return select(mask, gather16(address, offset), value);
    }

    static inline uint64x8 gather8(const uint64* address, int32x8 offset, uint64x8 value, mask64x8 mask)
    {
        return select(mask, gather8(address, offset), value);
    }

    static inline int64x8 gather8(const int64* address, int32x8 offset, int64x8 value, mask64x8 mask)
    {
        return select(mask, gather8(address, offset), value);
    }

} // namespace simd
} // namespace mango
