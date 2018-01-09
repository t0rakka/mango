/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/hash.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/bits.hpp>
#include <mango/core/endian.hpp>

namespace {
    using namespace mango;

#if defined(__ARM_FEATURE_CRYPTO)

    // ----------------------------------------------------------------------------------------
    // ARM Crypto SHA1
    // ----------------------------------------------------------------------------------------

#define ROUND0(K, A, B, C, D) \
    a = vgetq_lane_u32(abcd, 0);       \
    e0 = vsha1h_u32(a);                \
    abcd = vsha1cq_u32(abcd, e1, wk1); \
    wk1 = vaddq_u32(w3, K);            \
    A = vsha1su1q_u32(A, D);           \
    B = vsha1su0q_u32(B, C, D);

#define ROUND1(K, A, B, C, D) \
    a = vgetq_lane_u32(abcd, 0);       \
    e1 = vsha1h_u32(a);                \
    abcd = vsha1cq_u32(abcd, e0, wk0); \
    wk0 = vaddq_u32(w0, K);            \
    A = vsha1su1q_u32(A, D);           \
    B = vsha1su0q_u32(B, C, D);

    void sha1_update(uint32 state[5], const uint8* block, int count)
    {
        // set K0..K3 constants
        uint32x4_t k0 = vdupq_n_u32(0x5A827999);
        uint32x4_t k1 = vdupq_n_u32(0x6ED9EBA1);
        uint32x4_t k2 = vdupq_n_u32(0x8F1BBCDC);
        uint32x4_t k3 = vdupq_n_u32(0xCA62C1D6);

        for (int i = 0; i < count; ++i)
        {
            // load state
            uint32x4_t abcd = vld1q_u32(state);
            uint32x4_t abcd0 = abcd;
            uint32_t e = state[4];

            // load message
            uint32x4_t w0 = vld1q_u32(reinterpret_cast<const uint32_t *>(block +  0));
            uint32x4_t w1 = vld1q_u32(reinterpret_cast<const uint32_t *>(block + 16));
            uint32x4_t w2 = vld1q_u32(reinterpret_cast<const uint32_t *>(block + 32));
            uint32x4_t w3 = vld1q_u32(reinterpret_cast<const uint32_t *>(block + 48));
            block += 64;

#ifdef MANGO_LITTLE_ENDIAN
            w0 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(w0)));
            w1 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(w1)));
            w2 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(w2)));
            w3 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(w3)));
#endif

            // initialize wk0, wk1
            uint32x4_t wk0 = vaddq_u32(w0, k0);
            uint32x4_t wk1 = vaddq_u32(w1, k0);

            uint32_t a, e0, e1;

            a = vgetq_lane_u32(abcd, 0);
            e1 = vsha1h_u32(a);
            abcd = vsha1cq_u32(abcd, e, wk0);
            wk0 = vaddq_u32(w2, k0);
            w0 = vsha1su0q_u32(w0, w1, w2);

            ROUND0(k0, w0, w1, w2, w3);
            ROUND1(k0, w1, w2, w3, w0);
            ROUND0(k1, w2, w3, w0, w1);
            ROUND1(k1, w3, w0, w1, w2);
            ROUND0(k1, w0, w1, w2, w3);
            ROUND1(k1, w1, w2, w3, w0);
            ROUND0(k1, w2, w3, w0, w1);
            ROUND1(k2, w3, w0, w1, w2);
            ROUND0(k2, w0, w1, w2, w3);
            ROUND1(k2, w1, w2, w3, w0);
            ROUND0(k2, w2, w3, w0, w1);
            ROUND1(k2, w3, w0, w1, w2);
            ROUND0(k3, w0, w1, w2, w3);
            ROUND1(k3, w1, w2, w3, w0);
            ROUND0(k3, w2, w3, w0, w1);
            ROUND1(k3, w3, w0, w1, w2);

            a = vgetq_lane_u32(abcd, 0);
            e0 = vsha1h_u32(a);
            abcd = vsha1pq_u32(abcd, e1, wk1);
            wk1 = vaddq_u32(w3, k3);
            w0 = vsha1su1q_u32(w0, w3);

            a = vgetq_lane_u32(abcd, 0);
            e1 = vsha1h_u32(a);
            abcd = vsha1pq_u32(abcd, e0, wk0);

            a = vgetq_lane_u32(abcd, 0);
            e0 = vsha1h_u32(a);
            abcd = vsha1pq_u32(abcd, e1, wk1);

            // store state
            vst1q_u32(state, vaddq_u32(abcd0, abcd));
            state[4] = e + e0;
        }
    }

