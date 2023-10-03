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
    alignas(16) static
    s16 shortM128_tab_i_04 [] =
    {
        16384, 21407, 16384, 8867, 16384, -8867, 16384, -21407,
        16384, 8867, -16384, -21407, -16384, 21407, 16384, -8867,
        22725, 19266, 19266, -4520, 12873, -22725, 4520, -12873,
        12873, 4520, -22725, -12873, 4520, 19266, 19266, -22725
    };

    // Table for rows 1,7 - constants are multiplied on cos_1_16
    alignas(16) static
    s16 shortM128_tab_i_17 [] =
    {
        22725, 29692, 22725, 12299, 22725, -12299, 22725, -29692,
        22725, 12299, -22725, -29692, -22725, 29692, 22725, -12299,
        31521, 26722, 26722, -6270, 17855, -31521, 6270, -17855,
        17855, 6270, -31521, -17855, 6270, 26722, 26722, -31521
    };

    // Table for rows 2,6 - constants are multiplied on cos_2_16
    alignas(16) static
    s16 shortM128_tab_i_26 [] =
    {
        21407, 27969, 21407, 11585, 21407, -11585, 21407, -27969,
        21407, 11585, -21407, -27969, -21407, 27969, 21407, -11585,
        29692, 25172, 25172, -5906, 16819, -29692, 5906, -16819,
        16819, 5906, -29692, -16819, 5906, 25172, 25172, -29692
    };

    // Table for rows 3,5 - constants are multiplied on cos_3_16
    alignas(16) static
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

        r_xmm7 = _mm_subs_epi16(r_xmm7, r_xmm0);
        r_xmm5 = _mm_subs_epi16(r_xmm5, temp7);
        r_xmm3 = _mm_subs_epi16(r_xmm3, r_xmm4);
        r_xmm6 = _mm_adds_epi16(r_xmm2, temp3);
        r_xmm2 = _mm_subs_epi16(r_xmm2, temp3);

        __m128i r2 = _mm_srai_epi16(r_xmm1, 5);
        __m128i r3 = _mm_srai_epi16(r_xmm6, 5);
        __m128i r4 = _mm_srai_epi16(r_xmm2, 5);
        __m128i r5 = _mm_srai_epi16(r_xmm7, 5);
        __m128i r6 = _mm_srai_epi16(r_xmm3, 5);
        __m128i r7 = _mm_srai_epi16(r_xmm5, 5);

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

    // Table for rows 0,4 - constants are multiplied on cos_4_16
    alignas(16) static
    s16 shortM128_tab_i_04 [] =
    {
        16384, 21407, 16384, 8867, 16384, -8867, 16384, -21407,
        16384, 8867, -16384, -21407, -16384, 21407, 16384, -8867,
        22725, 19266, 19266, -4520, 12873, -22725, 4520, -12873,
        12873, 4520, -22725, -12873, 4520, 19266, 19266, -22725
    };

    // Table for rows 1,7 - constants are multiplied on cos_1_16
    alignas(16) static
    s16 shortM128_tab_i_17 [] =
    {
        22725, 29692, 22725, 12299, 22725, -12299, 22725, -29692,
        22725, 12299, -22725, -29692, -22725, 29692, 22725, -12299,
        31521, 26722, 26722, -6270, 17855, -31521, 6270, -17855,
        17855, 6270, -31521, -17855, 6270, 26722, 26722, -31521
    };

    // Table for rows 2,6 - constants are multiplied on cos_2_16
    alignas(16) static
    s16 shortM128_tab_i_26 [] =
    {
        21407, 27969, 21407, 11585, 21407, -11585, 21407, -27969,
        21407, 11585, -21407, -27969, -21407, 27969, 21407, -11585,
        29692, 25172, 25172, -5906, 16819, -29692, 5906, -16819,
        16819, 5906, -29692, -16819, 5906, 25172, 25172, -29692
    };

    // Table for rows 3,5 - constants are multiplied on cos_3_16
    alignas(16) static
    s16 shortM128_tab_i_35 [] =
    {
        19266, 25172, 19266, 10426, 19266, -10426, 19266, -25172,
        19266, 10426, -19266, -25172, -19266, 25172, 19266, -10426,
        26722, 22654, 22654, -5315, 15137, -26722, 5315, -15137,
        15137, 5315, -26722, -15137, 5315, 22654, 22654, -26722
    };

