/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float64x4
    // -----------------------------------------------------------------

    // shuffle

    template <u32 x, u32 y, u32 z, u32 w>
    static inline float64x4 shuffle(float64x4 v)
    {
        const float64x2 v0 = x & 2 ? v.hi : v.lo;
        const float64x2 v1 = y & 2 ? v.hi : v.lo;
        const float64x2 v2 = z & 2 ? v.hi : v.lo;
        const float64x2 v3 = w & 2 ? v.hi : v.lo;

        float64x4 result;
        result.lo = shuffle<x & 1, y & 1>(v0, v1);
        result.hi = shuffle<z & 1, w & 1>(v2, v3);
        return result;
    }

    template <>
    inline float64x4 shuffle<0, 1, 2, 3>(float64x4 v)
    {
        // .xyzw
        return v;
    }

    template <>
    inline float64x4 shuffle<0, 0, 0, 0>(float64x4 v)
    {
        // .xxxx
        const float64x2 xx = shuffle<0, 0>(v.lo);
        float64x4 result;
        result.lo = xx;
        result.hi = xx;
        return result;
    }

    template <>
    inline float64x4 shuffle<1, 1, 1, 1>(float64x4 v)
    {
        // .yyyy
        const float64x2 yy = shuffle<1, 1>(v.lo);
        float64x4 result;
        result.lo = yy;
        result.hi = yy;
        return result;
    }

    template <>
    inline float64x4 shuffle<2, 2, 2, 2>(float64x4 v)
    {
        // .zzzz
        const float64x2 zz = shuffle<0, 0>(v.hi);
        float64x4 result;
        result.lo = zz;
        result.hi = zz;
        return result;
    }

    template <>
    inline float64x4 shuffle<3, 3, 3, 3>(float64x4 v)
    {
        // .wwww
        const float64x2 ww = shuffle<1, 1>(v.hi);
        float64x4 result;
        result.lo = ww;
        result.hi = ww;
        return result;
    }

    // set component

    template <int Index>
    static inline float64x4 set_component(float64x4 a, double s)
    {
        static_assert(Index < 4, "Index out of range.");
        switch (Index)
        {
            case 0: a.lo = set_component<0>(a.lo, s); break;
            case 1: a.lo = set_component<1>(a.lo, s); break;
            case 2: a.hi = set_component<0>(a.hi, s); break;
            case 3: a.hi = set_component<1>(a.hi, s); break;
        }
        return a;
    }

    // get component

    template <int Index>
    static inline double get_component(float64x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        double s = 0.0;
        switch (Index)
        {
            case 0: s = get_component<0>(a.lo); break;
            case 1: s = get_component<1>(a.lo); break;
            case 2: s = get_component<0>(a.hi); break;
            case 3: s = get_component<1>(a.hi); break;
        }
        return s;
    }

    static inline float64x4 float64x4_zero()
    {
        float64x4 result;
        result.lo = float64x2_zero();
        result.hi = float64x2_zero();
        return result;
    }

    static inline float64x4 float64x4_set1(double s)
    {
        float64x4 result;
        result.lo = float64x2_set1(s);
        result.hi = float64x2_set1(s);
        return result;
    }

    static inline float64x4 float64x4_set4(double x, double y, double z, double w)
    {
        float64x4 result;
        result.lo = float64x2_set2(x, y);
        result.hi = float64x2_set2(z, w);
        return result;
    }

    static inline float64x4 float64x4_uload(const double* source)
    {
        float64x4 result;
        result.lo = float64x2_uload(source + 0);
        result.hi = float64x2_uload(source + 2);
        return result;
    }

    static inline void float64x4_ustore(double* dest, float64x4 a)
    {
        float64x2_ustore(dest + 0, a.lo);
        float64x2_ustore(dest + 2, a.hi);
    }

    static inline float64x4 movelh(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = a.lo;
        result.hi = b.lo;
        return result;
    }

    static inline float64x4 movehl(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = b.hi;
        result.hi = a.hi;
        return result;
    }

    static inline float64x4 unpackhi(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline float64x4 unpacklo(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline float64x4 bitwise_nand(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline float64x4 bitwise_and(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline float64x4 bitwise_or(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline float64x4 bitwise_xor(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline float64x4 bitwise_not(float64x4 a)
    {
        float64x4 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    static inline float64x4 min(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline float64x4 max(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    static inline float64x4 hmin(float64x4 a)
    {
        const float64x2 xy = min(a.lo, shuffle<1, 0>(a.lo));
        const float64x2 zw = min(a.hi, shuffle<1, 0>(a.hi));
        const float64x2 s = min(xy, zw);
        float64x4 result;
        result.lo = s;
        result.hi = s;
        return result;
    }

    static inline float64x4 hmax(float64x4 a)
    {
        const float64x2 xy = max(a.lo, shuffle<1, 0>(a.lo));
        const float64x2 zw = max(a.hi, shuffle<1, 0>(a.hi));
        const float64x2 s = max(xy, zw);
        float64x4 result;
        result.lo = s;
        result.hi = s;
        return result;
    }

    static inline float64x4 abs(float64x4 a)
    {
        float64x4 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline float64x4 neg(float64x4 a)
    {
        float64x4 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    static inline float64x4 sign(float64x4 a)
    {
        float64x4 result;
        result.lo = sign(a.lo);
        result.hi = sign(a.hi);
        return result;
    }

    static inline float64x4 add(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline float64x4 sub(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline float64x4 mul(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = mul(a.lo, b.lo);
        result.hi = mul(a.hi, b.hi);
        return result;
    }

    static inline float64x4 div(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = div(a.lo, b.lo);
        result.hi = div(a.hi, b.hi);
        return result;
    }

    static inline float64x4 div(float64x4 a, double b)
    {
        float64x4 result;
        result.lo = div(a.lo, b);
        result.hi = div(a.hi, b);
        return result;
    }

    static inline float64x4 madd(float64x4 a, float64x4 b, float64x4 c)
    {
        float64x4 result;
        result.lo = madd(a.lo, b.lo, c.lo);
        result.hi = madd(a.hi, b.hi, c.hi);
        return result;
    }

    static inline float64x4 msub(float64x4 a, float64x4 b, float64x4 c)
    {
        float64x4 result;
        result.lo = msub(a.lo, b.lo, c.lo);
        result.hi = msub(a.hi, b.hi, c.hi);
        return result;
    }

    static inline float64x4 fast_rcp(float64x4 a)
    {
        float64x4 result;
        result.lo = fast_rcp(a.lo);
        result.hi = fast_rcp(a.hi);
        return result;
    }

    static inline float64x4 fast_rsqrt(float64x4 a)
    {
        float64x4 result;
        result.lo = fast_rsqrt(a.lo);
        result.hi = fast_rsqrt(a.hi);
        return result;
    }

    static inline float64x4 fast_sqrt(float64x4 a)
    {
        float64x4 result;
        result.lo = fast_sqrt(a.lo);
        result.hi = fast_sqrt(a.hi);
        return result;
    }

    static inline float64x4 rcp(float64x4 a)
    {
        float64x4 result;
        result.lo = rcp(a.lo);
        result.hi = rcp(a.hi);
        return result;
    }

    static inline float64x4 rsqrt(float64x4 a)
    {
        float64x4 result;
        result.lo = rsqrt(a.lo);
        result.hi = rsqrt(a.hi);
        return result;
    }

    static inline float64x4 sqrt(float64x4 a)
    {
        float64x4 result;
        result.lo = sqrt(a.lo);
        result.hi = sqrt(a.hi);
        return result;
    }

    static inline double dot4(float64x4 a, float64x4 b)
    {
        double low = dot2(a.lo, b.lo);
        double high = dot2(a.hi, b.hi);
        return low + high;
    }

    // compare

    static inline mask64x4 compare_neq(float64x4 a, float64x4 b)
    {
        mask64x4 result;
        result.lo = compare_neq(a.lo, b.lo);
        result.hi = compare_neq(a.hi, b.hi);
        return result;
    }

    static inline mask64x4 compare_eq(float64x4 a, float64x4 b)
    {
        mask64x4 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline mask64x4 compare_lt(float64x4 a, float64x4 b)
    {
        mask64x4 result;
        result.lo = compare_lt(a.lo, b.lo);
        result.hi = compare_lt(a.hi, b.hi);
        return result;
    }

    static inline mask64x4 compare_le(float64x4 a, float64x4 b)
    {
        mask64x4 result;
        result.lo = compare_le(a.lo, b.lo);
        result.hi = compare_le(a.hi, b.hi);
        return result;
    }

    static inline mask64x4 compare_gt(float64x4 a, float64x4 b)
    {
        mask64x4 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline mask64x4 compare_ge(float64x4 a, float64x4 b)
    {
        mask64x4 result;
        result.lo = compare_ge(a.lo, b.lo);
        result.hi = compare_ge(a.hi, b.hi);
        return result;
    }

    static inline float64x4 select(mask64x4 mask, float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    // rounding

    static inline float64x4 round(float64x4 s)
    {
        float64x4 result;
        result.lo = round(s.lo);
        result.hi = round(s.hi);
        return result;
    }

    static inline float64x4 trunc(float64x4 s)
    {
        float64x4 result;
        result.lo = trunc(s.lo);
        result.hi = trunc(s.hi);
        return result;
    }

    static inline float64x4 floor(float64x4 s)
    {
        float64x4 result;
        result.lo = floor(s.lo);
        result.hi = floor(s.hi);
        return result;
    }

    static inline float64x4 ceil(float64x4 s)
    {
        float64x4 result;
        result.lo = ceil(s.lo);
        result.hi = ceil(s.hi);
        return result;
    }

    static inline float64x4 fract(float64x4 s)
    {
        float64x4 result;
        result.lo = fract(s.lo);
        result.hi = fract(s.hi);
        return result;
    }

} // namespace simd
} // namespace mango
