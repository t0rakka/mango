/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

#ifdef MANGO_SIMD_DOUBLE_AVX

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float64x4
    // -----------------------------------------------------------------

    template <int x, int y, int z, int w>
    static inline float64x4 float64x4_shuffle(float64x4 v)
    {
        const int select = ((w & 1) << 3) | ((z & 1) << 2) | ((y & 1) << 1) | (x & 1);
        const int mask = ((w & 2) << 2) | ((z & 2) << 1) | (y & 2) | ((x & 2) >> 1);
        __m256d xyxy = _mm256_permute2f128_pd(v, v, 0);
        __m256d zwzw = _mm256_permute2f128_pd(v, v, 0x11);
        xyxy = _mm256_permute_pd(xyxy, select);
        zwzw = _mm256_permute_pd(zwzw, select);
        return _mm256_blend_pd(xyxy, zwzw, mask);
    }

    template <>
    inline float64x4 float64x4_shuffle<0, 1, 2, 3>(float64x4 v)
    {
        // .xyzw
        return v;
    }

    template <>
    inline float64x4 float64x4_shuffle<0, 0, 0, 0>(float64x4 v)
    {
        // .xxxx
        const __m256d xyxy = _mm256_permute2f128_pd(v, v, 0);
        return _mm256_permute_pd(xyxy, 0);
    }

    template <>
    inline float64x4 float64x4_shuffle<1, 1, 1, 1>(float64x4 v)
    {
        // .yyyy
        const __m256d xyxy = _mm256_permute2f128_pd(v, v, 0);
        return _mm256_permute_pd(xyxy, 0xf);
    }

    template <>
    inline float64x4 float64x4_shuffle<2, 2, 2, 2>(float64x4 v)
    {
        // .zzzz
        const __m256d zwzw = _mm256_permute2f128_pd(v, v, 0x11);
        return _mm256_permute_pd(zwzw, 0);
    }

    template <>
    inline float64x4 float64x4_shuffle<3, 3, 3, 3>(float64x4 v)
    {
        // .wwww
        const __m256d zwzw = _mm256_permute2f128_pd(v, v, 0x11);
        return _mm256_permute_pd(zwzw, 0xf);
    }

    // set component

    template <int Index>
    static inline float64x4 float64x4_set_component(float64x4 a, double s);

    template <>
    inline float64x4 float64x4_set_component<0>(float64x4 a, double x)
    {
        __m128d xy = _mm256_extractf128_pd(a, 0);
        xy = _mm_move_sd(xy, _mm_set1_pd(x));
        return _mm256_insertf128_pd(a, xy, 0);
    }

    template <>
    inline float64x4 float64x4_set_component<1>(float64x4 a, double y)
    {
        __m128d xy = _mm256_extractf128_pd(a, 0);
        xy = _mm_move_sd(_mm_set1_pd(y), xy);
        return _mm256_insertf128_pd(a, xy, 0);
    }

    template <>
    inline float64x4 float64x4_set_component<2>(float64x4 a, double z)
    {
        __m128d zw = _mm256_extractf128_pd(a, 1);
        zw = _mm_move_sd(zw, _mm_set1_pd(z));
        return _mm256_insertf128_pd(a, zw, 1);
    }

    template <>
    inline float64x4 float64x4_set_component<3>(float64x4 a, double w)
    {
        __m128d zw = _mm256_extractf128_pd(a, 1);
        zw = _mm_move_sd(_mm_set1_pd(w), zw);
        return _mm256_insertf128_pd(a, zw, 1);
    }

    // get component

    template <int Index>
    static inline double float64x4_get_component(float64x4 a);

    template <>
    inline double float64x4_get_component<0>(float64x4 a)
    {
        const __m128d xy = _mm256_extractf128_pd(a, 0);
        return _mm_cvtsd_f64(xy);
    }

    template <>
    inline double float64x4_get_component<1>(float64x4 a)
    {
        const __m128d xy = _mm256_extractf128_pd(a, 0);
        const __m128d yy = _mm_unpackhi_pd(xy, xy);
        return _mm_cvtsd_f64(yy);
    }

    template <>
    inline double float64x4_get_component<2>(float64x4 a)
    {
        const __m128d zw = _mm256_extractf128_pd(a, 1);
        return _mm_cvtsd_f64(zw);
    }

    template <>
    inline double float64x4_get_component<3>(float64x4 a)
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

    static inline float64x4 float64x4_movelh(float64x4 a, float64x4 b)
    {
        return _mm256_permute2f128_pd(a, b, 0x20);
    }

    static inline float64x4 float64x4_movehl(float64x4 a, float64x4 b)
    {
        return _mm256_permute2f128_pd(a, b, 0x13);
    }

    static inline float64x4 float64x4_unpackhi(float64x4 a, float64x4 b)
    {
        return _mm256_unpackhi_pd(a, b);
    }

    static inline float64x4 float64x4_unpacklo(float64x4 a, float64x4 b)
    {
        return _mm256_unpacklo_pd(a, b);
    }

    // logical

    static inline float64x4 float64x4_and(float64x4 a, float64x4 b)
    {
        return _mm256_and_pd(a, b);
    }

    static inline float64x4 float64x4_nand(float64x4 a, float64x4 b)
    {
        return _mm256_andnot_pd(a, b);
    }

    static inline float64x4 float64x4_or(float64x4 a, float64x4 b)
    {
        return _mm256_or_pd(a, b);
    }

    static inline float64x4 float64x4_xor(float64x4 a, float64x4 b)
    {
        return _mm256_xor_pd(a, b);
    }

    static inline float64x4 float64x4_min(float64x4 a, float64x4 b)
    {
        return _mm256_min_pd(a, b);
    }

    static inline float64x4 float64x4_max(float64x4 a, float64x4 b)
    {
        return _mm256_max_pd(a, b);
    }

    static inline float64x4 float64x4_clamp(float64x4 v, float64x4 vmin, float64x4 vmax)
    {
        return _mm256_min_pd(vmax, _mm256_max_pd(vmin, v));
    }

    static inline float64x4 float64x4_abs(float64x4 a)
    {
        const __m256i mask = _mm256_set1_epi64x(0x7fffffffffffffff);
        return _mm256_and_pd(a, _mm256_castsi256_pd(mask));
    }

    static inline float64x4 float64x4_neg(float64x4 a)
    {
        const __m256i mask = _mm256_set1_epi64x(0x8000000000000000);
        return _mm256_xor_pd(a, _mm256_castsi256_pd(mask));
    }

    static inline float64x4 float64x4_add(float64x4 a, float64x4 b)
    {
        return _mm256_add_pd(a, b);
    }

    static inline float64x4 float64x4_sub(float64x4 a, float64x4 b)
    {
        return _mm256_sub_pd(a, b);
    }

    static inline float64x4 float64x4_mul(float64x4 a, float64x4 b)
    {
        return _mm256_mul_pd(a, b);
    }

    static inline float64x4 float64x4_div(float64x4 a, float64x4 b)
    {
        return _mm256_div_pd(a, b);
    }

    static inline float64x4 float64x4_div(float64x4 a, double b)
    {
        return _mm256_div_pd(a, _mm256_set1_pd(b));
    }

