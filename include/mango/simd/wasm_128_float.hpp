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
    static inline f32x4 shuffle(f32x4 a, f32x4 b)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return wasm_i32x4_shuffle(a, b, x, y, z + 4, w + 4);
    }

    template <u32 x, u32 y, u32 z, u32 w>
    static inline f32x4 shuffle(f32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return wasm_i32x4_shuffle(v, v, x, y, z, w);
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
        return wasm_f32x4_replace_lane(a, Index, s);
    }

    template <unsigned int Index>
    static inline f32 get_component(f32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return wasm_f32x4_extract_lane(a, Index);
    }

    static inline f32x4 f32x4_zero()
    {
        return wasm_f32x4_const_splat(0.0f);
    }

    static inline f32x4 f32x4_set(f32 s)
    {
        return wasm_f32x4_splat(s);
    }

    static inline f32x4 f32x4_set(f32 x, f32 y, f32 z, f32 w)
    {
        return wasm_f32x4_make(x, y, z, w);
    }

    static inline f32x4 f32x4_uload(const void* source)
    {
        return wasm_v128_load(source);
    }

    static inline void f32x4_ustore(void* dest, f32x4 a)
    {
        wasm_v128_store(dest, a);
    }

    static inline f32x4 movelh(f32x4 a, f32x4 b)
    {
        return wasm_i32x4_shuffle(a, b, 0, 1, 4, 5);
    }

    static inline f32x4 movehl(f32x4 a, f32x4 b)
    {
        return wasm_i32x4_shuffle(a, b, 2, 3, 6, 7);
    }

    static inline f32x4 unpacklo(f32x4 a, f32x4 b)
    {
        float a0 = get_component<0>(a);
        float a1 = get_component<1>(a);
        float b0 = get_component<0>(b);
        float b1 = get_component<1>(b);
        return f32x4_set(a0, b0, a1, b1);
    }

    static inline f32x4 unpackhi(f32x4 a, f32x4 b)
    {
        float a2 = get_component<2>(a);
        float a3 = get_component<3>(a);
        float b2 = get_component<2>(b);
        float b3 = get_component<3>(b);
        return f32x4_set(a2, b2, a3, b3);
    }

    // bitwise

    static inline f32x4 bitwise_nand(f32x4 a, f32x4 b)
    {
        return wasm_v128_andnot(a, b);
    }

    static inline f32x4 bitwise_and(f32x4 a, f32x4 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline f32x4 bitwise_or(f32x4 a, f32x4 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline f32x4 bitwise_xor(f32x4 a, f32x4 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline f32x4 bitwise_not(f32x4 a)
    {
        return wasm_v128_not(a);
    }

    static inline f32x4 min(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_min(a, b);
    }

    static inline f32x4 max(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_max(a, b);
    }

    static inline f32x4 hmin(f32x4 a)
    {
        f32x4 temp = wasm_f32x4_min(a, shuffle<2, 3, 0, 1>(a));
        return wasm_f32x4_min(temp, shuffle<1, 0, 3, 2>(temp));
    }

    static inline f32x4 hmax(f32x4 a)
    {
        f32x4 temp = wasm_f32x4_max(a, shuffle<2, 3, 0, 1>(a));
        return wasm_f32x4_max(temp, shuffle<1, 0, 3, 2>(temp));
    }

    static inline f32x4 abs(f32x4 a)
    {
        return wasm_f32x4_abs(a);
    }

    static inline f32x4 neg(f32x4 a)
    {
        return wasm_f32x4_neg(a);
    }

    static inline f32x4 sign(f32x4 a)
    {
        v128_t sign_mask = wasm_f32x4_splat(-0.0f);
        v128_t sign_bits = wasm_v128_and(a, sign_mask);
        v128_t value_mask = wasm_f32x4_ne(a, wasm_f32x4_splat(0.0f));
        v128_t value_bits = wasm_v128_and(value_mask, wasm_f32x4_splat(1.0f));
        return wasm_v128_or(value_bits, sign_bits);
    }

    static inline f32x4 add(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_add(a, b);
    }

    static inline f32x4 sub(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_sub(a, b);
    }

    static inline f32x4 mul(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_mul(a, b);
    }

    static inline f32x4 div(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_div(a, b);
    }

    static inline f32x4 div(f32x4 a, f32 b)
    {
        return wasm_f32x4_div(a, wasm_f32x4_splat(b));
    }

    static inline f32x4 hadd(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_add(wasm_i32x4_shuffle(a, b, 0, 2, 4, 6),
                              wasm_i32x4_shuffle(a, b, 1, 3, 5, 7));
    }

    static inline f32x4 hsub(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_sub(wasm_i32x4_shuffle(a, b, 0, 2, 4, 6),
                              wasm_i32x4_shuffle(a, b, 1, 3, 5, 7));
    }

    static inline f32x4 madd(f32x4 a, f32x4 b, f32x4 c)
    {
        // a + b * c
        return wasm_f32x4_add(a, wasm_f32x4_mul(b, c));
    }

    static inline f32x4 msub(f32x4 a, f32x4 b, f32x4 c)
    {
        // b * c - a
        return wasm_f32x4_sub(wasm_f32x4_mul(b, c), a);
    }

    static inline f32x4 nmadd(f32x4 a, f32x4 b, f32x4 c)
    {
        // a - b * c
        return wasm_f32x4_sub(a, wasm_f32x4_mul(b, c));
    }

    static inline f32x4 nmsub(f32x4 a, f32x4 b, f32x4 c)
    {
        // -(a + b * c)
        return wasm_f32x4_neg(wasm_f32x4_add(a, wasm_f32x4_mul(b, c)));
    }

    static inline f32x4 lerp(f32x4 a, f32x4 b, f32x4 s)
    {
        // a * (1.0 - s) + b * s
        // (a - a * s) + (b * s)
        return madd(nmadd(a, a, s), b, s);
    }

    static inline f32x4 rcp(f32x4 a)
    {
        return wasm_f32x4_div(wasm_f32x4_const_splat(1.0f), a);
    }

    static inline f32x4 rsqrt(f32x4 a)
    {
        return wasm_f32x4_div(wasm_f32x4_const_splat(1.0f), wasm_f32x4_sqrt(a));
    }

    static inline f32x4 sqrt(f32x4 a)
    {
        return wasm_f32x4_sqrt(a);
    }

    static inline f32 dot3(f32x4 a, f32x4 b)
    {
        f32x4 s = wasm_f32x4_mul(a, b);
        f32x4 x = shuffle<0, 0, 0, 0>(s);
        f32x4 y = shuffle<1, 1, 1, 1>(s);
        f32x4 z = shuffle<2, 2, 2, 2>(s);
        s = wasm_f32x4_add(x, wasm_f32x4_add(y, z));
        return get_component<0>(s);
    }

    static inline f32 dot4(f32x4 a, f32x4 b)
    {
        f32x4 s;
        s = wasm_f32x4_mul(a, b);
        s = wasm_f32x4_add(s, shuffle<2, 3, 0, 1>(s));
        s = wasm_f32x4_add(s, shuffle<1, 0, 3, 2>(s));
        return get_component<0>(s);
    }

    static inline f32x4 cross3(f32x4 a, f32x4 b)
    {
        f32x4 u = wasm_f32x4_mul(a, shuffle<1, 2, 0, 3>(b));
        f32x4 v = wasm_f32x4_mul(b, shuffle<1, 2, 0, 3>(a));
        f32x4 c = wasm_f32x4_sub(u, v);
        return shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline mask32x4 compare_neq(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_ne(a, b);
    }

    static inline mask32x4 compare_eq(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_eq(a, b);
    }

    static inline mask32x4 compare_lt(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_lt(a, b);
    }

    static inline mask32x4 compare_le(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_le(a, b);
    }

    static inline mask32x4 compare_gt(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_gt(a, b);
    }

    static inline mask32x4 compare_ge(f32x4 a, f32x4 b)
    {
        return wasm_f32x4_ge(a, b);
    }

    static inline f32x4 select(mask32x4 mask, f32x4 a, f32x4 b)
    {
        return wasm_v128_bitselect(a, b, mask);
    }

    // rounding

    static inline f32x4 round(f32x4 s)
    {
        return wasm_f32x4_nearest(s);
    }

    static inline f32x4 trunc(f32x4 s)
    {
        return wasm_f32x4_trunc(s);
    }

    static inline f32x4 floor(f32x4 s)
    {
        return wasm_f32x4_floor(s);
    }

    static inline f32x4 ceil(f32x4 s)
    {
        return wasm_f32x4_ceil(s);
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
        return wasm_i64x2_shuffle(v, v, x, y);
    }

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 a, f64x2 b)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return wasm_i64x2_shuffle(a, b, x, y + 2);
    }

    // set component

    template <unsigned int Index>
    static inline f64x2 set_component(f64x2 a, f64 s)
    {
        return wasm_f64x2_replace_lane(a, Index, s);
    }

    // get component

    template <unsigned int Index>
    static inline f64 get_component(f64x2 a)
    {
        return wasm_f64x2_extract_lane(a, Index);
    }

    static inline f64x2 f64x2_zero()
    {
        return wasm_f64x2_const_splat(0.0f);
    }

    static inline f64x2 f64x2_set(f64 s)
    {
        return wasm_f64x2_splat(s);
    }

    static inline f64x2 f64x2_set(f64 x, f64 y)
    {
        return wasm_f64x2_make(x, y);
    }

    static inline f64x2 f64x2_uload(const void* source)
    {
        return wasm_v128_load(source);
    }

    static inline void f64x2_ustore(void* dest, f64x2 a)
    {
        wasm_v128_store(dest, a);
    }

    static inline f64x2 unpackhi(f64x2 a, f64x2 b)
    {
        float a1 = get_component<1>(a);
        float b1 = get_component<1>(b);
        return f64x2_set(a1, b1);
    }

    static inline f64x2 unpacklo(f64x2 a, f64x2 b)
    {
        float a0 = get_component<0>(a);
        float b0 = get_component<0>(b);
        return f64x2_set(a0, b0);
    }

    // bitwise

    static inline f64x2 bitwise_nand(f64x2 a, f64x2 b)
    {
        return wasm_v128_andnot(a, b);
    }

    static inline f64x2 bitwise_and(f64x2 a, f64x2 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline f64x2 bitwise_or(f64x2 a, f64x2 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline f64x2 bitwise_xor(f64x2 a, f64x2 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline f64x2 bitwise_not(f64x2 a)
    {
        return wasm_v128_not(a);
    }

    static inline f64x2 min(f64x2 a, f64x2 b)
    {
        return wasm_f64x2_min(a, b);
    }

    static inline f64x2 max(f64x2 a, f64x2 b)
    {
        return wasm_f64x2_max(a, b);
    }

    static inline f64x2 abs(f64x2 a)
    {
        return wasm_f64x2_abs(a);
    }

    static inline f64x2 neg(f64x2 a)
    {
        return wasm_f64x2_neg(a);
    }

    static inline f64x2 sign(f64x2 a)
    {
        v128_t sign_mask = wasm_f64x2_splat(-0.0f);
        v128_t sign_bits = wasm_v128_and(a, sign_mask);
        v128_t value_mask = wasm_f64x2_ne(a, wasm_f64x2_splat(0.0f));
        v128_t value_bits = wasm_v128_and(value_mask, wasm_f64x2_splat(1.0f));
        return wasm_v128_or(value_bits, sign_bits);
    }

    static inline f64x2 add(f64x2 a, f64x2 b)
    {
        return wasm_f64x2_add(a, b);
    }

    static inline f64x2 sub(f64x2 a, f64x2 b)
    {
        return wasm_f64x2_sub(a, b);
    }

    static inline f64x2 mul(f64x2 a, f64x2 b)
    {
        return wasm_f64x2_mul(a, b);
    }

    static inline f64x2 div(f64x2 a, f64x2 b)
    {
        return wasm_f64x2_div(a, b);
    }

    static inline f64x2 div(f64x2 a, f64 b)
    {
        return wasm_f64x2_div(a, wasm_f64x2_splat(b));
    }

    static inline f64x2 hadd(f64x2 a, f64x2 b)
    {
        return add(unpacklo(a, b), unpackhi(a, b));
    }

    static inline f64x2 hsub(f64x2 a, f64x2 b)
    {
        return sub(unpacklo(a, b), unpackhi(a, b));
    }

    static inline f64x2 madd(f64x2 a, f64x2 b, f64x2 c)
    {
        // a + b * c
        return wasm_f64x2_add(a, wasm_f64x2_mul(b, c));
    }

    static inline f64x2 msub(f64x2 a, f64x2 b, f64x2 c)
    {
        // b * c - a
        return wasm_f64x2_sub(wasm_f64x2_mul(b, c), a);
    }

    static inline f64x2 nmadd(f64x2 a, f64x2 b, f64x2 c)
    {
        // a - b * c
        return wasm_f64x2_sub(a, wasm_f64x2_mul(b, c));
    }

    static inline f64x2 nmsub(f64x2 a, f64x2 b, f64x2 c)
    {
        // -(a + b * c)
        return wasm_f64x2_neg(wasm_f64x2_add(a, wasm_f64x2_mul(b, c)));
    }

    static inline f64x2 lerp(f64x2 a, f64x2 b, f64x2 s)
    {
        return madd(nmadd(a, a, s), b, s);
    }

    static inline f64x2 rcp(f64x2 a)
    {
        return wasm_f64x2_div(wasm_f64x2_const_splat(1.0f), a);
    }

    static inline f64x2 rsqrt(f64x2 a)
    {
        return wasm_f64x2_div(wasm_f64x2_const_splat(1.0f), wasm_f64x2_sqrt(a));
    }

    static inline f64x2 sqrt(f64x2 a)
    {
        return wasm_f64x2_sqrt(a);
    }

    static inline f64 dot2(f64x2 a, f64x2 b)
    {
        f64x2 xy = mul(a, b);
        f64x2 yx = shuffle<1, 0>(xy);
        f64x2 s = add(xy, yx);
        return get_component<0>(s);
    }

    // compare

    static inline mask64x2 compare_neq(f64x2 a, f64x2 b)
    {
        return wasm_f64x2_ne(a, b);
    }

    static inline mask64x2 compare_eq(f64x2 a, f64x2 b)
    {
        return wasm_f64x2_eq(a, b);
    }

    static inline mask64x2 compare_lt(f64x2 a, f64x2 b)
    {
        return wasm_f64x2_lt(a, b);
    }

    static inline mask64x2 compare_le(f64x2 a, f64x2 b)
    {
        return wasm_f64x2_le(a, b);
    }

    static inline mask64x2 compare_gt(f64x2 a, f64x2 b)
    {
        return wasm_f64x2_gt(a, b);
    }

    static inline mask64x2 compare_ge(f64x2 a, f64x2 b)
    {
        return wasm_f64x2_ge(a, b);
    }

    static inline f64x2 select(mask64x2 mask, f64x2 a, f64x2 b)
    {
        return wasm_v128_bitselect(a, b, mask);
    }

    // rounding

    static inline f64x2 round(f64x2 s)
    {
        return wasm_f64x2_nearest(s);
    }

    static inline f64x2 trunc(f64x2 s)
    {
        return wasm_f64x2_trunc(s);
    }

    static inline f64x2 floor(f64x2 s)
    {
        return wasm_f64x2_floor(s);
    }

    static inline f64x2 ceil(f64x2 s)
    {
        return wasm_f64x2_ceil(s);
    }

    static inline f64x2 fract(f64x2 s)
    {
        return sub(s, floor(s));
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
