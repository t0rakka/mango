/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float64x8
    // -----------------------------------------------------------------

    static inline float64x8 float64x8_zero()
    {
        float64x8 result;
        result.lo = float64x4_zero();
        result.hi = float64x4_zero();
        return result;
    }

    static inline float64x8 float64x8_set1(double s)
    {
        float64x8 result;
        result.lo = float64x4_set1(s);
        result.hi = float64x4_set1(s);
        return result;
    }

    static inline float64x8 float64x8_set8(double s0, double s1, double s2, double s3, double s4, double s5, double s6, double s7)
    {
        float64x8 result;
        result.lo = float64x4_set4(s0, s1, s2, s3);
        result.hi = float64x4_set4(s4, s5, s6, s7);
        return result;
    }

    static inline float64x8 float64x8_uload(const double* source)
    {
        float64x8 result;
        result.lo = float64x4_uload(source + 0);
        result.hi = float64x4_uload(source + 4);
        return result;
    }

    static inline void float64x8_ustore(double* dest, float64x8 a)
    {
        float64x4_ustore(dest + 0, a.lo);
        float64x4_ustore(dest + 4, a.hi);
    }

    static inline float64x8 unpackhi(float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline float64x8 unpacklo(float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline float64x8 bitwise_nand(float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline float64x8 bitwise_and(float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline float64x8 bitwise_or(float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline float64x8 bitwise_xor(float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline float64x8 bitwise_not(float64x8 a)
    {
        float64x8 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    static inline float64x8 min(float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline float64x8 max(float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    static inline float64x8 abs(float64x8 a)
    {
        float64x8 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline float64x8 neg(float64x8 a)
    {
        float64x8 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    static inline float64x8 sign(float64x8 a)
    {
        float64x8 result;
        result.lo = sign(a.lo);
        result.hi = sign(a.hi);
        return result;
    }

    static inline float64x8 add(float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline float64x8 sub(float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline float64x8 mul(float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = mul(a.lo, b.lo);
        result.hi = mul(a.hi, b.hi);
        return result;
    }

    static inline float64x8 div(float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = div(a.lo, b.lo);
        result.hi = div(a.hi, b.hi);
        return result;
    }

    static inline float64x8 div(float64x8 a, double b)
    {
        float64x8 result;
        result.lo = div(a.lo, b);
        result.hi = div(a.hi, b);
        return result;
    }

    static inline float64x8 madd(float64x8 a, float64x8 b, float64x8 c)
    {
        float64x8 result;
        result.lo = madd(a.lo, b.lo, c.lo);
        result.hi = madd(a.hi, b.hi, c.hi);
        return result;
    }

    static inline float64x8 msub(float64x8 a, float64x8 b, float64x8 c)
    {
        float64x8 result;
        result.lo = msub(a.lo, b.lo, c.lo);
        result.hi = msub(a.hi, b.hi, c.hi);
        return result;
    }

    static inline float64x8 fast_rcp(float64x8 a)
    {
        float64x8 result;
        result.lo = fast_rcp(a.lo);
        result.hi = fast_rcp(a.hi);
        return result;
    }

    static inline float64x8 fast_rsqrt(float64x8 a)
    {
        float64x8 result;
        result.lo = fast_rsqrt(a.lo);
        result.hi = fast_rsqrt(a.hi);
        return result;
    }

    static inline float64x8 fast_sqrt(float64x8 a)
    {
        float64x8 result;
        result.lo = fast_sqrt(a.lo);
        result.hi = fast_sqrt(a.hi);
        return result;
    }

    static inline float64x8 rcp(float64x8 a)
    {
        float64x8 result;
        result.lo = rcp(a.lo);
        result.hi = rcp(a.hi);
        return result;
    }

    static inline float64x8 rsqrt(float64x8 a)
    {
        float64x8 result;
        result.lo = rsqrt(a.lo);
        result.hi = rsqrt(a.hi);
        return result;
    }

    static inline float64x8 sqrt(float64x8 a)
    {
        float64x8 result;
        result.lo = sqrt(a.lo);
        result.hi = sqrt(a.hi);
        return result;
    }

    // compare

    static inline mask64x8 compare_neq(float64x8 a, float64x8 b)
    {
        mask64x8 result;
        result.lo = compare_neq(a.lo, b.lo);
        result.hi = compare_neq(a.hi, b.hi);
        return result;
    }

    static inline mask64x8 compare_eq(float64x8 a, float64x8 b)
    {
        mask64x8 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline mask64x8 compare_lt(float64x8 a, float64x8 b)
    {
        mask64x8 result;
        result.lo = compare_lt(a.lo, b.lo);
        result.hi = compare_lt(a.hi, b.hi);
        return result;
    }

    static inline mask64x8 compare_le(float64x8 a, float64x8 b)
    {
        mask64x8 result;
        result.lo = compare_le(a.lo, b.lo);
        result.hi = compare_le(a.hi, b.hi);
        return result;
    }

    static inline mask64x8 compare_gt(float64x8 a, float64x8 b)
    {
        mask64x8 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline mask64x8 compare_ge(float64x8 a, float64x8 b)
    {
        mask64x8 result;
        result.lo = compare_ge(a.lo, b.lo);
        result.hi = compare_ge(a.hi, b.hi);
        return result;
    }

    static inline float64x8 select(mask64x8 mask, float64x8 a, float64x8 b)
    {
        float64x8 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    // rounding

    static inline float64x8 round(float64x8 s)
    {
        float64x8 result;
        result.lo = round(s.lo);
        result.hi = round(s.hi);
        return result;
    }

    static inline float64x8 trunc(float64x8 s)
    {
        float64x8 result;
        result.lo = trunc(s.lo);
        result.hi = trunc(s.hi);
        return result;
    }

    static inline float64x8 floor(float64x8 s)
    {
        float64x8 result;
        result.lo = floor(s.lo);
        result.hi = floor(s.hi);
        return result;
    }

    static inline float64x8 ceil(float64x8 s)
    {
        float64x8 result;
        result.lo = ceil(s.lo);
        result.hi = ceil(s.hi);
        return result;
    }

    static inline float64x8 fract(float64x8 s)
    {
        float64x8 result;
        result.lo = fract(s.lo);
        result.hi = fract(s.hi);
        return result;
    }

} // namespace simd
} // namespace mango
