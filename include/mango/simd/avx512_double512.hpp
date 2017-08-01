/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        return _mm512_setzero_pd();
    }

    static inline float64x8 float64x8_set1(double s)
    {
        return _mm512_set1_pd(s);
    }

    static inline float64x8 float64x8_set8(double s0, double s1, double s2, double s3, double s4, double s5, double s6, double s7)
    {
        return _mm512_setr_pd(s0, s1, s2, s3, s4, s5, s6, s7);
    }

    static inline float64x8 float64x8_uload(const double* source)
    {
        return _mm512_loadu_pd(source);
    }

    static inline void float64x8_ustore(double* dest, float64x8 a)
    {
        _mm512_storeu_pd(dest, a);
    }

    static inline float64x8 unpackhi(float64x8 a, float64x8 b)
    {
        return _mm512_unpackhi_pd(a, b);
    }

    static inline float64x8 unpacklo(float64x8 a, float64x8 b)
    {
        return _mm512_unpacklo_pd(a, b);
    }

    // bitwise

    static inline float64x8 bitwise_nand(float64x8 a, float64x8 b)
    {
        return _mm512_andnot_pd(a, b);
    }

    static inline float64x8 bitwise_and(float64x8 a, float64x8 b)
    {
        return _mm512_and_pd(a, b);
    }

    static inline float64x8 bitwise_or(float64x8 a, float64x8 b)
    {
        return _mm512_or_pd(a, b);
    }

    static inline float64x8 bitwise_xor(float64x8 a, float64x8 b)
    {
        return _mm512_xor_pd(a, b);
    }

    static inline float64x8 bitwise_not(float64x8 a)
    {
        __m512d mask = _mm512_castsi512_pd(_mm512_set1_epi64(0xffffffffffffffffull));
        return _mm512_xor_pd(a, mask);
    }

    static inline float64x8 min(float64x8 a, float64x8 b)
    {
        return _mm512_min_pd(a, b);
    }

    static inline float64x8 max(float64x8 a, float64x8 b)
    {
        return _mm512_max_pd(a, b);
    }

    static inline float64x8 abs(float64x8 a)
    {
        // gcc 7.1 compiler bug: expects __m512 argument
        //return _mm512_abs_pd(a);
        // workaround: return a < 0 ? 0 - a : a;
        return _mm512_mask_blend_pd(_mm512_cmp_pd_mask(a, _mm512_setzero_pd(), 1), a, _mm512_sub_pd(_mm512_setzero_pd(), a));

    }

    static inline float64x8 neg(float64x8 a)
    {
        return _mm512_sub_pd(_mm512_setzero_pd(), a);
    }

    static inline float64x8 add(float64x8 a, float64x8 b)
    {
        return _mm512_add_pd(a, b);
    }

    static inline float64x8 sub(float64x8 a, float64x8 b)
    {
        return _mm512_sub_pd(a, b);
    }

    static inline float64x8 mul(float64x8 a, float64x8 b)
    {
        return _mm512_mul_pd(a, b);
    }

    static inline float64x8 div(float64x8 a, float64x8 b)
    {
        return _mm512_div_pd(a, b);
    }

    static inline float64x8 div(float64x8 a, double b)
    {
        return _mm512_div_pd(a, _mm512_set1_pd(b));
    }

#if defined(MANGO_ENABLE_FMA3)

    static inline float64x8 madd(float64x8 a, float64x8 b, float64x8 c)
    {
        return _mm512_fmadd_pd(b, c, a);
    }

    static inline float64x8 msub(float64x8 a, float64x8 b, float64x8 c)
    {
        return _mm512_fnmadd_pd(b, c, a);
    }

#else

    static inline float64x8 madd(float64x8 a, float64x8 b, float64x8 c)
    {
        return _mm512_add_pd(a, _mm512_mul_pd(b, c));
    }

    static inline float64x8 msub(float64x8 a, float64x8 b, float64x8 c)
    {
        return _mm512_sub_pd(a, _mm512_mul_pd(b, c));
    }

#endif

    static inline float64x8 fast_rcp(float64x8 a)
    {
        return _mm512_rcp14_pd(a);
    }

    static inline float64x8 fast_rsqrt(float64x8 a)
    {
        return _mm512_rsqrt14_pd(a);
    }

    static inline float64x8 fast_sqrt(float64x8 a)
    {
        return _mm512_sqrt_pd(a);
    }

    static inline float64x8 rcp(float64x8 a)
    {
        return _mm512_rcp28_pd(a);
    }

    static inline float64x8 rsqrt(float64x8 a)
    {
        return _mm512_rsqrt28_pd(a);
    }

    static inline float64x8 sqrt(float64x8 a)
    {
        return _mm512_sqrt_pd(a);
    }

    // compare

    static inline mask64x8 compare_neq(float64x8 a, float64x8 b)
    {
        return _mm512_cmp_pd_mask(a, b, 4);
    }

    static inline mask64x8 compare_eq(float64x8 a, float64x8 b)
    {
        return _mm512_cmp_pd_mask(a, b, 0);
    }

    static inline mask64x8 compare_lt(float64x8 a, float64x8 b)
    {
        return _mm512_cmp_pd_mask(a, b, 1);
    }

    static inline mask64x8 compare_le(float64x8 a, float64x8 b)
    {
        return _mm512_cmp_pd_mask(a, b, 2);
    }

    static inline mask64x8 compare_gt(float64x8 a, float64x8 b)
    {
        return _mm512_cmp_pd_mask(b, a, 1);
    }

    static inline mask64x8 compare_ge(float64x8 a, float64x8 b)
    {
        return _mm512_cmp_pd_mask(b, a, 2);
    }

    static inline float64x8 select(mask64x8 mask, float64x8 a, float64x8 b)
    {
        return _mm512_mask_blend_pd(mask, b, a);
    }

    // rounding

    static inline float64x8 round(float64x8 s)
    {
        return _mm512_add_round_pd(s, _mm512_setzero_pd(), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
    }

    static inline float64x8 trunc(float64x8 s)
    {
        return _mm512_add_round_pd(s, _mm512_setzero_pd(), _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
    }

    static inline float64x8 floor(float64x8 s)
    {
        return _mm512_add_round_pd(s, _mm512_setzero_pd(), _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    }

    static inline float64x8 ceil(float64x8 s)
    {
        return _mm512_add_round_pd(s, _mm512_setzero_pd(), _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
    }

    static inline float64x8 fract(float64x8 s)
    {
        return _mm512_sub_pd(s, floor(s));
    }

} // namespace simd
} // namespace mango
