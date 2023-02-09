/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "jpeg.hpp"

namespace
{
    using namespace mango;
    using namespace jpeg;

    struct IDCT
    {
        int x0, x1, x2, x3;
        int y0, y1, y2, y3;

        void compute(int s0, int s1, int s2, int s3, int s4, int s5, int s6, int s7)
        {
            const int n0 = (s2 + s6) * 2217;
            const int t2 = n0 + s6 * -7567;
            const int t3 = n0 + s2 * 3135;
            const int t0 = (s0 + s4) << 12;
            const int t1 = (s0 - s4) << 12;
            x0 = t0 + t3;
            x3 = t0 - t3;
            x1 = t1 + t2;
            x2 = t1 - t2;

            int p1 = s7 + s1;
            int p2 = s5 + s3;
            int p3 = s7 + s3;
            int p4 = s5 + s1;
            int p5 = (p3 + p4) * 4816;
            p1 = p1 * -3685 + p5;
            p2 = p2 * -10497 + p5;
            p3 = p3 * -8034;
            p4 = p4 * -1597;
            y0 = p1 + p3 + s7 * 1223;
            y1 = p2 + p4 + s5 * 8410;
            y2 = p2 + p3 + s3 * 12586;
            y3 = p1 + p4 + s1 * 6149;
        }
    };

    template <int PRECISION>
    void idct(u8* dest, const s16* data, const s16* qt)
    {
        int temp[64];
        int* v;

        const s16* s = data;

        v = temp;

        for (int i = 0; i < 8; ++i)
        {
            if (s[i + 8 * 1] || s[i + 8 * 2] || s[i + 8 * 3] || s[i + 8 * 4] || s[i + 8 * 5] || s[i + 8 * 6] || s[i + 8 * 7])
            {
                // dequantize
                const int s0 = s[i + 8 * 0] * qt[i + 8 * 0];
                const int s1 = s[i + 8 * 1] * qt[i + 8 * 1];
                const int s2 = s[i + 8 * 2] * qt[i + 8 * 2];
                const int s3 = s[i + 8 * 3] * qt[i + 8 * 3];
                const int s4 = s[i + 8 * 4] * qt[i + 8 * 4];
                const int s5 = s[i + 8 * 5] * qt[i + 8 * 5];
                const int s6 = s[i + 8 * 6] * qt[i + 8 * 6];
                const int s7 = s[i + 8 * 7] * qt[i + 8 * 7];

                IDCT idct;
                idct.compute(s0, s1, s2, s3, s4, s5, s6, s7);
                const int bias = 0x200;
                idct.x0 += bias;
                idct.x1 += bias;
                idct.x2 += bias;
                idct.x3 += bias;
                v[0] = (idct.x0 + idct.y3) >> 10;
                v[1] = (idct.x1 + idct.y2) >> 10;
                v[2] = (idct.x2 + idct.y1) >> 10;
                v[3] = (idct.x3 + idct.y0) >> 10;
                v[4] = (idct.x3 - idct.y0) >> 10;
                v[5] = (idct.x2 - idct.y1) >> 10;
                v[6] = (idct.x1 - idct.y2) >> 10;
                v[7] = (idct.x0 - idct.y3) >> 10;
            }
            else
            {
                int dc = (s[i] * qt[i]) << 2;
                v[0] = dc;
                v[1] = dc;
                v[2] = dc;
                v[3] = dc;
                v[4] = dc;
                v[5] = dc;
                v[6] = dc;
                v[7] = dc;
            }

            v += 8;
        }

        v = temp;
        const int shift = PRECISION + 9;

        for (int i = 0; i < 8; ++i)
        {
            IDCT idct;
            idct.compute(v[0], v[8], v[16], v[24], v[32], v[40], v[48], v[56]);
            ++v;
            const int bias = 0x10000 + (128 << shift);
            idct.x0 += bias;
            idct.x1 += bias;
            idct.x2 += bias;
            idct.x3 += bias;
            dest[0] = byteclamp((idct.x0 + idct.y3) >> shift);
            dest[1] = byteclamp((idct.x1 + idct.y2) >> shift);
            dest[2] = byteclamp((idct.x2 + idct.y1) >> shift);
            dest[3] = byteclamp((idct.x3 + idct.y0) >> shift);
            dest[4] = byteclamp((idct.x3 - idct.y0) >> shift);
            dest[5] = byteclamp((idct.x2 - idct.y1) >> shift);
            dest[6] = byteclamp((idct.x1 - idct.y2) >> shift);
            dest[7] = byteclamp((idct.x0 - idct.y3) >> shift);
            dest += 8;
        }
    }

} // namespace

namespace mango::jpeg
{

    // ------------------------------------------------------------------------------------------------
    // Generic C++ implementation
    // ------------------------------------------------------------------------------------------------

    void idct8(u8* dest, const s16* data, const s16* qt)
    {
        idct<8>(dest, data, qt);
    }

    void idct12(u8* dest, const s16* data, const s16* qt)
    {
        idct<12>(dest, data, qt);
    }

#if defined(MANGO_ENABLE_SSE2)

    // ------------------------------------------------------------------------------------------------
    // SSE2 implementation
    // ------------------------------------------------------------------------------------------------

