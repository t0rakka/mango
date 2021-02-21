/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstdio>
#include <cstddef>
#include <cstring>
#include "jpeg.hpp"

// ----------------------------------------------------------------------------
// jdarith
// ----------------------------------------------------------------------------

/*
 * jdarith.c
 *
 * Developed 1997-2009 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains portable arithmetic entropy decoding routines for JPEG
 * (implementing the ISO/IEC IS 10918-1 and CCITT Recommendation ITU-T T.81).
 *
 * Both sequential and progressive modes are supported in this single module.
 *
 * Suspension is not currently supported in this module.
 */

namespace
{
    using namespace mango;
    using namespace jpeg;

    const u32 jpeg_aritab[] =
    {
        0x5a1d0181, 0x2586020e, 0x11140310, 0x080b0412, 0x03d80514, 0x01da0617,
        0x00e50719, 0x006f081c, 0x0036091e, 0x001a0a21, 0x000d0b23, 0x00060c09,
        0x00030d0a, 0x00010d0c, 0x5a7f0f8f, 0x3f251024, 0x2cf21126, 0x207c1227,
        0x17b91328, 0x1182142a, 0x0cef152b, 0x09a1162d, 0x072f172e, 0x055c1830,
        0x04061931, 0x03031a33, 0x02401b34, 0x01b11c36, 0x01441d38, 0x00f51e39,
        0x00b71f3b, 0x008a203c, 0x0068213e, 0x004e223f, 0x003b2320, 0x002c0921,
        0x5ae125a5, 0x484c2640, 0x3a0d2741, 0x2ef12843, 0x261f2944, 0x1f332a45,
        0x19a82b46, 0x15182c48, 0x11772d49, 0x0e742e4a, 0x0bfb2f4b, 0x09f8304d,
        0x0861314e, 0x0706324f, 0x05cd3330, 0x04de3432, 0x040f3532, 0x03633633,
        0x02d43734, 0x025c3835, 0x01f83936, 0x01a43a37, 0x01603b38, 0x01253c39,
        0x00f63d3a, 0x00cb3e3b, 0x00ab3f3d, 0x008f203d, 0x5b1241c1, 0x4d044250,
        0x412c4351, 0x37d84452, 0x2fe84553, 0x293c4654, 0x23794756, 0x1edf4857,
        0x1aa94957, 0x174e4a48, 0x14244b48, 0x119c4c4a, 0x0f6b4d4a, 0x0d514e4b,
        0x0bb64f4d, 0x0a40304d, 0x583251d0, 0x4d1c5258, 0x438e5359, 0x3bdd545a,
        0x34ee555b, 0x2eae565c, 0x299a575d, 0x25164756, 0x557059d8, 0x4ca95a5f,
        0x44d95b60, 0x3e225c61, 0x38245d63, 0x32b45e63, 0x2e17565d, 0x56a860df,
        0x4f466165, 0x47e56266, 0x41cf6367, 0x3c3d6468, 0x375e5d63, 0x52316669,
        0x4c0f676a, 0x4639686b, 0x415e6367, 0x56276ae9, 0x50e76b6c, 0x4b85676d,
        0x55976d6e, 0x504f6b6f, 0x5a106fee, 0x55226d70, 0x59eb6ff0, 0x5a1d7171
    };

    inline u8 get_byte(BitBuffer& buffer)
    {
        // guard against corrupted bit-streams
        if (buffer.ptr >= buffer.end)
            return 0;

        u8 value = *buffer.ptr++;
        if (value == 0xff)
        {
            // skip stuff byte (0x00)
            ++buffer.ptr;
        }

        return value;
    }

    int arith_decode(Arithmetic& e, BitBuffer& buffer, u8* st)
    {
        while (e.a < 0x8000L)
        {
            if (--e.ct < 0)
            {
                int data = get_byte(buffer);
                e.c = (e.c << 8) | data;
                e.ct += 8;
            }

            e.a <<= 1;
        }

        // Fetch values from our compact representation of Table D.2:
        // Qe values and probability estimation state machine
        int sv = *st;
        
        u32 qe = jpeg_aritab[sv & 0x7F];
        u8 nextLPS = qe & 0xFF; qe >>= 8;
        u8 nextMPS = qe & 0xFF; qe >>= 8;
        
        // Decode & estimation procedures per sections D.2.4 & D.2.5
        u32 temp = e.a - qe;
        e.a = temp;
        temp <<= e.ct;
        
        if (e.c >= temp)
        {
            e.c -= temp;
            
            // Conditional LPS (less probable symbol) exchange
            if (e.a < qe)
            {
                e.a = qe;
                *st = (sv & 0x80) ^ nextMPS; // Estimate_after_MPS
            }
            else
            {
                e.a = qe;
                *st = (sv & 0x80) ^ nextLPS; // Estimate_after_LPS
                sv ^= 0x80; // Exchange LPS/MPS
            }
        }
        else
        {
            if (e.a < 0x8000L)
            {
                // Conditional MPS (more probable symbol) exchange
                if (e.a < qe)
                {
                    *st = (sv & 0x80) ^ nextLPS; // Estimate_after_LPS
                    sv ^= 0x80; // Exchange LPS/MPS
                }
                else
                {
                    *st = (sv & 0x80) ^ nextMPS; // Estimate_after_MPS
                }
            }
        }
        
        return sv >> 7;
    }

} // namespace

