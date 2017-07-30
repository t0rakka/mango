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
        return {{ v.data[x], v.data[y] }};
    }

    template <uint32 x, uint32 y>
    static inline float64x2 shuffle(float64x2 a, float64x2 b)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return {{ a.data[x], b.data[y] }};
    }

    // indexed access

    template <unsigned int Index>
    static inline float64x2 set_component(float64x2 a, double s)
    {
        static_assert(Index < 2, "Index out of range.");
        a.data[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline double get_component(float64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return a.data[Index];
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
        dest[0] = a.data[0];
        dest[1] = a.data[1];
    }

    static inline float64x2 unpackhi(float64x2 a, float64x2 b)
    {
        return float64x2_set2(a.data[1], b.data[1]);
    }

    static inline float64x2 unpacklo(float64x2 a, float64x2 b)
    {
        return float64x2_set2(a.data[0], b.data[0]);
    }

    // bitwise

    static inline float64x2 bitwise_nand(float64x2 a, float64x2 b)
    {
        const Double x(~Double(a.data[0]).u & Double(b.data[0]).u);
        const Double y(~Double(a.data[1]).u & Double(b.data[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 bitwise_and(float64x2 a, float64x2 b)
    {
        const Double x(Double(a.data[0]).u & Double(b.data[0]).u);
        const Double y(Double(a.data[1]).u & Double(b.data[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 bitwise_or(float64x2 a, float64x2 b)
    {
        const Double x(Double(a.data[0]).u | Double(b.data[0]).u);
        const Double y(Double(a.data[1]).u | Double(b.data[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 bitwise_xor(float64x2 a, float64x2 b)
    {
        const Double x(Double(a.data[0]).u ^ Double(b.data[0]).u);
        const Double y(Double(a.data[1]).u ^ Double(b.data[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 bitwise_not(float64x2 a)
    {
        const Double x(~Double(a.data[0]).u);
        const Double y(~Double(a.data[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 min(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v.data[0] = std::min(a.data[0], b.data[0]);
        v.data[1] = std::min(a.data[1], b.data[1]);
        return v;
    }

    static inline float64x2 max(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v.data[0] = std::max(a.data[0], b.data[0]);
        v.data[1] = std::max(a.data[1], b.data[1]);
        return v;
    }

    static inline float64x2 abs(float64x2 a)
    {
        float64x2 v;
        v.data[0] = std::abs(a.data[0]);
        v.data[1] = std::abs(a.data[1]);
        return v;
    }

    static inline float64x2 neg(float64x2 a)
    {
        return float64x2_set2(-a.data[0], -a.data[1]);
    }

    static inline float64x2 add(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v.data[0] = a.data[0] + b.data[0];
        v.data[1] = a.data[1] + b.data[1];
        return v;
    }

    static inline float64x2 sub(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v.data[0] = a.data[0] - b.data[0];
        v.data[1] = a.data[1] - b.data[1];
        return v;
    }

    static inline float64x2 mul(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v.data[0] = a.data[0] * b.data[0];
        v.data[1] = a.data[1] * b.data[1];
        return v;
    }

    static inline float64x2 div(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v.data[0] = a.data[0] / b.data[0];
        v.data[1] = a.data[1] / b.data[1];
        return v;
    }

    static inline float64x2 div(float64x2 a, double b)
    {
        float64x2 v;
        v.data[0] = a.data[0] / b;
        v.data[1] = a.data[1] / b;
        return v;
    }

    static inline float64x2 madd(float64x2 a, float64x2 b, float64x2 c)
    {
        float64x2 v;
        v.data[0] = a.data[0] + b.data[0] * c.data[0];
        v.data[1] = a.data[1] + b.data[1] * c.data[1];
        return v;
    }

    static inline float64x2 msub(float64x2 a, float64x2 b, float64x2 c)
    {
        float64x2 v;
        v.data[0] = a.data[0] - b.data[0] * c.data[0];
        v.data[1] = a.data[1] - b.data[1] * c.data[1];
        return v;
    }

    static inline float64x2 fast_rcp(float64x2 a)
    {
        float64x2 v;
        v.data[0] = 1.0 / a.data[0];
        v.data[1] = 1.0 / a.data[1];
        return v;
    }

    static inline float64x2 fast_rsqrt(float64x2 a)
    {
        float64x2 v;
        v.data[0] = 1.0 / std::sqrt(a.data[0]);
        v.data[1] = 1.0 / std::sqrt(a.data[1]);
        return v;
    }

    static inline float64x2 fast_sqrt(float64x2 a)
    {
        float64x2 v;
        v.data[0] = std::sqrt(a.data[0]);
        v.data[1] = std::sqrt(a.data[1]);
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
        const double s = a.data[0] * b.data[0] + a.data[1] * b.data[1];
        return float64x2_set1(s);
    }

    // compare

    static inline mask64x2 compare_neq(float64x2 a, float64x2 b)
    {
        uint64 x = a.data[0] != b.data[0] ? ~0 : 0;
        uint64 y = a.data[1] != b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_eq(float64x2 a, float64x2 b)
    {
        uint64 x = a.data[0] == b.data[0] ? ~0 : 0;
        uint64 y = a.data[1] == b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_lt(float64x2 a, float64x2 b)
    {
        uint64 x = a.data[0] < b.data[0] ? ~0 : 0;
        uint64 y = a.data[1] < b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_le(float64x2 a, float64x2 b)
    {
        uint64 x = a.data[0] <= b.data[0] ? ~0 : 0;
        uint64 y = a.data[1] <= b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_gt(float64x2 a, float64x2 b)
    {
        uint64 x = a.data[0] > b.data[0] ? ~0 : 0;
        uint64 y = a.data[1] > b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_ge(float64x2 a, float64x2 b)
    {
        uint64 x = a.data[0] >= b.data[0] ? ~0 : 0;
        uint64 y = a.data[1] >= b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline float64x2 select(mask64x2 mask, float64x2 a, float64x2 b)
    {
        float64x2 m;
        m.data[0] = vgetq_lane_u64(mask, 0);
        m.data[1] = vgetq_lane_u64(mask, 1);
        return bitwise_or(bitwise_and(m, a), bitwise_nand(m, b));
    }

    // rounding

    static inline float64x2 round(float64x2 s)
    {
        float64x2 v;
        v.data[0] = std::round(s.data[0]);
        v.data[1] = std::round(s.data[1]);
        return v;
    }

    static inline float64x2 trunc(float64x2 s)
    {
        float64x2 v;
        v.data[0] = std::trunc(s.data[0]);
        v.data[1] = std::trunc(s.data[1]);
        return v;
    }

    static inline float64x2 floor(float64x2 s)
    {
        float64x2 v;
        v.data[0] = std::floor(s.data[0]);
        v.data[1] = std::floor(s.data[1]);
        return v;
    }

    static inline float64x2 ceil(float64x2 s)
    {
        float64x2 v;
        v.data[0] = std::ceil(s.data[0]);
        v.data[1] = std::ceil(s.data[1]);
        return v;
    }

    static inline float64x2 fract(float64x2 s)
    {
        float64x2 v;
        v.data[0] = s.data[0] - std::floor(s.data[0]);
        v.data[1] = s.data[1] - std::floor(s.data[1]);
        return v;
    }

} // namespace simd
} // namespace mango
