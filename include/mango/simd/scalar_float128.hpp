/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    inline float32x4 shuffle(float32x4 a, float32x4 b)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return {{ a[x], a[y], b[z], b[w] }};
    }

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    inline float32x4 shuffle(float32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return {{ v[x], v[y], v[z], v[w] }};
    }

    template <>
    inline float32x4 shuffle<0, 1, 2, 3>(float32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline float32x4 set_component(float32x4 a, float s)
    {
        static_assert(Index < 4, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline float get_component(float32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
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

    static inline float32x4 movelh(float32x4 a, float32x4 b)
    {
        return float32x4_set4(a[0], a[1], b[0], b[1]);
    }

    static inline float32x4 movehl(float32x4 a, float32x4 b)
    {
        return float32x4_set4(b[2], b[3], a[2], a[3]);
    }

    static inline float32x4 unpackhi(float32x4 a, float32x4 b)
    {
        return float32x4_set4(a[2], b[2], a[3], b[3]);
    }

    static inline float32x4 unpacklo(float32x4 a, float32x4 b)
    {
        return float32x4_set4(a[0], b[0], a[1], b[1]);
    }

    // bitwise

    static inline float32x4 bitwise_nand(float32x4 a, float32x4 b)
    {
        const Float x(~Float(a[0]).u & Float(b[0]).u);
        const Float y(~Float(a[1]).u & Float(b[1]).u);
        const Float z(~Float(a[2]).u & Float(b[2]).u);
        const Float w(~Float(a[3]).u & Float(b[3]).u);
        return float32x4_set4(x, y, z, w);
    }

    static inline float32x4 bitwise_and(float32x4 a, float32x4 b)
    {
        const Float x(Float(a[0]).u & Float(b[0]).u);
        const Float y(Float(a[1]).u & Float(b[1]).u);
        const Float z(Float(a[2]).u & Float(b[2]).u);
        const Float w(Float(a[3]).u & Float(b[3]).u);
        return float32x4_set4(x, y, z, w);
    }

    static inline float32x4 bitwise_or(float32x4 a, float32x4 b)
    {
        const Float x(Float(a[0]).u | Float(b[0]).u);
        const Float y(Float(a[1]).u | Float(b[1]).u);
        const Float z(Float(a[2]).u | Float(b[2]).u);
        const Float w(Float(a[3]).u | Float(b[3]).u);
        return float32x4_set4(x, y, z, w);
    }

    static inline float32x4 bitwise_xor(float32x4 a, float32x4 b)
    {
        const Float x(Float(a[0]).u ^ Float(b[0]).u);
        const Float y(Float(a[1]).u ^ Float(b[1]).u);
        const Float z(Float(a[2]).u ^ Float(b[2]).u);
        const Float w(Float(a[3]).u ^ Float(b[3]).u);
        return float32x4_set4(x, y, z, w);
    }

    static inline float32x4 bitwise_not(float32x4 a)
    {
        const Float x(~Float(a[0]).u);
        const Float y(~Float(a[1]).u);
        const Float z(~Float(a[2]).u);
        const Float w(~Float(a[3]).u);
        return float32x4_set4(x, y, z, w);
    }

    static inline float32x4 min(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = std::min(a[0], b[0]);
        v[1] = std::min(a[1], b[1]);
        v[2] = std::min(a[2], b[2]);
        v[3] = std::min(a[3], b[3]);
        return v;
    }

    static inline float32x4 max(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = std::max(a[0], b[0]);
        v[1] = std::max(a[1], b[1]);
        v[2] = std::max(a[2], b[2]);
        v[3] = std::max(a[3], b[3]);
        return v;
    }

    static inline float32x4 hmin(float32x4 a)
    {
        float l = std::min(a[0], a[1]);
        float h = std::min(a[2], a[3]);
        float s = std::min(l, h);
        return float32x4_set1(s);
    }

    static inline float32x4 hmax(float32x4 a)
    {
        float l = std::max(a[0], a[1]);
        float h = std::max(a[2], a[3]);
        float s = std::max(l, h);
        return float32x4_set1(s);
    }

    static inline float32x4 abs(float32x4 a)
    {
        float32x4 v;
        v[0] = std::abs(a[0]);
        v[1] = std::abs(a[1]);
        v[2] = std::abs(a[2]);
        v[3] = std::abs(a[3]);
        return v;
    }

    static inline float32x4 neg(float32x4 a)
    {
        return float32x4_set4(-a[0], -a[1], -a[2], -a[3]);
    }

    static inline float32x4 add(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = a[0] + b[0];
        v[1] = a[1] + b[1];
        v[2] = a[2] + b[2];
        v[3] = a[3] + b[3];
        return v;
    }

    static inline float32x4 sub(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = a[0] - b[0];
        v[1] = a[1] - b[1];
        v[2] = a[2] - b[2];
        v[3] = a[3] - b[3];
        return v;
    }

    static inline float32x4 mul(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = a[0] * b[0];
        v[1] = a[1] * b[1];
        v[2] = a[2] * b[2];
        v[3] = a[3] * b[3];
        return v;
    }

    static inline float32x4 div(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = a[0] / b[0];
        v[1] = a[1] / b[1];
        v[2] = a[2] / b[2];
        v[3] = a[3] / b[3];
        return v;
    }

    static inline float32x4 div(float32x4 a, float b)
    {
        float32x4 v;
        v[0] = a[0] / b;
        v[1] = a[1] / b;
        v[2] = a[2] / b;
        v[3] = a[3] / b;
        return v;
    }

    static inline float32x4 hadd(float32x4 a, float32x4 b)
    {
	    float32x4 v;
	    v[0] = a[0] + a[1];
	    v[1] = a[2] + a[3];
	    v[2] = b[0] + b[1];
	    v[3] = b[2] + b[3];
	    return v;
    }

    static inline float32x4 madd(float32x4 a, float32x4 b, float32x4 c)
    {
        float32x4 v;
        v[0] = a[0] + b[0] * c[0];
        v[1] = a[1] + b[1] * c[1];
        v[2] = a[2] + b[2] * c[2];
        v[3] = a[3] + b[3] * c[3];
        return v;
    }

    static inline float32x4 msub(float32x4 a, float32x4 b, float32x4 c)
    {
        float32x4 v;
        v[0] = a[0] - b[0] * c[0];
        v[1] = a[1] - b[1] * c[1];
        v[2] = a[2] - b[2] * c[2];
        v[3] = a[3] - b[3] * c[3];
        return v;
    }

    static inline float32x4 fast_rcp(float32x4 a)
    {
        float32x4 v;
        v[0] = 1.0f / a[0];
        v[1] = 1.0f / a[1];
        v[2] = 1.0f / a[2];
        v[3] = 1.0f / a[3];
        return v;
    }

    static inline float32x4 fast_rsqrt(float32x4 a)
    {
        float32x4 v;
        v[0] = 1.0f / float(std::sqrt(a[0]));
        v[1] = 1.0f / float(std::sqrt(a[1]));
        v[2] = 1.0f / float(std::sqrt(a[2]));
        v[3] = 1.0f / float(std::sqrt(a[3]));
        return v;
    }

    static inline float32x4 fast_sqrt(float32x4 a)
    {
        float32x4 v;
        v[0] = float(std::sqrt(a[0]));
        v[1] = float(std::sqrt(a[1]));
        v[2] = float(std::sqrt(a[2]));
        v[3] = float(std::sqrt(a[3]));
        return v;
    }

    static inline float32x4 rcp(float32x4 a)
    {
        return fast_rcp(a);
    }

    static inline float32x4 rsqrt(float32x4 a)
    {
        return fast_rsqrt(a);
    }

    static inline float32x4 sqrt(float32x4 a)
    {
        return fast_sqrt(a);
    }

    static inline float32x4 dot3(float32x4 a, float32x4 b)
    {
        const float s = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
        return float32x4_set1(s);
    }

    static inline float32x4 dot4(float32x4 a, float32x4 b)
    {
        const float s = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
        return float32x4_set1(s);
    }

    static inline float32x4 cross3(float32x4 a, float32x4 b)
    {
        float32x4 v;
        v[0] = a[1] * b[2] - a[2] * b[1];
        v[1] = a[2] * b[0] - a[0] * b[2];
        v[2] = a[0] * b[1] - a[1] * b[0];
        v[3] = 0.0f;
        return v;
    }

    // compare

    static inline mask32x4 compare_neq(float32x4 a, float32x4 b)
    {
        mask32x4 v = 0;
        v.mask |= uint32(a[0] != b[0]) << 0;
        v.mask |= uint32(a[1] != b[1]) << 1;
        v.mask |= uint32(a[2] != b[2]) << 2;
        v.mask |= uint32(a[3] != b[3]) << 3;
        return v;
    }

    static inline mask32x4 compare_eq(float32x4 a, float32x4 b)
    {
        mask32x4 v = 0;
        v.mask |= uint32(a[0] == b[0]) << 0;
        v.mask |= uint32(a[1] == b[1]) << 1;
        v.mask |= uint32(a[2] == b[2]) << 2;
        v.mask |= uint32(a[3] == b[3]) << 3;
        return v;
    }

    static inline mask32x4 compare_lt(float32x4 a, float32x4 b)
    {
        mask32x4 v = 0;
        v.mask |= uint32(a[0] < b[0]) << 0;
        v.mask |= uint32(a[1] < b[1]) << 1;
        v.mask |= uint32(a[2] < b[2]) << 2;
        v.mask |= uint32(a[3] < b[3]) << 3;
        return v;
    }

    static inline mask32x4 compare_le(float32x4 a, float32x4 b)
    {
        mask32x4 v = 0;
        v.mask |= uint32(a[0] <= b[0]) << 0;
        v.mask |= uint32(a[1] <= b[1]) << 1;
        v.mask |= uint32(a[2] <= b[2]) << 2;
        v.mask |= uint32(a[3] <= b[3]) << 3;
        return v;
    }

    static inline mask32x4 compare_gt(float32x4 a, float32x4 b)
    {
        mask32x4 v = 0;
        v.mask |= uint32(a[0] > b[0]) << 0;
        v.mask |= uint32(a[1] > b[1]) << 1;
        v.mask |= uint32(a[2] > b[2]) << 2;
        v.mask |= uint32(a[3] > b[3]) << 3;
        return v;
    }

    static inline mask32x4 compare_ge(float32x4 a, float32x4 b)
    {
        mask32x4 v = 0;
        v.mask |= uint32(a[0] >= b[0]) << 0;
        v.mask |= uint32(a[1] >= b[1]) << 1;
        v.mask |= uint32(a[2] >= b[2]) << 2;
        v.mask |= uint32(a[3] >= b[3]) << 3;
        return v;
    }

    static inline float32x4 select(mask32x4 mask, float32x4 a, float32x4 b)
    {
        float32x4 result;
        result[0] = mask.mask & (1 << 0) ? a[0] : b[0];
        result[1] = mask.mask & (1 << 1) ? a[1] : b[1];
        result[2] = mask.mask & (1 << 2) ? a[2] : b[2];
        result[3] = mask.mask & (1 << 3) ? a[3] : b[3];
        return result;
    }

    // rounding

    static inline float32x4 round(float32x4 s)
    {
        float32x4 v;
        v[0] = std::round(s[0]);
        v[1] = std::round(s[1]);
        v[2] = std::round(s[2]);
        v[3] = std::round(s[3]);
        return v;
    }

    static inline float32x4 trunc(float32x4 s)
    {
        float32x4 v;
        v[0] = std::trunc(s[0]);
        v[1] = std::trunc(s[1]);
        v[2] = std::trunc(s[2]);
        v[3] = std::trunc(s[3]);
        return v;
    }

    static inline float32x4 floor(float32x4 s)
    {
        float32x4 v;
        v[0] = std::floor(s[0]);
        v[1] = std::floor(s[1]);
        v[2] = std::floor(s[2]);
        v[3] = std::floor(s[3]);
        return v;
    }

    static inline float32x4 ceil(float32x4 s)
    {
        float32x4 v;
        v[0] = std::ceil(s[0]);
        v[1] = std::ceil(s[1]);
        v[2] = std::ceil(s[2]);
        v[3] = std::ceil(s[3]);
        return v;
    }

    static inline float32x4 fract(float32x4 s)
    {
        return sub(s, floor(s));
    }

} // namespace simd
} // namespace mango
