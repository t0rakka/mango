/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_SIMD_INT
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

    // -----------------------------------------------------------------
    // conversion
    // -----------------------------------------------------------------

    static inline uint32x4 uint32x4_reinterpret(int32x4 s)
    {
        return int32x4::type(s);
    }

    static inline int32x4 int32x4_reinterpret(uint32x4 s)
    {
        return uint32x4::type(s);
    }

    static inline int32x4 int32x4_reinterpret(float32x4 s)
    {
        return _mm_castps_si128(s);
    }

    static inline int32x4 int32x4_convert(float32x4 s)
    {
        return _mm_cvtps_epi32(s);
    }

    static inline int32x4 int32x4_truncate(float32x4 s)
    {
        return _mm_cvttps_epi32(s);
    }

    // -----------------------------------------------------------------
    // uint32x4
    // -----------------------------------------------------------------

    static inline uint32x4 uint32x4_set1(uint32 s)
    {
        return _mm_set1_epi32(s);
    }

    static inline uint32x4 uint32x4_set4(uint32 x, uint32 y, uint32 z, uint32 w)
    {
        return _mm_setr_epi32(x, y, z, w);
    }

#if defined(MANGO_ENABLE_SSE4_1)

    // set

    static inline uint32x4 uint32x4_set_x(uint32x4 a, int x)
    {
        return _mm_insert_epi32(a, x, 0);
    }

    static inline uint32x4 uint32x4_set_y(uint32x4 a, int y)
    {
        return _mm_insert_epi32(a, y, 1);
    }

    static inline uint32x4 uint32x4_set_z(uint32x4 a, int z)
    {
        return _mm_insert_epi32(a, z, 2);
    }

    static inline uint32x4 uint32x4_set_w(uint32x4 a, int w)
    {
        return _mm_insert_epi32(a, w, 3);
    }

    // get

    static inline int uint32x4_get_x(uint32x4 a)
    {
        return _mm_extract_epi32(a, 0);
    }

    static inline int uint32x4_get_y(uint32x4 a)
    {
        return _mm_extract_epi32(a, 1);
    }

    static inline int uint32x4_get_z(uint32x4 a)
    {
        return _mm_extract_epi32(a, 2);
    }

    static inline int uint32x4_get_w(uint32x4 a)
    {
        return _mm_extract_epi32(a, 3);
    }

#else

    // set

#define _mm_shuffle_epi(a, b, mask) \
    _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), mask));

    static inline uint32x4 uint32x4_set_x(uint32x4 a, int x)
    {
        const __m128i b = _mm_unpacklo_epi32(_mm_set1_epi32(x), a);
        return _mm_shuffle_epi(b, a, _MM_SHUFFLE(3, 2, 3, 0));
    }

    static inline uint32x4 uint32x4_set_y(uint32x4 a, int y)
    {
        const __m128i b = _mm_unpacklo_epi32(_mm_set1_epi32(y), a);
        return _mm_shuffle_epi(b, a, _MM_SHUFFLE(3, 2, 0, 1));
    }

    static inline uint32x4 uint32x4_set_z(uint32x4 a, int z)
    {
        const __m128i b = _mm_unpackhi_epi32(_mm_set1_epi32(z), a);
        return _mm_shuffle_epi(a, b, _MM_SHUFFLE(3, 0, 1, 0));
    }

    static inline uint32x4 uint32x4_set_w(uint32x4 a, int w)
    {
        const __m128i b = _mm_unpackhi_epi32(_mm_set1_epi32(w), a);
        return _mm_shuffle_epi(a, b, _MM_SHUFFLE(0, 1, 1, 0));
    }

#undef _mm_shuffle_epi

    // get

    static inline uint32 uint32x4_get_x(uint32x4 a)
    {
        return _mm_cvtsi128_si32(a);
    }

    static inline uint32 uint32x4_get_y(uint32x4 a)
    {
        return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, 0x55));
    }

    static inline uint32 uint32x4_get_z(uint32x4 a)
    {
        return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, 0xaa));
    }

    static inline uint32 uint32x4_get_w(uint32x4 a)
    {
        return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, 0xff));
    }

#endif

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

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

#if defined(MANGO_ENABLE_SSE4_1)

    // set

    static inline int32x4 int32x4_set_x(int32x4 a, int x)
    {
        return _mm_insert_epi32(a, x, 0);
    }

    static inline int32x4 int32x4_set_y(int32x4 a, int y)
    {
        return _mm_insert_epi32(a, y, 1);
    }

    static inline int32x4 int32x4_set_z(int32x4 a, int z)
    {
        return _mm_insert_epi32(a, z, 2);
    }

    static inline int32x4 int32x4_set_w(int32x4 a, int w)
    {
        return _mm_insert_epi32(a, w, 3);
    }

    // get

    static inline int int32x4_get_x(int32x4 a)
    {
        return _mm_extract_epi32(a, 0);
    }

    static inline int int32x4_get_y(int32x4 a)
    {
        return _mm_extract_epi32(a, 1);
    }

    static inline int int32x4_get_z(int32x4 a)
    {
        return _mm_extract_epi32(a, 2);
    }

    static inline int int32x4_get_w(int32x4 a)
    {
        return _mm_extract_epi32(a, 3);
    }

