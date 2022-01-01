/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    Copyright 2017 The Chromium Authors. All rights reserved.
    Use of this source code is governed by a BSD-style license that can be
    found in the Chromium source repository LICENSE file.
*/
#include <mango/core/adler32.hpp>

#include "../../external/zlib/zlib.h"

namespace
{
    using namespace mango;

#if defined(MANGO_ENABLE_SSSE3)

    static inline
    u32 adler32_ssse3(u32 adler, const u8* buf, size_t len)
    {
        constexpr size_t BASE = 65521; // largest prime smaller than 65536
        constexpr size_t NMAX = 5552; // largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1

        // Split Adler-32 into component sums.
        u32 s1 = adler & 0xffff;
        u32 s2 = adler >> 16;

        // Process the data in blocks. 
        const unsigned BLOCK_SIZE = 1 << 5;
        size_t blocks = len / BLOCK_SIZE;
        len -= blocks * BLOCK_SIZE;
        while (blocks)
        {
            unsigned n = NMAX / BLOCK_SIZE;  // The NMAX constraint.
            if (n > blocks)
                n = (unsigned) blocks;
            blocks -= n;
            const __m128i tap1 = _mm_setr_epi8(32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17);
            const __m128i tap2 = _mm_setr_epi8(16,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1);
            const __m128i zero = _mm_setr_epi8( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            const __m128i ones = _mm_set_epi16( 1, 1, 1, 1, 1, 1, 1, 1);

            // Process n blocks of data. 
            // At most NMAX data bytes can be processed before s2 must be reduced modulo BASE.
            __m128i v_ps = _mm_set_epi32(0, 0, 0, s1 * n);
            __m128i v_s2 = _mm_set_epi32(0, 0, 0, s2);
            __m128i v_s1 = _mm_set_epi32(0, 0, 0, 0);
            do
            {
                // Load 32 input bytes. 
                const __m128i bytes1 = _mm_loadu_si128((__m128i*)(buf));
                const __m128i bytes2 = _mm_loadu_si128((__m128i*)(buf + 16));

                // Add previous block byte sum to v_ps.
                v_ps = _mm_add_epi32(v_ps, v_s1);

                // Horizontally add the bytes for s1, multiply-adds the
                // bytes by [ 32, 31, 30, ... ] for s2.
                v_s1 = _mm_add_epi32(v_s1, _mm_sad_epu8(bytes1, zero));
                const __m128i mad1 = _mm_maddubs_epi16(bytes1, tap1);
                v_s2 = _mm_add_epi32(v_s2, _mm_madd_epi16(mad1, ones));
                v_s1 = _mm_add_epi32(v_s1, _mm_sad_epu8(bytes2, zero));
                const __m128i mad2 = _mm_maddubs_epi16(bytes2, tap2);
                v_s2 = _mm_add_epi32(v_s2, _mm_madd_epi16(mad2, ones));
                buf += BLOCK_SIZE;
            } while (--n);
            v_s2 = _mm_add_epi32(v_s2, _mm_slli_epi32(v_ps, 5));

            // Sum epi32 ints v_s1(s2) and accumulate in s1(s2).
            v_s1 = _mm_add_epi32(v_s1, _mm_shuffle_epi32(v_s1, _MM_SHUFFLE(2,3,0,1)));
            v_s1 = _mm_add_epi32(v_s1, _mm_shuffle_epi32(v_s1, _MM_SHUFFLE(1,0,3,2)));
            s1 += _mm_cvtsi128_si32(v_s1);
            v_s2 = _mm_add_epi32(v_s2, _mm_shuffle_epi32(v_s2, _MM_SHUFFLE(2,3,0,1)));
            v_s2 = _mm_add_epi32(v_s2, _mm_shuffle_epi32(v_s2, _MM_SHUFFLE(1,0,3,2)));
            s2 = _mm_cvtsi128_si32(v_s2);

            // Reduce.
            s1 %= BASE;
            s2 %= BASE;
        }

        // Handle leftover data.
        if (len)
        {
            if (len >= 16)
            {
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                len -= 16;
            }

            while (len--)
            {
                s2 += (s1 += *buf++);
            }

            if (s1 >= BASE)
                s1 -= BASE;
            s2 %= BASE;
        }

        // Return the recombined sums.
        return s1 | (s2 << 16);
    }

#elif defined(MANGO_ENABLE_NEON)

    static inline
    u32 adler32_neon(u32 adler, const u8* buf, size_t len)
    {
        constexpr size_t BASE = 65521; // largest prime smaller than 65536
        constexpr size_t NMAX = 5552; // largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1

        // Split Adler-32 into component sums.
        u32 s1 = adler & 0xffff;
        u32 s2 = adler >> 16;

        // Serially compute s1 & s2, until the data is 16-byte aligned.
        if ((uintptr_t)buf & 15)
        {
            while ((uintptr_t)buf & 15)
            {
                s2 += (s1 += *buf++);
                --len;
            }

            if (s1 >= BASE)
                s1 -= BASE;
            s2 %= BASE;
        }

        // Process the data in blocks.
        const unsigned BLOCK_SIZE = 1 << 5;
        size_t blocks = len / BLOCK_SIZE;
        len -= blocks * BLOCK_SIZE;
        while (blocks)
        {
            unsigned n = NMAX / BLOCK_SIZE;  // The NMAX constraint.
            if (n > blocks)
                n = (unsigned) blocks;
            blocks -= n;

            // Process n blocks of data. 
            // At most NMAX data bytes can be processed before s2 must be reduced modulo BASE.
            uint32x4_t v_s2 = (uint32x4_t) { 0, 0, 0, s1 * n };
            uint32x4_t v_s1 = (uint32x4_t) { 0, 0, 0, 0 };
            uint16x8_t v_column_sum_1 = vdupq_n_u16(0);
            uint16x8_t v_column_sum_2 = vdupq_n_u16(0);
            uint16x8_t v_column_sum_3 = vdupq_n_u16(0);
            uint16x8_t v_column_sum_4 = vdupq_n_u16(0);
            do
            {
                // Load 32 input bytes.
                const uint8x16_t bytes1 = vld1q_u8((u8*)(buf));
                const uint8x16_t bytes2 = vld1q_u8((u8*)(buf + 16));

                // Add previous block byte sum to v_s2.
                v_s2 = vaddq_u32(v_s2, v_s1);

                // Horizontally add the bytes for s1.
                v_s1 = vpadalq_u16(v_s1, vpadalq_u8(vpaddlq_u8(bytes1), bytes2));

                // Vertically add the bytes for s2.
                v_column_sum_1 = vaddw_u8(v_column_sum_1, vget_low_u8 (bytes1));
                v_column_sum_2 = vaddw_u8(v_column_sum_2, vget_high_u8(bytes1));
                v_column_sum_3 = vaddw_u8(v_column_sum_3, vget_low_u8 (bytes2));
                v_column_sum_4 = vaddw_u8(v_column_sum_4, vget_high_u8(bytes2));
                buf += BLOCK_SIZE;
            } while (--n);
            v_s2 = vshlq_n_u32(v_s2, 5);

            // Multiply-add bytes by [ 32, 31, 30, ... ] for s2.
            v_s2 = vmlal_u16(v_s2, vget_low_u16 (v_column_sum_1), (uint16x4_t) { 32, 31, 30, 29 });
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_1), (uint16x4_t) { 28, 27, 26, 25 });
            v_s2 = vmlal_u16(v_s2, vget_low_u16 (v_column_sum_2), (uint16x4_t) { 24, 23, 22, 21 });
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_2), (uint16x4_t) { 20, 19, 18, 17 });
            v_s2 = vmlal_u16(v_s2, vget_low_u16 (v_column_sum_3), (uint16x4_t) { 16, 15, 14, 13 });
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_3), (uint16x4_t) { 12, 11, 10,  9 });
            v_s2 = vmlal_u16(v_s2, vget_low_u16 (v_column_sum_4), (uint16x4_t) {  8,  7,  6,  5 });
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_4), (uint16x4_t) {  4,  3,  2,  1 });

            // Sum epi32 ints v_s1(s2) and accumulate in s1(s2).
            uint32x2_t sum1 = vpadd_u32(vget_low_u32(v_s1), vget_high_u32(v_s1));
            uint32x2_t sum2 = vpadd_u32(vget_low_u32(v_s2), vget_high_u32(v_s2));
            uint32x2_t s1s2 = vpadd_u32(sum1, sum2);
            s1 += vget_lane_u32(s1s2, 0);
            s2 += vget_lane_u32(s1s2, 1);

            // Reduce.
            s1 %= BASE;
            s2 %= BASE;
        }

        // Handle leftover data.
        if (len)
        {
            if (len >= 16)
            {
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                len -= 16;
            }

            while (len--)
            {
                s2 += (s1 += *buf++);
            }

            if (s1 >= BASE)
                s1 -= BASE;
            s2 %= BASE;
        }

        // Return the recombined sums.
        return s1 | (s2 << 16);
    }

#endif

} // namespace

namespace mango
{

    u32 adler32(u32 adler, ConstMemory memory)
    {
#if defined(MANGO_ENABLE_SSSE3)
        return adler32_ssse3(adler, memory.address, memory.size);
#elif defined(MANGO_ENABLE_NEON)
        return adler32_neon(adler, memory.address, memory.size);
#else
        return ::adler32_z(adler, memory.address, memory.size);
#endif
    }

    u32 adler32_combine(u32 adler0, u32 adler1, size_t length1)
    {
        return ::adler32_combine(adler0, adler1, z_off_t(length1));
    }

} // namespace mango
