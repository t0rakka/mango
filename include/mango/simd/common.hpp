/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // ------------------------------------------------------------------
    // uint8x16
    // ------------------------------------------------------------------

    static inline uint8x16 clamp(uint8x16 v, uint8x16 vmin, uint8x16 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    // ------------------------------------------------------------------
    // uint16x8
    // ------------------------------------------------------------------

    static inline uint16x8 clamp(uint16x8 v, uint16x8 vmin, uint16x8 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    // ------------------------------------------------------------------
    // uint32x4
    // ------------------------------------------------------------------

    static inline uint32x4 set_x(uint32x4 a, uint32 x)
    {
        return set_component<0>(a, x);
    }

    static inline uint32x4 set_y(uint32x4 a, uint32 y)
    {
        return set_component<1>(a, y);
    }

    static inline uint32x4 set_z(uint32x4 a, uint32 z)
    {
        return set_component<2>(a, z);
    }

    static inline uint32x4 set_w(uint32x4 a, uint32 w)
    {
        return set_component<3>(a, w);
    }

    static inline uint32 get_x(uint32x4 a)
    {
        return get_component<0>(a);
    }

    static inline uint32 get_y(uint32x4 a)
    {
        return get_component<1>(a);
    }

    static inline uint32 get_z(uint32x4 a)
    {
        return get_component<2>(a);
    }

    static inline uint32 get_w(uint32x4 a)
    {
        return get_component<3>(a);
    }

    static inline uint32x4 splat_x(uint32x4 a)
    {
        return shuffle<0, 0, 0, 0>(a);
    }

    static inline uint32x4 splat_y(uint32x4 a)
    {
        return shuffle<1, 1, 1, 1>(a);
    }

    static inline uint32x4 splat_z(uint32x4 a)
    {
        return shuffle<2, 2, 2, 2>(a);
    }

    static inline uint32x4 splat_w(uint32x4 a)
    {
        return shuffle<3, 3, 3, 3>(a);
    }

    static inline uint32x4 clamp(uint32x4 v, uint32x4 vmin, uint32x4 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline uint32x4 clamp(uint32x4 v, uint32 vmin, uint32 vmax)
    {
        return min(uint32x4_set1(vmax), max(uint32x4_set1(vmin), v));
    }

    // ------------------------------------------------------------------
    // int8x16
    // ------------------------------------------------------------------

    static inline int8x16 clamp(int8x16 v, int8x16 vmin, int8x16 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    // ------------------------------------------------------------------
    // int16x8
    // ------------------------------------------------------------------

    static inline int16x8 clamp(int16x8 v, int16x8 vmin, int16x8 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    // ------------------------------------------------------------------
    // int32x4
    // ------------------------------------------------------------------

    static inline int32x4 set_x(int32x4 a, int32 x)
    {
        return set_component<0>(a, x);
    }

    static inline int32x4 set_y(int32x4 a, int32 y)
    {
        return set_component<1>(a, y);
    }

    static inline int32x4 set_z(int32x4 a, int32 z)
    {
        return set_component<2>(a, z);
    }

    static inline int32x4 set_w(int32x4 a, int32 w)
    {
        return set_component<3>(a, w);
    }

    static inline int32 get_x(int32x4 a)
    {
        return get_component<0>(a);
    }

    static inline int32 get_y(int32x4 a)
    {
        return get_component<1>(a);
    }

    static inline int32 get_z(int32x4 a)
    {
        return get_component<2>(a);
    }

    static inline int32 get_w(int32x4 a)
    {
        return get_component<3>(a);
    }

    static inline int32x4 splat_x(int32x4 a)
    {
        return shuffle<0, 0, 0, 0>(a);
    }

    static inline int32x4 splat_y(int32x4 a)
    {
        return shuffle<1, 1, 1, 1>(a);
    }

    static inline int32x4 splat_z(int32x4 a)
    {
        return shuffle<2, 2, 2, 2>(a);
    }

    static inline int32x4 splat_w(int32x4 a)
    {
        return shuffle<3, 3, 3, 3>(a);
    }

    static inline int32x4 clamp(int32x4 v, int32x4 vmin, int32x4 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline int32x4 clamp(int32x4 v, int vmin, int vmax)
    {
        return min(int32x4_set1(vmax), max(int32x4_set1(vmin), v));
    }

    // ------------------------------------------------------------------
    // float32x4
    // ------------------------------------------------------------------

    static inline float32x4 set_x(float32x4 a, float x)
    {
        return set_component<0>(a, x);
    }

    static inline float32x4 set_y(float32x4 a, float y)
    {
        return set_component<1>(a, y);
    }

    static inline float32x4 set_z(float32x4 a, float z)
    {
        return set_component<2>(a, z);
    }

    static inline float32x4 set_w(float32x4 a, float w)
    {
        return set_component<3>(a, w);
    }

    static inline float get_x(float32x4 a)
    {
        return get_component<0>(a);
    }

    static inline float get_y(float32x4 a)
    {
        return get_component<1>(a);
    }

    static inline float get_z(float32x4 a)
    {
        return get_component<2>(a);
    }

    static inline float get_w(float32x4 a)
    {
        return get_component<3>(a);
    }

    static inline float32x4 splat_x(float32x4 a)
    {
        return shuffle<0, 0, 0, 0>(a);
    }

    static inline float32x4 splat_y(float32x4 a)
    {
        return shuffle<1, 1, 1, 1>(a);
    }

    static inline float32x4 splat_z(float32x4 a)
    {
        return shuffle<2, 2, 2, 2>(a);
    }

    static inline float32x4 splat_w(float32x4 a)
    {
        return shuffle<3, 3, 3, 3>(a);
    }

    static inline float32x4 clamp(float32x4 v, float32x4 vmin, float32x4 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline float32x4 clamp(float32x4 v, float vmin, float vmax)
    {
        return min(float32x4_set1(vmax), max(float32x4_set1(vmin), v));
    }

    static inline float32x4 mod(float32x4 a, float32x4 b)
    {
        float32x4 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline float32x4 radians(float32x4 a)
    {
        static const float32x4 s = float32x4_set1(0.01745329251f);
        return mul(a, s);
    }

    static inline float32x4 degrees(float32x4 a)
    {
        static const float32x4 s = float32x4_set1(57.2957795131f);
        return mul(a, s);
    }

    static inline float32x4 square(float32x4 a)
    {
        return dot4(a, a);
    }

    static inline float32x4 length(float32x4 a)
    {
        return sqrt(dot4(a, a));
    }

    static inline float32x4 normalize(float32x4 a)
    {
        return mul(a, rsqrt(dot4(a, a)));
    }

    // ------------------------------------------------------------------
    // float32x8
    // ------------------------------------------------------------------

    static inline float32x8 clamp(float32x8 v, float32x8 vmin, float32x8 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline float32x8 clamp(float32x8 v, float vmin, float vmax)
    {
        return min(float32x8_set1(vmax), max(float32x8_set1(vmin), v));
    }

    static inline float32x8 mod(float32x8 a, float32x8 b)
    {
        float32x8 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline float32x8 radians(float32x8 a)
    {
        static const float32x8 s = float32x8_set1(0.01745329251f);
        return mul(a, s);
    }

    static inline float32x8 degrees(float32x8 a)
    {
        static const float32x8 s = float32x8_set1(57.2957795131f);
        return mul(a, s);
    }

    // ------------------------------------------------------------------
    // float64x2
    // ------------------------------------------------------------------

    static inline float64x2 set_x(float64x2 a, double x)
    {
        return set_component<0>(a, x);
    }

    static inline float64x2 set_y(float64x2 a, double y)
    {
        return set_component<1>(a, y);
    }

    static inline double get_x(float64x2 a)
    {
        return get_component<0>(a);
    }

    static inline double get_y(float64x2 a)
    {
        return get_component<1>(a);
    }

    static inline float64x2 splat_x(float64x2 a)
    {
        return shuffle<0, 0>(a);
    }

    static inline float64x2 splat_y(float64x2 a)
    {
        return shuffle<1, 1>(a);
    }

    static inline float64x2 clamp(float64x2 v, float64x2 vmin, float64x2 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline float64x2 clamp(float64x2 v, double vmin, double vmax)
    {
        return min(float64x2_set1(vmax), max(float64x2_set1(vmin), v));
    }

    static inline float64x2 mod(float64x2 a, float64x2 b)
    {
        float64x2 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline float64x2 radians(float64x2 a)
    {
        static const float64x2 s = float64x2_set1(0.01745329251);
        return mul(a, s);
    }

    static inline float64x2 degrees(float64x2 a)
    {
        static const float64x2 s = float64x2_set1(57.2957795131);
        return mul(a, s);
    }

    static inline float64x2 square(float64x2 a)
    {
        return dot2(a, a);
    }

    static inline float64x2 length(float64x2 a)
    {
        return sqrt(dot2(a, a));
    }

    static inline float64x2 normalize(float64x2 a)
    {
        return mul(a, rsqrt(dot2(a, a)));
    }

    // ------------------------------------------------------------------
    // float64x4
    // ------------------------------------------------------------------

    static inline float64x4 set_x(float64x4 a, double x)
    {
        return set_component<0>(a, x);
    }

    static inline float64x4 set_y(float64x4 a, double y)
    {
        return set_component<1>(a, y);
    }

    static inline float64x4 set_z(float64x4 a, double z)
    {
        return set_component<2>(a, z);
    }

    static inline float64x4 set_w(float64x4 a, double w)
    {
        return set_component<3>(a, w);
    }

    static inline double get_x(float64x4 a)
    {
        return get_component<0>(a);
    }

    static inline double get_y(float64x4 a)
    {
        return get_component<1>(a);
    }

    static inline double get_z(float64x4 a)
    {
        return get_component<2>(a);
    }

    static inline double get_w(float64x4 a)
    {
        return get_component<3>(a);
    }

    static inline float64x4 splat_x(float64x4 a)
    {
        return shuffle<0, 0, 0, 0>(a);
    }

    static inline float64x4 splat_y(float64x4 a)
    {
        return shuffle<1, 1, 1, 1>(a);
    }

    static inline float64x4 splat_z(float64x4 a)
    {
        return shuffle<2, 2, 2, 2>(a);
    }

    static inline float64x4 splat_w(float64x4 a)
    {
        return shuffle<3, 3, 3, 3>(a);
    }

    static inline float64x4 clamp(float64x4 v, float64x4 vmin, float64x4 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline float64x4 clamp(float64x4 v, double vmin, double vmax)
    {
        return min(float64x4_set1(vmax), max(float64x4_set1(vmin), v));
    }

    static inline float64x4 mod(float64x4 a, float64x4 b)
    {
        float64x4 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline float64x4 radians(float64x4 a)
    {
        static const float64x4 s = float64x4_set1(0.01745329251);
        return mul(a, s);
    }

    static inline float64x4 degrees(float64x4 a)
    {
        static const float64x4 s = float64x4_set1(57.2957795131);
        return mul(a, s);
    }

    static inline float64x4 square(float64x4 a)
    {
        return dot4(a, a);
    }

    static inline float64x4 length(float64x4 a)
    {
        return sqrt(dot4(a, a));
    }

    static inline float64x4 normalize(float64x4 a)
    {
        return mul(a, rsqrt(dot4(a, a)));
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
