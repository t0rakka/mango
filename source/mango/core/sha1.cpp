/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/hash.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/bits.hpp>
#include <mango/core/endian.hpp>
#include <mango/core/cpuinfo.hpp>

namespace
{
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

    void arm_sha1_transform(u32 state[5], const u8* block, int count)
    {
        // set K0..K3 constants
        uint32x4_t k0 = vdupq_n_u32(0x5A827999);
        uint32x4_t k1 = vdupq_n_u32(0x6ED9EBA1);
        uint32x4_t k2 = vdupq_n_u32(0x8F1BBCDC);
        uint32x4_t k3 = vdupq_n_u32(0xCA62C1D6);

        // load state
        uint32x4_t abcd = vld1q_u32(state);
        uint32_t e = state[4];

        for (int i = 0; i < count; ++i)
        {
            uint32x4_t abcd0 = abcd;

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

            // update state
            abcd = vaddq_u32(abcd, abcd0);
            e += e0;
        }

        // store state
        vst1q_u32(state, abcd);
        state[4] = e;
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

    void intel_sha1_transform(u32 *digest, const u8 *data, int blocks)
    {
        const __m128i e_mask    = _mm_set_epi64x(0xffffffff00000000ull, 0x0000000000000000ull);
        const __m128i shuf_mask = _mm_set_epi64x(0x0001020304050607ull, 0x08090a0b0c0d0e0full);

        // Load initial hash values
        __m128i abcd = _mm_loadu_si128((__m128i*) digest);
        __m128i e0   = _mm_setzero_si128();
        e0   = _mm_insert_epi32(e0, digest[4], 3);
        abcd = _mm_shuffle_epi32(abcd, 0x1b);
        e0   = _mm_and_si128(e0, e_mask);

        while (blocks-- > 0)
        {
            // Save hash values for addition after rounds
            __m128i abcd_save = abcd;
            __m128i e_save    = e0;

            __m128i msg0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 0));
            __m128i msg1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 16));
            __m128i msg2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 32));
            __m128i msg3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 48));

            __m128i e1;

            // Rounds 0-3
            msg0 = _mm_shuffle_epi8(msg0, shuf_mask);
            e0   = _mm_add_epi32(e0, msg0);
            e1   = abcd;
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 0);

            // Rounds 4-7
            msg1 = _mm_shuffle_epi8(msg1, shuf_mask);
            e1   = _mm_sha1nexte_epu32(e1, msg1);
            e0   = abcd;
            abcd = _mm_sha1rnds4_epu32(abcd, e1, 0);
            msg0 = _mm_sha1msg1_epu32(msg0, msg1);

            // Rounds 8-11
            msg2 = _mm_shuffle_epi8(msg2, shuf_mask);
            e0   = _mm_sha1nexte_epu32(e0, msg2);
            e1   = abcd;
            abcd = _mm_sha1rnds4_epu32(abcd, e0, 0);
            msg1 = _mm_sha1msg1_epu32(msg1, msg2);
            msg0 = _mm_xor_si128(msg0, msg2);

            // Rounds 12-15
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
        }

        abcd = _mm_shuffle_epi32(abcd, 0x1b);
        _mm_store_si128(reinterpret_cast<__m128i*>(digest), abcd);
        digest[4] = _mm_extract_epi32(e0, 3);
    }