    // Copyright 2009 Intel Corporation
    // All Rights Reserved
    //
    // Permission is granted to use, copy, distribute and prepare derivative works of this
    // software for any purpose and without fee, provided, that the above copyright notice
    // and this statement appear in all copies.  Intel makes no representations about the
    // suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
    // INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
    // INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
    // INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
    // WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
    // assume any responsibility for any errors which may appear in this software nor any
    // responsibility to update it.
    //
    // From:
    // https://software.intel.com/sites/default/files/m/d/4/1/d/8/UsingIntelAVXToImplementIDCT-r1_5.pdf
    // https://software.intel.com/file/29048
    //
    // Requires SSE
    //

    // Table for rows 0,4 - constants are multiplied on cos_4_16
    alignas(16)
    s16 shortM128_tab_i_04 [] =
    {
        16384, 21407, 16384, 8867, 16384, -8867, 16384, -21407,
        16384, 8867, -16384, -21407, -16384, 21407, 16384, -8867,
        22725, 19266, 19266, -4520, 12873, -22725, 4520, -12873,
        12873, 4520, -22725, -12873, 4520, 19266, 19266, -22725
    };

    // Table for rows 1,7 - constants are multiplied on cos_1_16
    alignas(16)
    s16 shortM128_tab_i_17 [] =
    {
        22725, 29692, 22725, 12299, 22725, -12299, 22725, -29692,
        22725, 12299, -22725, -29692, -22725, 29692, 22725, -12299,
        31521, 26722, 26722, -6270, 17855, -31521, 6270, -17855,
        17855, 6270, -31521, -17855, 6270, 26722, 26722, -31521
    };

    // Table for rows 2,6 - constants are multiplied on cos_2_16
    alignas(16)
    s16 shortM128_tab_i_26 [] =
    {
        21407, 27969, 21407, 11585, 21407, -11585, 21407, -27969,
        21407, 11585, -21407, -27969, -21407, 27969, 21407, -11585,
        29692, 25172, 25172, -5906, 16819, -29692, 5906, -16819,
        16819, 5906, -29692, -16819, 5906, 25172, 25172, -29692
    };

    // Table for rows 3,5 - constants are multiplied on cos_3_16
    alignas(16)
    s16 shortM128_tab_i_35 [] =
    {
        19266, 25172, 19266, 10426, 19266, -10426, 19266, -25172,
        19266, 10426, -19266, -25172, -19266, 25172, 19266, -10426,
        26722, 22654, 22654, -5315, 15137, -26722, 5315, -15137,
        15137, 5315, -26722, -15137, 5315, 22654, 22654, -26722
    };

