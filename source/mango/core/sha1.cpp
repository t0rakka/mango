/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/hash.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/bits.hpp>
#include <mango/core/endian.hpp>
#include <mango/core/cpuinfo.hpp>

namespace
{
    using namespace mango;

    #define K1 0x5A827999
    #define K2 0x6ED9EBA1
    #define K3 0x8F1BBCDC
    #define K4 0xCA62C1D6

#if defined(__ARM_FEATURE_CRYPTO)

    // ----------------------------------------------------------------------------------------
    // ARM Crypto SHA1
    // ----------------------------------------------------------------------------------------

    /* sha1-arm.c - ARMv8 SHA extensions using C intrinsics       */
    /*   Written and placed in public domain by Jeffrey Walton    */
    /*   Based on code from ARM, and by Johannes Schneiders, Skip */
    /*   Hovsmith and Barry O'Rourke for the mbedTLS project.     */

    void arm_sha1_transform(u32 state[5], const u8* data, int blocks)
    {
        // Load state
        uint32x4_t ABCD = vld1q_u32(&state[0]);
        uint32_t E0 = state[4];

        while (blocks-- > 0)
        {
            // Save state
            uint32x4_t ABCD_SAVED = ABCD;
            uint32_t E0_SAVED = E0;

            uint32x4_t TMP0, TMP1;
            uint32x4_t MSG0, MSG1, MSG2, MSG3;
            uint32_t E1;

            // Load message
            MSG0 = vld1q_u32((const uint32_t*)(data + 0));
            MSG1 = vld1q_u32((const uint32_t*)(data + 16));
            MSG2 = vld1q_u32((const uint32_t*)(data + 32));
            MSG3 = vld1q_u32((const uint32_t*)(data + 48));
            data += 64;

#ifdef MANGO_LITTLE_ENDIAN
            // Reverse for little endian
            MSG0 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG0)));
            MSG1 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG1)));
            MSG2 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG2)));
            MSG3 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG3)));
