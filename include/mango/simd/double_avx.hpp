/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_SIMD_DOUBLE
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

    // clang workaround
    #define simd_mm256_set_m128i(hi, lo) _mm256_insertf128_si256(_mm256_castsi128_si256(lo), hi, 1)

    // -----------------------------------------------------------------
    // conversion
    // -----------------------------------------------------------------

    static inline float64x4 float64x4_convert(int32x4 s)
    {
        return _mm256_cvtepi32_pd(s);
    }

    static inline float64x4 float64x4_convert(float32x4 s)
    {
        return _mm256_cvtps_pd(s);
    }

    static inline int32x4 int32x4_convert(float64x4 s)
    {
        return _mm256_cvtpd_epi32(s);
    }

    static inline float32x4 float32x4_convert(float64x4 s)
    {
        return _mm256_cvtpd_ps(s);
    }

#if defined(MANGO_ENABLE_AVX2)

    static inline float64x4 float64x4_convert(uint32x4 ui)
    {
        const __m256d bias = _mm256_set1_pd((1ll << 52) * 1.5);
        const __m256i xyzw = _mm256_cvtepu32_epi64(ui);
        __m256d v = _mm256_castsi256_pd(xyzw);
        v = _mm256_or_pd(v, bias);
        v = _mm256_sub_pd(v, bias);
        return v;
    }

#else

    static inline float64x4 float64x4_convert(uint32x4 ui)
    {
        const __m256d bias = _mm256_set1_pd((1ll << 52) * 1.5);
        const __m128i mask = _mm_set1_epi32(0x43380000);
        const __m128i xy = _mm_unpacklo_epi32(ui, mask);
        const __m128i zw = _mm_unpackhi_epi32(ui, mask);
        const __m256i xyzw = simd_mm256_set_m128i(zw, xy);
        __m256d v = _mm256_castsi256_pd(xyzw);
        v = _mm256_sub_pd(v, bias);
        return v;
    }

#endif

    static inline uint32x4 uint32x4_convert(float64x4 d)
    {
        const __m256d bias = _mm256_set1_pd((1ll << 52) * 1.5);
        const __m256d v = _mm256_add_pd(d, bias);
        const __m128d xxyy = _mm256_castpd256_pd128(v);
        const __m128d zzww = _mm256_extractf128_pd(v, 1);
        __m128 xyzw = _mm_shuffle_ps(_mm_castpd_ps(xxyy), _mm_castpd_ps(zzww), 0x88);
        return _mm_castps_si128(xyzw);
    }

    static inline int32x4 int32x4_truncate(float64x4 s)
    {
        return _mm256_cvttpd_epi32(s);
    }

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

    static inline float64x4 float64x4_set_x(float64x4 a, double x)
    {
        return float64x4_set_component<0>(a, x);
    }

    static inline float64x4 float64x4_set_y(float64x4 a, double y)
    {
        return float64x4_set_component<1>(a, y);
    }

    static inline float64x4 float64x4_set_z(float64x4 a, double z)
    {
        return float64x4_set_component<2>(a, z);
    }

    static inline float64x4 float64x4_set_w(float64x4 a, double w)
    {
        return float64x4_set_component<3>(a, w);
    }

    static inline double float64x4_get_x(float64x4 a)
    {
        return float64x4_get_component<0>(a);
    }

    static inline double float64x4_get_y(float64x4 a)
    {
        return float64x4_get_component<1>(a);
    }

    static inline double float64x4_get_z(float64x4 a)
    {
        return float64x4_get_component<2>(a);
    }

    static inline double float64x4_get_w(float64x4 a)
    {
        return float64x4_get_component<3>(a);
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

    static inline float64x4 float64x4_load(const double* source)
    {
        return _mm256_load_pd(source);
    }

    static inline float64x4 float64x4_uload(const double* source)
    {
        return _mm256_loadu_pd(source);
    }

    static inline void float64x4_store(double* dest, float64x4 a)
    {
        _mm256_store_pd(dest, a);
    }

    static inline void float64x4_ustore(double* dest, float64x4 a)
    {
        _mm256_storeu_pd(dest, a);
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
