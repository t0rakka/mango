/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifdef MANGO_INCLUDE_SIMD

    // -----------------------------------------------------------------
    // float32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    inline float32x4 float32x4_shuffle(float32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return {{ v[x], v[y], v[z], v[w] }};
    }

    template <>
    inline float32x4 float32x4_shuffle<0, 1, 2, 3>(float32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <int Index>
    static inline float32x4 float32x4_set_component(float32x4 a, float s)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <int Index>
    static inline float float32x4_get_component(float32x4 a)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        return a[Index];
    }

    static inline float32x4 float32x4_zero()
    {
        return {{ 0.0f, 0.0f, 0.0f, 0.0f }};
    }

    static inline float32x4 float32x4_set1(float s)
    {
        return {{ s, s, s, s }};
    }

    static inline float32x4 float32x4_set4(float x, float y, float z, float w)
    {
        return {{ x, y, z, w }};
    }

    static inline float32x4 float32x4_uload(const float* source)
    {
        return float32x4_set4(source[0], source[1], source[2], source[3]);
    }

    static inline void float32x4_ustore(float* dest, float32x4 a)
    {
        dest[0] = a[0];
        dest[1] = a[1];
        dest[2] = a[2];
        dest[3] = a[3];
    }

    static inline float32x4 float32x4_movelh(float32x4 a, float32x4 b)
    {
        return float32x4_set4(a[0], a[1], b[0], b[1]);
    }

    static inline float32x4 float32x4_movehl(float32x4 a, float32x4 b)
    {
        return float32x4_set4(b[2], b[3], a[2], a[3]);
    }

    static inline float32x4 float32x4_unpackhi(float32x4 a, float32x4 b)
    {
        return float32x4_set4(a[2], b[2], a[3], b[3]);
    }

    static inline float32x4 float32x4_unpacklo(float32x4 a, float32x4 b)
    {
        return float32x4_set4(a[0], b[0], a[1], b[1]);
    }

    // logical

    static inline float32x4 float32x4_and(float32x4 a, float32x4 b)
    {
        const Float x(Float(a[0]).u & Float(b[0]).u);
        const Float y(Float(a[1]).u & Float(b[1]).u);
        const Float z(Float(a[2]).u & Float(b[2]).u);
        const Float w(Float(a[3]).u & Float(b[3]).u);
        return float32x4_set4(x, y, z, w);
    }

    static inline float32x4 float32x4_nand(float32x4 a, float32x4 b)
    {
        const Float x(~Float(a[0]).u & Float(b[0]).u);
        const Float y(~Float(a[1]).u & Float(b[1]).u);
        const Float z(~Float(a[2]).u & Float(b[2]).u);
        const Float w(~Float(a[3]).u & Float(b[3]).u);
        return float32x4_set4(x, y, z, w);
    }

    static inline float32x4 float32x4_or(float32x4 a, float32x4 b)
    {
        const Float x(Float(a[0]).u | Float(b[0]).u);
        const Float y(Float(a[1]).u | Float(b[1]).u);
        const Float z(Float(a[2]).u | Float(b[2]).u);
        const Float w(Float(a[3]).u | Float(b[3]).u);
        return float32x4_set4(x, y, z, w);
    }

    static inline float32x4 float32x4_xor(float32x4 a, float32x4 b)
    {
        const Float x(Float(a[0]).u ^ Float(b[0]).u);
        const Float y(Float(a[1]).u ^ Float(b[1]).u);
        const Float z(Float(a[2]).u ^ Float(b[2]).u);
        const Float w(Float(a[3]).u ^ Float(b[3]).u);
        return float32x4_set4(x, y, z, w);
    }

    static inline float32x4 float32x4_min(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = std::min(a[0], b[0]);
        v[1] = std::min(a[1], b[1]);
        v[2] = std::min(a[2], b[2]);
        v[3] = std::min(a[3], b[3]);
        return v;
    }

    static inline float32x4 float32x4_max(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = std::max(a[0], b[0]);
        v[1] = std::max(a[1], b[1]);
        v[2] = std::max(a[2], b[2]);
        v[3] = std::max(a[3], b[3]);
        return v;
    }

    static inline float32x4 float32x4_hmin(float32x4 a)
    {
        float l = std::min(a[0], a[1]);
        float h = std::min(a[2], a[3]);
        float s = std::min(l, h);
        return float32x4_set1(s);
    }

    static inline float32x4 float32x4_hmax(float32x4 a)
    {
        float l = std::max(a[0], a[1]);
        float h = std::max(a[2], a[3]);
        float s = std::max(l, h);
        return float32x4_set1(s);
    }

    static inline float32x4 float32x4_abs(float32x4 a)
    {
        float32x4 v;
        v[0] = std::abs(a[0]);
        v[1] = std::abs(a[1]);
        v[2] = std::abs(a[2]);
        v[3] = std::abs(a[3]);
        return v;
    }

    static inline float32x4 float32x4_neg(float32x4 a)
    {
        return float32x4_set4(-a[0], -a[1], -a[2], -a[3]);
    }

    static inline float32x4 float32x4_add(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = a[0] + b[0];
        v[1] = a[1] + b[1];
        v[2] = a[2] + b[2];
        v[3] = a[3] + b[3];
        return v;
    }

    static inline float32x4 float32x4_sub(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = a[0] - b[0];
        v[1] = a[1] - b[1];
        v[2] = a[2] - b[2];
        v[3] = a[3] - b[3];
        return v;
    }

    static inline float32x4 float32x4_mul(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = a[0] * b[0];
        v[1] = a[1] * b[1];
        v[2] = a[2] * b[2];
        v[3] = a[3] * b[3];
        return v;
    }

    static inline float32x4 float32x4_div(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = a[0] / b[0];
        v[1] = a[1] / b[1];
        v[2] = a[2] / b[2];
        v[3] = a[3] / b[3];
        return v;
    }

    static inline float32x4 float32x4_div(float32x4 a, float b)
    {
        float32x4 v;
        v[0] = a[0] / b;
        v[1] = a[1] / b;
        v[2] = a[2] / b;
        v[3] = a[3] / b;
        return v;
    }

    static inline float32x4 float32x4_madd(float32x4 a, float32x4 b, float32x4 c)
    {
        float32x4 v;
        v[0] = a[0] + b[0] * c[0];
        v[1] = a[1] + b[1] * c[1];
        v[2] = a[2] + b[2] * c[2];
        v[3] = a[3] + b[3] * c[3];
        return v;
    }

    static inline float32x4 float32x4_msub(float32x4 a, float32x4 b, float32x4 c)
    {
        float32x4 v;
        v[0] = a[0] - b[0] * c[0];
        v[1] = a[1] - b[1] * c[1];
        v[2] = a[2] - b[2] * c[2];
        v[3] = a[3] - b[3] * c[3];
        return v;
    }

    static inline float32x4 float32x4_fast_reciprocal(float32x4 a)
    {
        float32x4 v;
        v[0] = 1.0f / a[0];
        v[1] = 1.0f / a[1];
        v[2] = 1.0f / a[2];
        v[3] = 1.0f / a[3];
        return v;
    }

    static inline float32x4 float32x4_fast_rsqrt(float32x4 a)
    {
        float32x4 v;
        v[0] = 1.0f / float(std::sqrt(a[0]));
        v[1] = 1.0f / float(std::sqrt(a[1]));
        v[2] = 1.0f / float(std::sqrt(a[2]));
        v[3] = 1.0f / float(std::sqrt(a[3]));
        return v;
    }

    static inline float32x4 float32x4_fast_sqrt(float32x4 a)
    {
        float32x4 v;
        v[0] = float(std::sqrt(a[0]));
        v[1] = float(std::sqrt(a[1]));
        v[2] = float(std::sqrt(a[2]));
        v[3] = float(std::sqrt(a[3]));
        return v;
    }

    static inline float32x4 float32x4_reciprocal(float32x4 a)
    {
        return float32x4_fast_reciprocal(a);
    }

    static inline float32x4 float32x4_rsqrt(float32x4 a)
    {
        return float32x4_fast_rsqrt(a);
    }

    static inline float32x4 float32x4_sqrt(float32x4 a)
    {
        return float32x4_fast_sqrt(a);
    }

    static inline float32x4 float32x4_dot3(float32x4 a, float32x4 b)
    {
        const float s = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
        return float32x4_set1(s);
    }

    static inline float32x4 float32x4_dot4(float32x4 a, float32x4 b)
    {
        const float s = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
        return float32x4_set1(s);
    }

    static inline float32x4 float32x4_cross3(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = a[1] * b[2] - a[2] * b[1];
        v[1] = a[2] * b[0] - a[0] * b[2];
        v[2] = a[0] * b[1] - a[1] * b[0];
        v[3] = 0.0f;
        return v;
    }

    // compare

    static inline float32x4 float32x4_compare_neq(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = Float(-uint32(a[0] != b[0]));
        v[1] = Float(-uint32(a[1] != b[1]));
        v[2] = Float(-uint32(a[2] != b[2]));
        v[3] = Float(-uint32(a[3] != b[3]));
        return v;
    }

    static inline float32x4 float32x4_compare_eq(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = Float(-uint32(a[0] == b[0]));
        v[1] = Float(-uint32(a[1] == b[1]));
        v[2] = Float(-uint32(a[2] == b[2]));
        v[3] = Float(-uint32(a[3] == b[3]));
        return v;
    }

    static inline float32x4 float32x4_compare_lt(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = Float(-uint32(a[0] < b[0]));
        v[1] = Float(-uint32(a[1] < b[1]));
        v[2] = Float(-uint32(a[2] < b[2]));
        v[3] = Float(-uint32(a[3] < b[3]));
        return v;
    }

    static inline float32x4 float32x4_compare_le(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = Float(-uint32(a[0] <= b[0]));
        v[1] = Float(-uint32(a[1] <= b[1]));
        v[2] = Float(-uint32(a[2] <= b[2]));
        v[3] = Float(-uint32(a[3] <= b[3]));
        return v;
    }

    static inline float32x4 float32x4_compare_gt(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = Float(-uint32(a[0] > b[0]));
        v[1] = Float(-uint32(a[1] > b[1]));
        v[2] = Float(-uint32(a[2] > b[2]));
        v[3] = Float(-uint32(a[3] > b[3]));
        return v;
    }

    static inline float32x4 float32x4_compare_ge(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = Float(-uint32(a[0] >= b[0]));
        v[1] = Float(-uint32(a[1] >= b[1]));
        v[2] = Float(-uint32(a[2] >= b[2]));
        v[3] = Float(-uint32(a[3] >= b[3]));
        return v;
    }

    static inline float32x4 float32x4_select(float32x4 mask, float32x4 a, float32x4 b)
    {
        return float32x4_or(float32x4_and(mask, a), float32x4_nand(mask, b));
    }

    // rounding

    static inline float32x4 float32x4_round(float32x4 s)
    {
        float32x4 v;
        v[0] = std::round(s[0]);
        v[1] = std::round(s[1]);
        v[2] = std::round(s[2]);
        v[3] = std::round(s[3]);
        return v;
    }

    static inline float32x4 float32x4_trunc(float32x4 s)
    {
        float32x4 v;
        v[0] = std::trunc(s[0]);
        v[1] = std::trunc(s[1]);
        v[2] = std::trunc(s[2]);
        v[3] = std::trunc(s[3]);
        return v;
    }

    static inline float32x4 float32x4_floor(float32x4 s)
    {
        float32x4 v;
        v[0] = std::floor(s[0]);
        v[1] = std::floor(s[1]);
        v[2] = std::floor(s[2]);
        v[3] = std::floor(s[3]);
        return v;
    }

    static inline float32x4 float32x4_ceil(float32x4 s)
    {
        float32x4 v;
        v[0] = std::ceil(s[0]);
        v[1] = std::ceil(s[1]);
        v[2] = std::ceil(s[2]);
        v[3] = std::ceil(s[3]);
        return v;
    }

    static inline float32x4 float32x4_fract(float32x4 s)
    {
        return float32x4_sub(s, float32x4_floor(s));
    }

    // -----------------------------------------------------------------
    // float32x4_matrix
    // -----------------------------------------------------------------

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

#endif // MANGO_INCLUDE_SIMD