namespace mango::jpeg
{

    // ----------------------------------------------------------------------------
    // arithmetic decoder
    // ----------------------------------------------------------------------------

    void arith_decode_mcu_lossless(s16* output, DecodeState* state)
    {
        Arithmetic& arithmetic = state->arithmetic;
        BitBuffer& buffer = state->buffer;
        DecodeBlock* block = state->block;

        for (int j = 0; j < state->blocks; ++j)
        {
            const int ci = block->pred;

            // DC
            int tbl = block->dc;
            u8* st = arithmetic.dc_stats[tbl] + arithmetic.dc_context[ci];

            if (arith_decode(arithmetic, buffer, st) == 0)
            {
                arithmetic.dc_context[ci] = 0;
            }
            else
            {
                int sign;
                int v, m;

                sign = arith_decode(arithmetic, buffer, st + 1);
                st += 2;
                st += sign;

                if ((m = arith_decode(arithmetic, buffer, st)) != 0)
                {
                    st = arithmetic.dc_stats[tbl] + 20;
                    while (arith_decode(arithmetic, buffer, st))
                    {
                        m <<= 1;
                        ++st;
                    }
                }

                if (m < (int) ((1L << arithmetic.dc_L[tbl]) >> 1))
                {
                    // zero diff category
                    arithmetic.dc_context[ci] = 0;
                }
                else if (m > (int) ((1L << arithmetic.dc_U[tbl]) >> 1))
                {
                    // large diff category
                    arithmetic.dc_context[ci] = 12 + (sign * 4);
                }
                else
                {
                    // small diff category
                    arithmetic.dc_context[ci] = 4 + (sign * 4);
                }

                v = m;

                st += 14;
                while (m >>= 1)
                {
                    if (arith_decode(arithmetic, buffer, st))
                        v |= m;
                }

                v += 1; if (sign) v = -v;
                arithmetic.last_dc_value[ci] += v;
            }

            output[j] = s16(arithmetic.last_dc_value[ci]);
        }
    }

    void arith_decode_mcu(s16* output, DecodeState* state)
    {
        const u8* zigzagTable = state->zigzagTable;
        Arithmetic& arithmetic = state->arithmetic;
        BitBuffer& buffer = state->buffer;
        DecodeBlock* block = state->block;

        std::memset(output, 0, state->blocks * 64 * sizeof(s16));

        const int end = state->spectralEnd;

        for (int j = 0; j < state->blocks; ++j)
        {
            const int ci = block->pred;

            // DC
            int tbl = block->dc;
            u8* st = arithmetic.dc_stats[tbl] + arithmetic.dc_context[ci];

            if (arith_decode(arithmetic, buffer, st) == 0)
            {
                arithmetic.dc_context[ci] = 0;
            }
            else
            {
                int sign;
                int v, m;

                sign = arith_decode(arithmetic, buffer, st + 1);
                st += 2;
                st += sign;

                if ((m = arith_decode(arithmetic, buffer, st)) != 0)
                {
                    st = arithmetic.dc_stats[tbl] + 20;
                    while (arith_decode(arithmetic, buffer, st))
                    {
                        m <<= 1;
                        ++st;
                    }
                }

                if (m < (int) ((1L << arithmetic.dc_L[tbl]) >> 1))
                {
                    // zero diff category
                    arithmetic.dc_context[ci] = 0;
                }
                else if (m > (int) ((1L << arithmetic.dc_U[tbl]) >> 1))
                {
                    // large diff category
                    arithmetic.dc_context[ci] = 12 + (sign * 4);
                }
                else
                {
                    // small diff category
                    arithmetic.dc_context[ci] = 4 + (sign * 4);
                }

                v = m;

                st += 14;
                while (m >>= 1)
                {
                    if (arith_decode(arithmetic, buffer, st))
                        v |= m;
                }

                v += 1; if (sign) v = -v;
                arithmetic.last_dc_value[ci] += v;
            }

            output[0] = s16(arithmetic.last_dc_value[ci]);

            // AC
            tbl = block->ac;
            u8* ac_stats = arithmetic.ac_stats[tbl];
            u8 ac_K = arithmetic.ac_K[tbl];

            for (int k = 1; k <= end; k++)
            {
                int sign;
                int v, m;

                st = ac_stats + 3 * (k - 1);

                if (arith_decode(arithmetic, buffer, st))
                    break;

                while (arith_decode(arithmetic, buffer, st + 1) == 0)
                {
                    st += 3;
                    ++k;
                }

                sign = arith_decode(arithmetic, buffer, arithmetic.fixed_bin);
                st += 2;

                if ((m = arith_decode(arithmetic, buffer, st)) != 0)
                {
                    if (arith_decode(arithmetic, buffer, st))
                    {
                        m <<= 1;
                        st = ac_stats + (k <= ac_K ? 189 : 217);

                        while (arith_decode(arithmetic, buffer, st))
                        {
                            m <<= 1;
                            ++st;
                        }
                    }
                }
                v = m;

                st += 14;
                while (m >>= 1)
                {
                    if (arith_decode(arithmetic, buffer, st))
                    {
                        v |= m;
                    }
                }
                v += 1; if (sign) v = -v;

                output[zigzagTable[k]] = s16(v);
            }

            ++block;
            output += 64;
        }
    }

