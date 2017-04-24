/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // ------------------------------------------------------------------
    // Trigonometric functions
    // ------------------------------------------------------------------

    float32x4 sin(float32x4 a);
    float32x4 cos(float32x4 a);
    float32x4 tan(float32x4 a);
    float32x4 asin(float32x4 a);
    float32x4 acos(float32x4 a);
    float32x4 atan(float32x4 a);
    float32x4 exp(float32x4 a);
    float32x4 log(float32x4 a);
    float32x4 exp2(float32x4 a);
    float32x4 log2(float32x4 a);
    float32x4 pow(float32x4 a, float32x4 b);
    float32x4 atan2(float32x4 a, float32x4 b);

    float64x4 sin(float64x4 a);
    float64x4 cos(float64x4 a);
    float64x4 tan(float64x4 a);
    float64x4 asin(float64x4 a);
    float64x4 acos(float64x4 a);
    float64x4 atan(float64x4 a);
    float64x4 exp(float64x4 a);
    float64x4 log(float64x4 a);
    float64x4 exp2(float64x4 a);
    float64x4 log2(float64x4 a);
    float64x4 pow(float64x4 a, float64x4 b);
    float64x4 atan2(float64x4 a, float64x4 b);

    // ------------------------------------------------------------------
    // uint8x16
    // ------------------------------------------------------------------

    static inline uint8x16 add(uint8 a, uint8x16 b)
    {
        return add(uint8x16_set1(a), b);
    }

    static inline uint8x16 add(uint8x16 a, uint8 b)
    {
        return add(a, uint8x16_set1(b));
    }

    static inline uint8x16 sub(uint8 a, uint8x16 b)
    {
        return sub(uint8x16_set1(a), b);
    }

    static inline uint8x16 sub(uint8x16 a, uint8 b)
    {
        return sub(a, uint8x16_set1(b));
    }

#if !defined(MANGO_ENABLE_NEON) && !defined(MANGO_ENABLE_XOP)

    static inline uint8x16 bitwise_not(uint8x16 a)
    {
        return bitwise_xor(a, compare_eq(a, a));
    }

    static inline uint8x16 compare_neq(uint8x16 a, uint8x16 b)
    {
        return bitwise_not(compare_eq(b, a));
    }

    static inline uint8x16 compare_lt(uint8x16 a, uint8x16 b)
    {
        return compare_gt(b, a);
    }

    static inline uint8x16 compare_le(uint8x16 a, uint8x16 b)
    {
        return bitwise_not(compare_gt(a, b));
    }

    static inline uint8x16 compare_ge(uint8x16 a, uint8x16 b)
    {
        return bitwise_not(compare_gt(b, a));
    }

