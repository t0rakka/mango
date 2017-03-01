/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifdef MANGO_INCLUDE_SIMD

    // -----------------------------------------------------------------
    // float32x2
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y>
    static inline float32x2 float32x2_shuffle(float32x2 v)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return {{ v[x], v[y] }};
    }

    // indexed access

    template <int Index>
    static inline float32x2 float32x2_set_component(float32x2 a, float s)
    {
        static_assert(Index >= 0 && Index < 2, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <int Index>
    static inline float float32x2_get_component(float32x2 a)
    {
        static_assert(Index >= 0 && Index < 2, "Index out of range.");
        return a[Index];
    }

    static inline float32x2 float32x2_zero()
    {
        return {{ 0.0f, 0.0f }};
    }

    static inline float32x2 float32x2_set1(float s)
    {
        return {{ s, s }};
    }

    static inline float32x2 float32x2_set2(float x, float y)
    {
        return {{ x, y }};
    }

    static inline float32x2 float32x2_uload(const float* source)
    {
        return float32x2_set2(source[0], source[1]);
    }

    static inline void float32x2_ustore(float* dest, float32x2 a)
    {
        dest[0] = a[0];
        dest[1] = a[1];
    }

    static inline float32x2 float32x2_unpackhi(float32x2 a, float32x2 b)
    {
        return float32x2_set2(a[1], b[1]);
    }

    static inline float32x2 float32x2_unpacklo(float32x2 a, float32x2 b)
    {
        return float32x2_set2(a[0], b[0]);
    }

    // logical

    static inline float32x2 float32x2_and(float32x2 a, float32x2 b)
    {
        const Float x(Float(a[0]).u & Float(b[0]).u);
        const Float y(Float(a[1]).u & Float(b[1]).u);
        return float32x2_set2(x, y);
    }

    static inline float32x2 float32x2_nand(float32x2 a, float32x2 b)
    {
        const Float x(~Float(a[0]).u & Float(b[0]).u);
        const Float y(~Float(a[1]).u & Float(b[1]).u);
        return float32x2_set2(x, y);
    }

    static inline float32x2 float32x2_or(float32x2 a, float32x2 b)
    {
        const Float x(Float(a[0]).u | Float(b[0]).u);
        const Float y(Float(a[1]).u | Float(b[1]).u);
        return float32x2_set2(x, y);
    }

    static inline float32x2 float32x2_xor(float32x2 a, float32x2 b)
    {
        const Float x(Float(a[0]).u ^ Float(b[0]).u);
        const Float y(Float(a[1]).u ^ Float(b[1]).u);
        return float32x2_set2(x, y);
    }

    static inline float32x2 float32x2_min(float32x2 a, float32x2 b)
    {
        float32x2 v;
        v[0] = std::min(a[0], b[0]);
        v[1] = std::min(a[1], b[1]);
        return v;
    }

    static inline float32x2 float32x2_max(float32x2 a, float32x2 b)
    {
        float32x2 v;
        v[0] = std::max(a[0], b[0]);
        v[1] = std::max(a[1], b[1]);
        return v;
    }

    static inline float32x2 float32x2_abs(float32x2 a)
    {
        float32x2 v;
        v[0] = std::abs(a[0]);
        v[1] = std::abs(a[1]);
        return v;
    }

    static inline float32x2 float32x2_neg(float32x2 a)
    {
        return float32x2_set2(-a[0], -a[1]);
    }

    static inline float32x2 float32x2_add(float32x2 a, float32x2 b)
    {
        float32x2 v;
        v[0] = a[0] + b[0];
        v[1] = a[1] + b[1];
        return v;
    }

    static inline float32x2 float32x2_sub(float32x2 a, float32x2 b)
    {
        float32x2 v;
        v[0] = a[0] - b[0];
        v[1] = a[1] - b[1];
        return v;
    }

    static inline float32x2 float32x2_mul(float32x2 a, float32x2 b)
    {
        float32x2 v;
        v[0] = a[0] * b[0];
        v[1] = a[1] * b[1];
        return v;
    }

    static inline float32x2 float32x2_div(float32x2 a, float32x2 b)
    {
        float32x2 v;
        v[0] = a[0] / b[0];
        v[1] = a[1] / b[1];
        return v;
    }

    static inline float32x2 float32x2_div(float32x2 a, float b)
    {
        float32x2 v;
        v[0] = a[0] / b;
        v[1] = a[1] / b;
        return v;
    }

    static inline float32x2 float32x2_madd(float32x2 a, float32x2 b, float32x2 c)
    {
        float32x2 v;
        v[0] = a[0] + b[0] * c[0];
        v[1] = a[1] + b[1] * c[1];
        return v;
    }

    static inline float32x2 float32x2_msub(float32x2 a, float32x2 b, float32x2 c)
    {
        float32x2 v;
        v[0] = a[0] - b[0] * c[0];
        v[1] = a[1] - b[1] * c[1];
        return v;
    }

    static inline float32x2 float32x2_fast_reciprocal(float32x2 a)
    {
        float32x2 v;
        v[0] = 1.0f / a[0];
        v[1] = 1.0f / a[1];
        return v;
    }

    static inline float32x2 float32x2_fast_rsqrt(float32x2 a)
    {
        float32x2 v;
        v[0] = 1.0f / float(std::sqrt(a[0]));
        v[1] = 1.0f / float(std::sqrt(a[1]));
        return v;
    }

    static inline float32x2 float32x2_fast_sqrt(float32x2 a)
    {
        float32x2 v;
        v[0] = float(std::sqrt(a[0]));
        v[1] = float(std::sqrt(a[1]));
        return v;
    }

    static inline float32x2 float32x2_reciprocal(float32x2 a)
    {
        return float32x2_fast_reciprocal(a);
    }

    static inline float32x2 float32x2_rsqrt(float32x2 a)
    {
        return float32x2_fast_rsqrt(a);
    }

    static inline float32x2 float32x2_sqrt(float32x2 a)
    {
        return float32x2_fast_sqrt(a);
    }

    static inline float32x2 float32x2_dot2(float32x2 a, float32x2 b)
    {
        const float s = a[0] * b[0] + a[1] * b[1];
        return float32x2_set1(s);
    }

    // compare

    static inline float32x2 float32x2_compare_neq(float32x2 a, float32x2 b)
    {
        float32x2 v;
        v[0] = Float(-uint32(a[0] != b[0]));
        v[1] = Float(-uint32(a[1] != b[1]));
        return v;
    }

    static inline float32x2 float32x2_compare_eq(float32x2 a, float32x2 b)
    {
        float32x2 v;
        v[0] = Float(-uint32(a[0] == b[0]));
        v[1] = Float(-uint32(a[1] == b[1]));
        return v;
    }

    static inline float32x2 float32x2_compare_lt(float32x2 a, float32x2 b)
    {
        float32x2 v;
        v[0] = Float(-uint32(a[0] < b[0]));
        v[1] = Float(-uint32(a[1] < b[1]));
        return v;
    }

    static inline float32x2 float32x2_compare_le(float32x2 a, float32x2 b)
    {
        float32x2 v;
        v[0] = Float(-uint32(a[0] <= b[0]));
        v[1] = Float(-uint32(a[1] <= b[1]));
        return v;
    }

    static inline float32x2 float32x2_compare_gt(float32x2 a, float32x2 b)
    {
        float32x2 v;
        v[0] = Float(-uint32(a[0] > b[0]));
        v[1] = Float(-uint32(a[1] > b[1]));
        return v;
    }

    static inline float32x2 float32x2_compare_ge(float32x2 a, float32x2 b)
    {
        float32x2 v;
        v[0] = Float(-uint32(a[0] >= b[0]));
        v[1] = Float(-uint32(a[1] >= b[1]));
        return v;
    }

    static inline float32x2 float32x2_select(float32x2 mask, float32x2 a, float32x2 b)
    {
        return float32x2_or(float32x2_and(mask, a), float32x2_nand(mask, b));
    }

    // rounding

    static inline float32x2 float32x2_round(float32x2 s)
    {
        float32x2 v;
        v[0] = std::round(s[0]);
        v[1] = std::round(s[1]);
        return v;
    }

    static inline float32x2 float32x2_trunc(float32x2 s)
    {
        float32x2 v;
        v[0] = std::trunc(s[0]);
        v[1] = std::trunc(s[1]);
        return v;
    }

    static inline float32x2 float32x2_floor(float32x2 s)
    {
        float32x2 v;
        v[0] = std::floor(s[0]);
        v[1] = std::floor(s[1]);
        return v;
    }

    static inline float32x2 float32x2_ceil(float32x2 s)
    {
        float32x2 v;
        v[0] = std::ceil(s[0]);
        v[1] = std::ceil(s[1]);
        return v;
    }

    static inline float32x2 float32x2_fract(float32x2 s)
    {
        float32x2 v;
        v[0] = s[0] - std::floor(s[0]);
        v[1] = s[1] - std::floor(s[1]);
        return v;
    }

#endif // MANGO_INCLUDE_SIMD