#if defined(__aarch64__)

    template <unsigned int Index>
    static inline
    int16x8_t splat_epi32(int16x8_t a)
    {
        return vreinterpretq_s16_u32(vdupq_laneq_u32(vreinterpretq_u32_s16(a), Index));
    }

    static inline
    int32x4_t madd_epi16(int16x8_t a, int16x8_t b)
    {
        int32x4_t low = vmull_s16(vget_low_s16(a), vget_low_s16(b));
        int32x4_t high = vmull_high_s16(a, b);
        return vpaddq_s32(low, high);
    }

#else

    template <unsigned int Index>
    static inline
    int16x8_t splat_epi32(int16x8_t a)
    {
        u32 s = vgetq_lane_u32(vreinterpretq_u32_s16(a), Index);
        return vreinterpretq_s16_u32(vdupq_n_u32(s));
    }

    static inline
    int32x4_t madd_epi16(int16x8_t a, int16x8_t b)
    {
        int32x4_t low = vmull_s16(vget_low_s16(a), vget_low_s16(b));
        int32x4_t high = vmull_s16(vget_high_s16(a), vget_high_s16(b));
        int32x2_t low_sum = vpadd_s32(vget_low_s32(low), vget_high_s32(low));
        int32x2_t high_sum = vpadd_s32(vget_low_s32(high), vget_high_s32(high));
        return vcombine_s32(low_sum, high_sum);
    }

