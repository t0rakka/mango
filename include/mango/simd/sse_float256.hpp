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
        result.lo = _mm_setzero_ps();
        result.hi = _mm_setzero_ps();
        return result;
    }

    static inline float32x8 float32x8_set1(float s)
    {
        float32x8 result;
        result.lo = _mm_set1_ps(s);
        result.hi = _mm_set1_ps(s);
        return result;
    }

    static inline float32x8 float32x8_set8(float s0, float s1, float s2, float s3, float s4, float s5, float s6, float s7)
    {
        float32x8 result;
        result.lo = _mm_setr_ps(s0, s1, s2, s3);
        result.hi = _mm_setr_ps(s4, s5, s6, s7);
        return result;
    }

    static inline float32x8 float32x8_uload(const float* source)
    {
        float32x8 result;
        result.lo = _mm_loadu_ps(source + 0);
        result.hi = _mm_loadu_ps(source + 4);
        return result;
    }

    static inline void float32x8_ustore(float* dest, float32x8 a)
    {
        _mm_storeu_ps(dest + 0, a.lo);
        _mm_storeu_ps(dest + 4, a.hi);
    }

    static inline float32x8 unpackhi(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_unpackhi_ps(a.lo, b.lo);
        result.hi = _mm_unpackhi_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 unpacklo(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_unpacklo_ps(a.lo, b.lo);
        result.hi = _mm_unpacklo_ps(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline float32x8 float32x8_and(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_and_ps(a.lo, b.lo);
        result.hi = _mm_and_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_nand(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_andnot_ps(a.lo, b.lo);
        result.hi = _mm_andnot_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_or(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_or_ps(a.lo, b.lo);
        result.hi = _mm_or_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_xor(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_xor_ps(a.lo, b.lo);
        result.hi = _mm_xor_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 min(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_min_ps(a.lo, b.lo);
        result.hi = _mm_min_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 max(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_max_ps(a.lo, b.lo);
        result.hi = _mm_max_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 abs(float32x8 a)
    {
        const __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
        float32x8 result;
        result.lo = _mm_and_ps(a.lo, mask);
        result.hi = _mm_and_ps(a.hi, mask);
        return result;
    }

    static inline float32x8 neg(float32x8 a)
    {
        const __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
        float32x8 result;
        result.lo = _mm_xor_ps(a.lo, mask);
        result.hi = _mm_xor_ps(a.hi, mask);
        return result;
    }

    static inline float32x8 add(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_add_ps(a.lo, b.lo);
        result.hi = _mm_add_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 sub(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_sub_ps(a.lo, b.lo);
        result.hi = _mm_sub_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 mul(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_mul_ps(a.lo, b.lo);
        result.hi = _mm_mul_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 div(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_div_ps(a.lo, b.lo);
        result.hi = _mm_div_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 div(float32x8 a, float b)
    {
        __m128 scale = _mm_set1_ps(b);
        float32x8 result;
        result.lo = _mm_div_ps(a.lo, scale);
        result.hi = _mm_div_ps(a.hi, scale);
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

    static inline float32x8 fast_reciprocal(float32x8 a)
    {
        float32x8 result;
        result.lo = _mm_rcp_ps(a.lo);
        result.hi = _mm_rcp_ps(a.hi);
        return result;
    }

    static inline float32x8 fast_rsqrt(float32x8 a)
    {
        float32x8 result;
        result.lo = _mm_rsqrt_ps(a.lo);
        result.hi = _mm_rsqrt_ps(a.hi);
        return result;
    }

    static inline float32x8 fast_sqrt(float32x8 a)
    {
        float32x8 result;
        result.lo = _mm_mul_ps(a.lo, _mm_rsqrt_ps(a.lo));
        result.hi = _mm_mul_ps(a.hi, _mm_rsqrt_ps(a.hi));
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
        result.lo = _mm_sqrt_ps(a.lo);
        result.hi = _mm_sqrt_ps(a.hi);
        return result;
    }

    // compare

    static inline float32x8 compare_neq(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_cmpneq_ps(a.lo, b.lo);
        result.hi = _mm_cmpneq_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 compare_eq(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_cmpeq_ps(a.lo, b.lo);
        result.hi = _mm_cmpeq_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 compare_lt(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_cmplt_ps(a.lo, b.lo);
        result.hi = _mm_cmplt_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 compare_le(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_cmple_ps(a.lo, b.lo);
        result.hi = _mm_cmple_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 compare_gt(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_cmpgt_ps(a.lo, b.lo);
        result.hi = _mm_cmpgt_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 compare_ge(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_cmpge_ps(a.lo, b.lo);
        result.hi = _mm_cmpge_ps(a.hi, b.hi);
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
