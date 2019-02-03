/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float64x4
    // -----------------------------------------------------------------

    template <u32 x, u32 y, u32 z, u32 w>
    static inline float64x4 shuffle(float64x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return _mm256_permute4x64_pd(v, _MM_SHUFFLE(w, z, y, x));
    }

    template <>
    inline float64x4 shuffle<0, 1, 2, 3>(float64x4 v)
    {
        // .xyzw
        return v;
    }

    template <>
    inline float64x4 shuffle<1, 0, 3, 2>(float64x4 v)
    {
        // .yxwz
        return _mm256_shuffle_pd(v, v, 0x05);
    }

    template <>
    inline float64x4 shuffle<2, 3, 0, 1>(float64x4 v)
    {
        // .zwxy
        return _mm256_shuffle_pd(v, v, 0x0a);
    }

    // set component

    template <int Index>
    static inline float64x4 set_component(float64x4 a, double s);

    template <>
    inline float64x4 set_component<0>(float64x4 a, double x)
    {
        __m128d xy = _mm256_extractf128_pd(a, 0);
        xy = _mm_move_sd(xy, _mm_set1_pd(x));
        return _mm256_insertf128_pd(a, xy, 0);
    }

    template <>
    inline float64x4 set_component<1>(float64x4 a, double y)
    {
        __m128d xy = _mm256_extractf128_pd(a, 0);
        xy = _mm_move_sd(_mm_set1_pd(y), xy);
        return _mm256_insertf128_pd(a, xy, 0);
    }

    template <>
    inline float64x4 set_component<2>(float64x4 a, double z)
    {
        __m128d zw = _mm256_extractf128_pd(a, 1);
        zw = _mm_move_sd(zw, _mm_set1_pd(z));
        return _mm256_insertf128_pd(a, zw, 1);
    }

    template <>
    inline float64x4 set_component<3>(float64x4 a, double w)
    {
        __m128d zw = _mm256_extractf128_pd(a, 1);
        zw = _mm_move_sd(_mm_set1_pd(w), zw);
        return _mm256_insertf128_pd(a, zw, 1);
    }

    // get component

    template <int Index>
    static inline double get_component(float64x4 a);

    template <>
    inline double get_component<0>(float64x4 a)
    {
        const __m128d xy = _mm256_extractf128_pd(a, 0);
        return _mm_cvtsd_f64(xy);
    }

    template <>
    inline double get_component<1>(float64x4 a)
    {
        const __m128d xy = _mm256_extractf128_pd(a, 0);
        const __m128d yy = _mm_unpackhi_pd(xy, xy);
        return _mm_cvtsd_f64(yy);
    }

    template <>
    inline double get_component<2>(float64x4 a)
    {
        const __m128d zw = _mm256_extractf128_pd(a, 1);
        return _mm_cvtsd_f64(zw);
    }

    template <>
    inline double get_component<3>(float64x4 a)
    {
        const __m128d zw = _mm256_extractf128_pd(a, 1);
        const __m128d ww = _mm_unpackhi_pd(zw, zw);
        return _mm_cvtsd_f64(ww);
    }

    static inline float64x4 float64x4_zero()
    {
        return _mm256_setzero_pd();
    }

    static inline float64x4 float64x4_set1(double s)
    {
        return _mm256_set1_pd(s);
    }

    static inline float64x4 float64x4_set4(double x, double y, double z, double w)
    {
        return _mm256_setr_pd(x, y, z, w);
    }

    static inline float64x4 float64x4_uload(const double* source)
    {
        return _mm256_loadu_pd(source);
    }

    static inline void float64x4_ustore(double* dest, float64x4 a)
    {
        _mm256_storeu_pd(dest, a);
    }

    static inline float64x4 movelh(float64x4 a, float64x4 b)
    {
        return _mm256_permute2f128_pd(a, b, 0x20);
    }

    static inline float64x4 movehl(float64x4 a, float64x4 b)
    {
        return _mm256_permute2f128_pd(a, b, 0x13);
    }

    static inline float64x4 unpackhi(float64x4 a, float64x4 b)
    {
        return _mm256_unpackhi_pd(a, b);
    }

    static inline float64x4 unpacklo(float64x4 a, float64x4 b)
    {
        return _mm256_unpacklo_pd(a, b);
    }

    // bitwise

    static inline float64x4 bitwise_nand(float64x4 a, float64x4 b)
    {
        return _mm256_andnot_pd(a, b);
    }

    static inline float64x4 bitwise_and(float64x4 a, float64x4 b)
    {
        return _mm256_and_pd(a, b);
    }

    static inline float64x4 bitwise_or(float64x4 a, float64x4 b)
    {
        return _mm256_or_pd(a, b);
    }

    static inline float64x4 bitwise_xor(float64x4 a, float64x4 b)
    {
        return _mm256_xor_pd(a, b);
    }

    static inline float64x4 bitwise_not(float64x4 a)
    {
        return _mm256_xor_pd(a, _mm256_cmp_pd(a, a, _CMP_EQ_OQ));
    }

    static inline float64x4 min(float64x4 a, float64x4 b)
    {
        return _mm256_min_pd(a, b);
    }

    static inline float64x4 max(float64x4 a, float64x4 b)
    {
        return _mm256_max_pd(a, b);
    }

    static inline float64x4 hmin(float64x4 a)
    {
        const __m256d temp = _mm256_min_pd(a, _mm256_shuffle_pd(a, a, 0x05));
        return _mm256_min_pd(temp, _mm256_shuffle_pd(temp, temp, 0x0a));
    }

    static inline float64x4 hmax(float64x4 a)
    {
        const __m256d temp = _mm256_max_pd(a, _mm256_shuffle_pd(a, a, 0x05));
        return _mm256_max_pd(temp, _mm256_shuffle_pd(temp, temp, 0x0a));
    }

    static inline float64x4 abs(float64x4 a)
    {
        return _mm256_max_pd(a, _mm256_sub_pd(_mm256_setzero_pd(), a));
    }

    static inline float64x4 neg(float64x4 a)
    {
        return _mm256_sub_pd(_mm256_setzero_pd(), a);
    }

    static inline float64x4 sign(float64x4 a)
    {
        __m256d sign_mask = _mm256_set1_pd(-0.0);
        __m256d value_mask = _mm256_cmp_pd(a, _mm256_setzero_pd(), _CMP_NEQ_UQ);
        __m256d sign_bits = _mm256_and_pd(a, sign_mask);
        __m256d value_bits = _mm256_and_pd(_mm256_set1_pd(1.0), value_mask);
        return _mm256_or_pd(value_bits, sign_bits);
    }

    static inline float64x4 add(float64x4 a, float64x4 b)
    {
        return _mm256_add_pd(a, b);
    }

    static inline float64x4 sub(float64x4 a, float64x4 b)
    {
        return _mm256_sub_pd(a, b);
    }

    static inline float64x4 mul(float64x4 a, float64x4 b)
    {
        return _mm256_mul_pd(a, b);
    }

    static inline float64x4 div(float64x4 a, float64x4 b)
    {
        return _mm256_div_pd(a, b);
    }

    static inline float64x4 div(float64x4 a, double b)
    {
        return _mm256_div_pd(a, _mm256_set1_pd(b));
    }