#endif

            TMP0 = vaddq_u32(MSG0, vdupq_n_u32(K1));
            TMP1 = vaddq_u32(MSG1, vdupq_n_u32(K1));

            // Rounds 0-3
            E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1cq_u32(ABCD, E0, TMP0);
            TMP0 = vaddq_u32(MSG2, vdupq_n_u32(K1));
            MSG0 = vsha1su0q_u32(MSG0, MSG1, MSG2);

            // Rounds 4-7
            E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1cq_u32(ABCD, E1, TMP1);
            TMP1 = vaddq_u32(MSG3, vdupq_n_u32(K1));
            MSG0 = vsha1su1q_u32(MSG0, MSG3);
            MSG1 = vsha1su0q_u32(MSG1, MSG2, MSG3);

            // Rounds 8-11
            E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1cq_u32(ABCD, E0, TMP0);
            TMP0 = vaddq_u32(MSG0, vdupq_n_u32(K1));
            MSG1 = vsha1su1q_u32(MSG1, MSG0);
            MSG2 = vsha1su0q_u32(MSG2, MSG3, MSG0);

            // Rounds 12-15
            E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1cq_u32(ABCD, E1, TMP1);
            TMP1 = vaddq_u32(MSG1, vdupq_n_u32(K2));
            MSG2 = vsha1su1q_u32(MSG2, MSG1);
            MSG3 = vsha1su0q_u32(MSG3, MSG0, MSG1);

            // Rounds 16-19
            E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1cq_u32(ABCD, E0, TMP0);
            TMP0 = vaddq_u32(MSG2, vdupq_n_u32(K2));
            MSG3 = vsha1su1q_u32(MSG3, MSG2);
            MSG0 = vsha1su0q_u32(MSG0, MSG1, MSG2);

            // Rounds 20-23
            E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1pq_u32(ABCD, E1, TMP1);
            TMP1 = vaddq_u32(MSG3, vdupq_n_u32(K2));
            MSG0 = vsha1su1q_u32(MSG0, MSG3);
            MSG1 = vsha1su0q_u32(MSG1, MSG2, MSG3);

            // Rounds 24-27
            E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1pq_u32(ABCD, E0, TMP0);
            TMP0 = vaddq_u32(MSG0, vdupq_n_u32(K2));
            MSG1 = vsha1su1q_u32(MSG1, MSG0);
            MSG2 = vsha1su0q_u32(MSG2, MSG3, MSG0);

            // Rounds 28-31
            E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1pq_u32(ABCD, E1, TMP1);
            TMP1 = vaddq_u32(MSG1, vdupq_n_u32(K2));
            MSG2 = vsha1su1q_u32(MSG2, MSG1);
            MSG3 = vsha1su0q_u32(MSG3, MSG0, MSG1);

            // Rounds 32-35
            E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1pq_u32(ABCD, E0, TMP0);
            TMP0 = vaddq_u32(MSG2, vdupq_n_u32(K3));
            MSG3 = vsha1su1q_u32(MSG3, MSG2);
            MSG0 = vsha1su0q_u32(MSG0, MSG1, MSG2);

            // Rounds 36-39
            E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1pq_u32(ABCD, E1, TMP1);
            TMP1 = vaddq_u32(MSG3, vdupq_n_u32(K3));
            MSG0 = vsha1su1q_u32(MSG0, MSG3);
            MSG1 = vsha1su0q_u32(MSG1, MSG2, MSG3);

            // Rounds 40-43
            E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1mq_u32(ABCD, E0, TMP0);
            TMP0 = vaddq_u32(MSG0, vdupq_n_u32(K3));
            MSG1 = vsha1su1q_u32(MSG1, MSG0);
            MSG2 = vsha1su0q_u32(MSG2, MSG3, MSG0);

            // Rounds 44-47
            E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1mq_u32(ABCD, E1, TMP1);
            TMP1 = vaddq_u32(MSG1, vdupq_n_u32(K3));
            MSG2 = vsha1su1q_u32(MSG2, MSG1);
            MSG3 = vsha1su0q_u32(MSG3, MSG0, MSG1);

            // Rounds 48-51
            E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1mq_u32(ABCD, E0, TMP0);
            TMP0 = vaddq_u32(MSG2, vdupq_n_u32(K3));
            MSG3 = vsha1su1q_u32(MSG3, MSG2);
            MSG0 = vsha1su0q_u32(MSG0, MSG1, MSG2);

            // Rounds 52-55
            E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1mq_u32(ABCD, E1, TMP1);
            TMP1 = vaddq_u32(MSG3, vdupq_n_u32(K4));
            MSG0 = vsha1su1q_u32(MSG0, MSG3);
            MSG1 = vsha1su0q_u32(MSG1, MSG2, MSG3);

            // Rounds 56-59
            E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1mq_u32(ABCD, E0, TMP0);
            TMP0 = vaddq_u32(MSG0, vdupq_n_u32(K4));
            MSG1 = vsha1su1q_u32(MSG1, MSG0);
            MSG2 = vsha1su0q_u32(MSG2, MSG3, MSG0);

            // Rounds 60-63
            E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1pq_u32(ABCD, E1, TMP1);
            TMP1 = vaddq_u32(MSG1, vdupq_n_u32(K4));
            MSG2 = vsha1su1q_u32(MSG2, MSG1);
            MSG3 = vsha1su0q_u32(MSG3, MSG0, MSG1);

            // Rounds 64-67
            E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1pq_u32(ABCD, E0, TMP0);
            TMP0 = vaddq_u32(MSG2, vdupq_n_u32(K4));
            MSG3 = vsha1su1q_u32(MSG3, MSG2);
            MSG0 = vsha1su0q_u32(MSG0, MSG1, MSG2);

            // Rounds 68-71
            E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1pq_u32(ABCD, E1, TMP1);
            TMP1 = vaddq_u32(MSG3, vdupq_n_u32(K4));
            MSG0 = vsha1su1q_u32(MSG0, MSG3);

            // Rounds 72-75
            E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1pq_u32(ABCD, E0, TMP0);

            // Rounds 76-79
            E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
            ABCD = vsha1pq_u32(ABCD, E1, TMP1);

            // Combine state
            E0 += E0_SAVED;
            ABCD = vaddq_u32(ABCD_SAVED, ABCD);
        }

        // Save state
        vst1q_u32(&state[0], ABCD);
        state[4] = E0;
    }