    void arith_decode_dc_first(s16* output, DecodeState* state)
    {
        Arithmetic& arithmetic = state->arithmetic;
        BitBuffer& buffer = state->buffer;
        DecodeBlock* block = state->block;

        for (int j = 0; j < state->blocks; ++j)
        {
            s16* dest = output + block->offset;
            const int ci = block->pred;

            std::memset(dest, 0, 64 * sizeof(s16));

            int tbl = block->dc;
            u8* st = arithmetic.dc_stats[tbl] + arithmetic.dc_context[ci];

            int sign;
            int v, m;

            if (arith_decode(arithmetic, buffer, st) == 0)
            {
                arithmetic.dc_context[ci] = 0;
            }
            else
            {
                sign = arith_decode(arithmetic, buffer, st + 1);
                st += 2; st += sign;
                
                if ((m = arith_decode(arithmetic, buffer, st)) != 0)
                {
                    st = arithmetic.dc_stats[tbl] + 20;	// Table F.4: X1 = 20
                    while (arith_decode(arithmetic, buffer, st))
                    {
                        m += m;
                        ++st;
                    }
                }
                
                // Section F.1.4.4.1.2: Establish dc_context conditioning category
                if (m < (int) ((1L << arithmetic.dc_L[tbl]) >> 1))
                {
                    // zero diff category
                    arithmetic.dc_context[ci] = 0;
                }
                else if (m > (int) ((1L << arithmetic.dc_U[tbl]) >> 1))
                {
                    // large diff category
                    arithmetic.dc_context[ci] = 12 + (sign * 4);
                }
                else
                {
                    // small diff category
                    arithmetic.dc_context[ci] = 4 + (sign * 4);
                }
                
                v = m;
                
                // Figure F.24: Decoding the magnitude bit pattern of v
                st += 14;
                while (m >>= 1)
                {
                    if (arith_decode(arithmetic, buffer, st))
                        v |= m;
                }
                
                v += 1; if (sign) v = -v;
                arithmetic.last_dc_value[ci] += v;
            }

            dest[0] = s16(arithmetic.last_dc_value[ci] << state->successiveLow);
            ++block;
        }
    }

    void arith_decode_dc_refine(s16* output, DecodeState* state)
    {
        Arithmetic& arithmetic = state->arithmetic;
        BitBuffer& buffer = state->buffer;
        u8* st = arithmetic.fixed_bin;

        for (int j = 0; j < state->blocks; ++j)
        {
            s16* dest = output + state->block[j].offset;

            // Encoded data is simply the next bit of the two's-complement DC value
            if (arith_decode(arithmetic, buffer, st))
            {
                dest[0] |= (1 << state->successiveLow);
            }
        }
    }

