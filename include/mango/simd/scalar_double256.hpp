/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float64x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    inline float64x4 shuffle(float64x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return {{ v[x], v[y], v[z], v[w] }};
    }

    template <>
    inline float64x4 shuffle<0, 1, 2, 3>(float64x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline float64x4 set_component(float64x4 a, double s)
    {
        static_assert(Index < 4, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline double get_component(float64x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return a[Index];
    }

    static inline float64x4 float64x4_zero()
    {
        return {{ 0.0, 0.0, 0.0, 0.0 }};
    }

    static inline float64x4 float64x4_set1(double s)
    {
        return {{ s, s, s, s }};
    }

    static inline float64x4 float64x4_set4(double x, double y, double z, double w)
    {
        return {{ x, y, z, w }};
    }

    static inline float64x4 float64x4_uload(const double* source)
    {
        return float64x4_set4(source[0], source[1], source[2], source[3]);
    }

    static inline void float64x4_ustore(double* dest, float64x4 a)
    {
        dest[0] = a[0];
        dest[1] = a[1];
        dest[2] = a[2];
        dest[3] = a[3];
    }

    static inline float64x4 movelh(float64x4 a, float64x4 b)
    {
        return float64x4_set4(a[0], a[1], b[0], b[1]);
    }

    static inline float64x4 movehl(float64x4 a, float64x4 b)
    {
        return float64x4_set4(b[2], b[3], a[2], a[3]);
    }

    static inline float64x4 unpackhi(float64x4 a, float64x4 b)
    {
        return float64x4_set4(a[1], b[1], a[3], b[3]);
    }

    static inline float64x4 unpacklo(float64x4 a, float64x4 b)
    {
        return float64x4_set4(a[0], b[0], a[2], b[2]);
    }

    // bitwise

    static inline float64x4 bitwise_nand(float64x4 a, float64x4 b)
    {
        const Double x(~Double(a[0]).u & Double(b[0]).u);
        const Double y(~Double(a[1]).u & Double(b[1]).u);
        const Double z(~Double(a[2]).u & Double(b[2]).u);
        const Double w(~Double(a[3]).u & Double(b[3]).u);
        return float64x4_set4(x, y, z, w);
    }

    static inline float64x4 bitwise_and(float64x4 a, float64x4 b)
    {
        const Double x(Double(a[0]).u & Double(b[0]).u);
        const Double y(Double(a[1]).u & Double(b[1]).u);
        const Double z(Double(a[2]).u & Double(b[2]).u);
        const Double w(Double(a[3]).u & Double(b[3]).u);
        return float64x4_set4(x, y, z, w);
    }

    static inline float64x4 bitwise_or(float64x4 a, float64x4 b)
    {
        const Double x(Double(a[0]).u | Double(b[0]).u);
        const Double y(Double(a[1]).u | Double(b[1]).u);
        const Double z(Double(a[2]).u | Double(b[2]).u);
        const Double w(Double(a[3]).u | Double(b[3]).u);
        return float64x4_set4(x, y, z, w);
    }

    static inline float64x4 bitwise_xor(float64x4 a, float64x4 b)
    {
        const Double x(Double(a[0]).u ^ Double(b[0]).u);
        const Double y(Double(a[1]).u ^ Double(b[1]).u);
        const Double z(Double(a[2]).u ^ Double(b[2]).u);
        const Double w(Double(a[3]).u ^ Double(b[3]).u);
        return float64x4_set4(x, y, z, w);
    }

    static inline float64x4 bitwise_not(float64x4 a)
    {
        const Double x(~Double(a[0]).u);
        const Double y(~Double(a[1]).u);
        const Double z(~Double(a[2]).u);
        const Double w(~Double(a[3]).u);
        return float64x4_set4(x, y, z, w);
    }

    static inline float64x4 min(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = std::min(a[0], b[0]);
        v[1] = std::min(a[1], b[1]);
        v[2] = std::min(a[2], b[2]);
        v[3] = std::min(a[3], b[3]);
        return v;
    }

    static inline float64x4 max(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = std::max(a[0], b[0]);
        v[1] = std::max(a[1], b[1]);
        v[2] = std::max(a[2], b[2]);
        v[3] = std::max(a[3], b[3]);
        return v;
    }

    static inline float64x4 hmin(float64x4 a)
    {
        double l = std::min(a[0], a[1]);
        double h = std::min(a[2], a[3]);
        double s = std::min(l, h);
        return float64x4_set1(s);
    }

    static inline float64x4 hmax(float64x4 a)
    {
        double l = std::max(a[0], a[1]);
        double h = std::max(a[2], a[3]);
        double s = std::max(l, h);
        return float64x4_set1(s);
    }

    static inline float64x4 abs(float64x4 a)
    {
        float64x4 v;
        v[0] = std::abs(a[0]);
        v[1] = std::abs(a[1]);
        v[2] = std::abs(a[2]);
        v[3] = std::abs(a[3]);
        return v;
    }

    static inline float64x4 neg(float64x4 a)
    {
        return float64x4_set4(-a[0], -a[1], -a[2], -a[3]);
    }

    static inline float64x4 add(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = a[0] + b[0];
        v[1] = a[1] + b[1];
        v[2] = a[2] + b[2];
        v[3] = a[3] + b[3];
        return v;
    }

    static inline float64x4 sub(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = a[0] - b[0];
        v[1] = a[1] - b[1];
        v[2] = a[2] - b[2];
        v[3] = a[3] - b[3];
        return v;
    }

    static inline float64x4 mul(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = a[0] * b[0];
        v[1] = a[1] * b[1];
        v[2] = a[2] * b[2];
        v[3] = a[3] * b[3];
        return v;
    }

    static inline float64x4 div(float64x4 a, float64x4 b)
    {
        float64x4 v;
        v[0] = a[0] / b[0];
        v[1] = a[1] / b[1];
        v[2] = a[2] / b[2];
        v[3] = a[3] / b[3];
        return v;
    }

    static inline float64x4 div(float64x4 a, double b)
    {
        float64x4 v;
        v[0] = a[0] / b;
        v[1] = a[1] / b;
        v[2] = a[2] / b;
        v[3] = a[3] / b;
        return v;
    }

    static inline float64x4 madd(float64x4 a, float64x4 b, float64x4 c)
    {
        float64x4 v;
        v[0] = a[0] + b[0] * c[0];
        v[1] = a[1] + b[1] * c[1];
        v[2] = a[2] + b[2] * c[2];
        v[3] = a[3] + b[3] * c[3];
        return v;
    }

    static inline float64x4 msub(float64x4 a, float64x4 b, float64x4 c)
    {
        float64x4 v;
        v[0] = a[0] - b[0] * c[0];
        v[1] = a[1] - b[1] * c[1];
        v[2] = a[2] - b[2] * c[2];
        v[3] = a[3] - b[3] * c[3];
        return v;
    }

    static inline float64x4 fast_reciprocal(float64x4 a)
    {
        float64x4 v;
        v[0] = 1.0 / a[0];
        v[1] = 1.0 / a[1];
        v[2] = 1.0 / a[2];
        v[3] = 1.0 / a[3];
        return v;
    }

    static inline float64x4 fast_rsqrt(float64x4 a)
    {
        float64x4 v;
        v[0] = 1.0 / std::sqrt(a[0]);
        v[1] = 1.0 / std::sqrt(a[1]);
        v[2] = 1.0 / std::sqrt(a[2]);
        v[3] = 1.0 / std::sqrt(a[3]);
        return v;
    }

    static inline float64x4 fast_sqrt(float64x4 a)
    {
        float64x4 v;
        v[0] = std::sqrt(a[0]);
        v[1] = std::sqrt(a[1]);
        v[2] = std::sqrt(a[2]);
        v[3] = std::sqrt(a[3]);
        return v;
    }

    static inline float64x4 reciprocal(float64x4 a)
    {
        return fast_reciprocal(a);
    }

    static inline float64x4 rsqrt(float64x4 a)
    {
        return fast_rsqrt(a);
    }

    static inline float64x4 sqrt(float64x4 a)
    {
        return fast_sqrt(a);
    }

    static inline float64x4 dot4(float64x4 a, float64x4 b)
    {
        const double s = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
        return float64x4_set1(s);
    }

    // compare

    static inline float64x4::mask compare_neq(float64x4 a, float64x4 b)
    {
        float64x4::mask v = 0;
        v |= uint32(a[0] != b[0]) << 0;
        v |= uint32(a[1] != b[1]) << 1;
        v |= uint32(a[2] != b[2]) << 2;
        v |= uint32(a[3] != b[3]) << 3;
        return v;
    }

    static inline float64x4::mask compare_eq(float64x4 a, float64x4 b)
    {
        float64x4::mask v = 0;
        v |= uint32(a[0] == b[0]) << 0;
        v |= uint32(a[1] == b[1]) << 1;
        v |= uint32(a[2] == b[2]) << 2;
        v |= uint32(a[3] == b[3]) << 3;
        return v;
    }

    static inline float64x4::mask compare_lt(float64x4 a, float64x4 b)
    {
        float64x4::mask v = 0;
        v |= uint32(a[0] < b[0]) << 0;
        v |= uint32(a[1] < b[1]) << 1;
        v |= uint32(a[2] < b[2]) << 2;
        v |= uint32(a[3] < b[3]) << 3;
        return v;
    }

    static inline float64x4::mask compare_le(float64x4 a, float64x4 b)
    {
        float64x4::mask v = 0;
        v |= uint32(a[0] <= b[0]) << 0;
        v |= uint32(a[1] <= b[1]) << 1;
        v |= uint32(a[2] <= b[2]) << 2;
        v |= uint32(a[3] <= b[3]) << 3;
        return v;
    }

    static inline float64x4::mask compare_gt(float64x4 a, float64x4 b)
    {
        float64x4::mask v = 0;
        v |= uint32(a[0] > b[0]) << 0;
        v |= uint32(a[1] > b[1]) << 1;
        v |= uint32(a[2] > b[2]) << 2;
        v |= uint32(a[3] > b[3]) << 3;
        return v;
    }

    static inline float64x4::mask compare_ge(float64x4 a, float64x4 b)
    {
        float64x4::mask v = 0;
        v |= uint32(a[0] >= b[0]) << 0;
        v |= uint32(a[1] >= b[1]) << 1;
        v |= uint32(a[2] >= b[2]) << 2;
        v |= uint32(a[3] >= b[3]) << 3;
        return v;
    }

    static inline float64x4 select(float64x4::mask mask, float64x4 a, float64x4 b)
    {
        float64x4 result;
        result[0] = mask & (1 << 0) ? a[0] : b[0];
        result[1] = mask & (1 << 1) ? a[1] : b[1];
        result[2] = mask & (1 << 2) ? a[2] : b[2];
        result[3] = mask & (1 << 3) ? a[3] : b[3];
        return result;
    }

    // rounding

    static inline float64x4 round(float64x4 s)
    {
        float64x4 v;
        v[0] = std::round(s[0]);
        v[1] = std::round(s[1]);
        v[2] = std::round(s[2]);
        v[3] = std::round(s[3]);
        return v;
    }

    static inline float64x4 trunc(float64x4 s)
    {
        float64x4 v;
        v[0] = std::trunc(s[0]);
        v[1] = std::trunc(s[1]);
        v[2] = std::trunc(s[2]);
        v[3] = std::trunc(s[3]);
        return v;
    }

    static inline float64x4 floor(float64x4 s)
    {
        float64x4 v;
        v[0] = std::floor(s[0]);
        v[1] = std::floor(s[1]);
        v[2] = std::floor(s[2]);
        v[3] = std::floor(s[3]);
        return v;
    }

    static inline float64x4 ceil(float64x4 s)
    {
        float64x4 v;
        v[0] = std::ceil(s[0]);
        v[1] = std::ceil(s[1]);
        v[2] = std::ceil(s[2]);
        v[3] = std::ceil(s[3]);
        return v;
    }

    static inline float64x4 fract(float64x4 s)
    {
        float64x4 v;
        v[0] = s[0] - std::floor(s[0]);
        v[1] = s[1] - std::floor(s[1]);
        v[2] = s[2] - std::floor(s[2]);
        v[3] = s[3] - std::floor(s[3]);
        return v;
    }

} // namespace simd
} // namespace mango
