/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // helpers
    // -----------------------------------------------------------------

#define simd128_shuffle_epi32(a, b, mask) \
    _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), mask))

#define simd128_shuffle_epi64(a, b, mask) \
    _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(a), _mm_castsi128_pd(b), mask))

    static inline __m128i simd128_packus_epi32(__m128i a, __m128i b)
    {
        a = _mm_slli_epi32(a, 16);
        a = _mm_srai_epi32(a, 16);
        b = _mm_slli_epi32(b, 16);
        b = _mm_srai_epi32(b, 16);
        return _mm_packs_epi32(a, b);
    }

    static inline __m128i _mm_not_si128(__m128i a)
    {
        return _mm_xor_si128(a, _mm_cmpeq_epi8(a, a));
    }

    static inline __m128i _mm_select_si128(__m128i mask, __m128i a, __m128i b)
    {
        return _mm_or_si128(_mm_and_si128(mask, a), _mm_andnot_si128(mask, b));
    }

#if defined(__x86_64__)

    static inline __m128i simd128_cvtsi64_si128(int64 a)
    {
        return _mm_cvtsi64_si128(a);
    }

    static inline int64 simd128_cvtsi128_si64(__m128i a)
    {
        return _mm_cvtsi128_si64(a);
    }

#else

    static inline __m128i simd128_cvtsi64_si128(int64 a)
    {
        return _mm_set_epi64x(0, a);
    }

    static inline int64 simd128_cvtsi128_si64(__m128i a)
    {
        uint64 value = _mm_cvtsi128_si32(a);
        value |= uint64(_mm_cvtsi128_si32(simd128_shuffle_epi32(a, a, 0xee))) << 32;
        return value;
    }