#else

    // set

#define _mm_shuffle_epi(a, b, mask) \
    _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), mask));

    static inline int32x4 int32x4_set_x(int32x4 a, int x)
    {
        const __m128i b = _mm_unpacklo_epi32(_mm_set1_epi32(x), a);
        return _mm_shuffle_epi(b, a, _MM_SHUFFLE(3, 2, 3, 0));
    }

    static inline int32x4 int32x4_set_y(int32x4 a, int y)
    {
        const __m128i b = _mm_unpacklo_epi32(_mm_set1_epi32(y), a);
        return _mm_shuffle_epi(b, a, _MM_SHUFFLE(3, 2, 0, 1));
    }

    static inline int32x4 int32x4_set_z(int32x4 a, int z)
    {
        const __m128i b = _mm_unpackhi_epi32(_mm_set1_epi32(z), a);
        return _mm_shuffle_epi(a, b, _MM_SHUFFLE(3, 0, 1, 0));
    }

    static inline int32x4 int32x4_set_w(int32x4 a, int w)
    {
        const __m128i b = _mm_unpackhi_epi32(_mm_set1_epi32(w), a);
        return _mm_shuffle_epi(a, b, _MM_SHUFFLE(0, 1, 1, 0));
    }

#undef _mm_shuffle_epi

    // get

    static inline int int32x4_get_x(int32x4 a)
    {
        return _mm_cvtsi128_si32(a);
    }

    static inline int int32x4_get_y(int32x4 a)
    {
        return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, 0x55));
    }

    static inline int int32x4_get_z(int32x4 a)
    {
        return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, 0xaa));
    }

    static inline int int32x4_get_w(int32x4 a)
    {
        return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, 0xff));
    }

#endif

    static inline int32x4 int32x4_load(const int* source)
    {
        return _mm_load_si128(reinterpret_cast<const __m128i*>(source));
    }

    static inline int32x4 int32x4_uload(const int* source)
    {
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(source));
    }

    static inline void int32x4_store(int* dest, int32x4 a)
    {
        _mm_store_si128(reinterpret_cast<__m128i*>(dest), a);
    }

    static inline void int32x4_ustore(int* dest, int32x4 a)
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest), a);
    }

    static inline int32x4 int32x4_neg(int32x4 a)
    {
        return _mm_xor_si128(a, _mm_set1_epi32(0x80000000));
    }

    static inline int32x4 int32x4_add(int32x4 a, int32x4 b)
    {
        return _mm_add_epi32(a, b);
    }

    static inline int32x4 int32x4_sub(int32x4 a, int32x4 b)
    {
        return _mm_sub_epi32(a, b);
    }

    // logical

    static inline int32x4 int32x4_and(int32x4 a, int32x4 b)
    {
        return _mm_and_si128(a, b);
    }

    static inline int32x4 int32x4_nand(int32x4 a, int32x4 b)
    {
        return _mm_andnot_si128(a, b);
    }

    static inline int32x4 int32x4_or(int32x4 a, int32x4 b)
    {
        return _mm_or_si128(a, b);
    }

    static inline int32x4 int32x4_xor(int32x4 a, int32x4 b)
    {
        return _mm_xor_si128(a, b);
    }

    // shift

    static inline int32x4 int32x4_sll(int32x4 a, int b)
    {
        return _mm_slli_epi32(a, b);
    }

    static inline int32x4 int32x4_srl(int32x4 a, int b)
    {
        return _mm_srli_epi32(a, b);
    }

    static inline int32x4 int32x4_sra(int32x4 a, int b)
    {
        return _mm_srai_epi32(a, b);
    }

    // compare

    static inline int32x4 int32x4_compare_eq(int32x4 a, int32x4 b)
    {
        return _mm_cmpeq_epi32(a, b);
    }

    static inline int32x4 int32x4_compare_gt(int32x4 a, int32x4 b)
    {
        return _mm_cmpgt_epi32(a, b);
    }

    static inline int32x4 int32x4_select(int32x4 mask, int32x4 a, int32x4 b)
    {
        return int32x4_or(int32x4_and(mask, a), int32x4_nand(mask, b));
    }

    static inline uint32 int32x4_get_mask(int32x4 a)
    {
        return _mm_movemask_ps(_mm_castsi128_ps(a));
    }

    static inline uint32 int32x4_pack(int32x4 s)
    {
        __m128i s16 = _mm_packs_epi32(s, s);
        __m128i s8 = _mm_packus_epi16(s16, s16);
        return _mm_cvtsi128_si32(s8);
    }

#if defined(MANGO_ENABLE_SSE4_1)

    static inline int32x4 int32x4_unpack(uint32 s)
    {
        const __m128i i = _mm_cvtsi32_si128(s);
        return _mm_cvtepu8_epi32(i);
    }

#else

    static inline int32x4 int32x4_unpack(uint32 s)
    {
        const __m128i zero = _mm_setzero_si128();
        const __m128i i = _mm_cvtsi32_si128(s);
        return _mm_unpacklo_epi16(_mm_unpacklo_epi8(i, zero), zero);
    }

#endif
