/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/hash.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/bits.hpp>
#include <mango/core/endian.hpp>
#include <mango/core/cpuinfo.hpp>

namespace {
    using namespace mango;

#if defined(MANGO_ENABLE_SHA)

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
    * Intel SHA Extensions optimized implementation of a SHA-256 update function 
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

    void intel_sha2_transform(uint32 digest[8], const uint8* data, int block_count)
    {
        __m128i state0, state1;
        __m128i msg;
        __m128i msgtmp0, msgtmp1, msgtmp2, msgtmp3;
        __m128i tmp;
        __m128i shuf_mask;
        __m128i abef_save, cdgh_save;

        // Load initial hash values
        // Need to reorder these appropriately
        // DCBA, HGFE -> ABEF, CDGH
        tmp    = _mm_loadu_si128((__m128i*) digest);
        state1 = _mm_loadu_si128((__m128i*) (digest+4));

        tmp    = _mm_shuffle_epi32(tmp, 0xB1);       // CDAB
        state1 = _mm_shuffle_epi32(state1, 0x1B);    // EFGH
        state0 = _mm_alignr_epi8(tmp, state1, 8);    // ABEF
        state1 = _mm_blend_epi16(state1, tmp, 0xF0); // CDGH

        shuf_mask = _mm_set_epi64x(0x0c0d0e0f08090a0bull, 0x0405060700010203ull);

        while (block_count > 0)
        {
            // Save hash values for addition after rounds
            abef_save = state0;
            cdgh_save = state1;

            // Rounds 0-3
            msg     = _mm_loadu_si128((__m128i*) data);
            msgtmp0 = _mm_shuffle_epi8(msg, shuf_mask);
            msg    = _mm_add_epi32(msgtmp0, 
                     _mm_set_epi64x(0xE9B5DBA5B5C0FBCFull, 0x71374491428A2F98ull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);

            // Rounds 4-7
            msgtmp1 = _mm_loadu_si128((__m128i*) (data+16));
            msgtmp1 = _mm_shuffle_epi8(msgtmp1, shuf_mask);
            msg    = _mm_add_epi32(msgtmp1, 
                     _mm_set_epi64x(0xAB1C5ED5923F82A4ull, 0x59F111F13956C25Bull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
            msgtmp0 = _mm_sha256msg1_epu32(msgtmp0, msgtmp1);

            // Rounds 8-11
            msgtmp2 = _mm_loadu_si128((__m128i*) (data+32));
            msgtmp2 = _mm_shuffle_epi8(msgtmp2, shuf_mask);
            msg    = _mm_add_epi32(msgtmp2, 
                     _mm_set_epi64x(0x550C7DC3243185BEull, 0x12835B01D807AA98ull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
            msgtmp1 = _mm_sha256msg1_epu32(msgtmp1, msgtmp2);

            // Rounds 12-15
            msgtmp3 = _mm_loadu_si128((__m128i*) (data+48));
            msgtmp3 = _mm_shuffle_epi8(msgtmp3, shuf_mask);
            msg    = _mm_add_epi32(msgtmp3, 
                     _mm_set_epi64x(0xC19BF1749BDC06A7ull, 0x80DEB1FE72BE5D74ull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            tmp     = _mm_alignr_epi8(msgtmp3, msgtmp2, 4);
            msgtmp0 = _mm_add_epi32(msgtmp0, tmp);
            msgtmp0 = _mm_sha256msg2_epu32(msgtmp0, msgtmp3);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
            msgtmp2 = _mm_sha256msg1_epu32(msgtmp2, msgtmp3);

            // Rounds 16-19
            msg    = _mm_add_epi32(msgtmp0, 
                     _mm_set_epi64x(0x240CA1CC0FC19DC6ull, 0xEFBE4786E49B69C1ull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            tmp     = _mm_alignr_epi8(msgtmp0, msgtmp3, 4);
            msgtmp1 = _mm_add_epi32(msgtmp1, tmp);
            msgtmp1 = _mm_sha256msg2_epu32(msgtmp1, msgtmp0);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
            msgtmp3 = _mm_sha256msg1_epu32(msgtmp3, msgtmp0);

            // Rounds 20-23
            msg    = _mm_add_epi32(msgtmp1, 
                     _mm_set_epi64x(0x76F988DA5CB0A9DCull, 0x4A7484AA2DE92C6Full));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            tmp     = _mm_alignr_epi8(msgtmp1, msgtmp0, 4);
            msgtmp2 = _mm_add_epi32(msgtmp2, tmp);
            msgtmp2 = _mm_sha256msg2_epu32(msgtmp2, msgtmp1);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
            msgtmp0 = _mm_sha256msg1_epu32(msgtmp0, msgtmp1);

            // Rounds 24-27
            msg    = _mm_add_epi32(msgtmp2, 
                     _mm_set_epi64x(0xBF597FC7B00327C8ull, 0xA831C66D983E5152ull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            tmp     = _mm_alignr_epi8(msgtmp2, msgtmp1, 4);
            msgtmp3 = _mm_add_epi32(msgtmp3, tmp);
            msgtmp3 = _mm_sha256msg2_epu32(msgtmp3, msgtmp2);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
            msgtmp1 = _mm_sha256msg1_epu32(msgtmp1, msgtmp2);

            // Rounds 28-31
            msg    = _mm_add_epi32(msgtmp3, 
                     _mm_set_epi64x(0x1429296706CA6351ull, 0xD5A79147C6E00BF3ull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            tmp     = _mm_alignr_epi8(msgtmp3, msgtmp2, 4);
            msgtmp0 = _mm_add_epi32(msgtmp0, tmp);
            msgtmp0 = _mm_sha256msg2_epu32(msgtmp0, msgtmp3);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
            msgtmp2 = _mm_sha256msg1_epu32(msgtmp2, msgtmp3);

            // Rounds 32-35
            msg    = _mm_add_epi32(msgtmp0, 
                     _mm_set_epi64x(0x53380D134D2C6DFCull, 0x2E1B213827B70A85ull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            tmp     = _mm_alignr_epi8(msgtmp0, msgtmp3, 4);
            msgtmp1 = _mm_add_epi32(msgtmp1, tmp);
            msgtmp1 = _mm_sha256msg2_epu32(msgtmp1, msgtmp0);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
            msgtmp3 = _mm_sha256msg1_epu32(msgtmp3, msgtmp0);

            // Rounds 36-39
            msg    = _mm_add_epi32(msgtmp1, 
                     _mm_set_epi64x(0x92722C8581C2C92Eull, 0x766A0ABB650A7354ull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            tmp     = _mm_alignr_epi8(msgtmp1, msgtmp0, 4);
            msgtmp2 = _mm_add_epi32(msgtmp2, tmp);
            msgtmp2 = _mm_sha256msg2_epu32(msgtmp2, msgtmp1);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
            msgtmp0 = _mm_sha256msg1_epu32(msgtmp0, msgtmp1);

            // Rounds 40-43
            msg    = _mm_add_epi32(msgtmp2, 
                     _mm_set_epi64x(0xC76C51A3C24B8B70ull, 0xA81A664BA2BFE8A1ull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            tmp     = _mm_alignr_epi8(msgtmp2, msgtmp1, 4);
            msgtmp3 = _mm_add_epi32(msgtmp3, tmp);
            msgtmp3 = _mm_sha256msg2_epu32(msgtmp3, msgtmp2);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
            msgtmp1 = _mm_sha256msg1_epu32(msgtmp1, msgtmp2);

            // Rounds 44-47
            msg    = _mm_add_epi32(msgtmp3, 
                     _mm_set_epi64x(0x106AA070F40E3585ull, 0xD6990624D192E819ull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            tmp     = _mm_alignr_epi8(msgtmp3, msgtmp2, 4);
            msgtmp0 = _mm_add_epi32(msgtmp0, tmp);
            msgtmp0 = _mm_sha256msg2_epu32(msgtmp0, msgtmp3);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
            msgtmp2 = _mm_sha256msg1_epu32(msgtmp2, msgtmp3);

            // Rounds 48-51
            msg    = _mm_add_epi32(msgtmp0, 
                     _mm_set_epi64x(0x34B0BCB52748774Cull, 0x1E376C0819A4C116ull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            tmp     = _mm_alignr_epi8(msgtmp0, msgtmp3, 4);
            msgtmp1 = _mm_add_epi32(msgtmp1, tmp);
            msgtmp1 = _mm_sha256msg2_epu32(msgtmp1, msgtmp0);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
            msgtmp3 = _mm_sha256msg1_epu32(msgtmp3, msgtmp0);

            // Rounds 52-55
            msg    = _mm_add_epi32(msgtmp1, 
                     _mm_set_epi64x(0x682E6FF35B9CCA4Full, 0x4ED8AA4A391C0CB3ull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            tmp     = _mm_alignr_epi8(msgtmp1, msgtmp0, 4);
            msgtmp2 = _mm_add_epi32(msgtmp2, tmp);
            msgtmp2 = _mm_sha256msg2_epu32(msgtmp2, msgtmp1);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);

            // Rounds 56-59
            msg    = _mm_add_epi32(msgtmp2, 
                     _mm_set_epi64x(0x8CC7020884C87814ull, 0x78A5636F748F82EEull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            tmp     = _mm_alignr_epi8(msgtmp2, msgtmp1, 4);
            msgtmp3 = _mm_add_epi32(msgtmp3, tmp);
            msgtmp3 = _mm_sha256msg2_epu32(msgtmp3, msgtmp2);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);

            // Rounds 60-63
            msg    = _mm_add_epi32(msgtmp3, 
                     _mm_set_epi64x(0xC67178F2BEF9A3F7ull, 0xA4506CEB90BEFFFAull));
            state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
            msg    = _mm_shuffle_epi32(msg, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, msg);

            // Add current hash values with previously saved
            state0 = _mm_add_epi32(state0, abef_save);
            state1 = _mm_add_epi32(state1, cdgh_save);

            data += 64;
            --block_count;
        }

        // Write hash values back in the correct order
        tmp    = _mm_shuffle_epi32(state0, 0x1B);    // FEBA
        state1 = _mm_shuffle_epi32(state1, 0xB1);    // DCHG
        state0 = _mm_blend_epi16(tmp, state1, 0xF0); // DCBA
        state1 = _mm_alignr_epi8(state1, tmp, 8);    // ABEF

        _mm_storeu_si128(reinterpret_cast<__m128i *>(digest + 0), state0);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(digest + 4), state1);
    }

#endif

    // ----------------------------------------------------------------------------------------
    // Generic C++ SHA-256
    // ----------------------------------------------------------------------------------------

    constexpr uint32 rotateRight(uint32 value, int count)
    {
        return (value >> count) | (value << (32 - count));
    }

    void generic_sha2_transform(uint32 state[8], const uint8* data, int block_count)
    {
        static const uint32 k[] =
        {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        while (block_count > 0)
        {
            uint32 a = state[0];
            uint32 b = state[1];
            uint32 c = state[2];
            uint32 d = state[3];
            uint32 e = state[4];
            uint32 f = state[5];
            uint32 g = state[6];
            uint32 h = state[7];

            uint32 w[64];

            for (int i = 0; i < 16; ++i)
            {
                w[i] = uload32be(data + i * 4);

                uint32 s1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
                uint32 ch = (e & f) ^ ((~e) & g);
                uint32 x = h + s1 + ch + k[i] + w[i];
                uint32 s0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
                uint32 maj = (a & b) ^ (a & c) ^ (b & c);
                uint32 y = s0 + maj;
                
                h = g;
                g = f;
                f = e;
                e = d + x;
                d = c;
                c = b;
                b = a;
                a = x + y;
            }

            for (int i = 16; i < 64; ++i)
            {
                uint32 t0 = rotateRight(w[i - 15], 7) ^ rotateRight(w[i - 15], 18) ^ (w[i - 15] >> 3);
                uint32 t1 = rotateRight(w[i - 2], 17) ^ rotateRight(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + t0 + w[i - 7] + t1;

                uint32 s1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
                uint32 ch = (e & f) ^ ((~e) & g);
                uint32 x = h + s1 + ch + k[i] + w[i];
                uint32 s0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
                uint32 maj = (a & b) ^ (a & c) ^ (b & c);
                uint32 y = s0 + maj;
                
                h = g;
                g = f;
                f = e;
                e = d + x;
                d = c;
                c = b;
                b = a;
                a = x + y;
            }

            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
            state[4] += e;
            state[5] += f;
            state[6] += g;
            state[7] += h;

            --block_count;
            data += 64;
        }
    }

} // namespace

namespace mango {

    void sha2(uint32 hash[8], Memory memory)
    {
        hash[0] = 0x6a09e667;
        hash[1] = 0xbb67ae85;
        hash[2] = 0x3c6ef372;
        hash[3] = 0xa54ff53a;
        hash[4] = 0x510e527f;
        hash[5] = 0x9b05688c;
        hash[6] = 0x1f83d9ab;
        hash[7] = 0x5be0cd19;

        auto transform = generic_sha2_transform;
#if defined(MANGO_ENABLE_SHA)
        if ((getCPUFlags() & CPU_SHA) != 0)
        {
            transform = intel_sha2_transform;
        }
#endif

        uint32 size = uint32(memory.size);
        const uint8* data = memory.address;

        const int block_count = size / 64;
        transform(hash, data, block_count);
        data += block_count * 64;
        size -= block_count * 64;
        
        uint8 buffer[64];
        std::memcpy(buffer, data, size);
        std::memset(buffer + size, 0, 64 - size);
        buffer[size] = 0x80;

        if (size >= 56)
        {
            transform(hash, buffer, 1);
            std::memset(buffer, 0, 56);
        }

        ustore64be(buffer + 56, memory.size * 8);
        transform(hash, buffer, 1);

#ifdef MANGO_LITTLE_ENDIAN
        hash[0] = byteswap(hash[0]);
        hash[1] = byteswap(hash[1]);
        hash[2] = byteswap(hash[2]);
        hash[3] = byteswap(hash[3]);
        hash[4] = byteswap(hash[4]);
        hash[5] = byteswap(hash[5]);
        hash[6] = byteswap(hash[6]);
        hash[7] = byteswap(hash[7]);
#endif
    }

} // namespace mango