#endif

    // -----------------------------------------------------------------
    // uint8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint8x16 set_component(uint8x16 a, uint8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return _mm_insert_epi8(a, s, Index);
    }

    template <unsigned int Index>
    static inline uint8 get_component(uint8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return _mm_extract_epi8(a, Index);
    }

    static inline uint8x16 uint8x16_zero()
    {
        return _mm_setzero_si128();
    }

    static inline uint8x16 uint8x16_set1(uint8 s)
    {
        return _mm_set1_epi8(s);
    }

    static inline uint8x16 uint8x16_set16(
        uint8 s0, uint8 s1, uint8 s2, uint8 s3, uint8 s4, uint8 s5, uint8 s6, uint8 s7,
        uint8 s8, uint8 s9, uint8 s10, uint8 s11, uint8 s12, uint8 s13, uint8 s14, uint8 s15)
    {
        return _mm_setr_epi8(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15);
    }

    static inline uint8x16 uint8x16_load_low(const uint8* source)
    {
        return _mm_loadl_epi64(reinterpret_cast<__m128i const *>(source));
    }

    static inline void uint8x16_store_low(uint8* dest, uint8x16 a)
    {
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dest), a);
    }

    static inline uint8x16 unpacklo(uint8x16 a, uint8x16 b)
    {
        return _mm_unpacklo_epi8(a, b);
    }

    static inline uint8x16 unpackhi(uint8x16 a, uint8x16 b)
    {
        return _mm_unpackhi_epi8(a, b);
    }

    static inline uint8x16 add(uint8x16 a, uint8x16 b)
    {
        return _mm_add_epi8(a, b);
    }

    static inline uint8x16 sub(uint8x16 a, uint8x16 b)
    {
        return _mm_sub_epi8(a, b);
    }

    // saturated

    static inline uint8x16 adds(uint8x16 a, uint8x16 b)
    {
        return _mm_adds_epu8(a, b);
    }

    static inline uint8x16 subs(uint8x16 a, uint8x16 b)
    {
        return _mm_subs_epu8(a, b);
    }

    // bitwise

    static inline uint8x16 bitwise_nand(uint8x16 a, uint8x16 b)
    {
        return _mm_andnot_si128(a, b);
    }

    static inline uint8x16 bitwise_and(uint8x16 a, uint8x16 b)
    {
        return _mm_and_si128(a, b);
    }

    static inline uint8x16 bitwise_or(uint8x16 a, uint8x16 b)
    {
        return _mm_or_si128(a, b);
    }

    static inline uint8x16 bitwise_xor(uint8x16 a, uint8x16 b)
    {
        return _mm_xor_si128(a, b);
    }

    static inline uint8x16 bitwise_not(uint8x16 a)
    {
        return _mm_not_si128(a);
    }

    // compare

    static inline mask8x16 compare_eq(uint8x16 a, uint8x16 b)
    {
        return _mm_cmp_epu8_mask(a, b, _MM_CMPINT_EQ);
    }

    static inline mask8x16 compare_gt(uint8x16 a, uint8x16 b)
    {
        return _mm_cmp_epu8_mask(b, a, _MM_CMPINT_LT);
    }

    static inline mask8x16 compare_neq(uint8x16 a, uint8x16 b)
    {
        return _mm_cmp_epu8_mask(a, b, 4);
    }

    static inline mask8x16 compare_lt(uint8x16 a, uint8x16 b)
    {
        return _mm_cmp_epu8_mask(a, b, _MM_CMPINT_LT);
    }

    static inline mask8x16 compare_le(uint8x16 a, uint8x16 b)
    {
        return _mm_cmp_epu8_mask(a, b, _MM_CMPINT_LE);
    }

    static inline mask8x16 compare_ge(uint8x16 a, uint8x16 b)
    {
        return _mm_cmp_epu8_mask(b, a, _MM_CMPINT_LE);
    }

    static inline uint8x16 select(mask8x16 mask, uint8x16 a, uint8x16 b)
    {
        return _mm_mask_blend_epi8(mask, b, a);
    }

    static inline uint8x16 min(uint8x16 a, uint8x16 b)
    {
        return _mm_min_epu8(a, b);
    }

    static inline uint8x16 max(uint8x16 a, uint8x16 b)
    {
        return _mm_max_epu8(a, b);
    }

    // -----------------------------------------------------------------
    // uint16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint16x8 set_component(uint16x8 a, uint16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return _mm_insert_epi16(a, s, Index);
    }

    template <unsigned int Index>
    static inline uint16 get_component(uint16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return _mm_extract_epi16(a, Index);
    }

    static inline uint16x8 uint16x8_zero()
    {
        return _mm_setzero_si128();
    }

    static inline uint16x8 uint16x8_set1(uint16 s)
    {
        return _mm_set1_epi16(s);
    }

    static inline uint16x8 uint16x8_set8(uint16 s0, uint16 s1, uint16 s2, uint16 s3, uint16 s4, uint16 s5, uint16 s6, uint16 s7)
    {
        return _mm_setr_epi16(s0, s1, s2, s3, s4, s5, s6, s7);
    }

    static inline uint16x8 uint16x8_load_low(const uint16* source)
    {
        return _mm_loadl_epi64(reinterpret_cast<__m128i const *>(source));
    }

    static inline void uint16x8_store_low(uint16* dest, uint16x8 a)
    {
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dest), a);
    }

    static inline uint16x8 unpacklo(uint16x8 a, uint16x8 b)
    {
        return _mm_unpacklo_epi16(a, b);
    }

    static inline uint16x8 unpackhi(uint16x8 a, uint16x8 b)
    {
        return _mm_unpackhi_epi16(a, b);
    }

    static inline uint16x8 add(uint16x8 a, uint16x8 b)
    {
        return _mm_add_epi16(a, b);
    }

    static inline uint16x8 sub(uint16x8 a, uint16x8 b)
    {
        return _mm_sub_epi16(a, b);
    }

    static inline uint16x8 mullo(uint16x8 a, uint16x8 b)
    {
        return _mm_mullo_epi16(a, b);
    }

    // saturated

    static inline uint16x8 adds(uint16x8 a, uint16x8 b)
    {
        return _mm_adds_epu16(a, b);
    }

    static inline uint16x8 subs(uint16x8 a, uint16x8 b)
    {
        return _mm_subs_epu16(a, b);
    }

    // bitwise

    static inline uint16x8 bitwise_nand(uint16x8 a, uint16x8 b)
    {
        return _mm_andnot_si128(a, b);
    }

    static inline uint16x8 bitwise_and(uint16x8 a, uint16x8 b)
    {
        return _mm_and_si128(a, b);
    }

    static inline uint16x8 bitwise_or(uint16x8 a, uint16x8 b)
    {
        return _mm_or_si128(a, b);
    }

    static inline uint16x8 bitwise_xor(uint16x8 a, uint16x8 b)
    {
        return _mm_xor_si128(a, b);
    }

    static inline uint16x8 bitwise_not(uint16x8 a)
    {
        return _mm_not_si128(a);
    }

    // compare

    static inline mask16x8 compare_eq(uint16x8 a, uint16x8 b)
    {
        return _mm_cmp_epu16_mask(a, b, _MM_CMPINT_EQ);
    }

    static inline mask16x8 compare_gt(uint16x8 a, uint16x8 b)
    {
        return _mm_cmp_epu16_mask(b, a, _MM_CMPINT_LT);
    }

    static inline mask16x8 compare_neq(uint16x8 a, uint16x8 b)
    {
        return _mm_cmp_epu16_mask(a, b, 4);
    }

    static inline mask16x8 compare_lt(uint16x8 a, uint16x8 b)
    {
        return _mm_cmp_epu16_mask(a, b, _MM_CMPINT_LT);
    }

    static inline mask16x8 compare_le(uint16x8 a, uint16x8 b)
    {
        return _mm_cmp_epu16_mask(a, b, _MM_CMPINT_LE);
    }

    static inline mask16x8 compare_ge(uint16x8 a, uint16x8 b)
    {
        return _mm_cmp_epu16_mask(b, a, _MM_CMPINT_LE);
    }

    static inline uint16x8 select(mask16x8 mask, uint16x8 a, uint16x8 b)
    {
        return _mm_mask_blend_epi16(mask, b, a);
    }

    // shift by constant

    template <int Count>
    static inline uint16x8 slli(uint16x8 a)
    {
        return _mm_slli_epi16(a, Count);
    }

    template <int Count>
    static inline uint16x8 srli(uint16x8 a)
    {
        return _mm_srli_epi16(a, Count);
    }

    template <int Count>
    static inline uint16x8 srai(uint16x8 a)
    {
        return _mm_srai_epi16(a, Count);
    }

    // shift by scalar

    static inline uint16x8 sll(uint16x8 a, int count)
    {
        return _mm_sll_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline uint16x8 srl(uint16x8 a, int count)
    {
        return _mm_srl_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline uint16x8 sra(uint16x8 a, int count)
    {
        return _mm_sra_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline uint16x8 min(uint16x8 a, uint16x8 b)
    {
        return _mm_min_epu16(a, b);
    }

    static inline uint16x8 max(uint16x8 a, uint16x8 b)
    {
        return _mm_max_epu16(a, b);
    }
    
    // -----------------------------------------------------------------
    // uint32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline uint32x4 shuffle(uint32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return _mm_shuffle_epi32(v, _MM_SHUFFLE(w, z, y, x));
    }

    template <>
    inline uint32x4 shuffle<0, 1, 2, 3>(uint32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline uint32x4 set_component(uint32x4 a, uint32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm_insert_epi32(a, s, Index);
    }

    template <unsigned int Index>
    static inline uint32 get_component(uint32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm_extract_epi32(a, Index);
    }

    static inline uint32x4 uint32x4_zero()
    {
        return _mm_setzero_si128();
    }

    static inline uint32x4 uint32x4_set1(uint32 s)
    {
        return _mm_set1_epi32(s);
    }

    static inline uint32x4 uint32x4_set4(uint32 x, uint32 y, uint32 z, uint32 w)
    {
        return _mm_setr_epi32(x, y, z, w);
    }

    static inline uint32x4 uint32x4_uload(const uint32* source)
    {
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(source));
    }

    static inline void uint32x4_ustore(uint32* dest, uint32x4 a)
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest), a);
    }

    static inline uint32x4 uint32x4_load_low(const uint32* source)
    {
        return _mm_loadl_epi64(reinterpret_cast<__m128i const *>(source));
    }

    static inline void uint32x4_store_low(uint32* dest, uint32x4 a)
    {
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dest), a);
    }

    static inline uint32x4 unpacklo(uint32x4 a, uint32x4 b)
    {
        return _mm_unpacklo_epi32(a, b);
    }

    static inline uint32x4 unpackhi(uint32x4 a, uint32x4 b)
    {
        return _mm_unpackhi_epi32(a, b);
    }

    static inline uint32x4 add(uint32x4 a, uint32x4 b)
    {
        return _mm_add_epi32(a, b);
    }

    static inline uint32x4 sub(uint32x4 a, uint32x4 b)
    {
        return _mm_sub_epi32(a, b);
    }

    static inline uint32x4 mullo(uint32x4 a, uint32x4 b)
    {
        return _mm_mullo_epi32(a, b);
    }

    // saturated

    static inline uint32x4 adds(uint32x4 a, uint32x4 b)
    {
  	    const __m128i temp = _mm_add_epi32(a, b);
  	    return _mm_or_si128(temp, _mm_cmplt_epi32(temp, a));
    }

    static inline uint32x4 subs(uint32x4 a, uint32x4 b)
    {
  	    const __m128i temp = _mm_sub_epi32(a, b);
  	    return _mm_and_si128(temp, _mm_cmpgt_epi32(a, temp));
    }

    // bitwise

    static inline uint32x4 bitwise_nand(uint32x4 a, uint32x4 b)
    {
        return _mm_andnot_si128(a, b);
    }

    static inline uint32x4 bitwise_and(uint32x4 a, uint32x4 b)
    {
        return _mm_and_si128(a, b);
    }

    static inline uint32x4 bitwise_or(uint32x4 a, uint32x4 b)
    {
        return _mm_or_si128(a, b);
    }

    static inline uint32x4 bitwise_xor(uint32x4 a, uint32x4 b)
    {
        return _mm_xor_si128(a, b);
    }

    static inline uint32x4 bitwise_not(uint32x4 a)
    {
        return _mm_not_si128(a);
    }

    // compare

    static inline mask32x4 compare_eq(uint32x4 a, uint32x4 b)
    {
        return _mm_cmp_epu32_mask(a, b, _MM_CMPINT_EQ);
    }

    static inline mask32x4 compare_gt(uint32x4 a, uint32x4 b)
    {
        return _mm_cmp_epu32_mask(b, a, _MM_CMPINT_LT);
    }

    static inline mask32x4 compare_neq(uint32x4 a, uint32x4 b)
    {
        return _mm_cmp_epu32_mask(a, b, 4);
    }

    static inline mask32x4 compare_lt(uint32x4 a, uint32x4 b)
    {
        return _mm_cmp_epu32_mask(a, b, _MM_CMPINT_LT);
    }

    static inline mask32x4 compare_le(uint32x4 a, uint32x4 b)
    {
        return _mm_cmp_epu32_mask(a, b, _MM_CMPINT_LE);
    }

    static inline mask32x4 compare_ge(uint32x4 a, uint32x4 b)
    {
        return _mm_cmp_epu32_mask(b, a, _MM_CMPINT_LE);
    }

    static inline uint32x4 select(mask32x4 mask, uint32x4 a, uint32x4 b)
    {
        return _mm_mask_blend_epi32(mask, b, a);
    }

    // shift by constant

    template <int Count>
    static inline uint32x4 slli(uint32x4 a)
    {
        return _mm_slli_epi32(a, Count);
    }

    template <int Count>
    static inline uint32x4 srli(uint32x4 a)
    {
        return _mm_srli_epi32(a, Count);
    }

    template <int Count>
    static inline uint32x4 srai(uint32x4 a)
    {
        return _mm_srai_epi32(a, Count);
    }

    // shift by scalar

    static inline uint32x4 sll(uint32x4 a, int count)
    {
        return _mm_sll_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline uint32x4 srl(uint32x4 a, int count)
    {
        return _mm_srl_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline uint32x4 sra(uint32x4 a, int count)
    {
        return _mm_sra_epi32(a, _mm_cvtsi32_si128(count));
    }

    // shift by vector

    static inline uint32x4 sll(uint32x4 a, uint32x4 count)
    {
        return _mm_sllv_epi32(a, count);
    }

    static inline uint32x4 srl(uint32x4 a, uint32x4 count)
    {
        return _mm_srlv_epi32(a, count);
    }

    static inline uint32x4 sra(uint32x4 a, uint32x4 count)
    {
        return _mm_srav_epi32(a, count);
    }

    static inline uint32x4 min(uint32x4 a, uint32x4 b)
    {
        return _mm_min_epu32(a, b);
    }

    static inline uint32x4 max(uint32x4 a, uint32x4 b)
    {
        return _mm_max_epu32(a, b);
    }

    // -----------------------------------------------------------------
    // uint64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint64x2 set_component(uint64x2 a, uint64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return _mm_insert_epi64(a, s, Index);
    }

    template <unsigned int Index>
    static inline uint64 get_component(uint64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return _mm_extract_epi64(a, Index);
    }

    static inline uint64x2 uint64x2_zero()
    {
        return _mm_setzero_si128();
    }

    static inline uint64x2 uint64x2_set1(uint64 s)
    {
        return _mm_set1_epi64x(s);
    }

    static inline uint64x2 uint64x2_set2(uint64 x, uint64 y)
    {
        return _mm_set_epi64x(y, x);
    }

    static inline uint64x2 unpacklo(uint64x2 a, uint64x2 b)
    {
        return _mm_unpacklo_epi64(a, b);
    }

    static inline uint64x2 unpackhi(uint64x2 a, uint64x2 b)
    {
        return _mm_unpackhi_epi64(a, b);
    }

    static inline uint64x2 add(uint64x2 a, uint64x2 b)
    {
        return _mm_add_epi64(a, b);
    }

    static inline uint64x2 sub(uint64x2 a, uint64x2 b)
    {
        return _mm_sub_epi64(a, b);
    }

    static inline uint64x2 bitwise_nand(uint64x2 a, uint64x2 b)
    {
        return _mm_andnot_si128(a, b);
    }

    static inline uint64x2 bitwise_and(uint64x2 a, uint64x2 b)
    {
        return _mm_and_si128(a, b);
    }

    static inline uint64x2 bitwise_or(uint64x2 a, uint64x2 b)
    {
        return _mm_or_si128(a, b);
    }

    static inline uint64x2 bitwise_xor(uint64x2 a, uint64x2 b)
    {
        return _mm_xor_si128(a, b);
    }

    static inline uint64x2 bitwise_not(uint64x2 a)
    {
        return _mm_not_si128(a);
    }

    static inline uint64x2 select(mask64x2 mask, uint64x2 a, uint64x2 b)
    {
        return _mm_mask_blend_epi64(mask, b, a);
    }

    // shift by constant

    template <int Count>
    static inline uint64x2 slli(uint64x2 a)
    {
        return _mm_slli_epi64(a, Count);
    }

    template <int Count>
    static inline uint64x2 srli(uint64x2 a)
    {
        return _mm_srli_epi64(a, Count);
    }

    // shift by scalar

    static inline uint64x2 sll(uint64x2 a, int count)
    {
        return _mm_sll_epi64(a, _mm_cvtsi32_si128(count));
    }

    static inline uint64x2 srl(uint64x2 a, int count)
    {
        return _mm_srl_epi64(a, _mm_cvtsi32_si128(count));
    }

    // -----------------------------------------------------------------
    // int8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int8x16 set_component(int8x16 a, int8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return _mm_insert_epi8(a, s, Index);
    }

    template <unsigned int Index>
    static inline int8 get_component(int8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return _mm_extract_epi8(a, Index);
    }

    static inline int8x16 int8x16_zero()
    {
        return _mm_setzero_si128();
    }

    static inline int8x16 int8x16_set1(int8 s)
    {
        return _mm_set1_epi8(s);
    }

    static inline int8x16 int8x16_set16(
        int8 s0, int8 s1, int8 s2, int8 s3, int8 s4, int8 s5, int8 s6, int8 s7,
        int8 s8, int8 s9, int8 s10, int8 s11, int8 s12, int8 s13, int8 s14, int8 s15)
    {
        return _mm_setr_epi8(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15);
    }

    static inline int8x16 int8x16_load_low(const int8* source)
    {
        return _mm_loadl_epi64(reinterpret_cast<__m128i const *>(source));
    }

    static inline void int8x16_store_low(int8* dest, int8x16 a)
    {
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dest), a);
    }

    static inline int8x16 unpacklo(int8x16 a, int8x16 b)
    {
        return _mm_unpacklo_epi8(a, b);
    }

    static inline int8x16 unpackhi(int8x16 a, int8x16 b)
    {
        return _mm_unpackhi_epi8(a, b);
    }

    static inline int8x16 add(int8x16 a, int8x16 b)
    {
        return _mm_add_epi8(a, b);
    }

    static inline int8x16 sub(int8x16 a, int8x16 b)
    {
        return _mm_sub_epi8(a, b);
    }

    // saturated

    static inline int8x16 adds(int8x16 a, int8x16 b)
    {
        return _mm_adds_epi8(a, b);
    }

    static inline int8x16 subs(int8x16 a, int8x16 b)
    {
        return _mm_subs_epi8(a, b);
    }

    static inline int8x16 abs(int8x16 a)
    {
        return _mm_abs_epi8(a);
    }

    static inline int8x16 neg(int8x16 a)
    {
        return _mm_sub_epi8(_mm_setzero_si128(), a);
    }

    // bitwise

    static inline int8x16 bitwise_nand(int8x16 a, int8x16 b)
    {
        return _mm_andnot_si128(a, b);
    }

    static inline int8x16 bitwise_and(int8x16 a, int8x16 b)
    {
        return _mm_and_si128(a, b);
    }

    static inline int8x16 bitwise_or(int8x16 a, int8x16 b)
    {
        return _mm_or_si128(a, b);
    }

    static inline int8x16 bitwise_xor(int8x16 a, int8x16 b)
    {
        return _mm_xor_si128(a, b);
    }

    static inline int8x16 bitwise_not(int8x16 a)
    {
        return _mm_not_si128(a);
    }

    // compare

    static inline mask8x16 compare_eq(int8x16 a, int8x16 b)
    {
        return _mm_cmp_epi8_mask(a, b, _MM_CMPINT_EQ);
    }

    static inline mask8x16 compare_gt(int8x16 a, int8x16 b)
    {
        return _mm_cmp_epi8_mask(b, a, _MM_CMPINT_LT);
    }

    static inline mask8x16 compare_neq(int8x16 a, int8x16 b)
    {
        return _mm_cmp_epi8_mask(a, b, 4);
    }

    static inline mask8x16 compare_lt(int8x16 a, int8x16 b)
    {
        return _mm_cmp_epi8_mask(a, b, _MM_CMPINT_LT);
    }

    static inline mask8x16 compare_le(int8x16 a, int8x16 b)
    {
        return _mm_cmp_epi8_mask(a, b, _MM_CMPINT_LE);
    }

    static inline mask8x16 compare_ge(int8x16 a, int8x16 b)
    {
        return _mm_cmp_epi8_mask(b, a, _MM_CMPINT_LE);
    }

    static inline int8x16 select(mask8x16 mask, int8x16 a, int8x16 b)
    {
        return _mm_mask_blend_epi8(mask, b, a);
    }

    static inline int8x16 min(int8x16 a, int8x16 b)
    {
        return _mm_min_epi8(a, b);
    }

    static inline int8x16 max(int8x16 a, int8x16 b)
    {
        return _mm_max_epi8(a, b);
    }

    // -----------------------------------------------------------------
    // int16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int16x8 set_component(int16x8 a, int16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return _mm_insert_epi16(a, s, Index);
    }

    template <unsigned int Index>
    static inline int16 get_component(int16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return _mm_extract_epi16(a, Index);
    }

    static inline int16x8 int16x8_zero()
    {
        return _mm_setzero_si128();
    }

    static inline int16x8 int16x8_set1(int16 s)
    {
        return _mm_set1_epi16(s);
    }

    static inline int16x8 int16x8_set8(int16 s0, int16 s1, int16 s2, int16 s3, int16 s4, int16 s5, int16 s6, int16 s7)
    {
        return _mm_setr_epi16(s0, s1, s2, s3, s4, s5, s6, s7);
    }

    static inline int16x8 int16x8_load_low(const int16* source)
    {
        return _mm_loadl_epi64(reinterpret_cast<__m128i const *>(source));
    }

    static inline void int16x8_store_low(int16* dest, int16x8 a)
    {
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dest), a);
    }

    static inline int16x8 unpacklo(int16x8 a, int16x8 b)
    {
        return _mm_unpacklo_epi16(a, b);
    }

    static inline int16x8 unpackhi(int16x8 a, int16x8 b)
    {
        return _mm_unpackhi_epi16(a, b);
    }

    static inline int16x8 add(int16x8 a, int16x8 b)
    {
        return _mm_add_epi16(a, b);
    }

    static inline int16x8 sub(int16x8 a, int16x8 b)
    {
        return _mm_sub_epi16(a, b);
    }

    static inline int16x8 mullo(int16x8 a, int16x8 b)
    {
        return _mm_mullo_epi16(a, b);
    }

    // saturated

    static inline int16x8 adds(int16x8 a, int16x8 b)
    {
        return _mm_adds_epi16(a, b);
    }

    static inline int16x8 subs(int16x8 a, int16x8 b)
    {
        return _mm_subs_epi16(a, b);
    }

    static inline int16x8 abs(int16x8 a)
    {
        return _mm_abs_epi16(a);
    }

    static inline int16x8 neg(int16x8 a)
    {
        return _mm_sub_epi16(_mm_setzero_si128(), a);
    }

    // bitwise

    static inline int16x8 bitwise_nand(int16x8 a, int16x8 b)
    {
        return _mm_andnot_si128(a, b);
    }

    static inline int16x8 bitwise_and(int16x8 a, int16x8 b)
    {
        return _mm_and_si128(a, b);
    }

    static inline int16x8 bitwise_or(int16x8 a, int16x8 b)
    {
        return _mm_or_si128(a, b);
    }

    static inline int16x8 bitwise_xor(int16x8 a, int16x8 b)
    {
        return _mm_xor_si128(a, b);
    }

    static inline int16x8 bitwise_not(int16x8 a)
    {
        return _mm_not_si128(a);
    }

    // compare

    static inline mask16x8 compare_eq(int16x8 a, int16x8 b)
    {
        return _mm_cmp_epi16_mask(a, b, _MM_CMPINT_EQ);
    }

    static inline mask16x8 compare_gt(int16x8 a, int16x8 b)
    {
        return _mm_cmp_epi16_mask(b, a, _MM_CMPINT_LT);
    }

    static inline mask16x8 compare_neq(int16x8 a, int16x8 b)
    {
        return _mm_cmp_epi16_mask(a, b, 4);
    }

    static inline mask16x8 compare_lt(int16x8 a, int16x8 b)
    {
        return _mm_cmp_epi16_mask(a, b, _MM_CMPINT_LT);
    }

    static inline mask16x8 compare_le(int16x8 a, int16x8 b)
    {
        return _mm_cmp_epi16_mask(a, b, _MM_CMPINT_LE);
    }

    static inline mask16x8 compare_ge(int16x8 a, int16x8 b)
    {
        return _mm_cmp_epi16_mask(b, a, _MM_CMPINT_LE);
    }

    static inline int16x8 select(mask16x8 mask, int16x8 a, int16x8 b)
    {
        return _mm_mask_blend_epi16(mask, b, a);
    }

    // shift by constant

    template <int Count>
    static inline int16x8 slli(int16x8 a)
    {
        return _mm_slli_epi16(a, Count);
    }

    template <int Count>
    static inline int16x8 srli(int16x8 a)
    {
        return _mm_srli_epi16(a, Count);
    }

    template <int Count>
    static inline int16x8 srai(int16x8 a)
    {
        return _mm_srai_epi16(a, Count);
    }

    // shift by scalar

    static inline int16x8 sll(int16x8 a, int count)
    {
        return _mm_sll_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline int16x8 srl(int16x8 a, int count)
    {
        return _mm_srl_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline int16x8 sra(int16x8 a, int count)
    {
        return _mm_sra_epi16(a, _mm_cvtsi32_si128(count));
    }

    static inline int16x8 min(int16x8 a, int16x8 b)
    {
        return _mm_min_epi16(a, b);
    }

    static inline int16x8 max(int16x8 a, int16x8 b)
    {
        return _mm_max_epi16(a, b);
    }

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline int32x4 shuffle(int32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return _mm_shuffle_epi32(v, _MM_SHUFFLE(w, z, y, x));
    }

    template <>
    inline int32x4 shuffle<0, 1, 2, 3>(int32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline int32x4 set_component(int32x4 a, int32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm_insert_epi32(a, s, Index);
    }

    template <unsigned int Index>
    static inline int32 get_component(int32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return _mm_extract_epi32(a, Index);
    }

    static inline int32x4 int32x4_zero()
    {
        return _mm_setzero_si128();
    }

    static inline int32x4 int32x4_set1(int s)
    {
        return _mm_set1_epi32(s);
    }

    static inline int32x4 int32x4_set4(int x, int y, int z, int w)
    {
        return _mm_setr_epi32(x, y, z, w);
    }

    static inline int32x4 int32x4_uload(const int* source)
    {
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(source));
    }

    static inline void int32x4_ustore(int* dest, int32x4 a)
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest), a);
    }

    static inline int32x4 int32x4_load_low(const int32* source)
    {
        return _mm_loadl_epi64(reinterpret_cast<__m128i const *>(source));
    }

    static inline void int32x4_store_low(int32* dest, int32x4 a)
    {
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dest), a);
    }

    static inline int32x4 unpacklo(int32x4 a, int32x4 b)
    {
        return _mm_unpacklo_epi32(a, b);
    }

    static inline int32x4 unpackhi(int32x4 a, int32x4 b)
    {
        return _mm_unpackhi_epi32(a, b);
    }

    static inline int32x4 abs(int32x4 a)
    {
        return _mm_abs_epi32(a);
    }

    static inline int32x4 neg(int32x4 a)
    {
        return _mm_sub_epi32(_mm_setzero_si128(), a);
    }

    static inline int32x4 add(int32x4 a, int32x4 b)
    {
        return _mm_add_epi32(a, b);
    }

    static inline int32x4 sub(int32x4 a, int32x4 b)
    {
        return _mm_sub_epi32(a, b);
    }

    static inline int32x4 mullo(int32x4 a, int32x4 b)
    {
        return _mm_mullo_epi32(a, b);
    }

    // saturated

    static inline int32x4 adds(int32x4 a, int32x4 b)
    {
        const __m128i v = _mm_add_epi32(a, b);
        a = _mm_srai_epi32(a, 31);
        __m128i temp = _mm_xor_si128(b, v);
        temp = _mm_xor_si128(temp, _mm_cmpeq_epi32(temp, temp));
        temp = _mm_or_si128(temp, _mm_xor_si128(a, b));
        return _mm_select_si128(_mm_cmpgt_epi32(_mm_setzero_si128(), temp), v, a);
    }

    static inline int32x4 subs(int32x4 a, int32x4 b)
    {
        const __m128i v = _mm_sub_epi32(a, b);
        a = _mm_srai_epi32(a, 31);
        __m128i temp = _mm_and_si128(_mm_xor_si128(a, b), _mm_xor_si128(a, v));
        return _mm_select_si128(_mm_cmpgt_epi32(_mm_setzero_si128(), temp), a, v);
    }

    // bitwise

    static inline int32x4 bitwise_nand(int32x4 a, int32x4 b)
    {
        return _mm_andnot_si128(a, b);
    }

    static inline int32x4 bitwise_and(int32x4 a, int32x4 b)
    {
        return _mm_and_si128(a, b);
    }

    static inline int32x4 bitwise_or(int32x4 a, int32x4 b)
    {
        return _mm_or_si128(a, b);
    }

    static inline int32x4 bitwise_xor(int32x4 a, int32x4 b)
    {
        return _mm_xor_si128(a, b);
    }

    static inline int32x4 bitwise_not(int32x4 a)
    {
        return _mm_not_si128(a);
    }

    // compare

    static inline mask32x4 compare_eq(int32x4 a, int32x4 b)
    {
        return _mm_cmp_epi32_mask(a, b, _MM_CMPINT_EQ);
    }

    static inline mask32x4 compare_gt(int32x4 a, int32x4 b)
    {
        return _mm_cmp_epi32_mask(b, a, _MM_CMPINT_LT);
    }

    static inline mask32x4 compare_neq(int32x4 a, int32x4 b)
    {
        return _mm_cmp_epi32_mask(a, b, 4);
    }

    static inline mask32x4 compare_lt(int32x4 a, int32x4 b)
    {
        return _mm_cmp_epi32_mask(a, b, _MM_CMPINT_LT);
    }

    static inline mask32x4 compare_le(int32x4 a, int32x4 b)
    {
        return _mm_cmp_epi32_mask(a, b, _MM_CMPINT_LE);
    }

    static inline mask32x4 compare_ge(int32x4 a, int32x4 b)
    {
        return _mm_cmp_epi32_mask(b, a, _MM_CMPINT_LE);
    }

    static inline int32x4 select(mask32x4 mask, int32x4 a, int32x4 b)
    {
        return _mm_mask_blend_epi32(mask, b, a);
    }

    // shift by constant

    template <int Count>
    static inline int32x4 slli(int32x4 a)
    {
        return _mm_slli_epi32(a, Count);
    }

    template <int Count>
    static inline int32x4 srli(int32x4 a)
    {
        return _mm_srli_epi32(a, Count);
    }

    template <int Count>
    static inline int32x4 srai(int32x4 a)
    {
        return _mm_srai_epi32(a, Count);
    }

    // shift by scalar

    static inline int32x4 sll(int32x4 a, int count)
    {
        return _mm_sll_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline int32x4 srl(int32x4 a, int count)
    {
        return _mm_srl_epi32(a, _mm_cvtsi32_si128(count));
    }

    static inline int32x4 sra(int32x4 a, int count)
    {
        return _mm_sra_epi32(a, _mm_cvtsi32_si128(count));
    }

    // shift by vector

    static inline int32x4 sll(int32x4 a, uint32x4 count)
    {
        return _mm_sllv_epi32(a, count);
    }

    static inline int32x4 srl(int32x4 a, uint32x4 count)
    {
        return _mm_srlv_epi32(a, count);
    }

    static inline int32x4 sra(int32x4 a, uint32x4 count)
    {
        return _mm_srav_epi32(a, count);
    }

    static inline uint32 pack(int32x4 s)
    {
        __m128i s16 = _mm_packs_epi32(s, s);
        __m128i s8 = _mm_packus_epi16(s16, s16);
        return _mm_cvtsi128_si32(s8);
    }

    static inline int32x4 min(int32x4 a, int32x4 b)
    {
        return _mm_min_epi32(a, b);
    }

    static inline int32x4 max(int32x4 a, int32x4 b)
    {
        return _mm_max_epi32(a, b);
    }

    static inline int32x4 unpack(uint32 s)
    {
        const __m128i i = _mm_cvtsi32_si128(s);
        return _mm_cvtepu8_epi32(i);
    }

    // -----------------------------------------------------------------
    // int64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int64x2 set_component(int64x2 a, int64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return _mm_insert_epi64(a, s, Index);
    }

    template <unsigned int Index>
    static inline int64 get_component(int64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return _mm_extract_epi64(a, Index);
    }

    static inline int64x2 int64x2_zero()
    {
        return _mm_setzero_si128();
    }

    static inline int64x2 int64x2_set1(int64 s)
    {
        return _mm_set1_epi64x(s);
    }

    static inline int64x2 int64x2_set2(int64 x, int64 y)
    {
        return _mm_set_epi64x(y, x);
    }

    static inline int64x2 unpacklo(int64x2 a, int64x2 b)
    {
        return _mm_unpacklo_epi64(a, b);
    }

    static inline int64x2 unpackhi(int64x2 a, int64x2 b)
    {
        return _mm_unpackhi_epi64(a, b);
    }

    static inline int64x2 add(int64x2 a, int64x2 b)
    {
        return _mm_add_epi64(a, b);
    }

    static inline int64x2 sub(int64x2 a, int64x2 b)
    {
        return _mm_sub_epi64(a, b);
    }

    static inline int64x2 bitwise_nand(int64x2 a, int64x2 b)
    {
        return _mm_andnot_si128(a, b);
    }

    static inline int64x2 bitwise_and(int64x2 a, int64x2 b)
    {
        return _mm_and_si128(a, b);
    }

    static inline int64x2 bitwise_or(int64x2 a, int64x2 b)
    {
        return _mm_or_si128(a, b);
    }

    static inline int64x2 bitwise_xor(int64x2 a, int64x2 b)
    {
        return _mm_xor_si128(a, b);
    }

    static inline int64x2 bitwise_not(int64x2 a)
    {
        return _mm_not_si128(a);
    }

    static inline int64x2 select(mask64x2 mask, int64x2 a, int64x2 b)
    {
        return _mm_mask_blend_epi64(mask, b, a);
    }

    // shift by constant

    template <int Count>
    static inline int64x2 slli(int64x2 a)
    {
        return _mm_slli_epi64(a, Count);
    }

    template <int Count>
    static inline int64x2 srli(int64x2 a)
    {
        return _mm_srli_epi64(a, Count);
    }

    // shift by scalar

    static inline int64x2 sll(int64x2 a, int count)
    {
        return _mm_sll_epi64(a, _mm_cvtsi32_si128(count));
    }

    static inline int64x2 srl(int64x2 a, int count)
    {
        return _mm_srl_epi64(a, _mm_cvtsi32_si128(count));
    }

    // -----------------------------------------------------------------
    // mask8x16
    // -----------------------------------------------------------------