#undef ROUND0
#undef ROUND1

#elif defined(MANGO_ENABLE_SHA)

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

    void sha1_update(uint32 *digest, const uint8 *data, int num_blks)
    {
        __m128i abcd, e0, e1;
        __m128i abcd_save, e_save;
        __m128i msg0, msg1, msg2, msg3;

        __m128i e_mask    = _mm_set_epi64x(0xFFFFFFFF00000000ull, 0x0000000000000000ull);
        __m128i shuf_mask = _mm_set_epi64x(0x0001020304050607ull, 0x08090a0b0c0d0e0full);

        // Load initial hash values
        e0        = _mm_setzero_si128();
        abcd      = _mm_loadu_si128((__m128i*) digest);
        e0        = _mm_insert_epi32(e0, *(digest+4), 3);
        abcd      = _mm_shuffle_epi32(abcd, 0x1B);
        e0        = _mm_and_si128(e0, e_mask);

        while (num_blks > 0) {
            // Save hash values for addition after rounds
            abcd_save = abcd;
            e_save    = e0;

            // Rounds 0-3
            msg0 = _mm_loadu_si128((__m128i*) data);
            msg0 = _mm_shuffle_epi8(msg0, shuf_mask);
            e0   = _mm_add_epi32(e0, msg0);
            e1   = abcd;
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 0);

            // Rounds 4-7
            msg1 = _mm_loadu_si128((__m128i*) (data+16));
            msg1 = _mm_shuffle_epi8(msg1, shuf_mask);
            e1   = _mm_sha1nexte_epu32(e1, msg1);
            e0   = abcd;
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 0);
            msg0 = _mm_sha1msg1_epu32(msg0, msg1);

            // Rounds 8-11
            msg2 = _mm_loadu_si128((__m128i*) (data+32));
            msg2 = _mm_shuffle_epi8(msg2, shuf_mask);
            e0   = _mm_sha1nexte_epu32(e0, msg2);
            e1   = abcd;
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 0);
            msg1 = _mm_sha1msg1_epu32(msg1, msg2);
            msg0 = _mm_xor_si128(msg0, msg2);

            // Rounds 12-15
            msg3 = _mm_loadu_si128((__m128i*) (data+48));
            msg3 = _mm_shuffle_epi8(msg3, shuf_mask);
            e1   = _mm_sha1nexte_epu32(e1, msg3);
            e0   = abcd;
            msg0 = _mm_sha1msg2_epu32(msg0, msg3);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 0);
            msg2 = _mm_sha1msg1_epu32(msg2, msg3);
            msg1 = _mm_xor_si128(msg1, msg3);

            // Rounds 16-19
            e0   = _mm_sha1nexte_epu32(e0, msg0);
            e1   = abcd;
            msg1 = _mm_sha1msg2_epu32(msg1, msg0);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 0);
            msg3 = _mm_sha1msg1_epu32(msg3, msg0);
            msg2 = _mm_xor_si128(msg2, msg0);

            // Rounds 20-23
            e1   = _mm_sha1nexte_epu32(e1, msg1);
            e0   = abcd;
            msg2 = _mm_sha1msg2_epu32(msg2, msg1);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 1);
            msg0 = _mm_sha1msg1_epu32(msg0, msg1);
            msg3 = _mm_xor_si128(msg3, msg1);
            
            // Rounds 24-27
            e0   = _mm_sha1nexte_epu32(e0, msg2);
            e1   = abcd;
            msg3 = _mm_sha1msg2_epu32(msg3, msg2);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 1);
            msg1 = _mm_sha1msg1_epu32(msg1, msg2);
            msg0 = _mm_xor_si128(msg0, msg2);

            // Rounds 28-31
            e1   = _mm_sha1nexte_epu32(e1, msg3);
            e0   = abcd;
            msg0 = _mm_sha1msg2_epu32(msg0, msg3);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 1);
            msg2 = _mm_sha1msg1_epu32(msg2, msg3);
            msg1 = _mm_xor_si128(msg1, msg3);

            // Rounds 32-35
            e0   = _mm_sha1nexte_epu32(e0, msg0);
            e1   = abcd;
            msg1 = _mm_sha1msg2_epu32(msg1, msg0);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 1);
            msg3 = _mm_sha1msg1_epu32(msg3, msg0);
            msg2 = _mm_xor_si128(msg2, msg0);

            // Rounds 36-39
            e1   = _mm_sha1nexte_epu32(e1, msg1);
            e0   = abcd;
            msg2 = _mm_sha1msg2_epu32(msg2, msg1);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 1);
            msg0 = _mm_sha1msg1_epu32(msg0, msg1);
            msg3 = _mm_xor_si128(msg3, msg1);
            
            // Rounds 40-43
            e0   = _mm_sha1nexte_epu32(e0, msg2);
            e1   = abcd;
            msg3 = _mm_sha1msg2_epu32(msg3, msg2);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 2);
            msg1 = _mm_sha1msg1_epu32(msg1, msg2);
            msg0 = _mm_xor_si128(msg0, msg2);

            // Rounds 44-47
            e1   = _mm_sha1nexte_epu32(e1, msg3);
            e0   = abcd;
            msg0 = _mm_sha1msg2_epu32(msg0, msg3);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 2);
            msg2 = _mm_sha1msg1_epu32(msg2, msg3);
            msg1 = _mm_xor_si128(msg1, msg3);

            // Rounds 48-51
            e0   = _mm_sha1nexte_epu32(e0, msg0);
            e1   = abcd;
            msg1 = _mm_sha1msg2_epu32(msg1, msg0);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 2);
            msg3 = _mm_sha1msg1_epu32(msg3, msg0);
            msg2 = _mm_xor_si128(msg2, msg0);

            // Rounds 52-55
            e1   = _mm_sha1nexte_epu32(e1, msg1);
            e0   = abcd;
            msg2 = _mm_sha1msg2_epu32(msg2, msg1);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 2);
            msg0 = _mm_sha1msg1_epu32(msg0, msg1);
            msg3 = _mm_xor_si128(msg3, msg1);
            
            // Rounds 56-59
            e0   = _mm_sha1nexte_epu32(e0, msg2);
            e1   = abcd;
            msg3 = _mm_sha1msg2_epu32(msg3, msg2);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 2);
            msg1 = _mm_sha1msg1_epu32(msg1, msg2);
            msg0 = _mm_xor_si128(msg0, msg2);

            // Rounds 60-63
            e1   = _mm_sha1nexte_epu32(e1, msg3);
            e0   = abcd;
            msg0 = _mm_sha1msg2_epu32(msg0, msg3);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 3);
            msg2 = _mm_sha1msg1_epu32(msg2, msg3);
            msg1 = _mm_xor_si128(msg1, msg3);

            // Rounds 64-67
            e0   = _mm_sha1nexte_epu32(e0, msg0);
            e1   = abcd;
            msg1 = _mm_sha1msg2_epu32(msg1, msg0);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 3);
            msg3 = _mm_sha1msg1_epu32(msg3, msg0);
            msg2 = _mm_xor_si128(msg2, msg0);

            // Rounds 68-71
            e1   = _mm_sha1nexte_epu32(e1, msg1);
            e0   = abcd;
            msg2 = _mm_sha1msg2_epu32(msg2, msg1);
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 3);
            msg3 = _mm_xor_si128(msg3, msg1);
            
            // Rounds 72-75
            e0   = _mm_sha1nexte_epu32(e0, msg2);
            e1   = abcd;
            msg3 = _mm_sha1msg2_epu32(msg3, msg2);
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 3);

            // Rounds 76-79
            e1   = _mm_sha1nexte_epu32(e1, msg3);
            e0   = abcd;
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 3);

            // Add current hash values with previously saved
            e0   = _mm_sha1nexte_epu32(e0, e_save);
            abcd = _mm_add_epi32(abcd, abcd_save);

            data += 64;
            num_blks--;
        }

        abcd = _mm_shuffle_epi32(abcd, 0x1B);
        _mm_store_si128((__m128i*) digest, abcd);
        *(digest+4) = _mm_extract_epi32(e0, 3);
    }

