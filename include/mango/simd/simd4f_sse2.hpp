/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_INCLUDE_SIMD
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

#include <cmath>
#include <algorithm>

namespace mango
{

    // ------------------------------------------------------------------
    // Intel SSE2
    // ------------------------------------------------------------------

    struct simd4h
    {
        half x, y, z, w;
    };

    typedef __m128 simd4f;
    typedef __m128i simd4i;

#if defined(MANGO_COMPILER_MICROSOFT) || defined(MANGO_COMPILER_INTEL)
    typedef const simd4h& __simd4h;
    typedef const __m128& __simd4f;
    typedef const __m128i& __simd4i;
#else
    typedef const simd4h& __simd4h;
    typedef const __m128 __simd4f;
    typedef const __m128i __simd4i;
#endif

    // -----------------------------------------------------------------
    // conversions
    // -----------------------------------------------------------------

    static inline simd4f simd4f_cast(__simd4i s)
    {
        return _mm_castsi128_ps(s);
    }

    static inline simd4i simd4i_cast(__simd4f s)
    {
        return _mm_castps_si128(s);
    }

    static inline simd4f simd4f_convert(__simd4i s)
    {
        return _mm_cvtepi32_ps(s);
    }

    static inline simd4f simd4f_unsigned_convert(__simd4i s)
    {
#if 1
        const __m128i mask = _mm_set1_epi32(0x0000ffff);
        const __m128i onep39 = _mm_set1_epi32(0x53000000);

        const __m128i x0 = _mm_or_si128(_mm_srli_epi32(s, 16), onep39);
        const __m128i x1 = _mm_and_si128(s, mask);
        const __m128 f1 = _mm_cvtepi32_ps(x1);
        const __m128 f0 = _mm_sub_ps(simd4f_cast(x0), simd4f_cast(onep39));
        return _mm_add_ps(f0, f1);
#else
        const __m128 two16 = _mm_set1_ps(0x1.0p16f);
        const __m128i hi = _mm_srli_epi32((__m128i)s, 16);
        const __m128i lo = _mm_srli_epi32(_mm_slli_epi32((__m128i)s, 16), 16);
        const __m128 high = _mm_mul_ps(_mm_cvtepi32_ps(hi), two16);
        const __m128 low = _mm_cvtepi32_ps(lo);
        return _mm_add_ps(high, low);
#endif
    }

    static inline simd4i simd4i_convert(__simd4f s)
    {
        return _mm_cvtps_epi32(s);
    }

    static inline simd4i simd4i_truncate(__simd4f s)
    {
        return _mm_cvttps_epi32(s);
    }

    // -----------------------------------------------------------------
    // simd4i
    // -----------------------------------------------------------------

#if defined(MANGO_ENABLE_SSE4_1)

    // set

    static inline simd4i simd4i_set_x(__simd4i a, int x)
    {
        return _mm_insert_epi32(a, x, 0);
    }

    static inline simd4i simd4i_set_y(__simd4i a, int y)
    {
        return _mm_insert_epi32(a, y, 1);
    }

    static inline simd4i simd4i_set_z(__simd4i a, int z)
    {
        return _mm_insert_epi32(a, z, 2);
    }

    static inline simd4i simd4i_set_w(__simd4i a, int w)
    {
        return _mm_insert_epi32(a, w, 3);
    }

    // get

    static inline int simd4i_get_x(__simd4i a)
    {
        return _mm_extract_epi32(a, 0);
    }

    static inline int simd4i_get_y(__simd4i a)
    {
        return _mm_extract_epi32(a, 1);
    }

    static inline int simd4i_get_z(__simd4i a)
    {
        return _mm_extract_epi32(a, 2);
    }

    static inline int simd4i_get_w(__simd4i a)
    {
        return _mm_extract_epi32(a, 3);
    }

#else

    // set

#define _mm_shuffle_epi(a, b, mask) \
    _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), mask));

    static inline simd4i simd4i_set_x(__simd4i a, int x)
    {
        const __m128i b = _mm_unpacklo_epi32(_mm_set1_epi32(x), a);
        return _mm_shuffle_epi(b, a, _MM_SHUFFLE(3, 2, 3, 0));
    }

    static inline simd4i simd4i_set_y(__simd4i a, int y)
    {
        const __m128i b = _mm_unpacklo_epi32(_mm_set1_epi32(y), a);
        return _mm_shuffle_epi(b, a, _MM_SHUFFLE(3, 2, 0, 1));
    }

    static inline simd4i simd4i_set_z(__simd4i a, int z)
    {
        const __m128i b = _mm_unpackhi_epi32(_mm_set1_epi32(z), a);
        return _mm_shuffle_epi(a, b, _MM_SHUFFLE(3, 0, 1, 0));
    }

    static inline simd4i simd4i_set_w(__simd4i a, int w)
    {
        const __m128i b = _mm_unpackhi_epi32(_mm_set1_epi32(w), a);
        return _mm_shuffle_epi(a, b, _MM_SHUFFLE(0, 1, 1, 0));
    }

#undef _mm_shuffle_epi

    // get

    static inline int simd4i_get_x(__simd4i a)
    {
        return _mm_cvtsi128_si32(a);
    }

    static inline int simd4i_get_y(__simd4i a)
    {
        return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, 0x55));
    }

    static inline int simd4i_get_z(__simd4i a)
    {
        return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, 0xaa));
    }

    static inline int simd4i_get_w(__simd4i a)
    {
        return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, 0xff));
    }