#endif // !defined(MANGO_ENABLE_NEON) && !defined(MANGO_ENABLE_XOP)

    static inline uint8x16 clamp(uint8x16 v, uint8x16 vmin, uint8x16 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    // ------------------------------------------------------------------
    // uint16x8
    // ------------------------------------------------------------------

    static inline uint16x8 add(uint16 a, uint16x8 b)
    {
        return add(uint16x8_set1(a), b);
    }

    static inline uint16x8 add(uint16x8 a, uint16 b)
    {
        return add(a, uint16x8_set1(b));
    }

    static inline uint16x8 sub(uint16 a, uint16x8 b)
    {
        return sub(uint16x8_set1(a), b);
    }

    static inline uint16x8 sub(uint16x8 a, uint16 b)
    {
        return sub(a, uint16x8_set1(b));
    }

#if !defined(MANGO_ENABLE_NEON) && !defined(MANGO_ENABLE_XOP)

    static inline uint16x8 bitwise_not(uint16x8 a)
    {
        return bitwise_xor(a, compare_eq(a, a));
    }

    static inline uint16x8 compare_neq(uint16x8 a, uint16x8 b)
    {
        return bitwise_not(compare_eq(b, a));
    }

    static inline uint16x8 compare_lt(uint16x8 a, uint16x8 b)
    {
        return compare_gt(b, a);
    }

    static inline uint16x8 compare_le(uint16x8 a, uint16x8 b)
    {
        return bitwise_not(compare_gt(a, b));
    }

    static inline uint16x8 compare_ge(uint16x8 a, uint16x8 b)
    {
        return bitwise_not(compare_gt(b, a));
    }

#endif // !defined(MANGO_ENABLE_NEON) && !defined(MANGO_ENABLE_XOP)

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

    static inline uint32x4 add(uint32 a, uint32x4 b)
    {
        return add(uint32x4_set1(a), b);
    }

    static inline uint32x4 add(uint32x4 a, uint32 b)
    {
        return add(a, uint32x4_set1(b));
    }

    static inline uint32x4 sub(uint32 a, uint32x4 b)
    {
        return sub(uint32x4_set1(a), b);
    }

    static inline uint32x4 sub(uint32x4 a, uint32 b)
    {
        return sub(a, uint32x4_set1(b));
    }

    static inline uint32x4 clamp(uint32x4 v, uint32x4 vmin, uint32x4 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline uint32x4 clamp(uint32x4 v, uint32 vmin, uint32 vmax)
    {
        return min(uint32x4_set1(vmax), max(uint32x4_set1(vmin), v));
    }

#if !defined(MANGO_ENABLE_NEON) && !defined(MANGO_ENABLE_XOP)

    static inline uint32x4 bitwise_not(uint32x4 a)
    {
        return bitwise_xor(a, compare_eq(a, a));
    }

    static inline uint32x4 compare_neq(uint32x4 a, uint32x4 b)
    {
        return bitwise_not(compare_eq(b, a));
    }

    static inline uint32x4 compare_lt(uint32x4 a, uint32x4 b)
    {
        return compare_gt(b, a);
    }

    static inline uint32x4 compare_le(uint32x4 a, uint32x4 b)
    {
        return bitwise_not(compare_gt(a, b));
    }

    static inline uint32x4 compare_ge(uint32x4 a, uint32x4 b)
    {
        return bitwise_not(compare_gt(b, a));
    }

#endif // !defined(MANGO_ENABLE_NEON) && !defined(MANGO_ENABLE_XOP)

    // ------------------------------------------------------------------
    // int8x16
    // ------------------------------------------------------------------

    static inline int8x16 add(int8 a, int8x16 b)
    {
        return add(int8x16_set1(a), b);
    }

    static inline int8x16 add(int8x16 a, int8 b)
    {
        return add(a, int8x16_set1(b));
    }

    static inline int8x16 sub(int8 a, int8x16 b)
    {
        return sub(int8x16_set1(a), b);
    }

    static inline int8x16 sub(int8x16 a, int8 b)
    {
        return sub(a, int8x16_set1(b));
    }

#if !defined(MANGO_ENABLE_NEON) && !defined(MANGO_ENABLE_XOP)

    static inline int8x16 bitwise_not(int8x16 a)
    {
        return bitwise_xor(a, compare_eq(a, a));
    }

    static inline int8x16 compare_neq(int8x16 a, int8x16 b)
    {
        return bitwise_not(compare_eq(b, a));
    }

    static inline int8x16 compare_lt(int8x16 a, int8x16 b)
    {
        return compare_gt(b, a);
    }

    static inline int8x16 compare_le(int8x16 a, int8x16 b)
    {
        return bitwise_not(compare_gt(a, b));
    }

    static inline int8x16 compare_ge(int8x16 a, int8x16 b)
    {
        return bitwise_not(compare_gt(b, a));
    }

#endif // !defined(MANGO_ENABLE_NEON) && !defined(MANGO_ENABLE_XOP)

    static inline int8x16 clamp(int8x16 v, int8x16 vmin, int8x16 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    // ------------------------------------------------------------------
    // int16x8
    // ------------------------------------------------------------------

    static inline int16x8 add(int16 a, int16x8 b)
    {
        return add(int16x8_set1(a), b);
    }

    static inline int16x8 add(int16x8 a, int16 b)
    {
        return add(a, int16x8_set1(b));
    }

    static inline int16x8 sub(int16 a, int16x8 b)
    {
        return sub(int16x8_set1(a), b);
    }

    static inline int16x8 sub(int16x8 a, int16 b)
    {
        return sub(a, int16x8_set1(b));
    }

#if !defined(MANGO_ENABLE_NEON) && !defined(MANGO_ENABLE_XOP)

    static inline int16x8 bitwise_not(int16x8 a)
    {
        return bitwise_xor(a, compare_eq(a, a));
    }

    static inline int16x8 compare_neq(int16x8 a, int16x8 b)
    {
        return bitwise_not(compare_eq(b, a));
    }

    static inline int16x8 compare_lt(int16x8 a, int16x8 b)
    {
        return compare_gt(b, a);
    }

    static inline int16x8 compare_le(int16x8 a, int16x8 b)
    {
        return bitwise_not(compare_gt(a, b));
    }

    static inline int16x8 compare_ge(int16x8 a, int16x8 b)
    {
        return bitwise_not(compare_gt(b, a));
    }

#endif // !defined(MANGO_ENABLE_NEON) && !defined(MANGO_ENABLE_XOP)

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

    static inline int32x4 add(int a, int32x4 b)
    {
        return add(int32x4_set1(a), b);
    }

    static inline int32x4 add(int32x4 a, int b)
    {
        return add(a, int32x4_set1(b));
    }

    static inline int32x4 sub(int a, int32x4 b)
    {
        return sub(int32x4_set1(a), b);
    }

    static inline int32x4 sub(int32x4 a, int b)
    {
        return sub(a, int32x4_set1(b));
    }

    static inline int32x4 bitwise_nand(int a, int32x4 b)
    {
        return bitwise_nand(int32x4_set1(a), b);
    }

    static inline int32x4 bitwise_nand(int32x4 a, int b)
    {
        return bitwise_nand(a, int32x4_set1(b));
    }

    static inline int32x4 bitwise_and(int a, int32x4 b)
    {
        return bitwise_and(int32x4_set1(a), b);
    }

    static inline int32x4 bitwise_and(int32x4 a, int b)
    {
        return bitwise_and(a, int32x4_set1(b));
    }

    static inline int32x4 bitwise_or(int a, int32x4 b)
    {
        return bitwise_or(int32x4_set1(a), b);
    }

    static inline int32x4 bitwise_or(int32x4 a, int b)
    {
        return bitwise_or(a, int32x4_set1(b));
    }

    static inline int32x4 bitwise_xor(int a, int32x4 b)
    {
        return bitwise_xor(int32x4_set1(a), b);
    }

    static inline int32x4 bitwise_xor(int32x4 a, int b)
    {
        return bitwise_xor(a, int32x4_set1(b));
    }

#if !defined(MANGO_ENABLE_NEON) && !defined(MANGO_ENABLE_XOP)

    static inline int32x4 bitwise_not(int32x4 a)
    {
        return bitwise_xor(a, compare_eq(a, a));
    }

    static inline int32x4 compare_neq(int32x4 a, int32x4 b)
    {
        return bitwise_not(compare_eq(b, a));
    }

    static inline int32x4 compare_lt(int32x4 a, int32x4 b)
    {
        return compare_gt(b, a);
    }

    static inline int32x4 compare_le(int32x4 a, int32x4 b)
    {
        return bitwise_not(compare_gt(a, b));
    }

    static inline int32x4 compare_ge(int32x4 a, int32x4 b)
    {
        return bitwise_not(compare_gt(b, a));
    }

#endif // !defined(MANGO_ENABLE_NEON) && !defined(MANGO_ENABLE_XOP)

    static inline int32x4 compare_eq(int32x4 a, int b)
    {
        return compare_eq(a, int32x4_set1(b));
    }

    static inline int32x4 compare_gt(int32x4 a, int b)
    {
        return compare_gt(a, int32x4_set1(b));
    }

    static inline int32x4 select(int32x4 mask, int a, int32x4 b)
    {
        return select(mask, int32x4_set1(a), b);
    }

    static inline int32x4 select(int32x4 mask, int32x4 a, int b)
    {
        return select(mask, a, int32x4_set1(b));
    }

    static inline int32x4 select(int32x4 mask, int a, int b)
    {
        return select(mask, int32x4_set1(a), int32x4_set1(b));
    }

    // ------------------------------------------------------------------
    // float32x2
    // ------------------------------------------------------------------

    static inline float32x2 set_x(float32x2 a, float x)
    {
        return set_component<0>(a, x);
    }

    static inline float32x2 set_y(float32x2 a, float y)
    {
        return set_component<1>(a, y);
    }

    static inline float get_x(float32x2 a)
    {
        return get_component<0>(a);
    }

    static inline float get_y(float32x2 a)
    {
        return get_component<1>(a);
    }

    static inline float32x2 splat_x(float32x2 a)
    {
        return shuffle<0, 0>(a);
    }

    static inline float32x2 splat_y(float32x2 a)
    {
        return shuffle<1, 1>(a);
    }

    static inline float32x2 add(float a, float32x2 b)
    {
        return add(float32x2_set1(a), b);
    }

    static inline float32x2 add(float32x2 a, float b)
    {
        return add(a, float32x2_set1(b));
    }

    static inline float32x2 sub(float a, float32x2 b)
    {
        return sub(float32x2_set1(a), b);
    }

    static inline float32x2 sub(float32x2 a, float b)
    {
        return sub(a, float32x2_set1(b));
    }

    static inline float32x2 mul(float32x2 a, float b)
    {
        return mul(a, float32x2_set1(b));
    }

    static inline float32x2 mul(float a, float32x2 b)
    {
        return mul(float32x2_set1(a), b);
    }

    static inline float32x2 div(float a, float32x2 b)
    {
        return div(float32x2_set1(a), b);
    }

    static inline float32x2 madd(float a, float32x2 b, float32x2 c)
    {
        return madd(float32x2_set1(a), b, c);
    }

    static inline float32x2 madd(float32x2 a, float b, float32x2 c)
    {
        return madd(a, float32x2_set1(b), c);
    }

    static inline float32x2 madd(float32x2 a, float32x2 b, float c)
    {
        return madd(a, b, float32x2_set1(c));
    }

    static inline float32x2 min(float a, float32x2 b)
    {
        return min(float32x2_set1(a), b);
    }

    static inline float32x2 min(float32x2 a, float b)
    {
        return min(a, float32x2_set1(b));
    }

    static inline float32x2 max(float a, float32x2 b)
    {
        return max(float32x2_set1(a), b);
    }

    static inline float32x2 max(float32x2 a, float b)
    {
        return max(a, float32x2_set1(b));
    }

    static inline float32x2 clamp(float32x2 v, float32x2 vmin, float32x2 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline float32x2 clamp(float32x2 v, float vmin, float vmax)
    {
        return min(float32x2_set1(vmax), max(float32x2_set1(vmin), v));
    }

    static inline float32x2 compare_neq(float32x2 a, float b)
    {
        return compare_neq(a, float32x2_set1(b));
    }

    static inline float32x2 compare_eq(float32x2 a, float b)
    {
        return compare_eq(a, float32x2_set1(b));
    }

    static inline float32x2 compare_lt(float32x2 a, float b)
    {
        return compare_lt(a, float32x2_set1(b));
    }

    static inline float32x2 compare_le(float32x2 a, float b)
    {
        return compare_le(a, float32x2_set1(b));
    }

    static inline float32x2 compare_gt(float32x2 a, float b)
    {
        return compare_gt(a, float32x2_set1(b));
    }

    static inline float32x2 compare_ge(float32x2 a, float b)
    {
        return compare_ge(a, float32x2_set1(b));
    }

    static inline float32x2 select(float32x2 mask, float a, float32x2 b)
    {
        return select(mask, float32x2_set1(a), b);
    }

    static inline float32x2 select(float32x2 mask, float32x2 a, float b)
    {
        return select(mask, a, float32x2_set1(b));
    }

    static inline float32x2 select(float32x2 mask, float a, float b)
    {
        return select(mask, float32x2_set1(a), float32x2_set1(b));
    }

    static inline float32x2 mod(float32x2 a, float32x2 b)
    {
        float32x2 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline float32x2 sign(float32x2 a)
    {
        const float32x2 zero_mask = compare_neq(a, float32x2_zero());
        const float32x2 sign_bits = bitwise_and(a, float32x2_set1(-0.0f));
        const float32x2 signed_one = bitwise_or(sign_bits, float32x2_set1(1.0f));
        return bitwise_and(signed_one, zero_mask);
    }

    static inline float32x2 radians(float32x2 a)
    {
        static const float32x2 s = float32x2_set1(0.01745329251f);
        return mul(a, s);
    }

    static inline float32x2 degrees(float32x2 a)
    {
        static const float32x2 s = float32x2_set1(57.2957795131f);
        return mul(a, s);
    }

    static inline float32x2 square(float32x2 a)
    {
        return dot2(a, a);
    }

    static inline float32x2 length(float32x2 a)
    {
        return sqrt(dot2(a, a));
    }

    static inline float32x2 normalize(float32x2 a)
    {
        return mul(a, rsqrt(dot2(a, a)));
    }

    static inline float32x2 bitwise_not(float32x2 a)
    {
        return bitwise_xor(a, compare_eq(a, a));
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

    static inline float32x4 add(float a, float32x4 b)
    {
        return add(float32x4_set1(a), b);
    }

    static inline float32x4 add(float32x4 a, float b)
    {
        return add(a, float32x4_set1(b));
    }

    static inline float32x4 sub(float a, float32x4 b)
    {
        return sub(float32x4_set1(a), b);
    }

    static inline float32x4 sub(float32x4 a, float b)
    {
        return sub(a, float32x4_set1(b));
    }

    static inline float32x4 mul(float32x4 a, float b)
    {
        return mul(a, float32x4_set1(b));
    }

    static inline float32x4 mul(float a, float32x4 b)
    {
        return mul(float32x4_set1(a), b);
    }

    static inline float32x4 div(float a, float32x4 b)
    {
        return div(float32x4_set1(a), b);
    }

    static inline float32x4 madd(float a, float32x4 b, float32x4 c)
    {
        return madd(float32x4_set1(a), b, c);
    }

    static inline float32x4 madd(float32x4 a, float b, float32x4 c)
    {
        return madd(a, float32x4_set1(b), c);
    }

    static inline float32x4 madd(float32x4 a, float32x4 b, float c)
    {
        return madd(a, b, float32x4_set1(c));
    }

    static inline float32x4 min(float a, float32x4 b)
    {
        return min(float32x4_set1(a), b);
    }

    static inline float32x4 min(float32x4 a, float b)
    {
        return min(a, float32x4_set1(b));
    }

    static inline float32x4 max(float a, float32x4 b)
    {
        return max(float32x4_set1(a), b);
    }

    static inline float32x4 max(float32x4 a, float b)
    {
        return max(a, float32x4_set1(b));
    }

    static inline float32x4 clamp(float32x4 v, float32x4 vmin, float32x4 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline float32x4 clamp(float32x4 v, float vmin, float vmax)
    {
        return min(float32x4_set1(vmax), max(float32x4_set1(vmin), v));
    }

    static inline float32x4 compare_neq(float32x4 a, float b)
    {
        return compare_neq(a, float32x4_set1(b));
    }

    static inline float32x4 compare_eq(float32x4 a, float b)
    {
        return compare_eq(a, float32x4_set1(b));
    }

    static inline float32x4 compare_lt(float32x4 a, float b)
    {
        return compare_lt(a, float32x4_set1(b));
    }

    static inline float32x4 compare_le(float32x4 a, float b)
    {
        return compare_le(a, float32x4_set1(b));
    }

    static inline float32x4 compare_gt(float32x4 a, float b)
    {
        return compare_gt(a, float32x4_set1(b));
    }

    static inline float32x4 compare_ge(float32x4 a, float b)
    {
        return compare_ge(a, float32x4_set1(b));
    }

    static inline float32x4 select(float32x4 mask, float a, float32x4 b)
    {
        return select(mask, float32x4_set1(a), b);
    }

    static inline float32x4 select(float32x4 mask, float32x4 a, float b)
    {
        return select(mask, a, float32x4_set1(b));
    }

    static inline float32x4 select(float32x4 mask, float a, float b)
    {
        return select(mask, float32x4_set1(a), float32x4_set1(b));
    }

    static inline float32x4 mod(float32x4 a, float32x4 b)
    {
        float32x4 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline float32x4 sign(float32x4 a)
    {
        const float32x4 zero_mask = compare_neq(a, float32x4_zero());
        const float32x4 sign_bits = bitwise_and(a, float32x4_set1(-0.0f));
        const float32x4 signed_one = bitwise_or(sign_bits, float32x4_set1(1.0f));
        return bitwise_and(signed_one, zero_mask);
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

    static inline float32x4 bitwise_not(float32x4 a)
    {
        return bitwise_xor(a, compare_eq(a, a));
    }

    // ------------------------------------------------------------------
    // float32x8
    // ------------------------------------------------------------------

    static inline float32x8 add(float a, float32x8 b)
    {
        return add(float32x8_set1(a), b);
    }

    static inline float32x8 add(float32x8 a, float b)
    {
        return add(a, float32x8_set1(b));
    }

    static inline float32x8 sub(float a, float32x8 b)
    {
        return sub(float32x8_set1(a), b);
    }

    static inline float32x8 sub(float32x8 a, float b)
    {
        return sub(a, float32x8_set1(b));
    }

    static inline float32x8 mul(float32x8 a, float b)
    {
        return mul(a, float32x8_set1(b));
    }

    static inline float32x8 mul(float a, float32x8 b)
    {
        return mul(float32x8_set1(a), b);
    }

    static inline float32x8 div(float a, float32x8 b)
    {
        return div(float32x8_set1(a), b);
    }

    static inline float32x8 madd(float a, float32x8 b, float32x8 c)
    {
        return madd(float32x8_set1(a), b, c);
    }

    static inline float32x8 madd(float32x8 a, float b, float32x8 c)
    {
        return madd(a, float32x8_set1(b), c);
    }

    static inline float32x8 madd(float32x8 a, float32x8 b, float c)
    {
        return madd(a, b, float32x8_set1(c));
    }

    static inline float32x8 min(float a, float32x8 b)
    {
        return min(float32x8_set1(a), b);
    }

    static inline float32x8 min(float32x8 a, float b)
    {
        return min(a, float32x8_set1(b));
    }

    static inline float32x8 max(float a, float32x8 b)
    {
        return max(float32x8_set1(a), b);
    }

    static inline float32x8 max(float32x8 a, float b)
    {
        return max(a, float32x8_set1(b));
    }

    static inline float32x8 clamp(float32x8 v, float32x8 vmin, float32x8 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline float32x8 clamp(float32x8 v, float vmin, float vmax)
    {
        return min(float32x8_set1(vmax), max(float32x8_set1(vmin), v));
    }

    static inline float32x8 compare_neq(float32x8 a, float b)
    {
        return compare_neq(a, float32x8_set1(b));
    }

    static inline float32x8 compare_eq(float32x8 a, float b)
    {
        return compare_eq(a, float32x8_set1(b));
    }

    static inline float32x8 compare_lt(float32x8 a, float b)
    {
        return compare_lt(a, float32x8_set1(b));
    }

    static inline float32x8 compare_le(float32x8 a, float b)
    {
        return compare_le(a, float32x8_set1(b));
    }

    static inline float32x8 compare_gt(float32x8 a, float b)
    {
        return compare_gt(a, float32x8_set1(b));
    }

    static inline float32x8 compare_ge(float32x8 a, float b)
    {
        return compare_ge(a, float32x8_set1(b));
    }

    static inline float32x8 select(float32x8 mask, float a, float32x8 b)
    {
        return select(mask, float32x8_set1(a), b);
    }

    static inline float32x8 select(float32x8 mask, float32x8 a, float b)
    {
        return select(mask, a, float32x8_set1(b));
    }

    static inline float32x8 select(float32x8 mask, float a, float b)
    {
        return select(mask, float32x8_set1(a), float32x8_set1(b));
    }

    static inline float32x8 mod(float32x8 a, float32x8 b)
    {
        float32x8 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline float32x8 sign(float32x8 a)
    {
        const float32x8 zero_mask = compare_neq(a, float32x8_zero());
        const float32x8 sign_bits = bitwise_and(a, float32x8_set1(-0.0f));
        const float32x8 signed_one = bitwise_or(sign_bits, float32x8_set1(1.0f));
        return bitwise_and(signed_one, zero_mask);
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

    static inline float32x8 bitwise_not(float32x8 a)
    {
        return bitwise_xor(a, compare_eq(a, a));
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

    static inline float64x2 add(double a, float64x2 b)
    {
        return add(float64x2_set1(a), b);
    }

    static inline float64x2 add(float64x2 a, double b)
    {
        return add(a, float64x2_set1(b));
    }

    static inline float64x2 sub(double a, float64x2 b)
    {
        return sub(float64x2_set1(a), b);
    }

    static inline float64x2 sub(float64x2 a, double b)
    {
        return sub(a, float64x2_set1(b));
    }

    static inline float64x2 mul(float64x2 a, double b)
    {
        return mul(a, float64x2_set1(b));
    }

    static inline float64x2 mul(double a, float64x2 b)
    {
        return mul(float64x2_set1(a), b);
    }

    static inline float64x2 div(double a, float64x2 b)
    {
        return div(float64x2_set1(a), b);
    }

    static inline float64x2 madd(double a, float64x2 b, float64x2 c)
    {
        return madd(float64x2_set1(a), b, c);
    }

    static inline float64x2 madd(float64x2 a, double b, float64x2 c)
    {
        return madd(a, float64x2_set1(b), c);
    }

    static inline float64x2 madd(float64x2 a, float64x2 b, double c)
    {
        return madd(a, b, float64x2_set1(c));
    }

    static inline float64x2 min(double a, float64x2 b)
    {
        return min(float64x2_set1(a), b);
    }

    static inline float64x2 min(float64x2 a, double b)
    {
        return min(a, float64x2_set1(b));
    }

    static inline float64x2 max(double a, float64x2 b)
    {
        return max(float64x2_set1(a), b);
    }

    static inline float64x2 max(float64x2 a, double b)
    {
        return max(a, float64x2_set1(b));
    }

    static inline float64x2 clamp(float64x2 v, float64x2 vmin, float64x2 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline float64x2 clamp(float64x2 v, double vmin, double vmax)
    {
        return min(float64x2_set1(vmax), max(float64x2_set1(vmin), v));
    }

    static inline float64x2 compare_neq(float64x2 a, double b)
    {
        return compare_neq(a, float64x2_set1(b));
    }

    static inline float64x2 compare_eq(float64x2 a, double b)
    {
        return compare_eq(a, float64x2_set1(b));
    }

    static inline float64x2 compare_lt(float64x2 a, double b)
    {
        return compare_lt(a, float64x2_set1(b));
    }

    static inline float64x2 compare_le(float64x2 a, double b)
    {
        return compare_le(a, float64x2_set1(b));
    }

    static inline float64x2 compare_gt(float64x2 a, double b)
    {
        return compare_gt(a, float64x2_set1(b));
    }

    static inline float64x2 compare_ge(float64x2 a, double b)
    {
        return compare_ge(a, float64x2_set1(b));
    }

    static inline float64x2 select(float64x2 mask, double a, float64x2 b)
    {
        return select(mask, float64x2_set1(a), b);
    }

    static inline float64x2 select(float64x2 mask, float64x2 a, double b)
    {
        return select(mask, a, float64x2_set1(b));
    }

    static inline float64x2 select(float64x2 mask, double a, double b)
    {
        return select(mask, float64x2_set1(a), float64x2_set1(b));
    }

    static inline float64x2 mod(float64x2 a, float64x2 b)
    {
        float64x2 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline float64x2 sign(float64x2 a)
    {
        const float64x2 zero_mask = compare_neq(a, float64x2_zero());
        const float64x2 sign_bits = bitwise_and(a, float64x2_set1(-0.0));
        const float64x2 signed_one = bitwise_or(sign_bits, float64x2_set1(1.0));
        return bitwise_and(signed_one, zero_mask);
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

    static inline float64x2 bitwise_not(float64x2 a)
    {
        return bitwise_xor(a, compare_eq(a, a));
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

    static inline float64x4 add(double a, float64x4 b)
    {
        return add(float64x4_set1(a), b);
    }

    static inline float64x4 add(float64x4 a, double b)
    {
        return add(a, float64x4_set1(b));
    }

    static inline float64x4 sub(double a, float64x4 b)
    {
        return sub(float64x4_set1(a), b);
    }

    static inline float64x4 sub(float64x4 a, double b)
    {
        return sub(a, float64x4_set1(b));
    }

    static inline float64x4 mul(float64x4 a, double b)
    {
        return mul(a, float64x4_set1(b));
    }

    static inline float64x4 mul(double a, float64x4 b)
    {
        return mul(float64x4_set1(a), b);
    }

    static inline float64x4 div(double a, float64x4 b)
    {
        return div(float64x4_set1(a), b);
    }

    static inline float64x4 madd(double a, float64x4 b, float64x4 c)
    {
        return madd(float64x4_set1(a), b, c);
    }

    static inline float64x4 madd(float64x4 a, double b, float64x4 c)
    {
        return madd(a, float64x4_set1(b), c);
    }

    static inline float64x4 madd(float64x4 a, float64x4 b, double c)
    {
        return madd(a, b, float64x4_set1(c));
    }

    static inline float64x4 min(double a, float64x4 b)
    {
        return min(float64x4_set1(a), b);
    }

    static inline float64x4 min(float64x4 a, double b)
    {
        return min(a, float64x4_set1(b));
    }

    static inline float64x4 max(double a, float64x4 b)
    {
        return max(float64x4_set1(a), b);
    }

    static inline float64x4 max(float64x4 a, double b)
    {
        return max(a, float64x4_set1(b));
    }

    static inline float64x4 clamp(float64x4 v, float64x4 vmin, float64x4 vmax)
    {
        return min(vmax, max(vmin, v));
    }

    static inline float64x4 clamp(float64x4 v, double vmin, double vmax)
    {
        return min(float64x4_set1(vmax), max(float64x4_set1(vmin), v));
    }

    static inline float64x4 compare_neq(float64x4 a, double b)
    {
        return compare_neq(a, float64x4_set1(b));
    }

    static inline float64x4 compare_eq(float64x4 a, double b)
    {
        return compare_eq(a, float64x4_set1(b));
    }

    static inline float64x4 compare_lt(float64x4 a, double b)
    {
        return compare_lt(a, float64x4_set1(b));
    }

    static inline float64x4 compare_le(float64x4 a, double b)
    {
        return compare_le(a, float64x4_set1(b));
    }

    static inline float64x4 compare_gt(float64x4 a, double b)
    {
        return compare_gt(a, float64x4_set1(b));
    }

    static inline float64x4 compare_ge(float64x4 a, double b)
    {
        return compare_ge(a, float64x4_set1(b));
    }

    static inline float64x4 select(float64x4 mask, double a, float64x4 b)
    {
        return select(mask, float64x4_set1(a), b);
    }

    static inline float64x4 select(float64x4 mask, float64x4 a, double b)
    {
        return select(mask, a, float64x4_set1(b));
    }

    static inline float64x4 select(float64x4 mask, double a, double b)
    {
        return select(mask, float64x4_set1(a), float64x4_set1(b));
    }

    static inline float64x4 mod(float64x4 a, float64x4 b)
    {
        float64x4 temp = floor(div(a, b));
        return sub(a, mul(b, temp));
    }

    static inline float64x4 sign(float64x4 a)
    {
        const float64x4 zero_mask = compare_neq(a, float64x4_zero());
        const float64x4 sign_bits = bitwise_and(a, float64x4_set1(-0.0));
        const float64x4 signed_one = bitwise_or(sign_bits, float64x4_set1(1.0));
        return bitwise_and(signed_one, zero_mask);
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

    static inline float64x4 bitwise_not(float64x4 a)
    {
        return bitwise_xor(a, compare_eq(a, a));
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

    #define slli(Value, Count) slli<Count>(Value)
    #define srli(Value, Count) srli<Count>(Value)
    #define srai(Value, Count) srai<Count>(Value)

} // namespace simd
} // namespace mango