#if defined(MANGO_ENABLE_FMA3)

    static inline float64x4 madd(float64x4 a, float64x4 b, float64x4 c)
    {
        return _mm256_fmadd_pd(b, c, a);
    }

    static inline float64x4 msub(float64x4 a, float64x4 b, float64x4 c)
    {
        return _mm256_fnmadd_pd(b, c, a);
    }

#else

    static inline float64x4 madd(float64x4 a, float64x4 b, float64x4 c)
    {
        return _mm256_add_pd(a, _mm256_mul_pd(b, c));
    }

    static inline float64x4 msub(float64x4 a, float64x4 b, float64x4 c)
    {
        return _mm256_sub_pd(a, _mm256_mul_pd(b, c));
    }

#endif

    static inline float64x4 fast_rcp(float64x4 a)
    {
        return _mm256_rcp14_pd(a);
    }

    static inline float64x4 fast_rsqrt(float64x4 a)
    {
        const __m256d one = _mm256_set1_pd(1.0);
        return _mm256_div_pd(one, _mm256_sqrt_pd(a));
    }

    static inline float64x4 fast_sqrt(float64x4 a)
    {
        return _mm256_sqrt_pd(a);
    }

    static inline float64x4 rcp(float64x4 a)
    {
        const __m256d one = _mm256_set1_pd(1.0);
        return _mm256_div_pd(one, a);
    }

    static inline float64x4 rsqrt(float64x4 a)
    {
        const __m256d one = _mm256_set1_pd(1.0);
        return _mm256_div_pd(one, _mm256_sqrt_pd(a));
    }

    static inline float64x4 sqrt(float64x4 a)
    {
        return _mm256_sqrt_pd(a);
    }

    static inline double dot4(float64x4 a, float64x4 b)
    {
        const __m256d prod = _mm256_mul_pd(a, b);
        const __m256d zwxy = _mm256_permute2f128_pd(prod, prod, 0x01);
        const __m256d n = _mm256_hadd_pd(prod, zwxy);
        float64x4 s = _mm256_hadd_pd(n, n);
        return get_component<0>(s);
    }

    // compare

    static inline mask64x4 compare_neq(float64x4 a, float64x4 b)
    {
        return _mm256_cmp_pd_mask(a, b, _CMP_NEQ_UQ);
    }

    static inline mask64x4 compare_eq(float64x4 a, float64x4 b)
    {
        return _mm256_cmp_pd_mask(a, b, _CMP_EQ_OQ);
    }

    static inline mask64x4 compare_lt(float64x4 a, float64x4 b)
    {
        return _mm256_cmp_pd_mask(a, b, _CMP_LT_OS);
    }

    static inline mask64x4 compare_le(float64x4 a, float64x4 b)
    {
        return _mm256_cmp_pd_mask(a, b, _CMP_LE_OS);
    }

    static inline mask64x4 compare_gt(float64x4 a, float64x4 b)
    {
        return _mm256_cmp_pd_mask(b, a, _CMP_LT_OS);
    }

    static inline mask64x4 compare_ge(float64x4 a, float64x4 b)
    {
        return _mm256_cmp_pd_mask(b, a, _CMP_LE_OS);
    }

    static inline float64x4 select(mask64x4 mask, float64x4 a, float64x4 b)
    {
        return _mm256_mask_blend_pd(mask, b, a);
    }

    // rounding

    static inline float64x4 round(float64x4 s)
    {
        return _mm256_round_pd(s, _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
    }

    static inline float64x4 trunc(float64x4 s)
    {
        return _mm256_round_pd(s, _MM_FROUND_TO_ZERO |_MM_FROUND_NO_EXC);
    }

    static inline float64x4 floor(float64x4 s)
    {
        return _mm256_floor_pd(s);
    }

    static inline float64x4 ceil(float64x4 s)
    {
        return _mm256_ceil_pd(s);
    }

    static inline float64x4 fract(float64x4 s)
    {
        return _mm256_sub_pd(s, _mm256_floor_pd(s));
    }

} // namespace simd
} // namespace mango
