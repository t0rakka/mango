/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

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
        m[0] = mul(m[0], v);
        m[1] = mul(m[1], v);
        m[2] = mul(m[2], v);
        m[3] = mul(m[3], v);
    }

    static inline void float32x4_matrix_scale(float32x4* m, float x, float y, float z)
    {
        const float32x4 v = float32x4_set4(x, y, z, 1.0f);
        m[0] = mul(m[0], v);
        m[1] = mul(m[1], v);
        m[2] = mul(m[2], v);
        m[3] = mul(m[3], v);
    }

    static inline void float32x4_matrix_translate(float32x4* m, float x, float y, float z)
    {
        const float32x4 v = float32x4_set4(x, y, z, 0.0f);
        m[0] = madd(m[0], shuffle<3, 3, 3, 3>(m[0]), v);
        m[1] = madd(m[1], shuffle<3, 3, 3, 3>(m[1]), v);
        m[2] = madd(m[2], shuffle<3, 3, 3, 3>(m[2]), v);
        m[3] = madd(m[3], shuffle<3, 3, 3, 3>(m[3]), v);
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
        float32x4 det4 = div(float32x4_set1(1.0f), dot4(m[0], a));
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
        const float32x4 m0zwyz = shuffle<2, 3, 1, 2>(m[0]);
        const float32x4 m0wzwy = shuffle<3, 2, 3, 1>(m[0]);
        const float32x4 m1zwyz = shuffle<2, 3, 1, 2>(m[1]);
        const float32x4 m1wzwy = shuffle<3, 2, 3, 1>(m[1]);
        const float32x4 m2zwyz = shuffle<2, 3, 1, 2>(m[2]);
        const float32x4 m2wzwy = shuffle<3, 2, 3, 1>(m[2]);
        const float32x4 m3zwyz = shuffle<2, 3, 1, 2>(m[3]);
        const float32x4 m3wzwy = shuffle<3, 2, 3, 1>(m[3]);

        const float32x4 v0 = msub(mul(m0wzwy, m1zwyz), m0zwyz, m1wzwy);
        const float32x4 v1 = msub(mul(m0zwyz, m2wzwy), m0wzwy, m2zwyz);
        const float32x4 v2 = msub(mul(m0wzwy, m3zwyz), m0zwyz, m3wzwy);
        const float32x4 v3 = msub(mul(m1wzwy, m2zwyz), m1zwyz, m2wzwy);
        const float32x4 v4 = msub(mul(m1zwyz, m3wzwy), m1wzwy, m3zwyz);
        const float32x4 v5 = msub(mul(m2zwyz, m3wzwy), m2wzwy, m3zwyz);
        const float32x4 v6 = msub(mul(m2wzwy, m3zwyz), m2zwyz, m3wzwy);

        const float32x4 m0yxxx = shuffle<1, 0, 0, 0>(m[0]);
        const float32x4 m1yxxx = shuffle<1, 0, 0, 0>(m[1]);
        const float32x4 m2yxxx = shuffle<1, 0, 0, 0>(m[2]);
        const float32x4 m3yxxx = shuffle<1, 0, 0, 0>(m[3]);

        float32x4 a = mul(m1yxxx, v5);
        float32x4 b = mul(m0yxxx, v6);
        float32x4 c = mul(m0yxxx, v4);
        float32x4 d = mul(m0yxxx, v3);
        a = msub(a, m2yxxx, v4);
        a = msub(a, m3yxxx, v3);
        b = msub(b, m2yxxx, v2);
        c = madd(c, m1yxxx, v2);
        b = msub(b, m3yxxx, v1);
        d = madd(d, m1yxxx, v1);
        c = msub(c, m3yxxx, v0);
        d = madd(d, m2yxxx, v0);

        float32x4 det = div(float32x4_set1(1.0f), dot4(m[0], a));

        result[0] = mul(a, det);
        result[1] = mul(b, det);
        result[2] = mul(c, det);
        result[3] = mul(d, det);
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
        temp = mul(_mm_shuffle_ps(v, v, 0x00), m[0]);
        temp = madd(temp, _mm_shuffle_ps(v, v, 0x55), m[1]);
        temp = madd(temp, _mm_shuffle_ps(v, v, 0xaa), m[2]);
        temp = madd(temp, _mm_shuffle_ps(v, v, 0xff), m[3]);
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
