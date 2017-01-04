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

    // conversion

    static inline simd4d simd4d_convert(__simd4i s)
    {
        return _mm256_cvtepi32_pd(s);
    }

    static inline simd4i simd4i_convert(__simd4d s)
    {
        return _mm256_cvtpd_epi32(s);
    }

#if defined(MANGO_ENABLE_AVX2)

    static inline simd4d simd4d_unsigned_convert(__simd4i i)
    {
        const __m256d bias = _mm256_set1_pd((1ll << 52) * 1.5);
        const __m256i xyzw = _mm256_cvtepu32_epi64(i);
        __m256d v = _mm256_castsi256_pd(xyzw);
        v = _mm256_or_pd(v, bias);
        v = _mm256_sub_pd(v, bias);
        return v;
    }

#else

    static inline simd4d simd4d_unsigned_convert(__simd4i i)
    {
        const __m256d bias = _mm256_set1_pd((1ll << 52) * 1.5);
        const __m128i mask = _mm_set1_epi32(0x43380000);
        const __m128i xy = _mm_unpacklo_epi32(i, mask);
        const __m128i zw = _mm_unpackhi_epi32(i, mask);
        const __m256i xyzw = simd_mm256_set_m128i(zw, xy);
        __m256d v = _mm256_castsi256_pd(xyzw);
        v = _mm256_sub_pd(v, bias);
        return v;
    }

