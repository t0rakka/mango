/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // ------------------------------------------------------------------
    // u8x16
    // ------------------------------------------------------------------

    static inline u8x16 clamp(u8x16 v, u8x16 vmin, u8x16 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    // ------------------------------------------------------------------
    // u16x8
    // ------------------------------------------------------------------

    static inline u16x8 clamp(u16x8 v, u16x8 vmin, u16x8 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    // ------------------------------------------------------------------
    // u32x4
    // ------------------------------------------------------------------

    static inline u32x4 set_x(u32x4 a, u32 x)
    {
        return set_component<0>(a, x);
    }

    static inline u32x4 set_y(u32x4 a, u32 y)
    {
        return set_component<1>(a, y);
    }

    static inline u32x4 set_z(u32x4 a, u32 z)
    {
        return set_component<2>(a, z);
    }

    static inline u32x4 set_w(u32x4 a, u32 w)
    {
        return set_component<3>(a, w);
    }

    static inline u32 get_x(u32x4 a)
    {
        return get_component<0>(a);
    }

    static inline u32 get_y(u32x4 a)
    {
        return get_component<1>(a);
    }

    static inline u32 get_z(u32x4 a)
    {
        return get_component<2>(a);
    }

    static inline u32 get_w(u32x4 a)
    {
        return get_component<3>(a);
    }

    static inline u32x4 splat_x(u32x4 a)
    {
        return shuffle<0, 0, 0, 0>(a);
    }

    static inline u32x4 splat_y(u32x4 a)
    {
        return shuffle<1, 1, 1, 1>(a);
    }

    static inline u32x4 splat_z(u32x4 a)
    {
        return shuffle<2, 2, 2, 2>(a);
    }

    static inline u32x4 splat_w(u32x4 a)
    {
        return shuffle<3, 3, 3, 3>(a);
    }

    static inline u32x4 clamp(u32x4 v, u32x4 vmin, u32x4 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline u32x4 clamp(u32x4 v, u32 vmin, u32 vmax)
    {
        return min(u32x4_set1(vmax), max(u32x4_set1(vmin), v));
    }

    // ------------------------------------------------------------------
    // s8x16
    // ------------------------------------------------------------------

    static inline s8x16 clamp(s8x16 v, s8x16 vmin, s8x16 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    // ------------------------------------------------------------------
    // s16x8
    // ------------------------------------------------------------------

    static inline s16x8 clamp(s16x8 v, s16x8 vmin, s16x8 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    // ------------------------------------------------------------------
    // s32x4
    // ------------------------------------------------------------------

    static inline s32x4 set_x(s32x4 a, s32 x)
    {
        return set_component<0>(a, x);
    }

    static inline s32x4 set_y(s32x4 a, s32 y)
    {
        return set_component<1>(a, y);
    }

    static inline s32x4 set_z(s32x4 a, s32 z)
    {
        return set_component<2>(a, z);
    }

    static inline s32x4 set_w(s32x4 a, s32 w)
    {
        return set_component<3>(a, w);
    }

    static inline s32 get_x(s32x4 a)
    {
        return get_component<0>(a);
    }

    static inline s32 get_y(s32x4 a)
    {
        return get_component<1>(a);
    }

    static inline s32 get_z(s32x4 a)
    {
        return get_component<2>(a);
    }

    static inline s32 get_w(s32x4 a)
    {
        return get_component<3>(a);
    }

    static inline s32x4 splat_x(s32x4 a)
    {
        return shuffle<0, 0, 0, 0>(a);
    }

    static inline s32x4 splat_y(s32x4 a)
    {
        return shuffle<1, 1, 1, 1>(a);
    }

    static inline s32x4 splat_z(s32x4 a)
    {
        return shuffle<2, 2, 2, 2>(a);
    }

    static inline s32x4 splat_w(s32x4 a)
    {
        return shuffle<3, 3, 3, 3>(a);
    }

    static inline s32x4 clamp(s32x4 v, s32x4 vmin, s32x4 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline s32x4 clamp(s32x4 v, s32 vmin, s32 vmax)
    {
        return min(s32x4_set1(vmax), max(s32x4_set1(vmin), v));
    }

    // ------------------------------------------------------------------
    // f32x4
    // ------------------------------------------------------------------

    static inline f32x4 set_x(f32x4 a, f32 x)
    {
        return set_component<0>(a, x);
    }

    static inline f32x4 set_y(f32x4 a, f32 y)
    {
        return set_component<1>(a, y);
    }

    static inline f32x4 set_z(f32x4 a, f32 z)
    {
        return set_component<2>(a, z);
    }

    static inline f32x4 set_w(f32x4 a, f32 w)
    {
        return set_component<3>(a, w);
    }

    static inline f32 get_x(f32x4 a)
    {
        return get_component<0>(a);
    }

    static inline f32 get_y(f32x4 a)
    {
        return get_component<1>(a);
    }

    static inline f32 get_z(f32x4 a)
    {
        return get_component<2>(a);
    }

    static inline f32 get_w(f32x4 a)
    {
        return get_component<3>(a);
    }

    static inline f32x4 splat_x(f32x4 a)
    {
        return shuffle<0, 0, 0, 0>(a);
    }

    static inline f32x4 splat_y(f32x4 a)
    {
        return shuffle<1, 1, 1, 1>(a);
    }

    static inline f32x4 splat_z(f32x4 a)
    {
        return shuffle<2, 2, 2, 2>(a);
    }

    static inline f32x4 splat_w(f32x4 a)
    {
        return shuffle<3, 3, 3, 3>(a);
    }

    static inline f32x4 clamp(f32x4 v, f32x4 vmin, f32x4 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline f32x4 clamp(f32x4 v, f32 vmin, f32 vmax)
    {
        return min(f32x4_set1(vmax), max(f32x4_set1(vmin), v));
    }

    static inline f32x4 mod(f32x4 a, f32x4 b)
    {
        f32x4 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline f32x4 radians(f32x4 a)
    {
        static const f32x4 s = f32x4_set1(0.01745329251f);
        return mul(a, s);
    }

    static inline f32x4 degrees(f32x4 a)
    {
        static const f32x4 s = f32x4_set1(57.2957795131f);
        return mul(a, s);
    }

    static inline f32 square(f32x4 a)
    {
        return dot4(a, a);
    }

    static inline f32 length(f32x4 a)
    {
        return f32(std::sqrt(dot4(a, a)));
    }

    static inline f32x4 normalize(f32x4 a)
    {
        f32 s = square(a);
        f32x4 v = f32x4_set1(s);
        return mul(a, rsqrt(v));
    }

    // ------------------------------------------------------------------
    // f32x8
    // ------------------------------------------------------------------

    static inline f32x8 clamp(f32x8 v, f32x8 vmin, f32x8 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline f32x8 clamp(f32x8 v, f32 vmin, f32 vmax)
    {
        return min(f32x8_set1(vmax), max(f32x8_set1(vmin), v));
    }

    static inline f32x8 mod(f32x8 a, f32x8 b)
    {
        f32x8 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline f32x8 radians(f32x8 a)
    {
        static const f32x8 s = f32x8_set1(0.01745329251f);
        return mul(a, s);
    }

    static inline f32x8 degrees(f32x8 a)
    {
        static const f32x8 s = f32x8_set1(57.2957795131f);
        return mul(a, s);
    }

    // ------------------------------------------------------------------
    // f64x2
    // ------------------------------------------------------------------

    static inline f64x2 set_x(f64x2 a, f64 x)
    {
        return set_component<0>(a, x);
    }

    static inline f64x2 set_y(f64x2 a, f64 y)
    {
        return set_component<1>(a, y);
    }

    static inline f64 get_x(f64x2 a)
    {
        return get_component<0>(a);
    }

    static inline f64 get_y(f64x2 a)
    {
        return get_component<1>(a);
    }

    static inline f64x2 splat_x(f64x2 a)
    {
        return shuffle<0, 0>(a);
    }

    static inline f64x2 splat_y(f64x2 a)
    {
        return shuffle<1, 1>(a);
    }

    static inline f64x2 clamp(f64x2 v, f64x2 vmin, f64x2 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline f64x2 clamp(f64x2 v, f64 vmin, f64 vmax)
    {
        return min(f64x2_set1(vmax), max(f64x2_set1(vmin), v));
    }

    static inline f64x2 mod(f64x2 a, f64x2 b)
    {
        f64x2 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline f64x2 radians(f64x2 a)
    {
        static const f64x2 s = f64x2_set1(0.01745329251);
        return mul(a, s);
    }

    static inline f64x2 degrees(f64x2 a)
    {
        static const f64x2 s = f64x2_set1(57.2957795131);
        return mul(a, s);
    }

    static inline f64 square(f64x2 a)
    {
        return dot2(a, a);
    }

    static inline f64 length(f64x2 a)
    {
        return std::sqrt(dot2(a, a));
    }

    static inline f64x2 normalize(f64x2 a)
    {
        f64 s = square(a);
        f64x2 v = f64x2_set1(s);
        return mul(a, rsqrt(v));
    }

    // ------------------------------------------------------------------
    // f64x4
    // ------------------------------------------------------------------

    static inline f64x4 set_x(f64x4 a, f64 x)
    {
        return set_component<0>(a, x);
    }

    static inline f64x4 set_y(f64x4 a, f64 y)
    {
        return set_component<1>(a, y);
    }

    static inline f64x4 set_z(f64x4 a, f64 z)
    {
        return set_component<2>(a, z);
    }

    static inline f64x4 set_w(f64x4 a, f64 w)
    {
        return set_component<3>(a, w);
    }

    static inline f64 get_x(f64x4 a)
    {
        return get_component<0>(a);
    }

    static inline f64 get_y(f64x4 a)
    {
        return get_component<1>(a);
    }

    static inline f64 get_z(f64x4 a)
    {
        return get_component<2>(a);
    }

    static inline f64 get_w(f64x4 a)
    {
        return get_component<3>(a);
    }

    static inline f64x4 splat_x(f64x4 a)
    {
        return shuffle<0, 0, 0, 0>(a);
    }

    static inline f64x4 splat_y(f64x4 a)
    {
        return shuffle<1, 1, 1, 1>(a);
    }

    static inline f64x4 splat_z(f64x4 a)
    {
        return shuffle<2, 2, 2, 2>(a);
    }

    static inline f64x4 splat_w(f64x4 a)
    {
        return shuffle<3, 3, 3, 3>(a);
    }

    static inline f64x4 clamp(f64x4 v, f64x4 vmin, f64x4 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline f64x4 clamp(f64x4 v, f64 vmin, f64 vmax)
    {
        return min(f64x4_set1(vmax), max(f64x4_set1(vmin), v));
    }

    static inline f64x4 mod(f64x4 a, f64x4 b)
    {
        f64x4 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline f64x4 radians(f64x4 a)
    {
        static const f64x4 s = f64x4_set1(0.01745329251);
        return mul(a, s);
    }

    static inline f64x4 degrees(f64x4 a)
    {
        static const f64x4 s = f64x4_set1(57.2957795131);
        return mul(a, s);
    }

    static inline f64 square(f64x4 a)
    {
        return dot4(a, a);
    }

    static inline f64 length(f64x4 a)
    {
        return std::sqrt(dot4(a, a));
    }

    static inline f64x4 normalize(f64x4 a)
    {
        f64 s = square(a);
        f64x4 v = f64x4_set1(s);
        return mul(a, rsqrt(v));
    }

    // ------------------------------------------------------------------
    // macros
    // ------------------------------------------------------------------

    // Hide the worst hacks here in the end. We desperately want to use
    // API that allows to the "constant integer" parameter to be passed as-if
    // the shift was a normal function. CLANG implementation for example does not
    // accept anything else so we do this immoral macro sleight-of-hand to get
    // what we want. The count still has to be a compile-time constant, of course.

    #define slli(Value, Count) slli<Count>(Value)
    #define srli(Value, Count) srli<Count>(Value)
    #define srai(Value, Count) srai<Count>(Value)

} // namespace simd
} // namespace mango
