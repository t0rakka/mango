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
        result.lo = vdupq_n_f32(0.0f);
        result.hi = vdupq_n_f32(0.0f);
        return result;
    }

    static inline float32x8 float32x8_set1(float s)
    {
        float32x8 result;
        result.lo = vdupq_n_f32(s);
        result.hi = vdupq_n_f32(s);
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

    static inline float32x8 float32x8_and(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(a.lo), vreinterpretq_s32_f32(b.lo)));
        result.hi = vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(a.hi), vreinterpretq_s32_f32(b.hi)));
        return result;
    }

    static inline float32x8 float32x8_nand(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = vreinterpretq_f32_s32(vbicq_s32(vreinterpretq_s32_f32(a.lo), vreinterpretq_s32_f32(b.lo)));
        result.hi = vreinterpretq_f32_s32(vbicq_s32(vreinterpretq_s32_f32(a.hi), vreinterpretq_s32_f32(b.hi)));
        return result;
    }

    static inline float32x8 float32x8_or(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = vreinterpretq_f32_s32(vorrq_s32(vreinterpretq_s32_f32(a.lo), vreinterpretq_s32_f32(b.lo)));
        result.hi = vreinterpretq_f32_s32(vorrq_s32(vreinterpretq_s32_f32(a.hi), vreinterpretq_s32_f32(b.hi)));
        return result;
    }

    static inline float32x8 float32x8_xor(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = vreinterpretq_f32_s32(veorq_s32(vreinterpretq_s32_f32(a.lo), vreinterpretq_s32_f32(b.lo)));
        result.hi = vreinterpretq_f32_s32(veorq_s32(vreinterpretq_s32_f32(a.hi), vreinterpretq_s32_f32(b.hi)));
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
        result.lo = vmulq_f32(a.lo, b.lo);
        result.hi = vmulq_f32(a.hi, b.hi);
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
        result.lo = vmlaq_f32(a.lo, b.lo, c.lo);
        result.hi = vmlaq_f32(a.hi, b.hi, c.hi);
        return result;
    }

    static inline float32x8 msub(float32x8 a, float32x8 b, float32x8 c)
    {
        float32x8 result;
        result.lo = vmlsq_f32(a.lo, b.lo, c.lo);
        result.hi = vmlsq_f32(a.hi, b.hi, c.hi);
        return result;
    }

    static inline float32x8 fast_reciprocal(float32x8 a)
    {
        float32x8 result;
        result.lo = fast_reciprocal(a.lo);
        result.hi = fast_reciprocal(a.hi);
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

    static inline float32x8 reciprocal(float32x8 a)
    {
        float32x8 result;
        result.lo = reciprocal(a.lo);
        result.hi = reciprocal(a.hi);
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

    static inline float32x8 compare_neq(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = compare_neq(a.lo, b.lo);
        result.hi = compare_neq(a.hi, b.hi);
        return result;
    }

    static inline float32x8 compare_eq(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline float32x8 compare_lt(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = compare_lt(a.lo, b.lo);
        result.hi = compare_lt(a.hi, b.hi);
        return result;
    }

    static inline float32x8 compare_le(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = compare_le(a.lo, b.lo);
        result.hi = compare_le(a.hi, b.hi);
        return result;
    }

    static inline float32x8 compare_gt(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline float32x8 compare_ge(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = compare_ge(a.lo, b.lo);
        result.hi = compare_ge(a.hi, b.hi);
        return result;
    }

    static inline float32x8 select(float32x8 mask, float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
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
