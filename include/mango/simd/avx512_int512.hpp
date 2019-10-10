/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // helpers
    // -----------------------------------------------------------------

namespace detail {

    static inline __m512i simd512_not(__m512i a)
    {
        return _mm512_xor_si512(a, _mm512_set1_epi32(0xffffffff));
    }

    static inline __m512i simd512_srli1_epi8(__m512i a)
    {
        a = _mm512_srli_epi16(a, 1);
        return _mm512_and_si512(a, _mm512_set1_epi32(0x7f7f7f7f));
    }

    static inline __m512i simd512_srli7_epi8(__m512i a)
    {
        a = _mm512_srli_epi16(a, 7);
        return _mm512_and_si512(a, _mm512_set1_epi32(0x01010101));
    }

    static inline __m512i simd512_srai1_epi8(__m512i a)
    {
        __m512i b = _mm512_slli_epi16(a, 8);
        a = _mm512_srai_epi16(a, 1);
        b = _mm512_srai_epi16(b, 1);
        a = _mm512_and_si512(a, _mm512_set1_epi32(0xff00ff00));
        b = _mm512_srli_epi16(b, 8);
        return _mm512_or_si512(a, b);
    }

    static inline __m512i simd512_srai1_epi64(__m512i a)
    {
        __m512i sign = _mm512_and_si512(a, _mm512_set1_epi64(0x8000000000000000ull));
        a = _mm512_or_si512(sign, _mm512_srli_epi64(a, 1));
        return a;
    }

} // namespace detail

    // -----------------------------------------------------------------
    // u8x64
    // -----------------------------------------------------------------

    static inline u8x64 u8x64_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline u8x64 u8x64_set1(u8 s)
    {
        return _mm512_set1_epi8(s);
    }

    static inline u8x64 u8x64_uload(const u8* source)
    {
        return _mm512_loadu_si512(source);
    }

    static inline void u8x64_ustore(u8* dest, u8x64 a)
    {
        _mm512_storeu_si512(dest, a);
    }

    static inline u8x64 unpacklo(u8x64 a, u8x64 b)
    {
        return _mm512_unpacklo_epi8(a, b);
    }

    static inline u8x64 unpackhi(u8x64 a, u8x64 b)
    {
        return _mm512_unpackhi_epi8(a, b);
    }

    static inline u8x64 add(u8x64 a, u8x64 b)
    {
        return _mm512_add_epi8(a, b);
    }

    static inline u8x64 sub(u8x64 a, u8x64 b)
    {
        return _mm512_sub_epi8(a, b);
    }

    static inline u8x64 adds(u8x64 a, u8x64 b)
    {
        return _mm512_adds_epu8(a, b);
    }

    static inline u8x64 subs(u8x64 a, u8x64 b)
    {
        return _mm512_subs_epu8(a, b);
    }

    static inline u8x64 avg(u8x64 a, u8x64 b)
    {
        __m512i axb = _mm512_xor_si512(a, b);
        __m512i temp = _mm512_add_epi8(_mm512_and_si512(a, b), detail::simd512_srli1_epi8(axb));
        return temp;
    }

    static inline u8x64 ravg(u8x64 a, u8x64 b)
    {
        return _mm512_avg_epu8(a, b);
    }

    // bitwise

    static inline u8x64 bitwise_nand(u8x64 a, u8x64 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline u8x64 bitwise_and(u8x64 a, u8x64 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline u8x64 bitwise_or(u8x64 a, u8x64 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline u8x64 bitwise_xor(u8x64 a, u8x64 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline u8x64 bitwise_not(u8x64 a)
    {
        return detail::simd512_not(a);
    }

    // compare

    static inline mask8x64 compare_eq(u8x64 a, u8x64 b)
    {
        return _mm512_cmp_epu8_mask(a, b, 0);
    }

    static inline mask8x64 compare_gt(u8x64 a, u8x64 b)
    {
        return _mm512_cmp_epu8_mask(b, a, 1);
    }

    static inline mask8x64 compare_neq(u8x64 a, u8x64 b)
    {
        return _mm512_cmp_epu8_mask(a, b, 4);
    }

    static inline mask8x64 compare_lt(u8x64 a, u8x64 b)
    {
        return _mm512_cmp_epu8_mask(a, b, 1);
    }

    static inline mask8x64 compare_le(u8x64 a, u8x64 b)
    {
        return _mm512_cmp_epu8_mask(a, b, 2);
    }

    static inline mask8x64 compare_ge(u8x64 a, u8x64 b)
    {
        return _mm512_cmp_epu8_mask(b, a, 2);
    }

    static inline u8x64 select(mask8x64 mask, u8x64 a, u8x64 b)
    {
        return _mm512_mask_blend_epi8(mask, b, a);
    }

    static inline u8x64 min(u8x64 a, u8x64 b)
    {
        return _mm512_min_epu8(a, b);
    }

    static inline u8x64 max(u8x64 a, u8x64 b)
    {
        return _mm512_max_epu8(a, b);
    }

    // -----------------------------------------------------------------
    // u16x32
    // -----------------------------------------------------------------

    static inline u16x32 u16x32_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline u16x32 u16x32_set1(u16 s)
    {
        return _mm512_set1_epi16(s);
    }

    static inline u16x32 u16x32_uload(const u16* source)
    {
        return _mm512_loadu_si512(source);
    }

    static inline void u16x32_ustore(u16* dest, u16x32 a)
    {
        _mm512_storeu_si512(dest, a);
    }

    static inline u16x32 unpacklo(u16x32 a, u16x32 b)
    {
        return _mm512_unpacklo_epi16(a, b);
    }

    static inline u16x32 unpackhi(u16x32 a, u16x32 b)
    {
        return _mm512_unpackhi_epi16(a, b);
    }

    static inline u16x32 add(u16x32 a, u16x32 b)
    {
        return _mm512_add_epi16(a, b);
    }

    static inline u16x32 sub(u16x32 a, u16x32 b)
    {
        return _mm512_sub_epi16(a, b);
    }

    static inline u16x32 adds(u16x32 a, u16x32 b)
    {
        return _mm512_adds_epu16(a, b);
    }

    static inline u16x32 subs(u16x32 a, u16x32 b)
    {
        return _mm512_subs_epu16(a, b);
    }

    static inline u16x32 avg(u16x32 a, u16x32 b)
    {
        __m512i axb = _mm512_xor_si512(a, b);
        __m512i temp = _mm512_add_epi16(_mm512_and_si512(a, b), _mm512_srli_epi16(axb, 1));
        return temp;
    }

    static inline u16x32 ravg(u16x32 a, u16x32 b)
    {
        return _mm512_avg_epu16(a, b);
    }

    static inline u16x32 mullo(u16x32 a, u16x32 b)
    {
        return _mm512_mullo_epi16(a, b);
    }

    // bitwise

    static inline u16x32 bitwise_nand(u16x32 a, u16x32 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline u16x32 bitwise_and(u16x32 a, u16x32 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline u16x32 bitwise_or(u16x32 a, u16x32 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline u16x32 bitwise_xor(u16x32 a, u16x32 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline u16x32 bitwise_not(u16x32 a)
    {
        return detail::simd512_not(a);
    }

    // compare

    static inline mask16x32 compare_eq(u16x32 a, u16x32 b)
    {
        return _mm512_cmp_epu16_mask(a, b, 0);
    }

    static inline mask16x32 compare_gt(u16x32 a, u16x32 b)
    {
        return _mm512_cmp_epu16_mask(b, a, 1);
    }

    static inline mask16x32 compare_neq(u16x32 a, u16x32 b)
    {
        return _mm512_cmp_epu16_mask(a, b, 4);
    }

    static inline mask16x32 compare_lt(u16x32 a, u16x32 b)
    {
        return _mm512_cmp_epu16_mask(a, b, 1);
    }

    static inline mask16x32 compare_le(u16x32 a, u16x32 b)
    {
        return _mm512_cmp_epu16_mask(a, b, 2);
    }

    static inline mask16x32 compare_ge(u16x32 a, u16x32 b)
    {
        return _mm512_cmp_epu16_mask(b, a, 2);
    }

    static inline u16x32 select(mask16x32 mask, u16x32 a, u16x32 b)
    {
        return _mm512_mask_blend_epi16(mask, b, a);
    }

    static inline u16x32 min(u16x32 a, u16x32 b)
    {
        return _mm512_min_epu16(a, b);
    }

    static inline u16x32 max(u16x32 a, u16x32 b)
    {
        return _mm512_max_epu16(a, b);
    }

    // shift by constant

    template <int Count>
    static inline u16x32 slli(u16x32 a)
    {
        return _mm512_slli_epi16(a, Count);
    }

    template <int Count>
    static inline u16x32 srli(u16x32 a)
    {
        return _mm512_srli_epi16(a, Count);
    }

    template <int Count>
    static inline u16x32 srai(u16x32 a)
    {
        return _mm512_srai_epi16(a, Count);
    }

    // shift by scalar

    static inline u16x32 sll(u16x32 a, int count)
    {
        return _mm512_sll_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline u16x32 srl(u16x32 a, int count)
    {
        return _mm512_srl_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline u16x32 sra(u16x32 a, int count)
    {
        return _mm512_sra_epi16(a, _mm_cvtsi32_si128(count));
    }

    // -----------------------------------------------------------------
    // u32x16
    // -----------------------------------------------------------------

    static inline u32x16 u32x16_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline u32x16 u32x16_set1(u32 s)
    {
        return _mm512_set1_epi32(s);
    }

    static inline u32x16 u32x16_set16(u32 s0, u32 s1, u32 s2, u32 s3, u32 s4, u32 s5, u32 s6, u32 s7,
        u32 s8, u32 s9, u32 s10, u32 s11, u32 s12, u32 s13, u32 s14, u32 s15)
    {
        return _mm512_setr_epi32(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15);
    }

    static inline u32x16 u32x16_uload(const u32* source)
    {
        return _mm512_loadu_si512(source);
    }

    static inline void u32x16_ustore(u32* dest, u32x16 a)
    {
        _mm512_storeu_si512(dest, a);
    }

    static inline u32x16 unpacklo(u32x16 a, u32x16 b)
    {
        return _mm512_unpacklo_epi32(a, b);
    }

    static inline u32x16 unpackhi(u32x16 a, u32x16 b)
    {
        return _mm512_unpackhi_epi32(a, b);
    }

    static inline u32x16 add(u32x16 a, u32x16 b)
    {
        return _mm512_add_epi32(a, b);
    }

    static inline u32x16 sub(u32x16 a, u32x16 b)
    {
        return _mm512_sub_epi32(a, b);
    }

    static inline u32x16 avg(u32x16 a, u32x16 b)
    {
        __m512i axb = _mm512_xor_si512(a, b);
        __m512i temp = _mm512_add_epi32(_mm512_and_si512(a, b), _mm512_srli_epi32(axb, 1));
        return temp;
    }

    static inline u32x16 ravg(u32x16 a, u32x16 b)
    {
        a = add(a, _mm512_set1_epi32(1));
        return avg(a, b);
    }

    static inline u32x16 mullo(u32x16 a, u32x16 b)
    {
        return _mm512_mullo_epi32(a, b);
    }

    // bitwise

    static inline u32x16 bitwise_nand(u32x16 a, u32x16 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline u32x16 bitwise_and(u32x16 a, u32x16 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline u32x16 bitwise_or(u32x16 a, u32x16 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline u32x16 bitwise_xor(u32x16 a, u32x16 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline u32x16 bitwise_not(u32x16 a)
    {
        return detail::simd512_not(a);
    }

    // compare

    static inline mask32x16 compare_eq(u32x16 a, u32x16 b)
    {
        return _mm512_cmp_epu32_mask(a, b, 0);
    }

    static inline mask32x16 compare_gt(u32x16 a, u32x16 b)
    {
        return _mm512_cmp_epu32_mask(b, a, 1);
    }

    static inline mask32x16 compare_neq(u32x16 a, u32x16 b)
    {
        return _mm512_cmp_epu32_mask(a, b, 4);
    }

    static inline mask32x16 compare_lt(u32x16 a, u32x16 b)
    {
        return _mm512_cmp_epu32_mask(a, b, 1);
    }

    static inline mask32x16 compare_le(u32x16 a, u32x16 b)
    {
        return _mm512_cmp_epu32_mask(a, b, 2);
    }

    static inline mask32x16 compare_ge(u32x16 a, u32x16 b)
    {
        return _mm512_cmp_epu32_mask(b, a, 2);
    }

    static inline u32x16 select(mask32x16 mask, u32x16 a, u32x16 b)
    {
        return _mm512_mask_blend_epi32(mask, b, a);
    }

    static inline u32x16 min(u32x16 a, u32x16 b)
    {
        return _mm512_min_epu32(a, b);
    }

    static inline u32x16 max(u32x16 a, u32x16 b)
    {
        return _mm512_max_epu32(a, b);
    }

    // shift by constant

    template <int Count>
    static inline u32x16 slli(u32x16 a)
    {
        return _mm512_slli_epi32(a, Count);
    }

    template <int Count>
    static inline u32x16 srli(u32x16 a)
    {
        return _mm512_srli_epi32(a, Count);
    }

    template <int Count>
    static inline u32x16 srai(u32x16 a)
    {
        return _mm512_srai_epi32(a, Count);
    }

    // shift by scalar

    static inline u32x16 sll(u32x16 a, int count)
    {
        return _mm512_sll_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline u32x16 srl(u32x16 a, int count)
    {
        return _mm512_srl_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline u32x16 sra(u32x16 a, int count)
    {
        return _mm512_sra_epi32(a, _mm_cvtsi32_si128(count));
    }

    // shift by vector

    static inline u32x16 sll(u32x16 a, u32x16 count)
    {
        return _mm512_sllv_epi32(a, count);
    }

    static inline u32x16 srl(u32x16 a, u32x16 count)
    {
        return _mm512_srlv_epi32(a, count);
    }

    static inline u32x16 sra(u32x16 a, u32x16 count)
    {
        return _mm512_srav_epi32(a, count);
    }

    // -----------------------------------------------------------------
    // u64x8
    // -----------------------------------------------------------------

    static inline u64x8 u64x8_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline u64x8 u64x8_set1(u64 s)
    {
        return _mm512_set1_epi64(s);
    }

    static inline u64x8 u64x8_set8(u64 s0, u64 s1, u64 s2, u64 s3, u64 s4, u64 s5, u64 s6, u64 s7)
    {
        return _mm512_setr_epi64(s0, s1, s2, s3, s4, s5, s6, s7);
    }

    static inline u64x8 u64x8_uload(const u64* source)
    {
        return _mm512_loadu_si512(source);
    }

    static inline void u64x8_ustore(u64* dest, u64x8 a)
    {
        _mm512_storeu_si512(dest, a);
    }

    static inline u64x8 unpacklo(u64x8 a, u64x8 b)
    {
        return _mm512_unpacklo_epi64(a, b);
    }

    static inline u64x8 unpackhi(u64x8 a, u64x8 b)
    {
        return _mm512_unpackhi_epi64(a, b);
    }

    static inline u64x8 add(u64x8 a, u64x8 b)
    {
        return _mm512_add_epi64(a, b);
    }

    static inline u64x8 sub(u64x8 a, u64x8 b)
    {
        return _mm512_sub_epi64(a, b);
    }

    static inline u64x8 avg(u64x8 a, u64x8 b)
    {
        __m512i axb = _mm512_xor_si512(a, b);
        __m512i temp = _mm512_add_epi64(_mm512_and_si512(a, b), _mm512_srli_epi64(axb, 1));
        return temp;
    }

    static inline u64x8 ravg(u64x8 a, u64x8 b)
    {
        a = add(a, _mm512_set1_epi64(1));
        return avg(a, b);
    }

    // bitwise

    static inline u64x8 bitwise_nand(u64x8 a, u64x8 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline u64x8 bitwise_and(u64x8 a, u64x8 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline u64x8 bitwise_or(u64x8 a, u64x8 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline u64x8 bitwise_xor(u64x8 a, u64x8 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline u64x8 bitwise_not(u64x8 a)
    {
        return detail::simd512_not(a);
    }

    // compare

    static inline mask64x8 compare_eq(u64x8 a, u64x8 b)
    {
        return _mm512_cmp_epu64_mask(a, b, 0);
    }

    static inline mask64x8 compare_gt(u64x8 a, u64x8 b)
    {
        return _mm512_cmp_epu64_mask(b, a, 1);
    }

    static inline mask64x8 compare_neq(u64x8 a, u64x8 b)
    {
        return _mm512_cmp_epu64_mask(a, b, 4);
    }

    static inline mask64x8 compare_lt(u64x8 a, u64x8 b)
    {
        return _mm512_cmp_epu64_mask(a, b, 1);
    }

    static inline mask64x8 compare_le(u64x8 a, u64x8 b)
    {
        return _mm512_cmp_epu64_mask(a, b, 2);
    }

    static inline mask64x8 compare_ge(u64x8 a, u64x8 b)
    {
        return _mm512_cmp_epu64_mask(b, a, 2);
    }

    static inline u64x8 select(mask64x8 mask, u64x8 a, u64x8 b)
    {
        return _mm512_mask_blend_epi64(mask, b, a);
    }

    static inline u64x8 min(u64x8 a, u64x8 b)
    {
        return _mm512_min_epu64(a, b);
    }

    static inline u64x8 max(u64x8 a, u64x8 b)
    {
        return _mm512_max_epu64(a, b);
    }

    // shift by constant

    template <int Count>
    static inline u64x8 slli(u64x8 a)
    {
        return _mm512_slli_epi64(a, Count);
    }

    template <int Count>
    static inline u64x8 srli(u64x8 a)
    {
        return _mm512_srli_epi64(a, Count);
    }

    // shift by scalar

    static inline u64x8 sll(u64x8 a, int count)
    {
        return _mm512_sll_epi64(a, _mm_cvtsi32_si128(count));
    }

    static inline u64x8 srl(u64x8 a, int count)
    {
        return _mm512_srl_epi64(a, _mm_cvtsi32_si128(count));
    }

    // -----------------------------------------------------------------
    // s8x64
    // -----------------------------------------------------------------

    static inline s8x64 s8x64_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline s8x64 s8x64_set1(s8 s)
    {
        return _mm512_set1_epi8(s);
    }

    static inline s8x64 s8x64_uload(const s8* source)
    {
        return _mm512_loadu_si512(source);
    }

    static inline void s8x64_ustore(s8* dest, s8x64 a)
    {
        _mm512_storeu_si512(dest, a);
    }

    static inline s8x64 unpacklo(s8x64 a, s8x64 b)
    {
        return _mm512_unpacklo_epi8(a, b);
    }

    static inline s8x64 unpackhi(s8x64 a, s8x64 b)
    {
        return _mm512_unpackhi_epi8(a, b);
    }

    static inline s8x64 add(s8x64 a, s8x64 b)
    {
        return _mm512_add_epi8(a, b);
    }

    static inline s8x64 sub(s8x64 a, s8x64 b)
    {
        return _mm512_sub_epi8(a, b);
    }

    static inline s8x64 adds(s8x64 a, s8x64 b)
    {
        return _mm512_adds_epi8(a, b);
    }

    static inline s8x64 subs(s8x64 a, s8x64 b)
    {
        return _mm512_subs_epi8(a, b);
    }

    static inline s8x64 avg(s8x64 a, s8x64 b)
    {
        __m512i axb = _mm512_xor_si512(a, b);
        __m512i temp = _mm512_add_epi8(_mm512_and_si512(a, b), detail::simd512_srai1_epi8(axb));
        temp = _mm512_add_epi8(temp, _mm512_and_si512(detail::simd512_srli7_epi8(temp), axb));
        return temp;
    }

    static inline s8x64 ravg(s8x64 a, s8x64 b)
    {
        a = add(a, _mm512_set1_epi8(1));
        return avg(a, b);
    }

    static inline s8x64 abs(s8x64 a)
    {
        return _mm512_abs_epi8(a);
    }

    static inline s8x64 neg(s8x64 a)
    {
        return _mm512_sub_epi8(_mm512_setzero_si512(), a);
    }

    // bitwise

    static inline s8x64 bitwise_nand(s8x64 a, s8x64 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline s8x64 bitwise_and(s8x64 a, s8x64 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline s8x64 bitwise_or(s8x64 a, s8x64 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline s8x64 bitwise_xor(s8x64 a, s8x64 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline s8x64 bitwise_not(s8x64 a)
    {
        return detail::simd512_not(a);
    }

    // compare

    static inline mask8x64 compare_eq(s8x64 a, s8x64 b)
    {
        return _mm512_cmp_epi8_mask(a, b, 0);
    }

    static inline mask8x64 compare_gt(s8x64 a, s8x64 b)
    {
        return _mm512_cmp_epi8_mask(b, a, 1);
    }

    static inline mask8x64 compare_neq(s8x64 a, s8x64 b)
    {
        return _mm512_cmp_epi8_mask(a, b, 4);
    }

    static inline mask8x64 compare_lt(s8x64 a, s8x64 b)
    {
        return _mm512_cmp_epi8_mask(a, b, 1);
    }

    static inline mask8x64 compare_le(s8x64 a, s8x64 b)
    {
        return _mm512_cmp_epi8_mask(a, b, 2);
    }

    static inline mask8x64 compare_ge(s8x64 a, s8x64 b)
    {
        return _mm512_cmp_epi8_mask(b, a, 2);
    }

    static inline s8x64 select(mask8x64 mask, s8x64 a, s8x64 b)
    {
        return _mm512_mask_blend_epi8(mask, b, a);
    }

    static inline s8x64 min(s8x64 a, s8x64 b)
    {
        return _mm512_min_epi8(a, b);
    }

    static inline s8x64 max(s8x64 a, s8x64 b)
    {
        return _mm512_max_epi8(a, b);
    }

    // -----------------------------------------------------------------
    // s16x32
    // -----------------------------------------------------------------

    static inline s16x32 s16x32_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline s16x32 s16x32_set1(s16 s)
    {
        return _mm512_set1_epi16(s);
    }

    static inline s16x32 s16x32_uload(const s16* source)
    {
        return _mm512_loadu_si512(source);
    }

    static inline void s16x32_ustore(s16* dest, s16x32 a)
    {
        _mm512_storeu_si512(dest, a);
    }

    static inline s16x32 unpacklo(s16x32 a, s16x32 b)
    {
        return _mm512_unpacklo_epi16(a, b);
    }

    static inline s16x32 unpackhi(s16x32 a, s16x32 b)
    {
        return _mm512_unpackhi_epi16(a, b);
    }

    static inline s16x32 add(s16x32 a, s16x32 b)
    {
        return _mm512_add_epi16(a, b);
    }

    static inline s16x32 sub(s16x32 a, s16x32 b)
    {
        return _mm512_sub_epi16(a, b);
    }

    static inline s16x32 adds(s16x32 a, s16x32 b)
    {
        return _mm512_adds_epi16(a, b);
    }

    static inline s16x32 subs(s16x32 a, s16x32 b)
    {
        return _mm512_subs_epi16(a, b);
    }

    static inline s16x32 avg(s16x32 a, s16x32 b)
    {
        __m512i axb = _mm512_xor_si512(a, b);
        __m512i temp = _mm512_add_epi16(_mm512_and_si512(a, b), _mm512_srai_epi16(axb, 1));
        temp = _mm512_add_epi16(temp, _mm512_and_si512(_mm512_srli_epi16(temp, 15), axb));
        return temp;
    }

    static inline s16x32 ravg(s16x32 a, s16x32 b)
    {
        a = add(a, _mm512_set1_epi16(1));
        return avg(a, b);
    }

    static inline s16x32 mullo(s16x32 a, s16x32 b)
    {
        return _mm512_mullo_epi16(a, b);
    }

    static inline s16x32 abs(s16x32 a)
    {
        return _mm512_abs_epi16(a);
    }

    static inline s16x32 neg(s16x32 a)
    {
        return _mm512_sub_epi16(_mm512_setzero_si512(), a);
    }

    // bitwise

    static inline s16x32 bitwise_nand(s16x32 a, s16x32 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline s16x32 bitwise_and(s16x32 a, s16x32 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline s16x32 bitwise_or(s16x32 a, s16x32 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline s16x32 bitwise_xor(s16x32 a, s16x32 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline s16x32 bitwise_not(s16x32 a)
    {
        return detail::simd512_not(a);
    }

    // compare

    static inline mask16x32 compare_eq(s16x32 a, s16x32 b)
    {
        return _mm512_cmp_epi16_mask(a, b, 0);
    }

    static inline mask16x32 compare_gt(s16x32 a, s16x32 b)
    {
        return _mm512_cmp_epi16_mask(b, a, 1);
    }

    static inline mask16x32 compare_neq(s16x32 a, s16x32 b)
    {
        return _mm512_cmp_epi16_mask(a, b, 4);
    }

    static inline mask16x32 compare_lt(s16x32 a, s16x32 b)
    {
        return _mm512_cmp_epi16_mask(a, b, 1);
    }

    static inline mask16x32 compare_le(s16x32 a, s16x32 b)
    {
        return _mm512_cmp_epi16_mask(a, b, 2);
    }

    static inline mask16x32 compare_ge(s16x32 a, s16x32 b)
    {
        return _mm512_cmp_epi16_mask(b, a, 2);
    }

    static inline s16x32 select(mask16x32 mask, s16x32 a, s16x32 b)
    {
        return _mm512_mask_blend_epi16(mask, b, a);
    }

    static inline s16x32 min(s16x32 a, s16x32 b)
    {
        return _mm512_min_epi16(a, b);
    }

    static inline s16x32 max(s16x32 a, s16x32 b)
    {
        return _mm512_max_epi16(a, b);
    }

    // shift by constant

    template <int Count>
    static inline s16x32 slli(s16x32 a)
    {
        return _mm512_slli_epi16(a, Count);
    }

    template <int Count>
    static inline s16x32 srli(s16x32 a)
    {
        return _mm512_srli_epi16(a, Count);
    }

    template <int Count>
    static inline s16x32 srai(s16x32 a)
    {
        return _mm512_srai_epi16(a, Count);
    }

    // shift by scalar

    static inline s16x32 sll(s16x32 a, int count)
    {
        return _mm512_sll_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline s16x32 srl(s16x32 a, int count)
    {
        return _mm512_srl_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline s16x32 sra(s16x32 a, int count)
    {
        return _mm512_sra_epi16(a, _mm_cvtsi32_si128(count));
    }

    // -----------------------------------------------------------------
    // s32x16
    // -----------------------------------------------------------------

    static inline s32x16 s32x16_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline s32x16 s32x16_set1(s32 s)
    {
        return _mm512_set1_epi32(s);
    }

    static inline s32x16 s32x16_set16(s32 v0, s32 v1, s32 v2, s32 v3, s32 v4, s32 v5, s32 v6, s32 v7,
        s32 v8, s32 v9, s32 v10, s32 v11, s32 v12, s32 v13, s32 v14, s32 v15)
    {
        return _mm512_setr_epi32(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15);
    }

    static inline s32x16 s32x16_uload(const s32* source)
    {
        return _mm512_loadu_si512(source);
    }

    static inline void s32x16_ustore(s32* dest, s32x16 a)
    {
        _mm512_storeu_si512(dest, a);
    }

    static inline s32x16 unpacklo(s32x16 a, s32x16 b)
    {
        return _mm512_unpacklo_epi32(a, b);
    }

    static inline s32x16 unpackhi(s32x16 a, s32x16 b)
    {
        return _mm512_unpackhi_epi32(a, b);
    }

    static inline s32x16 abs(s32x16 a)
    {
        return _mm512_abs_epi32(a);
    }

    static inline s32x16 neg(s32x16 a)
    {
        return _mm512_sub_epi32(_mm512_setzero_si512(), a);
    }

    static inline s32x16 add(s32x16 a, s32x16 b)
    {
        return _mm512_add_epi32(a, b);
    }

    static inline s32x16 sub(s32x16 a, s32x16 b)
    {
        return _mm512_sub_epi32(a, b);
    }

    static inline s32x16 avg(s32x16 a, s32x16 b)
    {
        __m512i axb = _mm512_xor_si512(a, b);
        __m512i temp = _mm512_add_epi32(_mm512_and_si512(a, b), _mm512_srai_epi32(axb, 1));
        temp = _mm512_add_epi32(temp, _mm512_and_si512(_mm512_srli_epi32(temp, 31), axb));
        return temp;
    }

    static inline s32x16 ravg(s32x16 a, s32x16 b)
    {
        a = add(a, _mm512_set1_epi32(1));
        return avg(a, b);
    }

    static inline s32x16 mullo(s32x16 a, s32x16 b)
    {
        return _mm512_mullo_epi32(a, b);
    }

    // bitwise

    static inline s32x16 bitwise_nand(s32x16 a, s32x16 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline s32x16 bitwise_and(s32x16 a, s32x16 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline s32x16 bitwise_or(s32x16 a, s32x16 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline s32x16 bitwise_xor(s32x16 a, s32x16 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline s32x16 bitwise_not(s32x16 a)
    {
        return detail::simd512_not(a);
    }

    // compare

    static inline mask32x16 compare_eq(s32x16 a, s32x16 b)
    {
        return _mm512_cmp_epi32_mask(a, b, 0);
    }

    static inline mask32x16 compare_gt(s32x16 a, s32x16 b)
    {
        return _mm512_cmp_epi32_mask(b, a, 1);
    }

    static inline mask32x16 compare_neq(s32x16 a, s32x16 b)
    {
        return _mm512_cmp_epi32_mask(a, b, 4);
    }

    static inline mask32x16 compare_lt(s32x16 a, s32x16 b)
    {
        return _mm512_cmp_epi32_mask(a, b, 1);
    }

    static inline mask32x16 compare_le(s32x16 a, s32x16 b)
    {
        return _mm512_cmp_epi32_mask(a, b, 2);
    }

    static inline mask32x16 compare_ge(s32x16 a, s32x16 b)
    {
        return _mm512_cmp_epi32_mask(b, a, 2);
    }

    static inline s32x16 select(mask32x16 mask, s32x16 a, s32x16 b)
    {
        return _mm512_mask_blend_epi32(mask, b, a);
    }

    static inline s32x16 min(s32x16 a, s32x16 b)
    {
        return _mm512_min_epi32(a, b);
    }

    static inline s32x16 max(s32x16 a, s32x16 b)
    {
        return _mm512_max_epi32(a, b);
    }

    // shift by constant

    template <int Count>
    static inline s32x16 slli(s32x16 a)
    {
        return _mm512_slli_epi32(a, Count);
    }

    template <int Count>
    static inline s32x16 srli(s32x16 a)
    {
        return _mm512_srli_epi32(a, Count);
    }

    template <int Count>
    static inline s32x16 srai(s32x16 a)
    {
        return _mm512_srai_epi32(a, Count);
    }

    // shift by scalar

    static inline s32x16 sll(s32x16 a, int count)
    {
        return _mm512_sll_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline s32x16 srl(s32x16 a, int count)
    {
        return _mm512_srl_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline s32x16 sra(s32x16 a, int count)
    {
        return _mm512_sra_epi32(a, _mm_cvtsi32_si128(count));
    }

    // shift by vector

    static inline s32x16 sll(s32x16 a, u32x16 count)
    {
        return _mm512_sllv_epi32(a, count);
    }

    static inline s32x16 srl(s32x16 a, u32x16 count)
    {
        return _mm512_srlv_epi32(a, count);
    }

    static inline s32x16 sra(s32x16 a, u32x16 count)
    {
        return _mm512_srav_epi32(a, count);
    }

    // -----------------------------------------------------------------
    // s64x8
    // -----------------------------------------------------------------

    static inline s64x8 s64x8_zero()
    {
        return _mm512_setzero_si512();
    }

    static inline s64x8 s64x8_set1(s64 s)
    {
        return _mm512_set1_epi64(s);
    }

    static inline s64x8 s64x8_set8(s64 s0, s64 s1, s64 s2, s64 s3, s64 s4, s64 s5, s64 s6, s64 s7)
    {
        return _mm512_setr_epi64(s0, s1, s2, s3, s4, s5, s6, s7);
    }

    static inline s64x8 s64x8_uload(const s64* source)
    {
        return _mm512_loadu_si512(source);
    }

    static inline void s64x8_ustore(s64* dest, s64x8 a)
    {
        _mm512_storeu_si512(dest, a);
    }

    static inline s64x8 unpacklo(s64x8 a, s64x8 b)
    {
        return _mm512_unpacklo_epi64(a, b);
    }

    static inline s64x8 unpackhi(s64x8 a, s64x8 b)
    {
        return _mm512_unpackhi_epi64(a, b);
    }

    static inline s64x8 add(s64x8 a, s64x8 b)
    {
        return _mm512_add_epi64(a, b);
    }

    static inline s64x8 sub(s64x8 a, s64x8 b)
    {
        return _mm512_sub_epi64(a, b);
    }

    static inline s64x8 avg(s64x8 a, s64x8 b)
    {
        __m512i axb = _mm512_xor_si512(a, b);
        __m512i temp = _mm512_add_epi64(_mm512_and_si512(a, b), detail::simd512_srai1_epi64(axb));
        temp = _mm512_add_epi64(temp, _mm512_and_si512(_mm512_srli_epi64(temp, 63), axb));
        return temp;
    }

    static inline s64x8 ravg(s64x8 a, s64x8 b)
    {
        a = add(a, _mm512_set1_epi64(1));
        return avg(a, b);
    }

    // bitwise

    static inline s64x8 bitwise_nand(s64x8 a, s64x8 b)
    {
        return _mm512_andnot_si512(a, b);
    }

    static inline s64x8 bitwise_and(s64x8 a, s64x8 b)
    {
        return _mm512_and_si512(a, b);
    }

    static inline s64x8 bitwise_or(s64x8 a, s64x8 b)
    {
        return _mm512_or_si512(a, b);
    }

    static inline s64x8 bitwise_xor(s64x8 a, s64x8 b)
    {
        return _mm512_xor_si512(a, b);
    }

    static inline s64x8 bitwise_not(s64x8 a)
    {
        return detail::simd512_not(a);
    }

    // compare

    static inline mask64x8 compare_eq(s64x8 a, s64x8 b)
    {
        return _mm512_cmp_epi64_mask(a, b, 0);
    }

    static inline mask64x8 compare_gt(s64x8 a, s64x8 b)
    {
        return _mm512_cmp_epi64_mask(b, a, 1);
    }

    static inline mask64x8 compare_neq(s64x8 a, s64x8 b)
    {
        return _mm512_cmp_epi64_mask(a, b, 4);
    }

    static inline mask64x8 compare_lt(s64x8 a, s64x8 b)
    {
        return _mm512_cmp_epi64_mask(a, b, 1);
    }

    static inline mask64x8 compare_le(s64x8 a, s64x8 b)
    {
        return _mm512_cmp_epi64_mask(a, b, 2);
    }

    static inline mask64x8 compare_ge(s64x8 a, s64x8 b)
    {
        return _mm512_cmp_epi64_mask(b, a, 2);
    }

    static inline s64x8 select(mask64x8 mask, s64x8 a, s64x8 b)
    {
        return _mm512_mask_blend_epi64(mask, b, a);
    }

    static inline s64x8 min(s64x8 a, s64x8 b)
    {
        return _mm512_min_epi64(a, b);
    }

    static inline s64x8 max(s64x8 a, s64x8 b)
    {
        return _mm512_max_epi64(a, b);
    }

    // shift by constant

    template <int Count>
    static inline s64x8 slli(s64x8 a)
    {
        return _mm512_slli_epi64(a, Count);
    }

    template <int Count>
    static inline s64x8 srli(s64x8 a)
    {
        return _mm512_srli_epi64(a, Count);
    }

    // shift by scalar

    static inline s64x8 sll(s64x8 a, int count)
    {
        return _mm512_sll_epi64(a, _mm_cvtsi32_si128(count));
    }

    static inline s64x8 srl(s64x8 a, int count)
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

    static inline mask8x64 operator ! (mask8x64 a)
    {
        return _mm512_knot(a);
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

    static inline mask16x32 operator ! (mask16x32 a)
    {
        return _mm512_knot(a);
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

    static inline mask32x16 operator ! (mask32x16 a)
    {
        return _mm512_knot(a);
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

    static inline mask64x8 operator ! (mask64x8 a)
    {
        return _mm512_knot(a);
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