#else

    /*
     * SHA-1 hash in C
     *
     * Copyright (c) 2014 Project Nayuki
     * https://www.nayuki.io/page/fast-sha1-hash-implementation-in-x86-assembly
     *
     * (MIT License)
     * Permission is hereby granted, free of charge, to any person obtaining a copy of
     * this software and associated documentation files (the "Software"), to deal in
     * the Software without restriction, including without limitation the rights to
     * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
     * the Software, and to permit persons to whom the Software is furnished to do so,
     * subject to the following conditions:
     * - The above copyright notice and this permission notice shall be included in
     *   all copies or substantial portions of the Software.
     * - The Software is provided "as is", without warranty of any kind, express or
     *   implied, including but not limited to the warranties of merchantability,
     *   fitness for a particular purpose and noninfringement. In no event shall the
     *   authors or copyright holders be liable for any claim, damages or other
     *   liability, whether in an action of contract, tort or otherwise, arising from,
     *   out of or in connection with the Software or the use or other dealings in the
     *   Software.
     */

#define SCHEDULE(i)  { \
    uint32 temp = schedule[(i -  3) & 0xF] \
                ^ schedule[(i -  8) & 0xF] \
                ^ schedule[(i - 14) & 0xF] \
                ^ schedule[(i - 16) & 0xF]; \
    schedule[i & 0xF] = temp << 1 | temp >> 31; }

