/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

#ifdef MANGO_SIMD_DOUBLE_SCALAR

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float64x4
    // -----------------------------------------------------------------

    // shuffle

    template <int x, int y, int z, int w>
    inline float64x4 float64x4_shuffle(float64x4 v)
    {
        // .generic
        return float64x4(v[x], v[y], v[z], v[w]);
    }

    template <>
    inline float64x4 float64x4_shuffle<0, 1, 2, 3>(float64x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <int Index>
    static inline float64x4 float64x4_set_component(float64x4 a, double s)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <int Index>
    static inline double float64x4_get_component(float64x4 a)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        return a[Index];
    }

    static inline float64x4 float64x4_zero()
    {
        return float64x4(0.0, 0.0, 0.0, 0.0);
    }

    static inline float64x4 float64x4_set1(double s)
    {
        return float64x4(s, s, s, s);
    }

    static inline float64x4 float64x4_set4(double x, double y, double z, double w)
    {
        return float64x4(x, y, z, w);
    }

    static inline float64x4 float64x4_uload(const double* source)
    {
        return float64x4(source[0], source[1], source[2], source[3]);
    }

    static inline void float64x4_ustore(double* dest, float64x4 a)
    {
        dest[0] = a[0];
        dest[1] = a[1];
        dest[2] = a[2];
        dest[3] = a[3];
    }

    static inline float64x4 float64x4_movelh(float64x4 a, float64x4 b)
    {
        return float64x4(a[0], a[1], b[0], b[1]);
    }

    static inline float64x4 float64x4_movehl(float64x4 a, float64x4 b)
    {
        return float64x4(b[2], b[3], a[2], a[3]);
    }

    static inline float64x4 float64x4_unpackhi(float64x4 a, float64x4 b)
    {
        return float64x4(a[1], b[1], a[3], b[3]);
    }

    static inline float64x4 float64x4_unpacklo(float64x4 a, float64x4 b)
    {
        return float64x4(a[0], b[0], a[2], b[2]);
    }

    // logical

    static inline float64x4 float64x4_and(float64x4 a, float64x4 b)
    {
        const Double x(Double(a[0]).u & Double(b[0]).u);
        const Double y(Double(a[1]).u & Double(b[1]).u);
        const Double z(Double(a[2]).u & Double(b[2]).u);
        const Double w(Double(a[3]).u & Double(b[3]).u);
        return float64x4_set4(x, y, z, w);
    }

    static inline float64x4 float64x4_nand(float64x4 a, float64x4 b)
    {
        const Double x(~Double(a[0]).u & Double(b[0]).u);
        const Double y(~Double(a[1]).u & Double(b[1]).u);
        const Double z(~Double(a[2]).u & Double(b[2]).u);
        const Double w(~Double(a[3]).u & Double(b[3]).u);
        return float64x4_set4(x, y, z, w);
    }

    static inline float64x4 float64x4_or(float64x4 a, float64x4 b)
    {
        const Double x(Double(a[0]).u | Double(b[0]).u);
        const Double y(Double(a[1]).u | Double(b[1]).u);
        const Double z(Double(a[2]).u | Double(b[2]).u);
        const Double w(Double(a[3]).u | Double(b[3]).u);
        return float64x4_set4(x, y, z, w);
    }

    static inline float64x4 float64x4_xor(float64x4 a, float64x4 b)
    {
        const Double x(Double(a[0]).u ^ Double(b[0]).u);
        const Double y(Double(a[1]).u ^ Double(b[1]).u);
        const Double z(Double(a[2]).u ^ Double(b[2]).u);
        const Double w(Double(a[3]).u ^ Double(b[3]).u);
        return float64x4_set4(x, y, z, w);
    }

    static inline float64x4 float64x4_min(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = std::min(a[0], b[0]);
        v[1] = std::min(a[1], b[1]);
        v[2] = std::min(a[2], b[2]);
        v[3] = std::min(a[3], b[3]);
        return v;
    }

    static inline float64x4 float64x4_max(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = std::max(a[0], b[0]);
        v[1] = std::max(a[1], b[1]);
        v[2] = std::max(a[2], b[2]);
        v[3] = std::max(a[3], b[3]);
        return v;
    }

    static inline float64x4 float64x4_clamp(float64x4 a, float64x4 vmin, float64x4 vmax)
    {
        float64x4 v;
        v[0] = std::min(vmax[0], std::max(vmin[0], a[0]));
        v[1] = std::min(vmax[1], std::max(vmin[1], a[1]));
        v[2] = std::min(vmax[2], std::max(vmin[2], a[2]));
        v[3] = std::min(vmax[3], std::max(vmin[3], a[3]));
        return v;
    }

    static inline float64x4 float64x4_abs(float64x4 a)
    {
        float64x4 v;
        v[0] = std::abs(a[0]);
        v[1] = std::abs(a[1]);
        v[2] = std::abs(a[2]);
        v[3] = std::abs(a[3]);
        return v;
    }

    static inline float64x4 float64x4_neg(float64x4 a)
    {
        return float64x4(-a[0], -a[1], -a[2], -a[3]);
    }

    static inline float64x4 float64x4_add(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = a[0] + b[0];
        v[1] = a[1] + b[1];
        v[2] = a[2] + b[2];
        v[3] = a[3] + b[3];
        return v;
    }

    static inline float64x4 float64x4_sub(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = a[0] - b[0];
        v[1] = a[1] - b[1];
        v[2] = a[2] - b[2];
        v[3] = a[3] - b[3];
        return v;
    }

    static inline float64x4 float64x4_mul(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = a[0] * b[0];
        v[1] = a[1] * b[1];
        v[2] = a[2] * b[2];
        v[3] = a[3] * b[3];
        return v;
    }

    static inline float64x4 float64x4_div(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = a[0] / b[0];
        v[1] = a[1] / b[1];
        v[2] = a[2] / b[2];
        v[3] = a[3] / b[3];
        return v;
    }

    static inline float64x4 float64x4_div(float64x4 a, double b)
    {
        float64x4 v;
        v[0] = a[0] / b;
        v[1] = a[1] / b;
        v[2] = a[2] / b;
        v[3] = a[3] / b;
        return v;
    }

    static inline float64x4 float64x4_madd(float64x4 a, float64x4 b, float64x4 c)
    {
        float64x4 v;
        v[0] = a[0] + b[0] * c[0];
        v[1] = a[1] + b[1] * c[1];
        v[2] = a[2] + b[2] * c[2];
        v[3] = a[3] + b[3] * c[3];
        return v;
    }

    static inline float64x4 float64x4_msub(float64x4 a, float64x4 b, float64x4 c)
    {
        float64x4 v;
        v[0] = a[0] - b[0] * c[0];
        v[1] = a[1] - b[1] * c[1];
        v[2] = a[2] - b[2] * c[2];
        v[3] = a[3] - b[3] * c[3];
        return v;
    }

    static inline float64x4 float64x4_fast_reciprocal(float64x4 a)
    {
        float64x4 v;
        v[0] = 1.0 / a[0];
        v[1] = 1.0 / a[1];
        v[2] = 1.0 / a[2];
        v[3] = 1.0 / a[3];
        return v;
    }

    static inline float64x4 float64x4_fast_rsqrt(float64x4 a)
    {
        float64x4 v;
        v[0] = 1.0 / std::sqrt(a[0]);
        v[1] = 1.0 / std::sqrt(a[1]);
        v[2] = 1.0 / std::sqrt(a[2]);
        v[3] = 1.0 / std::sqrt(a[3]);
        return v;
    }

    static inline float64x4 float64x4_fast_sqrt(float64x4 a)
    {
        float64x4 v;
        v[0] = std::sqrt(a[0]);
        v[1] = std::sqrt(a[1]);
        v[2] = std::sqrt(a[2]);
        v[3] = std::sqrt(a[3]);
        return v;
    }

    static inline float64x4 float64x4_reciprocal(float64x4 a)
    {
        return float64x4_fast_reciprocal(a);
    }

    static inline float64x4 float64x4_rsqrt(float64x4 a)
    {
        return float64x4_fast_rsqrt(a);
    }

    static inline float64x4 float64x4_sqrt(float64x4 a)
    {
        return float64x4_fast_sqrt(a);
    }

    static inline float64x4 float64x4_dot4(float64x4 a, float64x4 b)
    {
        const double s = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
        return float64x4_set1(s);
    }

    // compare

    static inline float64x4 float64x4_compare_neq(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = Double(uint64(a[0] != b[0] ? 0xffffffffffffffff : 0));
        v[1] = Double(uint64(a[1] != b[1] ? 0xffffffffffffffff : 0));
        v[2] = Double(uint64(a[2] != b[2] ? 0xffffffffffffffff : 0));
        v[3] = Double(uint64(a[3] != b[3] ? 0xffffffffffffffff : 0));
        return v;
    }

    static inline float64x4 float64x4_compare_eq(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = Double(uint64(a[0] == b[0] ? 0xffffffffffffffff : 0));
        v[1] = Double(uint64(a[1] == b[1] ? 0xffffffffffffffff : 0));
        v[2] = Double(uint64(a[2] == b[2] ? 0xffffffffffffffff : 0));
        v[3] = Double(uint64(a[3] == b[3] ? 0xffffffffffffffff : 0));
        return v;
    }

    static inline float64x4 float64x4_compare_lt(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = Double(uint64(a[0] < b[0] ? 0xffffffffffffffff : 0));
        v[1] = Double(uint64(a[1] < b[1] ? 0xffffffffffffffff : 0));
        v[2] = Double(uint64(a[2] < b[2] ? 0xffffffffffffffff : 0));
        v[3] = Double(uint64(a[3] < b[3] ? 0xffffffffffffffff : 0));
        return v;
    }

    static inline float64x4 float64x4_compare_le(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = Double(uint64(a[0] <= b[0] ? 0xffffffffffffffff : 0));
        v[1] = Double(uint64(a[1] <= b[1] ? 0xffffffffffffffff : 0));
        v[2] = Double(uint64(a[2] <= b[2] ? 0xffffffffffffffff : 0));
        v[3] = Double(uint64(a[3] <= b[3] ? 0xffffffffffffffff : 0));
        return v;
    }

    static inline float64x4 float64x4_compare_gt(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = Double(uint64(a[0] > b[0] ? 0xffffffffffffffff : 0));
        v[1] = Double(uint64(a[1] > b[1] ? 0xffffffffffffffff : 0));
        v[2] = Double(uint64(a[2] > b[2] ? 0xffffffffffffffff : 0));
        v[3] = Double(uint64(a[3] > b[3] ? 0xffffffffffffffff : 0));
        return v;
    }

    static inline float64x4 float64x4_compare_ge(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = Double(uint64(a[0] >= b[0] ? 0xffffffffffffffff : 0));
        v[1] = Double(uint64(a[1] >= b[1] ? 0xffffffffffffffff : 0));
        v[2] = Double(uint64(a[2] >= b[2] ? 0xffffffffffffffff : 0));
        v[3] = Double(uint64(a[3] >= b[3] ? 0xffffffffffffffff : 0));
        return v;
    }

    static inline float64x4 float64x4_select(float64x4 mask, float64x4 a, float64x4 b)
    {
        return float64x4_or(float64x4_and(mask, a), float64x4_nand(mask, b));
    }

    // rounding

    static inline float64x4 float64x4_round(float64x4 s)
    {
        float64x4 v;
        v[0] = std::round(s[0]);
        v[1] = std::round(s[1]);
        v[2] = std::round(s[2]);
        v[3] = std::round(s[3]);
        return v;
    }

    static inline float64x4 float64x4_trunc(float64x4 s)
    {
        float64x4 v;
        v[0] = std::trunc(s[0]);
        v[1] = std::trunc(s[1]);
        v[2] = std::trunc(s[2]);
        v[3] = std::trunc(s[3]);
        return v;
    }

    static inline float64x4 float64x4_floor(float64x4 s)
    {
        float64x4 v;
        v[0] = std::floor(s[0]);
        v[1] = std::floor(s[1]);
        v[2] = std::floor(s[2]);
        v[3] = std::floor(s[3]);
        return v;
    }

    static inline float64x4 float64x4_ceil(float64x4 s)
    {
        float64x4 v;
        v[0] = std::ceil(s[0]);
        v[1] = std::ceil(s[1]);
        v[2] = std::ceil(s[2]);
        v[3] = std::ceil(s[3]);
        return v;
    }

    static inline float64x4 float64x4_fract(float64x4 s)
    {
        return float64x4_sub(s, float64x4_floor(s));
    }

} // namespace simd
} // namespace mango

#endif // MANGO_SIMD_DOUBLE_SCALAR
