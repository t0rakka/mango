/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifdef MANGO_INCLUDE_SIMD

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

    static inline float32x8 float32x8_unpackhi(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_unpackhi_ps(a.lo, b.lo);
        result.hi = _mm_unpackhi_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_unpacklo(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_unpacklo_ps(a.lo, b.lo);
        result.hi = _mm_unpacklo_ps(a.hi, b.hi);
        return result;
    }

    // logical

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

    static inline float32x8 float32x8_min(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_min_ps(a.lo, b.lo);
        result.hi = _mm_min_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_max(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_max_ps(a.lo, b.lo);
        result.hi = _mm_max_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_abs(float32x8 a)
    {
        const __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
        float32x8 result;
        result.lo = _mm_and_ps(a.lo, mask);
        result.hi = _mm_and_ps(a.hi, mask);
        return result;
    }

    static inline float32x8 float32x8_neg(float32x8 a)
    {
        const __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
        float32x8 result;
        result.lo = _mm_xor_ps(a.lo, mask);
        result.hi = _mm_xor_ps(a.hi, mask);
        return result;
    }

    static inline float32x8 float32x8_add(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_add_ps(a.lo, b.lo);
        result.hi = _mm_add_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_sub(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_sub_ps(a.lo, b.lo);
        result.hi = _mm_sub_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_mul(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_mul_ps(a.lo, b.lo);
        result.hi = _mm_mul_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_div(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_div_ps(a.lo, b.lo);
        result.hi = _mm_div_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_div(float32x8 a, float b)
    {
        __m128 scale = _mm_set1_ps(b);
        float32x8 result;
        result.lo = _mm_div_ps(a.lo, scale);
        result.hi = _mm_div_ps(a.hi, scale);
        return result;
    }

    static inline float32x8 float32x8_fast_reciprocal(float32x8 a)
    {
        float32x8 result;
        result.lo = _mm_rcp_ps(a.lo);
        result.hi = _mm_rcp_ps(a.hi);
        return result;
    }

    static inline float32x8 float32x8_fast_rsqrt(float32x8 a)
    {
        float32x8 result;
        result.lo = _mm_rsqrt_ps(a.lo);
        result.hi = _mm_rsqrt_ps(a.hi);
        return result;
    }

    static inline float32x8 float32x8_fast_sqrt(float32x8 a)
    {
        float32x8 result;
        result.lo = _mm_mul_ps(a.lo, _mm_rsqrt_ps(a.lo));
        result.hi = _mm_mul_ps(a.hi, _mm_rsqrt_ps(a.hi));
        return result;
    }

    static inline float32x8 float32x8_reciprocal(float32x8 a)
    {
        float32x8 result;
        result.lo = float32x4_reciprocal(a.lo);
        result.hi = float32x4_reciprocal(a.hi);
        return result;
    }

    static inline float32x8 float32x8_rsqrt(float32x8 a)
    {
        float32x8 result;
        result.lo = float32x4_rsqrt(a.lo);
        result.hi = float32x4_rsqrt(a.hi);
        return result;
    }

    static inline float32x8 float32x8_sqrt(float32x8 a)
    {
        float32x8 result;
        result.lo = _mm_sqrt_ps(a.lo);
        result.hi = _mm_sqrt_ps(a.hi);
        return result;
    }

    // compare

    static inline float32x8 float32x8_compare_neq(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_cmpneq_ps(a.lo, b.lo);
        result.hi = _mm_cmpneq_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_compare_eq(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_cmpeq_ps(a.lo, b.lo);
        result.hi = _mm_cmpeq_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_compare_lt(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_cmplt_ps(a.lo, b.lo);
        result.hi = _mm_cmplt_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_compare_le(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_cmple_ps(a.lo, b.lo);
        result.hi = _mm_cmple_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_compare_gt(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_cmpgt_ps(a.lo, b.lo);
        result.hi = _mm_cmpgt_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_compare_ge(float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = _mm_cmpge_ps(a.lo, b.lo);
        result.hi = _mm_cmpge_ps(a.hi, b.hi);
        return result;
    }

    static inline float32x8 float32x8_select(float32x8 mask, float32x8 a, float32x8 b)
    {
        float32x8 result;
        result.lo = float32x4_select(mask.lo, a.lo, b.lo);
        result.hi = float32x4_select(mask.hi, a.hi, b.hi);
        return result;
    }

    // rounding

    static inline float32x8 float32x8_round(float32x8 a)
    {
        float32x8 result;
        result.lo = float32x4_round(a.lo);
        result.hi = float32x4_round(a.hi);
        return result;
    }

    static inline float32x8 float32x8_trunc(float32x8 a)
    {
        float32x8 result;
        result.lo = float32x4_trunc(a.lo);
        result.hi = float32x4_trunc(a.hi);
        return result;
    }

    static inline float32x8 float32x8_floor(float32x8 a)
    {
        float32x8 result;
        result.lo = float32x4_floor(a.lo);
        result.hi = float32x4_floor(a.hi);
        return result;
    }

    static inline float32x8 float32x8_ceil(float32x8 a)
    {
        float32x8 result;
        result.lo = float32x4_ceil(a.lo);
        result.hi = float32x4_ceil(a.hi);
        return result;
    }

    static inline float32x8 float32x8_fract(float32x8 a)
    {
        float32x8 result;
        result.lo = float32x4_fract(a.lo);
        result.hi = float32x4_fract(a.hi);
        return result;
    }

#endif // MANGO_INCLUDE_SIMD