#endif

    static inline simd4i simd4i_load(const int* source)
    {
        return _mm_load_si128(reinterpret_cast<const __m128i*>(source));
    }

    static inline simd4i simd4i_uload(const int* source)
    {
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(source));
    }

    static inline void simd4i_store(int* dest, __simd4i a)
    {
        _mm_store_si128(reinterpret_cast<__m128i*>(dest), a);
    }

    static inline void simd4i_ustore(int* dest, __simd4i a)
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest), a);
    }

    static inline simd4i simd4i_zero()
    {
        return _mm_setzero_si128();
    }

    static inline simd4i simd4i_set1(int s)
    {
        return _mm_set1_epi32(s);
    }

    static inline simd4i simd4i_set4(int x, int y, int z, int w)
    {
        return _mm_setr_epi32(x, y, z, w);
    }

    static inline simd4i simd4i_neg(__simd4i a)
    {
        return _mm_xor_si128(a, _mm_set1_epi32(0x80000000));
    }

    static inline simd4i simd4i_add(__simd4i a, __simd4i b)
    {
        return _mm_add_epi32(a, b);
    }

    static inline simd4i simd4i_sub(__simd4i a, __simd4i b)
    {
        return _mm_sub_epi32(a, b);
    }

    // logical

    static inline simd4i simd4i_and(__simd4i a, __simd4i b)
    {
        return _mm_and_si128(a, b);
    }

    static inline simd4i simd4i_nand(__simd4i a, __simd4i b)
    {
        return _mm_andnot_si128(a, b);
    }

    static inline simd4i simd4i_or(__simd4i a, __simd4i b)
    {
        return _mm_or_si128(a, b);
    }

    static inline simd4i simd4i_xor(__simd4i a, __simd4i b)
    {
        return _mm_xor_si128(a, b);
    }

    // shift

    static inline simd4i simd4i_sll(__simd4i a, int b)
    {
        return _mm_slli_epi32(a, b);
    }

    static inline simd4i simd4i_srl(__simd4i a, int b)
    {
        return _mm_srli_epi32(a, b);
    }

    static inline simd4i simd4i_sra(__simd4i a, int b)
    {
        return _mm_srai_epi32(a, b);
    }

    // compare

    static inline simd4i simd4i_compare_eq(__simd4i a, __simd4i b)
    {
        return _mm_cmpeq_epi32(a, b);
    }

    static inline simd4i simd4i_compare_gt(__simd4i a, __simd4i b)
    {
        return _mm_cmpgt_epi32(a, b);
    }

    static inline simd4i simd4i_select(__simd4i mask, __simd4i a, __simd4i b)
    {
        return simd4i_or(simd4i_and(mask, a), simd4i_nand(mask, b));
    }

    static inline uint32 simd4i_get_mask(__simd4i a)
    {
        const simd4f s = simd4f_cast(a);
        return _mm_movemask_ps(s);
    }

    static inline uint32 simd4i_pack(__simd4i s)
    {
        __m128i s16 = _mm_packs_epi32(s, s);
        __m128i s8 = _mm_packus_epi16(s16, s16);
        return _mm_cvtsi128_si32(s8);
    }

#if defined(MANGO_ENABLE_SSE4_1)

    static inline simd4i simd4i_unpack(uint32 s)
    {
        const __m128i i = _mm_cvtsi32_si128(s);
        return _mm_cvtepu8_epi32(i);
    }

#else

    static inline simd4i simd4i_unpack(uint32 s)
    {
        const __m128i zero = _mm_setzero_si128();
        const __m128i i = _mm_cvtsi32_si128(s);
        return _mm_unpacklo_epi16(_mm_unpacklo_epi8(i, zero), zero);
    }

#endif

    // -----------------------------------------------------------------
    // simd4f
    // -----------------------------------------------------------------

    // shuffle

    template <int x, int y, int z, int w>
    static inline simd4f simd4f_shuffle(__simd4f v)
    {
        // .generic
        return _mm_shuffle_ps(v, v, _MM_SHUFFLE(w, z, y, x));
    }

    template <>
    inline simd4f simd4f_shuffle<0, 1, 2, 3>(__simd4f v)
    {
        // .xyzw
        return v;
    }

    // logical

    static inline simd4f simd4f_and(__simd4f a, __simd4f b)
    {
        return _mm_and_ps(a, b);
    }

    static inline simd4f simd4f_nand(__simd4f a, __simd4f b)
    {
        return _mm_andnot_ps(a, b);
    }

    static inline simd4f simd4f_or(__simd4f a, __simd4f b)
    {
        return _mm_or_ps(a, b);
    }

    static inline simd4f simd4f_xor(__simd4f a, __simd4f b)
    {
        return _mm_xor_ps(a, b);
    }

    // set component

#if defined(MANGO_ENABLE_SSE4_1)

    template <int Index>
    static inline simd4f simd4f_set_component(__simd4f a, float s)
    {
        return _mm_insert_ps(a, _mm_set_ss(s), Index * 0x10);
    }