#elif defined(__SHA__)

    /*******************************************************************************
    * Copyright (c) 2013, Intel Corporation
    *
    * All rights reserved.
    *
    * Redistribution and use in source and binary forms, with or without
    * modification, are permitted provided that the following conditions are
    * met:
    *
    * * Redistributions of source code must retain the above copyright
    *   notice, this list of conditions and the following disclaimer.
    *
    * * Redistributions in binary form must reproduce the above copyright
    *   notice, this list of conditions and the following disclaimer in the
    *   documentation and/or other materials provided with the
    *   distribution.
    *
    * * Neither the name of the Intel Corporation nor the names of its
    *   contributors may be used to endorse or promote products derived from
    *   this software without specific prior written permission.
    *
    *
    * THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION ""AS IS"" AND ANY
    * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL CORPORATION OR
    * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    ********************************************************************************
    *
    * Intel SHA Extensions optimized implementation of a SHA-1 update function
    *
    * The function takes a pointer to the current hash values, a pointer to the
    * input data, and a number of 64 byte blocks to process.  Once all blocks have
    * been processed, the digest pointer is  updated with the resulting hash value.
    * The function only processes complete blocks, there is no functionality to
    * store partial blocks.  All message padding and hash value initialization must
    * be done outside the update function.
    *
    * The indented lines in the loop are instructions related to rounds processing.
    * The non-indented lines are instructions related to the message schedule.
    *
    * Author: Sean Gulley <sean.m.gulley@intel.com>
    * Date:   July 2013
    *
    *******************************************************************************/

    void intel_sha1_transform(u32* digest, const u8* data, int blocks)
    {
        const __m128i e_mask    = _mm_set_epi64x(0xffffffff00000000ull, 0x0000000000000000ull);
        const __m128i shuf_mask = _mm_set_epi64x(0x0001020304050607ull, 0x08090a0b0c0d0e0full);

        // Load initial hash values
        __m128i abcd = _mm_loadu_si128(reinterpret_cast<__m128i*>(digest));
        __m128i e0   = _mm_setzero_si128();
        e0   = _mm_insert_epi32(e0, int(digest[4]), 3);
        abcd = _mm_shuffle_epi32(abcd, 0x1b);
        e0   = _mm_and_si128(e0, e_mask);

        while (blocks-- > 0)
        {
            // Save hash values for addition after rounds
            __m128i abcd_save = abcd;
            __m128i e_save    = e0;
            __m128i e1;

            __m128i msg0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 0));
            __m128i msg1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 16));
            __m128i msg2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 32));
            __m128i msg3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 48));

            msg0 = _mm_shuffle_epi8(msg0, shuf_mask);
            msg1 = _mm_shuffle_epi8(msg1, shuf_mask);
            msg2 = _mm_shuffle_epi8(msg2, shuf_mask);
            msg3 = _mm_shuffle_epi8(msg3, shuf_mask);

            data += 64;

            // Rounds 0-3
            e0   = _mm_add_epi32(e0, msg0);
            e1   = _mm_sha1nexte_epu32(abcd, msg1);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 0);

            // Rounds 4-7
            e0   = _mm_sha1nexte_epu32(abcd, msg2);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 0);
            msg0 = _mm_sha1msg1_epu32(msg0, msg1);

            // Rounds 8-11
            e1   = _mm_sha1nexte_epu32(abcd, msg3);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 0);
            msg1 = _mm_sha1msg1_epu32(msg1, msg2);
            msg0 = _mm_xor_si128(msg0, msg2);

            // Rounds 12-15
            msg0 = _mm_sha1msg2_epu32(msg0, msg3);
            e0   = _mm_sha1nexte_epu32(abcd, msg0);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 0);
            msg2 = _mm_sha1msg1_epu32(msg2, msg3);
            msg1 = _mm_xor_si128(msg1, msg3);

            // Rounds 16-19
            msg1 = _mm_sha1msg2_epu32(msg1, msg0);
            e1   = _mm_sha1nexte_epu32(abcd, msg1);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 0);
            msg3 = _mm_sha1msg1_epu32(msg3, msg0);
            msg2 = _mm_xor_si128(msg2, msg0);

            // Rounds 20-23
            msg2 = _mm_sha1msg2_epu32(msg2, msg1);
            e0   = _mm_sha1nexte_epu32(abcd, msg2);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 1);
            msg0 = _mm_sha1msg1_epu32(msg0, msg1);
            msg3 = _mm_xor_si128(msg3, msg1);

            // Rounds 24-27
            msg3 = _mm_sha1msg2_epu32(msg3, msg2);
            e1   = _mm_sha1nexte_epu32(abcd, msg3);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 1);
            msg1 = _mm_sha1msg1_epu32(msg1, msg2);
            msg0 = _mm_xor_si128(msg0, msg2);

            // Rounds 28-31
            msg0 = _mm_sha1msg2_epu32(msg0, msg3);
            e0   = _mm_sha1nexte_epu32(abcd, msg0);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 1);
            msg2 = _mm_sha1msg1_epu32(msg2, msg3);
            msg1 = _mm_xor_si128(msg1, msg3);

            // Rounds 32-35
            msg1 = _mm_sha1msg2_epu32(msg1, msg0);
            e1   = _mm_sha1nexte_epu32(abcd, msg1);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 1);
            msg3 = _mm_sha1msg1_epu32(msg3, msg0);
            msg2 = _mm_xor_si128(msg2, msg0);

            // Rounds 36-39
            msg2 = _mm_sha1msg2_epu32(msg2, msg1);
            e0   = _mm_sha1nexte_epu32(abcd, msg2);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 1);
            msg0 = _mm_sha1msg1_epu32(msg0, msg1);
            msg3 = _mm_xor_si128(msg3, msg1);

            // Rounds 40-43
            msg3 = _mm_sha1msg2_epu32(msg3, msg2);
            e1   = _mm_sha1nexte_epu32(abcd, msg3);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 2);
            msg1 = _mm_sha1msg1_epu32(msg1, msg2);
            msg0 = _mm_xor_si128(msg0, msg2);

            // Rounds 44-47
            msg0 = _mm_sha1msg2_epu32(msg0, msg3);
            e0   = _mm_sha1nexte_epu32(abcd, msg0);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 2);
            msg2 = _mm_sha1msg1_epu32(msg2, msg3);
            msg1 = _mm_xor_si128(msg1, msg3);

            // Rounds 48-51
            msg1 = _mm_sha1msg2_epu32(msg1, msg0);
            e1   = _mm_sha1nexte_epu32(abcd, msg1);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 2);
            msg3 = _mm_sha1msg1_epu32(msg3, msg0);
            msg2 = _mm_xor_si128(msg2, msg0);

            // Rounds 52-55
            msg2 = _mm_sha1msg2_epu32(msg2, msg1);
            e0   = _mm_sha1nexte_epu32(abcd, msg2);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 2);
            msg0 = _mm_sha1msg1_epu32(msg0, msg1);
            msg3 = _mm_xor_si128(msg3, msg1);

            // Rounds 56-59
            msg3 = _mm_sha1msg2_epu32(msg3, msg2);
            e1   = _mm_sha1nexte_epu32(abcd, msg3);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 2);
            msg1 = _mm_sha1msg1_epu32(msg1, msg2);
            msg0 = _mm_xor_si128(msg0, msg2);

            // Rounds 60-63
            msg0 = _mm_sha1msg2_epu32(msg0, msg3);
            e0   = _mm_sha1nexte_epu32(abcd, msg0);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 3);
            msg2 = _mm_sha1msg1_epu32(msg2, msg3);
            msg1 = _mm_xor_si128(msg1, msg3);

            // Rounds 64-67
            msg1 = _mm_sha1msg2_epu32(msg1, msg0);
            e1   = _mm_sha1nexte_epu32(abcd, msg1);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 3);
            msg3 = _mm_sha1msg1_epu32(msg3, msg0);
            msg2 = _mm_xor_si128(msg2, msg0);

            // Rounds 68-71
            msg2 = _mm_sha1msg2_epu32(msg2, msg1);
            e0   = _mm_sha1nexte_epu32(abcd, msg2);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 3);
            msg3 = _mm_xor_si128(msg3, msg1);

            // Rounds 72-75
            msg3 = _mm_sha1msg2_epu32(msg3, msg2);
            e1   = _mm_sha1nexte_epu32(abcd, msg3);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 3);

            // Rounds 76-79
            e0   = _mm_sha1nexte_epu32(abcd, e_save);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 3);

            // Add current hash values with previously saved
            abcd = _mm_add_epi32(abcd, abcd_save);
        }

        abcd = _mm_shuffle_epi32(abcd, 0x1b);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(digest), abcd);
        digest[4] = u32(_mm_extract_epi32(e0, 3));
    }