    void idct_sse2(u8* dest, const s16* src, const s16* qt)
    {
        const __m128i* data = reinterpret_cast<const __m128i *>(src);
        const __m128i* qtable = reinterpret_cast<const __m128i *>(qt);

        // Load and dequantize
        __m128i v0 = _mm_mullo_epi16(data[0], qtable[0]);
        __m128i v1 = _mm_mullo_epi16(data[1], qtable[1]);
        __m128i v2 = _mm_mullo_epi16(data[2], qtable[2]);
        __m128i v3 = _mm_mullo_epi16(data[3], qtable[3]);
        __m128i v4 = _mm_mullo_epi16(data[4], qtable[4]);
        __m128i v5 = _mm_mullo_epi16(data[5], qtable[5]);
        __m128i v6 = _mm_mullo_epi16(data[6], qtable[6]);
        __m128i v7 = _mm_mullo_epi16(data[7], qtable[7]);

        __m128i r_xmm0, r_xmm4;
        __m128i r_xmm1, r_xmm2, r_xmm3, r_xmm5, r_xmm6, r_xmm7;

        __m128i row0, row1, row2, row3, row4, row5, row6, row7;

        const __m128i* table04 = reinterpret_cast<const __m128i*>(shortM128_tab_i_04);
        const __m128i* table26 = reinterpret_cast<const __m128i*>(shortM128_tab_i_26);

        //Row 1 and Row 3
        r_xmm0 = v0;
        r_xmm4 = v2;

        // *** Work on the data in xmm0
        //low shuffle mask = 0xd8 = 11 01 10 00
        //get short 2 and short 0 into ls 32-bits
        r_xmm0 = _mm_shufflelo_epi16(r_xmm0, 0xd8);

        // copy short 2 and short 0 to all locations
        r_xmm1 = _mm_shuffle_epi32(r_xmm0, 0);
            
        // add to those copies
        r_xmm1 = _mm_madd_epi16(r_xmm1, table04[0]);

        // shuffle mask = 0x55 = 01 01 01 01
        // copy short 3 and short 1 to all locations
        r_xmm3 = _mm_shuffle_epi32(r_xmm0, 0x55);
            
        // high shuffle mask = 0xd8 = 11 01 10 00
        // get short 6 and short 4 into bit positions 64-95
        // get short 7 and short 5 into bit positions 96-127
        r_xmm0 = _mm_shufflehi_epi16(r_xmm0, 0xd8);
            
        // add to short 3 and short 1
        r_xmm3 = _mm_madd_epi16(r_xmm3, table04[2]);
            
        // shuffle mask = 0xaa = 10 10 10 10
        // copy short 6 and short 4 to all locations
        r_xmm2 = _mm_shuffle_epi32(r_xmm0, 0xaa);
            
        // shuffle mask = 0xaa = 11 11 11 11
        // copy short 7 and short 5 to all locations
        r_xmm0 = _mm_shuffle_epi32(r_xmm0, 0xff);
            
        // add to short 6 and short 4
        r_xmm2 = _mm_madd_epi16(r_xmm2, table04[1]);

        // *** Work on the data in xmm4
        // high shuffle mask = 0xd8 11 01 10 00
        // get short 6 and short 4 into bit positions 64-95
        // get short 7 and short 5 into bit positions 96-127
        r_xmm4 = _mm_shufflehi_epi16(r_xmm4, 0xd8);

        const __m128i round_inv_row = _mm_set_epi16(0, 2048, 0, 2048, 0, 2048, 0, 2048);
        const __m128i tg = _mm_set_epi16(-19195, -19195, -21746, -21746, 27146, 27146, 13036, 13036);

        // (xmm0 short 2 and short 0 plus pSi) + some constants
        r_xmm1 = _mm_add_epi32(r_xmm1, round_inv_row);
        r_xmm4 = _mm_shufflelo_epi16(r_xmm4, 0xd8);
        r_xmm0 = _mm_madd_epi16(r_xmm0, table04[3]);
        r_xmm5 = _mm_shuffle_epi32(r_xmm4, 0);
        r_xmm6 = _mm_shuffle_epi32(r_xmm4, 0xaa);
        r_xmm5 = _mm_madd_epi16(r_xmm5, table26[0]);
        r_xmm1 = _mm_add_epi32(r_xmm1, r_xmm2);
        r_xmm2 = r_xmm1;
        r_xmm7 = _mm_shuffle_epi32(r_xmm4, 0x55);
        r_xmm6 = _mm_madd_epi16(r_xmm6, table26[1]);
        r_xmm0 = _mm_add_epi32(r_xmm0, r_xmm3);
        r_xmm4 = _mm_shuffle_epi32(r_xmm4, 0xff);
        r_xmm2 = _mm_sub_epi32(r_xmm2, r_xmm0);
        r_xmm7 = _mm_madd_epi16(r_xmm7, table26[2]);
        r_xmm0 = _mm_add_epi32(r_xmm0, r_xmm1);
        r_xmm2 = _mm_srai_epi32(r_xmm2, 12);
        r_xmm5 = _mm_add_epi32(r_xmm5, round_inv_row);
        r_xmm4 = _mm_madd_epi16(r_xmm4, table26[3]);
        r_xmm5 = _mm_add_epi32(r_xmm5, r_xmm6);
        r_xmm6 = r_xmm5;
        r_xmm0 = _mm_srai_epi32(r_xmm0, 12);
        r_xmm2 = _mm_shuffle_epi32(r_xmm2, 0x1b);
        row0 = _mm_packs_epi32(r_xmm0, r_xmm2);
        r_xmm4 = _mm_add_epi32(r_xmm4, r_xmm7);
        r_xmm6 = _mm_sub_epi32(r_xmm6, r_xmm4);
        r_xmm4 = _mm_add_epi32(r_xmm4, r_xmm5);
        r_xmm6 = _mm_srai_epi32(r_xmm6, 12);
        r_xmm4 = _mm_srai_epi32(r_xmm4, 12);
        r_xmm6 = _mm_shuffle_epi32(r_xmm6, 0x1b);
        row2 = _mm_packs_epi32(r_xmm4, r_xmm6);

        //Row 5 and row 7
        r_xmm0 = v4;
        r_xmm4 = v6;

        r_xmm0 = _mm_shufflelo_epi16(r_xmm0, 0xd8);
        r_xmm1 = _mm_shuffle_epi32(r_xmm0, 0);
        r_xmm1 = _mm_madd_epi16(r_xmm1, table04[0]);
        r_xmm3 = _mm_shuffle_epi32(r_xmm0, 0x55);
        r_xmm0 = _mm_shufflehi_epi16(r_xmm0, 0xd8);
        r_xmm3 = _mm_madd_epi16(r_xmm3, table04[2]);
        r_xmm2 = _mm_shuffle_epi32(r_xmm0, 0xaa);
        r_xmm0 = _mm_shuffle_epi32(r_xmm0, 0xff);
        r_xmm2 = _mm_madd_epi16(r_xmm2, table04[1]);
        r_xmm4 = _mm_shufflehi_epi16(r_xmm4, 0xd8);
        r_xmm1 = _mm_add_epi32(r_xmm1, round_inv_row);
        r_xmm4 = _mm_shufflelo_epi16(r_xmm4, 0xd8);
        r_xmm0 = _mm_madd_epi16(r_xmm0, table04[3]);
        r_xmm5 = _mm_shuffle_epi32(r_xmm4, 0);
        r_xmm6 = _mm_shuffle_epi32(r_xmm4, 0xaa);
        r_xmm5 = _mm_madd_epi16(r_xmm5, table26[0]);
        r_xmm1 = _mm_add_epi32(r_xmm1, r_xmm2);
        r_xmm2 = r_xmm1;
        r_xmm7 = _mm_shuffle_epi32(r_xmm4, 0x55);
        r_xmm6 = _mm_madd_epi16(r_xmm6, table26[1]);
        r_xmm0 = _mm_add_epi32(r_xmm0, r_xmm3);
        r_xmm4 = _mm_shuffle_epi32(r_xmm4, 0xff);
        r_xmm2 = _mm_sub_epi32(r_xmm2, r_xmm0);
        r_xmm7 = _mm_madd_epi16(r_xmm7, table26[2]);
        r_xmm0 = _mm_add_epi32(r_xmm0, r_xmm1);
        r_xmm2 = _mm_srai_epi32(r_xmm2, 12);
        r_xmm5 = _mm_add_epi32(r_xmm5, round_inv_row);
        r_xmm4 = _mm_madd_epi16(r_xmm4, table26[3]);
        r_xmm5 = _mm_add_epi32(r_xmm5, r_xmm6);
        r_xmm6 = r_xmm5;
        r_xmm0 = _mm_srai_epi32(r_xmm0, 12);
        r_xmm2 = _mm_shuffle_epi32(r_xmm2, 0x1b);
        row4 = _mm_packs_epi32(r_xmm0, r_xmm2);
        r_xmm4 = _mm_add_epi32(r_xmm4, r_xmm7);
        r_xmm6 = _mm_sub_epi32(r_xmm6, r_xmm4);
        r_xmm4 = _mm_add_epi32(r_xmm4, r_xmm5);
        r_xmm6 = _mm_srai_epi32(r_xmm6, 12);
        r_xmm4 = _mm_srai_epi32(r_xmm4, 12);
        r_xmm6 = _mm_shuffle_epi32(r_xmm6, 0x1b);
        row6 = _mm_packs_epi32(r_xmm4, r_xmm6);

        const __m128i* table35 = reinterpret_cast<const __m128i*>(shortM128_tab_i_35);
        const __m128i* table17 = reinterpret_cast<const __m128i*>(shortM128_tab_i_17);

        //Row 4 and row 2
        r_xmm0 = v3;
        r_xmm4 = v1;

        r_xmm0 = _mm_shufflelo_epi16(r_xmm0, 0xd8);
        r_xmm1 = _mm_shuffle_epi32(r_xmm0, 0);
        r_xmm1 = _mm_madd_epi16(r_xmm1, table35[0]);
        r_xmm3 = _mm_shuffle_epi32(r_xmm0, 0x55);
        r_xmm0 = _mm_shufflehi_epi16(r_xmm0, 0xd8);
        r_xmm3 = _mm_madd_epi16(r_xmm3, table35[2]);
        r_xmm2 = _mm_shuffle_epi32(r_xmm0, 0xaa);
        r_xmm0 = _mm_shuffle_epi32(r_xmm0, 0xff);
        r_xmm2 = _mm_madd_epi16(r_xmm2, table35[1]);
        r_xmm4 = _mm_shufflehi_epi16(r_xmm4, 0xd8);
        r_xmm1 = _mm_add_epi32(r_xmm1, round_inv_row);
        r_xmm4 = _mm_shufflelo_epi16(r_xmm4, 0xd8);
        r_xmm0 = _mm_madd_epi16(r_xmm0, table35[3]);
        r_xmm5 = _mm_shuffle_epi32(r_xmm4, 0);
        r_xmm6 = _mm_shuffle_epi32(r_xmm4, 0xaa);
        r_xmm5 = _mm_madd_epi16(r_xmm5, table17[0]);
        r_xmm1 = _mm_add_epi32(r_xmm1, r_xmm2);
        r_xmm2 = r_xmm1;
        r_xmm7 = _mm_shuffle_epi32(r_xmm4, 0x55);
        r_xmm6 = _mm_madd_epi16(r_xmm6, table17[1]);
        r_xmm0 = _mm_add_epi32(r_xmm0, r_xmm3);
        r_xmm4 = _mm_shuffle_epi32(r_xmm4, 0xff);
        r_xmm2 = _mm_sub_epi32(r_xmm2, r_xmm0);
        r_xmm7 = _mm_madd_epi16(r_xmm7, table17[2]);
        r_xmm0 = _mm_add_epi32(r_xmm0, r_xmm1);
        r_xmm2 = _mm_srai_epi32(r_xmm2, 12);
        r_xmm5 = _mm_add_epi32(r_xmm5, round_inv_row);
        r_xmm4 = _mm_madd_epi16(r_xmm4, table17[3]);
        r_xmm5 = _mm_add_epi32(r_xmm5, r_xmm6);
        r_xmm6 = r_xmm5;
        r_xmm0 = _mm_srai_epi32(r_xmm0, 12);
        r_xmm2 = _mm_shuffle_epi32(r_xmm2, 0x1b);
        row3 = _mm_packs_epi32(r_xmm0, r_xmm2);
        r_xmm4 = _mm_add_epi32(r_xmm4, r_xmm7);
        r_xmm6 = _mm_sub_epi32(r_xmm6, r_xmm4);
        r_xmm4 = _mm_add_epi32(r_xmm4, r_xmm5);
        r_xmm6 = _mm_srai_epi32(r_xmm6, 12);
        r_xmm4 = _mm_srai_epi32(r_xmm4, 12);
        r_xmm6 = _mm_shuffle_epi32(r_xmm6, 0x1b);
        row1 = _mm_packs_epi32(r_xmm4, r_xmm6);

        //Row 6 and row 8
        r_xmm0 = v5;
        r_xmm4 = v7;

        r_xmm0 = _mm_shufflelo_epi16(r_xmm0, 0xd8);
        r_xmm1 = _mm_shuffle_epi32(r_xmm0, 0);
        r_xmm1 = _mm_madd_epi16(r_xmm1, table35[0]);
        r_xmm3 = _mm_shuffle_epi32(r_xmm0, 0x55);
        r_xmm0 = _mm_shufflehi_epi16(r_xmm0, 0xd8);
        r_xmm3 = _mm_madd_epi16(r_xmm3, table35[2]);
        r_xmm2 = _mm_shuffle_epi32(r_xmm0, 0xaa);
        r_xmm0 = _mm_shuffle_epi32(r_xmm0, 0xff);
        r_xmm2 = _mm_madd_epi16(r_xmm2, table35[1]);
        r_xmm4 = _mm_shufflehi_epi16(r_xmm4, 0xd8);
        r_xmm1 = _mm_add_epi32(r_xmm1, round_inv_row);
        r_xmm4 = _mm_shufflelo_epi16(r_xmm4, 0xd8);
        r_xmm0 = _mm_madd_epi16(r_xmm0, table35[3]);
        r_xmm5 = _mm_shuffle_epi32(r_xmm4, 0);
        r_xmm6 = _mm_shuffle_epi32(r_xmm4, 0xaa);
        r_xmm5 = _mm_madd_epi16(r_xmm5, table17[0]);
        r_xmm1 = _mm_add_epi32(r_xmm1, r_xmm2);
        r_xmm2 = r_xmm1;
        r_xmm7 = _mm_shuffle_epi32(r_xmm4, 0x55);
        r_xmm6 = _mm_madd_epi16(r_xmm6, table17[1]);
        r_xmm0 = _mm_add_epi32(r_xmm0, r_xmm3);
        r_xmm4 = _mm_shuffle_epi32(r_xmm4, 0xff);
        r_xmm2 = _mm_sub_epi32(r_xmm2, r_xmm0);
        r_xmm7 = _mm_madd_epi16(r_xmm7, table17[2]);
        r_xmm0 = _mm_add_epi32(r_xmm0, r_xmm1);
        r_xmm2 = _mm_srai_epi32(r_xmm2, 12);
        r_xmm5 = _mm_add_epi32(r_xmm5, round_inv_row);
        r_xmm4 = _mm_madd_epi16(r_xmm4, table17[3]);
        r_xmm5 = _mm_add_epi32(r_xmm5, r_xmm6);
        r_xmm6 = r_xmm5;
        r_xmm0 = _mm_srai_epi32(r_xmm0, 12);
        r_xmm2 = _mm_shuffle_epi32(r_xmm2, 0x1b);
        row5 = _mm_packs_epi32(r_xmm0, r_xmm2);
        r_xmm4 = _mm_add_epi32(r_xmm4, r_xmm7);
        r_xmm6 = _mm_sub_epi32(r_xmm6, r_xmm4);
        r_xmm4 = _mm_add_epi32(r_xmm4, r_xmm5);
        r_xmm6 = _mm_srai_epi32(r_xmm6, 12);
        r_xmm4 = _mm_srai_epi32(r_xmm4, 12);
        r_xmm6 = _mm_shuffle_epi32(r_xmm6, 0x1b);
        row7 = _mm_packs_epi32(r_xmm4, r_xmm6);

        r_xmm1 = _mm_shuffle_epi32(tg, 0xaa);
        r_xmm2 = row5;
        r_xmm3 = row3;
        r_xmm0 = _mm_mulhi_epi16(row5, r_xmm1);

        r_xmm1 = _mm_mulhi_epi16(r_xmm1, r_xmm3);
        r_xmm5 = _mm_shuffle_epi32(tg, 0x00);
        r_xmm6 = row7;
        r_xmm4 = _mm_mulhi_epi16(row7, r_xmm5);

        r_xmm0 = _mm_adds_epi16(r_xmm0, r_xmm2);
        r_xmm5 = _mm_mulhi_epi16(r_xmm5, row1);
        r_xmm1 = _mm_adds_epi16(r_xmm1, r_xmm3);
        r_xmm7 = row6;

        const __m128i one = _mm_set1_epi16(1);

        r_xmm0 = _mm_adds_epi16(r_xmm0, r_xmm3);
        r_xmm3 = _mm_shuffle_epi32(tg, 0x55);
        r_xmm2 = _mm_subs_epi16(r_xmm2, r_xmm1);
        r_xmm7 = _mm_mulhi_epi16(r_xmm7, r_xmm3);
        r_xmm1 = r_xmm0;
        r_xmm3 = _mm_mulhi_epi16(r_xmm3, row2);
        r_xmm5 = _mm_subs_epi16(r_xmm5, r_xmm6);
        r_xmm4 = _mm_adds_epi16(r_xmm4, row1);
        r_xmm0 = _mm_adds_epi16(r_xmm0, r_xmm4);
        r_xmm0 = _mm_adds_epi16(r_xmm0, one);
        r_xmm4 = _mm_subs_epi16(r_xmm4, r_xmm1);
        r_xmm6 = r_xmm5;
        r_xmm5 = _mm_subs_epi16(r_xmm5, r_xmm2);
        r_xmm5 = _mm_adds_epi16(r_xmm5, one);
        r_xmm6 = _mm_adds_epi16(r_xmm6, r_xmm2);

        //Intermediate results, needed later
        __m128i temp7 = r_xmm0;

        r_xmm1 = r_xmm4;
        r_xmm0 = _mm_shuffle_epi32(tg, 0xff);
        r_xmm4 = _mm_adds_epi16(r_xmm4, r_xmm5);
        r_xmm2 = _mm_mulhi_epi16(r_xmm0, r_xmm4);

        //Intermediate results, needed later
        __m128i temp3 = r_xmm6;

        r_xmm1 = _mm_subs_epi16(r_xmm1, r_xmm5);
        r_xmm7 = _mm_adds_epi16(r_xmm7, row2);
        r_xmm3 = _mm_subs_epi16(r_xmm3, row6);
        r_xmm6 = row0;
        r_xmm0 = _mm_mulhi_epi16(r_xmm0, r_xmm1);
        r_xmm5 = row4;
        r_xmm5 = _mm_adds_epi16(r_xmm5, r_xmm6);
        r_xmm6 = _mm_subs_epi16(r_xmm6, row4);
        r_xmm4 = _mm_adds_epi16(r_xmm4, r_xmm2);

        r_xmm4 = _mm_or_si128(r_xmm4, one);
        r_xmm0 = _mm_adds_epi16(r_xmm0, r_xmm1);
        r_xmm0 = _mm_or_si128(r_xmm0, one);

        const __m128i round_inv_col = _mm_set1_epi16(16);
        const __m128i round_inv_corr = _mm_sub_epi16(round_inv_col, one);

        r_xmm2 = r_xmm5;
        r_xmm5 = _mm_adds_epi16(r_xmm5, r_xmm7);
        r_xmm1 = r_xmm6;
        r_xmm5 = _mm_adds_epi16(r_xmm5, round_inv_col);
        r_xmm2 = _mm_subs_epi16(r_xmm2, r_xmm7);
        r_xmm7 = temp7;
        r_xmm6 = _mm_adds_epi16(r_xmm6, r_xmm3);
        r_xmm6 = _mm_adds_epi16(r_xmm6, round_inv_col);
        r_xmm7 = _mm_adds_epi16(r_xmm7, r_xmm5);
        r_xmm7 = _mm_srai_epi16(r_xmm7, 5);
        r_xmm1 = _mm_subs_epi16(r_xmm1, r_xmm3);
        r_xmm1 = _mm_adds_epi16(r_xmm1, round_inv_corr);
        r_xmm3 = r_xmm6;
        r_xmm2 = _mm_adds_epi16(r_xmm2, round_inv_corr);
        r_xmm6 = _mm_adds_epi16(r_xmm6, r_xmm4);

        __m128i r0 = r_xmm7;

        r_xmm6 = _mm_srai_epi16(r_xmm6, 5);
        r_xmm7 = r_xmm1;
        r_xmm1 = _mm_adds_epi16(r_xmm1, r_xmm0);

        __m128i r1 = r_xmm6;

        r_xmm1 = _mm_srai_epi16(r_xmm1, 5);
        r_xmm6 = temp3;
        r_xmm7 = _mm_subs_epi16(r_xmm7, r_xmm0);
        r_xmm7 = _mm_srai_epi16(r_xmm7, 5);

        __m128i r2 = r_xmm1;

        r_xmm5 = _mm_subs_epi16(r_xmm5, temp7); 
        r_xmm5 = _mm_srai_epi16(r_xmm5, 5);

        __m128i r7 = r_xmm5;

        r_xmm3 = _mm_subs_epi16(r_xmm3, r_xmm4);
        r_xmm6 = _mm_adds_epi16(r_xmm6, r_xmm2);
        r_xmm2 = _mm_subs_epi16(r_xmm2, temp3); 
        r_xmm6 = _mm_srai_epi16(r_xmm6, 5);
        r_xmm2 = _mm_srai_epi16(r_xmm2, 5);

        __m128i r3 = r_xmm6;

        r_xmm3 = _mm_srai_epi16(r_xmm3, 5);

        __m128i r4 = r_xmm2;
        __m128i r5 = r_xmm7;
        __m128i r6 = r_xmm3;

        const __m128i bias = _mm_set1_epi16(128);

        r0 = _mm_add_epi16(r0, bias);
        r1 = _mm_add_epi16(r1, bias);
        r2 = _mm_add_epi16(r2, bias);
        r3 = _mm_add_epi16(r3, bias);
        r4 = _mm_add_epi16(r4, bias);
        r5 = _mm_add_epi16(r5, bias);
        r6 = _mm_add_epi16(r6, bias);
        r7 = _mm_add_epi16(r7, bias);

        __m128i s0 = _mm_packus_epi16(r0, r1);
        __m128i s1 = _mm_packus_epi16(r2, r3);
        __m128i s2 = _mm_packus_epi16(r4, r5);
        __m128i s3 = _mm_packus_epi16(r6, r7);

        // store
        __m128i* d = reinterpret_cast<__m128i *>(dest);
        _mm_storeu_si128(d + 0, s0);
        _mm_storeu_si128(d + 1, s1);
        _mm_storeu_si128(d + 2, s2);
        _mm_storeu_si128(d + 3, s3);
    }

#endif // MANGO_ENABLE_SSE2

#if defined(MANGO_ENABLE_NEON)

