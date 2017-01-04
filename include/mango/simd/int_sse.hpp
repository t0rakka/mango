/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_SIMD_INT
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

    // -----------------------------------------------------------------
    // simd4i
    // -----------------------------------------------------------------

    // conversion

    static inline simd4i simd4i_cast(__simd4f s)
    {
        return _mm_castps_si128(s);
    }

    static inline simd4i simd4i_convert(__simd4f s)
    {
        return _mm_cvtps_epi32(s);
    }

    static inline simd4i simd4i_truncate(__simd4f s)
    {
        return _mm_cvttps_epi32(s);
    }

#if defined(MANGO_ENABLE_SSE4_1)

    // set

    static inline simd4i simd4i_set_x(__simd4i a, int x)
    {
        return _mm_insert_epi32(a, x, 0);
    }

    static inline simd4i simd4i_set_y(__simd4i a, int y)
    {
        return _mm_insert_epi32(a, y, 1);
    }

    static inline simd4i simd4i_set_z(__simd4i a, int z)
    {
        return _mm_insert_epi32(a, z, 2);
    }

    static inline simd4i simd4i_set_w(__simd4i a, int w)
    {
        return _mm_insert_epi32(a, w, 3);
    }

    // get

    static inline int simd4i_get_x(__simd4i a)
    {
        return _mm_extract_epi32(a, 0);
    }

    static inline int simd4i_get_y(__simd4i a)
    {
        return _mm_extract_epi32(a, 1);
    }

    static inline int simd4i_get_z(__simd4i a)
    {
        return _mm_extract_epi32(a, 2);
    }

    static inline int simd4i_get_w(__simd4i a)
    {
        return _mm_extract_epi32(a, 3);
    }

#else

    // set

#define _mm_shuffle_epi(a, b, mask) \
    _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), mask));

    static inline simd4i simd4i_set_x(__simd4i a, int x)
    {
        const __m128i b = _mm_unpacklo_epi32(_mm_set1_epi32(x), a);
        return _mm_shuffle_epi(b, a, _MM_SHUFFLE(3, 2, 3, 0));
    }

    static inline simd4i simd4i_set_y(__simd4i a, int y)
    {
        const __m128i b = _mm_unpacklo_epi32(_mm_set1_epi32(y), a);
        return _mm_shuffle_epi(b, a, _MM_SHUFFLE(3, 2, 0, 1));
    }

    static inline simd4i simd4i_set_z(__simd4i a, int z)
    {
        const __m128i b = _mm_unpackhi_epi32(_mm_set1_epi32(z), a);
        return _mm_shuffle_epi(a, b, _MM_SHUFFLE(3, 0, 1, 0));
    }

    static inline simd4i simd4i_set_w(__simd4i a, int w)
    {
        const __m128i b = _mm_unpackhi_epi32(_mm_set1_epi32(w), a);
        return _mm_shuffle_epi(a, b, _MM_SHUFFLE(0, 1, 1, 0));
    }

#undef _mm_shuffle_epi

    // get

    static inline int simd4i_get_x(__simd4i a)
    {
        return _mm_cvtsi128_si32(a);
    }

    static inline int simd4i_get_y(__simd4i a)
    {
        return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, 0x55));
    }

    static inline int simd4i_get_z(__simd4i a)
    {
        return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, 0xaa));
    }

    static inline int simd4i_get_w(__simd4i a)
    {
        return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, 0xff));
    }

#endif

    static inline simd4i simd4i_load(const int* source)
    {
        return _mm_load_si128(reinterpret_cast<const __m128i*>(source));
    }

    static inline simd4i simd4i_uload(const int* source)
    {
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(source));
    }

    static inline void simd4i_store(int* dest, __simd4i a)
    {
        _mm_store_si128(reinterpret_cast<__m128i*>(dest), a);
    }

    static inline void simd4i_ustore(int* dest, __simd4i a)
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest), a);
    }

    static inline simd4i simd4i_zero()
    {
        return _mm_setzero_si128();
    }

    static inline simd4i simd4i_set1(int s)
    {
        return _mm_set1_epi32(s);
    }

    static inline simd4i simd4i_set4(int x, int y, int z, int w)
    {
        return _mm_setr_epi32(x, y, z, w);
    }

    static inline simd4i simd4i_neg(__simd4i a)
    {
        return _mm_xor_si128(a, _mm_set1_epi32(0x80000000));
    }

    static inline simd4i simd4i_add(__simd4i a, __simd4i b)
    {
        return _mm_add_epi32(a, b);
    }

    static inline simd4i simd4i_sub(__simd4i a, __simd4i b)
    {
        return _mm_sub_epi32(a, b);
    }

    // logical

    static inline simd4i simd4i_and(__simd4i a, __simd4i b)
    {
        return _mm_and_si128(a, b);
    }

    static inline simd4i simd4i_nand(__simd4i a, __simd4i b)
    {
        return _mm_andnot_si128(a, b);
    }

    static inline simd4i simd4i_or(__simd4i a, __simd4i b)
    {
        return _mm_or_si128(a, b);
    }

    static inline simd4i simd4i_xor(__simd4i a, __simd4i b)
    {
        return _mm_xor_si128(a, b);
    }

    // shift

    static inline simd4i simd4i_sll(__simd4i a, int b)
    {
        return _mm_slli_epi32(a, b);
    }

    static inline simd4i simd4i_srl(__simd4i a, int b)
    {
        return _mm_srli_epi32(a, b);
    }

    static inline simd4i simd4i_sra(__simd4i a, int b)
    {
        return _mm_srai_epi32(a, b);
    }

    // compare

    static inline simd4i simd4i_compare_eq(__simd4i a, __simd4i b)
    {
        return _mm_cmpeq_epi32(a, b);
    }

    static inline simd4i simd4i_compare_gt(__simd4i a, __simd4i b)
    {
        return _mm_cmpgt_epi32(a, b);
    }

    static inline simd4i simd4i_select(__simd4i mask, __simd4i a, __simd4i b)
    {
        return simd4i_or(simd4i_and(mask, a), simd4i_nand(mask, b));
    }

    static inline uint32 simd4i_get_mask(__simd4i a)
    {
        return _mm_movemask_ps(_mm_castsi128_ps(a));
    }

    static inline uint32 simd4i_pack(__simd4i s)
    {
        __m128i s16 = _mm_packs_epi32(s, s);
        __m128i s8 = _mm_packus_epi16(s16, s16);
        return _mm_cvtsi128_si32(s8);
    }

#if defined(MANGO_ENABLE_SSE4_1)

    static inline simd4i simd4i_unpack(uint32 s)
    {
        const __m128i i = _mm_cvtsi32_si128(s);
        return _mm_cvtepu8_epi32(i);
    }

#else

    static inline simd4i simd4i_unpack(uint32 s)
    {
        const __m128i zero = _mm_setzero_si128();
        const __m128i i = _mm_cvtsi32_si128(s);
        return _mm_unpacklo_epi16(_mm_unpacklo_epi8(i, zero), zero);
    }

#endif
