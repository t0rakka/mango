/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

#ifdef MANGO_SIMD_FLOAT_SSE

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline float32x4 float32x4_shuffle(float32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return _mm_shuffle_ps(v, v, _MM_SHUFFLE(w, z, y, x));
    }

    template <>
    inline float32x4 float32x4_shuffle<0, 1, 2, 3>(float32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

#if defined(MANGO_ENABLE_SSE4_1)

    template <int Index>
    static inline float32x4 float32x4_set_component(float32x4 a, float s)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        return _mm_insert_ps(a, _mm_set_ss(s), Index * 0x10);
    }

#else

    template <int Index>
    static inline float32x4 float32x4_set_component(float32x4 a, float s);

    template <>
    inline float32x4 float32x4_set_component<0>(float32x4 a, float x)
    {
        const __m128 b = _mm_unpacklo_ps(_mm_set_ps1(x), a);
        return _mm_shuffle_ps(b, a, _MM_SHUFFLE(3, 2, 3, 0));
    }

    template <>
    inline float32x4 float32x4_set_component<1>(float32x4 a, float y)
    {
        const __m128 b = _mm_unpacklo_ps(_mm_set_ps1(y), a);
        return _mm_shuffle_ps(b, a, _MM_SHUFFLE(3, 2, 0, 1));
    }

    template <>
    inline float32x4 float32x4_set_component<2>(float32x4 a, float z)
    {
        const __m128 b = _mm_unpackhi_ps(_mm_set_ps1(z), a);
        return _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 0, 1, 0));
    }

    template <>
    inline float32x4 float32x4_set_component<3>(float32x4 a, float w)
    {
        const __m128 b = _mm_unpackhi_ps(_mm_set_ps1(w), a);
        return _mm_shuffle_ps(a, b, _MM_SHUFFLE(0, 1, 1, 0));
    }

#endif

    template <int Index>
    static inline float float32x4_get_component(float32x4 a);

    template <>
    inline float float32x4_get_component<0>(float32x4 a)
    {
        return _mm_cvtss_f32(a);
    }

    template <>
    inline float float32x4_get_component<1>(float32x4 a)
    {
        return _mm_cvtss_f32(float32x4_shuffle<1, 1, 1, 1>(a));
    }

    template <>
    inline float float32x4_get_component<2>(float32x4 a)
    {
        return _mm_cvtss_f32(float32x4_shuffle<2, 2, 2, 2>(a));
    }

    template <>
    inline float float32x4_get_component<3>(float32x4 a)
    {
        return _mm_cvtss_f32(float32x4_shuffle<3, 3, 3, 3>(a));
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

    static inline float32x4 float32x4_movelh(float32x4 a, float32x4 b)
    {
        return _mm_movelh_ps(a, b);
    }

    static inline float32x4 float32x4_movehl(float32x4 a, float32x4 b)
    {
        return _mm_movehl_ps(a, b);
    }

    static inline float32x4 float32x4_unpackhi(float32x4 a, float32x4 b)
    {
        return _mm_unpackhi_ps(a, b);
    }

    static inline float32x4 float32x4_unpacklo(float32x4 a, float32x4 b)
    {
        return _mm_unpacklo_ps(a, b);
    }

    // logical

    static inline float32x4 float32x4_and(float32x4 a, float32x4 b)
    {
        return _mm_and_ps(a, b);
    }

    static inline float32x4 float32x4_nand(float32x4 a, float32x4 b)
    {
        return _mm_andnot_ps(a, b);
    }

    static inline float32x4 float32x4_or(float32x4 a, float32x4 b)
    {
        return _mm_or_ps(a, b);
    }

    static inline float32x4 float32x4_xor(float32x4 a, float32x4 b)
    {
        return _mm_xor_ps(a, b);
    }

    static inline float32x4 float32x4_min(float32x4 a, float32x4 b)
    {
        return _mm_min_ps(a, b);
    }

    static inline float32x4 float32x4_max(float32x4 a, float32x4 b)
    {
        return _mm_max_ps(a, b);
    }

    static inline float32x4 float32x4_hmin(float32x4 a)
    {
        __m128 temp = _mm_min_ps(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1)));
        return _mm_min_ps(temp, _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(1, 0, 3, 2)));
    }

    static inline float32x4 float32x4_hmax(float32x4 a)
    {
        __m128 temp = _mm_max_ps(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1)));
        return _mm_max_ps(temp, _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(1, 0, 3, 2)));
    }

    static inline float32x4 float32x4_abs(float32x4 a)
    {
        return _mm_and_ps(a, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
    }

    static inline float32x4 float32x4_neg(float32x4 a)
    {
        return _mm_xor_ps(a, _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));
    }

    static inline float32x4 float32x4_add(float32x4 a, float32x4 b)
    {
        return _mm_add_ps(a, b);
    }

    static inline float32x4 float32x4_sub(float32x4 a, float32x4 b)
    {
        return _mm_sub_ps(a, b);
    }

    static inline float32x4 float32x4_mul(float32x4 a, float32x4 b)
    {
        return _mm_mul_ps(a, b);
    }

    static inline float32x4 float32x4_div(float32x4 a, float32x4 b)
    {
        return _mm_div_ps(a, b);
    }

    static inline float32x4 float32x4_div(float32x4 a, float b)
    {
        return _mm_div_ps(a, _mm_set1_ps(b));
    }

