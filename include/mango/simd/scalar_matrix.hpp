/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    static inline void float32x4_matrix_set_scale(float32x4* result, float s)
    {
        float* dest = reinterpret_cast<float*>(result);
        dest[0] = s;
        dest[1] = 0;
        dest[2] = 0;
        dest[3] = 0;
        dest[4] = 0;
        dest[5] = s;
        dest[6] = 0;
        dest[7] = 0;
        dest += 8;
        dest[0] = 0;
        dest[1] = 0;
        dest[2] = s;
        dest[3] = 0;
        dest[4] = 0;
        dest[5] = 0;
        dest[6] = 0;
        dest[7] = 1;
    }

    static inline void float32x4_matrix_set_scale(float32x4* result, float x, float y, float z)
    {
        float* dest = reinterpret_cast<float*>(result);
        dest[0] = x;
        dest[1] = 0;
        dest[2] = 0;
        dest[3] = 0;
        dest[4] = 0;
        dest[5] = y;
        dest[6] = 0;
        dest[7] = 0;
        dest += 8;
        dest[0] = 0;
        dest[1] = 0;
        dest[2] = z;
        dest[3] = 0;
        dest[4] = 0;
        dest[5] = 0;
        dest[6] = 0;
        dest[7] = 1;
    }

    static inline void float32x4_matrix_set_translate(float32x4* result, float x, float y, float z)
    {
        float* dest = reinterpret_cast<float*>(result);
        dest[0] = 1;
        dest[1] = 0;
        dest[2] = 0;
        dest[3] = 0;
        dest[4] = 0;
        dest[5] = 1;
        dest[6] = 0;
        dest[7] = 0;
        dest += 8;
        dest[0] = 0;
        dest[1] = 0;
        dest[2] = 1;
        dest[3] = 0;
        dest[4] = x;
        dest[5] = y;
        dest[6] = z;
        dest[7] = 1;
    }

    static inline void float32x4_matrix_scale(float32x4* result, float s)
    {
        float* dest = reinterpret_cast<float*>(result);
        for (int i = 0; i < 4; ++i)
        {
            dest[0] *= s;
            dest[1] *= s;
            dest[2] *= s;
            dest += 4;
        }
    }

    static inline void float32x4_matrix_scale(float32x4* result, float x, float y, float z)
    {
        float* dest = reinterpret_cast<float*>(result);
        for (int i = 0; i < 4; ++i)
        {
            dest[0] *= x;
            dest[1] *= y;
            dest[2] *= z;
            dest += 4;
        }
    }

    static inline void float32x4_matrix_translate(float32x4* result, float x, float y, float z)
    {
        float* dest = reinterpret_cast<float*>(result);
        for (int i = 0; i < 4; ++i)
        {
            dest[0] += dest[3] * x;
            dest[1] += dest[3] * y;
            dest[2] += dest[3] * z;
            dest += 4;
        }
    }

    static inline void float32x4_matrix_transpose(float32x4* result, const float32x4* m)
    {
        const float* src = reinterpret_cast<const float*>(m);
        float* dest = reinterpret_cast<float*>(result);
        dest[0] = src[0];
        dest[1] = src[4];
        dest[2] = src[8];
        dest[3] = src[12];
        dest[4] = src[1];
        dest[5] = src[5];
        dest[6] = src[9];
        dest[7] = src[13];
        dest[8] = src[2];
        dest[9] = src[6];
        dest[10] = src[10];
        dest[11] = src[14];
        dest[12] = src[3];
        dest[13] = src[7];
        dest[14] = src[11];
        dest[15] = src[15];
    }

    static inline void float32x4_matrix_inverse(float32x4* result, const float32x4* m)
    {
        const float* src = reinterpret_cast<const float*>(m);
        float* dest = reinterpret_cast<float*>(result);
        float m00 = src[0];
        float m01 = src[1];
        float m02 = src[2];
        float m03 = src[3];
        float m04 = src[4];
        float m05 = src[5];
        float m06 = src[6];
        float m07 = src[7];
        float m08 = src[8];
        float m09 = src[9];
        float m10 = src[10];
        float m11 = src[11];
        float m12 = src[12];
        float m13 = src[13];
        float m14 = src[14];
        float m15 = src[15];

        float i0 = m05 * (m10*m15 - m11*m14) - m09 * (m06*m15 - m07*m14) - m13 * (m07*m10 - m06*m11);
        float i1 = m04 * (m11*m14 - m10*m15) - m08 * (m07*m14 - m06*m15) - m12 * (m06*m11 - m07*m10);
        float i2 = m04 * (m09*m15 - m11*m13) - m08 * (m05*m15 - m07*m13) - m12 * (m07*m09 - m05*m11);
        float i3 = m04 * (m10*m13 - m09*m14) - m08 * (m06*m13 - m05*m14) - m12 * (m05*m10 - m06*m09);

        float det = src[0] * i0 + src[1] * i1 + src[2] * i2 + src[3] * i3;
        if (det) det = 1.0f / det;

        float a1 = m01 * (m11*m14 - m10*m15) - m09 * (m03*m14 - m02*m15) - m13 * (m02*m11 - m03*m10);
        float a2 = m01 * (m06*m15 - m07*m14) - m05 * (m02*m15 - m03*m14) - m13 * (m03*m06 - m02*m07);
        float a3 = m01 * (m07*m10 - m06*m11) - m05 * (m03*m10 - m02*m11) - m09 * (m02*m07 - m03*m06);

        float b1 = m00 * (m10*m15 - m11*m14) - m08 * (m02*m15 - m03*m14) - m12 * (m03*m10 - m02*m11);
        float b2 = m00 * (m07*m14 - m06*m15) - m04 * (m03*m14 - m02*m15) - m12 * (m02*m07 - m03*m06);
        float b3 = m00 * (m06*m11 - m07*m10) - m04 * (m02*m11 - m03*m10) - m08 * (m03*m06 - m02*m07);

        float c1 = m00 * (m11*m13 - m09*m15) - m08 * (m03*m13 - m01*m15) - m12 * (m01*m11 - m03*m09);
        float c2 = m00 * (m05*m15 - m07*m13) - m04 * (m01*m15 - m03*m13) - m12 * (m03*m05 - m01*m07);
        float c3 = m00 * (m07*m09 - m05*m11) - m04 * (m03*m09 - m01*m11) - m08 * (m01*m07 - m03*m05);

        float d1 = m00 * (m09*m14 - m10*m13) - m08 * (m01*m14 - m02*m13) - m12*(m02*m09 - m01*m10);
        float d2 = m00 * (m06*m13 - m05*m14) - m04 * (m02*m13 - m01*m14) - m12*(m01*m06 - m02*m05);
        float d3 = m00 * (m05*m10 - m06*m09) - m04 * (m01*m10 - m02*m09) - m08*(m02*m05 - m01*m06);

        dest[0] = det * i0;
        dest[1] = det * a1;
        dest[2] = det * a2;
        dest[3] = det * a3;
        dest[4] = det * i1;
        dest[5] = det * b1;
        dest[6] = det * b2;
        dest[7] = det * b3;
        dest[8] = det * i2;
        dest[9] = det * c1;
        dest[10] = det * c2;
        dest[11] = det * c3;
        dest[12] = det * i3;
        dest[13] = det * d1;
        dest[14] = det * d2;
        dest[15] = det * d3;
    }

    static inline void float32x4_matrix_inverse_transpose(float32x4* result, const float32x4* m)
    {
        const float* src = reinterpret_cast<const float*>(m);
        float* dest = reinterpret_cast<float*>(result);
        float m00 = src[0];
        float m01 = src[1];
        float m02 = src[2];
        float m03 = src[3];
        float m04 = src[4];
        float m05 = src[5];
        float m06 = src[6];
        float m07 = src[7];
        float m08 = src[8];
        float m09 = src[9];
        float m10 = src[10];
        float m11 = src[11];
        float m12 = src[12];
        float m13 = src[13];
        float m14 = src[14];
        float m15 = src[15];

        float s0 = m10*m15 - m11*m14;
        float s1 = m09*m15 - m11*m13;
        float s2 = m10*m13 - m09*m14;
        float s3 = m06*m15 - m07*m14;
        float s4 = m05*m15 - m07*m13;
        float s5 = m06*m13 - m05*m14;
        float s6 = m07*m10 - m06*m11;
        float s7 = m07*m09 - m05*m11;
        float s8 = m05*m10 - m06*m09;
        float s9 = m03*m14 - m02*m15;
        float s10 = m03*m13 - m01*m15;
        float s11 = m01*m14 - m02*m13;
        float s12 = m02*m11 - m03*m10;
        float s13 = m01*m11 - m03*m09;
        float s14 = m02*m09 - m01*m10;
        float s15 = m03*m06 - m02*m07;
        float s16 = m03*m05 - m01*m07;
        float s17 = m01*m06 - m02*m05;

        float i0 = m05 * s0 - m09 * s3 - m13 * s6;
        float i1 = m04 *-s0 + m08 * s3 + m12 * s6;
        float i2 = m04 * s1 - m08 * s4 - m12 * s7;
        float i3 = m04 * s2 - m08 * s5 - m12 * s8;

        float det = src[0] * i0 + src[1] * i1 + src[2] * i2 + src[3] * i3;
        if (det) det = 1.0f / det;

        float v04 = m01 *-s0 - m09 * s9 - m13 * s12;
        float v05 = m00 * s0 + m08 * s9 + m12 * s12;
        float v06 = m00 *-s1 - m08 * s10 - m12 * s13;
        float v07 = m00 *-s2 - m08 * s11 - m12 * s14;

        float v08 = m01 * s3 + m05 * s9 - m13 * s15;
        float v09 = m00 *-s3 - m04 * s9 + m12 * s15;
        float v10 = m00 * s4 + m04 * s10 - m12 * s16;
        float v11 = m00 * s5 + m04 * s11 - m12 * s17;

        float v12 = m01 * s6 + m05 * s12 + m09 * s15;
        float v13 = m00 *-s6 - m04 * s12 - m08 * s15;
        float v14 = m00 * s7 + m04 * s13 + m08 * s16;
        float v15 = m00 * s8 + m04 * s14 + m08 * s17;

        dest[0] = det * i0;
        dest[1] = det * i1;
        dest[2] = det * i2;
        dest[3] = det * i3;
        dest[4] = det * v04;
        dest[5] = det * v05;
        dest[6] = det * v06;
        dest[7] = det * v07;
        dest[8] = det * v08;
        dest[9] = det * v09;
        dest[10] = det * v10;
        dest[11] = det * v11;
        dest[12] = det * v12;
        dest[13] = det * v13;
        dest[14] = det * v14;
        dest[15] = det * v15;
    }

    static inline float32x4 float32x4_vector_matrix_multiply(float32x4 v, const float32x4* m)
    {
        float32x4 temp;
        temp[0] = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0] + v[3] * m[3][0];
        temp[1] = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1] + v[3] * m[3][1];
        temp[2] = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2] + v[3] * m[3][2];
        temp[3] = v[0] * m[0][3] + v[1] * m[1][3] + v[2] * m[2][3] + v[3] * m[3][3];
        return temp;
    }

    static inline void float32x4_matrix_matrix_multiply(float32x4* result, const float32x4* a, const float32x4* b)
    {
        result[0] = float32x4_vector_matrix_multiply(a[0], b);
        result[1] = float32x4_vector_matrix_multiply(a[1], b);
        result[2] = float32x4_vector_matrix_multiply(a[2], b);
        result[3] = float32x4_vector_matrix_multiply(a[3], b);
    }

} // namespace simd
} // namespace mango
