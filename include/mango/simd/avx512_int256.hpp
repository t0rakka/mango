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

    static inline __m256i simd256_not_si256(__m256i a)
    {
        return _mm256_xor_si256(a, _mm256_cmpeq_epi8(a, a));
    }

    static inline __m256i simd256_select_si256(__m256i mask, __m256i a, __m256i b)
    {
        return _mm256_blendv_epi8(b, a, mask);
    }

    static inline __m256i simd256_srli1_epi8(__m256i a)
    {
        a = _mm256_srli_epi16(a, 1);
        return _mm256_and_si256(a, _mm256_set1_epi32(0x7f7f7f7f));
    }

    static inline __m256i simd256_srli7_epi8(__m256i a)
    {
        a = _mm256_srli_epi16(a, 7);
        return _mm256_and_si256(a, _mm256_set1_epi32(0x01010101));
    }

    static inline __m256i simd256_srai1_epi8(__m256i a)
    {
        __m256i b = _mm256_slli_epi16(a, 8);
        a = _mm256_srai_epi16(a, 1);
        b = _mm256_srai_epi16(b, 1);
        a = _mm256_and_si256(a, _mm256_set1_epi32(0xff00ff00));
        b = _mm256_srli_epi16(b, 8);
        return _mm256_or_si256(a, b);
    }

    static inline __m256i simd256_srai1_epi64(__m256i a)
    {
        __m256i sign = _mm256_and_si256(a, _mm256_set1_epi64x(0x8000000000000000ull));
        a = _mm256_or_si256(sign, _mm256_srli_epi64(a, 1));
        return a;
    }

} // namespace detail

    // -----------------------------------------------------------------
    // u8x32
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u8x32 set_component(u8x32 a, u8 b)
    {
        static_assert(Index < 32, "Index out of range.");
        return _mm256_insert_epi8(a, b, Index);
    }

    template <unsigned int Index>
    static inline u8 get_component(u8x32 a)
    {
        static_assert(Index < 32, "Index out of range.");
        return _mm256_extract_epi8(a, Index);
    }

    static inline u8x32 u8x32_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline u8x32 u8x32_set1(u8 s)
    {
        return _mm256_set1_epi8(s);
    }

    static inline u8x32 unpacklo(u8x32 a, u8x32 b)
    {
        return _mm256_unpacklo_epi8(a, b);
    }

    static inline u8x32 unpackhi(u8x32 a, u8x32 b)
    {
        return _mm256_unpackhi_epi8(a, b);
    }

    static inline u8x32 add(u8x32 a, u8x32 b)
    {
        return _mm256_add_epi8(a, b);
    }

    static inline u8x32 sub(u8x32 a, u8x32 b)
    {
        return _mm256_sub_epi8(a, b);
    }

    static inline u8x32 adds(u8x32 a, u8x32 b)
    {
        return _mm256_adds_epu8(a, b);
    }

    static inline u8x32 subs(u8x32 a, u8x32 b)
    {
        return _mm256_subs_epu8(a, b);
    }

    static inline u8x32 avg(u8x32 a, u8x32 b)
    {
        __m256i axb = _mm256_xor_si256(a, b);
        __m256i temp = _mm256_add_epi8(_mm256_and_si256(a, b), detail::simd256_srli1_epi8(axb));
        return temp;
    }

    static inline u8x32 ravg(u8x32 a, u8x32 b)
    {
        return _mm256_avg_epu8(a, b);
    }

    // bitwise

    static inline u8x32 bitwise_nand(u8x32 a, u8x32 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline u8x32 bitwise_and(u8x32 a, u8x32 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline u8x32 bitwise_or(u8x32 a, u8x32 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline u8x32 bitwise_xor(u8x32 a, u8x32 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline u8x32 bitwise_not(u8x32 a)
    {
        return detail::simd256_not_si256(a);
    }

    // compare

    static inline mask8x32 compare_eq(u8x32 a, u8x32 b)
    {
        return _mm256_cmp_epu8_mask(a, b, _MM_CMPINT_EQ);
    }

    static inline mask8x32 compare_gt(u8x32 a, u8x32 b)
    {
        return _mm256_cmp_epu8_mask(b, a, _MM_CMPINT_LT);
    }

    static inline mask8x32 compare_neq(u8x32 a, u8x32 b)
    {
        return _mm256_cmp_epu8_mask(a, b, 4);
    }

    static inline mask8x32 compare_lt(u8x32 a, u8x32 b)
    {
        return _mm256_cmp_epu8_mask(a, b, 1);
    }

    static inline mask8x32 compare_le(u8x32 a, u8x32 b)
    {
        return _mm256_cmp_epu8_mask(a, b, 2);
    }

    static inline mask8x32 compare_ge(u8x32 a, u8x32 b)
    {
        return _mm256_cmp_epu8_mask(b, a, 2);
    }

    static inline u8x32 select(mask8x32 mask, u8x32 a, u8x32 b)
    {
        return _mm256_mask_blend_epi8(mask, b, a);
    }

    static inline u8x32 min(u8x32 a, u8x32 b)
    {
        return _mm256_min_epu8(a, b);
    }

    static inline u8x32 max(u8x32 a, u8x32 b)
    {
        return _mm256_max_epu8(a, b);
    }

    // -----------------------------------------------------------------
    // u16x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u16x16 set_component(u16x16 a, u16 b)
    {
        static_assert(Index < 16, "Index out of range.");
        return _mm256_insert_epi16(a, b, Index);
    }

    template <unsigned int Index>
    static inline u16 get_component(u16x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return _mm256_extract_epi16(a, Index);
    }

    static inline u16x16 u16x16_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline u16x16 u16x16_set1(u16 s)
    {
        return _mm256_set1_epi16(s);
    }

    static inline u16x16 unpacklo(u16x16 a, u16x16 b)
    {
        return _mm256_unpacklo_epi16(a, b);
    }

    static inline u16x16 unpackhi(u16x16 a, u16x16 b)
    {
        return _mm256_unpackhi_epi16(a, b);
    }

    static inline u16x16 add(u16x16 a, u16x16 b)
    {
        return _mm256_add_epi16(a, b);
    }

    static inline u16x16 sub(u16x16 a, u16x16 b)
    {
        return _mm256_sub_epi16(a, b);
    }

    static inline u16x16 adds(u16x16 a, u16x16 b)
    {
        return _mm256_adds_epu16(a, b);
    }

    static inline u16x16 subs(u16x16 a, u16x16 b)
    {
        return _mm256_subs_epu16(a, b);
    }

    static inline u16x16 avg(u16x16 a, u16x16 b)
    {
        __m256i axb = _mm256_xor_si256(a, b);
        __m256i temp = _mm256_add_epi16(_mm256_and_si256(a, b), _mm256_srli_epi16(axb, 1));
        return temp;
    }

    static inline u16x16 ravg(u16x16 a, u16x16 b)
    {
        return _mm256_avg_epu16(a, b);
    }

    static inline u16x16 mullo(u16x16 a, u16x16 b)
    {
        return _mm256_mullo_epi16(a, b);
    }

    // bitwise

    static inline u16x16 bitwise_nand(u16x16 a, u16x16 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline u16x16 bitwise_and(u16x16 a, u16x16 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline u16x16 bitwise_or(u16x16 a, u16x16 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline u16x16 bitwise_xor(u16x16 a, u16x16 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline u16x16 bitwise_not(u16x16 a)
    {
        return detail::simd256_not_si256(a);
    }

    // compare

    static inline mask16x16 compare_eq(u16x16 a, u16x16 b)
    {
        return _mm256_cmp_epu16_mask(a, b, _MM_CMPINT_EQ);
    }

    static inline mask16x16 compare_gt(u16x16 a, u16x16 b)
    {
        return _mm256_cmp_epu16_mask(b, a, _MM_CMPINT_LT);
    }

    static inline mask16x16 compare_neq(u16x16 a, u16x16 b)
    {
        return _mm256_cmp_epu16_mask(a, b, 4);
    }

    static inline mask16x16 compare_lt(u16x16 a, u16x16 b)
    {
        return _mm256_cmp_epu16_mask(a, b, 1);
    }

    static inline mask16x16 compare_le(u16x16 a, u16x16 b)
    {
        return _mm256_cmp_epu16_mask(a, b, 2);
    }

    static inline mask16x16 compare_ge(u16x16 a, u16x16 b)
    {
        return _mm256_cmp_epu16_mask(b, a, 2);
    }

    static inline u16x16 select(mask16x16 mask, u16x16 a, u16x16 b)
    {
        return _mm256_mask_blend_epi16(mask, b, a);
    }

    static inline u16x16 min(u16x16 a, u16x16 b)
    {
        return _mm256_min_epu16(a, b);
    }

    static inline u16x16 max(u16x16 a, u16x16 b)
    {
        return _mm256_max_epu16(a, b);
    }

    // shift by constant

    template <int Count>
    static inline u16x16 slli(u16x16 a)
    {
        return _mm256_slli_epi16(a, Count);
    }

    template <int Count>
    static inline u16x16 srli(u16x16 a)
    {
        return _mm256_srli_epi16(a, Count);
    }

    template <int Count>
    static inline u16x16 srai(u16x16 a)
    {
        return _mm256_srai_epi16(a, Count);
    }

    // shift by scalar

    static inline u16x16 sll(u16x16 a, int count)
    {
        return _mm256_sll_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline u16x16 srl(u16x16 a, int count)
    {
        return _mm256_srl_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline u16x16 sra(u16x16 a, int count)
    {
        return _mm256_sra_epi16(a, _mm_cvtsi32_si128(count));
    }

    // -----------------------------------------------------------------
    // u32x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u32x8 set_component(u32x8 a, u32 b)
    {
        static_assert(Index < 8, "Index out of range.");
        return _mm256_insert_epi32(a, b, Index);
    }

    template <unsigned int Index>
    static inline u32 get_component(u32x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return _mm256_extract_epi32(a, Index);
    }

    static inline u32x8 u32x8_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline u32x8 u32x8_set1(u32 s)
    {
        return _mm256_set1_epi32(s);
    }

    static inline u32x8 u32x8_set8(u32 s0, u32 s1, u32 s2, u32 s3, u32 s4, u32 s5, u32 s6, u32 s7)
    {
        return _mm256_setr_epi32(s0, s1, s2, s3, s4, s5, s6, s7);
    }

    static inline u32x8 u32x8_uload(const u32* source)
    {
        return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(source));
    }

    static inline void u32x8_ustore(u32* dest, u32x8 a)
    {
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), a);
    }

    static inline u32x8 unpacklo(u32x8 a, u32x8 b)
    {
        return _mm256_unpacklo_epi32(a, b);
    }

    static inline u32x8 unpackhi(u32x8 a, u32x8 b)
    {
        return _mm256_unpackhi_epi32(a, b);
    }

    static inline u32x8 add(u32x8 a, u32x8 b)
    {
        return _mm256_add_epi32(a, b);
    }

    static inline u32x8 sub(u32x8 a, u32x8 b)
    {
        return _mm256_sub_epi32(a, b);
    }

    static inline u32x8 adds(u32x8 a, u32x8 b)
    {
  	    const __m256i temp = _mm256_add_epi32(a, b);
  	    return _mm256_or_si256(temp, _mm256_cmpgt_epi32(a, temp));
    }

    static inline u32x8 subs(u32x8 a, u32x8 b)
    {
  	    const __m256i temp = _mm256_sub_epi32(a, b);
  	    return _mm256_and_si256(temp, _mm256_cmpgt_epi32(a, temp));
    }

    static inline u32x8 avg(u32x8 a, u32x8 b)
    {
        __m256i axb = _mm256_xor_si256(a, b);
        __m256i temp = _mm256_add_epi32(_mm256_and_si256(a, b), _mm256_srli_epi32(axb, 1));
        return temp;
    }

    static inline u32x8 ravg(u32x8 a, u32x8 b)
    {
        a = add(a, u32x8_set1(1));
        return avg(a, b);
    }

    static inline u32x8 mullo(u32x8 a, u32x8 b)
    {
        return _mm256_mullo_epi32(a, b);
    }

    // bitwise

    static inline u32x8 bitwise_nand(u32x8 a, u32x8 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline u32x8 bitwise_and(u32x8 a, u32x8 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline u32x8 bitwise_or(u32x8 a, u32x8 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline u32x8 bitwise_xor(u32x8 a, u32x8 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline u32x8 bitwise_not(u32x8 a)
    {
        return detail::simd256_not_si256(a);
    }

    // compare

    static inline mask32x8 compare_eq(u32x8 a, u32x8 b)
    {
        return _mm256_cmp_epu32_mask(a, b, _MM_CMPINT_EQ);
    }

    static inline mask32x8 compare_gt(u32x8 a, u32x8 b)
    {
        return _mm256_cmp_epu32_mask(b, a, _MM_CMPINT_LT);
    }

    static inline mask32x8 compare_neq(u32x8 a, u32x8 b)
    {
        return _mm256_cmp_epu32_mask(a, b, 4);
    }

    static inline mask32x8 compare_lt(u32x8 a, u32x8 b)
    {
        return _mm256_cmp_epu32_mask(a, b, 1);
    }

    static inline mask32x8 compare_le(u32x8 a, u32x8 b)
    {
        return _mm256_cmp_epu32_mask(a, b, 2);
    }

    static inline mask32x8 compare_ge(u32x8 a, u32x8 b)
    {
        return _mm256_cmp_epu32_mask(b, a, 2);
    }

    static inline u32x8 select(mask32x8 mask, u32x8 a, u32x8 b)
    {
        return _mm256_mask_blend_epi32(mask, b, a);
    }

    static inline u32x8 min(u32x8 a, u32x8 b)
    {
        return _mm256_min_epu32(a, b);
    }

    static inline u32x8 max(u32x8 a, u32x8 b)
    {
        return _mm256_max_epu32(a, b);
    }

    // shift by constant

    template <int Count>
    static inline u32x8 slli(u32x8 a)
    {
        return _mm256_slli_epi32(a, Count);
    }

    template <int Count>
    static inline u32x8 srli(u32x8 a)
    {
        return _mm256_srli_epi32(a, Count);
    }

    template <int Count>
    static inline u32x8 srai(u32x8 a)
    {
        return _mm256_srai_epi32(a, Count);
    }

    // shift by scalar

    static inline u32x8 sll(u32x8 a, int count)
    {
        return _mm256_sll_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline u32x8 srl(u32x8 a, int count)
    {
        return _mm256_srl_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline u32x8 sra(u32x8 a, int count)
    {
        return _mm256_sra_epi32(a, _mm_cvtsi32_si128(count));
    }

    // shift by vector

    static inline u32x8 sll(u32x8 a, u32x8 count)
    {
        return _mm256_sllv_epi32(a, count);
    }

    static inline u32x8 srl(u32x8 a, u32x8 count)
    {
        return _mm256_srlv_epi32(a, count);
    }

    static inline u32x8 sra(u32x8 a, u32x8 count)
    {
        return _mm256_srav_epi32(a, count);
    }

    // -----------------------------------------------------------------
    // u64x4
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u64x4 set_component(u64x4 a, u64 b)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm256_insert_epi64(a, b, Index);
    }

    template <unsigned int Index>
    static inline u64 get_component(u64x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm256_extract_epi64(a, Index);
    }

    static inline u64x4 u64x4_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline u64x4 u64x4_set1(u64 s)
    {
        return _mm256_set1_epi64x(s);
    }

    static inline u64x4 u64x4_set4(u64 x, u64 y, u64 z, u64 w)
    {
        return _mm256_setr_epi64x(x, y, z, w);
    }

    static inline u64x4 unpacklo(u64x4 a, u64x4 b)
    {
        return _mm256_unpacklo_epi64(a, b);
    }

    static inline u64x4 unpackhi(u64x4 a, u64x4 b)
    {
        return _mm256_unpackhi_epi64(a, b);
    }

    static inline u64x4 add(u64x4 a, u64x4 b)
    {
        return _mm256_add_epi64(a, b);
    }

    static inline u64x4 sub(u64x4 a, u64x4 b)
    {
        return _mm256_sub_epi64(a, b);
    }

    static inline u64x4 avg(u64x4 a, u64x4 b)
    {
        __m256i axb = _mm256_xor_si256(a, b);
        __m256i temp = _mm256_add_epi64(_mm256_and_si256(a, b), _mm256_srli_epi64(axb, 1));
        return temp;
    }

    static inline u64x4 ravg(u64x4 a, u64x4 b)
    {
        a = add(a, u64x4_set1(1));
        return avg(a, b);
    }

    // bitwise

    static inline u64x4 bitwise_nand(u64x4 a, u64x4 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline u64x4 bitwise_and(u64x4 a, u64x4 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline u64x4 bitwise_or(u64x4 a, u64x4 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline u64x4 bitwise_xor(u64x4 a, u64x4 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline u64x4 bitwise_not(u64x4 a)
    {
        return detail::simd256_not_si256(a);
    }

    // compare

    static inline mask64x4 compare_eq(u64x4 a, u64x4 b)
    {
        return _mm256_cmp_epu64_mask(a, b, 0);
    }

    static inline mask64x4 compare_gt(u64x4 a, u64x4 b)
    {
        return _mm256_cmp_epu64_mask(b, a, 1);
    }

    static inline mask64x4 compare_neq(u64x4 a, u64x4 b)
    {
        return _mm256_cmp_epu64_mask(a, b, 4);
    }

    static inline mask64x4 compare_lt(u64x4 a, u64x4 b)
    {
        return _mm256_cmp_epu64_mask(a, b, 1);
    }

    static inline mask64x4 compare_le(u64x4 a, u64x4 b)
    {
        return _mm256_cmp_epu64_mask(a, b, 2);
    }

    static inline mask64x4 compare_ge(u64x4 a, u64x4 b)
    {
        return _mm256_cmp_epu64_mask(b, a, 2);
    }

    static inline u64x4 select(mask64x4 mask, u64x4 a, u64x4 b)
    {
        return _mm256_mask_blend_epi64(mask, b, a);
    }

    static inline u64x4 min(u64x4 a, u64x4 b)
    {
        return _mm256_min_epu64(a, b);
    }

    static inline u64x4 max(u64x4 a, u64x4 b)
    {
        return _mm256_max_epu64(a, b);
    }

    // shift by constant

    template <int Count>
    static inline u64x4 slli(u64x4 a)
    {
        return _mm256_slli_epi64(a, Count);
    }

    template <int Count>
    static inline u64x4 srli(u64x4 a)
    {
        return _mm256_srli_epi64(a, Count);
    }

    // shift by scalar

    static inline u64x4 sll(u64x4 a, int count)
    {
        return _mm256_sll_epi64(a, _mm_cvtsi32_si128(count));
    }

    static inline u64x4 srl(u64x4 a, int count)
    {
        return _mm256_srl_epi64(a, _mm_cvtsi32_si128(count));
    }

    // -----------------------------------------------------------------
    // s8x32
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s8x32 set_component(s8x32 a, s8 b)
    {
        static_assert(Index < 32, "Index out of range.");
        return _mm256_insert_epi8(a, b, Index);
    }

    template <unsigned int Index>
    static inline s8 get_component(s8x32 a)
    {
        static_assert(Index < 32, "Index out of range.");
        return _mm256_extract_epi8(a, Index);
    }

    static inline s8x32 s8x32_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline s8x32 s8x32_set1(s8 s)
    {
        return _mm256_set1_epi8(s);
    }

    static inline s8x32 unpacklo(s8x32 a, s8x32 b)
    {
        return _mm256_unpacklo_epi8(a, b);
    }

    static inline s8x32 unpackhi(s8x32 a, s8x32 b)
    {
        return _mm256_unpackhi_epi8(a, b);
    }

    static inline s8x32 add(s8x32 a, s8x32 b)
    {
        return _mm256_add_epi8(a, b);
    }

    static inline s8x32 sub(s8x32 a, s8x32 b)
    {
        return _mm256_sub_epi8(a, b);
    }

    static inline s8x32 adds(s8x32 a, s8x32 b)
    {
        return _mm256_adds_epi8(a, b);
    }

    static inline s8x32 subs(s8x32 a, s8x32 b)
    {
        return _mm256_subs_epi8(a, b);
    }

    static inline s8x32 avg(s8x32 a, s8x32 b)
    {
        __m256i axb = _mm256_xor_si256(a, b);
        __m256i temp = _mm256_add_epi8(_mm256_and_si256(a, b), detail::simd256_srai1_epi8(axb));
        temp = _mm256_add_epi8(temp, _mm256_and_si256(detail::simd256_srli7_epi8(temp), axb));
        return temp;
    }

    static inline s8x32 ravg(s8x32 a, s8x32 b)
    {
        a = add(a, s8x32_set1(1));
        return avg(a, b);
    }

    static inline s8x32 abs(s8x32 a)
    {
        return _mm256_abs_epi8(a);
    }

    static inline s8x32 neg(s8x32 a)
    {
        return _mm256_sub_epi8(_mm256_setzero_si256(), a);
    }

    // bitwise

    static inline s8x32 bitwise_nand(s8x32 a, s8x32 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline s8x32 bitwise_and(s8x32 a, s8x32 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline s8x32 bitwise_or(s8x32 a, s8x32 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline s8x32 bitwise_xor(s8x32 a, s8x32 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline s8x32 bitwise_not(s8x32 a)
    {
        return detail::simd256_not_si256(a);
    }

    // compare

    static inline mask8x32 compare_eq(s8x32 a, s8x32 b)
    {
        return _mm256_cmp_epi8_mask(a, b, _MM_CMPINT_EQ);
    }

    static inline mask8x32 compare_gt(s8x32 a, s8x32 b)
    {
        return _mm256_cmp_epi8_mask(b, a, _MM_CMPINT_LT);
    }

    static inline mask8x32 compare_neq(s8x32 a, s8x32 b)
    {
        return _mm256_cmp_epi8_mask(a, b, 4);
    }

    static inline mask8x32 compare_lt(s8x32 a, s8x32 b)
    {
        return _mm256_cmp_epi8_mask(a, b, 1);
    }

    static inline mask8x32 compare_le(s8x32 a, s8x32 b)
    {
        return _mm256_cmp_epi8_mask(a, b, 2);
    }

    static inline mask8x32 compare_ge(s8x32 a, s8x32 b)
    {
        return _mm256_cmp_epi8_mask(b, a, 2);
    }

    static inline s8x32 select(mask8x32 mask, s8x32 a, s8x32 b)
    {
        return _mm256_mask_blend_epi8(mask, b, a);
    }

    static inline s8x32 min(s8x32 a, s8x32 b)
    {
        return _mm256_min_epi8(a, b);
    }

    static inline s8x32 max(s8x32 a, s8x32 b)
    {
        return _mm256_max_epi8(a, b);
    }

    // -----------------------------------------------------------------
    // s16x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s16x16 set_component(s16x16 a, s16 b)
    {
        static_assert(Index < 16, "Index out of range.");
        return _mm256_insert_epi16(a, b, Index);
    }

    template <unsigned int Index>
    static inline s16 get_component(s16x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return _mm256_extract_epi16(a, Index);
    }

    static inline s16x16 s16x16_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline s16x16 s16x16_set1(s16 s)
    {
        return _mm256_set1_epi16(s);
    }

    static inline s16x16 unpacklo(s16x16 a, s16x16 b)
    {
        return _mm256_unpacklo_epi16(a, b);
    }

    static inline s16x16 unpackhi(s16x16 a, s16x16 b)
    {
        return _mm256_unpackhi_epi16(a, b);
    }

    static inline s16x16 add(s16x16 a, s16x16 b)
    {
        return _mm256_add_epi16(a, b);
    }

    static inline s16x16 sub(s16x16 a, s16x16 b)
    {
        return _mm256_sub_epi16(a, b);
    }

    static inline s16x16 adds(s16x16 a, s16x16 b)
    {
        return _mm256_adds_epi16(a, b);
    }

    static inline s16x16 subs(s16x16 a, s16x16 b)
    {
        return _mm256_subs_epi16(a, b);
    }

    static inline s16x16 hadd(s16x16 a, s16x16 b)
    {
        return _mm256_hadd_epi16(a, b);
    }

    static inline s16x16 hsub(s16x16 a, s16x16 b)
    {
        return _mm256_hsub_epi16(a, b);
    }

    static inline s16x16 hadds(s16x16 a, s16x16 b)
    {
        return _mm256_hadds_epi16(a, b);
    }

    static inline s16x16 hsubs(s16x16 a, s16x16 b)
    {
        return _mm256_hsubs_epi16(a, b);
    }

    static inline s16x16 avg(s16x16 a, s16x16 b)
    {
        __m256i axb = _mm256_xor_si256(a, b);
        __m256i temp = _mm256_add_epi16(_mm256_and_si256(a, b), _mm256_srai_epi16(axb, 1));
        temp = _mm256_add_epi16(temp, _mm256_and_si256(_mm256_srli_epi16(temp, 15), axb));
        return temp;
    }

    static inline s16x16 ravg(s16x16 a, s16x16 b)
    {
        a = add(a, s16x16_set1(1));
        return avg(a, b);
    }

    static inline s16x16 mullo(s16x16 a, s16x16 b)
    {
        return _mm256_mullo_epi16(a, b);
    }

    static inline s16x16 abs(s16x16 a)
    {
        return _mm256_abs_epi16(a);
    }

    static inline s16x16 neg(s16x16 a)
    {
        return _mm256_sub_epi16(_mm256_setzero_si256(), a);
    }

    // bitwise

    static inline s16x16 bitwise_nand(s16x16 a, s16x16 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline s16x16 bitwise_and(s16x16 a, s16x16 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline s16x16 bitwise_or(s16x16 a, s16x16 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline s16x16 bitwise_xor(s16x16 a, s16x16 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline s16x16 bitwise_not(s16x16 a)
    {
        return detail::simd256_not_si256(a);
    }

    // compare

    static inline mask16x16 compare_eq(s16x16 a, s16x16 b)
    {
        return _mm256_cmp_epi16_mask(a, b, _MM_CMPINT_EQ);
    }

    static inline mask16x16 compare_gt(s16x16 a, s16x16 b)
    {
        return _mm256_cmp_epi16_mask(b, a, _MM_CMPINT_LT);
    }

    static inline mask16x16 compare_neq(s16x16 a, s16x16 b)
    {
        return _mm256_cmp_epi16_mask(a, b, 4);
    }

    static inline mask16x16 compare_lt(s16x16 a, s16x16 b)
    {
        return _mm256_cmp_epi16_mask(a, b, 1);
    }

    static inline mask16x16 compare_le(s16x16 a, s16x16 b)
    {
        return _mm256_cmp_epi16_mask(a, b, 2);
    }

    static inline mask16x16 compare_ge(s16x16 a, s16x16 b)
    {
        return _mm256_cmp_epi16_mask(b, a, 2);
    }

    static inline s16x16 select(mask16x16 mask, s16x16 a, s16x16 b)
    {
        return _mm256_mask_blend_epi16(mask, b, a);
    }

    static inline s16x16 min(s16x16 a, s16x16 b)
    {
        return _mm256_min_epi16(a, b);
    }

    static inline s16x16 max(s16x16 a, s16x16 b)
    {
        return _mm256_max_epi16(a, b);
    }

    // shift by scalar

    template <int Count>
    static inline s16x16 slli(s16x16 a)
    {
        return _mm256_slli_epi16(a, Count);
    }

    template <int Count>
    static inline s16x16 srli(s16x16 a)
    {
        return _mm256_srli_epi16(a, Count);
    }

    template <int Count>
    static inline s16x16 srai(s16x16 a)
    {
        return _mm256_srai_epi16(a, Count);
    }

    // shift by scalar

    static inline s16x16 sll(s16x16 a, int count)
    {
        return _mm256_sll_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline s16x16 srl(s16x16 a, int count)
    {
        return _mm256_srl_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline s16x16 sra(s16x16 a, int count)
    {
        return _mm256_sra_epi16(a, _mm_cvtsi32_si128(count));
    }

    // -----------------------------------------------------------------
    // s32x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s32x8 set_component(s32x8 a, s32 b)
    {
        static_assert(Index < 8, "Index out of range.");
        return _mm256_insert_epi32(a, b, Index);
    }

    template <unsigned int Index>
    static inline s32 get_component(s32x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return _mm256_extract_epi32(a, Index);
    }

    static inline s32x8 s32x8_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline s32x8 s32x8_set1(s32 s)
    {
        return _mm256_set1_epi32(s);
    }

    static inline s32x8 s32x8_set8(s32 s0, s32 s1, s32 s2, s32 s3, s32 s4, s32 s5, s32 s6, s32 s7)
    {
        return _mm256_setr_epi32(s0, s1, s2, s3, s4, s5, s6, s7);
    }

    static inline s32x8 s32x8_uload(const s32* source)
    {
        return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(source));
    }

    static inline void s32x8_ustore(s32* dest, s32x8 a)
    {
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), a);
    }

    static inline s32x8 unpacklo(s32x8 a, s32x8 b)
    {
        return _mm256_unpacklo_epi32(a, b);
    }

    static inline s32x8 unpackhi(s32x8 a, s32x8 b)
    {
        return _mm256_unpackhi_epi32(a, b);
    }

    static inline s32x8 abs(s32x8 a)
    {
        return _mm256_abs_epi32(a);
    }

    static inline s32x8 neg(s32x8 a)
    {
        return _mm256_sub_epi32(_mm256_setzero_si256(), a);
    }

    static inline s32x8 add(s32x8 a, s32x8 b)
    {
        return _mm256_add_epi32(a, b);
    }

    static inline s32x8 sub(s32x8 a, s32x8 b)
    {
        return _mm256_sub_epi32(a, b);
    }

    static inline s32x8 adds(s32x8 a, s32x8 b)
    {
        const __m256i v = _mm256_add_epi32(a, b);
        a = _mm256_srai_epi32(a, 31);
        __m256i temp = _mm256_xor_si256(b, v);
        temp = _mm256_xor_si256(temp, _mm256_cmpeq_epi32(temp, temp));
        temp = _mm256_or_si256(temp, _mm256_xor_si256(a, b));
        return detail::simd256_select_si256(_mm256_cmpgt_epi32(_mm256_setzero_si256(), temp), v, a);
    }

    static inline s32x8 subs(s32x8 a, s32x8 b)
    {
        const __m256i v = _mm256_sub_epi32(a, b);
        a = _mm256_srai_epi32(a, 31);
        __m256i temp = _mm256_and_si256(_mm256_xor_si256(a, b), _mm256_xor_si256(a, v));
        return detail::simd256_select_si256(_mm256_cmpgt_epi32(_mm256_setzero_si256(), temp), a, v);
    }

    static inline s32x8 hadd(s32x8 a, s32x8 b)
    {
        return _mm256_hadd_epi32(a, b);
    }

    static inline s32x8 hsub(s32x8 a, s32x8 b)
    {
        return _mm256_hsub_epi32(a, b);
    }

    static inline s32x8 avg(s32x8 a, s32x8 b)
    {
        __m256i axb = _mm256_xor_si256(a, b);
        __m256i temp = _mm256_add_epi32(_mm256_and_si256(a, b), _mm256_srai_epi32(axb, 1));
        temp = _mm256_add_epi32(temp, _mm256_and_si256(_mm256_srli_epi32(temp, 31), axb));
        return temp;
    }

    static inline s32x8 ravg(s32x8 a, s32x8 b)
    {
        a = add(a, s32x8_set1(1));
        return avg(a, b);
    }

    static inline s32x8 mullo(s32x8 a, s32x8 b)
    {
        return _mm256_mullo_epi32(a, b);
    }

    // bitwise

    static inline s32x8 bitwise_nand(s32x8 a, s32x8 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline s32x8 bitwise_and(s32x8 a, s32x8 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline s32x8 bitwise_or(s32x8 a, s32x8 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline s32x8 bitwise_xor(s32x8 a, s32x8 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline s32x8 bitwise_not(s32x8 a)
    {
        return detail::simd256_not_si256(a);
    }

    // compare

    static inline mask32x8 compare_eq(s32x8 a, s32x8 b)
    {
        return _mm256_cmp_epi32_mask(a, b, _MM_CMPINT_EQ);
    }

    static inline mask32x8 compare_gt(s32x8 a, s32x8 b)
    {
        return _mm256_cmp_epi32_mask(b, a, _MM_CMPINT_LT);
    }

    static inline mask32x8 compare_neq(s32x8 a, s32x8 b)
    {
        return _mm256_cmp_epi32_mask(a, b, 4);
    }

    static inline mask32x8 compare_lt(s32x8 a, s32x8 b)
    {
        return _mm256_cmp_epi32_mask(a, b, 1);
    }

    static inline mask32x8 compare_le(s32x8 a, s32x8 b)
    {
        return _mm256_cmp_epi32_mask(a, b, 2);
    }

    static inline mask32x8 compare_ge(s32x8 a, s32x8 b)
    {
        return _mm256_cmp_epi32_mask(b, a, 2);
    }

    static inline s32x8 select(mask32x8 mask, s32x8 a, s32x8 b)
    {
        return _mm256_mask_blend_epi32(mask, b, a);
    }

    static inline s32x8 min(s32x8 a, s32x8 b)
    {
        return _mm256_min_epi32(a, b);
    }

    static inline s32x8 max(s32x8 a, s32x8 b)
    {
        return _mm256_max_epi32(a, b);
    }

    // shift by constant

    template <int Count>
    static inline s32x8 slli(s32x8 a)
    {
        return _mm256_slli_epi32(a, Count);
    }

    template <int Count>
    static inline s32x8 srli(s32x8 a)
    {
        return _mm256_srli_epi32(a, Count);
    }

    template <int Count>
    static inline s32x8 srai(s32x8 a)
    {
        return _mm256_srai_epi32(a, Count);
    }

    // shift by scalar

    static inline s32x8 sll(s32x8 a, int count)
    {
        return _mm256_sll_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline s32x8 srl(s32x8 a, int count)
    {
        return _mm256_srl_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline s32x8 sra(s32x8 a, int count)
    {
        return _mm256_sra_epi32(a, _mm_cvtsi32_si128(count));
    }

    // shift by vector

    static inline s32x8 sll(s32x8 a, u32x8 count)
    {
        return _mm256_sllv_epi32(a, count);
    }

    static inline s32x8 srl(s32x8 a, u32x8 count)
    {
        return _mm256_srlv_epi32(a, count);
    }

    static inline s32x8 sra(s32x8 a, u32x8 count)
    {
        return _mm256_srav_epi32(a, count);
    }

    // -----------------------------------------------------------------
    // s64x4
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s64x4 set_component(s64x4 a, s64 b)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm256_insert_epi64(a, b, Index);
    }

    template <unsigned int Index>
    static inline s64 get_component(s64x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm256_extract_epi64(a, Index);
    }

    static inline s64x4 s64x4_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline s64x4 s64x4_set1(s64 s)
    {
        return _mm256_set1_epi64x(s);
    }

    static inline s64x4 s64x4_set4(s64 x, s64 y, s64 z, s64 w)
    {
        return _mm256_setr_epi64x(x, y, z, w);
    }

    static inline s64x4 unpacklo(s64x4 a, s64x4 b)
    {
        return _mm256_unpacklo_epi64(a, b);
    }

    static inline s64x4 unpackhi(s64x4 a, s64x4 b)
    {
        return _mm256_unpackhi_epi64(a, b);
    }

    static inline s64x4 add(s64x4 a, s64x4 b)
    {
        return _mm256_add_epi64(a, b);
    }

    static inline s64x4 sub(s64x4 a, s64x4 b)
    {
        return _mm256_sub_epi64(a, b);
    }

    static inline s64x4 avg(s64x4 a, s64x4 b)
    {
        __m256i axb = _mm256_xor_si256(a, b);
        __m256i temp = _mm256_add_epi64(_mm256_and_si256(a, b), detail::simd256_srai1_epi64(axb));
        temp = _mm256_add_epi64(temp, _mm256_and_si256(_mm256_srli_epi64(temp, 63), axb));
        return temp;
    }

    static inline s64x4 ravg(s64x4 a, s64x4 b)
    {
        a = add(a, s64x4_set1(1));
        return avg(a, b);
    }

    // bitwise

    static inline s64x4 bitwise_nand(s64x4 a, s64x4 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline s64x4 bitwise_and(s64x4 a, s64x4 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline s64x4 bitwise_or(s64x4 a, s64x4 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline s64x4 bitwise_xor(s64x4 a, s64x4 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline s64x4 bitwise_not(s64x4 a)
    {
        return detail::simd256_not_si256(a);
    }

    // compare

    static inline mask64x4 compare_eq(s64x4 a, s64x4 b)
    {
        return _mm256_cmp_epi64_mask(a, b, 0);
    }

    static inline mask64x4 compare_gt(s64x4 a, s64x4 b)
    {
        return _mm256_cmp_epi64_mask(b, a, 1);
    }

    static inline mask64x4 compare_neq(s64x4 a, s64x4 b)
    {
        return _mm256_cmp_epi64_mask(a, b, 4);
    }

    static inline mask64x4 compare_lt(s64x4 a, s64x4 b)
    {
        return _mm256_cmp_epi64_mask(a, b, 1);
    }

    static inline mask64x4 compare_le(s64x4 a, s64x4 b)
    {
        return _mm256_cmp_epi64_mask(a, b, 2);
    }

    static inline mask64x4 compare_ge(s64x4 a, s64x4 b)
    {
        return _mm256_cmp_epi64_mask(b, a, 2);
    }

    static inline s64x4 select(mask64x4 mask, s64x4 a, s64x4 b)
    {
        return _mm256_mask_blend_epi64(mask, b, a);
    }

    static inline s64x4 min(s64x4 a, s64x4 b)
    {
        return _mm256_min_epi64(a, b);
    }

    static inline s64x4 max(s64x4 a, s64x4 b)
    {
        return _mm256_max_epi64(a, b);
    }

    // shift by constant

    template <int Count>
    static inline s64x4 slli(s64x4 a)
    {
        return _mm256_slli_epi64(a, Count);
    }

    template <int Count>
    static inline s64x4 srli(s64x4 a)
    {
        return _mm256_srli_epi64(a, Count);
    }

    // shift by scalar

    static inline s64x4 sll(s64x4 a, int count)
    {
        return _mm256_sll_epi64(a, _mm_cvtsi32_si128(count));
    }

    static inline s64x4 srl(s64x4 a, int count)
    {
        return _mm256_srl_epi64(a, _mm_cvtsi32_si128(count));
    }

    // -----------------------------------------------------------------
    // mask8x32
    // -----------------------------------------------------------------

#if !defined(MANGO_COMPILER_MICROSOFT)

    static inline mask8x32 operator & (mask8x32 a, mask8x32 b)
    {
        return _mm512_kand(a, b);
    }

    static inline mask8x32 operator | (mask8x32 a, mask8x32 b)
    {
        return _mm512_kor(a, b);
    }

    static inline mask8x32 operator ^ (mask8x32 a, mask8x32 b)
    {
        return _mm512_kxor(a, b);
    }

    static inline mask8x32 operator ! (mask8x32 a)
    {
        return _mm512_knot(a);
    }

#endif

    static inline u32 get_mask(mask8x32 a)
    {
        return u32(a);
    }

    static inline bool none_of(mask8x32 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask8x32 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask8x32 a)
    {
        return get_mask(a) == 0xffffffff;
    }

    // -----------------------------------------------------------------
    // mask16x16
    // -----------------------------------------------------------------

#if !defined(MANGO_COMPILER_MICROSOFT)

    static inline mask16x16 operator & (mask16x16 a, mask16x16 b)
    {
        return _mm512_kand(a, b);
    }

    static inline mask16x16 operator | (mask16x16 a, mask16x16 b)
    {
        return _mm512_kor(a, b);
    }

    static inline mask16x16 operator ^ (mask16x16 a, mask16x16 b)
    {
        return _mm512_kxor(a, b);
    }

    static inline mask16x16 operator ! (mask16x16 a)
    {
        return _mm512_knot(a);
    }

#endif

    static inline u32 get_mask(mask16x16 a)
    {
        return u32(a);
    }

    static inline bool none_of(mask16x16 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask16x16 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask16x16 a)
    {
        return get_mask(a) == 0xffff;
    }

    // -----------------------------------------------------------------
    // mask32x8
    // -----------------------------------------------------------------

#if !defined(MANGO_COMPILER_MICROSOFT)

    static inline mask32x8 operator & (mask32x8 a, mask32x8 b)
    {
        return _mm512_kand(a, b);
    }

    static inline mask32x8 operator | (mask32x8 a, mask32x8 b)
    {
        return _mm512_kor(a, b);
    }

    static inline mask32x8 operator ^ (mask32x8 a, mask32x8 b)
    {
        return _mm512_kxor(a, b);
    }

    static inline mask32x8 operator ! (mask32x8 a)
    {
        return _mm512_knot(a);
    }

#endif

    static inline u32 get_mask(mask32x8 a)
    {
        return u32(a);
    }

    static inline bool none_of(mask32x8 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask32x8 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask32x8 a)
    {
        return get_mask(a) == 0xff;
    }

    // -----------------------------------------------------------------
    // mask64x4
    // -----------------------------------------------------------------

#if !defined(MANGO_COMPILER_MICROSOFT)

    static inline mask64x4 operator & (mask64x4 a, mask64x4 b)
    {
        return _mm512_kand(a, b);
    }

    static inline mask64x4 operator | (mask64x4 a, mask64x4 b)
    {
        return _mm512_kor(a, b);
    }

    static inline mask64x4 operator ^ (mask64x4 a, mask64x4 b)
    {
        return _mm512_kxor(a, b);
    }

    static inline mask64x4 operator ! (mask64x4 a)
    {
        return _mm512_knot(a);
    }

#endif

    static inline u32 get_mask(mask64x4 a)
    {
        return u32(a);
    }

    static inline bool none_of(mask64x4 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask64x4 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask64x4 a)
    {
        return get_mask(a) == 0xf;
    }

} // namespace simd
} // namespace mango