#define LOADSCHEDULE(i)  \
    schedule[i] = uload32be(block + i * 4)

#define ROUNDTAIL(a, b, e, f, i, k) \
    e += (a << 5 | a >> 27) + f + UINT32_C(k) + schedule[i & 0xF]; \
    b = b << 30 | b >> 2

#define ROUND0a(a, b, c, d, e, i)  LOADSCHEDULE(i);  ROUNDTAIL(a, b, e, ((b & c) | (~b & d))         , i, 0x5A827999);
#define ROUND0b(a, b, c, d, e, i)  SCHEDULE(i);      ROUNDTAIL(a, b, e, ((b & c) | (~b & d))         , i, 0x5A827999);
#define ROUND1(a, b, c, d, e, i)   SCHEDULE(i);      ROUNDTAIL(a, b, e, (b ^ c ^ d)                  , i, 0x6ED9EBA1);
#define ROUND2(a, b, c, d, e, i)   SCHEDULE(i);      ROUNDTAIL(a, b, e, ((b & c) ^ (b & d) ^ (c & d)), i, 0x8F1BBCDC);
#define ROUND3(a, b, c, d, e, i)   SCHEDULE(i);      ROUNDTAIL(a, b, e, (b ^ c ^ d)                  , i, 0xCA62C1D6);

    void sha1_update(uint32 state[5], const uint8* block, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            uint32 a = state[0];
            uint32 b = state[1];
            uint32 c = state[2];
            uint32 d = state[3];
            uint32 e = state[4];

            uint32 schedule[16];

            ROUND0a(a, b, c, d, e,  0);
            ROUND0a(e, a, b, c, d,  1);
            ROUND0a(d, e, a, b, c,  2);
            ROUND0a(c, d, e, a, b,  3);
            ROUND0a(b, c, d, e, a,  4);
            ROUND0a(a, b, c, d, e,  5);
            ROUND0a(e, a, b, c, d,  6);
            ROUND0a(d, e, a, b, c,  7);
            ROUND0a(c, d, e, a, b,  8);
            ROUND0a(b, c, d, e, a,  9);
            ROUND0a(a, b, c, d, e, 10);
            ROUND0a(e, a, b, c, d, 11);
            ROUND0a(d, e, a, b, c, 12);
            ROUND0a(c, d, e, a, b, 13);
            ROUND0a(b, c, d, e, a, 14);
            ROUND0a(a, b, c, d, e, 15);
            ROUND0b(e, a, b, c, d, 16);
            ROUND0b(d, e, a, b, c, 17);
            ROUND0b(c, d, e, a, b, 18);
            ROUND0b(b, c, d, e, a, 19);
            ROUND1(a, b, c, d, e, 20);
            ROUND1(e, a, b, c, d, 21);
            ROUND1(d, e, a, b, c, 22);
            ROUND1(c, d, e, a, b, 23);
            ROUND1(b, c, d, e, a, 24);
            ROUND1(a, b, c, d, e, 25);
            ROUND1(e, a, b, c, d, 26);
            ROUND1(d, e, a, b, c, 27);
            ROUND1(c, d, e, a, b, 28);
            ROUND1(b, c, d, e, a, 29);
            ROUND1(a, b, c, d, e, 30);
            ROUND1(e, a, b, c, d, 31);
            ROUND1(d, e, a, b, c, 32);
            ROUND1(c, d, e, a, b, 33);
            ROUND1(b, c, d, e, a, 34);
            ROUND1(a, b, c, d, e, 35);
            ROUND1(e, a, b, c, d, 36);
            ROUND1(d, e, a, b, c, 37);
            ROUND1(c, d, e, a, b, 38);
            ROUND1(b, c, d, e, a, 39);
            ROUND2(a, b, c, d, e, 40);
            ROUND2(e, a, b, c, d, 41);
            ROUND2(d, e, a, b, c, 42);
            ROUND2(c, d, e, a, b, 43);
            ROUND2(b, c, d, e, a, 44);
            ROUND2(a, b, c, d, e, 45);
            ROUND2(e, a, b, c, d, 46);
            ROUND2(d, e, a, b, c, 47);
            ROUND2(c, d, e, a, b, 48);
            ROUND2(b, c, d, e, a, 49);
            ROUND2(a, b, c, d, e, 50);
            ROUND2(e, a, b, c, d, 51);
            ROUND2(d, e, a, b, c, 52);
            ROUND2(c, d, e, a, b, 53);
            ROUND2(b, c, d, e, a, 54);
            ROUND2(a, b, c, d, e, 55);
            ROUND2(e, a, b, c, d, 56);
            ROUND2(d, e, a, b, c, 57);
            ROUND2(c, d, e, a, b, 58);
            ROUND2(b, c, d, e, a, 59);
            ROUND3(a, b, c, d, e, 60);
            ROUND3(e, a, b, c, d, 61);
            ROUND3(d, e, a, b, c, 62);
            ROUND3(c, d, e, a, b, 63);
            ROUND3(b, c, d, e, a, 64);
            ROUND3(a, b, c, d, e, 65);
            ROUND3(e, a, b, c, d, 66);
            ROUND3(d, e, a, b, c, 67);
            ROUND3(c, d, e, a, b, 68);
            ROUND3(b, c, d, e, a, 69);
            ROUND3(a, b, c, d, e, 70);
            ROUND3(e, a, b, c, d, 71);
            ROUND3(d, e, a, b, c, 72);
            ROUND3(c, d, e, a, b, 73);
            ROUND3(b, c, d, e, a, 74);
            ROUND3(a, b, c, d, e, 75);
            ROUND3(e, a, b, c, d, 76);
            ROUND3(d, e, a, b, c, 77);
            ROUND3(c, d, e, a, b, 78);
            ROUND3(b, c, d, e, a, 79);

            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
            state[4] += e;

            block += 64;
        }
    }