#endif

    // ----------------------------------------------------------------------------------------
    // Generic C++ SHA1
    // ----------------------------------------------------------------------------------------

    // Copyright (c) 2012 Nayuki Minase

#define SCHEDULE(i) \
	temp = schedule[(i - 3) & 0xf] ^ schedule[(i - 8) & 0xf] ^ schedule[(i - 14) & 0xf] ^ schedule[(i - 16) & 0xf]; \
	schedule[i & 0xf] = u32_rol(temp, 1);

#define ROUNDTAIL(a,b,e,f,i,k) \
	e += u32_rol(a, 5) + f + k + schedule[i & 0xf]; \
    b = u32_rol(b, 30);

#define ROUND0a(a,b,c,d,e,i) \
	schedule[i] = bigEndian::uload32(data + i * 4); \
	ROUNDTAIL(a, b, e, ((b & c) | (~b & d)), i, K1)

#define ROUND0b(a,b,c,d,e,i) \
	SCHEDULE(i) \
	ROUNDTAIL(a, b, e, ((b & c) | (~b & d)), i, K1)

#define ROUND1(a,b,c,d,e,i) \
	SCHEDULE(i) \
	ROUNDTAIL(a, b, e, (b ^ c ^ d), i, K2)

#define ROUND2(a,b,c,d,e,i) \
	SCHEDULE(i) \
	ROUNDTAIL(a, b, e, ((b & c) ^ (b & d) ^ (c & d)), i, K3)

