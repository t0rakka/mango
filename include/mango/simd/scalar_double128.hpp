/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float64x2
    // -----------------------------------------------------------------

    template <uint32 x, uint32 y>
    static inline float64x2 shuffle(float64x2 v)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return {{ v[x], v[y] }};
    }

    template <uint32 x, uint32 y>
    static inline float64x2 shuffle(float64x2 a, float64x2 b)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return {{ a[x], b[y] }};
    }

    // indexed access

    template <unsigned int Index>
    static inline float64x2 set_component(float64x2 a, double s)
    {
        static_assert(Index < 2, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline double get_component(float64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return a[Index];
    }

    static inline float64x2 float64x2_zero()
    {
        return {{ 0.0, 0.0 }};
    }

    static inline float64x2 float64x2_set1(double s)
    {
        return {{ s, s }};
    }

    static inline float64x2 float64x2_set2(double x, double y)
    {
        return {{ x, y }};
    }

    static inline float64x2 float64x2_uload(const double* source)
    {
        return float64x2_set2(source[0], source[1]);
    }

    static inline void float64x2_ustore(double* dest, float64x2 a)
    {
        dest[0] = a[0];
        dest[1] = a[1];
    }

    static inline float64x2 unpacklo(float64x2 a, float64x2 b)
    {
        return float64x2_set2(a[0], b[0]);
    }

    static inline float64x2 unpackhi(float64x2 a, float64x2 b)
    {
        return float64x2_set2(a[1], b[1]);
    }

    // bitwise

    static inline float64x2 bitwise_nand(float64x2 a, float64x2 b)
    {
        const Double x(~Double(a[0]).u & Double(b[0]).u);
        const Double y(~Double(a[1]).u & Double(b[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 bitwise_and(float64x2 a, float64x2 b)
    {
        const Double x(Double(a[0]).u & Double(b[0]).u);
        const Double y(Double(a[1]).u & Double(b[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 bitwise_or(float64x2 a, float64x2 b)
    {
        const Double x(Double(a[0]).u | Double(b[0]).u);
        const Double y(Double(a[1]).u | Double(b[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 bitwise_xor(float64x2 a, float64x2 b)
    {
        const Double x(Double(a[0]).u ^ Double(b[0]).u);
        const Double y(Double(a[1]).u ^ Double(b[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 bitwise_not(float64x2 a)
    {
        const Double x(~Double(a[0]).u);
        const Double y(~Double(a[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 min(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = std::min(a[0], b[0]);
        v[1] = std::min(a[1], b[1]);
        return v;
    }

    static inline float64x2 max(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = std::max(a[0], b[0]);
        v[1] = std::max(a[1], b[1]);
        return v;
    }

    static inline float64x2 abs(float64x2 a)
    {
        float64x2 v;
        v[0] = std::abs(a[0]);
        v[1] = std::abs(a[1]);
        return v;
    }

    static inline float64x2 neg(float64x2 a)
    {
        return float64x2_set2(-a[0], -a[1]);
    }

    static inline float64x2 sign(float64x2 a)
    {
        float64x2 v;
        v[0] = a[0] < 0 ? -1.0 : (a[0] > 0 ? 1.0 : 0.0);
        v[1] = a[1] < 0 ? -1.0 : (a[1] > 0 ? 1.0 : 0.0);
        return v;
    }

    static inline float64x2 add(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = a[0] + b[0];
        v[1] = a[1] + b[1];
        return v;
    }

    static inline float64x2 sub(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = a[0] - b[0];
        v[1] = a[1] - b[1];
        return v;
    }

    static inline float64x2 mul(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = a[0] * b[0];
        v[1] = a[1] * b[1];
        return v;
    }

    static inline float64x2 div(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = a[0] / b[0];
        v[1] = a[1] / b[1];
        return v;
    }

    static inline float64x2 div(float64x2 a, double b)
    {
        float64x2 v;
        v[0] = a[0] / b;
        v[1] = a[1] / b;
        return v;
    }

    static inline float64x2 madd(float64x2 a, float64x2 b, float64x2 c)
    {
        float64x2 v;
        v[0] = a[0] + b[0] * c[0];
        v[1] = a[1] + b[1] * c[1];
        return v;
    }

    static inline float64x2 msub(float64x2 a, float64x2 b, float64x2 c)
    {
        float64x2 v;
        v[0] = a[0] - b[0] * c[0];
        v[1] = a[1] - b[1] * c[1];
        return v;
    }

    static inline float64x2 fast_rcp(float64x2 a)
    {
        float64x2 v;
        v[0] = 1.0 / a[0];
        v[1] = 1.0 / a[1];
        return v;
    }

    static inline float64x2 fast_rsqrt(float64x2 a)
    {
        float64x2 v;
        v[0] = 1.0 / std::sqrt(a[0]);
        v[1] = 1.0 / std::sqrt(a[1]);
        return v;
    }

    static inline float64x2 fast_sqrt(float64x2 a)
    {
        float64x2 v;
        v[0] = std::sqrt(a[0]);
        v[1] = std::sqrt(a[1]);
        return v;
    }

    static inline float64x2 rcp(float64x2 a)
    {
        return fast_rcp(a);
    }

    static inline float64x2 rsqrt(float64x2 a)
    {
        return fast_rsqrt(a);
    }

    static inline float64x2 sqrt(float64x2 a)
    {
        return fast_sqrt(a);
    }

    static inline float64x2 dot2(float64x2 a, float64x2 b)
    {
        const double s = a[0] * b[0] + a[1] * b[1];
        return float64x2_set1(s);
    }

    // compare

    static inline mask64x2 compare_neq(float64x2 a, float64x2 b)
    {
        mask64x2 v = 0;
        v.mask |= uint32(a[0] != b[0]) << 0;
        v.mask |= uint32(a[1] != b[1]) << 1;
        return v;
    }

    static inline mask64x2 compare_eq(float64x2 a, float64x2 b)
    {
        mask64x2 v = 0;
        v.mask |= uint32(a[0] == b[0]) << 0;
        v.mask |= uint32(a[1] == b[1]) << 1;
        return v;
    }

    static inline mask64x2 compare_lt(float64x2 a, float64x2 b)
    {
        mask64x2 v = 0;
        v.mask |= uint32(a[0] < b[0]) << 0;
        v.mask |= uint32(a[1] < b[1]) << 1;
        return v;
    }

    static inline mask64x2 compare_le(float64x2 a, float64x2 b)
    {
        mask64x2 v = 0;
        v.mask |= uint32(a[0] <= b[0]) << 0;
        v.mask |= uint32(a[1] <= b[1]) << 1;
        return v;
    }

    static inline mask64x2 compare_gt(float64x2 a, float64x2 b)
    {
        mask64x2 v = 0;
        v.mask |= uint32(a[0] > b[0]) << 0;
        v.mask |= uint32(a[1] > b[1]) << 1;
        return v;
    }

    static inline mask64x2 compare_ge(float64x2 a, float64x2 b)
    {
        mask64x2 v = 0;
        v.mask |= uint32(a[0] >= b[0]) << 0;
        v.mask |= uint32(a[1] >= b[1]) << 1;
        return v;
    }

    static inline float64x2 select(mask64x2 mask, float64x2 a, float64x2 b)
    {
        float64x2 result;
        result[0] = mask.mask & (1 << 0) ? a[0] : b[0];
        result[1] = mask.mask & (1 << 1) ? a[1] : b[1];
        return result;
    }

    // rounding

    static inline float64x2 round(float64x2 s)
    {
        float64x2 v;
        v[0] = std::round(s[0]);
        v[1] = std::round(s[1]);
        return v;
    }

    static inline float64x2 trunc(float64x2 s)
    {
        float64x2 v;
        v[0] = std::trunc(s[0]);
        v[1] = std::trunc(s[1]);
        return v;
    }

    static inline float64x2 floor(float64x2 s)
    {
        float64x2 v;
        v[0] = std::floor(s[0]);
        v[1] = std::floor(s[1]);
        return v;
    }

    static inline float64x2 ceil(float64x2 s)
    {
        float64x2 v;
        v[0] = std::ceil(s[0]);
        v[1] = std::ceil(s[1]);
        return v;
    }

    static inline float64x2 fract(float64x2 s)
    {
        float64x2 v;
        v[0] = s[0] - std::floor(s[0]);
        v[1] = s[1] - std::floor(s[1]);
        return v;
    }

} // namespace simd
} // namespace mango