#undef SCHEDULE
#undef LOADSCHEDULE
#undef ROUNDTAIL
#undef ROUND0a
#undef ROUND0b
#undef ROUND1
#undef ROUND2
#undef ROUND3

#endif

} // namespace

namespace mango {

    void sha1(uint32 hash[5], Memory memory)
    {
        hash[0] = 0x67452301;
        hash[1] = 0xEFCDAB89;
        hash[2] = 0x98BADCFE;
        hash[3] = 0x10325476;
        hash[4] = 0xC3D2E1F0;

        const uint32 len = uint32(memory.size);
        const uint8* message = memory.address;

        int block_count = len / 64;
        sha1_update(hash, message, block_count);
        message += block_count * 64;
        uint32 i = block_count * 64;

        uint8 block[64];
        uint32 rem = len - i;
        memcpy(block, message + i, rem);

        block[rem++] = 0x80;
        if (64 - rem >= 8)
        {
            memset(block + rem, 0, 56 - rem);
        }
        else
        {
            memset(block + rem, 0, 64 - rem);
            sha1_update(hash, block, 1);
            memset(block, 0, 56);
        }

        uint64 longLen = uint64(len) << 3;
        for (i = 0; i < 8; i++)
        {
            block[64 - 1 - i] = longLen & 0xff;
            longLen >>= 8;
        }
        sha1_update(hash, block, 1);

#ifdef MANGO_LITTLE_ENDIAN
        hash[0] = byteswap(hash[0]);
        hash[1] = byteswap(hash[1]);
        hash[2] = byteswap(hash[2]);
        hash[3] = byteswap(hash[3]);
        hash[4] = byteswap(hash[4]);
#endif
    }

} // namespace mango
