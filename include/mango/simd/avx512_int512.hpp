/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // helpers
    // -----------------------------------------------------------------

    static inline __m512i simd512_not(__m512i a)
    {
        return _mm512_xor_si512(a, _mm512_set1_epi32(0xffffffff));
    }

    // -----------------------------------------------------------------
    // uint8x64
    // -----------------------------------------------------------------

    static inline uint8x64 uint8x64_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline uint8x64 uint8x64_set1(u8 s)
    {
        return _mm512_set1_epi8(s);
    }

    static inline uint8x64 unpacklo(uint8x64 a, uint8x64 b)
    {
        return _mm512_unpacklo_epi8(a, b);
    }

    static inline uint8x64 unpackhi(uint8x64 a, uint8x64 b)
    {
        return _mm512_unpackhi_epi8(a, b);
    }

    static inline uint8x64 add(uint8x64 a, uint8x64 b)
    {
        return _mm512_add_epi8(a, b);
    }

    static inline uint8x64 sub(uint8x64 a, uint8x64 b)
    {
        return _mm512_sub_epi8(a, b);
    }

    // saturated

    static inline uint8x64 adds(uint8x64 a, uint8x64 b)
    {
        return _mm512_adds_epu8(a, b);
    }

    static inline uint8x64 subs(uint8x64 a, uint8x64 b)
    {
        return _mm512_subs_epu8(a, b);
    }

    // bitwise

    static inline uint8x64 bitwise_nand(uint8x64 a, uint8x64 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline uint8x64 bitwise_and(uint8x64 a, uint8x64 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline uint8x64 bitwise_or(uint8x64 a, uint8x64 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline uint8x64 bitwise_xor(uint8x64 a, uint8x64 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline uint8x64 bitwise_not(uint8x64 a)
    {
        return simd512_not(a);
    }

    // compare

    static inline mask8x64 compare_eq(uint8x64 a, uint8x64 b)
    {
        return _mm512_cmp_epu8_mask(a, b, 0);
    }

    static inline mask8x64 compare_gt(uint8x64 a, uint8x64 b)
    {
        return _mm512_cmp_epu8_mask(b, a, 1);
    }

    static inline uint8x64 select(mask8x64 mask, uint8x64 a, uint8x64 b)
    {
        return _mm512_mask_blend_epi8(mask, b, a);
    }

    static inline uint8x64 min(uint8x64 a, uint8x64 b)
    {
        return _mm512_min_epu8(a, b);
    }

    static inline uint8x64 max(uint8x64 a, uint8x64 b)
    {
        return _mm512_max_epu8(a, b);
    }

    // -----------------------------------------------------------------
    // uint16x32
    // -----------------------------------------------------------------

    static inline uint16x32 uint16x32_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline uint16x32 uint16x32_set1(u16 s)
    {
        return _mm512_set1_epi16(s);
    }

    static inline uint16x32 unpacklo(uint16x32 a, uint16x32 b)
    {
        return _mm512_unpacklo_epi16(a, b);
    }

    static inline uint16x32 unpackhi(uint16x32 a, uint16x32 b)
    {
        return _mm512_unpackhi_epi16(a, b);
    }

    static inline uint16x32 add(uint16x32 a, uint16x32 b)
    {
        return _mm512_add_epi16(a, b);
    }

    static inline uint16x32 sub(uint16x32 a, uint16x32 b)
    {
        return _mm512_sub_epi16(a, b);
    }

    static inline uint16x32 mullo(uint16x32 a, uint16x32 b)
    {
        return _mm512_mullo_epi16(a, b);
    }

    // saturated

    static inline uint16x32 adds(uint16x32 a, uint16x32 b)
    {
        return _mm512_adds_epu16(a, b);
    }

    static inline uint16x32 subs(uint16x32 a, uint16x32 b)
    {
        return _mm512_subs_epu16(a, b);
    }

    // bitwise

    static inline uint16x32 bitwise_nand(uint16x32 a, uint16x32 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline uint16x32 bitwise_and(uint16x32 a, uint16x32 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline uint16x32 bitwise_or(uint16x32 a, uint16x32 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline uint16x32 bitwise_xor(uint16x32 a, uint16x32 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline uint16x32 bitwise_not(uint16x32 a)
    {
        return simd512_not(a);
    }

    // compare

    static inline mask16x32 compare_eq(uint16x32 a, uint16x32 b)
    {
        return _mm512_cmp_epu16_mask(a, b, 0);
    }

    static inline mask16x32 compare_gt(uint16x32 a, uint16x32 b)
    {
        return _mm512_cmp_epu16_mask(b, a, 1);
    }

    static inline uint16x32 select(mask16x32 mask, uint16x32 a, uint16x32 b)
    {
        return _mm512_mask_blend_epi16(mask, b, a);
    }

    // shift by constant

    template <int Count>
    static inline uint16x32 slli(uint16x32 a)
    {
        return _mm512_slli_epi16(a, Count);
    }

    template <int Count>
    static inline uint16x32 srli(uint16x32 a)
    {
        return _mm512_srli_epi16(a, Count);
    }

    template <int Count>
    static inline uint16x32 srai(uint16x32 a)
    {
        return _mm512_srai_epi16(a, Count);
    }

    // shift by scalar

    static inline uint16x32 sll(uint16x32 a, int count)
    {
        return _mm512_sll_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline uint16x32 srl(uint16x32 a, int count)
    {
        return _mm512_srl_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline uint16x32 sra(uint16x32 a, int count)
    {
        return _mm512_sra_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline uint16x32 min(uint16x32 a, uint16x32 b)
    {
        return _mm512_min_epu16(a, b);
    }

    static inline uint16x32 max(uint16x32 a, uint16x32 b)
    {
        return _mm512_max_epu16(a, b);
    }

    // -----------------------------------------------------------------
    // uint32x16
    // -----------------------------------------------------------------

    static inline uint32x16 uint32x16_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline uint32x16 uint32x16_set1(u32 s)
    {
        return _mm512_set1_epi32(s);
    }

    static inline uint32x16 uint32x16_set16(u32 s0, u32 s1, u32 s2, u32 s3, u32 s4, u32 s5, u32 s6, u32 s7,
        u32 s8, u32 s9, u32 s10, u32 s11, u32 s12, u32 s13, u32 s14, u32 s15)
    {
        return _mm512_setr_epi32(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15);
    }

    static inline uint32x16 uint32x16_uload(const u32* source)
    {
        return _mm512_loadu_si512(source);
    }

    static inline void uint32x16_ustore(u32* dest, uint32x16 a)
    {
        _mm512_storeu_si512(dest, a);
    }

    static inline uint32x16 unpacklo(uint32x16 a, uint32x16 b)
    {
        return _mm512_unpacklo_epi32(a, b);
    }

    static inline uint32x16 unpackhi(uint32x16 a, uint32x16 b)
    {
        return _mm512_unpackhi_epi32(a, b);
    }

    static inline uint32x16 add(uint32x16 a, uint32x16 b)
    {
        return _mm512_add_epi32(a, b);
    }

    static inline uint32x16 sub(uint32x16 a, uint32x16 b)
    {
        return _mm512_sub_epi32(a, b);
    }

    static inline uint32x16 mullo(uint32x16 a, uint32x16 b)
    {
        return _mm512_mullo_epi32(a, b);
    }

    // bitwise

    static inline uint32x16 bitwise_nand(uint32x16 a, uint32x16 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline uint32x16 bitwise_and(uint32x16 a, uint32x16 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline uint32x16 bitwise_or(uint32x16 a, uint32x16 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline uint32x16 bitwise_xor(uint32x16 a, uint32x16 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline uint32x16 bitwise_not(uint32x16 a)
    {
        return simd512_not(a);
    }

    // compare

    static inline mask32x16 compare_eq(uint32x16 a, uint32x16 b)
    {
        return _mm512_cmp_epu32_mask(a, b, 0);
    }

    static inline mask32x16 compare_gt(uint32x16 a, uint32x16 b)
    {
        return _mm512_cmp_epu32_mask(b, a, 1);
    }

    static inline uint32x16 select(mask32x16 mask, uint32x16 a, uint32x16 b)
    {
        return _mm512_mask_blend_epi32(mask, b, a);
    }

    // shift by constant

    template <int Count>
    static inline uint32x16 slli(uint32x16 a)
    {
        return _mm512_slli_epi32(a, Count);
    }

    template <int Count>
    static inline uint32x16 srli(uint32x16 a)
    {
        return _mm512_srli_epi32(a, Count);
    }

    template <int Count>
    static inline uint32x16 srai(uint32x16 a)
    {
        return _mm512_srai_epi32(a, Count);
    }

    // shift by scalar

    static inline uint32x16 sll(uint32x16 a, int count)
    {
        return _mm512_sll_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline uint32x16 srl(uint32x16 a, int count)
    {
        return _mm512_srl_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline uint32x16 sra(uint32x16 a, int count)
    {
        return _mm512_sra_epi32(a, _mm_cvtsi32_si128(count));
    }

    // shift by vector

    static inline uint32x16 sll(uint32x16 a, uint32x16 count)
    {
        return _mm512_sllv_epi32(a, count);
    }

    static inline uint32x16 srl(uint32x16 a, uint32x16 count)
    {
        return _mm512_srlv_epi32(a, count);
    }

    static inline uint32x16 sra(uint32x16 a, uint32x16 count)
    {
        return _mm512_srav_epi32(a, count);
    }

    static inline uint32x16 min(uint32x16 a, uint32x16 b)
    {
        return _mm512_min_epu32(a, b);
    }

    static inline uint32x16 max(uint32x16 a, uint32x16 b)
    {
        return _mm512_max_epu32(a, b);
    }

    // -----------------------------------------------------------------
    // uint64x8
    // -----------------------------------------------------------------

    static inline uint64x8 uint64x8_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline uint64x8 uint64x8_set1(u64 s)
    {
        return _mm512_set1_epi64(s);
    }

    static inline uint64x8 uint64x8_set8(u64 s0, u64 s1, u64 s2, u64 s3, u64 s4, u64 s5, u64 s6, u64 s7)
    {
        return _mm512_setr_epi64(s0, s1, s2, s3, s4, s5, s6, s7);
    }

    static inline uint64x8 unpacklo(uint64x8 a, uint64x8 b)
    {
        return _mm512_unpacklo_epi64(a, b);
    }

    static inline uint64x8 unpackhi(uint64x8 a, uint64x8 b)
    {
        return _mm512_unpackhi_epi64(a, b);
    }

    static inline uint64x8 add(uint64x8 a, uint64x8 b)
    {
        return _mm512_add_epi64(a, b);
    }

    static inline uint64x8 sub(uint64x8 a, uint64x8 b)
    {
        return _mm512_sub_epi64(a, b);
    }

    static inline uint64x8 bitwise_nand(uint64x8 a, uint64x8 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline uint64x8 bitwise_and(uint64x8 a, uint64x8 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline uint64x8 bitwise_or(uint64x8 a, uint64x8 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline uint64x8 bitwise_xor(uint64x8 a, uint64x8 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline uint64x8 bitwise_not(uint64x8 a)
    {
        return simd512_not(a);
    }

    static inline uint64x8 select(mask64x8 mask, uint64x8 a, uint64x8 b)
    {
        return _mm512_mask_blend_epi64(mask, b, a);
    }

    // shift by constant

    template <int Count>
    static inline uint64x8 slli(uint64x8 a)
    {
        return _mm512_slli_epi64(a, Count);
    }

    template <int Count>
    static inline uint64x8 srli(uint64x8 a)
    {
        return _mm512_srli_epi64(a, Count);
    }

    // shift by scalar

    static inline uint64x8 sll(uint64x8 a, int count)
    {
        return _mm512_sll_epi64(a, _mm_cvtsi32_si128(count));
    }

    static inline uint64x8 srl(uint64x8 a, int count)
    {
        return _mm512_srl_epi64(a, _mm_cvtsi32_si128(count));
    }

    // -----------------------------------------------------------------
    // int8x64
    // -----------------------------------------------------------------

    static inline int8x64 int8x64_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline int8x64 int8x64_set1(s8 s)
    {
        return _mm512_set1_epi8(s);
    }

    static inline int8x64 unpacklo(int8x64 a, int8x64 b)
    {
        return _mm512_unpacklo_epi8(a, b);
    }

    static inline int8x64 unpackhi(int8x64 a, int8x64 b)
    {
        return _mm512_unpackhi_epi8(a, b);
    }

    static inline int8x64 add(int8x64 a, int8x64 b)
    {
        return _mm512_add_epi8(a, b);
    }

    static inline int8x64 sub(int8x64 a, int8x64 b)
    {
        return _mm512_sub_epi8(a, b);
    }

    // saturated

    static inline int8x64 adds(int8x64 a, int8x64 b)
    {
        return _mm512_adds_epi8(a, b);
    }

    static inline int8x64 subs(int8x64 a, int8x64 b)
    {
        return _mm512_subs_epi8(a, b);
    }

    static inline int8x64 abs(int8x64 a)
    {
        return _mm512_abs_epi8(a);
    }

    static inline int8x64 neg(int8x64 a)
    {
        return _mm512_sub_epi8(_mm512_setzero_si512(), a);
    }

    // bitwise

    static inline int8x64 bitwise_nand(int8x64 a, int8x64 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline int8x64 bitwise_and(int8x64 a, int8x64 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline int8x64 bitwise_or(int8x64 a, int8x64 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline int8x64 bitwise_xor(int8x64 a, int8x64 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline int8x64 bitwise_not(int8x64 a)
    {
        return simd512_not(a);
    }

    // compare

    static inline mask8x64 compare_eq(int8x64 a, int8x64 b)
    {
        return _mm512_cmp_epi8_mask(a, b, 0);
    }

    static inline mask8x64 compare_gt(int8x64 a, int8x64 b)
    {
        return _mm512_cmp_epi8_mask(b, a, 1);
    }

    static inline int8x64 select(mask8x64 mask, int8x64 a, int8x64 b)
    {
        return _mm512_mask_blend_epi8(mask, b, a);
    }

    static inline int8x64 min(int8x64 a, int8x64 b)
    {
        return _mm512_min_epi8(a, b);
    }

    static inline int8x64 max(int8x64 a, int8x64 b)
    {
        return _mm512_max_epi8(a, b);
    }

    // -----------------------------------------------------------------
    // int16x32
    // -----------------------------------------------------------------

    static inline int16x32 int16x32_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline int16x32 int16x32_set1(s16 s)
    {
        return _mm512_set1_epi16(s);
    }

    static inline int16x32 unpacklo(int16x32 a, int16x32 b)
    {
        return _mm512_unpacklo_epi16(a, b);
    }

    static inline int16x32 unpackhi(int16x32 a, int16x32 b)
    {
        return _mm512_unpackhi_epi16(a, b);
    }

    static inline int16x32 add(int16x32 a, int16x32 b)
    {
        return _mm512_add_epi16(a, b);
    }

    static inline int16x32 sub(int16x32 a, int16x32 b)
    {
        return _mm512_sub_epi16(a, b);
    }

    static inline int16x32 mullo(int16x32 a, int16x32 b)
    {
        return _mm512_mullo_epi16(a, b);
    }

    // saturated

    static inline int16x32 adds(int16x32 a, int16x32 b)
    {
        return _mm512_adds_epi16(a, b);
    }

    static inline int16x32 subs(int16x32 a, int16x32 b)
    {
        return _mm512_subs_epi16(a, b);
    }

    static inline int16x32 abs(int16x32 a)
    {
        return _mm512_abs_epi16(a);
    }

    static inline int16x32 neg(int16x32 a)
    {
        return _mm512_sub_epi16(_mm512_setzero_si512(), a);
    }

    // bitwise

    static inline int16x32 bitwise_nand(int16x32 a, int16x32 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline int16x32 bitwise_and(int16x32 a, int16x32 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline int16x32 bitwise_or(int16x32 a, int16x32 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline int16x32 bitwise_xor(int16x32 a, int16x32 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline int16x32 bitwise_not(int16x32 a)
    {
        return simd512_not(a);
    }

    // compare

    static inline mask16x32 compare_eq(int16x32 a, int16x32 b)
    {
        return _mm512_cmp_epi16_mask(a, b, 0);
    }

    static inline mask16x32 compare_gt(int16x32 a, int16x32 b)
    {
        return _mm512_cmp_epi16_mask(b, a, 1);
    }

    static inline int16x32 select(mask16x32 mask, int16x32 a, int16x32 b)
    {
        return _mm512_mask_blend_epi16(mask, b, a);
    }

    // shift by constant

    template <int Count>
    static inline int16x32 slli(int16x32 a)
    {
        return _mm512_slli_epi16(a, Count);
    }

    template <int Count>
    static inline int16x32 srli(int16x32 a)
    {
        return _mm512_srli_epi16(a, Count);
    }

    template <int Count>
    static inline int16x32 srai(int16x32 a)
    {
        return _mm512_srai_epi16(a, Count);
    }

    // shift by scalar

    static inline int16x32 sll(int16x32 a, int count)
    {
        return _mm512_sll_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline int16x32 srl(int16x32 a, int count)
    {
        return _mm512_srl_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline int16x32 sra(int16x32 a, int count)
    {
        return _mm512_sra_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline int16x32 min(int16x32 a, int16x32 b)
    {
        return _mm512_min_epi16(a, b);
    }

    static inline int16x32 max(int16x32 a, int16x32 b)
    {
        return _mm512_max_epi16(a, b);
    }

    // -----------------------------------------------------------------
    // int32x16
    // -----------------------------------------------------------------

    static inline int32x16 int32x16_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline int32x16 int32x16_set1(s32 s)
    {
        return _mm512_set1_epi32(s);
    }

    static inline int32x16 int32x16_set16(s32 v0, s32 v1, s32 v2, s32 v3, s32 v4, s32 v5, s32 v6, s32 v7,
        s32 v8, s32 v9, s32 v10, s32 v11, s32 v12, s32 v13, s32 v14, s32 v15)
    {
        return _mm512_setr_epi32(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15);
    }

    static inline int32x16 int32x16_uload(const s32* source)
    {
        return _mm512_loadu_si512(source);
    }

    static inline void int32x16_ustore(s32* dest, int32x16 a)
    {
        _mm512_storeu_si512(dest, a);
    }

    static inline int32x16 unpacklo(int32x16 a, int32x16 b)
    {
        return _mm512_unpacklo_epi32(a, b);
    }

    static inline int32x16 unpackhi(int32x16 a, int32x16 b)
    {
        return _mm512_unpackhi_epi32(a, b);
    }

    static inline int32x16 abs(int32x16 a)
    {
        return _mm512_abs_epi32(a);
    }

    static inline int32x16 neg(int32x16 a)
    {
        return _mm512_sub_epi32(_mm512_setzero_si512(), a);
    }

    static inline int32x16 add(int32x16 a, int32x16 b)
    {
        return _mm512_add_epi32(a, b);
    }

    static inline int32x16 sub(int32x16 a, int32x16 b)
    {
        return _mm512_sub_epi32(a, b);
    }

    static inline int32x16 mullo(int32x16 a, int32x16 b)
    {
        return _mm512_mullo_epi32(a, b);
    }

    // bitwise

    static inline int32x16 bitwise_nand(int32x16 a, int32x16 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline int32x16 bitwise_and(int32x16 a, int32x16 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline int32x16 bitwise_or(int32x16 a, int32x16 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline int32x16 bitwise_xor(int32x16 a, int32x16 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline int32x16 bitwise_not(int32x16 a)
    {
        return simd512_not(a);
    }

    // compare

    static inline mask32x16 compare_eq(int32x16 a, int32x16 b)
    {
        return _mm512_cmp_epi32_mask(a, b, 0);
    }

    static inline mask32x16 compare_gt(int32x16 a, int32x16 b)
    {
        return _mm512_cmp_epi32_mask(b, a, 1);
    }

    static inline int32x16 select(mask32x16 mask, int32x16 a, int32x16 b)
    {
        return _mm512_mask_blend_epi32(mask, b, a);
    }

    // shift by constant

    template <int Count>
    static inline int32x16 slli(int32x16 a)
    {
        return _mm512_slli_epi32(a, Count);
    }

    template <int Count>
    static inline int32x16 srli(int32x16 a)
    {
        return _mm512_srli_epi32(a, Count);
    }

    template <int Count>
    static inline int32x16 srai(int32x16 a)
    {
        return _mm512_srai_epi32(a, Count);
    }

    // shift by scalar

    static inline int32x16 sll(int32x16 a, int count)
    {
        return _mm512_sll_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline int32x16 srl(int32x16 a, int count)
    {
        return _mm512_srl_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline int32x16 sra(int32x16 a, int count)
    {
        return _mm512_sra_epi32(a, _mm_cvtsi32_si128(count));
    }

    // shift by vector

    static inline int32x16 sll(int32x16 a, uint32x16 count)
    {
        return _mm512_sllv_epi32(a, count);
    }

    static inline int32x16 srl(int32x16 a, uint32x16 count)
    {
        return _mm512_srlv_epi32(a, count);
    }

    static inline int32x16 sra(int32x16 a, uint32x16 count)
    {
        return _mm512_srav_epi32(a, count);
    }

    static inline int32x16 min(int32x16 a, int32x16 b)
    {
        return _mm512_min_epi32(a, b);
    }

    static inline int32x16 max(int32x16 a, int32x16 b)
    {
        return _mm512_max_epi32(a, b);
    }

    // -----------------------------------------------------------------
    // int64x8
    // -----------------------------------------------------------------

    static inline int64x8 int64x8_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline int64x8 int64x8_set1(s64 s)
    {
        return _mm512_set1_epi64(s);
    }

    static inline int64x8 int64x8_set8(s64 s0, s64 s1, s64 s2, s64 s3, s64 s4, s64 s5, s64 s6, s64 s7)
    {
        return _mm512_setr_epi64(s0, s1, s2, s3, s4, s5, s6, s7);
    }

    static inline int64x8 unpacklo(int64x8 a, int64x8 b)
    {
        return _mm512_unpacklo_epi64(a, b);
    }

    static inline int64x8 unpackhi(int64x8 a, int64x8 b)
    {
        return _mm512_unpackhi_epi64(a, b);
    }

    static inline int64x8 add(int64x8 a, int64x8 b)
    {
        return _mm512_add_epi64(a, b);
    }

    static inline int64x8 sub(int64x8 a, int64x8 b)
    {
        return _mm512_sub_epi64(a, b);
    }

    static inline int64x8 bitwise_nand(int64x8 a, int64x8 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline int64x8 bitwise_and(int64x8 a, int64x8 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline int64x8 bitwise_or(int64x8 a, int64x8 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline int64x8 bitwise_xor(int64x8 a, int64x8 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline int64x8 bitwise_not(int64x8 a)
    {
        return simd512_not(a);
    }

    static inline int64x8 select(mask64x8 mask, int64x8 a, int64x8 b)
    {
        return _mm512_mask_blend_epi64(mask, b, a);
    }

    // shift by constant

    template <int Count>
    static inline int64x8 slli(int64x8 a)
    {
        return _mm512_slli_epi64(a, Count);
    }

    template <int Count>
    static inline int64x8 srli(int64x8 a)
    {
        return _mm512_srli_epi64(a, Count);
    }

    // shift by scalar

    static inline int64x8 sll(int64x8 a, int count)
    {
        return _mm512_sll_epi64(a, _mm_cvtsi32_si128(count));
    }

    static inline int64x8 srl(int64x8 a, int count)
    {
        return _mm512_srl_epi64(a, _mm_cvtsi32_si128(count));
    }

    // -----------------------------------------------------------------
    // mask8x64
    // -----------------------------------------------------------------

#if !defined(MANGO_COMPILER_MICROSOFT)

    static inline mask8x64 operator & (mask8x64 a, mask8x64 b)
    {
        return _mm512_kand(a, b);
    }

    static inline mask8x64 operator | (mask8x64 a, mask8x64 b)
    {
        return _mm512_kor(a, b);
    }

    static inline mask8x64 operator ^ (mask8x64 a, mask8x64 b)
    {
        return _mm512_kxor(a, b);
    }

#endif

    static inline u64 get_mask(mask8x64 a)
    {
        return u64(a);
    }

    static inline bool none_of(mask8x64 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask8x64 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask8x64 a)
    {
        return get_mask(a) == 0xffffffffffffffffull;
    }

    // -----------------------------------------------------------------
    // mask16x32
    // -----------------------------------------------------------------

#if !defined(MANGO_COMPILER_MICROSOFT)

    static inline mask16x32 operator & (mask16x32 a, mask16x32 b)
    {
        return _mm512_kand(a, b);
    }

    static inline mask16x32 operator | (mask16x32 a, mask16x32 b)
    {
        return _mm512_kor(a, b);
    }

    static inline mask16x32 operator ^ (mask16x32 a, mask16x32 b)
    {
        return _mm512_kxor(a, b);
    }

#endif

    static inline u32 get_mask(mask16x32 a)
    {
        return u32(a);
    }

    static inline bool none_of(mask16x32 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask16x32 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask16x32 a)
    {
        return get_mask(a) == 0xffffffff;
    }

    // -----------------------------------------------------------------
    // mask32x16
    // -----------------------------------------------------------------

#if !defined(MANGO_COMPILER_MICROSOFT)

    static inline mask32x16 operator & (mask32x16 a, mask32x16 b)
    {
        return _mm512_kand(a, b);
    }

    static inline mask32x16 operator | (mask32x16 a, mask32x16 b)
    {
        return _mm512_kor(a, b);
    }

    static inline mask32x16 operator ^ (mask32x16 a, mask32x16 b)
    {
        return _mm512_kxor(a, b);
    }

#endif

    static inline u32 get_mask(mask32x16 a)
    {
        return u32(a);
    }

    static inline bool none_of(mask32x16 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask32x16 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask32x16 a)
    {
        return get_mask(a) == 0xffff;
    }

    // -----------------------------------------------------------------
    // mask64x8
    // -----------------------------------------------------------------

#if !defined(MANGO_COMPILER_MICROSOFT)

    static inline mask64x8 operator & (mask64x8 a, mask64x8 b)
    {
        return _mm512_kand(a, b);
    }

    static inline mask64x8 operator | (mask64x8 a, mask64x8 b)
    {
        return _mm512_kor(a, b);
    }

    static inline mask64x8 operator ^ (mask64x8 a, mask64x8 b)
    {
        return _mm512_kxor(a, b);
    }

#endif

    static inline u32 get_mask(mask64x8 a)
    {
        return u32(a);
    }

    static inline bool none_of(mask64x8 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask64x8 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask64x8 a)
    {
        return get_mask(a) == 0xff;
    }

} // namespace simd
} // namespace mango
