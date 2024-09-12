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
        return wasm_i32x4_shuffle(a, b, x, y, z, w);
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
        return wasm_f32x4_const_splat(s);
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

    /*
    static inline f32x4 movelh(f32x4 a, f32x4 b)
    {
        // TODO
    }

    static inline f32x4 movehl(f32x4 a, f32x4 b)
    {
        // TODO
    }

    static inline f32x4 unpackhi(f32x4 a, f32x4 b)
    {
        // TODO
    }

    static inline f32x4 unpacklo(f32x4 a, f32x4 b)
    {
        // TODO
    }
    */

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

    /*
    static inline f32x4 hmin(f32x4 a)
    {
        // TODO
    }

    static inline f32x4 hmax(f32x4 a)
    {
        // TODO
    }
    */

    static inline f32x4 abs(f32x4 a)
    {
        return wasm_f32x4_abs(a);
    }

    static inline f32x4 neg(f32x4 a)
    {
        return wasm_f32x4_neg(a);
    }

    /*
    static inline f32x4 sign(f32x4 a)
    {
        // TODO
    }
    */

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
        return wasm_f32x4_div(a, wasm_f32x4_const_splat(b));
    }

    /*
    static inline f32x4 hadd(f32x4 a, f32x4 b)
    {
        // TODO
    }

    static inline f32x4 hsub(f32x4 a, f32x4 b)
    {
        // TODO
    }
    */

    static inline f32x4 madd(f32x4 a, f32x4 b, f32x4 c)
    {
        // a + b * c
        return wasm_f32x4_relaxed_madd(a, b, c);
    }

    static inline f32x4 msub(f32x4 a, f32x4 b, f32x4 c)
    {
        // b * c - a
        return wasm_f32x4_sub(wasm_f32x4_mul(b, c), a);
    }

    static inline f32x4 nmadd(f32x4 a, f32x4 b, f32x4 c)
    {
        // a - b * c
        return wasm_f32x4_relaxed_nmadd(a, b, c);
    }

    static inline f32x4 nmsub(f32x4 a, f32x4 b, f32x4 c)
    {
        // -(a + b * c)
        return wasm_f32x4_neg(wasm_f32x4_relaxed_madd(a, b, c));
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
        return wasm_v128_bitselect(b, a, mask);
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
    // masked functions
    // -----------------------------------------------------------------

#define SIMD_ZEROMASK_FLOAT128
#define SIMD_MASK_FLOAT128
#include <mango/simd/common_mask.hpp>
#undef SIMD_ZEROMASK_FLOAT128
#undef SIMD_MASK_FLOAT128

} // namespace mango::simd