#endif

    static inline
    int16x8_t mulhi(int16x8_t a, s16 b)
    {
        return vqrdmulhq_n_s16(a, b / 2);
    }

    static inline
    int16x8_t packs_epi32(int32x4_t a, int32x4_t b)
    {
        return vcombine_s16(vqmovn_s32(a), vqmovn_s32(b));
    }

    static inline
    int32x4_t rev_epi32(int32x4_t a)
    {
        a = vrev64q_s32(a);
        a = vcombine_s32(vget_high_s32(a), vget_low_s32(a));
        return a;
    }

    static inline
    int16x8_t shufflelo(int16x8_t a)
    {
        s16 v1 = vgetq_lane_s16(a, 1);
        s16 v2 = vgetq_lane_s16(a, 2);
        a = vsetq_lane_s16(v1, a, 2);
        a = vsetq_lane_s16(v2, a, 1);
        return a;
    }

    static inline
    int16x8_t shufflehi(int16x8_t a)
    {
        s16 v5 = vgetq_lane_s16(a, 5);
        s16 v6 = vgetq_lane_s16(a, 6);
        a = vsetq_lane_s16(v5, a, 6);
        a = vsetq_lane_s16(v6, a, 5);
        return a;
    }

    void idct_neon(u8* dest, const s16* src, const s16* qt)
    {
        // load dct coefficients
        int16x8_t row0 = vld1q_s16(src + 8 * 0);
        int16x8_t row1 = vld1q_s16(src + 8 * 1);
        int16x8_t row2 = vld1q_s16(src + 8 * 2);
        int16x8_t row3 = vld1q_s16(src + 8 * 3);
        int16x8_t row4 = vld1q_s16(src + 8 * 4);
        int16x8_t row5 = vld1q_s16(src + 8 * 5);
        int16x8_t row6 = vld1q_s16(src + 8 * 6);
        int16x8_t row7 = vld1q_s16(src + 8 * 7);

        // load quantization factors
        int16x8_t qt0 = vld1q_s16(qt + 8 * 0);
        int16x8_t qt1 = vld1q_s16(qt + 8 * 1);
        int16x8_t qt2 = vld1q_s16(qt + 8 * 2);
        int16x8_t qt3 = vld1q_s16(qt + 8 * 3);
        int16x8_t qt4 = vld1q_s16(qt + 8 * 4);
        int16x8_t qt5 = vld1q_s16(qt + 8 * 5);
        int16x8_t qt6 = vld1q_s16(qt + 8 * 6);
        int16x8_t qt7 = vld1q_s16(qt + 8 * 7);

        // dequantize
        int16x8_t v0 = vmulq_s16(row0, qt0);
        int16x8_t v1 = vmulq_s16(row1, qt1);
        int16x8_t v2 = vmulq_s16(row2, qt2);
        int16x8_t v3 = vmulq_s16(row3, qt3);
        int16x8_t v4 = vmulq_s16(row4, qt4);
        int16x8_t v5 = vmulq_s16(row5, qt5);
        int16x8_t v6 = vmulq_s16(row6, qt6);
        int16x8_t v7 = vmulq_s16(row7, qt7);

        int16x8_t r16_xmm0, r16_xmm1, r16_xmm2, r16_xmm3, r16_xmm4, r16_xmm5, r16_xmm6, r16_xmm7;
        int32x4_t r32_xmm0, r32_xmm1, r32_xmm2, r32_xmm3, r32_xmm4, r32_xmm5, r32_xmm6, r32_xmm7;

        // row1 and row3
        r16_xmm0 = v0;
        r16_xmm4 = v2;

        const int16x8_t* table04 = reinterpret_cast<const int16x8_t*>(shortM128_tab_i_04);
        const int16x8_t* table26 = reinterpret_cast<const int16x8_t*>(shortM128_tab_i_26);

        r16_xmm0 = shufflelo(r16_xmm0);
        r16_xmm1 = splat_epi32<0>(r16_xmm0);
        r32_xmm1 = madd_epi16(r16_xmm1, table04[0]);
        r16_xmm3 = splat_epi32<1>(r16_xmm0);
        r16_xmm0 = shufflehi(r16_xmm0);
        r32_xmm3 = madd_epi16(r16_xmm3, table04[2]);
        r16_xmm2 = splat_epi32<2>(r16_xmm0);
        r16_xmm0 = splat_epi32<3>(r16_xmm0);
        r32_xmm2 = madd_epi16(r16_xmm2, table04[1]);
        r16_xmm4 = shufflehi(r16_xmm4);

        const int32x4_t round_inv_row = vdupq_n_s32(2048);

        r32_xmm1 = vaddq_s32(r32_xmm1, round_inv_row);
        r16_xmm4 = shufflelo(r16_xmm4);
        r32_xmm0 = madd_epi16(r16_xmm0, table04[3]);
        r16_xmm5 = splat_epi32<0>(r16_xmm4);
        r16_xmm6 = splat_epi32<2>(r16_xmm4);
        r32_xmm5 = madd_epi16(r16_xmm5, table26[0]);
        r32_xmm1 = vaddq_s32(r32_xmm1, r32_xmm2);
        r32_xmm2 = r32_xmm1;
        r16_xmm7 = splat_epi32<1>(r16_xmm4);
        r32_xmm6 = madd_epi16(r16_xmm6, table26[1]);
        r32_xmm0 = vaddq_s32(r32_xmm0, r32_xmm3);
        r16_xmm4 = splat_epi32<3>(r16_xmm4);
        r32_xmm2 = vsubq_s32(r32_xmm2, r32_xmm0);
        r32_xmm7 = madd_epi16(r16_xmm7, table26[2]);
        r32_xmm0 = vaddq_s32(r32_xmm0, r32_xmm1);
        r32_xmm2 = vshrq_n_s32(r32_xmm2, 12);
        r32_xmm5 = vaddq_s32(r32_xmm5, round_inv_row);
        r32_xmm4 = madd_epi16(r16_xmm4, table26[3]);
        r32_xmm5 = vaddq_s32(r32_xmm5, r32_xmm6);
        r32_xmm6 = r32_xmm5;
        r32_xmm0 = vshrq_n_s32(r32_xmm0, 12);
        r32_xmm2 = rev_epi32(r32_xmm2);
        row0 = packs_epi32(r32_xmm0, r32_xmm2);
        r32_xmm4 = vaddq_s32(r32_xmm4, r32_xmm7);
        r32_xmm6 = vsubq_s32(r32_xmm6, r32_xmm4);
        r32_xmm4 = vaddq_s32(r32_xmm4, r32_xmm5);
        r32_xmm6 = vshrq_n_s32(r32_xmm6, 12);
        r32_xmm4 = vshrq_n_s32(r32_xmm4, 12);
        r32_xmm6 = rev_epi32(r32_xmm6);
        row2 = packs_epi32(r32_xmm4, r32_xmm6);

        // row5 and row7
        r16_xmm0 = v4;
        r16_xmm4 = v6;

        r16_xmm0 = shufflelo(r16_xmm0);
        r16_xmm1 = splat_epi32<0>(r16_xmm0);
        r32_xmm1 = madd_epi16(r16_xmm1, table04[0]);
        r16_xmm3 = splat_epi32<1>(r16_xmm0);
        r16_xmm0 = shufflehi(r16_xmm0);
        r32_xmm3 = madd_epi16(r16_xmm3, table04[2]);
        r16_xmm2 = splat_epi32<2>(r16_xmm0);
        r16_xmm0 = splat_epi32<3>(r16_xmm0);
        r32_xmm2 = madd_epi16(r16_xmm2, table04[1]);
        r16_xmm4 = shufflehi(r16_xmm4);
        r32_xmm1 = vaddq_s32(r32_xmm1, round_inv_row);
        r16_xmm4 = shufflelo(r16_xmm4);
        r32_xmm0 = madd_epi16(r16_xmm0, table04[3]);
        r16_xmm5 = splat_epi32<0>(r16_xmm4);
        r16_xmm6 = splat_epi32<2>(r16_xmm4);
        r32_xmm5 = madd_epi16(r16_xmm5, table26[0]);
        r32_xmm1 = vaddq_s32(r32_xmm1, r32_xmm2);
        r32_xmm2 = r32_xmm1;
        r16_xmm7 = splat_epi32<1>(r16_xmm4);
        r32_xmm6 = madd_epi16(r16_xmm6, table26[1]);
        r32_xmm0 = vaddq_s32(r32_xmm0, r32_xmm3);
        r16_xmm4 = splat_epi32<3>(r16_xmm4);
        r32_xmm2 = vsubq_s32(r32_xmm2, r32_xmm0);
        r32_xmm7 = madd_epi16(r16_xmm7, table26[2]);
        r32_xmm0 = vaddq_s32(r32_xmm0, r32_xmm1);
        r32_xmm2 = vshrq_n_s32(r32_xmm2, 12);
        r32_xmm5 = vaddq_s32(r32_xmm5, round_inv_row);
        r32_xmm4 = madd_epi16(r16_xmm4, table26[3]);
        r32_xmm5 = vaddq_s32(r32_xmm5, r32_xmm6);
        r32_xmm6 = r32_xmm5;
        r32_xmm0 = vshrq_n_s32(r32_xmm0, 12);
        r32_xmm2 = rev_epi32(r32_xmm2);
        row4 = packs_epi32(r32_xmm0, r32_xmm2);
        r32_xmm4 = vaddq_s32(r32_xmm4, r32_xmm7);
        r32_xmm6 = vsubq_s32(r32_xmm6, r32_xmm4);
        r32_xmm4 = vaddq_s32(r32_xmm4, r32_xmm5);
        r32_xmm6 = vshrq_n_s32(r32_xmm6, 12);
        r32_xmm4 = vshrq_n_s32(r32_xmm4, 12);
        r32_xmm6 = rev_epi32(r32_xmm6);
        row6 = packs_epi32(r32_xmm4, r32_xmm6);

        // row4 and row2
        r16_xmm0 = v3;
        r16_xmm4 = v1;

        const int16x8_t* table35 = reinterpret_cast<const int16x8_t*>(shortM128_tab_i_35);
        const int16x8_t* table17 = reinterpret_cast<const int16x8_t*>(shortM128_tab_i_17);

        r16_xmm0 = shufflelo(r16_xmm0);
        r16_xmm1 = splat_epi32<0>(r16_xmm0);
        r32_xmm1 = madd_epi16(r16_xmm1, table35[0]);
        r16_xmm3 = splat_epi32<1>(r16_xmm0);
        r16_xmm0 = shufflehi(r16_xmm0);
        r32_xmm3 = madd_epi16(r16_xmm3, table35[2]);
        r16_xmm2 = splat_epi32<2>(r16_xmm0);
        r16_xmm0 = splat_epi32<3>(r16_xmm0);
        r32_xmm2 = madd_epi16(r16_xmm2, table35[1]);
        r16_xmm4 = shufflehi(r16_xmm4);
        r32_xmm1 = vaddq_s32(r32_xmm1, round_inv_row);
        r16_xmm4 = shufflelo(r16_xmm4);
        r32_xmm0 = madd_epi16(r16_xmm0, table35[3]);
        r16_xmm5 = splat_epi32<0>(r16_xmm4);
        r16_xmm6 = splat_epi32<2>(r16_xmm4);
        r32_xmm5 = madd_epi16(r16_xmm5, table17[0]);
        r32_xmm1 = vaddq_s32(r32_xmm1, r32_xmm2);
        r32_xmm2 = r32_xmm1;
        r16_xmm7 = splat_epi32<1>(r16_xmm4);
        r32_xmm6 = madd_epi16(r16_xmm6, table17[1]);
        r32_xmm0 = vaddq_s32(r32_xmm0, r32_xmm3);
        r16_xmm4 = splat_epi32<3>(r16_xmm4);
        r32_xmm2 = vsubq_s32(r32_xmm2, r32_xmm0);
        r32_xmm7 = madd_epi16(r16_xmm7, table17[2]);
        r32_xmm0 = vaddq_s32(r32_xmm0, r32_xmm1);
        r32_xmm2 = vshrq_n_s32(r32_xmm2, 12);
        r32_xmm5 = vaddq_s32(r32_xmm5, round_inv_row);
        r32_xmm4 = madd_epi16(r16_xmm4, table17[3]);
        r32_xmm5 = vaddq_s32(r32_xmm5, r32_xmm6);
        r32_xmm6 = r32_xmm5;
        r32_xmm0 = vshrq_n_s32(r32_xmm0, 12);
        r32_xmm2 = rev_epi32(r32_xmm2);
        row3 = packs_epi32(r32_xmm0, r32_xmm2);
        r32_xmm4 = vaddq_s32(r32_xmm4, r32_xmm7);
        r32_xmm6 = vsubq_s32(r32_xmm6, r32_xmm4);
        r32_xmm4 = vaddq_s32(r32_xmm4, r32_xmm5);
        r32_xmm6 = vshrq_n_s32(r32_xmm6, 12);
        r32_xmm4 = vshrq_n_s32(r32_xmm4, 12);
        r32_xmm6 = rev_epi32(r32_xmm6);
        row1 = packs_epi32(r32_xmm4, r32_xmm6);

        // row6 and row8
        r16_xmm0 = v5;
        r16_xmm4 = v7;

        r16_xmm0 = shufflelo(r16_xmm0);
        r16_xmm1 = splat_epi32<0>(r16_xmm0);
        r32_xmm1 = madd_epi16(r16_xmm1, table35[0]);
        r16_xmm3 = splat_epi32<1>(r16_xmm0);
        r16_xmm0 = shufflehi(r16_xmm0);
        r32_xmm3 = madd_epi16(r16_xmm3, table35[2]);
        r16_xmm2 = splat_epi32<2>(r16_xmm0);
        r16_xmm0 = splat_epi32<3>(r16_xmm0);
        r32_xmm2 = madd_epi16(r16_xmm2, table35[1]);
        r16_xmm4 = shufflehi(r16_xmm4);
        r32_xmm1 = vaddq_s32(r32_xmm1, round_inv_row);
        r16_xmm4 = shufflelo(r16_xmm4);
        r32_xmm0 = madd_epi16(r16_xmm0, table35[3]);
        r16_xmm5 = splat_epi32<0>(r16_xmm4);
        r16_xmm6 = splat_epi32<2>(r16_xmm4);
        r32_xmm5 = madd_epi16(r16_xmm5, table17[0]);
        r32_xmm1 = vaddq_s32(r32_xmm1, r32_xmm2);
        r32_xmm2 = r32_xmm1;
        r16_xmm7 = splat_epi32<1>(r16_xmm4);
        r32_xmm6 = madd_epi16(r16_xmm6, table17[1]);
        r32_xmm0 = vaddq_s32(r32_xmm0, r32_xmm3);
        r16_xmm4 = splat_epi32<3>(r16_xmm4);
        r32_xmm2 = vsubq_s32(r32_xmm2, r32_xmm0);
        r32_xmm7 = madd_epi16(r16_xmm7, table17[2]);
        r32_xmm0 = vaddq_s32(r32_xmm0, r32_xmm1);
        r32_xmm2 = vshrq_n_s32(r32_xmm2, 12);
        r32_xmm5 = vaddq_s32(r32_xmm5, round_inv_row);
        r32_xmm4 = madd_epi16(r16_xmm4, table17[3]);
        r32_xmm5 = vaddq_s32(r32_xmm5, r32_xmm6);
        r32_xmm6 = r32_xmm5;
        r32_xmm0 = vshrq_n_s32(r32_xmm0, 12);
        r32_xmm2 = rev_epi32(r32_xmm2);
        row5 = packs_epi32(r32_xmm0, r32_xmm2);
        r32_xmm4 = vaddq_s32(r32_xmm4, r32_xmm7);
        r32_xmm6 = vsubq_s32(r32_xmm6, r32_xmm4);
        r32_xmm4 = vaddq_s32(r32_xmm4, r32_xmm5);
        r32_xmm6 = vshrq_n_s32(r32_xmm6, 12);
        r32_xmm4 = vshrq_n_s32(r32_xmm4, 12);
        r32_xmm6 = rev_epi32(r32_xmm6);
        row7 = packs_epi32(r32_xmm4, r32_xmm6);

        r16_xmm2 = row5;
        r16_xmm3 = row3;
        r16_xmm0 = mulhi(row5, -21746);

        r16_xmm1 = mulhi(r16_xmm3, -21746);
        r16_xmm6 = row7;
        r16_xmm4 = mulhi(row7, 13036);

        r16_xmm0 = vqaddq_s16(r16_xmm0, r16_xmm2);
        r16_xmm5 = mulhi(row1, 13036);
        r16_xmm1 = vqaddq_s16(r16_xmm1, r16_xmm3);
        r16_xmm7 = row6;

        const int16x8_t one = vdupq_n_s16(1);

        r16_xmm0 = vqaddq_s16(r16_xmm0, r16_xmm3);
        r16_xmm2 = vqsubq_s16(r16_xmm2, r16_xmm1);
        r16_xmm7 = mulhi(r16_xmm7, 27146);
        r16_xmm1 = r16_xmm0;
        r16_xmm3 = mulhi(row2, 27146);
        r16_xmm5 = vqsubq_s16(r16_xmm5, r16_xmm6);
        r16_xmm4 = vqaddq_s16(r16_xmm4, row1);
        r16_xmm0 = vqaddq_s16(r16_xmm0, r16_xmm4);
        r16_xmm0 = vqaddq_s16(r16_xmm0, one);
        r16_xmm4 = vqsubq_s16(r16_xmm4, r16_xmm1);
        r16_xmm6 = r16_xmm5;
        r16_xmm5 = vqsubq_s16(r16_xmm5, r16_xmm2);
        r16_xmm5 = vqaddq_s16(r16_xmm5, one);
        r16_xmm6 = vqaddq_s16(r16_xmm6, r16_xmm2);

        int16x8_t temp7 = r16_xmm0;

        r16_xmm1 = r16_xmm4;
        r16_xmm4 = vqaddq_s16(r16_xmm4, r16_xmm5);
        r16_xmm2 = mulhi(r16_xmm4, -19195);

        int16x8_t temp3 = r16_xmm6;

        r16_xmm1 = vqsubq_s16(r16_xmm1, r16_xmm5);
        r16_xmm7 = vqaddq_s16(r16_xmm7, row2);
        r16_xmm3 = vqsubq_s16(r16_xmm3, row6);
        r16_xmm6 = row0;
        r16_xmm0 = mulhi(r16_xmm1, -19195);
        r16_xmm5 = row4;
        r16_xmm5 = vqaddq_s16(r16_xmm5, r16_xmm6);
        r16_xmm6 = vqsubq_s16(r16_xmm6, row4);
        r16_xmm4 = vqaddq_s16(r16_xmm4, r16_xmm2);

        r16_xmm4 = vorrq_s16(r16_xmm4, one);
        r16_xmm0 = vqaddq_s16(r16_xmm0, r16_xmm1);
        r16_xmm0 = vorrq_s16(r16_xmm0, one);

        const int16x8_t round_inv_col = vdupq_n_s16(16);
        const int16x8_t round_inv_corr = vqsubq_s16(round_inv_col, one);

        r16_xmm2 = r16_xmm5;
        r16_xmm5 = vqaddq_s16(r16_xmm5, r16_xmm7);
        r16_xmm1 = r16_xmm6;
        r16_xmm5 = vqaddq_s16(r16_xmm5, round_inv_col);
        r16_xmm2 = vqsubq_s16(r16_xmm2, r16_xmm7);
        r16_xmm7 = temp7;
        r16_xmm6 = vqaddq_s16(r16_xmm6, r16_xmm3);
        r16_xmm6 = vqaddq_s16(r16_xmm6, round_inv_col);
        r16_xmm7 = vqaddq_s16(r16_xmm7, r16_xmm5);
        r16_xmm7 = vshrq_n_s16(r16_xmm7, 5);
        r16_xmm1 = vqsubq_s16(r16_xmm1, r16_xmm3);
        r16_xmm1 = vqaddq_s16(r16_xmm1, round_inv_corr);
        r16_xmm3 = r16_xmm6;
        r16_xmm2 = vqaddq_s16(r16_xmm2, round_inv_corr);
        r16_xmm6 = vqaddq_s16(r16_xmm6, r16_xmm4);

        int16x8_t r0 = r16_xmm7;

        r16_xmm7 = r16_xmm1;
        r16_xmm1 = vqaddq_s16(r16_xmm1, r16_xmm0);

        int16x8_t r1 = vshrq_n_s16(r16_xmm6, 5);

        r16_xmm6 = vqaddq_s16(r16_xmm2, temp3);
        r16_xmm2 = vqsubq_s16(r16_xmm2, temp3);
        r16_xmm7 = vqsubq_s16(r16_xmm7, r16_xmm0);
        r16_xmm3 = vqsubq_s16(r16_xmm3, r16_xmm4);
        r16_xmm5 = vqsubq_s16(r16_xmm5, temp7);

        int16x8_t r2 = vshrq_n_s16(r16_xmm1, 5);
        int16x8_t r3 = vshrq_n_s16(r16_xmm6, 5);
        int16x8_t r4 = vshrq_n_s16(r16_xmm2, 5);
        int16x8_t r5 = vshrq_n_s16(r16_xmm7, 5);
        int16x8_t r6 = vshrq_n_s16(r16_xmm3, 5);
        int16x8_t r7 = vshrq_n_s16(r16_xmm5, 5);

        const int16x8_t bias = vdupq_n_s16(128);
        r0 = vaddq_s16(r0, bias);
        r1 = vaddq_s16(r1, bias);
        r2 = vaddq_s16(r2, bias);
        r3 = vaddq_s16(r3, bias);
        r4 = vaddq_s16(r4, bias);
        r5 = vaddq_s16(r5, bias);
        r6 = vaddq_s16(r6, bias);
        r7 = vaddq_s16(r7, bias);

        uint8x16_t s0 = vcombine_u8(vqmovun_s16(r0), vqmovun_s16(r1));
        uint8x16_t s1 = vcombine_u8(vqmovun_s16(r2), vqmovun_s16(r3));
        uint8x16_t s2 = vcombine_u8(vqmovun_s16(r4), vqmovun_s16(r5));
        uint8x16_t s3 = vcombine_u8(vqmovun_s16(r6), vqmovun_s16(r7));

        // store
        vst1q_u8(dest + 16 * 0, s0);
        vst1q_u8(dest + 16 * 1, s1);
        vst1q_u8(dest + 16 * 2, s2);
        vst1q_u8(dest + 16 * 3, s3);
    }

#endif // MANGO_ENABLE_NEON

} // namespace mango::jpeg
