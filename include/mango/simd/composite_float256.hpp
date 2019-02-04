/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // f32x8
    // -----------------------------------------------------------------

    static inline f32x8 f32x8_zero()
    {
        f32x8 result;
        result.lo = f32x4_zero();
        result.hi = f32x4_zero();
        return result;
    }

    static inline f32x8 f32x8_set1(float s)
    {
        f32x8 result;
        result.lo = f32x4_set1(s);
        result.hi = f32x4_set1(s);
        return result;
    }

    static inline f32x8 f32x8_set8(float s0, float s1, float s2, float s3, float s4, float s5, float s6, float s7)
    {
        f32x8 result;
        result.lo = f32x4_set4(s0, s1, s2, s3);
        result.hi = f32x4_set4(s4, s5, s6, s7);
        return result;
    }

    static inline f32x8 f32x8_uload(const float* source)
    {
        f32x8 result;
        result.lo = f32x4_uload(source + 0);
        result.hi = f32x4_uload(source + 4);
        return result;
    }

    static inline void f32x8_ustore(float* dest, f32x8 a)
    {
        f32x4_ustore(dest + 0, a.lo);
        f32x4_ustore(dest + 4, a.hi);
    }

    static inline f32x8 unpackhi(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline f32x8 unpacklo(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline f32x8 bitwise_nand(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline f32x8 bitwise_and(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline f32x8 bitwise_or(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline f32x8 bitwise_xor(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline f32x8 bitwise_not(f32x8 a)
    {
        f32x8 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    static inline f32x8 min(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline f32x8 max(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    static inline f32x8 abs(f32x8 a)
    {
        f32x8 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline f32x8 neg(f32x8 a)
    {
        f32x8 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    static inline f32x8 sign(f32x8 a)
    {
        f32x8 result;
        result.lo = sign(a.lo);
        result.hi = sign(a.hi);
        return result;
    }

    static inline f32x8 add(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline f32x8 sub(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline f32x8 mul(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = mul(a.lo, b.lo);
        result.hi = mul(a.hi, b.hi);
        return result;
    }

    static inline f32x8 div(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = div(a.lo, b.lo);
        result.hi = div(a.hi, b.hi);
        return result;
    }

    static inline f32x8 div(f32x8 a, float b)
    {
        f32x8 result;
        result.lo = div(a.lo, b);
        result.hi = div(a.hi, b);
        return result;
    }

    static inline f32x8 hadd(f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = hadd(a.lo, b.lo);
        result.hi = hadd(a.hi, b.hi);
        return result;
    }

    static inline f32x8 madd(f32x8 a, f32x8 b, f32x8 c)
    {
        f32x8 result;
        result.lo = madd(a.lo, b.lo, c.lo);
        result.hi = madd(a.hi, b.hi, c.hi);
        return result;
    }

    static inline f32x8 msub(f32x8 a, f32x8 b, f32x8 c)
    {
        f32x8 result;
        result.lo = msub(a.lo, b.lo, c.lo);
        result.hi = msub(a.hi, b.hi, c.hi);
        return result;
    }

    static inline f32x8 fast_rcp(f32x8 a)
    {
        f32x8 result;
        result.lo = fast_rcp(a.lo);
        result.hi = fast_rcp(a.hi);
        return result;
    }

    static inline f32x8 fast_rsqrt(f32x8 a)
    {
        f32x8 result;
        result.lo = fast_rsqrt(a.lo);
        result.hi = fast_rsqrt(a.hi);
        return result;
    }

    static inline f32x8 fast_sqrt(f32x8 a)
    {
        f32x8 result;
        result.lo = fast_sqrt(a.lo);
        result.hi = fast_sqrt(a.hi);
        return result;
    }

    static inline f32x8 rcp(f32x8 a)
    {
        f32x8 result;
        result.lo = rcp(a.lo);
        result.hi = rcp(a.hi);
        return result;
    }

    static inline f32x8 rsqrt(f32x8 a)
    {
        f32x8 result;
        result.lo = rsqrt(a.lo);
        result.hi = rsqrt(a.hi);
        return result;
    }

    static inline f32x8 sqrt(f32x8 a)
    {
        f32x8 result;
        result.lo = sqrt(a.lo);
        result.hi = sqrt(a.hi);
        return result;
    }

    // compare

    static inline mask32x8 compare_neq(f32x8 a, f32x8 b)
    {
        mask32x8 result;
        result.lo = compare_neq(a.lo, b.lo);
        result.hi = compare_neq(a.hi, b.hi);
        return result;
    }

    static inline mask32x8 compare_eq(f32x8 a, f32x8 b)
    {
        mask32x8 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline mask32x8 compare_lt(f32x8 a, f32x8 b)
    {
        mask32x8 result;
        result.lo = compare_lt(a.lo, b.lo);
        result.hi = compare_lt(a.hi, b.hi);
        return result;
    }

    static inline mask32x8 compare_le(f32x8 a, f32x8 b)
    {
        mask32x8 result;
        result.lo = compare_le(a.lo, b.lo);
        result.hi = compare_le(a.hi, b.hi);
        return result;
    }

    static inline mask32x8 compare_gt(f32x8 a, f32x8 b)
    {
        mask32x8 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline mask32x8 compare_ge(f32x8 a, f32x8 b)
    {
        mask32x8 result;
        result.lo = compare_ge(a.lo, b.lo);
        result.hi = compare_ge(a.hi, b.hi);
        return result;
    }

    static inline f32x8 select(mask32x8 mask, f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    // rounding

    static inline f32x8 round(f32x8 a)
    {
        f32x8 result;
        result.lo = round(a.lo);
        result.hi = round(a.hi);
        return result;
    }

    static inline f32x8 trunc(f32x8 a)
    {
        f32x8 result;
        result.lo = trunc(a.lo);
        result.hi = trunc(a.hi);
        return result;
    }

    static inline f32x8 floor(f32x8 a)
    {
        f32x8 result;
        result.lo = floor(a.lo);
        result.hi = floor(a.hi);
        return result;
    }

    static inline f32x8 ceil(f32x8 a)
    {
        f32x8 result;
        result.lo = ceil(a.lo);
        result.hi = ceil(a.hi);
        return result;
    }

    static inline f32x8 fract(f32x8 a)
    {
        f32x8 result;
        result.lo = fract(a.lo);
        result.hi = fract(a.hi);
        return result;
    }

} // namespace simd
} // namespace mango