    // ------------------------------------------------------------------------------------------------
    // NEON implementation
    // ------------------------------------------------------------------------------------------------

    // The original code is by Sean Barrett ; WE HAVE TAKEN LIBERTIES TO ADAPT IT TO OUR USE!!!
    // https://github.com/nothings/stb
    // [License]
    // Public Domain / MIT

    #define JPEG_FIXED(x)  ((int) (((x) * 4096 + 0.5)))

    static inline
    void dct_trn16(int16x8_t& x, int16x8_t& y)
    {
        int16x8x2_t t = vtrnq_s16(x, y);
        x = t.val[0];
        y = t.val[1];
    }

    static inline
    void dct_trn32(int16x8_t& x, int16x8_t& y)
    {
        int32x4x2_t t = vtrnq_s32(vreinterpretq_s32_s16(x), vreinterpretq_s32_s16(y));
        x = vreinterpretq_s16_s32(t.val[0]);
        y = vreinterpretq_s16_s32(t.val[1]);
    }

    static inline
    void dct_trn64(int16x8_t& x, int16x8_t& y)
    {
        int16x8_t x0 = x;
        int16x8_t y0 = y;
        x = vcombine_s16(vget_low_s16(x0), vget_low_s16(y0));
        y = vcombine_s16(vget_high_s16(x0), vget_high_s16(y0));
    }