    void arith_decode_ac_first(s16* output, DecodeState* state)
    {
        const u8* zigzagTable = state->zigzagTable;
        Arithmetic& arithmetic = state->arithmetic;
        BitBuffer& buffer = state->buffer;

        const int start = state->spectralStart;
        const int end = state->spectralEnd;

        u8* ac_stats = arithmetic.ac_stats[state->block[0].ac];
        u8 ac_K = arithmetic.ac_K[state->block[0].ac];
        
        // Figure F.20: Decode_AC_coefficients
        for (int k = start; k <= end; k++)
        {
            u8* st = ac_stats + 3 * (k - 1);
            
            if (arith_decode(arithmetic, buffer, st))
                break; // EOB flag
            
            while (arith_decode(arithmetic, buffer, st + 1) == 0)
            {
                st += 3;
                ++k;
            }
            
            // Figure F.21: Decoding nonzero value v
            // Figure F.22: Decoding the sign of v
            int sign = arith_decode(arithmetic, buffer, arithmetic.fixed_bin);
            st += 2;
            
            int m;
            
            // Figure F.23: Decoding the magnitude category of v
            if ((m = arith_decode(arithmetic, buffer, st)) != 0)
            {
                if (arith_decode(arithmetic, buffer, st))
                {
                    m <<= 1;
                    st = ac_stats + (k <= ac_K ? 189 : 217);
                    
                    while (arith_decode(arithmetic, buffer, st))
                    {
                        m += m;
                        ++st;
                    }
                }
            }
            
            int v = m;
            
            // Figure F.24: Decoding the magnitude bit pattern of v
            st += 14;
            while (m >>= 1)
            {
                if (arith_decode(arithmetic, buffer, st))
                    v |= m;
            }
            
            v += 1; if (sign) v = -v;
            
            // Scale and output coefficient in natural order
            output[zigzagTable[k]] = s16(v << state->successiveLow);
        }
    }

    void arith_decode_ac_refine(s16* output, DecodeState* state)
    {
        const u8* zigzagTable = state->zigzagTable;
        Arithmetic& arithmetic = state->arithmetic;
        BitBuffer& buffer = state->buffer;

        const int start = state->spectralStart;
        const int end = state->spectralEnd;

        u8* ac_stats = arithmetic.ac_stats[state->block[0].ac];

        int p1 = 1 << state->successiveLow; //  1 in the bit position being coded
        int m1 = (-1) << state->successiveLow; // -1 in the bit position being coded

        int kex;

        // Establish EOBx (previous stage end-of-block) index
        for (kex = end; kex > 0; kex--)
        {
            if (output[zigzagTable[kex]])
                break;
        }

        for (int k = start; k <= end; k++)
        {
            u8* st = ac_stats + 3 * (k - 1);

            if (k > kex)
            {
                if (arith_decode(arithmetic, buffer, st))
                    break; // EOB flag
            }

            s16* coef = output + zigzagTable[k];
            for (;;)
            {
                if (*coef)
                {
                    // previously nonzero coef
                    if (arith_decode(arithmetic, buffer, st + 2))
                    {
                        if (*coef < 0)
                            *coef += s16(m1);
                        else
                            *coef += s16(p1);
                    }
                    break;
                }

                if (arith_decode(arithmetic, buffer, st + 1))
                {
                    // newly nonzero coef
                    if (arith_decode(arithmetic, buffer, arithmetic.fixed_bin))
                        *coef = s16(m1);
                    else
                        *coef = s16(p1);
                    
                    break;
                }

                st += 3;
                ++k;
            }
        }
    }

    // ----------------------------------------------------------------------------
    // Arithmetic
    // ----------------------------------------------------------------------------

    Arithmetic::Arithmetic()
    {
        fixed_bin[0] = 113;
        std::memset(dc_L, 0, JPEG_NUM_ARITH_TBLS);
        std::memset(dc_U, 1, JPEG_NUM_ARITH_TBLS);
        std::memset(ac_K, 5, JPEG_NUM_ARITH_TBLS);
    }

    Arithmetic::~Arithmetic()
    {
    }

    void Arithmetic::restart(BitBuffer& buffer)
    {
        u8 v0 = get_byte(buffer);
        u8 v1 = get_byte(buffer);

        c = (v0 << 8) | v1;
        a = 0x10000;
        ct = 0;

        std::memset(dc_stats, 0, JPEG_NUM_ARITH_TBLS * JPEG_DC_STAT_BINS);
        std::memset(ac_stats, 0, JPEG_NUM_ARITH_TBLS * JPEG_AC_STAT_BINS);

        for (int i = 0; i < JPEG_MAX_COMPS_IN_SCAN; ++i)
        {
            last_dc_value[i] = 0;
            dc_context[i] = 0;
        }
    }

} // namespace mango::jpeg
