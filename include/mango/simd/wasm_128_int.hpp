/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/simd/simd.hpp>

namespace mango::simd
{

    // -----------------------------------------------------------------
    // u8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u8x16 set_component(u8x16 a, u8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return wasm_u8x16_replace_lane(a, Index, s);
    }

    template <unsigned int Index>
    static inline u8 get_component(u8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return wasm_i8x16_extract_lane(a, Index);
    }

    static inline u8x16 u8x16_zero()
    {
        return wasm_u8x16_splat(0);
    }

    static inline u8x16 u8x16_set(u8 s)
    {
        return wasm_u8x16_splat(s);
    }

    static inline u8x16 u8x16_set(
        u8 v00, u8 v01, u8 v02, u8 v03, u8 v04, u8 v05, u8 v06, u8 v07,
        u8 v08, u8 v09, u8 v10, u8 v11, u8 v12, u8 v13, u8 v14, u8 v15)
    {
        return wasm_u8x16_make(v00, v01, v02, v03, v04, v05, v06, v07,
                               v08, v09, v10, v11, v12, v13, v14, v15);
    }

    static inline u8x16 u8x16_uload(const void* source)
    {
        return wasm_v128_load(source);
    }

    static inline void u8x16_ustore(void* dest, u8x16 a)
    {
        wasm_v128_store(dest, a);
    }

    static inline u8x16 u8x16_load_low(const u8* source)
    {
        return wasm_v128_load64_zero(source);
    }

    static inline void u8x16_store_low(u8* dest, u8x16 a)
    {
        wasm_v128_store64_lane(dest, a, 0);
    }

    static inline u8x16 unpacklo(u8x16 a, u8x16 b)
    {
        return wasm_i8x16_shuffle(a, b, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);
    }

    static inline u8x16 unpackhi(u8x16 a, u8x16 b)
    {
        return wasm_i8x16_shuffle(a, b, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31);
    }

    static inline u8x16 add(u8x16 a, u8x16 b)
    {
        // NOTE: same instruction is used for signed and unsigned
        return wasm_i8x16_add(a, b);
    }

    static inline u8x16 sub(u8x16 a, u8x16 b)
    {
        // NOTE: same instruction is used for signed and unsigned
        return wasm_i8x16_sub(a, b);
    }

    static inline u8x16 adds(u8x16 a, u8x16 b)
    {
        return wasm_u8x16_add_sat(a, b);
    }

    static inline u8x16 subs(u8x16 a, u8x16 b)
    {
        return wasm_u8x16_sub_sat(a, b);
    }

    static inline u8x16 avg(u8x16 a, u8x16 b)
    {
        return wasm_u8x16_avgr(a, b);
    }

    // bitwise

    static inline u8x16 bitwise_nand(u8x16 a, u8x16 b)
    {
        return wasm_v128_andnot(b, a);
    }

    static inline u8x16 bitwise_and(u8x16 a, u8x16 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline u8x16 bitwise_or(u8x16 a, u8x16 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline u8x16 bitwise_xor(u8x16 a, u8x16 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline u8x16 bitwise_not(u8x16 a)
    {
        return wasm_v128_not(a);
    }

    // compare

    static inline mask8x16 compare_eq(u8x16 a, u8x16 b)
    {
        // HACK: signed compare used as wasm does not have unsigned variant
        return wasm_i8x16_eq(a, b);
    }

    static inline mask8x16 compare_gt(u8x16 a, u8x16 b)
    {
        return wasm_u8x16_gt(a, b);
    }

    static inline mask8x16 compare_neq(u8x16 a, u8x16 b)
    {
        // HACK: signed compare used as wasm does not have unsigned variant
        return wasm_i8x16_ne(a, b);
    }
  
    static inline mask8x16 compare_lt(u8x16 a, u8x16 b)
    {
        return wasm_u8x16_lt(a, b);
    }

    static inline mask8x16 compare_le(u8x16 a, u8x16 b)
    {
        return wasm_u8x16_le(a, b);
    }

    static inline mask8x16 compare_ge(u8x16 a, u8x16 b)
    {
        return wasm_u8x16_ge(a, b);
    }

    static inline u8x16 select(mask8x16 mask, u8x16 a, u8x16 b)
    {
        return wasm_v128_bitselect(a, b, mask);
    }

    static inline u8x16 min(u8x16 a, u8x16 b)
    {
        return wasm_u8x16_min(a, b);
    }

    static inline u8x16 max(u8x16 a, u8x16 b)
    {
        return wasm_u8x16_max(a, b);
    }

    // -----------------------------------------------------------------
    // u16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u16x8 set_component(u16x8 a, u16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return wasm_u16x8_replace_lane(a, Index, s);
    }

    template <unsigned int Index>
    static inline u16 get_component(u16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return wasm_u16x8_extract_lane(a, Index);
    }

    static inline u16x8 u16x8_zero()
    {
        return wasm_u16x8_splat(0);
    }

    static inline u16x8 u16x8_set(u16 s)
    {
        return wasm_u16x8_splat(s);
    }

    static inline u16x8 u16x8_set(u16 v0, u16 v1, u16 v2, u16 v3, u16 v4, u16 v5, u16 v6, u16 v7)
    {
        return wasm_u16x8_make(v0, v1, v2, v3, v4, v5, v6, v7);
    }

    static inline u16x8 u16x8_uload(const void* source)
    {
        return wasm_v128_load(source);
    }

    static inline void u16x8_ustore(void* dest, u16x8 a)
    {
        wasm_v128_store(dest, a);
    }

    static inline u16x8 u16x8_load_low(const u16* source)
    {
        return wasm_v128_load64_zero(source);
    }

    static inline void u16x8_store_low(u16* dest, u16x8 a)
    {
        wasm_v128_store64_lane(dest, a, 0);
    }

    static inline u16x8 unpacklo(u16x8 a, u16x8 b)
    {
        return wasm_i16x8_shuffle(a, b, 0, 8, 1, 9, 2, 10, 3, 11);
    }

    static inline u16x8 unpackhi(u16x8 a, u16x8 b)
    {
        return wasm_i16x8_shuffle(a, b, 4, 12, 5, 13, 6, 14, 7, 15);
    }

    static inline u16x8 add(u16x8 a, u16x8 b)
    {
        // NOTE: same instruction is used for signed and unsigned
        return wasm_i16x8_add(a, b);
    }

    static inline u16x8 sub(u16x8 a, u16x8 b)
    {
        // NOTE: same instruction is used for signed and unsigned
        return wasm_i16x8_sub(a, b);
    }

    static inline u16x8 adds(u16x8 a, u16x8 b)
    {
        return wasm_u16x8_add_sat(a, b);
    }

    static inline u16x8 subs(u16x8 a, u16x8 b)
    {
        return wasm_u16x8_sub_sat(a, b);
    }

    static inline u16x8 avg(u16x8 a, u16x8 b)
    {
        return wasm_u16x8_avgr(a, b);
    }

    static inline u16x8 mullo(u16x8 a, u16x8 b)
    {
        return wasm_i16x8_mul(a, b);
    }

    // bitwise

    static inline u16x8 bitwise_nand(u16x8 a, u16x8 b)
    {
        return wasm_v128_andnot(b, a);
    }

    static inline u16x8 bitwise_and(u16x8 a, u16x8 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline u16x8 bitwise_or(u16x8 a, u16x8 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline u16x8 bitwise_xor(u16x8 a, u16x8 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline u16x8 bitwise_not(u16x8 a)
    {
        return wasm_v128_not(a);
    }

    // compare

    static inline mask16x8 compare_eq(u16x8 a, u16x8 b)
    {
        // HACK: signed compare used as wasm does not have unsigned variant
        return wasm_i16x8_eq(a, b);
    }

    static inline mask16x8 compare_gt(u16x8 a, u16x8 b)
    {
        return wasm_u16x8_gt(a, b);
    }

    static inline mask16x8 compare_neq(u16x8 a, u16x8 b)
    {
        // HACK: signed compare used as wasm does not have unsigned variant
        return wasm_i16x8_ne(a, b);
    }

    static inline mask16x8 compare_lt(u16x8 a, u16x8 b)
    {
        return wasm_u16x8_lt(a, b);
    }

    static inline mask16x8 compare_le(u16x8 a, u16x8 b)
    {
        return wasm_u16x8_le(a, b);
    }

    static inline mask16x8 compare_ge(u16x8 a, u16x8 b)
    {
        return wasm_u16x8_ge(a, b);
    }

    static inline u16x8 select(mask16x8 mask, u16x8 a, u16x8 b)
    {
        return wasm_v128_bitselect(a, b, mask);
    }

    static inline u16x8 min(u16x8 a, u16x8 b)
    {
        return wasm_u16x8_min(a, b);
    }

    static inline u16x8 max(u16x8 a, u16x8 b)
    {
        return wasm_u16x8_max(a, b);
    }

    // shift by constant

    template <int Count>
    static inline u16x8 slli(u16x8 a)
    {
        return wasm_i16x8_shl(a, Count);
    }

    template <int Count>
    static inline u16x8 srli(u16x8 a)
    {
        return wasm_u16x8_shr(a, Count);
    }

    template <int Count>
    static inline u16x8 srai(u16x8 a)
    {
        return wasm_i16x8_shr(a, Count);
    }

    // shift by scalar

    static inline u16x8 sll(u16x8 a, int count)
    {
        return wasm_i16x8_shl(a, count);
    }

    static inline u16x8 srl(u16x8 a, int count)
    {
        return wasm_u16x8_shr(a, count);
    }

    static inline u16x8 sra(u16x8 a, int count)
    {
        return wasm_i16x8_shr(a, count);
    }

    // -----------------------------------------------------------------
    // u32x4
    // -----------------------------------------------------------------

    // shuffle

    template <u32 x, u32 y, u32 z, u32 w>
    static inline u32x4 shuffle(u32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return wasm_i32x4_shuffle(v, v, x, y, z + 4, w + 4);
    }

    template <>
    inline u32x4 shuffle<0, 1, 2, 3>(u32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline u32x4 set_component(u32x4 a, u32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return wasm_u32x4_replace_lane(a, Index, s);
    }

    template <unsigned int Index>
    static inline u32 get_component(u32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return wasm_u32x4_extract_lane(a, Index);
    }

    static inline u32x4 u32x4_zero()
    {
        return wasm_u32x4_splat(0);
    }

    static inline u32x4 u32x4_set(u32 s)
    {
        return wasm_u32x4_splat(s);
    }

    static inline u32x4 u32x4_set(u32 x, u32 y, u32 z, u32 w)
    {
        return wasm_u32x4_make(x, y, z, w);
    }

    static inline u32x4 u32x4_uload(const void* source)
    {
        return wasm_v128_load(source);
    }

    static inline void u32x4_ustore(void* dest, u32x4 a)
    {
        wasm_v128_store(dest, a);
    }

    static inline u32x4 u32x4_load_low(const u32* source)
    {
        return wasm_v128_load64_zero(source);
    }

    static inline void u32x4_store_low(u32* dest, u32x4 a)
    {
        wasm_v128_store64_lane(dest, a, 0);
    }

    static inline u32x4 unpacklo(u32x4 a, u32x4 b)
    {
        return wasm_i32x4_shuffle(a, b, 0, 4, 1, 5);
    }

    static inline u32x4 unpackhi(u32x4 a, u32x4 b)
    {
        return wasm_i32x4_shuffle(a, b, 2, 6, 3, 7);
    }

    static inline u32x4 add(u32x4 a, u32x4 b)
    {
        // NOTE: same instruction is used for signed and unsigned
        return wasm_i32x4_add(a, b);
    }

    static inline u32x4 sub(u32x4 a, u32x4 b)
    {
        // NOTE: same instruction is used for signed and unsigned
        return wasm_i32x4_sub(a, b);
    }

    static inline u32x4 adds(u32x4 a, u32x4 b)
    {
        // NOTE: same instruction is used for signed and unsigned
        const v128_t temp = wasm_i32x4_add(a, b);
        return wasm_v128_or(temp, wasm_i32x4_lt(temp, a));
    }

    static inline u32x4 subs(u32x4 a, u32x4 b)
    {
        // NOTE: same instruction is used for signed and unsigned
        const v128_t temp = wasm_i32x4_sub(a, b);
        return wasm_v128_and(temp, wasm_i32x4_gt(a, temp));
    }

    static inline u32x4 avg(u32x4 a, u32x4 b)
    {
        v128_t one = wasm_u32x4_splat(1);
        v128_t axb = wasm_v128_xor(a, b);
        v128_t temp = wasm_v128_and(a, b);
        temp = wasm_i32x4_add(temp, wasm_u32x4_shr(axb, 1));
        temp = wasm_i32x4_add(temp, wasm_v128_and(axb, one));
        return temp;
    }

    static inline u32x4 mullo(u32x4 a, u32x4 b)
    {
        return wasm_i32x4_mul(a, b);
    }

    // bitwise

    static inline u32x4 bitwise_nand(u32x4 a, u32x4 b)
    {
        return wasm_v128_andnot(b, a);
    }

    static inline u32x4 bitwise_and(u32x4 a, u32x4 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline u32x4 bitwise_or(u32x4 a, u32x4 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline u32x4 bitwise_xor(u32x4 a, u32x4 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline u32x4 bitwise_not(u32x4 a)
    {
        return wasm_v128_not(a);
    }

    // compare

    static inline mask32x4 compare_eq(u32x4 a, u32x4 b)
    {
        // HACK: signed compare used as wasm does not have unsigned variant
        return wasm_i32x4_eq(a, b);
    }

    static inline mask32x4 compare_gt(u32x4 a, u32x4 b)
    {
        return wasm_u32x4_gt(a, b);
    }

    static inline mask32x4 compare_neq(u32x4 a, u32x4 b)
    {
        // HACK: signed compare used as wasm does not have unsigned variant
        return wasm_i32x4_ne(a, b);
    }

    static inline mask32x4 compare_lt(u32x4 a, u32x4 b)
    {
        return wasm_u32x4_lt(a, b);
    }

    static inline mask32x4 compare_le(u32x4 a, u32x4 b)
    {
        return wasm_u32x4_le(a, b);
    }

    static inline mask32x4 compare_ge(u32x4 a, u32x4 b)
    {
        return wasm_u32x4_ge(a, b);
    }

    static inline u32x4 select(mask32x4 mask, u32x4 a, u32x4 b)
    {
        return wasm_v128_bitselect(a, b, mask);
    }

    static inline u32x4 min(u32x4 a, u32x4 b)
    {
        return wasm_u32x4_min(a, b);
    }

    static inline u32x4 max(u32x4 a, u32x4 b)
    {
        return wasm_u32x4_max(a, b);
    }

    // shift by constant

    template <int Count>
    static inline u32x4 slli(u32x4 a)
    {
        return wasm_i32x4_shl(a, Count);
    }

    template <int Count>
    static inline u32x4 srli(u32x4 a)
    {
        return wasm_u32x4_shr(a, Count);
    }

    template <int Count>
    static inline u32x4 srai(u32x4 a)
    {
        return wasm_i32x4_shr(a, Count);
    }

    // shift by scalar

    static inline u32x4 sll(u32x4 a, int count)
    {
        return wasm_i32x4_shl(a, count);
    }

    static inline u32x4 srl(u32x4 a, int count)
    {
        return wasm_u32x4_shr(a, count);
    }

    static inline u32x4 sra(u32x4 a, int count)
    {
        return wasm_i32x4_shr(a, count);
    }

    // shift by vector

    static inline u32x4 sll(u32x4 a, u32x4 count)
    {
        u32 count0 = wasm_u32x4_extract_lane(count, 0);
        u32 count1 = wasm_u32x4_extract_lane(count, 1);
        u32 count2 = wasm_u32x4_extract_lane(count, 2);
        u32 count3 = wasm_u32x4_extract_lane(count, 3);
        v128_t v0 = wasm_i32x4_shl(a, count0);
        v128_t v1 = wasm_i32x4_shl(a, count1);
        v128_t v2 = wasm_i32x4_shl(a, count2);
        v128_t v3 = wasm_i32x4_shl(a, count3);
        v128_t xyxy = wasm_i32x4_shuffle(v0, v1, 0, 5, 0, 5);
        v128_t zwzw = wasm_i32x4_shuffle(v2, v3, 2, 7, 2, 7);
        return wasm_i32x4_shuffle(xyxy, zwzw, 0, 1, 4, 5);
    }

    static inline u32x4 srl(u32x4 a, u32x4 count)
    {
        u32 count0 = wasm_u32x4_extract_lane(count, 0);
        u32 count1 = wasm_u32x4_extract_lane(count, 1);
        u32 count2 = wasm_u32x4_extract_lane(count, 2);
        u32 count3 = wasm_u32x4_extract_lane(count, 3);
        v128_t v0 = wasm_u32x4_shr(a, count0);
        v128_t v1 = wasm_u32x4_shr(a, count1);
        v128_t v2 = wasm_u32x4_shr(a, count2);
        v128_t v3 = wasm_u32x4_shr(a, count3);
        v128_t xyxy = wasm_i32x4_shuffle(v0, v1, 0, 5, 0, 5);
        v128_t zwzw = wasm_i32x4_shuffle(v2, v3, 2, 7, 2, 7);
        return wasm_i32x4_shuffle(xyxy, zwzw, 0, 1, 4, 5);
    }

    static inline u32x4 sra(u32x4 a, u32x4 count)
    {
        u32 count0 = wasm_u32x4_extract_lane(count, 0);
        u32 count1 = wasm_u32x4_extract_lane(count, 1);
        u32 count2 = wasm_u32x4_extract_lane(count, 2);
        u32 count3 = wasm_u32x4_extract_lane(count, 3);
        v128_t v0 = wasm_i32x4_shr(a, count0);
        v128_t v1 = wasm_i32x4_shr(a, count1);
        v128_t v2 = wasm_i32x4_shr(a, count2);
        v128_t v3 = wasm_i32x4_shr(a, count3);
        v128_t xyxy = wasm_i32x4_shuffle(v0, v1, 0, 5, 0, 5);
        v128_t zwzw = wasm_i32x4_shuffle(v2, v3, 2, 7, 2, 7);
        return wasm_i32x4_shuffle(xyxy, zwzw, 0, 1, 4, 5);
    }

    static inline u32 pack(u32x4 s)
    {
        v128_t s_16 = wasm_i16x8_narrow_i32x4(s, s);
        v128_t s_8 = wasm_u8x16_narrow_i16x8(s_16, s_16);
        return wasm_u32x4_extract_lane(s_8, 0);
    }

    // -----------------------------------------------------------------
    // u64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u64x2 set_component(u64x2 a, u64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return wasm_u64x2_replace_lane(a, Index, s);
    }

    template <unsigned int Index>
    static inline u64 get_component(u64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return wasm_u64x2_extract_lane(a, Index);
    }

    static inline u64x2 u64x2_zero()
    {
        return wasm_u64x2_splat(0);
    }

    static inline u64x2 u64x2_set(u64 s)
    {
        return wasm_u64x2_splat(s);
    }

    static inline u64x2 u64x2_set(u64 x, u64 y)
    {
        return wasm_u64x2_make(x, y);
    }

    static inline u64x2 u64x2_uload(const void* source)
    {
        return wasm_v128_load(source);
    }

    static inline void u64x2_ustore(void* dest, u64x2 a)
    {
        wasm_v128_store(dest, a);
    }

    static inline u64x2 unpacklo(u64x2 a, u64x2 b)
    {
        return wasm_i64x2_shuffle(a, b, 0, 2);
    }

    static inline u64x2 unpackhi(u64x2 a, u64x2 b)
    {
        return wasm_i64x2_shuffle(a, b, 1, 3);
    }

    static inline u64x2 add(u64x2 a, u64x2 b)
    {
        // NOTE: same instruction is used for signed and unsigned
        return wasm_i64x2_add(a, b);
    }

    static inline u64x2 sub(u64x2 a, u64x2 b)
    {
        // NOTE: same instruction is used for signed and unsigned
        return wasm_i64x2_sub(a, b);
    }

    // bitwise

    static inline u64x2 bitwise_nand(u64x2 a, u64x2 b)
    {
        return wasm_v128_andnot(a, b);
    }

    static inline u64x2 bitwise_and(u64x2 a, u64x2 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline u64x2 bitwise_or(u64x2 a, u64x2 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline u64x2 bitwise_xor(u64x2 a, u64x2 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline u64x2 bitwise_not(u64x2 a)
    {
        return wasm_v128_not(a);
    }

    // compare

    static inline mask64x2 compare_eq(u64x2 a, u64x2 b)
    {
        // HACK: signed compare used as wasm does not have unsigned variant
        return wasm_i64x2_eq(a, b);
    }

    static inline mask64x2 compare_gt(u64x2 a, u64x2 b)
    {
        const v128_t sign = wasm_u64x2_splat(0x8000000000000000ull);
        a = wasm_v128_xor(a, sign);
        b = wasm_v128_xor(b, sign);
        // signed compare
        return wasm_i64x2_gt(a, b);
    }

    static inline mask64x2 compare_neq(u64x2 a, u64x2 b)
    {
        return wasm_v128_not(compare_eq(b, a));
    }

    static inline mask64x2 compare_lt(u64x2 a, u64x2 b)
    {
        return compare_gt(b, a);
    }

    static inline mask64x2 compare_le(u64x2 a, u64x2 b)
    {
        return wasm_v128_not(compare_gt(a, b));
    }

    static inline mask64x2 compare_ge(u64x2 a, u64x2 b)
    {
        return wasm_v128_not(compare_gt(b, a));
    }

    static inline u64x2 select(mask64x2 mask, u64x2 a, u64x2 b)
    {
        return wasm_v128_bitselect(a, b, mask);
    }

    static inline u64x2 min(u64x2 a, u64x2 b)
    {
        return select(compare_gt(a, b), b, a);
    }

    static inline u64x2 max(u64x2 a, u64x2 b)
    {
        return select(compare_gt(a, b), a, b);
    }

    // shift by constant

    template <int Count>
    static inline u64x2 slli(u64x2 a)
    {
        return wasm_i64x2_shl(a, Count);
    }

    template <int Count>
    static inline u64x2 srli(u64x2 a)
    {
        return wasm_u64x2_shr(a, Count);
    }

    // shift by scalar

    static inline u64x2 sll(u64x2 a, int count)
    {
        return wasm_i64x2_shl(a, count);
    }

    static inline u64x2 srl(u64x2 a, int count)
    {
        return wasm_u64x2_shr(a, count);
    }

    // -----------------------------------------------------------------
    // s8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s8x16 set_component(s8x16 a, s8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return wasm_i8x16_replace_lane(a, Index, s);
    }

    template <unsigned int Index>
    static inline s8 get_component(s8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return wasm_u8x16_extract_lane(a, Index);
    }

    static inline s8x16 s8x16_zero()
    {
        return wasm_i8x16_splat(0);
    }

    static inline s8x16 s8x16_set(s8 s)
    {
        return wasm_i8x16_splat(s);
    }

    static inline s8x16 s8x16_set(
        s8 v00, s8 v01, s8 v02, s8 v03, s8 v04, s8 v05, s8 v06, s8 v07,
        s8 v08, s8 v09, s8 v10, s8 v11, s8 v12, s8 v13, s8 v14, s8 v15)
    {
        return wasm_i8x16_make(v00, v01, v02, v03, v04, v05, v06, v07,
                               v08, v09, v10, v11, v12, v13, v14, v15);
    }

    static inline s8x16 s8x16_uload(const void* source)
    {
        return wasm_v128_load(source);
    }

    static inline void s8x16_ustore(void* dest, s8x16 a)
    {
        wasm_v128_store(dest, a);
    }

    static inline s8x16 s8x16_load_low(const s8* source)
    {
        return wasm_v128_load64_zero(source);
    }

    static inline void s8x16_store_low(s8* dest, s8x16 a)
    {
        wasm_v128_store64_lane(dest, a, 0);
    }

    static inline s8x16 unpacklo(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_shuffle(a, b, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);
    }

    static inline s8x16 unpackhi(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_shuffle(a, b, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31);
    }

    static inline s8x16 add(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_add(a, b);
    }

    static inline s8x16 sub(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_sub(a, b);
    }

    static inline s8x16 adds(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_add_sat(a, b);
    }

    static inline s8x16 subs(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_sub_sat(a, b);
    }

    static inline s8x16 avg(s8x16 a, s8x16 b)
    {
        const v128_t sign = wasm_u8x16_splat(0x80u);
        a = wasm_v128_xor(a, sign);
        b = wasm_v128_xor(b, sign);
        return wasm_v128_xor(wasm_u8x16_avgr(a, b), sign);
    }

    static inline s8x16 abs(s8x16 a)
    {
        return wasm_i8x16_abs(a);
    }

    static inline s8x16 neg(s8x16 a)
    {
        return wasm_i8x16_neg(a);
    }

    // bitwise

    static inline s8x16 bitwise_nand(s8x16 a, s8x16 b)
    {
        return wasm_v128_andnot(b, a);
    }

    static inline s8x16 bitwise_and(s8x16 a, s8x16 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline s8x16 bitwise_or(s8x16 a, s8x16 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline s8x16 bitwise_xor(s8x16 a, s8x16 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline s8x16 bitwise_not(s8x16 a)
    {
        return wasm_v128_not(a);
    }

    // compare

    static inline mask8x16 compare_eq(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_eq(a, b);
    }

    static inline mask8x16 compare_gt(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_gt(a, b);
    }

    static inline mask8x16 compare_neq(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_ne(a, b);
    }

    static inline mask8x16 compare_lt(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_lt(a, b);
    }

    static inline mask8x16 compare_le(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_le(a, b);
    }

    static inline mask8x16 compare_ge(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_ge(a, b);
    }

    static inline s8x16 select(mask8x16 mask, s8x16 a, s8x16 b)
    {
        return wasm_v128_bitselect(a, b, mask);
    }

    static inline s8x16 min(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_min(a, b);
    }

    static inline s8x16 max(s8x16 a, s8x16 b)
    {
        return wasm_i8x16_max(a, b);
    }

    // -----------------------------------------------------------------
    // s16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s16x8 set_component(s16x8 a, s16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return wasm_i16x8_replace_lane(a, Index, s);
    }

    template <unsigned int Index>
    static inline s16 get_component(s16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return wasm_i16x8_extract_lane(a, Index);
    }

    static inline s16x8 s16x8_zero()
    {
        return wasm_i16x8_splat(0);
    }

    static inline s16x8 s16x8_set(s16 s)
    {
        return wasm_i16x8_splat(s);
    }

    static inline s16x8 s16x8_set(s16 v0, s16 v1, s16 v2, s16 v3, s16 v4, s16 v5, s16 v6, s16 v7)
    {
        return wasm_i16x8_make(v0, v1, v2, v3, v4, v5, v6, v7);
    }

    static inline s16x8 s16x8_uload(const void* source)
    {
        return wasm_v128_load(source);
    }

    static inline void s16x8_ustore(void* dest, s16x8 a)
    {
        wasm_v128_store(dest, a);
    }

    static inline s16x8 s16x8_load_low(const s16* source)
    {
        return wasm_v128_load64_zero(source);
    }

    static inline void s16x8_store_low(s16* dest, s16x8 a)
    {
        wasm_v128_store64_lane(dest, a, 0);
    }

    static inline s16x8 unpacklo(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_shuffle(a, b, 0, 8, 1, 9, 2, 10, 3, 11);
    }

    static inline s16x8 unpackhi(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_shuffle(a, b, 4, 12, 5, 13, 6, 14, 7, 15);
    }

    static inline s16x8 add(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_add(a, b);
    }

    static inline s16x8 sub(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_sub(a, b);
    }

    static inline s16x8 adds(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_add_sat(a, b);
    }

    static inline s16x8 subs(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_sub_sat(a, b);
    }

    static inline s16x8 hadd(s16x8 a, s16x8 b)
    {
        v128_t v0 = wasm_i16x8_shuffle(a, b, 0, 2, 4, 6, 8, 10, 12, 14);
        v128_t v1 = wasm_i16x8_shuffle(a, b, 1, 3, 5, 7, 9, 11, 13, 15);
        return wasm_i16x8_add(v0, v1);
    }

    static inline s16x8 hsub(s16x8 a, s16x8 b)
    {
        v128_t v0 = wasm_i16x8_shuffle(a, b, 0, 2, 4, 6, 8, 10, 12, 14);
        v128_t v1 = wasm_i16x8_shuffle(a, b, 1, 3, 5, 7, 9, 11, 13, 15);
        return wasm_i16x8_sub(v0, v1);
    }

    static inline s16x8 hadds(s16x8 a, s16x8 b)
    {
        v128_t v0 = wasm_i16x8_shuffle(a, b, 0, 2, 4, 6, 8, 10, 12, 14);
        v128_t v1 = wasm_i16x8_shuffle(a, b, 1, 3, 5, 7, 9, 11, 13, 15);
        return wasm_i16x8_add_sat(v0, v1);
    }

    static inline s16x8 hsubs(s16x8 a, s16x8 b)
    {
        v128_t v0 = wasm_i16x8_shuffle(a, b, 0, 2, 4, 6, 8, 10, 12, 14);
        v128_t v1 = wasm_i16x8_shuffle(a, b, 1, 3, 5, 7, 9, 11, 13, 15);
        return wasm_i16x8_sub_sat(v0, v1);
    }

    static inline s16x8 avg(s16x8 a, s16x8 b)
    {
        const v128_t sign = wasm_u16x8_splat(0x8000u);
        a = wasm_v128_xor(a, sign);
        b = wasm_v128_xor(b, sign);
        return wasm_v128_xor(wasm_u16x8_avgr(a, b), sign);
    }

    static inline s16x8 mullo(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_mul(a, b);
    }

    static inline s32x4 madd(s16x8 v_a, s16x8 v_b)
    {
        s16 a[8];
        std::memcpy(a, &v_a, 16);

        s16 b[8];
        std::memcpy(b, &v_b, 16);

        s32 x = s32(a[0]) * s32(b[0]) + s32(a[1]) * s32(b[1]);
        s32 y = s32(a[2]) * s32(b[2]) + s32(a[3]) * s32(b[3]);
        s32 z = s32(a[4]) * s32(b[4]) + s32(a[5]) * s32(b[5]);
        s32 w = s32(a[6]) * s32(b[6]) + s32(a[7]) * s32(b[7]);

        return wasm_i32x4_make(x, y, z, w);
    }

    static inline s16x8 abs(s16x8 a)
    {
        return wasm_i16x8_abs(a);
    }

    static inline s16x8 neg(s16x8 a)
    {
        return wasm_i16x8_neg(a);
    }

    // bitwise

    static inline s16x8 bitwise_nand(s16x8 a, s16x8 b)
    {
        return wasm_v128_andnot(b, a);
    }

    static inline s16x8 bitwise_and(s16x8 a, s16x8 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline s16x8 bitwise_or(s16x8 a, s16x8 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline s16x8 bitwise_xor(s16x8 a, s16x8 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline s16x8 bitwise_not(s16x8 a)
    {
        return wasm_v128_not(a);
    }

    // compare

    static inline mask16x8 compare_eq(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_eq(a, b);
    }

    static inline mask16x8 compare_gt(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_gt(a, b);
    }

    static inline mask16x8 compare_neq(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_ne(a, b);
    }

    static inline mask16x8 compare_lt(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_lt(a, b);
    }

    static inline mask16x8 compare_le(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_le(a, b);
    }

    static inline mask16x8 compare_ge(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_ge(a, b);
    }

    static inline s16x8 select(mask16x8 mask, s16x8 a, s16x8 b)
    {
        return wasm_v128_bitselect(a, b, mask);
    }

    static inline s16x8 min(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_min(a, b);
    }

    static inline s16x8 max(s16x8 a, s16x8 b)
    {
        return wasm_i16x8_max(a, b);
    }

    // shift by constant

    template <int Count>
    static inline s16x8 slli(s16x8 a)
    {
        return wasm_i16x8_shl(a, Count);
    }

    template <int Count>
    static inline s16x8 srli(s16x8 a)
    {
        return wasm_u16x8_shr(a, Count);
    }

    template <int Count>
    static inline s16x8 srai(s16x8 a)
    {
        return wasm_i16x8_shr(a, Count);
    }

    // shift by scalar

    static inline s16x8 sll(s16x8 a, int count)
    {
        return wasm_i16x8_shl(a, count);
    }

    static inline s16x8 srl(s16x8 a, int count)
    {
        return wasm_u16x8_shr(a, count);
    }

    static inline s16x8 sra(s16x8 a, int count)
    {
        return wasm_i16x8_shr(a, count);
    }

    // -----------------------------------------------------------------
    // s32x4
    // -----------------------------------------------------------------

    // shuffle

    template <u32 x, u32 y, u32 z, u32 w>
    static inline s32x4 shuffle(s32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return wasm_i32x4_shuffle(v, v, x, y, z + 4, w + 4);
    }

    template <>
    inline s32x4 shuffle<0, 1, 2, 3>(s32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline s32x4 set_component(s32x4 a, s32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return wasm_i32x4_replace_lane(a, Index, s);
    }

    template <unsigned int Index>
    static inline s32 get_component(s32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return wasm_i32x4_extract_lane(a, Index);
    }

    static inline s32x4 s32x4_zero()
    {
        return wasm_i32x4_splat(0);
    }

    static inline s32x4 s32x4_set(s32 s)
    {
        return wasm_i32x4_splat(s);
    }

    static inline s32x4 s32x4_set(s32 x, s32 y, s32 z, s32 w)
    {
        return wasm_i32x4_make(x, y, z, w);
    }

    static inline s32x4 s32x4_uload(const void* source)
    {
        return wasm_v128_load(source);
    }

    static inline void s32x4_ustore(void* dest, s32x4 a)
    {
        wasm_v128_store(dest, a);
    }

    static inline s32x4 s32x4_load_low(const s32* source)
    {
        return wasm_v128_load64_zero(source);
    }

    static inline void s32x4_store_low(s32* dest, s32x4 a)
    {
        wasm_v128_store64_lane(dest, a, 0);
    }

    static inline s32x4 unpacklo(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_shuffle(a, b, 0, 4, 1, 5);
    }

    static inline s32x4 unpackhi(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_shuffle(a, b, 2, 6, 3, 7);
    }

    static inline s32x4 abs(s32x4 a)
    {
        return wasm_i32x4_abs(a);
    }

    static inline s32x4 neg(s32x4 a)
    {
        return wasm_i32x4_neg(a);
    }

    static inline s32x4 add(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_add(a, b);
    }

    static inline s32x4 sub(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_sub(a, b);
    }

    static inline s32x4 adds(s32x4 a, s32x4 b)
    {
        const v128_t v = wasm_i32x4_add(a, b);
        a = wasm_i32x4_shr(a, 31);
        v128_t temp = wasm_v128_xor(b, v);
        temp = wasm_v128_or(wasm_v128_not(temp), wasm_v128_xor(a, b));
        return wasm_v128_bitselect(v, a, wasm_i32x4_gt(wasm_i32x4_splat(0), temp));
    }

    static inline s32x4 subs(s32x4 a, s32x4 b)
    {
        const v128_t v = wasm_i32x4_sub(a, b);
        a = wasm_i32x4_shr(a, 31);
        v128_t temp = wasm_v128_and(wasm_v128_xor(a, b), wasm_v128_xor(a, v));
        return wasm_v128_bitselect(a, v, wasm_i32x4_gt(wasm_i32x4_splat(0), temp));
    }

    static inline s32x4 hadd(s32x4 a, s32x4 b)
    {
        v128_t v0 = wasm_i32x4_shuffle(a, b, 0, 2, 4, 6);
        v128_t v1 = wasm_i32x4_shuffle(a, b, 1, 3, 5, 7);
        return wasm_i32x4_add(v0, v1);
    }

    static inline s32x4 hsub(s32x4 a, s32x4 b)
    {
        v128_t v0 = wasm_i32x4_shuffle(a, b, 0, 2, 4, 6);
        v128_t v1 = wasm_i32x4_shuffle(a, b, 1, 3, 5, 7);
        return wasm_i32x4_sub(v0, v1);
    }

    static inline s32x4 avg(s32x4 a, s32x4 b)
    {
        const v128_t sign = wasm_u32x4_splat(0x80000000);
        a = wasm_v128_xor(a, sign);
        b = wasm_v128_xor(b, sign);

        // unsigned rounded average
        v128_t one = wasm_u32x4_splat(1);
        v128_t axb = wasm_v128_xor(a, b);
        v128_t temp = wasm_v128_and(a, b);
        temp = wasm_i32x4_add(temp, wasm_u32x4_shr(axb, 1));
        temp = wasm_i32x4_add(temp, wasm_v128_and(axb, one));

        temp = wasm_v128_xor(temp, sign);
        return temp;
    }

    static inline s32x4 mullo(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_mul(a, b);
    }

    // bitwise

    static inline s32x4 bitwise_nand(s32x4 a, s32x4 b)
    {
        return wasm_v128_andnot(b, a);
    }

    static inline s32x4 bitwise_and(s32x4 a, s32x4 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline s32x4 bitwise_or(s32x4 a, s32x4 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline s32x4 bitwise_xor(s32x4 a, s32x4 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline s32x4 bitwise_not(s32x4 a)
    {
        return wasm_v128_not(a);
    }

    // compare

    static inline mask32x4 compare_eq(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_eq(a, b);
    }

    static inline mask32x4 compare_gt(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_gt(a, b);
    }

    static inline mask32x4 compare_neq(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_ne(a, b);
    }

    static inline mask32x4 compare_lt(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_lt(a, b);
    }

    static inline mask32x4 compare_le(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_le(a, b);
    }

    static inline mask32x4 compare_ge(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_ge(a, b);
    }

    static inline s32x4 select(mask32x4 mask, s32x4 a, s32x4 b)
    {
        return wasm_v128_bitselect(a, b, mask);
    }

    static inline s32x4 min(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_min(a, b);
    }

    static inline s32x4 max(s32x4 a, s32x4 b)
    {
        return wasm_i32x4_max(a, b);
    }

    // shift by constant

    template <int Count>
    static inline s32x4 slli(s32x4 a)
    {
        return wasm_i32x4_shl(a, Count);
    }

    template <int Count>
    static inline s32x4 srli(s32x4 a)
    {
        return wasm_u32x4_shr(a, Count);
    }

    template <int Count>
    static inline s32x4 srai(s32x4 a)
    {
        return wasm_i32x4_shr(a, Count);
    }

    // shift by scalar

    static inline s32x4 sll(s32x4 a, int count)
    {
        return wasm_i32x4_shl(a, count);
    }

    static inline s32x4 srl(s32x4 a, int count)
    {
        return wasm_u32x4_shr(a, count);
    }

    static inline s32x4 sra(s32x4 a, int count)
    {
        return wasm_i32x4_shr(a, count);
    }

    // shift by vector

    static inline s32x4 sll(s32x4 a, u32x4 count)
    {
        u32 count0 = wasm_u32x4_extract_lane(count, 0);
        u32 count1 = wasm_u32x4_extract_lane(count, 1);
        u32 count2 = wasm_u32x4_extract_lane(count, 2);
        u32 count3 = wasm_u32x4_extract_lane(count, 3);
        v128_t v0 = wasm_i32x4_shl(a, count0);
        v128_t v1 = wasm_i32x4_shl(a, count1);
        v128_t v2 = wasm_i32x4_shl(a, count2);
        v128_t v3 = wasm_i32x4_shl(a, count3);
        v128_t xyxy = wasm_i32x4_shuffle(v0, v1, 0, 5, 0, 5);
        v128_t zwzw = wasm_i32x4_shuffle(v2, v3, 2, 7, 2, 7);
        return wasm_i32x4_shuffle(xyxy, zwzw, 0, 1, 4, 5);
    }

    static inline s32x4 srl(s32x4 a, u32x4 count)
    {
        u32 count0 = wasm_u32x4_extract_lane(count, 0);
        u32 count1 = wasm_u32x4_extract_lane(count, 1);
        u32 count2 = wasm_u32x4_extract_lane(count, 2);
        u32 count3 = wasm_u32x4_extract_lane(count, 3);
        v128_t v0 = wasm_u32x4_shr(a, count0);
        v128_t v1 = wasm_u32x4_shr(a, count1);
        v128_t v2 = wasm_u32x4_shr(a, count2);
        v128_t v3 = wasm_u32x4_shr(a, count3);
        v128_t xyxy = wasm_i32x4_shuffle(v0, v1, 0, 5, 0, 5);
        v128_t zwzw = wasm_i32x4_shuffle(v2, v3, 2, 7, 2, 7);
        return wasm_i32x4_shuffle(xyxy, zwzw, 0, 1, 4, 5);
    }

    static inline s32x4 sra(s32x4 a, u32x4 count)
    {
        u32 count0 = wasm_u32x4_extract_lane(count, 0);
        u32 count1 = wasm_u32x4_extract_lane(count, 1);
        u32 count2 = wasm_u32x4_extract_lane(count, 2);
        u32 count3 = wasm_u32x4_extract_lane(count, 3);
        v128_t v0 = wasm_i32x4_shr(a, count0);
        v128_t v1 = wasm_i32x4_shr(a, count1);
        v128_t v2 = wasm_i32x4_shr(a, count2);
        v128_t v3 = wasm_i32x4_shr(a, count3);
        v128_t xyxy = wasm_i32x4_shuffle(v0, v1, 0, 5, 0, 5);
        v128_t zwzw = wasm_i32x4_shuffle(v2, v3, 2, 7, 2, 7);
        return wasm_i32x4_shuffle(xyxy, zwzw, 0, 1, 4, 5);
    }

    static inline u32 pack(s32x4 s)
    {
        v128_t s_16 = wasm_i16x8_narrow_i32x4(s, s);
        v128_t s_8 = wasm_u8x16_narrow_i16x8(s_16, s_16);
        return wasm_u32x4_extract_lane(s_8, 0);
    }

    static inline s32x4 unpack(u32 s)
    {
        s32 x = s32((s >>  0) & 0xff);
        s32 y = s32((s >>  8) & 0xff);
        s32 z = s32((s >> 16) & 0xff);
        s32 w = s32((s >> 24) & 0xff);
        return wasm_i32x4_make(x, y, z, w);
    }

    // -----------------------------------------------------------------
    // s64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s64x2 set_component(s64x2 a, s64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return wasm_i64x2_replace_lane(a, Index, s);
    }

    template <unsigned int Index>
    static inline s64 get_component(s64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return wasm_i64x2_extract_lane(a, Index);
    }

    static inline s64x2 s64x2_zero()
    {
        return wasm_i64x2_splat(0);
    }

    static inline s64x2 s64x2_set(s64 s)
    {
        return wasm_i64x2_splat(s);
    }

    static inline s64x2 s64x2_set(s64 x, s64 y)
    {
        return wasm_i64x2_make(x, y);
    }

    static inline s64x2 s64x2_uload(const void* source)
    {
        return wasm_v128_load(source);
    }

    static inline void s64x2_ustore(void* dest, s64x2 a)
    {
        wasm_v128_store(dest, a);
    }

    static inline s64x2 unpacklo(s64x2 a, s64x2 b)
    {
        return wasm_i64x2_shuffle(a, b, 0, 2);
    }

    static inline s64x2 unpackhi(s64x2 a, s64x2 b)
    {
        return wasm_i64x2_shuffle(a, b, 1, 3);
    }

    static inline s64x2 add(s64x2 a, s64x2 b)
    {
        return wasm_i64x2_add(a, b);
    }

    static inline s64x2 sub(s64x2 a, s64x2 b)
    {
        return wasm_i64x2_sub(a, b);
    }

    static inline s64x2 neg(s64x2 a)
    {
        return wasm_i64x2_sub(wasm_i64x2_splat(0), a);
    }

    // bitwise

    static inline s64x2 bitwise_nand(s64x2 a, s64x2 b)
    {
        return wasm_v128_andnot(a, b);
    }

    static inline s64x2 bitwise_and(s64x2 a, s64x2 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline s64x2 bitwise_or(s64x2 a, s64x2 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline s64x2 bitwise_xor(s64x2 a, s64x2 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline s64x2 bitwise_not(s64x2 a)
    {
        return wasm_v128_not(a);
    }

    // compare

    static inline mask64x2 compare_eq(s64x2 a, s64x2 b)
    {
        return wasm_i64x2_eq(a, b);
    }

    static inline mask64x2 compare_gt(s64x2 a, s64x2 b)
    {
        return wasm_i64x2_gt(a, b);
    }

    static inline mask64x2 compare_neq(s64x2 a, s64x2 b)
    {
        return wasm_i64x2_ne(a, a);
    }

    static inline mask64x2 compare_lt(s64x2 a, s64x2 b)
    {
        return wasm_i64x2_lt(a, b);
    }

    static inline mask64x2 compare_le(s64x2 a, s64x2 b)
    {
        return wasm_i64x2_le(a, b);
    }

    static inline mask64x2 compare_ge(s64x2 a, s64x2 b)
    {
        return wasm_i64x2_ge(a, b);
    }

    static inline s64x2 select(mask64x2 mask, s64x2 a, s64x2 b)
    {
        return wasm_v128_bitselect(a, b, mask);
    }

    static inline s64x2 min(s64x2 a, s64x2 b)
    {
        return select(compare_gt(a, b), b, a);
    }

    static inline s64x2 max(s64x2 a, s64x2 b)
    {
        return select(compare_gt(a, b), a, b);
    }

    // shift by constant

    template <int Count>
    static inline s64x2 slli(s64x2 a)
    {
        return wasm_i64x2_shl(a, Count);
    }

    template <int Count>
    static inline s64x2 srli(s64x2 a)
    {
        return wasm_u64x2_shr(a, Count);
    }

    // shift by scalar

    static inline s64x2 sll(s64x2 a, int count)
    {
        return wasm_i64x2_shl(a, count);
    }

    static inline s64x2 srl(s64x2 a, int count)
    {
        return wasm_u64x2_shr(a, count);
    }

    // -----------------------------------------------------------------
    // mask8x16
    // -----------------------------------------------------------------

    static inline mask8x16 operator & (mask8x16 a, mask8x16 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline mask8x16 operator | (mask8x16 a, mask8x16 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline mask8x16 operator ^ (mask8x16 a, mask8x16 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline mask8x16 operator ! (mask8x16 a)
    {
        return wasm_v128_not(a);
    }

    static inline u32 get_mask(mask8x16 a)
    {
        return wasm_i8x16_bitmask(a);
    }

    static inline bool none_of(mask8x16 a)
    {
        return !wasm_v128_any_true(a);
    }

    static inline bool any_of(mask8x16 a)
    {
        return wasm_v128_any_true(a);
    }

    static inline bool all_of(mask8x16 a)
    {
        return wasm_i8x16_all_true(a);
    }

    // -----------------------------------------------------------------
    // mask16x8
    // -----------------------------------------------------------------

    static inline mask16x8 operator & (mask16x8 a, mask16x8 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline mask16x8 operator | (mask16x8 a, mask16x8 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline mask16x8 operator ^ (mask16x8 a, mask16x8 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline mask16x8 operator ! (mask16x8 a)
    {
        return wasm_v128_not(a);
    }

    static inline u32 get_mask(mask16x8 a)
    {
        return wasm_i16x8_bitmask(a);
    }

    static inline bool none_of(mask16x8 a)
    {
        return !wasm_v128_any_true(a);
    }

    static inline bool any_of(mask16x8 a)
    {
        return wasm_v128_any_true(a);
    }

    static inline bool all_of(mask16x8 a)
    {
        return wasm_i16x8_all_true(a);
    }

    // -----------------------------------------------------------------
    // mask32x4
    // -----------------------------------------------------------------

    static inline mask32x4 operator & (mask32x4 a, mask32x4 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline mask32x4 operator | (mask32x4 a, mask32x4 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline mask32x4 operator ^ (mask32x4 a, mask32x4 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline mask32x4 operator ! (mask32x4 a)
    {
        return wasm_v128_not(a);
    }

    static inline u32 get_mask(mask32x4 a)
    {
        return wasm_i32x4_bitmask(a);
    }

    static inline bool none_of(mask32x4 a)
    {
        return !wasm_v128_any_true(a);
    }

    static inline bool any_of(mask32x4 a)
    {
        return wasm_v128_any_true(a);
    }

    static inline bool all_of(mask32x4 a)
    {
        return wasm_i32x4_all_true(a);
    }

    // -----------------------------------------------------------------
    // mask64x2
    // -----------------------------------------------------------------

    static inline mask64x2 operator & (mask64x2 a, mask64x2 b)
    {
        return wasm_v128_and(a, b);
    }

    static inline mask64x2 operator | (mask64x2 a, mask64x2 b)
    {
        return wasm_v128_or(a, b);
    }

    static inline mask64x2 operator ^ (mask64x2 a, mask64x2 b)
    {
        return wasm_v128_xor(a, b);
    }

    static inline mask64x2 operator ! (mask64x2 a)
    {
        return wasm_v128_not(a);
    }

    static inline u32 get_mask(mask64x2 a)
    {
        return wasm_i64x2_bitmask(a);
    }

    static inline bool none_of(mask64x2 a)
    {
        return !wasm_v128_any_true(a);
    }

    static inline bool any_of(mask64x2 a)
    {
        return wasm_v128_any_true(a);
    }

    static inline bool all_of(mask64x2 a)
    {
        return wasm_i64x2_all_true(a);
    }

    // -----------------------------------------------------------------
    // masked functions
    // -----------------------------------------------------------------

    // min

    static inline u8x16 min(u8x16 a, u8x16 b, mask8x16 mask)
    {
        return wasm_v128_and(mask, min(a, b));
    }

    static inline u16x8 min(u16x8 a, u16x8 b, mask16x8 mask)
    {
        return wasm_v128_and(mask, min(a, b));
    }

    static inline u32x4 min(u32x4 a, u32x4 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, min(a, b));
    }

    static inline u64x2 min(u64x2 a, u64x2 b, mask64x2 mask)
    {
        return wasm_v128_and(mask, min(a, b));
    }

    static inline s8x16 min(s8x16 a, s8x16 b, mask8x16 mask)
    {
        return wasm_v128_and(mask, min(a, b));
    }

    static inline s16x8 min(s16x8 a, s16x8 b, mask16x8 mask)
    {
        return wasm_v128_and(mask, min(a, b));
    }

    static inline s32x4 min(s32x4 a, s32x4 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, min(a, b));
    }

    static inline s64x2 min(s64x2 a, s64x2 b, mask64x2 mask)
    {
        return wasm_v128_and(mask, min(a, b));
    }

    // max

    static inline u8x16 max(u8x16 a, u8x16 b, mask8x16 mask)
    {
        return wasm_v128_and(mask, max(a, b));
    }

    static inline u16x8 max(u16x8 a, u16x8 b, mask16x8 mask)
    {
        return wasm_v128_and(mask, max(a, b));
    }

    static inline u32x4 max(u32x4 a, u32x4 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, max(a, b));
    }

    static inline u64x2 max(u64x2 a, u64x2 b, mask64x2 mask)
    {
        return wasm_v128_and(mask, max(a, b));
    }

    static inline s8x16 max(s8x16 a, s8x16 b, mask8x16 mask)
    {
        return wasm_v128_and(mask, max(a, b));
    }

    static inline s16x8 max(s16x8 a, s16x8 b, mask16x8 mask)
    {
        return wasm_v128_and(mask, max(a, b));
    }

    static inline s32x4 max(s32x4 a, s32x4 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, max(a, b));
    }

    static inline s64x2 max(s64x2 a, s64x2 b, mask64x2 mask)
    {
        return wasm_v128_and(mask, max(a, b));
    }

    // add

    static inline u8x16 add(u8x16 a, u8x16 b, mask8x16 mask)
    {
        return wasm_v128_and(mask, add(a, b));
    }

    static inline u16x8 add(u16x8 a, u16x8 b, mask16x8 mask)
    {
        return wasm_v128_and(mask, add(a, b));
    }

    static inline u32x4 add(u32x4 a, u32x4 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, add(a, b));
    }

    static inline u64x2 add(u64x2 a, u64x2 b, mask64x2 mask)
    {
        return wasm_v128_and(mask, add(a, b));
    }

    static inline s8x16 add(s8x16 a, s8x16 b, mask8x16 mask)
    {
        return wasm_v128_and(mask, add(a, b));
    }

    static inline s16x8 add(s16x8 a, s16x8 b, mask16x8 mask)
    {
        return wasm_v128_and(mask, add(a, b));
    }

    static inline s32x4 add(s32x4 a, s32x4 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, add(a, b));
    }

    static inline s64x2 add(s64x2 a, s64x2 b, mask64x2 mask)
    {
        return wasm_v128_and(mask, add(a, b));
    }

    // sub

    static inline u8x16 sub(u8x16 a, u8x16 b, mask8x16 mask)
    {
        return wasm_v128_and(mask, sub(a, b));
    }

    static inline u16x8 sub(u16x8 a, u16x8 b, mask16x8 mask)
    {
        return wasm_v128_and(mask, sub(a, b));
    }

    static inline u32x4 sub(u32x4 a, u32x4 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, sub(a, b));
    }

    static inline u64x2 sub(u64x2 a, u64x2 b, mask64x2 mask)
    {
        return wasm_v128_and(mask, sub(a, b));
    }

    static inline s8x16 sub(s8x16 a, s8x16 b, mask8x16 mask)
    {
        return wasm_v128_and(mask, sub(a, b));
    }

    static inline s16x8 sub(s16x8 a, s16x8 b, mask16x8 mask)
    {
        return wasm_v128_and(mask, sub(a, b));
    }

    static inline s32x4 sub(s32x4 a, s32x4 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, sub(a, b));
    }

    static inline s64x2 sub(s64x2 a, s64x2 b, mask64x2 mask)
    {
        return wasm_v128_and(mask, sub(a, b));
    }

    // adds

    static inline u8x16 adds(u8x16 a, u8x16 b, mask8x16 mask)
    {
        return wasm_v128_and(mask, adds(a, b));
    }

    static inline u16x8 adds(u16x8 a, u16x8 b, mask16x8 mask)
    {
        return wasm_v128_and(mask, adds(a, b));
    }

    static inline u32x4 adds(u32x4 a, u32x4 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, adds(a, b));
    }

    static inline s8x16 adds(s8x16 a, s8x16 b, mask8x16 mask)
    {
        return wasm_v128_and(mask, adds(a, b));
    }

    static inline s16x8 adds(s16x8 a, s16x8 b, mask16x8 mask)
    {
        return wasm_v128_and(mask, adds(a, b));
    }

    static inline s32x4 adds(s32x4 a, s32x4 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, adds(a, b));
    }

    // subs

    static inline u8x16 subs(u8x16 a, u8x16 b, mask8x16 mask)
    {
        return wasm_v128_and(mask, subs(a, b));
    }

    static inline u16x8 subs(u16x8 a, u16x8 b, mask16x8 mask)
    {
        return wasm_v128_and(mask, subs(a, b));
    }

    static inline u32x4 subs(u32x4 a, u32x4 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, subs(a, b));
    }

    static inline s8x16 subs(s8x16 a, s8x16 b, mask8x16 mask)
    {
        return wasm_v128_and(mask, subs(a, b));
    }

    static inline s16x8 subs(s16x8 a, s16x8 b, mask16x8 mask)
    {
        return wasm_v128_and(mask, subs(a, b));
    }

    static inline s32x4 subs(s32x4 a, s32x4 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, subs(a, b));
    }

    // madd

    static inline s32x4 madd(s16x8 a, s16x8 b, mask32x4 mask)
    {
        return wasm_v128_and(mask, madd(a, b));
    }

    // abs

    static inline s8x16 abs(s8x16 a, mask8x16 mask)
    {
        return wasm_v128_and(mask, abs(a));
    }

    static inline s16x8 abs(s16x8 a, mask16x8 mask)
    {
        return wasm_v128_and(mask, abs(a));
    }

    static inline s32x4 abs(s32x4 a, mask32x4 mask)
    {
        return wasm_v128_and(mask, abs(a));
    }

#define SIMD_MASK_INT128
#include <mango/simd/common_mask.hpp>
#undef SIMD_MASK_INT128

} // namespace mango::simd