#endif

    // ----------------------------------------------------------------------------------------
    // Generic C++ SHA1
    // ----------------------------------------------------------------------------------------

    #define K1 0x5A827999
    #define K2 0x6ED9EBA1
    #define K3 0x8F1BBCDC
    #define K4 0xCA62C1D6

    inline
    void F1(u32 A, u32& B, u32 C, u32 D, u32& E, u32 msg)
    {
        E += u32_select(B, C, D) + msg + K1 + u32_rol(A, 5);
        B = u32_rol(B, 30);
    }

    inline
    void F2(u32 A, u32& B, u32 C, u32 D, u32& E, u32 msg)
    {
        E += (B ^ C ^ D) + msg + K2 + u32_rol(A, 5);
        B = u32_rol(B, 30);
    }

    inline
    void F3(u32 A, u32& B, u32 C, u32 D, u32& E, u32 msg)
    {
        E += u32_select(B ^ C, D, C) + msg + K3 + u32_rol(A, 5);
        B = u32_rol(B, 30);
    }

    inline
    void F4(u32 A, u32& B, u32 C, u32 D, u32& E, u32 msg)
    {
        E += (B ^ C ^ D) + msg + K4 + u32_rol(A, 5);
        B = u32_rol(B, 30);
    }

    void generic_sha1_transform(u32* digest, const u8* data, int blocks)
    {
        u32 A = digest[0];
        u32 B = digest[1];
        u32 C = digest[2];
        u32 D = digest[3];
        u32 E = digest[4];

        while (blocks-- > 0)
        {
            u32 w[80];

            for (int i = 0; i < 16; ++i)
            {
                w[i] = uload32be(data + i * 4);
            }

            for (int i = 16; i < 80; i += 8)
            {
                w[i + 0] = u32_rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
                w[i + 1] = u32_rol(w[i - 2] ^ w[i - 7] ^ w[i - 13] ^ w[i - 15], 1);
                w[i + 2] = u32_rol(w[i - 1] ^ w[i - 6] ^ w[i - 12] ^ w[i - 14], 1);
                w[i + 3] = u32_rol(w[i + 0] ^ w[i - 5] ^ w[i - 11] ^ w[i - 13], 1);
                w[i + 4] = u32_rol(w[i + 1] ^ w[i - 4] ^ w[i - 10] ^ w[i - 12], 1);
                w[i + 5] = u32_rol(w[i + 2] ^ w[i - 3] ^ w[i -  9] ^ w[i - 11], 1);
                w[i + 6] = u32_rol(w[i + 3] ^ w[i - 2] ^ w[i -  8] ^ w[i - 10], 1);
                w[i + 7] = u32_rol(w[i + 4] ^ w[i - 1] ^ w[i -  7] ^ w[i -  9], 1);
            }

            F1(A, B, C, D, E, w[0]);
            F1(E, A, B, C, D, w[1]);
            F1(D, E, A, B, C, w[2]);
            F1(C, D, E, A, B, w[3]);
            F1(B, C, D, E, A, w[4]);
            F1(A, B, C, D, E, w[5]);
            F1(E, A, B, C, D, w[6]);
            F1(D, E, A, B, C, w[7]);
            F1(C, D, E, A, B, w[8]);
            F1(B, C, D, E, A, w[9]);
            F1(A, B, C, D, E, w[10]);
            F1(E, A, B, C, D, w[11]);
            F1(D, E, A, B, C, w[12]);
            F1(C, D, E, A, B, w[13]);
            F1(B, C, D, E, A, w[14]);
            F1(A, B, C, D, E, w[15]);
            F1(E, A, B, C, D, w[16]);
            F1(D, E, A, B, C, w[17]);
            F1(C, D, E, A, B, w[18]);
            F1(B, C, D, E, A, w[19]);

            F2(A, B, C, D, E, w[20]);
            F2(E, A, B, C, D, w[21]);
            F2(D, E, A, B, C, w[22]);
            F2(C, D, E, A, B, w[23]);
            F2(B, C, D, E, A, w[24]);
            F2(A, B, C, D, E, w[25]);
            F2(E, A, B, C, D, w[26]);
            F2(D, E, A, B, C, w[27]);
            F2(C, D, E, A, B, w[28]);
            F2(B, C, D, E, A, w[29]);
            F2(A, B, C, D, E, w[30]);
            F2(E, A, B, C, D, w[31]);
            F2(D, E, A, B, C, w[32]);
            F2(C, D, E, A, B, w[33]);
            F2(B, C, D, E, A, w[34]);
            F2(A, B, C, D, E, w[35]);
            F2(E, A, B, C, D, w[36]);
            F2(D, E, A, B, C, w[37]);
            F2(C, D, E, A, B, w[38]);
            F2(B, C, D, E, A, w[39]);

            F3(A, B, C, D, E, w[40]);
            F3(E, A, B, C, D, w[41]);
            F3(D, E, A, B, C, w[42]);
            F3(C, D, E, A, B, w[43]);
            F3(B, C, D, E, A, w[44]);
            F3(A, B, C, D, E, w[45]);
            F3(E, A, B, C, D, w[46]);
            F3(D, E, A, B, C, w[47]);
            F3(C, D, E, A, B, w[48]);
            F3(B, C, D, E, A, w[49]);
            F3(A, B, C, D, E, w[50]);
            F3(E, A, B, C, D, w[51]);
            F3(D, E, A, B, C, w[52]);
            F3(C, D, E, A, B, w[53]);
            F3(B, C, D, E, A, w[54]);
            F3(A, B, C, D, E, w[55]);
            F3(E, A, B, C, D, w[56]);
            F3(D, E, A, B, C, w[57]);
            F3(C, D, E, A, B, w[58]);
            F3(B, C, D, E, A, w[59]);

            F4(A, B, C, D, E, w[60]);
            F4(E, A, B, C, D, w[61]);
            F4(D, E, A, B, C, w[62]);
            F4(C, D, E, A, B, w[63]);
            F4(B, C, D, E, A, w[64]);
            F4(A, B, C, D, E, w[65]);
            F4(E, A, B, C, D, w[66]);
            F4(D, E, A, B, C, w[67]);
            F4(C, D, E, A, B, w[68]);
            F4(B, C, D, E, A, w[69]);
            F4(A, B, C, D, E, w[70]);
            F4(E, A, B, C, D, w[71]);
            F4(D, E, A, B, C, w[72]);
            F4(C, D, E, A, B, w[73]);
            F4(B, C, D, E, A, w[74]);
            F4(A, B, C, D, E, w[75]);
            F4(E, A, B, C, D, w[76]);
            F4(D, E, A, B, C, w[77]);
            F4(C, D, E, A, B, w[78]);
            F4(B, C, D, E, A, w[79]);

            A = (digest[0] += A);
            B = (digest[1] += B);
            C = (digest[2] += C);
            D = (digest[3] += D);
            E = (digest[4] += E);

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
        if ((getCPUFlags() & ARM_SHA1) != 0)
        {
            transform = arm_sha1_transform;
        }
#elif defined(MANGO_ENABLE_SHA)
        if ((getCPUFlags() & INTEL_SHA) != 0)
        {
            transform = intel_sha1_transform;
        }
#endif

        u32 size = u32(memory.size);
        const u8* data = memory.address;

        const int block_count = size / 64;
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

        ustore64be(block + 56, u64(memory.size * 8));
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