    static inline
    void dct_trn8(uint8x8_t& x, uint8x8_t& y)
    {
        uint8x8x2_t t = vtrn_u8(x, y); 
        x = t.val[0]; 
        y = t.val[1];
    }

    static inline
    void dct_trn16(uint8x8_t& x, uint8x8_t& y)
    {
        uint16x4x2_t t = vtrn_u16(vreinterpret_u16_u8(x), vreinterpret_u16_u8(y)); 
        x = vreinterpret_u8_u16(t.val[0]); 
        y = vreinterpret_u8_u16(t.val[1]); 
    }

    static inline
    void dct_trn32(uint8x8_t& x, uint8x8_t& y)
    {
        uint32x2x2_t t = vtrn_u32(vreinterpret_u32_u8(x), vreinterpret_u32_u8(y)); 
        x = vreinterpret_u8_u32(t.val[0]); 
        y = vreinterpret_u8_u32(t.val[1]); 
    }

#define dct_long_mul(out, inq, coeff) \
    int32x4_t out##_l = vmull_s16(vget_low_s16(inq), coeff); \
    int32x4_t out##_h = vmull_s16(vget_high_s16(inq), coeff)

#define dct_long_mac(out, acc, inq, coeff) \
    int32x4_t out##_l = vmlal_s16(acc##_l, vget_low_s16(inq), coeff); \
    int32x4_t out##_h = vmlal_s16(acc##_h, vget_high_s16(inq), coeff)

#define dct_widen(out, inq) \
    int32x4_t out##_l = vshll_n_s16(vget_low_s16(inq), 12); \
    int32x4_t out##_h = vshll_n_s16(vget_high_s16(inq), 12)

// wide add
#define dct_wadd(out, a, b) \
    int32x4_t out##_l = vaddq_s32(a##_l, b##_l); \
    int32x4_t out##_h = vaddq_s32(a##_h, b##_h)

// wide sub
#define dct_wsub(out, a, b) \
    int32x4_t out##_l = vsubq_s32(a##_l, b##_l); \
    int32x4_t out##_h = vsubq_s32(a##_h, b##_h)

// butterfly a/b, then shift using "shiftop" by "s" and pack
#define dct_bfly32o(out0, out1, a, b, shiftop, s) \
    { \
        dct_wadd(sum, a, b); \
        dct_wsub(dif, a, b); \
        out0 = vcombine_s16(shiftop(sum_l, s), shiftop(sum_h, s)); \
        out1 = vcombine_s16(shiftop(dif_l, s), shiftop(dif_h, s)); \
    }

#define dct_pass(shiftop, shift) \
    { \
        /* even part */ \
        int16x8_t sum26 = vaddq_s16(row2, row6); \
        dct_long_mul(p1e, sum26, rot0_0); \
        dct_long_mac(t2e, p1e, row6, rot0_1); \
        dct_long_mac(t3e, p1e, row2, rot0_2); \
        int16x8_t sum04 = vaddq_s16(row0, row4); \
        int16x8_t dif04 = vsubq_s16(row0, row4); \
        dct_widen(t0e, sum04); \
        dct_widen(t1e, dif04); \
        dct_wadd(x0, t0e, t3e); \
        dct_wsub(x3, t0e, t3e); \
        dct_wadd(x1, t1e, t2e); \
        dct_wsub(x2, t1e, t2e); \
        /* odd part */ \
        int16x8_t sum15 = vaddq_s16(row1, row5); \
        int16x8_t sum17 = vaddq_s16(row1, row7); \
        int16x8_t sum35 = vaddq_s16(row3, row5); \
        int16x8_t sum37 = vaddq_s16(row3, row7); \
        int16x8_t sumodd = vaddq_s16(sum17, sum35); \
        dct_long_mul(p5o, sumodd, rot1_0); \
        dct_long_mac(p1o, p5o, sum17, rot1_1); \
        dct_long_mac(p2o, p5o, sum35, rot1_2); \
        dct_long_mul(p3o, sum37, rot2_0); \
        dct_long_mul(p4o, sum15, rot2_1); \
        dct_wadd(sump13o, p1o, p3o); \
        dct_wadd(sump24o, p2o, p4o); \
        dct_wadd(sump23o, p2o, p3o); \
        dct_wadd(sump14o, p1o, p4o); \
        dct_long_mac(x4, sump13o, row7, rot3_0); \
        dct_long_mac(x5, sump24o, row5, rot3_1); \
        dct_long_mac(x6, sump23o, row3, rot3_2); \
        dct_long_mac(x7, sump14o, row1, rot3_3); \
        dct_bfly32o(row0,row7, x0,x7,shiftop,shift); \
        dct_bfly32o(row1,row6, x1,x6,shiftop,shift); \
        dct_bfly32o(row2,row5, x2,x5,shiftop,shift); \
        dct_bfly32o(row3,row4, x3,x4,shiftop,shift); \
    }

void idct_neon(u8* out, const s16* data, const s16* qt)
{
    const int16x4_t rot0_0 = vdup_n_s16(JPEG_FIXED(0.5411961f));
    const int16x4_t rot0_1 = vdup_n_s16(JPEG_FIXED(-1.847759065f));
    const int16x4_t rot0_2 = vdup_n_s16(JPEG_FIXED( 0.765366865f));
    const int16x4_t rot1_0 = vdup_n_s16(JPEG_FIXED( 1.175875602f));
    const int16x4_t rot1_1 = vdup_n_s16(JPEG_FIXED(-0.899976223f));
    const int16x4_t rot1_2 = vdup_n_s16(JPEG_FIXED(-2.562915447f));
    const int16x4_t rot2_0 = vdup_n_s16(JPEG_FIXED(-1.961570560f));
    const int16x4_t rot2_1 = vdup_n_s16(JPEG_FIXED(-0.390180644f));
    const int16x4_t rot3_0 = vdup_n_s16(JPEG_FIXED( 0.298631336f));
    const int16x4_t rot3_1 = vdup_n_s16(JPEG_FIXED( 2.053119869f));
    const int16x4_t rot3_2 = vdup_n_s16(JPEG_FIXED( 3.072711026f));
    const int16x4_t rot3_3 = vdup_n_s16(JPEG_FIXED( 1.501321110f));

    // load
    int16x8_t row0 = vld1q_s16(data + 0 * 8);
    int16x8_t row1 = vld1q_s16(data + 1 * 8);
    int16x8_t row2 = vld1q_s16(data + 2 * 8);
    int16x8_t row3 = vld1q_s16(data + 3 * 8);
    int16x8_t row4 = vld1q_s16(data + 4 * 8);
    int16x8_t row5 = vld1q_s16(data + 5 * 8);
    int16x8_t row6 = vld1q_s16(data + 6 * 8);
    int16x8_t row7 = vld1q_s16(data + 7 * 8);

    int16x8_t qt0 = vld1q_s16(qt + 0 * 8);
    int16x8_t qt1 = vld1q_s16(qt + 1 * 8);
    int16x8_t qt2 = vld1q_s16(qt + 2 * 8);
    int16x8_t qt3 = vld1q_s16(qt + 3 * 8);
    int16x8_t qt4 = vld1q_s16(qt + 4 * 8);
    int16x8_t qt5 = vld1q_s16(qt + 5 * 8);
    int16x8_t qt6 = vld1q_s16(qt + 6 * 8);
    int16x8_t qt7 = vld1q_s16(qt + 7 * 8);

    row0 = vmulq_s16(row0, qt0);
    row1 = vmulq_s16(row1, qt1);
    row2 = vmulq_s16(row2, qt2);
    row3 = vmulq_s16(row3, qt3);
    row4 = vmulq_s16(row4, qt4);
    row5 = vmulq_s16(row5, qt5);
    row6 = vmulq_s16(row6, qt6);
    row7 = vmulq_s16(row7, qt7);

    // add DC bias
    row0 = vaddq_s16(row0, vsetq_lane_s16(1024, vdupq_n_s16(0), 0));

    // column pass
    dct_pass(vrshrn_n_s32, 10);

    // 16bit 8x8 transpose
    dct_trn16(row0, row1);
    dct_trn16(row2, row3);
    dct_trn16(row4, row5);
    dct_trn16(row6, row7);
    dct_trn32(row0, row2);
    dct_trn32(row1, row3);
    dct_trn32(row4, row6);
    dct_trn32(row5, row7);
    dct_trn64(row0, row4);
    dct_trn64(row1, row5);
    dct_trn64(row2, row6);
    dct_trn64(row3, row7);

    // row pass
    // vrshrn_n_s32 only supports shifts up to 16, we need
    // 17. so do a non-rounding shift of 16 first then follow
    // up with a rounding shift by 1.
    dct_pass(vshrn_n_s32, 16);

    // pack and round
    uint8x8_t p0 = vqrshrun_n_s16(row0, 1);
    uint8x8_t p1 = vqrshrun_n_s16(row1, 1);
    uint8x8_t p2 = vqrshrun_n_s16(row2, 1);
    uint8x8_t p3 = vqrshrun_n_s16(row3, 1);
    uint8x8_t p4 = vqrshrun_n_s16(row4, 1);
    uint8x8_t p5 = vqrshrun_n_s16(row5, 1);
    uint8x8_t p6 = vqrshrun_n_s16(row6, 1);
    uint8x8_t p7 = vqrshrun_n_s16(row7, 1);

    // 8x8 8-bit transpose
    dct_trn8(p0, p1);
    dct_trn8(p2, p3);
    dct_trn8(p4, p5);
    dct_trn8(p6, p7);
    dct_trn16(p0, p2);
    dct_trn16(p1, p3);
    dct_trn16(p4, p6);
    dct_trn16(p5, p7);
    dct_trn32(p0, p4);
    dct_trn32(p1, p5);
    dct_trn32(p2, p6);
    dct_trn32(p3, p7);

    // store
    vst1_u8(out, p0); out += 8;
    vst1_u8(out, p1); out += 8;
    vst1_u8(out, p2); out += 8;
    vst1_u8(out, p3); out += 8;
    vst1_u8(out, p4); out += 8;
    vst1_u8(out, p5); out += 8;
    vst1_u8(out, p6); out += 8;
    vst1_u8(out, p7);
}

#undef dct_long_mul
#undef dct_long_mac
#undef dct_widen
#undef dct_wadd
#undef dct_wsub
#undef dct_bfly32o
#undef dct_pass

#endif // MANGO_ENABLE_NEON

} // namespace mango::jpeg