#define ROUND3(a,b,c,d,e,i) \
	SCHEDULE(i) \
	ROUNDTAIL(a, b, e, (b ^ c ^ d), i, K4)

    void generic_sha1_transform(u32* digest, const u8* data, int blocks)
    {
        while (blocks-- > 0)
        {
            u32 a = digest[0];
            u32 b = digest[1];
            u32 c = digest[2];
            u32 d = digest[3];
            u32 e = digest[4];

            u32 schedule[16];
            u32 temp;

            ROUND0a(a, b, c, d, e,  0)
            ROUND0a(e, a, b, c, d,  1)
            ROUND0a(d, e, a, b, c,  2)
            ROUND0a(c, d, e, a, b,  3)
            ROUND0a(b, c, d, e, a,  4)
            ROUND0a(a, b, c, d, e,  5)
            ROUND0a(e, a, b, c, d,  6)
            ROUND0a(d, e, a, b, c,  7)
            ROUND0a(c, d, e, a, b,  8)
            ROUND0a(b, c, d, e, a,  9)
            ROUND0a(a, b, c, d, e, 10)
            ROUND0a(e, a, b, c, d, 11)
            ROUND0a(d, e, a, b, c, 12)
            ROUND0a(c, d, e, a, b, 13)
            ROUND0a(b, c, d, e, a, 14)
            ROUND0a(a, b, c, d, e, 15)
            ROUND0b(e, a, b, c, d, 16)
            ROUND0b(d, e, a, b, c, 17)
            ROUND0b(c, d, e, a, b, 18)
            ROUND0b(b, c, d, e, a, 19)
            ROUND1(a, b, c, d, e, 20)
            ROUND1(e, a, b, c, d, 21)
            ROUND1(d, e, a, b, c, 22)
            ROUND1(c, d, e, a, b, 23)
            ROUND1(b, c, d, e, a, 24)
            ROUND1(a, b, c, d, e, 25)
            ROUND1(e, a, b, c, d, 26)
            ROUND1(d, e, a, b, c, 27)
            ROUND1(c, d, e, a, b, 28)
            ROUND1(b, c, d, e, a, 29)
            ROUND1(a, b, c, d, e, 30)
            ROUND1(e, a, b, c, d, 31)
            ROUND1(d, e, a, b, c, 32)
            ROUND1(c, d, e, a, b, 33)
            ROUND1(b, c, d, e, a, 34)
            ROUND1(a, b, c, d, e, 35)
            ROUND1(e, a, b, c, d, 36)
            ROUND1(d, e, a, b, c, 37)
            ROUND1(c, d, e, a, b, 38)
            ROUND1(b, c, d, e, a, 39)
            ROUND2(a, b, c, d, e, 40)
            ROUND2(e, a, b, c, d, 41)
            ROUND2(d, e, a, b, c, 42)
            ROUND2(c, d, e, a, b, 43)
            ROUND2(b, c, d, e, a, 44)
            ROUND2(a, b, c, d, e, 45)
            ROUND2(e, a, b, c, d, 46)
            ROUND2(d, e, a, b, c, 47)
            ROUND2(c, d, e, a, b, 48)
            ROUND2(b, c, d, e, a, 49)
            ROUND2(a, b, c, d, e, 50)
            ROUND2(e, a, b, c, d, 51)
            ROUND2(d, e, a, b, c, 52)
            ROUND2(c, d, e, a, b, 53)
            ROUND2(b, c, d, e, a, 54)
            ROUND2(a, b, c, d, e, 55)
            ROUND2(e, a, b, c, d, 56)
            ROUND2(d, e, a, b, c, 57)
            ROUND2(c, d, e, a, b, 58)
            ROUND2(b, c, d, e, a, 59)
            ROUND3(a, b, c, d, e, 60)
            ROUND3(e, a, b, c, d, 61)
            ROUND3(d, e, a, b, c, 62)
            ROUND3(c, d, e, a, b, 63)
            ROUND3(b, c, d, e, a, 64)
            ROUND3(a, b, c, d, e, 65)
            ROUND3(e, a, b, c, d, 66)
            ROUND3(d, e, a, b, c, 67)
            ROUND3(c, d, e, a, b, 68)
            ROUND3(b, c, d, e, a, 69)
            ROUND3(a, b, c, d, e, 70)
            ROUND3(e, a, b, c, d, 71)
            ROUND3(d, e, a, b, c, 72)
            ROUND3(c, d, e, a, b, 73)
            ROUND3(b, c, d, e, a, 74)
            ROUND3(a, b, c, d, e, 75)
            ROUND3(e, a, b, c, d, 76)
            ROUND3(d, e, a, b, c, 77)
            ROUND3(c, d, e, a, b, 78)
            ROUND3(b, c, d, e, a, 79)

            digest[0] += a;
            digest[1] += b;
            digest[2] += c;
            digest[3] += d;
            digest[4] += e;

            data += 64;
        }
    }

} // namespace

