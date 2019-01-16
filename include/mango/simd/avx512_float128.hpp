/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline float32x4 shuffle(float32x4 a, float32x4 b)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return _mm_shuffle_ps(a, b, _MM_SHUFFLE(w, z, y, x));
    }

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline float32x4 shuffle(float32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return _mm_shuffle_ps(v, v, _MM_SHUFFLE(w, z, y, x));
    }

    template <>
    inline float32x4 shuffle<0, 1, 2, 3>(float32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline float32x4 set_component(float32x4 a, float s)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm_insert_ps(a, _mm_set_ss(s), Index * 0x10);
    }

    template <int Index>
    static inline float get_component(float32x4 a);

    template <>
    inline float get_component<0>(float32x4 a)
    {
        return _mm_cvtss_f32(a);
    }

    template <>
    inline float get_component<1>(float32x4 a)
    {
        return _mm_cvtss_f32(shuffle<1, 1, 1, 1>(a));
    }

    template <>
    inline float get_component<2>(float32x4 a)
    {
        return _mm_cvtss_f32(shuffle<2, 2, 2, 2>(a));
    }

    template <>
    inline float get_component<3>(float32x4 a)
    {
        return _mm_cvtss_f32(shuffle<3, 3, 3, 3>(a));
    }

    static inline float32x4 float32x4_zero()
    {
        return _mm_setzero_ps();
    }

    static inline float32x4 float32x4_set1(float s)
    {
        return _mm_set1_ps(s);
    }

    static inline float32x4 float32x4_set4(float x, float y, float z, float w)
    {
        return _mm_setr_ps(x, y, z, w);
    }

    static inline float32x4 float32x4_uload(const float* source)
    {
        return _mm_loadu_ps(source);
    }

    static inline void float32x4_ustore(float* dest, float32x4 a)
    {
        _mm_storeu_ps(dest, a);
    }

    static inline float32x4 movelh(float32x4 a, float32x4 b)
    {
        return _mm_movelh_ps(a, b);
    }

    static inline float32x4 movehl(float32x4 a, float32x4 b)
    {
        return _mm_movehl_ps(a, b);
    }

    static inline float32x4 unpackhi(float32x4 a, float32x4 b)
    {
        return _mm_unpackhi_ps(a, b);
    }

    static inline float32x4 unpacklo(float32x4 a, float32x4 b)
    {
        return _mm_unpacklo_ps(a, b);
    }

    // bitwise

    static inline float32x4 bitwise_nand(float32x4 a, float32x4 b)
    {
        return _mm_andnot_ps(a, b);
    }

    static inline float32x4 bitwise_and(float32x4 a, float32x4 b)
    {
        return _mm_and_ps(a, b);
    }

    static inline float32x4 bitwise_or(float32x4 a, float32x4 b)
    {
        return _mm_or_ps(a, b);
    }

    static inline float32x4 bitwise_xor(float32x4 a, float32x4 b)
    {
        return _mm_xor_ps(a, b);
    }

    static inline float32x4 bitwise_not(float32x4 a)
    {
        return _mm_xor_ps(a, _mm_cmpeq_ps(a, a));
    }

    static inline float32x4 min(float32x4 a, float32x4 b)
    {
        return _mm_min_ps(a, b);
    }

    static inline float32x4 max(float32x4 a, float32x4 b)
    {
        return _mm_max_ps(a, b);
    }

    static inline float32x4 hmin(float32x4 a)
    {
        __m128 temp = _mm_min_ps(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1)));
        return _mm_min_ps(temp, _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(1, 0, 3, 2)));
    }

    static inline float32x4 hmax(float32x4 a)
    {
        __m128 temp = _mm_max_ps(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1)));
        return _mm_max_ps(temp, _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(1, 0, 3, 2)));
    }

    static inline float32x4 abs(float32x4 a)
    {
        return _mm_max_ps(a, _mm_sub_ps(_mm_setzero_ps(), a));
    }

    static inline float32x4 neg(float32x4 a)
    {
        return _mm_sub_ps(_mm_setzero_ps(), a);
    }

    static inline float32x4 sign(float32x4 a)
    {
        __m128 sign_mask = _mm_set1_ps(-0.0f);
        __m128 value_mask = _mm_cmpneq_ps(a, _mm_setzero_ps());
        __m128 sign_bits = _mm_and_ps(a, sign_mask);
        __m128 value_bits = _mm_and_ps(_mm_set1_ps(1.0f), value_mask);
        return _mm_or_ps(value_bits, sign_bits);
    }
    
    static inline float32x4 add(float32x4 a, float32x4 b)
    {
        return _mm_add_ps(a, b);
    }

    static inline float32x4 sub(float32x4 a, float32x4 b)
    {
        return _mm_sub_ps(a, b);
    }

    static inline float32x4 mul(float32x4 a, float32x4 b)
    {
        return _mm_mul_ps(a, b);
    }

    static inline float32x4 div(float32x4 a, float32x4 b)
    {
        return _mm_div_ps(a, b);
    }

    static inline float32x4 div(float32x4 a, float b)
    {
        return _mm_div_ps(a, _mm_set1_ps(b));
    }

    static inline float32x4 hadd(float32x4 a, float32x4 b)
    {
	    return _mm_hadd_ps(a, b);
    }