#else

    template <int Index>
    static inline simd4f simd4f_set_component(__simd4f a, float s);

    template <>
    inline simd4f simd4f_set_component<0>(__simd4f a, float x)
    {
        const __m128 b = _mm_unpacklo_ps(_mm_set_ps1(x), a);
        return _mm_shuffle_ps(b, a, _MM_SHUFFLE(3, 2, 3, 0));
    }

    template <>
    inline simd4f simd4f_set_component<1>(__simd4f a, float y)
    {
        const __m128 b = _mm_unpacklo_ps(_mm_set_ps1(y), a);
        return _mm_shuffle_ps(b, a, _MM_SHUFFLE(3, 2, 0, 1));
    }

    template <>
    inline simd4f simd4f_set_component<2>(__simd4f a, float z)
    {
        const __m128 b = _mm_unpackhi_ps(_mm_set_ps1(z), a);
        return _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 0, 1, 0));
    }

    template <>
    inline simd4f simd4f_set_component<3>(__simd4f a, float w)
    {
        const __m128 b = _mm_unpackhi_ps(_mm_set_ps1(w), a);
        return _mm_shuffle_ps(a, b, _MM_SHUFFLE(0, 1, 1, 0));
    }

#endif

    // get component

    template <int Index>
    static inline float simd4f_get_component(__simd4f a);

    template <>
    inline float simd4f_get_component<0>(__simd4f a)
    {
        return _mm_cvtss_f32(a);
    }

    template <>
    inline float simd4f_get_component<1>(__simd4f a)
    {
        return _mm_cvtss_f32(simd4f_shuffle<1, 1, 1, 1>(a));
    }

    template <>
    inline float simd4f_get_component<2>(__simd4f a)
    {
        return _mm_cvtss_f32(simd4f_shuffle<2, 2, 2, 2>(a));
    }

    template <>
    inline float simd4f_get_component<3>(__simd4f a)
    {
        return _mm_cvtss_f32(simd4f_shuffle<3, 3, 3, 3>(a));
    }

    static inline simd4f simd4f_set_x(__simd4f a, float x)
    {
        return simd4f_set_component<0>(a, x);
    }

    static inline simd4f simd4f_set_y(__simd4f a, float y)
    {
        return simd4f_set_component<1>(a, y);
    }

    static inline simd4f simd4f_set_z(__simd4f a, float z)
    {
        return simd4f_set_component<2>(a, z);
    }

    static inline simd4f simd4f_set_w(__simd4f a, float w)
    {
        return simd4f_set_component<3>(a, w);
    }

    static inline float simd4f_get_x(__simd4f a)
    {
        return simd4f_get_component<0>(a);
    }

    static inline float simd4f_get_y(__simd4f a)
    {
        return simd4f_get_component<1>(a);
    }

    static inline float simd4f_get_z(__simd4f a)
    {
        return simd4f_get_component<2>(a);
    }

    static inline float simd4f_get_w(__simd4f a)
    {
        return simd4f_get_component<3>(a);
    }

    static inline simd4f simd4f_splat_x(__simd4f a)
    {
        return _mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 0, 0, 0));
    }

    static inline simd4f simd4f_splat_y(__simd4f a)
    {
        return _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1));
    }

    static inline simd4f simd4f_splat_z(__simd4f a)
    {
        return _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 2, 2, 2));
    }

    static inline simd4f simd4f_splat_w(__simd4f a)
    {
        return _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 3, 3, 3));
    }

    static inline simd4f simd4f_zero()
    {
        return _mm_setzero_ps();
    }

    static inline simd4f simd4f_set1(float s)
    {
        return _mm_set1_ps(s);
    }

    static inline simd4f simd4f_set4(float x, float y, float z, float w)
    {
        return _mm_setr_ps(x, y, z, w);
    }

    static inline simd4f simd4f_load(const float* source)
    {
        return _mm_load_ps(source);
    }

    static inline simd4f simd4f_uload(const float* source)
    {
        return _mm_loadu_ps(source);
    }

    static inline void simd4f_store(float* dest, __simd4f a)
    {
        _mm_store_ps(dest, a);
    }

    static inline void simd4f_ustore(float* dest, __simd4f a)
    {
        _mm_storeu_ps(dest, a);
    }

    static inline simd4f simd4f_movelh(__simd4f a, __simd4f b)
    {
        return _mm_movelh_ps(a, b);
    }

    static inline simd4f simd4f_movehl(__simd4f a, __simd4f b)
    {
        return _mm_movehl_ps(a, b);
    }

    static inline simd4f simd4f_unpackhi(__simd4f a, __simd4f b)
    {
        return _mm_unpackhi_ps(a, b);
    }

    static inline simd4f simd4f_unpacklo(__simd4f a, __simd4f b)
    {
        return _mm_unpacklo_ps(a, b);
    }

    static inline simd4f simd4f_min(__simd4f a, __simd4f b)
    {
        return _mm_min_ps(a, b);
    }

    static inline simd4f simd4f_max(__simd4f a, __simd4f b)
    {
        return _mm_max_ps(a, b);
    }

    static inline simd4f simd4f_clamp(__simd4f v, __simd4f vmin, __simd4f vmax)
    {
        return _mm_min_ps(vmax, _mm_max_ps(vmin, v));
    }

    static inline simd4f simd4f_abs(__simd4f a)
    {
        return _mm_and_ps(a, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
    }

    static inline simd4f simd4f_neg(__simd4f a)
    {
        return _mm_xor_ps(a, _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));
    }

    static inline simd4f simd4f_add(__simd4f a, __simd4f b)
    {
        return _mm_add_ps(a, b);
    }

    static inline simd4f simd4f_sub(__simd4f a, __simd4f b)
    {
        return _mm_sub_ps(a, b);
    }

    static inline simd4f simd4f_mul(__simd4f a, __simd4f b)
    {
        return _mm_mul_ps(a, b);
    }

    static inline simd4f simd4f_div(__simd4f a, __simd4f b)
    {
        return _mm_div_ps(a, b);
    }

    static inline simd4f simd4f_div(__simd4f a, float b)
    {
        return _mm_div_ps(a, _mm_set1_ps(b));
    }