namespace mango
{

    SHA1 sha1(ConstMemory memory)
    {
        SHA1 hash;

        hash.data[0] = 0x67452301;
        hash.data[1] = 0xefcdab89;
        hash.data[2] = 0x98badcfe;
        hash.data[3] = 0x10325476;
        hash.data[4] = 0xc3d2e1f0;

        // select implementation
        auto transform = generic_sha1_transform;

#if defined(__ARM_FEATURE_CRYPTO)
        if ((cpu::getFlags() & ARM_SHA1) != 0)
        {
            transform = arm_sha1_transform;
        }
#elif defined(__SHA__)
        if ((cpu::getFlags() & INTEL_SHA) != 0)
        {
            transform = intel_sha1_transform;
        }
#endif

        u32 size = u32(memory.size);
        const u8* data = memory.address;

        const int block_count = int(size / 64);
        transform(hash.data, data, block_count);
        data += block_count * 64;
        size -= block_count * 64;

        u8 block[64];

        std::memcpy(block, data, size);
        block[size++] = 0x80;

        if (size <= 56)
        {
            std::memset(block + size, 0, 56 - size);
        }
        else
        {
            std::memset(block + size, 0, 64 - size);
            transform(hash.data, block, 1);
            std::memset(block, 0, 56);
        }

        bigEndian::ustore64(block + 56, u64(memory.size * 8));
        transform(hash.data, block, 1);

#ifdef MANGO_LITTLE_ENDIAN
        hash.data[0] = byteswap(hash.data[0]);
        hash.data[1] = byteswap(hash.data[1]);
        hash.data[2] = byteswap(hash.data[2]);
        hash.data[3] = byteswap(hash.data[3]);
        hash.data[4] = byteswap(hash.data[4]);
#endif

        return hash;
    }

} // namespace mango
