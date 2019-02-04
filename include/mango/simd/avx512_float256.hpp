/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        return _mm256_setzero_ps();
    }

    static inline float32x8 float32x8_set1(float s)
    {
        return _mm256_set1_ps(s);
    }

    static inline float32x8 float32x8_set8(float s0, float s1, float s2, float s3, float s4, float s5, float s6, float s7)
    {
        return _mm256_setr_ps(s0, s1, s2, s3, s4, s5, s6, s7);
    }

    static inline float32x8 float32x8_uload(const float* source)
    {
        return _mm256_loadu_ps(source);
    }

    static inline void float32x8_ustore(float* dest, float32x8 a)
    {
        _mm256_storeu_ps(dest, a);
    }

    static inline float32x8 unpackhi(float32x8 a, float32x8 b)
    {
        return _mm256_unpackhi_ps(a, b);
    }

    static inline float32x8 unpacklo(float32x8 a, float32x8 b)
    {
        return _mm256_unpacklo_ps(a, b);
    }

    // bitwise

    static inline float32x8 bitwise_nand(float32x8 a, float32x8 b)
    {
        return _mm256_andnot_ps(a, b);
    }

    static inline float32x8 bitwise_and(float32x8 a, float32x8 b)
    {
         return _mm256_and_ps(a, b);
    }

    static inline float32x8 bitwise_or(float32x8 a, float32x8 b)
    {
         return _mm256_or_ps(a, b);
    }

    static inline float32x8 bitwise_xor(float32x8 a, float32x8 b)
    {
         return _mm256_xor_ps(a, b);
    }

    static inline float32x8 bitwise_not(float32x8 a)
    {
         return _mm256_xor_ps(a, _mm256_cmp_ps(a, a, _CMP_EQ_OQ));
    }

    static inline float32x8 min(float32x8 a, float32x8 b)
    {
        return _mm256_min_ps(a, b);
    }

    static inline float32x8 max(float32x8 a, float32x8 b)
    {
        return _mm256_max_ps(a, b);
    }

    static inline float32x8 abs(float32x8 a)
    {
        return _mm256_max_ps(a, _mm256_sub_ps(_mm256_setzero_ps(), a));
    }

    static inline float32x8 neg(float32x8 a)
    {
        return _mm256_sub_ps(_mm256_setzero_ps(), a);
    }

    static inline float32x8 sign(float32x8 a)
    {
        __m256 sign_mask = _mm256_set1_ps(-0.0f);
        __m256 value_mask = _mm256_cmp_ps(a, _mm256_setzero_ps(), _CMP_NEQ_UQ);
        __m256 sign_bits = _mm256_and_ps(a, sign_mask);
        __m256 value_bits = _mm256_and_ps(_mm256_set1_ps(1.0f), value_mask);
        return _mm256_or_ps(value_bits, sign_bits);
    }

    static inline float32x8 add(float32x8 a, float32x8 b)
    {
        return _mm256_add_ps(a, b);
    }

    static inline float32x8 sub(float32x8 a, float32x8 b)
    {
        return _mm256_sub_ps(a, b);
    }

    static inline float32x8 mul(float32x8 a, float32x8 b)
    {
        return _mm256_mul_ps(a, b);
    }

    static inline float32x8 div(float32x8 a, float32x8 b)
    {
        return _mm256_div_ps(a, b);
    }

    static inline float32x8 div(float32x8 a, float b)
    {
        return _mm256_div_ps(a, _mm256_set1_ps(b));
    }

    static inline float32x8 hadd(float32x8 a, float32x8 b)
    {
        return _mm256_hadd_ps(a, b);
    }

#if defined(MANGO_ENABLE_FMA3)

    static inline float32x8 madd(float32x8 a, float32x8 b, float32x8 c)
    {
        return _mm256_fmadd_ps(b, c, a);
    }

    static inline float32x8 msub(float32x8 a, float32x8 b, float32x8 c)
    {
        return _mm256_fnmadd_ps(b, c, a);
    }

#else

    static inline float32x8 madd(float32x8 a, float32x8 b, float32x8 c)
    {
        return _mm256_add_ps(a, _mm256_mul_ps(b, c));
    }

    static inline float32x8 msub(float32x8 a, float32x8 b, float32x8 c)
    {
        return _mm256_sub_ps(a, _mm256_mul_ps(b, c));
    }

#endif

    static inline float32x8 fast_rcp(float32x8 a)
    {
        return _mm256_rcp14_ps(a);
    }

    static inline float32x8 fast_rsqrt(float32x8 a)
    {
        return _mm256_rsqrt_ps(a);
    }

    static inline float32x8 fast_sqrt(float32x8 a)
    {
        return _mm256_sqrt_ps(a);
    }

    static inline float32x8 rcp(float32x8 a)
    {
        const __m256 n = _mm256_rcp_ps(a);
        const __m256 m = _mm256_mul_ps(_mm256_mul_ps(n, n), a);
        return _mm256_sub_ps(_mm256_add_ps(n, n), m);
    }

    static inline float32x8 rsqrt(float32x8 a)
    {
        __m256 n = _mm256_rsqrt_ps(a);
        __m256 e = _mm256_mul_ps(_mm256_mul_ps(n, n), a);
        n = _mm256_mul_ps(_mm256_set1_ps(0.5f), n);
        e = _mm256_sub_ps(_mm256_set1_ps(3.0f), e);
        return _mm256_mul_ps(n, e);
    }

    static inline float32x8 sqrt(float32x8 a)
    {
        return _mm256_sqrt_ps(a);
    }

    // compare

    static inline mask32x8 compare_neq(float32x8 a, float32x8 b)
    {
        return _mm256_cmp_ps_mask(a, b, _CMP_NEQ_UQ);
    }

    static inline mask32x8 compare_eq(float32x8 a, float32x8 b)
    {
        return _mm256_cmp_ps_mask(a, b, _CMP_EQ_OQ);
    }

    static inline mask32x8 compare_lt(float32x8 a, float32x8 b)
    {
        return _mm256_cmp_ps_mask(a, b, _CMP_LT_OS);
    }

    static inline mask32x8 compare_le(float32x8 a, float32x8 b)
    {
        return _mm256_cmp_ps_mask(a, b, _CMP_LE_OS);
    }

    static inline mask32x8 compare_gt(float32x8 a, float32x8 b)
    {
        return _mm256_cmp_ps_mask(b, a, _CMP_LT_OS);
    }

    static inline mask32x8 compare_ge(float32x8 a, float32x8 b)
    {
        return _mm256_cmp_ps_mask(b, a, _CMP_LE_OS);
    }

    static inline float32x8 select(mask32x8 mask, float32x8 a, float32x8 b)
    {
        return _mm256_mask_blend_ps(mask, b, a);
    }

    // rounding

    static inline float32x8 round(float32x8 s)
    {
        return _mm256_round_ps(s, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
    }

    static inline float32x8 trunc(float32x8 s)
    {
        return _mm256_round_ps(s, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
    }

    static inline float32x8 floor(float32x8 s)
    {
        return _mm256_round_ps(s, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    }

    static inline float32x8 ceil(float32x8 s)
    {
        return _mm256_round_ps(s, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
    }

    static inline float32x8 fract(float32x8 s)
    {
        return _mm256_sub_ps(s, floor(s));
    }

} // namespace simd
} // namespace mango
