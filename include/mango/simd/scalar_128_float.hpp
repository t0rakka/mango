/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/simd/simd.hpp>

namespace mango::simd
{

    // -----------------------------------------------------------------
    // f32x4
    // -----------------------------------------------------------------

    // shuffle

    template <u32 x, u32 y, u32 z, u32 w>
    inline f32x4 shuffle(f32x4 a, f32x4 b)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return {{ a[x], a[y], b[z], b[w] }};
    }

    template <u32 x, u32 y, u32 z, u32 w>
    inline f32x4 shuffle(f32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return {{ v[x], v[y], v[z], v[w] }};
    }

    template <>
    inline f32x4 shuffle<0, 1, 2, 3>(f32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline f32x4 set_component(f32x4 a, f32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline f32 get_component(f32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return a[Index];
    }

    static inline f32x4 f32x4_zero()
    {
        return {{ 0.0f, 0.0f, 0.0f, 0.0f }};
    }

    static inline f32x4 f32x4_set(f32 s)
    {
        return {{ s, s, s, s }};
    }

    static inline f32x4 f32x4_set(f32 x, f32 y, f32 z, f32 w)
    {
        return {{ x, y, z, w }};
    }

    static inline f32x4 f32x4_uload(const void* source)
    {
        f32x4 temp;
        std::memcpy(&temp, source, sizeof(temp));
        return temp;
    }

    static inline void f32x4_ustore(void* dest, f32x4 a)
    {
        std::memcpy(dest, &a, sizeof(a));
    }

    static inline f32x4 movelh(f32x4 a, f32x4 b)
    {
        return f32x4_set(a[0], a[1], b[0], b[1]);
    }

    static inline f32x4 movehl(f32x4 a, f32x4 b)
    {
        return f32x4_set(b[2], b[3], a[2], a[3]);
    }

    static inline f32x4 unpacklo(f32x4 a, f32x4 b)
    {
        return f32x4_set(a[0], b[0], a[1], b[1]);
    }

    static inline f32x4 unpackhi(f32x4 a, f32x4 b)
    {
        return f32x4_set(a[2], b[2], a[3], b[3]);
    }

    // bitwise

    static inline f32x4 bitwise_nand(f32x4 a, f32x4 b)
    {
        const Float x(~Float(a[0]).u & Float(b[0]).u);
        const Float y(~Float(a[1]).u & Float(b[1]).u);
        const Float z(~Float(a[2]).u & Float(b[2]).u);
        const Float w(~Float(a[3]).u & Float(b[3]).u);
        return f32x4_set(x, y, z, w);
    }

    static inline f32x4 bitwise_and(f32x4 a, f32x4 b)
    {
        const Float x(Float(a[0]).u & Float(b[0]).u);
        const Float y(Float(a[1]).u & Float(b[1]).u);
        const Float z(Float(a[2]).u & Float(b[2]).u);
        const Float w(Float(a[3]).u & Float(b[3]).u);
        return f32x4_set(x, y, z, w);
    }

    static inline f32x4 bitwise_or(f32x4 a, f32x4 b)
    {
        const Float x(Float(a[0]).u | Float(b[0]).u);
        const Float y(Float(a[1]).u | Float(b[1]).u);
        const Float z(Float(a[2]).u | Float(b[2]).u);
        const Float w(Float(a[3]).u | Float(b[3]).u);
        return f32x4_set(x, y, z, w);
    }

    static inline f32x4 bitwise_xor(f32x4 a, f32x4 b)
    {
        const Float x(Float(a[0]).u ^ Float(b[0]).u);
        const Float y(Float(a[1]).u ^ Float(b[1]).u);
        const Float z(Float(a[2]).u ^ Float(b[2]).u);
        const Float w(Float(a[3]).u ^ Float(b[3]).u);
        return f32x4_set(x, y, z, w);
    }

    static inline f32x4 bitwise_not(f32x4 a)
    {
        const Float x(~Float(a[0]).u);
        const Float y(~Float(a[1]).u);
        const Float z(~Float(a[2]).u);
        const Float w(~Float(a[3]).u);
        return f32x4_set(x, y, z, w);
    }

    static inline f32x4 min(f32x4 a, f32x4 b)
    {
        f32x4 v;
        v[0] = std::min(a[0], b[0]);
        v[1] = std::min(a[1], b[1]);
        v[2] = std::min(a[2], b[2]);
        v[3] = std::min(a[3], b[3]);
        return v;
    }

    static inline f32x4 max(f32x4 a, f32x4 b)
    {
        f32x4 v;
        v[0] = std::max(a[0], b[0]);
        v[1] = std::max(a[1], b[1]);
        v[2] = std::max(a[2], b[2]);
        v[3] = std::max(a[3], b[3]);
        return v;
    }

    static inline f32x4 hmin(f32x4 a)
    {
        f32 l = std::min(a[0], a[1]);
        f32 h = std::min(a[2], a[3]);
        f32 s = std::min(l, h);
        return f32x4_set(s);
    }

    static inline f32x4 hmax(f32x4 a)
    {
        f32 l = std::max(a[0], a[1]);
        f32 h = std::max(a[2], a[3]);
        f32 s = std::max(l, h);
        return f32x4_set(s);
    }

    static inline f32x4 abs(f32x4 a)
    {
        f32x4 v;
        v[0] = std::abs(a[0]);
        v[1] = std::abs(a[1]);
        v[2] = std::abs(a[2]);
        v[3] = std::abs(a[3]);
        return v;
    }

    static inline f32x4 neg(f32x4 a)
    {
        return f32x4_set(-a[0], -a[1], -a[2], -a[3]);
    }

    static inline f32x4 sign(f32x4 a)
    {
        f32x4 v;
        v[0] = a[0] < 0 ? -1.0f : (a[0] > 0 ? 1.0f : 0.0f);
        v[1] = a[1] < 0 ? -1.0f : (a[1] > 0 ? 1.0f : 0.0f);
        v[2] = a[2] < 0 ? -1.0f : (a[2] > 0 ? 1.0f : 0.0f);
        v[3] = a[3] < 0 ? -1.0f : (a[3] > 0 ? 1.0f : 0.0f);
        return v;
    }

    static inline f32x4 add(f32x4 a, f32x4 b)
    {
        f32x4 v;
        v[0] = a[0] + b[0];
        v[1] = a[1] + b[1];
        v[2] = a[2] + b[2];
        v[3] = a[3] + b[3];
        return v;
    }

    static inline f32x4 sub(f32x4 a, f32x4 b)
    {
        f32x4 v;
        v[0] = a[0] - b[0];
        v[1] = a[1] - b[1];
        v[2] = a[2] - b[2];
        v[3] = a[3] - b[3];
        return v;
    }

    static inline f32x4 mul(f32x4 a, f32x4 b)
    {
        f32x4 v;
        v[0] = a[0] * b[0];
        v[1] = a[1] * b[1];
        v[2] = a[2] * b[2];
        v[3] = a[3] * b[3];
        return v;
    }

    static inline f32x4 div(f32x4 a, f32x4 b)
    {
        f32x4 v;
        v[0] = a[0] / b[0];
        v[1] = a[1] / b[1];
        v[2] = a[2] / b[2];
        v[3] = a[3] / b[3];
        return v;
    }

    static inline f32x4 div(f32x4 a, f32 b)
    {
        f32x4 v;
        v[0] = a[0] / b;
        v[1] = a[1] / b;
        v[2] = a[2] / b;
        v[3] = a[3] / b;
        return v;
    }

    static inline f32x4 hadd(f32x4 a, f32x4 b)
    {
        f32x4 v;
        v[0] = a[0] + a[1];
        v[1] = a[2] + a[3];
        v[2] = b[0] + b[1];
        v[3] = b[2] + b[3];
        return v;
    }

    static inline f32x4 hsub(f32x4 a, f32x4 b)
    {
        f32x4 v;
        v[0] = a[0] - a[1];
        v[1] = a[2] - a[3];
        v[2] = b[0] - b[1];
        v[3] = b[2] - b[3];
        return v;
    }

    static inline f32x4 madd(f32x4 a, f32x4 b, f32x4 c)
    {
        // a + b * c
        f32x4 v;
        v[0] = a[0] + b[0] * c[0];
        v[1] = a[1] + b[1] * c[1];
        v[2] = a[2] + b[2] * c[2];
        v[3] = a[3] + b[3] * c[3];
        return v;
    }

    static inline f32x4 msub(f32x4 a, f32x4 b, f32x4 c)
    {
        // b * c - a
        f32x4 v;
        v[0] = b[0] * c[0] - a[0];
        v[1] = b[1] * c[1] - a[1];
        v[2] = b[2] * c[2] - a[2];
        v[3] = b[3] * c[3] - a[3];
        return v;
    }

    static inline f32x4 nmadd(f32x4 a, f32x4 b, f32x4 c)
    {
        // a - b * c
        f32x4 v;
        v[0] = a[0] - b[0] * c[0];
        v[1] = a[1] - b[1] * c[1];
        v[2] = a[2] - b[2] * c[2];
        v[3] = a[3] - b[3] * c[3];
        return v;
    }

    static inline f32x4 nmsub(f32x4 a, f32x4 b, f32x4 c)
    {
        // -(a + b * c)
        f32x4 v;
        v[0] = -(a[0] + b[0] * c[0]);
        v[1] = -(a[1] + b[1] * c[1]);
        v[2] = -(a[2] + b[2] * c[2]);
        v[3] = -(a[3] + b[3] * c[3]);
        return v;
    }

    static inline f32x4 lerp(f32x4 a, f32x4 b, f32x4 s)
    {
        // a * (1.0 - s) + b * s
        // (a - a * s) + (b * s)
        return madd(nmadd(a, a, s), b, s);
    }

    static inline f32x4 rcp(f32x4 a)
    {
        f32x4 v;
        v[0] = 1.0f / a[0];
        v[1] = 1.0f / a[1];
        v[2] = 1.0f / a[2];
        v[3] = 1.0f / a[3];
        return v;
    }

    static inline f32x4 rsqrt(f32x4 a)
    {
        f32x4 v;
        v[0] = 1.0f / f32(std::sqrt(a[0]));
        v[1] = 1.0f / f32(std::sqrt(a[1]));
        v[2] = 1.0f / f32(std::sqrt(a[2]));
        v[3] = 1.0f / f32(std::sqrt(a[3]));
        return v;
    }

    static inline f32x4 sqrt(f32x4 a)
    {
        f32x4 v;
        v[0] = f32(std::sqrt(a[0]));
        v[1] = f32(std::sqrt(a[1]));
        v[2] = f32(std::sqrt(a[2]));
        v[3] = f32(std::sqrt(a[3]));
        return v;
    }

    static inline f32 dot3(f32x4 a, f32x4 b)
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

    static inline f32 dot4(f32x4 a, f32x4 b)
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    }

    static inline f32x4 cross3(f32x4 a, f32x4 b)
    {
        f32x4 v;
        v[0] = a[1] * b[2] - a[2] * b[1];
        v[1] = a[2] * b[0] - a[0] * b[2];
        v[2] = a[0] * b[1] - a[1] * b[0];
        v[3] = 0.0f;
        return v;
    }

    // compare

    static inline mask32x4 compare_neq(f32x4 a, f32x4 b)
    {
        mask32x4 v = 0;
        v.mask |= u32(a[0] != b[0]) << 0;
        v.mask |= u32(a[1] != b[1]) << 1;
        v.mask |= u32(a[2] != b[2]) << 2;
        v.mask |= u32(a[3] != b[3]) << 3;
        return v;
    }

    static inline mask32x4 compare_eq(f32x4 a, f32x4 b)
    {
        mask32x4 v = 0;
        v.mask |= u32(a[0] == b[0]) << 0;
        v.mask |= u32(a[1] == b[1]) << 1;
        v.mask |= u32(a[2] == b[2]) << 2;
        v.mask |= u32(a[3] == b[3]) << 3;
        return v;
    }

    static inline mask32x4 compare_lt(f32x4 a, f32x4 b)
    {
        mask32x4 v = 0;
        v.mask |= u32(a[0] < b[0]) << 0;
        v.mask |= u32(a[1] < b[1]) << 1;
        v.mask |= u32(a[2] < b[2]) << 2;
        v.mask |= u32(a[3] < b[3]) << 3;
        return v;
    }

    static inline mask32x4 compare_le(f32x4 a, f32x4 b)
    {
        mask32x4 v = 0;
        v.mask |= u32(a[0] <= b[0]) << 0;
        v.mask |= u32(a[1] <= b[1]) << 1;
        v.mask |= u32(a[2] <= b[2]) << 2;
        v.mask |= u32(a[3] <= b[3]) << 3;
        return v;
    }

    static inline mask32x4 compare_gt(f32x4 a, f32x4 b)
    {
        mask32x4 v = 0;
        v.mask |= u32(a[0] > b[0]) << 0;
        v.mask |= u32(a[1] > b[1]) << 1;
        v.mask |= u32(a[2] > b[2]) << 2;
        v.mask |= u32(a[3] > b[3]) << 3;
        return v;
    }

    static inline mask32x4 compare_ge(f32x4 a, f32x4 b)
    {
        mask32x4 v = 0;
        v.mask |= u32(a[0] >= b[0]) << 0;
        v.mask |= u32(a[1] >= b[1]) << 1;
        v.mask |= u32(a[2] >= b[2]) << 2;
        v.mask |= u32(a[3] >= b[3]) << 3;
        return v;
    }

    static inline f32x4 select(mask32x4 mask, f32x4 a, f32x4 b)
    {
        f32x4 result;
        result[0] = mask.mask & (1 << 0) ? a[0] : b[0];
        result[1] = mask.mask & (1 << 1) ? a[1] : b[1];
        result[2] = mask.mask & (1 << 2) ? a[2] : b[2];
        result[3] = mask.mask & (1 << 3) ? a[3] : b[3];
        return result;
    }

    // rounding

    static inline f32x4 round(f32x4 s)
    {
        f32x4 v;
        v[0] = std::round(s[0]);
        v[1] = std::round(s[1]);
        v[2] = std::round(s[2]);
        v[3] = std::round(s[3]);
        return v;
    }

    static inline f32x4 trunc(f32x4 s)
    {
        f32x4 v;
        v[0] = std::trunc(s[0]);
        v[1] = std::trunc(s[1]);
        v[2] = std::trunc(s[2]);
        v[3] = std::trunc(s[3]);
        return v;
    }

    static inline f32x4 floor(f32x4 s)
    {
        f32x4 v;
        v[0] = std::floor(s[0]);
        v[1] = std::floor(s[1]);
        v[2] = std::floor(s[2]);
        v[3] = std::floor(s[3]);
        return v;
    }

    static inline f32x4 ceil(f32x4 s)
    {
        f32x4 v;
        v[0] = std::ceil(s[0]);
        v[1] = std::ceil(s[1]);
        v[2] = std::ceil(s[2]);
        v[3] = std::ceil(s[3]);
        return v;
    }

    static inline f32x4 fract(f32x4 s)
    {
        return sub(s, floor(s));
    }

    // -----------------------------------------------------------------
    // f64x2
    // -----------------------------------------------------------------

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 v)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return {{ v[x], v[y] }};
    }

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 a, f64x2 b)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return {{ a[x], b[y] }};
    }

    // indexed access

    template <unsigned int Index>
    static inline f64x2 set_component(f64x2 a, f64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline f64 get_component(f64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return a[Index];
    }

    static inline f64x2 f64x2_zero()
    {
        return {{ 0.0, 0.0 }};
    }

    static inline f64x2 f64x2_set(f64 s)
    {
        return {{ s, s }};
    }

    static inline f64x2 f64x2_set(f64 x, f64 y)
    {
        return {{ x, y }};
    }

    static inline f64x2 f64x2_uload(const void* source)
    {
        f64x2 temp;
        std::memcpy(&temp, source, sizeof(temp));
        return temp;
    }

    static inline void f64x2_ustore(void* dest, f64x2 a)
    {
        std::memcpy(dest, &a, sizeof(a));
    }

    static inline f64x2 unpacklo(f64x2 a, f64x2 b)
    {
        return f64x2_set(a[0], b[0]);
    }

    static inline f64x2 unpackhi(f64x2 a, f64x2 b)
    {
        return f64x2_set(a[1], b[1]);
    }

    // bitwise

    static inline f64x2 bitwise_nand(f64x2 a, f64x2 b)
    {
        const Double x(~Double(a[0]).u & Double(b[0]).u);
        const Double y(~Double(a[1]).u & Double(b[1]).u);
        return f64x2_set(x, y);
    }

    static inline f64x2 bitwise_and(f64x2 a, f64x2 b)
    {
        const Double x(Double(a[0]).u & Double(b[0]).u);
        const Double y(Double(a[1]).u & Double(b[1]).u);
        return f64x2_set(x, y);
    }

    static inline f64x2 bitwise_or(f64x2 a, f64x2 b)
    {
        const Double x(Double(a[0]).u | Double(b[0]).u);
        const Double y(Double(a[1]).u | Double(b[1]).u);
        return f64x2_set(x, y);
    }

    static inline f64x2 bitwise_xor(f64x2 a, f64x2 b)
    {
        const Double x(Double(a[0]).u ^ Double(b[0]).u);
        const Double y(Double(a[1]).u ^ Double(b[1]).u);
        return f64x2_set(x, y);
    }

    static inline f64x2 bitwise_not(f64x2 a)
    {
        const Double x(~Double(a[0]).u);
        const Double y(~Double(a[1]).u);
        return f64x2_set(x, y);
    }

    static inline f64x2 min(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v[0] = std::min(a[0], b[0]);
        v[1] = std::min(a[1], b[1]);
        return v;
    }

    static inline f64x2 max(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v[0] = std::max(a[0], b[0]);
        v[1] = std::max(a[1], b[1]);
        return v;
    }

    static inline f64x2 abs(f64x2 a)
    {
        f64x2 v;
        v[0] = std::abs(a[0]);
        v[1] = std::abs(a[1]);
        return v;
    }

    static inline f64x2 neg(f64x2 a)
    {
        return f64x2_set(-a[0], -a[1]);
    }

    static inline f64x2 sign(f64x2 a)
    {
        f64x2 v;
        v[0] = a[0] < 0 ? -1.0 : (a[0] > 0 ? 1.0 : 0.0);
        v[1] = a[1] < 0 ? -1.0 : (a[1] > 0 ? 1.0 : 0.0);
        return v;
    }

    static inline f64x2 add(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v[0] = a[0] + b[0];
        v[1] = a[1] + b[1];
        return v;
    }

    static inline f64x2 sub(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v[0] = a[0] - b[0];
        v[1] = a[1] - b[1];
        return v;
    }

    static inline f64x2 mul(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v[0] = a[0] * b[0];
        v[1] = a[1] * b[1];
        return v;
    }

    static inline f64x2 div(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v[0] = a[0] / b[0];
        v[1] = a[1] / b[1];
        return v;
    }

    static inline f64x2 div(f64x2 a, f64 b)
    {
        f64x2 v;
        v[0] = a[0] / b;
        v[1] = a[1] / b;
        return v;
    }

    static inline f64x2 hadd(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v[0] = a[0] + a[1];
        v[1] = b[0] + b[1];
        return v;
    }

    static inline f64x2 hsub(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v[0] = a[0] - a[1];
        v[1] = b[0] - b[1];
        return v;
    }

    static inline f64x2 madd(f64x2 a, f64x2 b, f64x2 c)
    {
        // a + b * c
        f64x2 v;
        v[0] = a[0] + b[0] * c[0];
        v[1] = a[1] + b[1] * c[1];
        return v;
    }

    static inline f64x2 msub(f64x2 a, f64x2 b, f64x2 c)
    {
        // b * c - a
        f64x2 v;
        v[0] = b[0] * c[0] - a[0];
        v[1] = b[1] * c[1] - a[1];
        return v;
    }

    static inline f64x2 nmadd(f64x2 a, f64x2 b, f64x2 c)
    {
        // a - b * c
        f64x2 v;
        v[0] = a[0] - b[0] * c[0];
        v[1] = a[1] - b[1] * c[1];
        return v;
    }

    static inline f64x2 nmsub(f64x2 a, f64x2 b, f64x2 c)
    {
        // -(a + b * c)
        f64x2 v;
        v[0] = -(a[0] + b[0] * c[0]);
        v[1] = -(a[1] + b[1] * c[1]);
        return v;
    }

    static inline f64x2 lerp(f64x2 a, f64x2 b, f64x2 s)
    {
        // a * (1.0 - s) + b * s
        // (a - a * s) + (b * s)
        return madd(nmadd(a, a, s), b, s);
    }

    static inline f64x2 rcp(f64x2 a)
    {
        f64x2 v;
        v[0] = 1.0 / a[0];
        v[1] = 1.0 / a[1];
        return v;
    }

    static inline f64x2 rsqrt(f64x2 a)
    {
        f64x2 v;
        v[0] = 1.0 / std::sqrt(a[0]);
        v[1] = 1.0 / std::sqrt(a[1]);
        return v;
    }

    static inline f64x2 sqrt(f64x2 a)
    {
        f64x2 v;
        v[0] = std::sqrt(a[0]);
        v[1] = std::sqrt(a[1]);
        return v;
    }

    static inline f64 dot2(f64x2 a, f64x2 b)
    {
        return a[0] * b[0] + a[1] * b[1];
    }

    // compare

    static inline mask64x2 compare_neq(f64x2 a, f64x2 b)
    {
        mask64x2 v = 0;
        v.mask |= u32(a[0] != b[0]) << 0;
        v.mask |= u32(a[1] != b[1]) << 1;
        return v;
    }

    static inline mask64x2 compare_eq(f64x2 a, f64x2 b)
    {
        mask64x2 v = 0;
        v.mask |= u32(a[0] == b[0]) << 0;
        v.mask |= u32(a[1] == b[1]) << 1;
        return v;
    }

    static inline mask64x2 compare_lt(f64x2 a, f64x2 b)
    {
        mask64x2 v = 0;
        v.mask |= u32(a[0] < b[0]) << 0;
        v.mask |= u32(a[1] < b[1]) << 1;
        return v;
    }

    static inline mask64x2 compare_le(f64x2 a, f64x2 b)
    {
        mask64x2 v = 0;
        v.mask |= u32(a[0] <= b[0]) << 0;
        v.mask |= u32(a[1] <= b[1]) << 1;
        return v;
    }

    static inline mask64x2 compare_gt(f64x2 a, f64x2 b)
    {
        mask64x2 v = 0;
        v.mask |= u32(a[0] > b[0]) << 0;
        v.mask |= u32(a[1] > b[1]) << 1;
        return v;
    }

    static inline mask64x2 compare_ge(f64x2 a, f64x2 b)
    {
        mask64x2 v = 0;
        v.mask |= u32(a[0] >= b[0]) << 0;
        v.mask |= u32(a[1] >= b[1]) << 1;
        return v;
    }

    static inline f64x2 select(mask64x2 mask, f64x2 a, f64x2 b)
    {
        f64x2 result;
        result[0] = mask.mask & (1 << 0) ? a[0] : b[0];
        result[1] = mask.mask & (1 << 1) ? a[1] : b[1];
        return result;
    }

    // rounding

    static inline f64x2 round(f64x2 s)
    {
        f64x2 v;
        v[0] = std::round(s[0]);
        v[1] = std::round(s[1]);
        return v;
    }

    static inline f64x2 trunc(f64x2 s)
    {
        f64x2 v;
        v[0] = std::trunc(s[0]);
        v[1] = std::trunc(s[1]);
        return v;
    }

    static inline f64x2 floor(f64x2 s)
    {
        f64x2 v;
        v[0] = std::floor(s[0]);
        v[1] = std::floor(s[1]);
        return v;
    }

    static inline f64x2 ceil(f64x2 s)
    {
        f64x2 v;
        v[0] = std::ceil(s[0]);
        v[1] = std::ceil(s[1]);
        return v;
    }

    static inline f64x2 fract(f64x2 s)
    {
        f64x2 v;
        v[0] = s[0] - std::floor(s[0]);
        v[1] = s[1] - std::floor(s[1]);
        return v;
    }

    // -----------------------------------------------------------------
    // masked functions
    // -----------------------------------------------------------------

#define SIMD_ZEROMASK_FLOAT128
#define SIMD_ZEROMASK_DOUBLE128
#define SIMD_MASK_FLOAT128
#define SIMD_MASK_DOUBLE128
#include <mango/simd/common_mask.hpp>
#undef SIMD_ZEROMASK_FLOAT128
#undef SIMD_ZEROMASK_DOUBLE128
#undef SIMD_MASK_FLOAT128
#undef SIMD_MASK_DOUBLE128

} // namespace mango::simd