#if !defined(MANGO_COMPILER_MICROSOFT)

	static inline mask8x16 operator & (mask8x16 a, mask8x16 b)
    {
        return _mm512_kand(a, b);
    }

    static inline mask8x16 operator | (mask8x16 a, mask8x16 b)
    {
        return _mm512_kor(a, b);
    }

    static inline mask8x16 operator ^ (mask8x16 a, mask8x16 b)
    {
        return _mm512_kxor(a, b);
    }

#endif

    static inline uint32 get_mask(mask8x16 a)
    {
        return uint32(a);
    }

    // -----------------------------------------------------------------
    // mask16x8
    // -----------------------------------------------------------------

#if !defined(MANGO_COMPILER_MICROSOFT)

    static inline mask16x8 operator & (mask16x8 a, mask16x8 b)
    {
        return _mm512_kand(a, b);
    }

    static inline mask16x8 operator | (mask16x8 a, mask16x8 b)
    {
        return _mm512_kor(a, b);
    }

    static inline mask16x8 operator ^ (mask16x8 a, mask16x8 b)
    {
        return _mm512_kxor(a, b);
    }

#endif

    static inline uint32 get_mask(mask16x8 a)
    {
        return uint32(a);
    }

    // -----------------------------------------------------------------
    // mask32x4
    // -----------------------------------------------------------------