#if defined(MANGO_ENABLE_FMA3)

    static inline float64x4 float64x4_madd(float64x4 a, float64x4 b, float64x4 c)
    {
        return _mm256_fmadd_pd(b, c, a);
    }

    static inline float64x4 float64x4_msub(float64x4 a, float64x4 b, float64x4 c)
    {
        return _mm256_fnmadd_pd(b, c, a);
    }

#else

    static inline float64x4 float64x4_madd(float64x4 a, float64x4 b, float64x4 c)
    {
        return _mm256_add_pd(a, _mm256_mul_pd(b, c));
    }

    static inline float64x4 float64x4_msub(float64x4 a, float64x4 b, float64x4 c)
    {
        return _mm256_sub_pd(a, _mm256_mul_pd(b, c));
    }

#endif

    static inline float64x4 float64x4_fast_reciprocal(float64x4 a)
    {
        const __m256d one = _mm256_set1_pd(1.0);
        return _mm256_div_pd(one, a);
    }

    static inline float64x4 float64x4_fast_rsqrt(float64x4 a)
    {
        const __m256d one = _mm256_set1_pd(1.0);
        return _mm256_div_pd(one, _mm256_sqrt_pd(a));
    }

    static inline float64x4 float64x4_fast_sqrt(float64x4 a)
    {
        return _mm256_sqrt_pd(a);
    }

    static inline float64x4 float64x4_reciprocal(float64x4 a)
    {
        const __m256d one = _mm256_set1_pd(1.0);
        return _mm256_div_pd(one, a);
    }

    static inline float64x4 float64x4_rsqrt(float64x4 a)
    {
        const __m256d one = _mm256_set1_pd(1.0);
        return _mm256_div_pd(one, _mm256_sqrt_pd(a));
    }

    static inline float64x4 float64x4_sqrt(float64x4 a)
    {
        return _mm256_sqrt_pd(a);
    }

    static inline float64x4 float64x4_dot4(float64x4 a, float64x4 b)
    {
        const __m256d s = _mm256_mul_pd(a, b);
        const __m256d zwxy = _mm256_permute2f128_pd(s, s, 0x01);
        const __m256d n = _mm256_hadd_pd(s, zwxy);
        return _mm256_hadd_pd(n, n);
    }

    // compare

    static inline float64x4 float64x4_compare_neq(float64x4 a, float64x4 b)
    {
        return _mm256_cmp_pd(a, b, 4);
    }

    static inline float64x4 float64x4_compare_eq(float64x4 a, float64x4 b)
    {
        return _mm256_cmp_pd(a, b, 0);
    }

    static inline float64x4 float64x4_compare_lt(float64x4 a, float64x4 b)
    {
        return _mm256_cmp_pd(a, b, 1);
    }

    static inline float64x4 float64x4_compare_le(float64x4 a, float64x4 b)
    {
        return _mm256_cmp_pd(a, b, 2);
    }

    static inline float64x4 float64x4_compare_gt(float64x4 a, float64x4 b)
    {
        return _mm256_cmp_pd(b, a, 1);
    }

    static inline float64x4 float64x4_compare_ge(float64x4 a, float64x4 b)
    {
        return _mm256_cmp_pd(b, a, 2);
    }

    static inline float64x4 float64x4_select(float64x4 mask, float64x4 a, float64x4 b)
    {
        return _mm256_blendv_pd(b, a, mask);
    }

    // rounding

    static inline float64x4 float64x4_round(float64x4 s)
    {
        return _mm256_round_pd(s, _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
    }

    static inline float64x4 float64x4_trunc(float64x4 s)
    {
        return _mm256_round_pd(s, _MM_FROUND_TO_ZERO |_MM_FROUND_NO_EXC);
    }

    static inline float64x4 float64x4_floor(float64x4 s)
    {
        return _mm256_floor_pd(s);
    }

    static inline float64x4 float64x4_ceil(float64x4 s)
    {
        return _mm256_ceil_pd(s);
    }

    static inline float64x4 float64x4_fract(float64x4 s)
    {
        return _mm256_sub_pd(s, _mm256_floor_pd(s));
    }

} // namespace simd
} // namespace mango

#endif // MANGO_SIMD_DOUBLE_AVX
