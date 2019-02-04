/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // f64x2
    // -----------------------------------------------------------------

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 v)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return {{ v.data[x], v.data[y] }};
    }

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 a, f64x2 b)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return {{ a.data[x], b.data[y] }};
    }

    // indexed access

    template <unsigned int Index>
    static inline f64x2 set_component(f64x2 a, double s)
    {
        static_assert(Index < 2, "Index out of range.");
        a.data[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline double get_component(f64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return a.data[Index];
    }

    static inline f64x2 f64x2_zero()
    {
        return {{ 0.0, 0.0 }};
    }

    static inline f64x2 f64x2_set1(double s)
    {
        return {{ s, s }};
    }

    static inline f64x2 f64x2_set2(double x, double y)
    {
        return {{ x, y }};
    }

    static inline f64x2 f64x2_uload(const double* source)
    {
        return f64x2_set2(source[0], source[1]);
    }

    static inline void f64x2_ustore(double* dest, f64x2 a)
    {
        dest[0] = a.data[0];
        dest[1] = a.data[1];
    }

    static inline f64x2 unpackhi(f64x2 a, f64x2 b)
    {
        return f64x2_set2(a.data[1], b.data[1]);
    }

    static inline f64x2 unpacklo(f64x2 a, f64x2 b)
    {
        return f64x2_set2(a.data[0], b.data[0]);
    }

    // bitwise

    static inline f64x2 bitwise_nand(f64x2 a, f64x2 b)
    {
        const Double x(~Double(a.data[0]).u & Double(b.data[0]).u);
        const Double y(~Double(a.data[1]).u & Double(b.data[1]).u);
        return f64x2_set2(x, y);
    }

    static inline f64x2 bitwise_and(f64x2 a, f64x2 b)
    {
        const Double x(Double(a.data[0]).u & Double(b.data[0]).u);
        const Double y(Double(a.data[1]).u & Double(b.data[1]).u);
        return f64x2_set2(x, y);
    }

    static inline f64x2 bitwise_or(f64x2 a, f64x2 b)
    {
        const Double x(Double(a.data[0]).u | Double(b.data[0]).u);
        const Double y(Double(a.data[1]).u | Double(b.data[1]).u);
        return f64x2_set2(x, y);
    }

    static inline f64x2 bitwise_xor(f64x2 a, f64x2 b)
    {
        const Double x(Double(a.data[0]).u ^ Double(b.data[0]).u);
        const Double y(Double(a.data[1]).u ^ Double(b.data[1]).u);
        return f64x2_set2(x, y);
    }

    static inline f64x2 bitwise_not(f64x2 a)
    {
        const Double x(~Double(a.data[0]).u);
        const Double y(~Double(a.data[1]).u);
        return f64x2_set2(x, y);
    }

    static inline f64x2 min(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = std::min(a.data[0], b.data[0]);
        v.data[1] = std::min(a.data[1], b.data[1]);
        return v;
    }

    static inline f64x2 max(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = std::max(a.data[0], b.data[0]);
        v.data[1] = std::max(a.data[1], b.data[1]);
        return v;
    }

    static inline f64x2 abs(f64x2 a)
    {
        f64x2 v;
        v.data[0] = std::abs(a.data[0]);
        v.data[1] = std::abs(a.data[1]);
        return v;
    }

    static inline f64x2 neg(f64x2 a)
    {
        return f64x2_set2(-a.data[0], -a.data[1]);
    }

    static inline f64x2 sign(f64x2 a)
    {
        f64x2 v;
        v.data[0] = a.data[0] < 0 ? -1.0f : (a.data[0] > 0 ? 1.0f : 0.0f);
        v.data[1] = a.data[1] < 0 ? -1.0f : (a.data[1] > 0 ? 1.0f : 0.0f);
        return v;
    }
    
    static inline f64x2 add(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = a.data[0] + b.data[0];
        v.data[1] = a.data[1] + b.data[1];
        return v;
    }

    static inline f64x2 sub(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = a.data[0] - b.data[0];
        v.data[1] = a.data[1] - b.data[1];
        return v;
    }

    static inline f64x2 mul(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = a.data[0] * b.data[0];
        v.data[1] = a.data[1] * b.data[1];
        return v;
    }

    static inline f64x2 div(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = a.data[0] / b.data[0];
        v.data[1] = a.data[1] / b.data[1];
        return v;
    }

    static inline f64x2 div(f64x2 a, double b)
    {
        f64x2 v;
        v.data[0] = a.data[0] / b;
        v.data[1] = a.data[1] / b;
        return v;
    }

    static inline f64x2 madd(f64x2 a, f64x2 b, f64x2 c)
    {
        f64x2 v;
        v.data[0] = a.data[0] + b.data[0] * c.data[0];
        v.data[1] = a.data[1] + b.data[1] * c.data[1];
        return v;
    }

    static inline f64x2 msub(f64x2 a, f64x2 b, f64x2 c)
    {
        f64x2 v;
        v.data[0] = a.data[0] - b.data[0] * c.data[0];
        v.data[1] = a.data[1] - b.data[1] * c.data[1];
        return v;
    }

    static inline f64x2 fast_rcp(f64x2 a)
    {
        f64x2 v;
        v.data[0] = 1.0 / a.data[0];
        v.data[1] = 1.0 / a.data[1];
        return v;
    }

    static inline f64x2 fast_rsqrt(f64x2 a)
    {
        f64x2 v;
        v.data[0] = 1.0 / std::sqrt(a.data[0]);
        v.data[1] = 1.0 / std::sqrt(a.data[1]);
        return v;
    }

    static inline f64x2 fast_sqrt(f64x2 a)
    {
        f64x2 v;
        v.data[0] = std::sqrt(a.data[0]);
        v.data[1] = std::sqrt(a.data[1]);
        return v;
    }

    static inline f64x2 rcp(f64x2 a)
    {
        return fast_rcp(a);
    }

    static inline f64x2 rsqrt(f64x2 a)
    {
        return fast_rsqrt(a);
    }

    static inline f64x2 sqrt(f64x2 a)
    {
        return fast_sqrt(a);
    }

    static inline double dot2(f64x2 a, f64x2 b)
    {
        return a.data[0] * b.data[0] + a.data[1] * b.data[1];
    }

    // compare

    static inline mask64x2 compare_neq(f64x2 a, f64x2 b)
    {
        u64 x = a.data[0] != b.data[0] ? ~0 : 0;
        u64 y = a.data[1] != b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_eq(f64x2 a, f64x2 b)
    {
        u64 x = a.data[0] == b.data[0] ? ~0 : 0;
        u64 y = a.data[1] == b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_lt(f64x2 a, f64x2 b)
    {
        u64 x = a.data[0] < b.data[0] ? ~0 : 0;
        u64 y = a.data[1] < b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_le(f64x2 a, f64x2 b)
    {
        u64 x = a.data[0] <= b.data[0] ? ~0 : 0;
        u64 y = a.data[1] <= b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_gt(f64x2 a, f64x2 b)
    {
        u64 x = a.data[0] > b.data[0] ? ~0 : 0;
        u64 y = a.data[1] > b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_ge(f64x2 a, f64x2 b)
    {
        u64 x = a.data[0] >= b.data[0] ? ~0 : 0;
        u64 y = a.data[1] >= b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline f64x2 select(mask64x2 mask, f64x2 a, f64x2 b)
    {
        f64x2 m;
        m.data[0] = vgetq_lane_u64(mask, 0);
        m.data[1] = vgetq_lane_u64(mask, 1);
        return bitwise_or(bitwise_and(m, a), bitwise_nand(m, b));
    }

    // rounding

    static inline f64x2 round(f64x2 s)
    {
        f64x2 v;
        v.data[0] = std::round(s.data[0]);
        v.data[1] = std::round(s.data[1]);
        return v;
    }

    static inline f64x2 trunc(f64x2 s)
    {
        f64x2 v;
        v.data[0] = std::trunc(s.data[0]);
        v.data[1] = std::trunc(s.data[1]);
        return v;
    }

    static inline f64x2 floor(f64x2 s)
    {
        f64x2 v;
        v.data[0] = std::floor(s.data[0]);
        v.data[1] = std::floor(s.data[1]);
        return v;
    }

    static inline f64x2 ceil(f64x2 s)
    {
        f64x2 v;
        v.data[0] = std::ceil(s.data[0]);
        v.data[1] = std::ceil(s.data[1]);
        return v;
    }

    static inline f64x2 fract(f64x2 s)
    {
        f64x2 v;
        v.data[0] = s.data[0] - std::floor(s.data[0]);
        v.data[1] = s.data[1] - std::floor(s.data[1]);
        return v;
    }

} // namespace simd
} // namespace mango