#if !defined(MANGO_COMPILER_MICROSOFT)
	
	static inline mask32x4 operator & (mask32x4 a, mask32x4 b)
    {
        return _mm512_kand(a, b);
    }

    static inline mask32x4 operator | (mask32x4 a, mask32x4 b)
    {
        return _mm512_kor(a, b);
    }

    static inline mask32x4 operator ^ (mask32x4 a, mask32x4 b)
    {
        return _mm512_kxor(a, b);
    }

#endif

    static inline uint32 get_mask(mask32x4 a)
    {
        return uint32(a);
    }

    // -----------------------------------------------------------------
    // mask64x2
    // -----------------------------------------------------------------

#if !defined(MANGO_COMPILER_MICROSOFT)

	static inline mask64x2 operator & (mask64x2 a, mask64x2 b)
    {
        return _mm512_kand(a, b);
    }

    static inline mask64x2 operator | (mask64x2 a, mask64x2 b)
    {
        return _mm512_kor(a, b);
    }

    static inline mask64x2 operator ^ (mask64x2 a, mask64x2 b)
    {
        return _mm512_kxor(a, b);
    }

#endif

    static inline uint32 get_mask(mask64x2 a)
    {
        return uint32(a);
    }

#undef simd128_shuffle_epi32
#undef simd128_shuffle_epi64

} // namespace simd
} // namespace mango
