/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    static inline void float32x4_matrix_set_scale(float32x4* m, float s)
    {
        const float32x4 zero = float32x4_zero();
        const float one = 1.0f;
        m[0] = vsetq_lane_f32(s, zero, 0);
        m[1] = vsetq_lane_f32(s, zero, 1);
        m[2] = vsetq_lane_f32(s, zero, 2);
        m[3] = vsetq_lane_f32(one, zero, 3);
    }

    static inline void float32x4_matrix_set_scale(float32x4* m, float x, float y, float z)
    {
        const float32x4 zero = float32x4_zero();
        const float one = 1.0f;
        m[0] = vsetq_lane_f32(x, zero, 0);
        m[1] = vsetq_lane_f32(y, zero, 1);
        m[2] = vsetq_lane_f32(z, zero, 2);
        m[3] = vsetq_lane_f32(one, zero, 3);
    }

    static inline void float32x4_matrix_set_translate(float32x4* m, float x, float y, float z)
    {
        const float32x4 zero = float32x4_zero();
        const float one = 1.0f;
        m[0] = vsetq_lane_f32(one, zero, 0);
        m[1] = vsetq_lane_f32(one, zero, 1);
        m[2] = vsetq_lane_f32(one, zero, 2);
        m[3] = float32x4_set4(x, y, z, one);
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
#if 1
        const float32_t* src = reinterpret_cast<const float32_t*>(m);
        float32_t* dest = reinterpret_cast<float32_t*>(result);
        float32x4x4_t temp = vld4q_f32(src);
        vst1q_f32(dest + 0, temp.val[0]);
        vst1q_f32(dest + 4, temp.val[1]);
        vst1q_f32(dest + 8, temp.val[2]);
        vst1q_f32(dest + 12, temp.val[3]);
#else
        asm volatile (
            "vldmia     %1, { q0-q3 }   \n\t"
            "vtrn.32    q0, q1          \n\t"
            "vtrn.32    q2, q3          \n\t"
            "vswp       d1, d4          \n\t"
            "vswp       d3, d6          \n\t"
            "vstmia     %0, { q0-q3 }   \n\t"
            :
            : "r" (result), "r" (m)
            : "q0", "q1", "q2","q3", "memory"
        );
#endif
    }

    static inline void float32x4_matrix_inverse(float32x4* result, const float32x4* m)
    {
        float32x2_t low0 = vget_low_f32(m[0]);
        float32x2_t low1 = vget_low_f32(m[1]);
        float32x2_t low2 = vget_low_f32(m[2]);
        float32x2_t low3 = vget_low_f32(m[3]);
        float32x2_t high0 = vget_high_f32(m[0]);
        float32x2_t high1 = vget_high_f32(m[1]);
        float32x2_t high2 = vget_high_f32(m[2]);
        float32x2_t high3 = vget_high_f32(m[3]);

        float32x2x2_t n0 = vzip_f32(low0, low1);   // x0, x1, y0, y1
        float32x2x2_t n1 = vzip_f32(high0, high1); // z0, z1, w0, w1
        float32x2x2_t n2 = vzip_f32(low2, low3);   // x2, x3, y2, y3
        float32x2x2_t n3 = vzip_f32(high2, high3); // z2, z3, w2, w3

        float32x4 row0 = vcombine_f32(n0.val[0], n2.val[0]); // x0, x1, x2, x3
        float32x4 row1 = vcombine_f32(n2.val[1], n0.val[1]); // y2, y3, y0, y1
        float32x4 row2 = vcombine_f32(n1.val[0], n3.val[0]); // z0, z1, z2, z3
       	float32x4 row3 = vcombine_f32(n3.val[1], n1.val[1]); // w2, w3, w0, w1

        float32x4 temp;
        float32x4 res0;
        float32x4 res1;
        float32x4 res2;
        float32x4 res3;

        temp = vmulq_f32(row2, row3);
        temp = shuffle<1, 0, 3, 2>(temp);
        res0 = vmulq_f32(row1, temp);
        res1 = vmulq_f32(row0, temp);
        temp = shuffle<2, 3, 0, 1>(temp);
        res0 = vsubq_f32(vmulq_f32(row1, temp), res0);
        res1 = vsubq_f32(vmulq_f32(row0, temp), res1);
        res1 = shuffle<2, 3, 0, 1>(res1);
        temp = vmulq_f32(row1, row2);
        temp = shuffle<1, 0, 3, 2>(temp);
        res0 = vmlaq_f32(res0, row3, temp);
        res3 = vmulq_f32(row0, temp);
        temp = shuffle<2, 3, 0, 1>(temp);
        res0 = vmlsq_f32(res0, row3, temp);
        res3 = vsubq_f32(vmulq_f32(row0, temp), res3);
        res3 = shuffle<2, 3, 0, 1>(res3);
        temp = vmulq_f32(shuffle<2, 3, 0, 1>(row1), row3);
        temp = shuffle<1, 0, 3, 2>(temp);
        row2 = shuffle<2, 3, 0, 1>(row2);
        res0 = vmlaq_f32(res0, row2, temp);
        res2 = vmulq_f32(row0, temp);
        temp = shuffle<2, 3, 0, 1>(temp);
        res0 = vmlsq_f32(res0, row2, temp);
        res2 = vsubq_f32(vmulq_f32(row0, temp), res2);
        res2 = shuffle<2, 3, 0, 1>(res2);
        temp = vmulq_f32(row0, row1);
        temp = shuffle<1, 0, 3, 2>(temp);
        res2 = vmlaq_f32(res2, row3, temp);
        res3 = vsubq_f32(vmulq_f32(row2, temp), res3);
        temp = shuffle<2, 3, 0, 1>(temp);
        res2 = vsubq_f32(vmulq_f32(row3, temp), res2);
        res3 = vmlsq_f32(res3, row2, temp);
        temp = vmulq_f32(row0, row3);
        temp = shuffle<1, 0, 3,2>(temp);
        res1 = vmlsq_f32(res1, row2, temp);
        res2 = vmlaq_f32(res2, row1, temp);
        temp = shuffle<2, 3, 0, 1>(temp);
        res1 = vmlaq_f32(res1, row2, temp);
        res2 = vmlsq_f32(res2, row1, temp);
        temp = vmulq_f32(row0, row2);
        temp = shuffle<1, 0, 3, 2>(temp);
        res1 = vmlaq_f32(res1, row3, temp);
        res3 = vmlsq_f32(res3, row1, temp);
        temp = shuffle<2, 3, 0, 1>(temp);
        res1 = vmlsq_f32(res1, row3, temp);
        res3 = vmlaq_f32(res3, row1, temp);

        float32x4 det = vmulq_f32(row0, res0);
        det = vaddq_f32(det, shuffle<1, 0, 3, 2>(det));
        det = vaddq_f32(det, float32x4_set1(vgetq_lane_f32(det, 2)));
        temp = vdupq_n_f32(1.0f / vgetq_lane_f32(det, 0));

        result[0] = vmulq_f32(res0, temp);
        result[1] = vmulq_f32(res1, temp);
        result[2] = vmulq_f32(res2, temp);
        result[3] = vmulq_f32(res3, temp);
    }

    static inline void float32x4_matrix_inverse_transpose(float32x4* result, const float32x4* m)
    {
        float32x4 temp[4];
        float32x4_matrix_inverse(temp, m);
        float32x4_matrix_transpose(result, temp);
    }

    static inline float32x4 float32x4_vector_matrix_multiply(float32x4 v, const float32x4* m)
    {
#if 1
        const float32x4 x = vdupq_n_f32(vgetq_lane_f32(v, 0));
        const float32x4 y = vdupq_n_f32(vgetq_lane_f32(v, 1));
        const float32x4 z = vdupq_n_f32(vgetq_lane_f32(v, 2));
        const float32x4 w = vdupq_n_f32(vgetq_lane_f32(v, 3));
        float32x4 temp = vmulq_f32(x, m[0]);
        temp = vmlaq_f32(temp, y, m[1]);
        temp = vmlaq_f32(temp, z, m[2]);
        temp = vmlaq_f32(temp, w, m[3]);
        return temp;
#else
        float32x4 temp;
        asm volatile (
            "vldmia    %2, { q8-q11 }    \n\t"
            "vmul.f32  %q0, q8, %e1[0]   \n\t"
            "vmla.f32  %q0, q9, %e1[1]   \n\t"
            "vmla.f32  %q0, q10, %f1[0]  \n\t"
            "vmla.f32  %q0, q11, %f1[1]  \n\t"
            : "=w" (temp)
            : "x" (v), "r" (m)
            : "memory", "q8", "q9", "q10", "q11"
        );
        return temp;
#endif
    }

    static inline void float32x4_matrix_matrix_multiply(float32x4* result, const float32x4* a, const float32x4* b)
    {
#if 1
        result[0] = float32x4_vector_matrix_multiply(a[0], b);
        result[1] = float32x4_vector_matrix_multiply(a[1], b);
        result[2] = float32x4_vector_matrix_multiply(a[2], b);
        result[3] = float32x4_vector_matrix_multiply(a[3], b);
#else
        asm volatile (
            "vldmia    %1, { q0-q3 }    \n\t"
            "vldmia    %2, { q8-q11 }   \n\t"
            "vmul.f32  q12, q8, d0[0]   \n\t"
            "vmul.f32  q13, q8, d2[0]   \n\t"
            "vmul.f32  q14, q8, d4[0]   \n\t"
            "vmul.f32  q15, q8, d6[0]   \n\t"
            "vmla.f32  q12, q9, d0[1]   \n\t"
            "vmla.f32  q13, q9, d2[1]   \n\t"
            "vmla.f32  q14, q9, d4[1]   \n\t"
            "vmla.f32  q15, q9, d6[1]   \n\t"
            "vmla.f32  q12, q10, d1[0]  \n\t"
            "vmla.f32  q13, q10, d3[0]  \n\t"
            "vmla.f32  q14, q10, d5[0]  \n\t"
            "vmla.f32  q15, q10, d7[0]  \n\t"
            "vmla.f32  q12, q11, d1[1]  \n\t"
            "vmla.f32  q13, q11, d3[1]  \n\t"
            "vmla.f32  q14, q11, d5[1]  \n\t"
            "vmla.f32  q15, q11, d7[1]  \n\t"
            "vstmia    %0, { q12-q15 }  \n\t"
            :
            : "r" (result), "r" (a), "r" (b)
            : "memory", "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
        );
#endif
    }

} // namespace simd
} // namespace mango
