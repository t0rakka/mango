/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // f32x16
    // -----------------------------------------------------------------

    static inline f32x16 f32x16_zero()
    {
        f32x16 result;
        result.lo = f32x8_zero();
        result.hi = f32x8_zero();
        return result;
    }

    static inline f32x16 f32x16_set1(f32 s)
    {
        f32x16 result;
        result.lo = f32x8_set1(s);
        result.hi = f32x8_set1(s);
        return result;
    }

    static inline f32x16 f32x16_set16(f32 s0, f32 s1, f32 s2, f32 s3, f32 s4, f32 s5, f32 s6, f32 s7,
        f32 s8, f32 s9, f32 s10, f32 s11, f32 s12, f32 s13, f32 s14, f32 s15)
    {
        f32x16 result;
        result.lo = f32x8_set8(s0, s1, s2, s3, s4, s5, s6, s7);
        result.hi = f32x8_set8(s8, s9, s10, s11, s12, s13, s14, s15);
        return result;
    }

    static inline f32x16 f32x16_uload(const f32* source)
    {
        f32x16 result;
        result.lo = f32x8_uload(source + 0);
        result.hi = f32x8_uload(source + 8);
        return result;
    }

    static inline void f32x16_ustore(f32* dest, f32x16 a)
    {
        f32x8_ustore(dest + 0, a.lo);
        f32x8_ustore(dest + 8, a.hi);
    }

    static inline f32x16 unpackhi(f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline f32x16 unpacklo(f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline f32x16 bitwise_nand(f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline f32x16 bitwise_and(f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline f32x16 bitwise_or(f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline f32x16 bitwise_xor(f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline f32x16 bitwise_not(f32x16 a)
    {
        f32x16 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    static inline f32x16 min(f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline f32x16 max(f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    static inline f32x16 abs(f32x16 a)
    {
        f32x16 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline f32x16 neg(f32x16 a)
    {
        f32x16 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    static inline f32x16 sign(f32x16 a)
    {
        f32x16 result;
        result.lo = sign(a.lo);
        result.hi = sign(a.hi);
        return result;
    }

    static inline f32x16 add(f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline f32x16 sub(f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline f32x16 mul(f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = mul(a.lo, b.lo);
        result.hi = mul(a.hi, b.hi);
        return result;
    }

    static inline f32x16 div(f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = div(a.lo, b.lo);
        result.hi = div(a.hi, b.hi);
        return result;
    }

    static inline f32x16 div(f32x16 a, f32 b)
    {
        f32x16 result;
        result.lo = div(a.lo, b);
        result.hi = div(a.hi, b);
        return result;
    }

    static inline f32x16 madd(f32x16 a, f32x16 b, f32x16 c)
    {
        f32x16 result;
        result.lo = madd(a.lo, b.lo, c.lo);
        result.hi = madd(a.hi, b.hi, c.hi);
        return result;
    }

    static inline f32x16 msub(f32x16 a, f32x16 b, f32x16 c)
    {
        f32x16 result;
        result.lo = msub(a.lo, b.lo, c.lo);
        result.hi = msub(a.hi, b.hi, c.hi);
        return result;
    }

    static inline f32x16 fast_rcp(f32x16 a)
    {
        f32x16 result;
        result.lo = fast_rcp(a.lo);
        result.hi = fast_rcp(a.hi);
        return result;
    }

    static inline f32x16 fast_rsqrt(f32x16 a)
    {
        f32x16 result;
        result.lo = fast_rsqrt(a.lo);
        result.hi = fast_rsqrt(a.hi);
        return result;
    }

    static inline f32x16 fast_sqrt(f32x16 a)
    {
        f32x16 result;
        result.lo = fast_sqrt(a.lo);
        result.hi = fast_sqrt(a.hi);
        return result;
    }

    static inline f32x16 rcp(f32x16 a)
    {
        f32x16 result;
        result.lo = rcp(a.lo);
        result.hi = rcp(a.hi);
        return result;
    }

    static inline f32x16 rsqrt(f32x16 a)
    {
        f32x16 result;
        result.lo = rsqrt(a.lo);
        result.hi = rsqrt(a.hi);
        return result;
    }

    static inline f32x16 sqrt(f32x16 a)
    {
        f32x16 result;
        result.lo = sqrt(a.lo);
        result.hi = sqrt(a.hi);
        return result;
    }

    // compare

    static inline mask32x16 compare_neq(f32x16 a, f32x16 b)
    {
        mask32x16 result;
        result.lo = compare_neq(a.lo, b.lo);
        result.hi = compare_neq(a.hi, b.hi);
        return result;
    }

    static inline mask32x16 compare_eq(f32x16 a, f32x16 b)
    {
        mask32x16 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline mask32x16 compare_lt(f32x16 a, f32x16 b)
    {
        mask32x16 result;
        result.lo = compare_lt(a.lo, b.lo);
        result.hi = compare_lt(a.hi, b.hi);
        return result;
    }

    static inline mask32x16 compare_le(f32x16 a, f32x16 b)
    {
        mask32x16 result;
        result.lo = compare_le(a.lo, b.lo);
        result.hi = compare_le(a.hi, b.hi);
        return result;
    }

    static inline mask32x16 compare_gt(f32x16 a, f32x16 b)
    {
        mask32x16 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline mask32x16 compare_ge(f32x16 a, f32x16 b)
    {
        mask32x16 result;
        result.lo = compare_ge(a.lo, b.lo);
        result.hi = compare_ge(a.hi, b.hi);
        return result;
    }

    static inline f32x16 select(mask32x16 mask, f32x16 a, f32x16 b)
    {
        f32x16 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    // rounding

    static inline f32x16 round(f32x16 a)
    {
        f32x16 result;
        result.lo = round(a.lo);
        result.hi = round(a.hi);
        return result;
    }

    static inline f32x16 trunc(f32x16 a)
    {
        f32x16 result;
        result.lo = trunc(a.lo);
        result.hi = trunc(a.hi);
        return result;
    }

    static inline f32x16 floor(f32x16 a)
    {
        f32x16 result;
        result.lo = floor(a.lo);
        result.hi = floor(a.hi);
        return result;
    }

    static inline f32x16 ceil(f32x16 a)
    {
        f32x16 result;
        result.lo = ceil(a.lo);
        result.hi = ceil(a.hi);
        return result;
    }

    static inline f32x16 fract(f32x16 a)
    {
        f32x16 result;
        result.lo = fract(a.lo);
        result.hi = fract(a.hi);
        return result;
    }

} // namespace simd
} // namespace mango
