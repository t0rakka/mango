/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float32x16
    // -----------------------------------------------------------------

    static inline float32x16 float32x16_zero()
    {
        float32x16 result;
        result.lo = float32x8_zero();
        result.hi = float32x8_zero();
        return result;
    }

    static inline float32x16 float32x16_set1(float s)
    {
        float32x16 result;
        result.lo = float32x8_set1(s);
        result.hi = float32x8_set1(s);
        return result;
    }

    static inline float32x16 float32x16_set16(float s0, float s1, float s2, float s3, float s4, float s5, float s6, float s7,
        float s8, float s9, float s10, float s11, float s12, float s13, float s14, float s15)
    {
        float32x16 result;
        result.lo = float32x8_set8(s0, s1, s2, s3, s4, s5, s6, s7);
        result.hi = float32x8_set8(s8, s9, s10, s11, s12, s13, s14, s15);
        return result;
    }

    static inline float32x16 float32x16_uload(const float* source)
    {
        float32x16 result;
        result.lo = float32x8_uload(source + 0);
        result.hi = float32x8_uload(source + 8);
        return result;
    }

    static inline void float32x16_ustore(float* dest, float32x16 a)
    {
        float32x8_ustore(dest + 0, a.lo);
        float32x8_ustore(dest + 8, a.hi);
    }

    static inline float32x16 unpackhi(float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline float32x16 unpacklo(float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline float32x16 bitwise_nand(float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline float32x16 bitwise_and(float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline float32x16 bitwise_or(float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline float32x16 bitwise_xor(float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline float32x16 bitwise_not(float32x16 a)
    {
        float32x16 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    static inline float32x16 min(float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline float32x16 max(float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    static inline float32x16 abs(float32x16 a)
    {
        float32x16 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline float32x16 neg(float32x16 a)
    {
        float32x16 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    static inline float32x16 sign(float32x16 a)
    {
        float32x16 result;
        result.lo = sign(a.lo);
        result.hi = sign(a.hi);
        return result;
    }

    static inline float32x16 add(float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline float32x16 sub(float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline float32x16 mul(float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = mul(a.lo, b.lo);
        result.hi = mul(a.hi, b.hi);
        return result;
    }

    static inline float32x16 div(float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = div(a.lo, b.lo);
        result.hi = div(a.hi, b.hi);
        return result;
    }

    static inline float32x16 div(float32x16 a, float b)
    {
        float32x16 result;
        result.lo = div(a.lo, b);
        result.hi = div(a.hi, b);
        return result;
    }

    static inline float32x16 madd(float32x16 a, float32x16 b, float32x16 c)
    {
        float32x16 result;
        result.lo = madd(a.lo, b.lo, c.lo);
        result.hi = madd(a.hi, b.hi, c.hi);
        return result;
    }

    static inline float32x16 msub(float32x16 a, float32x16 b, float32x16 c)
    {
        float32x16 result;
        result.lo = msub(a.lo, b.lo, c.lo);
        result.hi = msub(a.hi, b.hi, c.hi);
        return result;
    }

    static inline float32x16 fast_rcp(float32x16 a)
    {
        float32x16 result;
        result.lo = fast_rcp(a.lo);
        result.hi = fast_rcp(a.hi);
        return result;
    }

    static inline float32x16 fast_rsqrt(float32x16 a)
    {
        float32x16 result;
        result.lo = fast_rsqrt(a.lo);
        result.hi = fast_rsqrt(a.hi);
        return result;
    }

    static inline float32x16 fast_sqrt(float32x16 a)
    {
        float32x16 result;
        result.lo = fast_sqrt(a.lo);
        result.hi = fast_sqrt(a.hi);
        return result;
    }

    static inline float32x16 rcp(float32x16 a)
    {
        float32x16 result;
        result.lo = rcp(a.lo);
        result.hi = rcp(a.hi);
        return result;
    }

    static inline float32x16 rsqrt(float32x16 a)
    {
        float32x16 result;
        result.lo = rsqrt(a.lo);
        result.hi = rsqrt(a.hi);
        return result;
    }

    static inline float32x16 sqrt(float32x16 a)
    {
        float32x16 result;
        result.lo = sqrt(a.lo);
        result.hi = sqrt(a.hi);
        return result;
    }

    // compare

    static inline mask32x16 compare_neq(float32x16 a, float32x16 b)
    {
        mask32x16 result;
        result.lo = compare_neq(a.lo, b.lo);
        result.hi = compare_neq(a.hi, b.hi);
        return result;
    }

    static inline mask32x16 compare_eq(float32x16 a, float32x16 b)
    {
        mask32x16 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline mask32x16 compare_lt(float32x16 a, float32x16 b)
    {
        mask32x16 result;
        result.lo = compare_lt(a.lo, b.lo);
        result.hi = compare_lt(a.hi, b.hi);
        return result;
    }

    static inline mask32x16 compare_le(float32x16 a, float32x16 b)
    {
        mask32x16 result;
        result.lo = compare_le(a.lo, b.lo);
        result.hi = compare_le(a.hi, b.hi);
        return result;
    }

    static inline mask32x16 compare_gt(float32x16 a, float32x16 b)
    {
        mask32x16 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline mask32x16 compare_ge(float32x16 a, float32x16 b)
    {
        mask32x16 result;
        result.lo = compare_ge(a.lo, b.lo);
        result.hi = compare_ge(a.hi, b.hi);
        return result;
    }

    static inline float32x16 select(mask32x16 mask, float32x16 a, float32x16 b)
    {
        float32x16 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    // rounding

    static inline float32x16 round(float32x16 a)
    {
        float32x16 result;
        result.lo = round(a.lo);
        result.hi = round(a.hi);
        return result;
    }

    static inline float32x16 trunc(float32x16 a)
    {
        float32x16 result;
        result.lo = trunc(a.lo);
        result.hi = trunc(a.hi);
        return result;
    }

    static inline float32x16 floor(float32x16 a)
    {
        float32x16 result;
        result.lo = floor(a.lo);
        result.hi = floor(a.hi);
        return result;
    }

    static inline float32x16 ceil(float32x16 a)
    {
        float32x16 result;
        result.lo = ceil(a.lo);
        result.hi = ceil(a.hi);
        return result;
    }

    static inline float32x16 fract(float32x16 a)
    {
        float32x16 result;
        result.lo = fract(a.lo);
        result.hi = fract(a.hi);
        return result;
    }

} // namespace simd
} // namespace mango
