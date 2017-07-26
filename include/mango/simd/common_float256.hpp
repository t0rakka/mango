/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float32x8
    // -----------------------------------------------------------------

    static inline float32x8 float32x8_zero()
    {
        float32x8 result;
        result.lo = float32x4_zero();
        result.hi = float32x4_zero();
        return result;
    }

    static inline float32x8 float32x8_set1(float s)
    {
        float32x8 result;
        result.lo = float32x4_set1(s);
        result.hi = float32x4_set1(s);
        return result;
    }

    static inline float32x8 float32x8_set8(float s0, float s1, float s2, float s3, float s4, float s5, float s6, float s7)
    {
        float32x8 result;
        result.lo = float32x4_set4(s0, s1, s2, s3);
        result.hi = float32x4_set4(s4, s5, s6, s7);
        return result;
    }

    static inline float32x8 float32x8_uload(const float* source)
    {
        float32x8 result;
        result.lo = float32x4_uload(source + 0);
        result.hi = float32x4_uload(source + 4);
        return result;
    }

    static inline void float32x8_ustore(float* dest, float32x8 a)
    {
        float32x4_ustore(dest + 0, a.lo);
        float32x4_ustore(dest + 4, a.hi);
    }

    static inline float32x8 unpackhi(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline float32x8 unpacklo(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline float32x8 bitwise_nand(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline float32x8 bitwise_and(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline float32x8 bitwise_or(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline float32x8 bitwise_xor(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline float32x8 bitwise_not(float32x8 a)
    {
        float32x8 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    static inline float32x8 min(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline float32x8 max(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    static inline float32x8 abs(float32x8 a)
    {
        float32x8 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline float32x8 neg(float32x8 a)
    {
        float32x8 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    static inline float32x8 add(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline float32x8 sub(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline float32x8 mul(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = mul(a.lo, b.lo);
        result.hi = mul(a.hi, b.hi);
        return result;
    }

    static inline float32x8 div(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = div(a.lo, b.lo);
        result.hi = div(a.hi, b.hi);
        return result;
    }

    static inline float32x8 div(float32x8 a, float b)
    {
        float32x8 result;
        result.lo = div(a.lo, b);
        result.hi = div(a.hi, b);
        return result;
    }

    static inline float32x8 hadd(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = hadd(a.lo, b.lo);
        result.hi = hadd(a.hi, b.hi);
        return result;
    }

    static inline float32x8 madd(float32x8 a, float32x8 b, float32x8 c)
    {
        float32x8 result;
        result.lo = madd(b.lo, c.lo, a.lo);
        result.hi = madd(b.hi, c.hi, a.hi);
        return result;
    }

    static inline float32x8 msub(float32x8 a, float32x8 b, float32x8 c)
    {
        float32x8 result;
        result.lo = msub(b.lo, c.lo, a.lo);
        result.hi = msub(b.hi, c.hi, a.hi);
        return result;
    }

    static inline float32x8 fast_rcp(float32x8 a)
    {
        float32x8 result;
        result.lo = fast_rcp(a.lo);
        result.hi = fast_rcp(a.hi);
        return result;
    }

    static inline float32x8 fast_rsqrt(float32x8 a)
    {
        float32x8 result;
        result.lo = fast_rsqrt(a.lo);
        result.hi = fast_rsqrt(a.hi);
        return result;
    }

    static inline float32x8 fast_sqrt(float32x8 a)
    {
        float32x8 result;
        result.lo = fast_sqrt(a.lo);
        result.hi = fast_sqrt(a.hi);
        return result;
    }

    static inline float32x8 rcp(float32x8 a)
    {
        float32x8 result;
        result.lo = rcp(a.lo);
        result.hi = rcp(a.hi);
        return result;
    }

    static inline float32x8 rsqrt(float32x8 a)
    {
        float32x8 result;
        result.lo = rsqrt(a.lo);
        result.hi = rsqrt(a.hi);
        return result;
    }

    static inline float32x8 sqrt(float32x8 a)
    {
        float32x8 result;
        result.lo = sqrt(a.lo);
        result.hi = sqrt(a.hi);
        return result;
    }

    // compare

    static inline float32x8::mask compare_neq(float32x8 a, float32x8 b)
    {
        float32x8::mask result;
        result.lo = compare_neq(a.lo, b.lo);
        result.hi = compare_neq(a.hi, b.hi);
        return result;
    }

    static inline float32x8::mask compare_eq(float32x8 a, float32x8 b)
    {
        float32x8::mask result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline float32x8::mask compare_lt(float32x8 a, float32x8 b)
    {
        float32x8::mask result;
        result.lo = compare_lt(a.lo, b.lo);
        result.hi = compare_lt(a.hi, b.hi);
        return result;
    }

    static inline float32x8::mask compare_le(float32x8 a, float32x8 b)
    {
        float32x8::mask result;
        result.lo = compare_le(a.lo, b.lo);
        result.hi = compare_le(a.hi, b.hi);
        return result;
    }

    static inline float32x8::mask compare_gt(float32x8 a, float32x8 b)
    {
        float32x8::mask result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline float32x8::mask compare_ge(float32x8 a, float32x8 b)
    {
        float32x8::mask result;
        result.lo = compare_ge(a.lo, b.lo);
        result.hi = compare_ge(a.hi, b.hi);
        return result;
    }

    static inline float32x8 select(float32x8::mask mask, float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    static inline uint32 get_mask(float32x8::mask a)
    {
        uint32 mask = get_mask(a.lo) | (get_mask(a.hi) << 4);
        return mask;
    }

    // rounding

    static inline float32x8 round(float32x8 a)
    {
        float32x8 result;
        result.lo = round(a.lo);
        result.hi = round(a.hi);
        return result;
    }

    static inline float32x8 trunc(float32x8 a)
    {
        float32x8 result;
        result.lo = trunc(a.lo);
        result.hi = trunc(a.hi);
        return result;
    }

    static inline float32x8 floor(float32x8 a)
    {
        float32x8 result;
        result.lo = floor(a.lo);
        result.hi = floor(a.hi);
        return result;
    }

    static inline float32x8 ceil(float32x8 a)
    {
        float32x8 result;
        result.lo = ceil(a.lo);
        result.hi = ceil(a.hi);
        return result;
    }

    static inline float32x8 fract(float32x8 a)
    {
        float32x8 result;
        result.lo = fract(a.lo);
        result.hi = fract(a.hi);
        return result;
    }

} // namespace simd
} // namespace mango