#endif

    static inline simd4i simd4i_unsigned_convert(__simd4d d)
    {
        const __m256d bias = _mm256_set1_pd((1ll << 52) * 1.5);
        const __m256d v = _mm256_add_pd(d, bias);
        const __m128d xxyy = _mm256_castpd256_pd128(v);
        const __m128d zzww = _mm256_extractf128_pd(v, 1);
        __m128 xyzw = _mm_shuffle_ps(_mm_castpd_ps(xxyy), _mm_castpd_ps(zzww), 0x88);
        return _mm_castps_si128(xyzw);
    }

    static inline simd4i simd4i_truncate(__simd4d s)
    {
        return _mm256_cvttpd_epi32(s);
    }

    template <int x, int y, int z, int w>
    static inline simd4d simd4d_shuffle(__simd4d v)
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
    inline simd4d simd4d_shuffle<0, 1, 2, 3>(__simd4d v)
    {
        // .xyzw
        return v;
    }

    template <>
    inline simd4d simd4d_shuffle<0, 0, 0, 0>(__simd4d v)
    {
        // .xxxx
        const __m256d xyxy = _mm256_permute2f128_pd(v, v, 0);
        return _mm256_permute_pd(xyxy, 0);
    }

    template <>
    inline simd4d simd4d_shuffle<1, 1, 1, 1>(__simd4d v)
    {
        // .yyyy
        const __m256d xyxy = _mm256_permute2f128_pd(v, v, 0);
        return _mm256_permute_pd(xyxy, 0xf);
    }

    template <>
    inline simd4d simd4d_shuffle<2, 2, 2, 2>(__simd4d v)
    {
        // .zzzz
        const __m256d zwzw = _mm256_permute2f128_pd(v, v, 0x11);
        return _mm256_permute_pd(zwzw, 0);
    }

    template <>
    inline simd4d simd4d_shuffle<3, 3, 3, 3>(__simd4d v)
    {
        // .wwww
        const __m256d zwzw = _mm256_permute2f128_pd(v, v, 0x11);
        return _mm256_permute_pd(zwzw, 0xf);
    }

    // set component

    template <int Index>
    static inline simd4d simd4d_set_component(__simd4d a, double s);

    template <>
    inline simd4d simd4d_set_component<0>(__simd4d a, double x)
    {
        __m128d xy = _mm256_extractf128_pd(a, 0);
        xy = _mm_move_sd(xy, _mm_set1_pd(x));
        return _mm256_insertf128_pd(a, xy, 0);
    }

    template <>
    inline simd4d simd4d_set_component<1>(__simd4d a, double y)
    {
        __m128d xy = _mm256_extractf128_pd(a, 0);
        xy = _mm_move_sd(_mm_set1_pd(y), xy);
        return _mm256_insertf128_pd(a, xy, 0);
    }

    template <>
    inline simd4d simd4d_set_component<2>(__simd4d a, double z)
    {
        __m128d zw = _mm256_extractf128_pd(a, 1);
        zw = _mm_move_sd(zw, _mm_set1_pd(z));
        return _mm256_insertf128_pd(a, zw, 1);
    }

    template <>
    inline simd4d simd4d_set_component<3>(__simd4d a, double w)
    {
        __m128d zw = _mm256_extractf128_pd(a, 1);
        zw = _mm_move_sd(_mm_set1_pd(w), zw);
        return _mm256_insertf128_pd(a, zw, 1);
    }

    // get component

    template <int Index>
    static inline double simd4d_get_component(__simd4d a);

    template <>
    inline double simd4d_get_component<0>(__simd4d a)
    {
        const __m128d xy = _mm256_extractf128_pd(a, 0);
        return _mm_cvtsd_f64(xy);
    }

    template <>
    inline double simd4d_get_component<1>(__simd4d a)
    {
        const __m128d xy = _mm256_extractf128_pd(a, 0);
        const __m128d yy = _mm_unpackhi_pd(xy, xy);
        return _mm_cvtsd_f64(yy);
    }

    template <>
    inline double simd4d_get_component<2>(__simd4d a)
    {
        const __m128d zw = _mm256_extractf128_pd(a, 1);
        return _mm_cvtsd_f64(zw);
    }

    template <>
    inline double simd4d_get_component<3>(__simd4d a)
    {
        const __m128d zw = _mm256_extractf128_pd(a, 1);
        const __m128d ww = _mm_unpackhi_pd(zw, zw);
        return _mm_cvtsd_f64(ww);
    }

    static inline simd4d simd4d_set_x(__simd4d a, double x)
    {
        return simd4d_set_component<0>(a, x);
    }

    static inline simd4d simd4d_set_y(__simd4d a, double y)
    {
        return simd4d_set_component<1>(a, y);
    }

    static inline simd4d simd4d_set_z(__simd4d a, double z)
    {
        return simd4d_set_component<2>(a, z);
    }

    static inline simd4d simd4d_set_w(__simd4d a, double w)
    {
        return simd4d_set_component<3>(a, w);
    }

    static inline double simd4d_get_x(__simd4d a)
    {
        return simd4d_get_component<0>(a);
    }

    static inline double simd4d_get_y(__simd4d a)
    {
        return simd4d_get_component<1>(a);
    }

    static inline double simd4d_get_z(__simd4d a)
    {
        return simd4d_get_component<2>(a);
    }

    static inline double simd4d_get_w(__simd4d a)
    {
        return simd4d_get_component<3>(a);
    }

    static inline simd4d simd4d_zero()
    {
        return _mm256_setzero_pd();
    }

    static inline simd4d simd4d_set1(double s)
    {
        return _mm256_set1_pd(s);
    }

    static inline simd4d simd4d_set4(double x, double y, double z, double w)
    {
        return _mm256_setr_pd(x, y, z, w);
    }

    static inline simd4d simd4d_load(const double* source)
    {
        return _mm256_load_pd(source);
    }

    static inline simd4d simd4d_uload(const double* source)
    {
        return _mm256_loadu_pd(source);
    }

    static inline void simd4d_store(double* dest, __simd4d a)
    {
        _mm256_store_pd(dest, a);
    }

    static inline void simd4d_ustore(double* dest, __simd4d a)
    {
        _mm256_storeu_pd(dest, a);
    }

    static inline simd4d simd4d_unpackhi(__simd4d a, __simd4d b)
    {
        return _mm256_unpackhi_pd(a, b);
    }

    static inline simd4d simd4d_unpacklo(__simd4d a, __simd4d b)
    {
        return _mm256_unpacklo_pd(a, b);
    }

    // logical

    static inline simd4d simd4d_and(__simd4d a, __simd4d b)
    {
        return _mm256_and_pd(a, b);
    }

    static inline simd4d simd4d_nand(__simd4d a, __simd4d b)
    {
        return _mm256_andnot_pd(a, b);
    }

    static inline simd4d simd4d_or(__simd4d a, __simd4d b)
    {
        return _mm256_or_pd(a, b);
    }

    static inline simd4d simd4d_xor(__simd4d a, __simd4d b)
    {
        return _mm256_xor_pd(a, b);
    }

    static inline simd4d simd4d_min(__simd4d a, __simd4d b)
    {
        return _mm256_min_pd(a, b);
    }

    static inline simd4d simd4d_max(__simd4d a, __simd4d b)
    {
        return _mm256_max_pd(a, b);
    }

    static inline simd4d simd4d_clamp(__simd4d v, __simd4d vmin, __simd4d vmax)
    {
        return _mm256_min_pd(vmax, _mm256_max_pd(vmin, v));
    }

    static inline simd4d simd4d_abs(__simd4d a)
    {
        const __m256i mask = _mm256_set1_epi64x(0x7fffffffffffffff);
        return _mm256_and_pd(a, _mm256_castsi256_pd(mask));
    }

    static inline simd4d simd4d_neg(__simd4d a)
    {
        const __m256i mask = _mm256_set1_epi64x(0x8000000000000000);
        return _mm256_xor_pd(a, _mm256_castsi256_pd(mask));
    }

    static inline simd4d simd4d_add(__simd4d a, __simd4d b)
    {
        return _mm256_add_pd(a, b);
    }

    static inline simd4d simd4d_sub(__simd4d a, __simd4d b)
    {
        return _mm256_sub_pd(a, b);
    }

    static inline simd4d simd4d_mul(__simd4d a, __simd4d b)
    {
        return _mm256_mul_pd(a, b);
    }

    static inline simd4d simd4d_div(__simd4d a, __simd4d b)
    {
        return _mm256_div_pd(a, b);
    }

    static inline simd4d simd4d_div(__simd4d a, double b)
    {
        return _mm256_div_pd(a, _mm256_set1_pd(b));
    }

