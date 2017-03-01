/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifdef MANGO_INCLUDE_SIMD

    // -----------------------------------------------------------------
    // float64x2
    // -----------------------------------------------------------------

    template <uint32 x, uint32 y>
    static inline float64x2 float64x2_shuffle(float64x2 v)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return {{ v[x], v[y] }};
    }

    // indexed access

    template <int Index>
    static inline float64x2 float64x2_set_component(float64x2 a, double s)
    {
        static_assert(Index >= 0 && Index < 2, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <int Index>
    static inline double float64x2_get_component(float64x2 a)
    {
        static_assert(Index >= 0 && Index < 2, "Index out of range.");
        return a[Index];
    }

    static inline float64x2 float64x2_zero()
    {
        return {{ 0.0, 0.0 }};
    }

    static inline float64x2 float64x2_set1(double s)
    {
        return {{ s, s }};
    }

    static inline float64x2 float64x2_set2(double x, double y)
    {
        return {{ x, y }};
    }

    static inline float64x2 float64x2_uload(const double* source)
    {
        return float64x2_set2(source[0], source[1]);
    }

    static inline void float64x2_ustore(double* dest, float64x2 a)
    {
        dest[0] = a[0];
        dest[1] = a[1];
    }

    static inline float64x2 float64x2_unpackhi(float64x2 a, float64x2 b)
    {
        return float64x2_set2(a[1], b[1]);
    }

    static inline float64x2 float64x2_unpacklo(float64x2 a, float64x2 b)
    {
        return float64x2_set2(a[0], b[0]);
    }

    // logical

    static inline float64x2 float64x2_and(float64x2 a, float64x2 b)
    {
        const Double x(Double(a[0]).u & Double(b[0]).u);
        const Double y(Double(a[1]).u & Double(b[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 float64x2_nand(float64x2 a, float64x2 b)
    {
        const Double x(~Double(a[0]).u & Double(b[0]).u);
        const Double y(~Double(a[1]).u & Double(b[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 float64x2_or(float64x2 a, float64x2 b)
    {
        const Double x(Double(a[0]).u | Double(b[0]).u);
        const Double y(Double(a[1]).u | Double(b[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 float64x2_xor(float64x2 a, float64x2 b)
    {
        const Double x(Double(a[0]).u ^ Double(b[0]).u);
        const Double y(Double(a[1]).u ^ Double(b[1]).u);
        return float64x2_set2(x, y);
    }

    static inline float64x2 float64x2_min(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = std::min(a[0], b[0]);
        v[1] = std::min(a[1], b[1]);
        return v;
    }

    static inline float64x2 float64x2_max(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = std::max(a[0], b[0]);
        v[1] = std::max(a[1], b[1]);
        return v;
    }

    static inline float64x2 float64x2_abs(float64x2 a)
    {
        float64x2 v;
        v[0] = std::abs(a[0]);
        v[1] = std::abs(a[1]);
        return v;
    }

    static inline float64x2 float64x2_neg(float64x2 a)
    {
        return float64x2_set2(-a[0], -a[1]);
    }

    static inline float64x2 float64x2_add(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = a[0] + b[0];
        v[1] = a[1] + b[1];
        return v;
    }

    static inline float64x2 float64x2_sub(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = a[0] - b[0];
        v[1] = a[1] - b[1];
        return v;
    }

    static inline float64x2 float64x2_mul(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = a[0] * b[0];
        v[1] = a[1] * b[1];
        return v;
    }

    static inline float64x2 float64x2_div(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = a[0] / b[0];
        v[1] = a[1] / b[1];
        return v;
    }

    static inline float64x2 float64x2_div(float64x2 a, double b)
    {
        float64x2 v;
        v[0] = a[0] + b;
        v[1] = a[1] + b;
        return v;
    }

    static inline float64x2 float64x2_madd(float64x2 a, float64x2 b, float64x2 c)
    {
        float64x2 v;
        v[0] = a[0] + b[0] * c[0];
        v[1] = a[1] + b[1] * c[1];
        return v;
    }

    static inline float64x2 float64x2_msub(float64x2 a, float64x2 b, float64x2 c)
    {
        float64x2 v;
        v[0] = a[0] - b[0] * c[0];
        v[1] = a[1] - b[1] * c[1];
        return v;
    }

    static inline float64x2 float64x2_fast_reciprocal(float64x2 a)
    {
        float64x2 v;
        v[0] = 1.0 / a[0];
        v[1] = 1.0 / a[1];
        return v;
    }

    static inline float64x2 float64x2_fast_rsqrt(float64x2 a)
    {
        float64x2 v;
        v[0] = 1.0 / std::sqrt(a[0]);
        v[1] = 1.0 / std::sqrt(a[1]);
        return v;
    }

    static inline float64x2 float64x2_fast_sqrt(float64x2 a)
    {
        float64x2 v;
        v[0] = std::sqrt(a[0]);
        v[1] = std::sqrt(a[1]);
        return v;
    }

    static inline float64x2 float64x2_reciprocal(float64x2 a)
    {
        return float64x2_fast_reciprocal(a);
    }

    static inline float64x2 float64x2_rsqrt(float64x2 a)
    {
        return float64x2_fast_rsqrt(a);
    }

    static inline float64x2 float64x2_sqrt(float64x2 a)
    {
        return float64x2_fast_sqrt(a);
    }

    static inline float64x2 float64x2_dot2(float64x2 a, float64x2 b)
    {
        const double s = a[0] * b[0] + a[1] * b[1];
        return float64x2_set1(s);
    }

    // compare

    static inline float64x2 float64x2_compare_neq(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = Double(-uint64(a[0] != b[0]));
        v[1] = Double(-uint64(a[1] != b[1]));
        return v;
    }

    static inline float64x2 float64x2_compare_eq(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = Double(-uint64(a[0] == b[0]));
        v[1] = Double(-uint64(a[1] == b[1]));
        return v;
    }

    static inline float64x2 float64x2_compare_lt(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = Double(-uint64(a[0] < b[0]));
        v[1] = Double(-uint64(a[1] < b[1]));
        return v;
    }

    static inline float64x2 float64x2_compare_le(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = Double(-uint64(a[0] <= b[0]));
        v[1] = Double(-uint64(a[1] <= b[1]));
        return v;
    }

    static inline float64x2 float64x2_compare_gt(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = Double(-uint64(a[0] > b[0]));
        v[1] = Double(-uint64(a[1] > b[1]));
        return v;
    }

    static inline float64x2 float64x2_compare_ge(float64x2 a, float64x2 b)
    {
        float64x2 v;
        v[0] = Double(-uint64(a[0] >= b[0]));
        v[1] = Double(-uint64(a[1] >= b[1]));
        return v;
    }

    static inline float64x2 float64x2_select(float64x2 mask, float64x2 a, float64x2 b)
    {
        return float64x2_or(float64x2_and(mask, a), float64x2_nand(mask, b));
    }

    // rounding

    static inline float64x2 float64x2_round(float64x2 s)
    {
        float64x2 v;
        v[0] = std::round(s[0]);
        v[1] = std::round(s[1]);
        return v;
    }

    static inline float64x2 float64x2_trunc(float64x2 s)
    {
        float64x2 v;
        v[0] = std::trunc(s[0]);
        v[1] = std::trunc(s[1]);
        return v;
    }

    static inline float64x2 float64x2_floor(float64x2 s)
    {
        float64x2 v;
        v[0] = std::floor(s[0]);
        v[1] = std::floor(s[1]);
        return v;
    }

    static inline float64x2 float64x2_ceil(float64x2 s)
    {
        float64x2 v;
        v[0] = std::ceil(s[0]);
        v[1] = std::ceil(s[1]);
        return v;
    }

    static inline float64x2 float64x2_fract(float64x2 s)
    {
        float64x2 v;
        v[0] = s[0] - std::floor(s[0]);
        v[1] = s[1] - std::floor(s[1]);
        return v;
    }

#endif // MANGO_INCLUDE_SIMD