#if defined(MANGO_ENABLE_FMA3)

    static inline simd4f simd4f_madd(__simd4f a, __simd4f b, __simd4f c)
    {
        return _mm_fmadd_ps(b, c, a);
    }

    static inline simd4f simd4f_msub(__simd4f a, __simd4f b, __simd4f c)
    {
        return _mm_fnmadd_ps(b, c, a);
    }

#elif defined(MANGO_ENABLE_FMA4)

    static inline simd4f simd4f_madd(__simd4f a, __simd4f b, __simd4f c)
    {
        return _mm_macc_ps(b, c, a);
    }

    static inline simd4f simd4f_msub(__simd4f a, __simd4f b, __simd4f c)
    {
        return _mm_sub_ps(a, _mm_mul_ps(b, c));
    }

#else

    static inline simd4f simd4f_madd(__simd4f a, __simd4f b, __simd4f c)
    {
        return _mm_add_ps(a, _mm_mul_ps(b, c));
    }

    static inline simd4f simd4f_msub(__simd4f a, __simd4f b, __simd4f c)
    {
        return _mm_sub_ps(a, _mm_mul_ps(b, c));
    }

#endif

    static inline simd4f simd4f_fast_reciprocal(__simd4f a)
    {
        return _mm_rcp_ps(a);
    }

    static inline simd4f simd4f_fast_rsqrt(__simd4f a)
    {
        return _mm_rsqrt_ps(a);
    }

    static inline simd4f simd4f_fast_sqrt(__simd4f a)
    {
        simd4f n = _mm_rsqrt_ps(a);
        return _mm_mul_ps(a, n);
    }

    static inline simd4f simd4f_reciprocal(__simd4f a)
    {
        simd4f n = _mm_rcp_ps(a);
        simd4f m = _mm_mul_ps(_mm_mul_ps(n, n), a);
        return _mm_sub_ps(_mm_add_ps(n, n), m);
    }

    static inline simd4f simd4f_rsqrt(__simd4f a)
    {
        simd4f n = _mm_rsqrt_ps(a);
        simd4f e = _mm_mul_ps(_mm_mul_ps(n, n), a);
        n = _mm_mul_ps(_mm_set_ps1(0.5f), n);
        e = _mm_sub_ps(_mm_set_ps1(3.0f), e);
        return _mm_mul_ps(n, e);
    }

    static inline simd4f simd4f_sqrt(__simd4f a)
    {
        return _mm_sqrt_ps(a);
    }

    static inline simd4f simd4f_dot3(__simd4f a, __simd4f b)
    {
#if defined(MANGO_ENABLE_SSE4_1)
        return _mm_dp_ps(a, b, 0x7f);
#else
        simd4f s = _mm_mul_ps(a, b);
        return _mm_add_ps(simd4f_shuffle<0, 0, 0, 0>(s),
               _mm_add_ps(simd4f_shuffle<1, 1, 1, 1>(s), simd4f_shuffle<2, 2, 2, 2>(s)));
#endif
    }

    static inline simd4f simd4f_dot4(__simd4f a, __simd4f b)
    {
#if defined(MANGO_ENABLE_SSE4_1)
        return _mm_dp_ps(a, b, 0xff);
#elif defined(MANGO_ENABLE_SSE3)
        simd4f s = _mm_mul_ps(a, b);
        s = _mm_hadd_ps(s, s);
        s = _mm_hadd_ps(s, s);
        return s;
#else
        simd4f s = _mm_mul_ps(a, b);
        s = _mm_add_ps(s, simd4f_shuffle<2, 3, 0, 1>(s));
        s = _mm_add_ps(s, simd4f_shuffle<1, 0, 3, 2>(s));
        return s;
#endif
    }

    static inline simd4f simd4f_cross3(__simd4f a, __simd4f b)
    {
        simd4f c = _mm_sub_ps(_mm_mul_ps(a, simd4f_shuffle<1, 2, 0, 3>(b)),
                              _mm_mul_ps(b, simd4f_shuffle<1, 2, 0, 3>(a)));
        return simd4f_shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline simd4f simd4f_compare_neq(__simd4f a, __simd4f b)
    {
        return _mm_cmpneq_ps(a, b);
    }

    static inline simd4f simd4f_compare_eq(__simd4f a, __simd4f b)
    {
        return _mm_cmpeq_ps(a, b);
    }

    static inline simd4f simd4f_compare_lt(__simd4f a, __simd4f b)
    {
        return _mm_cmplt_ps(a, b);
    }

    static inline simd4f simd4f_compare_le(__simd4f a, __simd4f b)
    {
        return _mm_cmple_ps(a, b);
    }

    static inline simd4f simd4f_compare_gt(__simd4f a, __simd4f b)
    {
        return _mm_cmpgt_ps(a, b);
    }

    static inline simd4f simd4f_compare_ge(__simd4f a, __simd4f b)
    {
        return _mm_cmpge_ps(a, b);
    }

#if defined(MANGO_ENABLE_SSE4_1)

    static inline simd4f simd4f_select(__simd4f mask, __simd4f a, __simd4f b)
    {
        return _mm_blendv_ps(b, a, mask);
    }

#else

    static inline simd4f simd4f_select(__simd4f mask, __simd4f a, __simd4f b)
    {
#if 1
        return simd4f_or(simd4f_and(mask, a), simd4f_nand(mask, b));
#else
        return _mm_xor_ps(b, _mm_and_ps(mask, _mm_xor_ps(a, b)));
#endif
    }

#endif

    // rounding

#if defined(MANGO_ENABLE_SSE4_1)

    static inline simd4f simd4f_round(__simd4f s)
    {
        return _mm_round_ps(s, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
    }

    static inline simd4f simd4f_trunc(__simd4f s)
    {
        return _mm_round_ps(s, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
    }

    static inline simd4f simd4f_floor(__simd4f s)
    {
        return _mm_round_ps(s, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    }

    static inline simd4f simd4f_ceil(__simd4f s)
    {
        return _mm_round_ps(s, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
    }

#else

    static inline simd4f simd4f_round(__simd4f s)
    {
        return _mm_cvtepi32_ps(_mm_cvtps_epi32(s));
    }

    static inline simd4f simd4f_trunc(__simd4f s)
    {
        return _mm_cvtepi32_ps(_mm_cvttps_epi32(s));
    }

    static inline simd4f simd4f_floor(__simd4f s)
    {
        const simd4f temp = simd4f_round(s);
        const simd4f mask = _mm_cmplt_ps(s, temp);
        return _mm_sub_ps(temp, _mm_and_ps(mask, _mm_set1_ps(1.0f)));
    }

    static inline simd4f simd4f_ceil(__simd4f s)
    {
        const simd4f temp = simd4f_round(s);
        const simd4f mask = _mm_cmpgt_ps(s, temp);
        return _mm_add_ps(temp, _mm_and_ps(mask, _mm_set1_ps(1.0f)));
    }

#endif

    static inline simd4f simd4f_fract(__simd4f s)
    {
        return simd4f_sub(s, simd4f_floor(s));
    }

    // -----------------------------------------------------------------
    // float <-> half conversions
    // -----------------------------------------------------------------

#ifdef MANGO_ENABLE_F16C

    static inline simd4f simd4f_convert(__simd4h h)
    {
        const __m128i* p = reinterpret_cast<const __m128i *>(&h);
        return _mm_cvtph_ps(_mm_loadl_epi64(p));
    }

    static inline simd4h simd4h_convert(__simd4f f)
    {
        simd4h h;
        __m128i* p = reinterpret_cast<__m128i *>(&h);
        _mm_storel_epi64(p, _mm_cvtps_ph(f, 0));
        return h;
    }

#else

    static inline simd4f simd4f_convert(__simd4h h)
    {
        const __m128i* p = reinterpret_cast<const __m128i *>(&h);
        const simd4i u = _mm_unpacklo_epi16(_mm_loadl_epi64(p), _mm_setzero_si128());

        simd4i no_sign  = simd4i_and(u, simd4i_set1(0x7fff));
        simd4i sign     = simd4i_and(u, simd4i_set1(0x8000));
        simd4i exponent = simd4i_and(u, simd4i_set1(0x7c00));
        simd4i mantissa = simd4i_and(u, simd4i_set1(0x03ff));

        // NaN or Inf
        simd4i a = simd4i_or(simd4i_set1(0x7f800000), simd4i_sll(mantissa, 13));

        // Zero or Denormal
        const simd4i magic = simd4i_set1(0x3f000000);
        simd4i b;
        b = simd4i_add(magic, mantissa);
        b = simd4i_cast(simd4f_sub(simd4f_cast(b), simd4f_cast(magic)));

        // Numeric Value
        simd4i c = simd4i_add(simd4i_set1(0x38000000), simd4i_sll(no_sign, 13));

        // Select a, b, or c based on exponent
        simd4i mask;
        simd4i result;

        mask = simd4i_compare_eq(exponent, simd4i_zero());
        result = simd4i_select(mask, b, c);

        mask = simd4i_compare_eq(exponent, simd4i_set1(0x7c00));
        result = simd4i_select(mask, a, result);

        // Sign
        result = simd4i_or(result, simd4i_sll(sign, 16));

        return simd4f_cast(result);
    }

    static inline simd4h simd4h_convert(__simd4f f)
    {
        const simd4f magic = simd4f_set1(Float(0, 15, 0).f);
        const simd4i vinf = simd4i_set1(31 << 23);

        const simd4i u = simd4i_cast(f);
        const simd4i sign = simd4i_srl(simd4i_and(u, simd4i_set1(0x80000000)), 16);

        const simd4i vexponent = simd4i_set1(0x7f800000);

        // Inf / NaN
        const simd4i s0 = simd4i_compare_eq(simd4i_and(u, vexponent), vexponent);
        simd4i mantissa = simd4i_and(u, simd4i_set1(0x007fffff));
        simd4i x0 = simd4i_compare_eq(mantissa, simd4i_zero());
        mantissa = simd4i_select(x0, simd4i_zero(), simd4i_sra(mantissa, 13));
        const simd4i v0 = simd4i_or(simd4i_set1(0x7c00), mantissa);

        simd4i v1 = simd4i_and(u, simd4i_set1(0x7ffff000));
        v1 = simd4i_cast(simd4f_mul(simd4f_cast(v1), magic));
        v1 = simd4i_add(v1, simd4i_set1(0x1000));

#if defined(MANGO_ENABLE_SSE4_1)
        v1 = _mm_min_epi32(v1, vinf);
        v1 = simd4i_sra(v1, 13);

        simd4i v = simd4i_select(s0, v0, v1);
        v = simd4i_or(v, sign);
        v = _mm_packus_epi32(v, v);
#else
        v1 = simd4i_select(simd4i_compare_gt(v1, vinf), vinf, v1);
        v1 = simd4i_sra(v1, 13);

        simd4i v = simd4i_select(s0, v0, v1);
        v = simd4i_or(v, sign);
        v = _mm_slli_epi32 (v, 16);
        v = _mm_srai_epi32 (v, 16);
        v = _mm_packs_epi32 (v, v);
#endif

        simd4h h;
        _mm_storel_epi64(reinterpret_cast<__m128i *>(&h), v);
        return h;
    }

#endif

    // -----------------------------------------------------------------
    // simd4f_matrix
    // -----------------------------------------------------------------

#define SIMD_SHUFFLE(a, b, x, y, z, w) \
    _mm_shuffle_ps(a, b, _MM_SHUFFLE(w, z, y, x))

    static inline void simd4f_matrix_set_scale(simd4f* m, float s)
    {
        const simd4f v = simd4f_set4(0.0f, 1.0f, s, 0.0f);
        m[0] = SIMD_SHUFFLE(v, v, 2, 0, 0, 0);
        m[1] = SIMD_SHUFFLE(v, v, 0, 2, 0, 0);
        m[2] = SIMD_SHUFFLE(v, v, 0, 0, 2, 0);
        m[3] = SIMD_SHUFFLE(v, v, 0, 0, 0, 1);
    }

    static inline void simd4f_matrix_set_scale(simd4f* m, float x, float y, float z)
    {
        const simd4f v = simd4f_set4(x, y, z, 0.0f);
        m[0] = SIMD_SHUFFLE(v, v, 0, 3, 3, 3);
        m[1] = SIMD_SHUFFLE(v, v, 3, 1, 3, 3);
        m[2] = SIMD_SHUFFLE(v, v, 3, 3, 2, 3);
        m[3] = simd4f_set4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    static inline void simd4f_matrix_set_translate(simd4f* m, float x, float y, float z)
    {
        const simd4f v = simd4f_set4(0.0f, 1.0f, 0.0f, 0.0f);
        m[0] = SIMD_SHUFFLE(v, v, 1, 0, 0, 0);
        m[1] = SIMD_SHUFFLE(v, v, 0, 1, 0, 0);
        m[2] = SIMD_SHUFFLE(v, v, 0, 0, 1, 0);
        m[3] = simd4f_set4(x, y, z, 1.0f);
    }

    static inline void simd4f_matrix_scale(simd4f* m, float s)
    {
        const simd4f v = simd4f_set4(s, s, s, 1.0f);
        m[0] = simd4f_mul(m[0], v);
        m[1] = simd4f_mul(m[1], v);
        m[2] = simd4f_mul(m[2], v);
        m[3] = simd4f_mul(m[3], v);
    }

    static inline void simd4f_matrix_scale(simd4f* m, float x, float y, float z)
    {
        const simd4f v = simd4f_set4(x, y, z, 1.0f);
        m[0] = simd4f_mul(m[0], v);
        m[1] = simd4f_mul(m[1], v);
        m[2] = simd4f_mul(m[2], v);
        m[3] = simd4f_mul(m[3], v);
    }

    static inline void simd4f_matrix_translate(simd4f* m, float x, float y, float z)
    {
        const simd4f v = simd4f_set4(x, y, z, 0.0f);
        m[0] = simd4f_madd(m[0], simd4f_shuffle<3, 3, 3, 3>(m[0]), v);
        m[1] = simd4f_madd(m[1], simd4f_shuffle<3, 3, 3, 3>(m[1]), v);
        m[2] = simd4f_madd(m[2], simd4f_shuffle<3, 3, 3, 3>(m[2]), v);
        m[3] = simd4f_madd(m[3], simd4f_shuffle<3, 3, 3, 3>(m[3]), v);
    }

    static inline void simd4f_matrix_transpose(simd4f* result, const simd4f* m)
    {
#if 0
        const __m128 temp0 = _mm_unpacklo_ps(m[0], m[1]);
        const __m128 temp1 = _mm_unpacklo_ps(m[2], m[3]);
        const __m128 temp2 = _mm_unpackhi_ps(m[0], m[1]);
        const __m128 temp3 = _mm_unpackhi_ps(m[2], m[3]);
        result[0] = _mm_movelh_ps(temp0, temp1);
        result[1] = _mm_movehl_ps(temp1, temp0);
        result[2] = _mm_movelh_ps(temp2, temp3);
        result[3] = _mm_movehl_ps(temp3, temp2);
#else
        simd4f temp0 = SIMD_SHUFFLE(m[0], m[1], 0, 1, 0, 1); // x0, y0, x1, y1
        simd4f temp1 = SIMD_SHUFFLE(m[2], m[3], 0, 1, 0, 1); // x2, y2, x3, y3
        simd4f temp2 = SIMD_SHUFFLE(m[0], m[1], 2, 3, 2, 3); // z0, w0, z1, w1
        simd4f temp3 = SIMD_SHUFFLE(m[2], m[3], 2, 3, 2, 3); // z2, w2, z3, w3
        result[0] = SIMD_SHUFFLE(temp0, temp1, 0, 2, 0, 2);  // x0, x1, x2, x3
        result[1] = SIMD_SHUFFLE(temp0, temp1, 1, 3, 1, 3);  // y0, y1, y2, y3
        result[2] = SIMD_SHUFFLE(temp2, temp3, 0, 2, 0, 2);  // z0, z1, z2, z3
        result[3] = SIMD_SHUFFLE(temp2, temp3, 1, 3, 1, 3);  // w0, w1, w2, w3
#endif
    }

#if 0 //defined(MANGO_ENABLE_AVX)

    static inline void simd4f_matrix_inverse_transpose(simd4f* result, const simd4f* m)
    {
        const float* src = reinterpret_cast<const float*>(m);
        __m256 m01 = _mm256_loadu_ps(src + 0);
        __m256 m23 = _mm256_loadu_ps(src + 8);

        const __m256 m01y = _mm256_shuffle_ps(m01, m01, _MM_SHUFFLE(0, 0, 0, 1));
        const __m256 m23y = _mm256_shuffle_ps(m23, m23, _MM_SHUFFLE(0, 0, 0, 1));
        const __m256 m01z = _mm256_shuffle_ps(m01, m01, _MM_SHUFFLE(2, 1, 3, 2));
        const __m256 m23z = _mm256_shuffle_ps(m23, m23, _MM_SHUFFLE(2, 1, 3, 2));
        const __m256 m01w = _mm256_shuffle_ps(m01, m01, _MM_SHUFFLE(1, 3, 2, 3));
        const __m256 m23w = _mm256_shuffle_ps(m23, m23, _MM_SHUFFLE(1, 3, 2, 3));

        const __m256 m0zw = _mm256_permute2f128_ps(m01z, m01w, 0x20);
        const __m256 m1wz = _mm256_permute2f128_ps(m01z, m01w, 0x13);
        const __m256 m2zw = _mm256_permute2f128_ps(m23z, m23w, 0x20);
        const __m256 m3wz = _mm256_permute2f128_ps(m23z, m23w, 0x13);
        const __m256 m0wz = _mm256_permute2f128_ps(m01z, m01w, 0x02);
        const __m256 m1zw = _mm256_permute2f128_ps(m01z, m01w, 0x31);
        const __m256 m2wz = _mm256_permute2f128_ps(m23z, m23w, 0x02);
        const __m256 m3zw = _mm256_permute2f128_ps(m23z, m23w, 0x31);

        const __m256 p0 = _mm256_permute2f128_ps(m01z, m01w, 0x03);
        const __m256 p1 = _mm256_permute2f128_ps(m01z, m01w, 0x21);
        const __m256 p2 = _mm256_permute2f128_ps(m23z, m23w, 0x03);
        const __m256 p3 = _mm256_permute2f128_ps(m23z, m23w, 0x21);

        const __m256 s0 = _mm256_mul_ps(m2zw, m3wz);
        const __m256 s1 = _mm256_mul_ps(m2wz, m3zw);
        const __m256 s2 = _mm256_mul_ps(m0zw, m1wz);
        const __m256 s3 = _mm256_mul_ps(m0wz, m1zw);
        const __m256 s4 = _mm256_mul_ps(m2zw, p0);
        const __m256 s5 = _mm256_mul_ps(m2wz, p1);
        const __m256 s6 = _mm256_mul_ps(m3wz, p1);
        const __m256 s7 = _mm256_mul_ps(m3zw, p0);
        const __m256 s8 = _mm256_mul_ps(m1zw, p2);
        const __m256 s9 = _mm256_mul_ps(m1wz, p3);
        const __m256 sa = _mm256_mul_ps(m0wz, p3);
        const __m256 sb = _mm256_mul_ps(m0zw, p2);

        const __m256 v56 = _mm256_sub_ps(s0, s1);
        const __m256 v31 = _mm256_sub_ps(s4, s5);
        const __m256 v42 = _mm256_sub_ps(s6, s7);
        const __m256 v43 = _mm256_sub_ps(s8, s9);
        const __m256 v21 = _mm256_sub_ps(sa, sb);
        const __m256 v70 = _mm256_sub_ps(s2, s3);

        const __m256 m10 = _mm256_permute2f128_ps(m01y, m01y, 0x01);
        const __m256 m22 = _mm256_permute2f128_ps(m23y, m23y, 0x00);
        const __m256 m33 = _mm256_permute2f128_ps(m23y, m23y, 0x11);
        const __m256 m00 = _mm256_permute2f128_ps(m01y, m01y, 0x00);
        const __m256 m11 = _mm256_permute2f128_ps(m01y, m01y, 0x11);
        const __m256 m32 = _mm256_permute2f128_ps(m23y, m23y, 0x01);

        __m256 ab = _mm256_mul_ps(m10, v56);
        ab = _mm256_sub_ps(ab, _mm256_mul_ps(m22, v42));
        ab = _mm256_sub_ps(ab, _mm256_mul_ps(m33, v31));

        __m256 cd = _mm256_mul_ps(m00, v43);
        cd = _mm256_add_ps(cd, _mm256_mul_ps(m11, v21));
        cd = _mm256_add_ps(cd, _mm256_mul_ps(m32, v70));

        __m128 a = _mm256_extractf128_ps(ab, 0);
        simd4f det4 = simd4f_div(simd4f_set1(1.0f), simd4f_dot4(m[0], a));
        __m256 det8 = _mm256_permute2f128_ps(_mm256_castps128_ps256(det4), _mm256_castps128_ps256(det4), 0x20);

        ab = _mm256_mul_ps(ab, det8);
        cd = _mm256_mul_ps(cd, det8);

        float* dest = reinterpret_cast<float*>(result);
        _mm256_storeu_ps(dest + 0, ab);
        _mm256_storeu_ps(dest + 8, cd);
    }

#else

    static inline void simd4f_matrix_inverse_transpose(simd4f* result, const simd4f* m)
    {
		const simd4f m0zwyz = simd4f_shuffle<2, 3, 1, 2>(m[0]);
		const simd4f m0wzwy = simd4f_shuffle<3, 2, 3, 1>(m[0]);
		const simd4f m1zwyz = simd4f_shuffle<2, 3, 1, 2>(m[1]);
		const simd4f m1wzwy = simd4f_shuffle<3, 2, 3, 1>(m[1]);
		const simd4f m2zwyz = simd4f_shuffle<2, 3, 1, 2>(m[2]);
		const simd4f m2wzwy = simd4f_shuffle<3, 2, 3, 1>(m[2]);
		const simd4f m3zwyz = simd4f_shuffle<2, 3, 1, 2>(m[3]);
		const simd4f m3wzwy = simd4f_shuffle<3, 2, 3, 1>(m[3]);

        const simd4f v0 = simd4f_sub(simd4f_mul(m0wzwy, m1zwyz), simd4f_mul(m0zwyz, m1wzwy));
        const simd4f v1 = simd4f_sub(simd4f_mul(m0zwyz, m2wzwy), simd4f_mul(m0wzwy, m2zwyz));
        const simd4f v2 = simd4f_sub(simd4f_mul(m0wzwy, m3zwyz), simd4f_mul(m0zwyz, m3wzwy));
        const simd4f v3 = simd4f_sub(simd4f_mul(m1wzwy, m2zwyz), simd4f_mul(m1zwyz, m2wzwy));
        const simd4f v4 = simd4f_sub(simd4f_mul(m1zwyz, m3wzwy), simd4f_mul(m1wzwy, m3zwyz));
        const simd4f v5 = simd4f_sub(simd4f_mul(m2zwyz, m3wzwy), simd4f_mul(m2wzwy, m3zwyz));
        const simd4f v6 = simd4f_sub(simd4f_mul(m2wzwy, m3zwyz), simd4f_mul(m2zwyz, m3wzwy));

		const simd4f m0yxxx = simd4f_shuffle<1, 0, 0, 0>(m[0]);
		const simd4f m1yxxx = simd4f_shuffle<1, 0, 0, 0>(m[1]);
		const simd4f m2yxxx = simd4f_shuffle<1, 0, 0, 0>(m[2]);
		const simd4f m3yxxx = simd4f_shuffle<1, 0, 0, 0>(m[3]);

        simd4f a = simd4f_mul(m1yxxx, v5);
		simd4f b = simd4f_mul(m0yxxx, v6);
		simd4f c = simd4f_mul(m0yxxx, v4);
		simd4f d = simd4f_mul(m0yxxx, v3);
		a = simd4f_msub(a, m2yxxx, v4);
        a = simd4f_msub(a, m3yxxx, v3);
        b = simd4f_msub(b, m2yxxx, v2);
		c = simd4f_madd(c, m1yxxx, v2);
		b = simd4f_msub(b, m3yxxx, v1);
		d = simd4f_madd(d, m1yxxx, v1);
		c = simd4f_msub(c, m3yxxx, v0);
        d = simd4f_madd(d, m2yxxx, v0);

        simd4f det = simd4f_div(simd4f_set1(1.0f), simd4f_dot4(m[0], a));

        result[0] = simd4f_mul(a, det);
        result[1] = simd4f_mul(b, det);
        result[2] = simd4f_mul(c, det);
        result[3] = simd4f_mul(d, det);
    }

#endif

    static inline void simd4f_matrix_inverse(simd4f* result, const simd4f* m)
    {
        simd4f temp[4];
        simd4f_matrix_inverse_transpose(temp, m);
        simd4f_matrix_transpose(result, temp);
    }

    static inline simd4f simd4f_vector_matrix_multiply(__simd4f v, const simd4f* m)
    {
        simd4f temp;
        temp = simd4f_mul(_mm_shuffle_ps(v, v, 0x00), m[0]);
        temp = simd4f_madd(temp, _mm_shuffle_ps(v, v, 0x55), m[1]);
        temp = simd4f_madd(temp, _mm_shuffle_ps(v, v, 0xaa), m[2]);
        temp = simd4f_madd(temp, _mm_shuffle_ps(v, v, 0xff), m[3]);
        return temp;
    }

    static inline void simd4f_matrix_matrix_multiply(simd4f* result, const simd4f* a, const simd4f* b)
    {
        result[0] = simd4f_vector_matrix_multiply(a[0], b);
        result[1] = simd4f_vector_matrix_multiply(a[1], b);
        result[2] = simd4f_vector_matrix_multiply(a[2], b);
        result[3] = simd4f_vector_matrix_multiply(a[3], b);
    }

#undef SIMD_SHUFFLE

} // namespace mango
