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

    static inline __m256i simd256_mullo_epi8(__m256i a, __m256i b)
    {
        const __m256i temp0 = _mm256_mullo_epi16(a, b);
        const __m256i temp1 = _mm256_mullo_epi16(_mm256_srli_epi16(a, 8),_mm256_srli_epi16(b, 8));
        return _mm256_or_si256(_mm256_slli_epi16(temp1, 8), _mm256_srli_epi16(_mm256_slli_epi16(temp0, 8), 8));
    }

    static inline __m256i _mm256_not_si256(__m256i a)
    {
        return _mm256_xor_si256(a, _mm256_cmpeq_epi8(a, a));
    }

    static inline __m256i _mm256_select_si256(__m256i mask, __m256i a, __m256i b)
    {
        return _mm256_or_si256(_mm256_and_si256(mask, a), _mm256_andnot_si256(mask, b));
    }

    // -----------------------------------------------------------------
    // uint8x32
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint8x32 set_component(uint8x32 a, uint8 b)
    {
        static_assert(Index < 32, "Index out of range.");
        return _mm256_insert_epi8(a, b, Index);
    }

    template <unsigned int Index>
    static inline uint8 get_component(uint8x32 a)
    {
        static_assert(Index < 32, "Index out of range.");
        return _mm256_extract_epi8(a, Index);
    }

    static inline uint8x32 uint8x32_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline uint8x32 uint8x32_set1(uint8 s)
    {
        return _mm256_set1_epi8(s);
    }

    static inline uint8x32 unpacklo(uint8x32 a, uint8x32 b)
    {
        return _mm256_unpacklo_epi8(a, b);
    }

    static inline uint8x32 unpackhi(uint8x32 a, uint8x32 b)
    {
        return _mm256_unpackhi_epi8(a, b);
    }

    static inline uint8x32 add(uint8x32 a, uint8x32 b)
    {
        return _mm256_add_epi8(a, b);
    }

    static inline uint8x32 sub(uint8x32 a, uint8x32 b)
    {
        return _mm256_sub_epi8(a, b);
    }

    static inline uint8x32 mullo(uint8x32 a, uint8x32 b)
    {
        return simd256_mullo_epi8(a, b);
    }

    // saturated

    static inline uint8x32 adds(uint8x32 a, uint8x32 b)
    {
        return _mm256_adds_epu8(a, b);
    }

    static inline uint8x32 subs(uint8x32 a, uint8x32 b)
    {
        return _mm256_subs_epu8(a, b);
    }

    // bitwise

    static inline uint8x32 bitwise_nand(uint8x32 a, uint8x32 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline uint8x32 bitwise_and(uint8x32 a, uint8x32 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline uint8x32 bitwise_or(uint8x32 a, uint8x32 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline uint8x32 bitwise_xor(uint8x32 a, uint8x32 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline uint8x32 bitwise_not(uint8x32 a)
    {
        return _mm256_not_si256(a);
    }

    // compare

    static inline uint8x32::mask compare_eq(uint8x32 a, uint8x32 b)
    {
        return _mm256_cmpeq_epi8(a, b);
    }

    static inline uint8x32::mask compare_gt(uint8x32 a, uint8x32 b)
    {
        const __m256i sign = _mm256_set1_epi32(0x80808080);
        return _mm256_cmpgt_epi8(_mm256_xor_si256(a, sign), _mm256_xor_si256(b, sign));
    }

    static inline uint8x32 select(uint8x32::mask mask, uint8x32 a, uint8x32 b)
    {
        return _mm256_select_si256(mask, a, b);
    }

    static inline uint8x32 min(uint8x32 a, uint8x32 b)
    {
        return _mm256_min_epu8(a, b);
    }

    static inline uint8x32 max(uint8x32 a, uint8x32 b)
    {
        return _mm256_max_epu8(a, b);
    }

    // -----------------------------------------------------------------
    // uint16x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint16x16 set_component(uint16x16 a, uint16 b)
    {
        static_assert(Index < 16, "Index out of range.");
        return _mm256_insert_epi16(a, b, Index);
    }

    template <unsigned int Index>
    static inline uint16 get_component(uint16x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return _mm256_extract_epi16(a, Index);
    }

    static inline uint16x16 uint16x16_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline uint16x16 uint16x16_set1(uint16 s)
    {
        return _mm256_set1_epi16(s);
    }

    static inline uint16x16 unpacklo(uint16x16 a, uint16x16 b)
    {
        return _mm256_unpacklo_epi16(a, b);
    }

    static inline uint16x16 unpackhi(uint16x16 a, uint16x16 b)
    {
        return _mm256_unpackhi_epi16(a, b);
    }

    static inline uint16x16 add(uint16x16 a, uint16x16 b)
    {
        return _mm256_add_epi16(a, b);
    }

    static inline uint16x16 sub(uint16x16 a, uint16x16 b)
    {
        return _mm256_sub_epi16(a, b);
    }

    static inline uint16x16 mullo(uint16x16 a, uint16x16 b)
    {
        return _mm256_mullo_epi16(a, b);
    }

    // saturated

    static inline uint16x16 adds(uint16x16 a, uint16x16 b)
    {
        return _mm256_adds_epu16(a, b);
    }

    static inline uint16x16 subs(uint16x16 a, uint16x16 b)
    {
        return _mm256_subs_epu16(a, b);
    }

    // bitwise

    static inline uint16x16 bitwise_nand(uint16x16 a, uint16x16 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline uint16x16 bitwise_and(uint16x16 a, uint16x16 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline uint16x16 bitwise_or(uint16x16 a, uint16x16 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline uint16x16 bitwise_xor(uint16x16 a, uint16x16 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline uint16x16 bitwise_not(uint16x16 a)
    {
        return _mm256_not_si256(a);
    }

    // compare

    static inline uint16x16::mask compare_eq(uint16x16 a, uint16x16 b)
    {
        return _mm256_cmpeq_epi16(a, b);
    }

    static inline uint16x16::mask compare_gt(uint16x16 a, uint16x16 b)
    {
        const __m256i sign = _mm256_set1_epi32(0x80008000);
        return _mm256_cmpgt_epi16(_mm256_xor_si256(a, sign), _mm256_xor_si256(b, sign));
    }

    static inline uint16x16 select(uint16x16::mask mask, uint16x16 a, uint16x16 b)
    {
        return _mm256_select_si256(mask, a, b);
    }

    // shift

    template <int Count>
    static inline uint16x16 slli(uint16x16 a)
    {
        return _mm256_slli_epi16(a, Count);
    }

    template <int Count>
    static inline uint16x16 srli(uint16x16 a)
    {
        return _mm256_srli_epi16(a, Count);
    }

    template <int Count>
    static inline uint16x16 srai(uint16x16 a)
    {
        return _mm256_srai_epi16(a, Count);
    }

    static inline uint16x16 sll(uint16x16 a, int count)
    {
        return _mm256_sll_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline uint16x16 srl(uint16x16 a, int count)
    {
        return _mm256_srl_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline uint16x16 sra(uint16x16 a, int count)
    {
        return _mm256_sra_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline uint16x16 min(uint16x16 a, uint16x16 b)
    {
        return _mm256_min_epu16(a, b);
    }

    static inline uint16x16 max(uint16x16 a, uint16x16 b)
    {
        return _mm256_max_epu16(a, b);
    }

    // -----------------------------------------------------------------
    // uint32x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint32x8 set_component(uint32x8 a, uint32 b)
    {
        static_assert(Index < 8, "Index out of range.");
        return _mm256_insert_epi32(a, b, Index);
    }

    template <unsigned int Index>
    static inline uint32 get_component(uint32x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return _mm256_extract_epi32(a, Index);
    }

    static inline uint32x8 uint32x8_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline uint32x8 uint32x8_set1(uint32 s)
    {
        return _mm256_set1_epi32(s);
    }

    static inline uint32x8 uint32x8_set8(uint32 s0, uint32 s1, uint32 s2, uint32 s3, uint32 s4, uint32 s5, uint32 s6, uint32 s7)
    {
        return _mm256_setr_epi32(s0, s1, s2, s3, s4, s5, s6, s7);
    }

    static inline uint32x8 uint32x8_uload(const uint32* source)
    {
        return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(source));
    }

    static inline void uint32x8_ustore(uint32* dest, uint32x8 a)
    {
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), a);
    }

    static inline uint32x8 unpacklo(uint32x8 a, uint32x8 b)
    {
        return _mm256_unpacklo_epi32(a, b);
    }

    static inline uint32x8 unpackhi(uint32x8 a, uint32x8 b)
    {
        return _mm256_unpackhi_epi32(a, b);
    }

    static inline uint32x8 add(uint32x8 a, uint32x8 b)
    {
        return _mm256_add_epi32(a, b);
    }

    static inline uint32x8 sub(uint32x8 a, uint32x8 b)
    {
        return _mm256_sub_epi32(a, b);
    }

    static inline uint32x8 mullo(uint32x8 a, uint32x8 b)
    {
        return _mm256_mullo_epi32(a, b);
    }

    // saturated

    static inline uint32x8 adds(uint32x8 a, uint32x8 b)
    {
  	    const __m256i temp = _mm256_add_epi32(a, b);
  	    return _mm256_or_si256(temp, _mm256_cmpgt_epi32(a, temp));
    }

    static inline uint32x8 subs(uint32x8 a, uint32x8 b)
    {
  	    const __m256i temp = _mm256_sub_epi32(a, b);
  	    return _mm256_and_si256(temp, _mm256_cmpgt_epi32(a, temp));
    }

    // bitwise

    static inline uint32x8 bitwise_nand(uint32x8 a, uint32x8 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline uint32x8 bitwise_and(uint32x8 a, uint32x8 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline uint32x8 bitwise_or(uint32x8 a, uint32x8 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline uint32x8 bitwise_xor(uint32x8 a, uint32x8 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline uint32x8 bitwise_not(uint32x8 a)
    {
        return _mm256_not_si256(a);
    }

    // compare

    static inline uint32x8::mask compare_eq(uint32x8 a, uint32x8 b)
    {
        return _mm256_cmpeq_epi32(a, b);
    }

    static inline uint32x8::mask compare_gt(uint32x8 a, uint32x8 b)
    {
        const __m256i sign = _mm256_set1_epi32(0x80000000);
        return _mm256_cmpgt_epi32(_mm256_xor_si256(a, sign), _mm256_xor_si256(b, sign));
    }

    static inline uint32x8 select(uint32x8::mask mask, uint32x8 a, uint32x8 b)
    {
        return _mm256_select_si256(mask, a, b);
    }

    static inline uint32 get_mask(uint32x8::mask a)
    {
        return _mm256_movemask_ps(_mm256_castsi256_ps(a));
    }

    // shift

    template <int Count>
    static inline uint32x8 slli(uint32x8 a)
    {
        return _mm256_slli_epi32(a, Count);
    }

    template <int Count>
    static inline uint32x8 srli(uint32x8 a)
    {
        return _mm256_srli_epi32(a, Count);
    }

    template <int Count>
    static inline uint32x8 srai(uint32x8 a)
    {
        return _mm256_srai_epi32(a, Count);
    }

    static inline uint32x8 sll(uint32x8 a, int count)
    {
        return _mm256_sll_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline uint32x8 srl(uint32x8 a, int count)
    {
        return _mm256_srl_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline uint32x8 sra(uint32x8 a, int count)
    {
        return _mm256_sra_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline uint32x8 min(uint32x8 a, uint32x8 b)
    {
        return _mm256_min_epu32(a, b);
    }

    static inline uint32x8 max(uint32x8 a, uint32x8 b)
    {
        return _mm256_max_epu32(a, b);
    }

    // -----------------------------------------------------------------
    // uint64x4
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint64x4 set_component(uint64x4 a, uint64 b)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm256_insert_epi64(a, b, Index);
    }

    template <unsigned int Index>
    static inline uint64 get_component(uint64x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm256_extract_epi64(a, Index);
    }

    static inline uint64x4 uint64x4_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline uint64x4 uint64x4_set1(uint64 s)
    {
        return _mm256_set1_epi64x(s);
    }

    static inline uint64x4 uint64x4_set4(uint64 x, uint64 y, uint64 z, uint64 w)
    {
        return _mm256_setr_epi64x(x, y, z, w);
    }

    static inline uint64x4 unpacklo(uint64x4 a, uint64x4 b)
    {
        return _mm256_unpacklo_epi64(a, b);
    }

    static inline uint64x4 unpackhi(uint64x4 a, uint64x4 b)
    {
        return _mm256_unpackhi_epi64(a, b);
    }

    static inline uint64x4 add(uint64x4 a, uint64x4 b)
    {
        return _mm256_add_epi64(a, b);
    }

    static inline uint64x4 sub(uint64x4 a, uint64x4 b)
    {
        return _mm256_sub_epi64(a, b);
    }

    static inline uint64x4 bitwise_nand(uint64x4 a, uint64x4 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline uint64x4 bitwise_and(uint64x4 a, uint64x4 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline uint64x4 bitwise_or(uint64x4 a, uint64x4 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline uint64x4 bitwise_xor(uint64x4 a, uint64x4 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline uint64x4 bitwise_not(uint64x4 a)
    {
        return _mm256_not_si256(a);
    }

    static inline uint64x4 select(uint64x4::mask mask, uint64x4 a, uint64x4 b)
    {
        return _mm256_select_si256(mask, a, b);
    }

    // shift

    template <int Count>
    static inline uint64x4 slli(uint64x4 a)
    {
        return _mm256_slli_epi64(a, Count);
    }

    template <int Count>
    static inline uint64x4 srli(uint64x4 a)
    {
        return _mm256_srli_epi64(a, Count);
    }

    static inline uint64x4 sll(uint64x4 a, int count)
    {
        return _mm256_sll_epi64(a, _mm_cvtsi32_si128(count));
    }

    static inline uint64x4 srl(uint64x4 a, int count)
    {
        return _mm256_srl_epi64(a, _mm_cvtsi32_si128(count));
    }

    // -----------------------------------------------------------------
    // int8x32
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int8x32 set_component(int8x32 a, int8 b)
    {
        static_assert(Index < 32, "Index out of range.");
        return _mm256_insert_epi8(a, b, Index);
    }

    template <unsigned int Index>
    static inline int8 get_component(int8x32 a)
    {
        static_assert(Index < 32, "Index out of range.");
        return _mm256_extract_epi8(a, Index);
    }

    static inline int8x32 int8x32_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline int8x32 int8x32_set1(int8 s)
    {
        return _mm256_set1_epi8(s);
    }

    static inline int8x32 unpacklo(int8x32 a, int8x32 b)
    {
        return _mm256_unpacklo_epi8(a, b);
    }

    static inline int8x32 unpackhi(int8x32 a, int8x32 b)
    {
        return _mm256_unpackhi_epi8(a, b);
    }

    static inline int8x32 add(int8x32 a, int8x32 b)
    {
        return _mm256_add_epi8(a, b);
    }

    static inline int8x32 sub(int8x32 a, int8x32 b)
    {
        return _mm256_sub_epi8(a, b);
    }

    static inline int8x32 mullo(int8x32 a, int8x32 b)
    {
        return simd256_mullo_epi8(a, b);
    }

    // saturated

    static inline int8x32 adds(int8x32 a, int8x32 b)
    {
        return _mm256_adds_epi8(a, b);
    }

    static inline int8x32 subs(int8x32 a, int8x32 b)
    {
        return _mm256_subs_epi8(a, b);
    }

    static inline int8x32 abs(int8x32 a)
    {
        return _mm256_abs_epi8(a);
    }

    static inline int8x32 neg(int8x32 a)
    {
        return _mm256_sub_epi8(_mm256_setzero_si256(), a);
    }

    // bitwise

    static inline int8x32 bitwise_nand(int8x32 a, int8x32 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline int8x32 bitwise_and(int8x32 a, int8x32 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline int8x32 bitwise_or(int8x32 a, int8x32 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline int8x32 bitwise_xor(int8x32 a, int8x32 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline int8x32 bitwise_not(int8x32 a)
    {
        return _mm256_not_si256(a);
    }

    // compare

    static inline int8x32::mask compare_eq(int8x32 a, int8x32 b)
    {
        return _mm256_cmpeq_epi8(a, b);
    }

    static inline int8x32::mask compare_gt(int8x32 a, int8x32 b)
    {
        return _mm256_cmpgt_epi8(a, b);
    }

    static inline int8x32 select(int8x32::mask mask, int8x32 a, int8x32 b)
    {
        return _mm256_select_si256(mask, a, b);
    }

    static inline int8x32 min(int8x32 a, int8x32 b)
    {
        return _mm256_min_epi8(a, b);
    }

    static inline int8x32 max(int8x32 a, int8x32 b)
    {
        return _mm256_max_epi8(a, b);
    }

    // -----------------------------------------------------------------
    // int16x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int16x16 set_component(int16x16 a, int16 b)
    {
        static_assert(Index < 16, "Index out of range.");
        return _mm256_insert_epi16(a, b, Index);
    }

    template <unsigned int Index>
    static inline int16 get_component(int16x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return _mm256_extract_epi16(a, Index);
    }

    static inline int16x16 int16x16_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline int16x16 int16x16_set1(int16 s)
    {
        return _mm256_set1_epi16(s);
    }

    static inline int16x16 unpacklo(int16x16 a, int16x16 b)
    {
        return _mm256_unpacklo_epi16(a, b);
    }

    static inline int16x16 unpackhi(int16x16 a, int16x16 b)
    {
        return _mm256_unpackhi_epi16(a, b);
    }

    static inline int16x16 add(int16x16 a, int16x16 b)
    {
        return _mm256_add_epi16(a, b);
    }

    static inline int16x16 sub(int16x16 a, int16x16 b)
    {
        return _mm256_sub_epi16(a, b);
    }

    static inline int16x16 mullo(int16x16 a, int16x16 b)
    {
        return _mm256_mullo_epi16(a, b);
    }

    // saturated

    static inline int16x16 adds(int16x16 a, int16x16 b)
    {
        return _mm256_adds_epi16(a, b);
    }

    static inline int16x16 subs(int16x16 a, int16x16 b)
    {
        return _mm256_subs_epi16(a, b);
    }

    static inline int16x16 abs(int16x16 a)
    {
        return _mm256_abs_epi16(a);
    }

    static inline int16x16 neg(int16x16 a)
    {
        return _mm256_sub_epi16(_mm256_setzero_si256(), a);
    }

    // bitwise

    static inline int16x16 bitwise_nand(int16x16 a, int16x16 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline int16x16 bitwise_and(int16x16 a, int16x16 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline int16x16 bitwise_or(int16x16 a, int16x16 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline int16x16 bitwise_xor(int16x16 a, int16x16 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline int16x16 bitwise_not(int16x16 a)
    {
        return _mm256_not_si256(a);
    }

    // compare

    static inline int16x16::mask compare_eq(int16x16 a, int16x16 b)
    {
        return _mm256_cmpeq_epi16(a, b);
    }

    static inline int16x16::mask compare_gt(int16x16 a, int16x16 b)
    {
        return _mm256_cmpgt_epi16(a, b);
    }

    static inline int16x16 select(int16x16::mask mask, int16x16 a, int16x16 b)
    {
        return _mm256_select_si256(mask, a, b);
    }

    // shift

    template <int Count>
    static inline int16x16 slli(int16x16 a)
    {
        return _mm256_slli_epi16(a, Count);
    }

    template <int Count>
    static inline int16x16 srli(int16x16 a)
    {
        return _mm256_srli_epi16(a, Count);
    }

    template <int Count>
    static inline int16x16 srai(int16x16 a)
    {
        return _mm256_srai_epi16(a, Count);
    }

    static inline int16x16 sll(int16x16 a, int count)
    {
        return _mm256_sll_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline int16x16 srl(int16x16 a, int count)
    {
        return _mm256_srl_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline int16x16 sra(int16x16 a, int count)
    {
        return _mm256_sra_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline int16x16 min(int16x16 a, int16x16 b)
    {
        return _mm256_min_epi16(a, b);
    }

    static inline int16x16 max(int16x16 a, int16x16 b)
    {
        return _mm256_max_epi16(a, b);
    }

    // -----------------------------------------------------------------
    // int32x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int32x8 set_component(int32x8 a, int32 b)
    {
        static_assert(Index < 8, "Index out of range.");
        return _mm256_insert_epi32(a, b, Index);
    }

    template <unsigned int Index>
    static inline int32 get_component(int32x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return _mm256_extract_epi32(a, Index);
    }

    static inline int32x8 int32x8_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline int32x8 int32x8_set1(int32 s)
    {
        return _mm256_set1_epi32(s);
    }

    static inline int32x8 int32x8_set8(int32 s0, int32 s1, int32 s2, int32 s3, int32 s4, int32 s5, int32 s6, int32 s7)
    {
        return _mm256_setr_epi32(s0, s1, s2, s3, s4, s5, s6, s7);
    }

    static inline int32x8 int32x8_uload(const int32* source)
    {
        return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(source));
    }

    static inline void int32x8_ustore(int32* dest, int32x8 a)
    {
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), a);
    }

    static inline int32x8 unpacklo(int32x8 a, int32x8 b)
    {
        return _mm256_unpacklo_epi32(a, b);
    }

    static inline int32x8 unpackhi(int32x8 a, int32x8 b)
    {
        return _mm256_unpackhi_epi32(a, b);
    }

    static inline int32x8 abs(int32x8 a)
    {
        return _mm256_abs_epi32(a);
    }

    static inline int32x8 neg(int32x8 a)
    {
        return _mm256_sub_epi32(_mm256_setzero_si256(), a);
    }

    static inline int32x8 add(int32x8 a, int32x8 b)
    {
        return _mm256_add_epi32(a, b);
    }

    static inline int32x8 sub(int32x8 a, int32x8 b)
    {
        return _mm256_sub_epi32(a, b);
    }

    static inline int32x8 mullo(int32x8 a, int32x8 b)
    {
        return _mm256_mullo_epi32(a, b);
    }

    // saturated

    static inline int32x8 adds(int32x8 a, int32x8 b)
    {
        const __m256i v = _mm256_add_epi32(a, b);
        a = _mm256_srai_epi32(a, 31);
        __m256i temp = _mm256_xor_si256(b, v);
        temp = _mm256_xor_si256(temp, _mm256_cmpeq_epi32(temp, temp));
        temp = _mm256_or_si256(temp, _mm256_xor_si256(a, b));
        return _mm256_select_si256(_mm256_cmpgt_epi32(_mm256_setzero_si256(), temp), v, a);
    }

    static inline int32x8 subs(int32x8 a, int32x8 b)
    {
        const __m256i v = _mm256_sub_epi32(a, b);
        a = _mm256_srai_epi32(a, 31);
        __m256i temp = _mm256_and_si256(_mm256_xor_si256(a, b), _mm256_xor_si256(a, v));
        return _mm256_select_si256(_mm256_cmpgt_epi32(_mm256_setzero_si256(), temp), a, v);
    }

    // bitwise

    static inline int32x8 bitwise_nand(int32x8 a, int32x8 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline int32x8 bitwise_and(int32x8 a, int32x8 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline int32x8 bitwise_or(int32x8 a, int32x8 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline int32x8 bitwise_xor(int32x8 a, int32x8 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline int32x8 bitwise_not(int32x8 a)
    {
        return _mm256_not_si256(a);
    }

    // compare

    static inline int32x8::mask compare_eq(int32x8 a, int32x8 b)
    {
        return _mm256_cmpeq_epi32(a, b);
    }

    static inline int32x8::mask compare_gt(int32x8 a, int32x8 b)
    {
        return _mm256_cmpgt_epi32(a, b);
    }

    static inline int32x8 select(int32x8::mask mask, int32x8 a, int32x8 b)
    {
        return _mm256_select_si256(mask, a, b);
    }

    static inline uint32 get_mask(int32x8::mask a)
    {
        return _mm256_movemask_ps(_mm256_castsi256_ps(a));
    }

    // shift

    template <int Count>
    static inline int32x8 slli(int32x8 a)
    {
        return _mm256_slli_epi32(a, Count);
    }

    template <int Count>
    static inline int32x8 srli(int32x8 a)
    {
        return _mm256_srli_epi32(a, Count);
    }

    template <int Count>
    static inline int32x8 srai(int32x8 a)
    {
        return _mm256_srai_epi32(a, Count);
    }

    static inline int32x8 sll(int32x8 a, int count)
    {
        return _mm256_sll_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline int32x8 srl(int32x8 a, int count)
    {
        return _mm256_srl_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline int32x8 sra(int32x8 a, int count)
    {
        return _mm256_sra_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline int32x8 min(int32x8 a, int32x8 b)
    {
        return _mm256_min_epi32(a, b);
    }

    static inline int32x8 max(int32x8 a, int32x8 b)
    {
        return _mm256_max_epi32(a, b);
    }

    // -----------------------------------------------------------------
    // int64x4
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int64x4 set_component(int64x4 a, int64 b)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm256_insert_epi64(a, b, Index);
    }

    template <unsigned int Index>
    static inline int64 get_component(int64x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm256_extract_epi64(a, Index);
    }

    static inline int64x4 int64x4_zero()
    {
        return _mm256_setzero_si256();
    }

    static inline int64x4 int64x4_set1(int64 s)
    {
        return _mm256_set1_epi64x(s);
    }

    static inline int64x4 int64x4_set4(int64 x, int64 y, int64 z, int64 w)
    {
        return _mm256_setr_epi64x(x, y, z, w);
    }

    static inline int64x4 unpacklo(int64x4 a, int64x4 b)
    {
        return _mm256_unpacklo_epi64(a, b);
    }

    static inline int64x4 unpackhi(int64x4 a, int64x4 b)
    {
        return _mm256_unpackhi_epi64(a, b);
    }

    static inline int64x4 add(int64x4 a, int64x4 b)
    {
        return _mm256_add_epi64(a, b);
    }

    static inline int64x4 sub(int64x4 a, int64x4 b)
    {
        return _mm256_sub_epi64(a, b);
    }

    static inline int64x4 bitwise_nand(int64x4 a, int64x4 b)
    {
        return _mm256_andnot_si256(a, b);
    }

    static inline int64x4 bitwise_and(int64x4 a, int64x4 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline int64x4 bitwise_or(int64x4 a, int64x4 b)
    {
        return _mm256_or_si256(a, b);
    }

    static inline int64x4 bitwise_xor(int64x4 a, int64x4 b)
    {
        return _mm256_xor_si256(a, b);
    }

    static inline int64x4 bitwise_not(int64x4 a)
    {
        return _mm256_not_si256(a);
    }

    static inline int64x4 select(int64x4::mask mask, int64x4 a, int64x4 b)
    {
        return _mm256_select_si256(mask, a, b);
    }

    // shift

    template <int Count>
    static inline int64x4 slli(int64x4 a)
    {
        return _mm256_slli_epi64(a, Count);
    }

    template <int Count>
    static inline int64x4 srli(int64x4 a)
    {
        return _mm256_srli_epi64(a, Count);
    }

    static inline int64x4 sll(int64x4 a, int count)
    {
        return _mm256_sll_epi64(a, _mm_cvtsi32_si128(count));
    }

    static inline int64x4 srl(int64x4 a, int count)
    {
        return _mm256_srl_epi64(a, _mm_cvtsi32_si128(count));
    }

} // namespace simd
} // namespace mango