#if defined(MANGO_ENABLE_FMA3)

    static inline float32x4 madd(float32x4 a, float32x4 b, float32x4 c)
    {
        return _mm_fmadd_ps(b, c, a);
    }

    static inline float32x4 msub(float32x4 a, float32x4 b, float32x4 c)
    {
        return _mm_fnmadd_ps(b, c, a);
    }

#else

    static inline float32x4 madd(float32x4 a, float32x4 b, float32x4 c)
    {
        return _mm_add_ps(a, _mm_mul_ps(b, c));
    }

    static inline float32x4 msub(float32x4 a, float32x4 b, float32x4 c)
    {
        return _mm_sub_ps(a, _mm_mul_ps(b, c));
    }

#endif

    static inline float32x4 fast_rcp(float32x4 a)
    {
        return _mm_rcp14_ps(a);
    }

    static inline float32x4 fast_rsqrt(float32x4 a)
    {
        return _mm_maskz_rsqrt14_ps(_mm_cmp_ps_mask(a, a, _CMP_EQ_OQ), a);
    }

    static inline float32x4 fast_sqrt(float32x4 a)
    {
        return _mm_sqrt_ps(a);
    }

    static inline float32x4 rcp(float32x4 a)
    {
        float32x4 n = _mm_rcp_ps(a);
        float32x4 m = _mm_mul_ps(_mm_mul_ps(n, n), a);
        return _mm_sub_ps(_mm_add_ps(n, n), m);
    }

    static inline float32x4 rsqrt(float32x4 a)
    {
        float32x4 n = _mm_rsqrt_ps(a);
        float32x4 e = _mm_mul_ps(_mm_mul_ps(n, n), a);
        n = _mm_mul_ps(_mm_set_ps1(0.5f), n);
        e = _mm_sub_ps(_mm_set_ps1(3.0f), e);
        return _mm_mul_ps(n, e);
    }

    static inline float32x4 sqrt(float32x4 a)
    {
        return _mm_sqrt_ps(a);
    }

    static inline float dot3(float32x4 a, float32x4 b)
    {
        float32x4 s = _mm_dp_ps(a, b, 0x7f);
        return get_component<0>(s);
    }

    static inline float dot4(float32x4 a, float32x4 b)
    {
        float32x4 s = _mm_dp_ps(a, b, 0xff);
        return get_component<0>(s);
    }

    static inline float32x4 cross3(float32x4 a, float32x4 b)
    {
        float32x4 c = _mm_sub_ps(_mm_mul_ps(a, shuffle<1, 2, 0, 3>(b)),
                                 _mm_mul_ps(b, shuffle<1, 2, 0, 3>(a)));
        return shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline mask32x4 compare_neq(float32x4 a, float32x4 b)
    {
        return _mm_cmp_ps_mask(a, b, _CMP_NEQ_UQ);
    }

    static inline mask32x4 compare_eq(float32x4 a, float32x4 b)
    {
        return _mm_cmp_ps_mask(a, b, _CMP_EQ_OQ);
    }

    static inline mask32x4 compare_lt(float32x4 a, float32x4 b)
    {
        return _mm_cmp_ps_mask(a, b, _CMP_LT_OS);
    }

    static inline mask32x4 compare_le(float32x4 a, float32x4 b)
    {
        return _mm_cmp_ps_mask(a, b, _CMP_LE_OS);
    }

    static inline mask32x4 compare_gt(float32x4 a, float32x4 b)
    {
        return _mm_cmp_ps_mask(b, a, _CMP_LT_OS);
    }

    static inline mask32x4 compare_ge(float32x4 a, float32x4 b)
    {
        return _mm_cmp_ps_mask(b, a, _CMP_LE_OS);
    }

    static inline float32x4 select(mask32x4 mask, float32x4 a, float32x4 b)
    {
        return _mm_mask_blend_ps(mask, b, a);
    }

    // rounding

    static inline float32x4 round(float32x4 s)
    {
        return _mm_round_ps(s, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
    }

    static inline float32x4 trunc(float32x4 s)
    {
        return _mm_round_ps(s, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
    }

    static inline float32x4 floor(float32x4 s)
    {
        return _mm_round_ps(s, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    }

    static inline float32x4 ceil(float32x4 s)
    {
        return _mm_round_ps(s, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
    }

    static inline float32x4 fract(float32x4 s)
    {
        return sub(s, floor(s));
    }

} // namespace simd
} // namespace mango