#if defined(MANGO_ENABLE_FMA3)

    static inline float32x4 float32x4_madd(float32x4 a, float32x4 b, float32x4 c)
    {
        return _mm_fmadd_ps(b, c, a);
    }

    static inline float32x4 float32x4_msub(float32x4 a, float32x4 b, float32x4 c)
    {
        return _mm_fnmadd_ps(b, c, a);
    }

#elif defined(MANGO_ENABLE_FMA4)

    static inline float32x4 float32x4_madd(float32x4 a, float32x4 b, float32x4 c)
    {
        return _mm_macc_ps(b, c, a);
    }

    static inline float32x4 float32x4_msub(float32x4 a, float32x4 b, float32x4 c)
    {
        return _mm_sub_ps(a, _mm_mul_ps(b, c));
    }

#else

    static inline float32x4 float32x4_madd(float32x4 a, float32x4 b, float32x4 c)
    {
        return _mm_add_ps(a, _mm_mul_ps(b, c));
    }

    static inline float32x4 float32x4_msub(float32x4 a, float32x4 b, float32x4 c)
    {
        return _mm_sub_ps(a, _mm_mul_ps(b, c));
    }

#endif

    static inline float32x4 float32x4_fast_reciprocal(float32x4 a)
    {
        return _mm_rcp_ps(a);
    }

    static inline float32x4 float32x4_fast_rsqrt(float32x4 a)
    {
        return _mm_rsqrt_ps(a);
    }

    static inline float32x4 float32x4_fast_sqrt(float32x4 a)
    {
        float32x4 n = _mm_rsqrt_ps(a);
        return _mm_mul_ps(a, n);
    }

    static inline float32x4 float32x4_reciprocal(float32x4 a)
    {
        float32x4 n = _mm_rcp_ps(a);
        float32x4 m = _mm_mul_ps(_mm_mul_ps(n, n), a);
        return _mm_sub_ps(_mm_add_ps(n, n), m);
    }

    static inline float32x4 float32x4_rsqrt(float32x4 a)
    {
        float32x4 n = _mm_rsqrt_ps(a);
        float32x4 e = _mm_mul_ps(_mm_mul_ps(n, n), a);
        n = _mm_mul_ps(_mm_set_ps1(0.5f), n);
        e = _mm_sub_ps(_mm_set_ps1(3.0f), e);
        return _mm_mul_ps(n, e);
    }

    static inline float32x4 float32x4_sqrt(float32x4 a)
    {
        return _mm_sqrt_ps(a);
    }

    static inline float32x4 float32x4_dot3(float32x4 a, float32x4 b)
    {
#if defined(MANGO_ENABLE_SSE4_1)
        return _mm_dp_ps(a, b, 0x7f);
#else
        float32x4 s = _mm_mul_ps(a, b);
        return _mm_add_ps(float32x4_shuffle<0, 0, 0, 0>(s),
               _mm_add_ps(float32x4_shuffle<1, 1, 1, 1>(s), float32x4_shuffle<2, 2, 2, 2>(s)));
#endif
    }

    static inline float32x4 float32x4_dot4(float32x4 a, float32x4 b)
    {
#if defined(MANGO_ENABLE_SSE4_1)
        return _mm_dp_ps(a, b, 0xff);
#elif defined(MANGO_ENABLE_SSE3)
        float32x4 s = _mm_mul_ps(a, b);
        s = _mm_hadd_ps(s, s);
        s = _mm_hadd_ps(s, s);
        return s;
#else
        float32x4 s = _mm_mul_ps(a, b);
        s = _mm_add_ps(s, float32x4_shuffle<2, 3, 0, 1>(s));
        s = _mm_add_ps(s, float32x4_shuffle<1, 0, 3, 2>(s));
        return s;
#endif
    }

    static inline float32x4 float32x4_cross3(float32x4 a, float32x4 b)
    {
        float32x4 c = _mm_sub_ps(_mm_mul_ps(a, float32x4_shuffle<1, 2, 0, 3>(b)),
                                 _mm_mul_ps(b, float32x4_shuffle<1, 2, 0, 3>(a)));
        return float32x4_shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline float32x4 float32x4_compare_neq(float32x4 a, float32x4 b)
    {
        return _mm_cmpneq_ps(a, b);
    }

    static inline float32x4 float32x4_compare_eq(float32x4 a, float32x4 b)
    {
        return _mm_cmpeq_ps(a, b);
    }

    static inline float32x4 float32x4_compare_lt(float32x4 a, float32x4 b)
    {
        return _mm_cmplt_ps(a, b);
    }

    static inline float32x4 float32x4_compare_le(float32x4 a, float32x4 b)
    {
        return _mm_cmple_ps(a, b);
    }

    static inline float32x4 float32x4_compare_gt(float32x4 a, float32x4 b)
    {
        return _mm_cmpgt_ps(a, b);
    }

    static inline float32x4 float32x4_compare_ge(float32x4 a, float32x4 b)
    {
        return _mm_cmpge_ps(a, b);
    }

#if defined(MANGO_ENABLE_SSE4_1)

    static inline float32x4 float32x4_select(float32x4 mask, float32x4 a, float32x4 b)
    {
        return _mm_blendv_ps(b, a, mask);
    }

#else

    static inline float32x4 float32x4_select(float32x4 mask, float32x4 a, float32x4 b)
    {
        return float32x4_or(float32x4_and(mask, a), float32x4_nand(mask, b));
    }

#endif

    // rounding

#if defined(MANGO_ENABLE_SSE4_1)

    static inline float32x4 float32x4_round(float32x4 s)
    {
        return _mm_round_ps(s, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
    }

    static inline float32x4 float32x4_trunc(float32x4 s)
    {
        return _mm_round_ps(s, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
    }

    static inline float32x4 float32x4_floor(float32x4 s)
    {
        return _mm_round_ps(s, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    }

    static inline float32x4 float32x4_ceil(float32x4 s)
    {
        return _mm_round_ps(s, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
    }

#else

    static inline float32x4 float32x4_round(float32x4 s)
    {
        return _mm_cvtepi32_ps(_mm_cvtps_epi32(s));
    }

    static inline float32x4 float32x4_trunc(float32x4 s)
    {
        return _mm_cvtepi32_ps(_mm_cvttps_epi32(s));
    }

    static inline float32x4 float32x4_floor(float32x4 s)
    {
        const float32x4 temp = float32x4_round(s);
        const float32x4 mask = _mm_cmplt_ps(s, temp);
        return _mm_sub_ps(temp, _mm_and_ps(mask, _mm_set1_ps(1.0f)));
    }

    static inline float32x4 float32x4_ceil(float32x4 s)
    {
        const float32x4 temp = float32x4_round(s);
        const float32x4 mask = _mm_cmpgt_ps(s, temp);
        return _mm_add_ps(temp, _mm_and_ps(mask, _mm_set1_ps(1.0f)));
    }

#endif

    static inline float32x4 float32x4_fract(float32x4 s)
    {
        return float32x4_sub(s, float32x4_floor(s));
    }

    // -----------------------------------------------------------------
    // float32x4_matrix
    // -----------------------------------------------------------------

#define SIMD_SHUFFLE(a, b, x, y, z, w) \
    _mm_shuffle_ps(a, b, _MM_SHUFFLE(w, z, y, x))

    static inline void float32x4_matrix_set_scale(float32x4* m, float s)
    {
        const float32x4 v = float32x4_set4(0.0f, 1.0f, s, 0.0f);
        m[0] = SIMD_SHUFFLE(v, v, 2, 0, 0, 0);
        m[1] = SIMD_SHUFFLE(v, v, 0, 2, 0, 0);
        m[2] = SIMD_SHUFFLE(v, v, 0, 0, 2, 0);
        m[3] = SIMD_SHUFFLE(v, v, 0, 0, 0, 1);
    }

    static inline void float32x4_matrix_set_scale(float32x4* m, float x, float y, float z)
    {
        const float32x4 v = float32x4_set4(x, y, z, 0.0f);
        m[0] = SIMD_SHUFFLE(v, v, 0, 3, 3, 3);
        m[1] = SIMD_SHUFFLE(v, v, 3, 1, 3, 3);
        m[2] = SIMD_SHUFFLE(v, v, 3, 3, 2, 3);
        m[3] = float32x4_set4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    static inline void float32x4_matrix_set_translate(float32x4* m, float x, float y, float z)
    {
        const float32x4 v = float32x4_set4(0.0f, 1.0f, 0.0f, 0.0f);
        m[0] = SIMD_SHUFFLE(v, v, 1, 0, 0, 0);
        m[1] = SIMD_SHUFFLE(v, v, 0, 1, 0, 0);
        m[2] = SIMD_SHUFFLE(v, v, 0, 0, 1, 0);
        m[3] = float32x4_set4(x, y, z, 1.0f);
    }

    static inline void float32x4_matrix_scale(float32x4* m, float s)
    {
        const float32x4 v = float32x4_set4(s, s, s, 1.0f);
        m[0] = float32x4_mul(m[0], v);
        m[1] = float32x4_mul(m[1], v);
        m[2] = float32x4_mul(m[2], v);
        m[3] = float32x4_mul(m[3], v);
    }

    static inline void float32x4_matrix_scale(float32x4* m, float x, float y, float z)
    {
        const float32x4 v = float32x4_set4(x, y, z, 1.0f);
        m[0] = float32x4_mul(m[0], v);
        m[1] = float32x4_mul(m[1], v);
        m[2] = float32x4_mul(m[2], v);
        m[3] = float32x4_mul(m[3], v);
    }

    static inline void float32x4_matrix_translate(float32x4* m, float x, float y, float z)
    {
        const float32x4 v = float32x4_set4(x, y, z, 0.0f);
        m[0] = float32x4_madd(m[0], float32x4_shuffle<3, 3, 3, 3>(m[0]), v);
        m[1] = float32x4_madd(m[1], float32x4_shuffle<3, 3, 3, 3>(m[1]), v);
        m[2] = float32x4_madd(m[2], float32x4_shuffle<3, 3, 3, 3>(m[2]), v);
        m[3] = float32x4_madd(m[3], float32x4_shuffle<3, 3, 3, 3>(m[3]), v);
    }

    static inline void float32x4_matrix_transpose(float32x4* result, const float32x4* m)
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
        float32x4 temp0 = SIMD_SHUFFLE(m[0], m[1], 0, 1, 0, 1); // x0, y0, x1, y1
        float32x4 temp1 = SIMD_SHUFFLE(m[2], m[3], 0, 1, 0, 1); // x2, y2, x3, y3
        float32x4 temp2 = SIMD_SHUFFLE(m[0], m[1], 2, 3, 2, 3); // z0, w0, z1, w1
        float32x4 temp3 = SIMD_SHUFFLE(m[2], m[3], 2, 3, 2, 3); // z2, w2, z3, w3
        result[0] = SIMD_SHUFFLE(temp0, temp1, 0, 2, 0, 2);  // x0, x1, x2, x3
        result[1] = SIMD_SHUFFLE(temp0, temp1, 1, 3, 1, 3);  // y0, y1, y2, y3
        result[2] = SIMD_SHUFFLE(temp2, temp3, 0, 2, 0, 2);  // z0, z1, z2, z3
        result[3] = SIMD_SHUFFLE(temp2, temp3, 1, 3, 1, 3);  // w0, w1, w2, w3
#endif
    }

#if 0 //defined(MANGO_ENABLE_AVX)

    static inline void float32x4_matrix_inverse_transpose(float32x4* result, const float32x4* m)
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
        float32x4 det4 = float32x4_div(float32x4_set1(1.0f), float32x4_dot4(m[0], a));
        __m256 det8 = _mm256_permute2f128_ps(_mm256_castps128_ps256(det4), _mm256_castps128_ps256(det4), 0x20);

        ab = _mm256_mul_ps(ab, det8);
        cd = _mm256_mul_ps(cd, det8);

        float* dest = reinterpret_cast<float*>(result);
        _mm256_storeu_ps(dest + 0, ab);
        _mm256_storeu_ps(dest + 8, cd);
    }

#else

    static inline void float32x4_matrix_inverse_transpose(float32x4* result, const float32x4* m)
    {
        const float32x4 m0zwyz = float32x4_shuffle<2, 3, 1, 2>(m[0]);
        const float32x4 m0wzwy = float32x4_shuffle<3, 2, 3, 1>(m[0]);
        const float32x4 m1zwyz = float32x4_shuffle<2, 3, 1, 2>(m[1]);
        const float32x4 m1wzwy = float32x4_shuffle<3, 2, 3, 1>(m[1]);
        const float32x4 m2zwyz = float32x4_shuffle<2, 3, 1, 2>(m[2]);
        const float32x4 m2wzwy = float32x4_shuffle<3, 2, 3, 1>(m[2]);
        const float32x4 m3zwyz = float32x4_shuffle<2, 3, 1, 2>(m[3]);
        const float32x4 m3wzwy = float32x4_shuffle<3, 2, 3, 1>(m[3]);

        const float32x4 v0 = float32x4_sub(float32x4_mul(m0wzwy, m1zwyz), float32x4_mul(m0zwyz, m1wzwy));
        const float32x4 v1 = float32x4_sub(float32x4_mul(m0zwyz, m2wzwy), float32x4_mul(m0wzwy, m2zwyz));
        const float32x4 v2 = float32x4_sub(float32x4_mul(m0wzwy, m3zwyz), float32x4_mul(m0zwyz, m3wzwy));
        const float32x4 v3 = float32x4_sub(float32x4_mul(m1wzwy, m2zwyz), float32x4_mul(m1zwyz, m2wzwy));
        const float32x4 v4 = float32x4_sub(float32x4_mul(m1zwyz, m3wzwy), float32x4_mul(m1wzwy, m3zwyz));
        const float32x4 v5 = float32x4_sub(float32x4_mul(m2zwyz, m3wzwy), float32x4_mul(m2wzwy, m3zwyz));
        const float32x4 v6 = float32x4_sub(float32x4_mul(m2wzwy, m3zwyz), float32x4_mul(m2zwyz, m3wzwy));

        const float32x4 m0yxxx = float32x4_shuffle<1, 0, 0, 0>(m[0]);
        const float32x4 m1yxxx = float32x4_shuffle<1, 0, 0, 0>(m[1]);
        const float32x4 m2yxxx = float32x4_shuffle<1, 0, 0, 0>(m[2]);
        const float32x4 m3yxxx = float32x4_shuffle<1, 0, 0, 0>(m[3]);

        float32x4 a = float32x4_mul(m1yxxx, v5);
        float32x4 b = float32x4_mul(m0yxxx, v6);
        float32x4 c = float32x4_mul(m0yxxx, v4);
        float32x4 d = float32x4_mul(m0yxxx, v3);
        a = float32x4_msub(a, m2yxxx, v4);
        a = float32x4_msub(a, m3yxxx, v3);
        b = float32x4_msub(b, m2yxxx, v2);
        c = float32x4_madd(c, m1yxxx, v2);
        b = float32x4_msub(b, m3yxxx, v1);
        d = float32x4_madd(d, m1yxxx, v1);
        c = float32x4_msub(c, m3yxxx, v0);
        d = float32x4_madd(d, m2yxxx, v0);

        float32x4 det = float32x4_div(float32x4_set1(1.0f), float32x4_dot4(m[0], a));

        result[0] = float32x4_mul(a, det);
        result[1] = float32x4_mul(b, det);
        result[2] = float32x4_mul(c, det);
        result[3] = float32x4_mul(d, det);
    }

#endif

    static inline void float32x4_matrix_inverse(float32x4* result, const float32x4* m)
    {
        float32x4 temp[4];
        float32x4_matrix_inverse_transpose(temp, m);
        float32x4_matrix_transpose(result, temp);
    }

    static inline float32x4 float32x4_vector_matrix_multiply(float32x4 v, const float32x4* m)
    {
        float32x4 temp;
        temp = float32x4_mul(_mm_shuffle_ps(v, v, 0x00), m[0]);
        temp = float32x4_madd(temp, _mm_shuffle_ps(v, v, 0x55), m[1]);
        temp = float32x4_madd(temp, _mm_shuffle_ps(v, v, 0xaa), m[2]);
        temp = float32x4_madd(temp, _mm_shuffle_ps(v, v, 0xff), m[3]);
        return temp;
    }

    static inline void float32x4_matrix_matrix_multiply(float32x4* result, const float32x4* a, const float32x4* b)
    {
        result[0] = float32x4_vector_matrix_multiply(a[0], b);
        result[1] = float32x4_vector_matrix_multiply(a[1], b);
        result[2] = float32x4_vector_matrix_multiply(a[2], b);
        result[3] = float32x4_vector_matrix_multiply(a[3], b);
    }

#undef SIMD_SHUFFLE

} // namespace simd
} // namespace mango

#endif // MANGO_SIMD_FLOAT_SSE