#if defined(MANGO_ENABLE_FMA3)

    static inline simd4d simd4d_madd(__simd4d a, __simd4d b, __simd4d c)
    {
        return _mm256_fmadd_pd(b, c, a);
    }

    static inline simd4d simd4d_msub(__simd4d a, __simd4d b, __simd4d c)
    {
        return _mm256_fnmadd_pd(b, c, a);
    }

#else

    static inline simd4d simd4d_madd(__simd4d a, __simd4d b, __simd4d c)
    {
        return _mm256_add_pd(a, _mm256_mul_pd(b, c));
    }

    static inline simd4d simd4d_msub(__simd4d a, __simd4d b, __simd4d c)
    {
        return _mm256_sub_pd(a, _mm256_mul_pd(b, c));
    }

#endif

    static inline simd4d simd4d_fast_reciprocal(__simd4d a)
    {
        const __m256d one = _mm256_set1_pd(1.0);
        return _mm256_div_pd(one, a);
    }

    static inline simd4d simd4d_fast_rsqrt(__simd4d a)
    {
        const __m256d one = _mm256_set1_pd(1.0);
        return _mm256_div_pd(one, _mm256_sqrt_pd(a));
    }

    static inline simd4d simd4d_fast_sqrt(__simd4d a)
    {
        return _mm256_sqrt_pd(a);
    }

    static inline simd4d simd4d_reciprocal(__simd4d a)
    {
        const __m256d one = _mm256_set1_pd(1.0);
        return _mm256_div_pd(one, a);
    }

    static inline simd4d simd4d_rsqrt(__simd4d a)
    {
        const __m256d one = _mm256_set1_pd(1.0);
        return _mm256_div_pd(one, _mm256_sqrt_pd(a));
    }

    static inline simd4d simd4d_sqrt(__simd4d a)
    {
        return _mm256_sqrt_pd(a);
    }

    static inline simd4d simd4d_dot4(__simd4d a, __simd4d b)
    {
        const __m256d s = _mm256_mul_pd(a, b);
        const __m256d zwxy = _mm256_permute2f128_pd(s, s, 0x01);
        const __m256d n = _mm256_hadd_pd(s, zwxy);
        return _mm256_hadd_pd(n, n);
    }

    // compare

    static inline simd4d simd4d_compare_neq(__simd4d a, __simd4d b)
    {
        return _mm256_cmp_pd(a, b, 4);
    }

    static inline simd4d simd4d_compare_eq(__simd4d a, __simd4d b)
    {
        return _mm256_cmp_pd(a, b, 0);
    }

    static inline simd4d simd4d_compare_lt(__simd4d a, __simd4d b)
    {
        return _mm256_cmp_pd(a, b, 1);
    }

    static inline simd4d simd4d_compare_le(__simd4d a, __simd4d b)
    {
        return _mm256_cmp_pd(a, b, 2);
    }

    static inline simd4d simd4d_compare_gt(__simd4d a, __simd4d b)
    {
        return _mm256_cmp_pd(b, a, 1);
    }

    static inline simd4d simd4d_compare_ge(__simd4d a, __simd4d b)
    {
        return _mm256_cmp_pd(b, a, 2);
    }

    static inline simd4d simd4d_select(__simd4d mask, __simd4d a, __simd4d b)
    {
        return _mm256_blendv_pd(b, a, mask);
    }

    // rounding

    static inline simd4d simd4d_round(__simd4d s)
    {
        return _mm256_round_pd(s, _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
    }

    static inline simd4d simd4d_trunc(__simd4d s)
    {
        return _mm256_round_pd(s, _MM_FROUND_TO_ZERO |_MM_FROUND_NO_EXC);
    }

    static inline simd4d simd4d_floor(__simd4d s)
    {
        return _mm256_floor_pd(s);
    }

    static inline simd4d simd4d_ceil(__simd4d s)
    {
        return _mm256_ceil_pd(s);
    }

    static inline simd4d simd4d_fract(__simd4d s)
    {
        return _mm256_sub_pd(s, _mm256_floor_pd(s));
    }
