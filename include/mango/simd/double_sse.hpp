/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_SIMD_DOUBLE
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

    // conversion

    static inline simd4d simd4d_convert(__simd4i s)
    {
        simd4d result;
        result.xy = _mm_cvtepi32_pd(s);
        result.zw = _mm_cvtepi32_pd(_mm_shuffle_epi32(s, 0xee));
        return result;
    }

    static inline simd4i simd4i_convert(__simd4d s)
    {
        __m128i xy = _mm_cvtpd_epi32(s.xy);
        __m128i zw = _mm_cvtpd_epi32(s.zw);
        __m128i xzyw = _mm_unpacklo_epi32(xy, zw);
        return _mm_shuffle_epi32(xzyw, 0xd8);
    }

    static inline simd4d simd4d_unsigned_convert(__simd4i i)
    {
        const __m128d bias = _mm_set1_pd((1ll << 52) * 1.5);
        const __m128i mask = _mm_set1_epi32(0x43380000);
        __m128i xy = _mm_unpacklo_epi32(i, mask);
        __m128i zw = _mm_unpackhi_epi32(i, mask);
        simd4d result;
        result.xy = _mm_sub_pd(_mm_castsi128_pd(xy), bias);
        result.zw = _mm_sub_pd(_mm_castsi128_pd(zw), bias);
        return result;
    }

    static inline simd4i simd4i_unsigned_convert(__simd4d d)
    {
        const __m128d bias = _mm_set1_pd((1ll << 52) * 1.5);
        __m128 xy = _mm_castpd_ps(_mm_add_pd(d.xy, bias));
        __m128 zw = _mm_castpd_ps(_mm_add_pd(d.zw, bias));
        __m128 u = _mm_shuffle_ps(xy, zw, 0x88);
        return _mm_castps_si128(u);
    }

    static inline simd4i simd4i_truncate(__simd4d s)
    {
        __m128i xy = _mm_cvttpd_epi32(s.xy);
        __m128i zw = _mm_cvttpd_epi32(s.zw);
        __m128i xzyw = _mm_unpacklo_epi32(xy, zw);
        return _mm_shuffle_epi32(xzyw, 0xd8);
    }

    template <int x, int y, int z, int w>
    static inline simd4d simd4d_shuffle(__simd4d v)
    {
        const int select0 = ((y & 1) << 1) | (x & 1);
        const int select1 = ((w & 1) << 1) | (z & 1);
        const __m128d& v0 = x & 2 ? v.zw : v.xy;
        const __m128d& v1 = y & 2 ? v.zw : v.xy;
        const __m128d& v2 = z & 2 ? v.zw : v.xy;
        const __m128d& v3 = w & 2 ? v.zw : v.xy;
        simd4d result;
        result.xy = _mm_shuffle_pd(v0, v1, select0);
        result.zw = _mm_shuffle_pd(v2, v3, select1);
        return result;
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
        const __m128d xx = _mm_shuffle_pd(v.xy, v.xy, 0);
        simd4d result;
        result.xy = xx;
        result.zw = xx;
        return result;
    }

    template <>
    inline simd4d simd4d_shuffle<1, 1, 1, 1>(__simd4d v)
    {
        // .yyyy
        const __m128d yy = _mm_shuffle_pd(v.xy, v.xy, 3);
        simd4d result;
        result.xy = yy;
        result.zw = yy;
        return result;
    }

    template <>
    inline simd4d simd4d_shuffle<2, 2, 2, 2>(__simd4d v)
    {
        // .zzzz
        const __m128d zz = _mm_shuffle_pd(v.zw, v.zw, 0);
        simd4d result;
        result.xy = zz;
        result.zw = zz;
        return result;
    }

    template <>
    inline simd4d simd4d_shuffle<3, 3, 3, 3>(__simd4d v)
    {
        // .wwww
        const __m128d ww = _mm_shuffle_pd(v.zw, v.zw, 3);
        simd4d result;
        result.xy = ww;
        result.zw = ww;
        return result;
    }

    // set component

    template <int Index>
    static inline simd4d simd4d_set_component(simd4d a, double s);

    template <>
    inline simd4d simd4d_set_component<0>(simd4d a, double x)
    {
        a.xy = _mm_move_sd(a.xy, _mm_set1_pd(x));
        return a;
    }

    template <>
    inline simd4d simd4d_set_component<1>(simd4d a, double y)
    {
        a.xy = _mm_move_sd(_mm_set1_pd(y), a.xy);
        return a;
    }

    template <>
    inline simd4d simd4d_set_component<2>(simd4d a, double z)
    {
        a.zw = _mm_move_sd(a.zw, _mm_set1_pd(z));
        return a;
    }

    template <>
    inline simd4d simd4d_set_component<3>(simd4d a, double w)
    {
        a.zw = _mm_move_sd(_mm_set1_pd(w), a.zw);
        return a;
    }

    // get component

    template <int Index>
    static inline double simd4d_get_component(__simd4d a);

    template <>
    inline double simd4d_get_component<0>(__simd4d a)
    {
        return _mm_cvtsd_f64(a.xy);
    }

    template <>
    inline double simd4d_get_component<1>(__simd4d a)
    {
        const __m128d yy = _mm_unpackhi_pd(a.xy, a.xy);
        return _mm_cvtsd_f64(yy);
    }

    template <>
    inline double simd4d_get_component<2>(__simd4d a)
    {
        return _mm_cvtsd_f64(a.zw);
    }

    template <>
    inline double simd4d_get_component<3>(__simd4d a)
    {
        const __m128d ww = _mm_unpackhi_pd(a.zw, a.zw);
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
        simd4d result;
        result.xy =
        result.zw = _mm_setzero_pd();
        return result;
    }

    static inline simd4d simd4d_set1(double s)
    {
        simd4d result;
        result.xy =
        result.zw = _mm_set1_pd(s);
        return result;
    }

    static inline simd4d simd4d_set4(double x, double y, double z, double w)
    {
        simd4d result;
        result.xy = _mm_setr_pd(x, y);
        result.zw = _mm_setr_pd(z, w);
        return result;
    }

    static inline simd4d simd4d_load(const double* source)
    {
        simd4d result;
        result.xy = _mm_load_pd(source + 0);
        result.zw = _mm_load_pd(source + 2);
        return result;
    }

    static inline simd4d simd4d_uload(const double* source)
    {
        simd4d result;
        result.xy = _mm_loadu_pd(source + 0);
        result.zw = _mm_loadu_pd(source + 2);
        return result;
    }

    static inline void simd4d_store(double* dest, __simd4d a)
    {
        _mm_store_pd(dest + 0, a.xy);
        _mm_store_pd(dest + 2, a.zw);
    }

    static inline void simd4d_ustore(double* dest, __simd4d a)
    {
        _mm_storeu_pd(dest + 0, a.xy);
        _mm_storeu_pd(dest + 2, a.zw);
    }

    static inline simd4d simd4d_unpackhi(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_unpackhi_pd(a.xy, b.xy);
        result.zw = _mm_unpackhi_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_unpacklo(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_unpacklo_pd(a.xy, b.xy);
        result.zw = _mm_unpacklo_pd(a.zw, b.zw);
        return result;
    }

    // logical

    static inline simd4d simd4d_and(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_and_pd(a.xy, b.xy);
        result.zw = _mm_and_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_nand(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_andnot_pd(a.xy, b.xy);
        result.zw = _mm_andnot_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_or(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_or_pd(a.xy, b.xy);
        result.zw = _mm_or_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_xor(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_xor_pd(a.xy, b.xy);
        result.zw = _mm_xor_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_min(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_min_pd(a.xy, b.xy);
        result.zw = _mm_min_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_max(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_max_pd(a.xy, b.xy);
        result.zw = _mm_max_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_clamp(__simd4d v, __simd4d vmin, __simd4d vmax)
    {
        simd4d result;
        result.xy = _mm_min_pd(vmax.xy, _mm_max_pd(vmin.xy, v.xy));
        result.zw = _mm_min_pd(vmax.zw, _mm_max_pd(vmin.zw, v.zw));
        return result;
    }

    static inline simd4d simd4d_abs(__simd4d a)
    {
        const __m128d mask = _mm_castsi128_pd(_mm_set1_epi64x(0x7fffffffffffffff));
        simd4d result;
        result.xy = _mm_and_pd(a.xy, mask);
        result.zw = _mm_and_pd(a.zw, mask);
        return result;
    }

    static inline simd4d simd4d_neg(__simd4d a)
    {
        const __m128d mask = _mm_castsi128_pd(_mm_set1_epi64x(0x8000000000000000));
        simd4d result;
        result.xy = _mm_xor_pd(a.xy, mask);
        result.zw = _mm_xor_pd(a.zw, mask);
        return result;
    }

    static inline simd4d simd4d_add(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_add_pd(a.xy, b.xy);
        result.zw = _mm_add_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_sub(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_sub_pd(a.xy, b.xy);
        result.zw = _mm_sub_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_mul(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_mul_pd(a.xy, b.xy);
        result.zw = _mm_mul_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_div(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_div_pd(a.xy, b.xy);
        result.zw = _mm_div_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_div(__simd4d a, double b)
    {
        const __m128d bb = _mm_set1_pd(b);
        simd4d result;
        result.xy = _mm_xor_pd(a.xy, bb);
        result.zw = _mm_xor_pd(a.zw, bb);
        return result;
    }

#if defined(MANGO_ENABLE_FMA3)

    static inline simd4d simd4d_madd(__simd4d a, __simd4d b, __simd4d c)
    {
        simd4d result;
        result.xy = _mm_fmadd_pd(b.xy, c.xy, a.xy);
        result.zw = _mm_fmadd_pd(b.zw, c.zw, a.zw);
        return result;
    }

    static inline simd4d simd4d_msub(__simd4d a, __simd4d b, __simd4d c)
    {
        simd4d result;
        result.xy = _mm_fnmadd_pd(b.xy, c.xy, a.xy);
        result.zw = _mm_fnmadd_pd(b.zw, c.zw, a.zw);
        return result;
    }

#else

    static inline simd4d simd4d_madd(__simd4d a, __simd4d b, __simd4d c)
    {
        simd4d result;
        result.xy = _mm_add_pd(a.xy, _mm_mul_pd(b.xy, c.xy));
        result.zw = _mm_add_pd(a.zw, _mm_mul_pd(b.zw, c.zw));
        return result;
    }

    static inline simd4d simd4d_msub(__simd4d a, __simd4d b, __simd4d c)
    {
        simd4d result;
        result.xy = _mm_sub_pd(a.xy, _mm_mul_pd(b.xy, c.xy));
        result.zw = _mm_sub_pd(a.zw, _mm_mul_pd(b.zw, c.zw));
        return result;
    }

#endif

    static inline simd4d simd4d_fast_reciprocal(__simd4d a)
    {
        const __m128d one = _mm_set1_pd(1.0);
        simd4d result;
        result.xy = _mm_div_pd(one, a.xy);
        result.zw = _mm_div_pd(one, a.zw);
        return result;
    }

    static inline simd4d simd4d_fast_rsqrt(__simd4d a)
    {
        const __m128d one = _mm_set1_pd(1.0);
        simd4d result;
        result.xy = _mm_div_pd(one, _mm_sqrt_pd(a.xy));
        result.zw = _mm_div_pd(one, _mm_sqrt_pd(a.zw));
        return result;
    }

    static inline simd4d simd4d_fast_sqrt(__simd4d a)
    {
        simd4d result;
        result.xy = _mm_sqrt_pd(a.xy);
        result.zw = _mm_sqrt_pd(a.zw);
        return result;
    }

    static inline simd4d simd4d_reciprocal(__simd4d a)
    {
        const __m128d one = _mm_set1_pd(1.0);
        simd4d result;
        result.xy = _mm_div_pd(one, a.xy);
        result.zw = _mm_div_pd(one, a.zw);
        return result;
    }

    static inline simd4d simd4d_rsqrt(__simd4d a)
    {
        const __m128d one = _mm_set1_pd(1.0);
        simd4d result;
        result.xy = _mm_div_pd(one, _mm_sqrt_pd(a.xy));
        result.zw = _mm_div_pd(one, _mm_sqrt_pd(a.zw));
        return result;
    }

    static inline simd4d simd4d_sqrt(__simd4d a)
    {
        simd4d result;
        result.xy = _mm_sqrt_pd(a.xy);
        result.zw = _mm_sqrt_pd(a.zw);
        return result;
    }

    static inline simd4d simd4d_dot4(__simd4d a, __simd4d b)
    {
        const __m128d xy = _mm_mul_pd(a.xy, b.xy);
        const __m128d zw = _mm_mul_pd(a.zw, b.zw);
        __m128d s;
        s = _mm_add_pd(xy, zw);
        s = _mm_add_pd(s, _mm_shuffle_pd(xy, xy, 0x01));
        s = _mm_add_pd(s, _mm_shuffle_pd(zw, zw, 0x01));

        simd4d result;
        result.xy = s;
        result.zw = s;
        return result;
    }

    // compare

    static inline simd4d simd4d_compare_neq(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_cmpneq_pd(a.xy, b.xy);
        result.zw = _mm_cmpneq_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_compare_eq(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_cmpeq_pd(a.xy, b.xy);
        result.zw = _mm_cmpeq_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_compare_lt(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_cmplt_pd(a.xy, b.xy);
        result.zw = _mm_cmplt_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_compare_le(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_cmple_pd(a.xy, b.xy);
        result.zw = _mm_cmple_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_compare_gt(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_cmpgt_pd(a.xy, b.xy);
        result.zw = _mm_cmpgt_pd(a.zw, b.zw);
        return result;
    }

    static inline simd4d simd4d_compare_ge(__simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_cmpge_pd(a.xy, b.xy);
        result.zw = _mm_cmpge_pd(a.zw, b.zw);
        return result;
    }

#if defined(MANGO_ENABLE_SSE4_1)

    static inline simd4d simd4d_select(__simd4d mask, __simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_blendv_pd(b.xy, a.xy, mask.xy);
        result.zw = _mm_blendv_pd(b.zw, a.zw, mask.zw);
        return result;
    }

#else

    static inline simd4d simd4d_select(__simd4d mask, __simd4d a, __simd4d b)
    {
        simd4d result;
        result.xy = _mm_or_pd(_mm_and_pd(mask.xy, a.xy), _mm_andnot_pd(mask.xy, b.xy));
        result.zw = _mm_or_pd(_mm_and_pd(mask.zw, a.zw), _mm_andnot_pd(mask.zw, b.zw));
        return result;
    }

#endif

    // rounding

#if defined(MANGO_ENABLE_SSE4_1)

    static inline simd4d simd4d_round(__simd4d s)
    {
        simd4d result;
        result.xy = _mm_round_pd(s.xy, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
        result.zw = _mm_round_pd(s.zw, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
        return result;
    }

    static inline simd4d simd4d_trunc(__simd4d s)
    {
        simd4d result;
        result.xy = _mm_round_pd(s.xy, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
        result.zw = _mm_round_pd(s.zw, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
        return result;
    }

    static inline simd4d simd4d_floor(__simd4d s)
    {
        simd4d result;
        result.xy = _mm_round_pd(s.xy, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
        result.zw = _mm_round_pd(s.zw, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
        return result;
    }

    static inline simd4d simd4d_ceil(__simd4d s)
    {
        simd4d result;
        result.xy = _mm_round_pd(s.xy, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
        result.zw = _mm_round_pd(s.zw, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
        return result;
    }

#else

    static inline simd4d simd4d_round(__simd4d s)
    {
        simd4d result;
        result.xy = _mm_cvtepi32_pd(_mm_cvtpd_epi32(s.xy));
        result.zw = _mm_cvtepi32_pd(_mm_cvtpd_epi32(s.zw));
        return result;
    }

    static inline simd4d simd4d_trunc(__simd4d s)
    {
        simd4d result;
        result.xy = _mm_cvtepi32_pd(_mm_cvttpd_epi32(s.xy));
        result.zw = _mm_cvtepi32_pd(_mm_cvttpd_epi32(s.zw));
        return result;
    }

    static inline simd4d simd4d_floor(__simd4d s)
    {
        const __m128d one = _mm_set1_pd(1.0);
        const __m128d temp_xy = _mm_cvtepi32_pd(_mm_cvtpd_epi32(s.xy));
        const __m128d temp_zw = _mm_cvtepi32_pd(_mm_cvtpd_epi32(s.zw));
        const __m128d mask_xy = _mm_cmplt_pd(s.xy, temp_xy);
        const __m128d mask_zw = _mm_cmplt_pd(s.zw, temp_zw);

        simd4d result;
        result.xy = _mm_sub_pd(temp_xy, _mm_and_pd(mask_xy, one));
        result.zw = _mm_sub_pd(temp_zw, _mm_and_pd(mask_zw, one));
        return result;
    }

    static inline simd4d simd4d_ceil(__simd4d s)
    {
        const __m128d one = _mm_set1_pd(1.0);
        const __m128d temp_xy = _mm_cvtepi32_pd(_mm_cvtpd_epi32(s.xy));
        const __m128d temp_zw = _mm_cvtepi32_pd(_mm_cvtpd_epi32(s.zw));
        const __m128d mask_xy = _mm_cmpgt_pd(s.xy, temp_xy);
        const __m128d mask_zw = _mm_cmpgt_pd(s.zw, temp_zw);

        simd4d result;
        result.xy = _mm_add_pd(temp_xy, _mm_and_pd(mask_xy, one));
        result.zw = _mm_add_pd(temp_zw, _mm_and_pd(mask_zw, one));
        return result;
    }

#endif

    static inline simd4d simd4d_fract(__simd4d s)
    {
        const __m128d one = _mm_set1_pd(1.0);
        const __m128d temp_xy = _mm_cvtepi32_pd(_mm_cvtpd_epi32(s.xy));
        const __m128d temp_zw = _mm_cvtepi32_pd(_mm_cvtpd_epi32(s.zw));
        const __m128d mask_xy = _mm_cmplt_pd(s.xy, temp_xy);
        const __m128d mask_zw = _mm_cmplt_pd(s.zw, temp_zw);
        const __m128d xy = _mm_sub_pd(temp_xy, _mm_and_pd(mask_xy, one));
        const __m128d zw = _mm_sub_pd(temp_zw, _mm_and_pd(mask_zw, one));

        simd4d result;
        result.xy = _mm_sub_pd(s.xy, xy);
        result.zw = _mm_sub_pd(s.zw, zw);
        return result;
    }
