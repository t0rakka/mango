/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Compressed Texture Utilities.
 *//*--------------------------------------------------------------------*/
/*
    The original source code has been modified for integration.
*/

#include "../../../include/mango/core/configure.hpp"
#include "../../../include/mango/core/bits.hpp"
#include "../../../include/mango/core/endian.hpp"
#include "../../../include/mango/math/vector.hpp"
#include "../../../include/mango/image/compression.hpp"

#define ETC_ENABLE_SIMD

namespace
{
    using namespace mango;
    using namespace mango::math;

    inline u32 getBits (u64 src, int offset, int count)
    {
        return u32(u64_extract_bits(src, offset, count));
    }

    inline u8 extend5Delta3To8 (int base5, int delta3)
    {
        const int t = base5 + s32_extend(delta3, 3);
        return u32_extend(t, 5, 8);
    }

    inline s16 extend11To16WithSign (s16 src)
    {
        if (src < 0)
            return -s16(u16_extend(-src, 11, 16));
        else
            return s16(u16_extend(src, 11, 16));
    }

    inline bool inRange(int value, int vmin, int vmax)
    {
        return value >= vmin && value <= vmax;
    }

    void decompress_block_etc1(u8* output, size_t stride, const u64 src)
    {
        const int  flipBit  = getBits(src, 32, 1);
        const int  diffBit  = getBits(src, 33, 1);
        const u32  table[2] = { getBits(src, 37, 3), getBits(src, 34, 3) };
        u32 baseR[2];
        u32 baseG[2];
        u32 baseB[2];

        if (!diffBit)
        {
            // Individual mode.
            baseR[0] = u32_extend(getBits(src, 60, 4), 4, 8);
            baseR[1] = u32_extend(getBits(src, 56, 4), 4, 8);
            baseG[0] = u32_extend(getBits(src, 52, 4), 4, 8);
            baseG[1] = u32_extend(getBits(src, 48, 4), 4, 8);
            baseB[0] = u32_extend(getBits(src, 44, 4), 4, 8);
            baseB[1] = u32_extend(getBits(src, 40, 4), 4, 8);
        }
        else
        {
            // Differential mode.
            int bR = getBits(src, 59, 5);
            int dR = getBits(src, 56, 3);
            int bG = getBits(src, 51, 5);
            int dG = getBits(src, 48, 3);
            int bB = getBits(src, 43, 5);
            int dB = getBits(src, 40, 3);

            baseR[0] = u32_extend(bR, 5, 8);
            baseG[0] = u32_extend(bG, 5, 8);
            baseB[0] = u32_extend(bB, 5, 8);
            baseR[1] = extend5Delta3To8(bR, dR);
            baseG[1] = extend5Delta3To8(bG, dG);
            baseB[1] = extend5Delta3To8(bB, dB);
        }

        static const u8 modifierTable[2][8] =
        {
            { 2, 5, 9, 13, 18, 24, 33, 47 },
            { 8, 17, 29, 42, 60, 80, 106, 183 }
        };

#ifdef ETC_ENABLE_SIMD
        const int32x4 base[] =
        {
            int32x4(baseR[0], baseG[0], baseB[0], 0xff),
            int32x4(baseR[1], baseG[1], baseB[1], 0xff)
        };

        // Write final pixels.
        for (int y = 0; y < 4; ++y)
        {
            u32* dest = reinterpret_cast<u32*>(output);
            output += stride;

            for (int x = 0; x < 4; ++x)
            {
                const int pixelNdx    = x * 4 + y;
                const int subBlock	  = (flipBit ? y : x) >> 1;
                const u32 tableNdx	  = table[subBlock];
                const u32 modifierNdx = getBits(src, pixelNdx, 1);
                const u32 modifierSgn = getBits(src, pixelNdx + 16, 1);
                const int modifier	  = modifierTable[modifierNdx][tableNdx] ^ (0 - modifierSgn);

                int32x4 c = clamp(base[subBlock] + modifier, 0, 255);
                dest[x] = c.pack();
            }
        }
#else
        // Write final pixels.
        for (int y = 0; y < 4; ++y)
        {
            u8* dest = output;
            output += stride;

            for (int x = 0; x < 4; ++x)
            {
                const int pixelNdx    = x * 4 + y;
                const int subBlock	  = (flipBit ? y : x) >> 1;
                const u32 tableNdx	  = table[subBlock];
                const u32 modifierNdx = getBits(src, pixelNdx, 1);
                const u32 modifierSgn = getBits(src, pixelNdx + 16, 1);
                const int modifier	  = modifierTable[modifierNdx][tableNdx] ^ (0 - modifierSgn);

                dest[0] = byteclamp(baseR[subBlock] + modifier);
                dest[1] = byteclamp(baseG[subBlock] + modifier);
                dest[2] = byteclamp(baseB[subBlock] + modifier);
                dest[3] = 255;
                dest += 4;
            }
        }
#endif
    }

    void decompress_block_etc2(u8* output, size_t stride, const u64 src, bool alphaMode)
    {
        enum Etc2Mode
        {
            MODE_INDIVIDUAL = 0,
            MODE_DIFFERENTIAL,
            MODE_T,
            MODE_H,
            MODE_PLANAR
        };

        const int diffOpaqueBit = getBits(src, 33, 1);
        const int selBR         = getBits(src, 59, 5);
        const int selBG         = getBits(src, 51, 5);
        const int selBB         = getBits(src, 43, 5);
        const int selDR         = s32_extend(getBits(src, 56, 3), 3);
        const int selDG         = s32_extend(getBits(src, 48, 3), 3);
        const int selDB         = s32_extend(getBits(src, 40, 3), 3);
        Etc2Mode  mode;

        if (!alphaMode && diffOpaqueBit == 0)
            mode = MODE_INDIVIDUAL;
        else if (!inRange(selBR + selDR, 0, 31))
            mode = MODE_T;
        else if (!inRange(selBG + selDG, 0, 31))
            mode = MODE_H;
        else if (!inRange(selBB + selDB, 0, 31))
            mode = MODE_PLANAR;
        else
            mode = MODE_DIFFERENTIAL;

        if (mode == MODE_INDIVIDUAL || mode == MODE_DIFFERENTIAL)
        {
            // Individual and differential modes have some steps in common, handle them here.
            static const int modifierTable[8][4] =
            {
                // 00   01   10    11
                {  2,   8,  -2,   -8 },
                {  5,  17,  -5,  -17 },
                {  9,  29,  -9,  -29 },
                { 13,  42, -13,  -42 },
                { 18,  60, -18,  -60 },
                { 24,  80, -24,  -80 },
                { 33, 106, -33, -106 },
                { 47, 183, -47, -183 }
            };

            const int flipBit  = getBits(src, 32, 1);
            const u32 table[2] = { getBits(src, 37, 3), getBits(src, 34, 3) };

            u8 baseR[2];
            u8 baseG[2];
            u8 baseB[2];

            if (mode == MODE_INDIVIDUAL)
            {
                // Individual mode, initial values.
                baseR[0] = u32_extend(getBits(src, 60, 4), 4, 8);
                baseR[1] = u32_extend(getBits(src, 56, 4), 4, 8);
                baseG[0] = u32_extend(getBits(src, 52, 4), 4, 8);
                baseG[1] = u32_extend(getBits(src, 48, 4), 4, 8);
                baseB[0] = u32_extend(getBits(src, 44, 4), 4, 8);
                baseB[1] = u32_extend(getBits(src, 40, 4), 4, 8);
            }
            else
            {
                // Differential mode, initial values.
                baseR[0] = u32_extend(selBR, 5, 8);
                baseG[0] = u32_extend(selBG, 5, 8);
                baseB[0] = u32_extend(selBB, 5, 8);
                baseR[1] = u32_extend(selBR + selDR, 5, 8);
                baseG[1] = u32_extend(selBG + selDG, 5, 8);
                baseB[1] = u32_extend(selBB + selDB, 5, 8);
            }

#ifdef ETC_ENABLE_SIMD
            const int32x4 base[] = 
            {
                int32x4(baseR[0], baseG[0], baseB[0], 0xff),
                int32x4(baseR[1], baseG[1], baseB[1], 0xff)
            };

            // Write final pixels for individual or differential mode.
            for (int y = 0; y < 4; ++y)
            {
                u32* dest = reinterpret_cast<u32*>(output);
                output += stride;

                for (int x = 0; x < 4; ++x)
                {
                    const int pixelNdx    = x * 4 + y;
                    const int subBlock	  = (flipBit ? y : x) >> 1;
                    const u32 tableNdx	  = table[subBlock];
                    const u32 modifierNdx = (getBits(src, 16+pixelNdx, 1) << 1) | getBits(src, pixelNdx, 1);

                    // If doing PUNCHTHROUGH version (alphaMode), opaque bit may affect colors.
                    if (alphaMode && diffOpaqueBit == 0 && modifierNdx == 2)
                    {
                        dest[x] = 0;
                    }
                    else
                    {
                        // PUNCHTHROUGH version and opaque bit may also affect modifiers.
                        int modifier;
                        if (alphaMode && diffOpaqueBit == 0 && (modifierNdx == 0 || modifierNdx == 2))
                            modifier = 0;
                        else
                            modifier = modifierTable[tableNdx][modifierNdx];

                        int32x4 c = clamp(base[subBlock] + modifier, 0, 255);
                        dest[x] = c.pack();
                    }
                }
            }
#else
            // Write final pixels for individual or differential mode.
            for (int y = 0; y < 4; ++y)
            {
                u8* dest = output;
                output += stride;

                for (int x = 0; x < 4; ++x)
                {
                    const int pixelNdx    = x * 4 + y;
                    const int subBlock	  = (flipBit ? y : x) >> 1;
                    const u32 tableNdx	  = table[subBlock];
                    const u32 modifierNdx = (getBits(src, 16+pixelNdx, 1) << 1) | getBits(src, pixelNdx, 1);

                    // If doing PUNCHTHROUGH version (alphaMode), opaque bit may affect colors.
                    if (alphaMode && diffOpaqueBit == 0 && modifierNdx == 2)
                    {
                        dest[0] = 0;
                        dest[1] = 0;
                        dest[2] = 0;
                        dest[3] = 0;
                    }
                    else
                    {
                        // PUNCHTHROUGH version and opaque bit may also affect modifiers.
                        int modifier;
                        if (alphaMode && diffOpaqueBit == 0 && (modifierNdx == 0 || modifierNdx == 2))
                            modifier = 0;
                        else
                            modifier = modifierTable[tableNdx][modifierNdx];

                        dest[0] = byteclamp(baseR[subBlock] + modifier);
                        dest[1] = byteclamp(baseG[subBlock] + modifier);
                        dest[2] = byteclamp(baseB[subBlock] + modifier);
                        dest[3] = 255;
                    }

                    dest += 4;
                }
            }
#endif
        }
        else if (mode == MODE_T || mode == MODE_H)
        {
            // T and H modes have some steps in common, handle them here.
            static const int distTable[8] = { 3, 6, 11, 16, 23, 32, 41, 64 };

            u8 paintR[4];
            u8 paintG[4];
            u8 paintB[4];

            if (mode == MODE_T)
            {
                // T mode, calculate paint values.
                const u8  R1a     = getBits(src, 59, 2);
                const u8  R1b     = getBits(src, 56, 2);
                const u8  G1      = getBits(src, 52, 4);
                const u8  B1      = getBits(src, 48, 4);
                const u8  R2      = getBits(src, 44, 4);
                const u8  G2      = getBits(src, 40, 4);
                const u8  B2      = getBits(src, 36, 4);
                const u32 distNdx = (getBits(src, 34, 2) << 1) | getBits(src, 32, 1);
                const int dist    = distTable[distNdx];

                paintR[0] = u32_extend((R1a << 2) | R1b, 4, 8);
                paintG[0] = u32_extend(G1, 4, 8);
                paintB[0] = u32_extend(B1, 4, 8);
                paintR[2] = u32_extend(R2, 4, 8);
                paintG[2] = u32_extend(G2, 4, 8);
                paintB[2] = u32_extend(B2, 4, 8);
                paintR[1] = byteclamp(paintR[2] + dist);
                paintG[1] = byteclamp(paintG[2] + dist);
                paintB[1] = byteclamp(paintB[2] + dist);
                paintR[3] = byteclamp(paintR[2] - dist);
                paintG[3] = byteclamp(paintG[2] - dist);
                paintB[3] = byteclamp(paintB[2] - dist);
            }
            else
            {
                // H mode, calculate paint values.
                const u8 R1	  = getBits(src, 59, 4);
                const u8 G1a  = getBits(src, 56, 3);
                const u8 G1b  = getBits(src, 52, 1);
                const u8 B1a  = getBits(src, 51, 1);
                const u8 B1b  = getBits(src, 47, 3);
                const u8 R2	  = getBits(src, 43, 4);
                const u8 G2	  = getBits(src, 39, 4);
                const u8 B2	  = getBits(src, 35, 4);

                int baseR[2];
                int	baseG[2];
                int baseB[2];
                u32 baseValue[2];
                u32 distNdx;
                int dist;

                baseR[0]      = u32_extend(R1, 4, 8);
                baseG[0]      = u32_extend((G1a << 1) | G1b, 4, 8);
                baseB[0]      = u32_extend((B1a << 3) | B1b, 4, 8);
                baseR[1]      = u32_extend(R2, 4, 8);
                baseG[1]      = u32_extend(G2, 4, 8);
                baseB[1]      = u32_extend(B2, 4, 8);
                baseValue[0]  = ((baseR[0]) << 16) | ((baseG[0]) << 8) | baseB[0];
                baseValue[1]  = ((baseR[1]) << 16) | ((baseG[1]) << 8) | baseB[1];
                distNdx       = (getBits(src, 34, 1) << 2) | (getBits(src, 32, 1) << 1) | (baseValue[0] >= baseValue[1]);
                dist          = distTable[distNdx];

                paintR[0] = byteclamp(baseR[0] + dist);
                paintG[0] = byteclamp(baseG[0] + dist);
                paintB[0] = byteclamp(baseB[0] + dist);
                paintR[1] = byteclamp(baseR[0] - dist);
                paintG[1] = byteclamp(baseG[0] - dist);
                paintB[1] = byteclamp(baseB[0] - dist);
                paintR[2] = byteclamp(baseR[1] + dist);
                paintG[2] = byteclamp(baseG[1] + dist);
                paintB[2] = byteclamp(baseB[1] + dist);
                paintR[3] = byteclamp(baseR[1] - dist);
                paintG[3] = byteclamp(baseG[1] - dist);
                paintB[3] = byteclamp(baseB[1] - dist);
            }

            const u32 paint[] =
            {
                image::makeRGBA(paintR[0], paintG[0], paintB[0], 0xff),
                image::makeRGBA(paintR[1], paintG[1], paintB[1], 0xff),
                image::makeRGBA(paintR[2], paintG[2], paintB[2], 0xff),
                image::makeRGBA(paintR[3], paintG[3], paintB[3], 0xff)
            };

            // Write final pixels for T or H mode.
            for (int y = 0; y < 4; ++y)
            {
                u32* dest = reinterpret_cast<u32*>(output);
                output += stride;

                for (int x = 0; x < 4; ++x)
                {
                    const int pixelNdx = x * 4 + y;
                    const u32 paintNdx = (getBits(src, 16+pixelNdx, 1) << 1) | getBits(src, pixelNdx, 1);

                    if (alphaMode && diffOpaqueBit == 0 && paintNdx == 2)
                    {
                        dest[x] = 0;
                    }
                    else
                    {
                        dest[x] = paint[paintNdx];
                    }
                }
            }
        }
        else
        {
            // Planar mode.
            const u8 GO1 = getBits(src, 56, 1);
            const u8 GO2 = getBits(src, 49, 6);
            const u8 BO1 = getBits(src, 48, 1);
            const u8 BO2 = getBits(src, 43, 2);
            const u8 BO3 = getBits(src, 39, 3);
            const u8 RH1 = getBits(src, 34, 5);
            const u8 RH2 = getBits(src, 32, 1);

            const int RO = u32_extend(getBits(src, 57, 6), 6, 8);
            const int GO = u32_extend((GO1 << 6) | GO2, 7, 8);
            const int BO = u32_extend((BO1 << 5) | (BO2 << 3) | BO3, 6, 8);

            int RH = u32_extend((RH1 << 1) | RH2, 6, 8) - RO;
            int GH = u32_extend(getBits(src, 25, 7), 7, 8) - GO;
            int BH = u32_extend(getBits(src, 19, 6), 6, 8) - BO;

            int RV = u32_extend(getBits(src, 13, 6), 6, 8) - RO;
            int GV = u32_extend(getBits(src, 6, 7), 7, 8) - GO;
            int BV = u32_extend(getBits(src, 0, 6), 6, 8) - BO;

#ifdef ETC_ENABLE_SIMD
            int32x4 CO(4 * RO + 2, 4 * GO + 2, 4 * BO + 2, 255 * 4);
            int32x4 V(RV, GV, BV, 0);
            int32x4 H(RH, GH, BH, 0);

            // Write final pixels for planar mode.
            for (int y = 0; y < 4; ++y)
            {
                u32* dest = reinterpret_cast<u32*>(output);
                output += stride;

                int32x4 C = CO;
                CO += V;

                for (int x = 0; x < 4; ++x)
                {
                    int32x4 c = clamp((C >> 2), 0, 255);
                    dest[x] = c.pack();
                    C += H;
                }
            }
#else
            // Write final pixels for planar mode.
            for (int y = 0; y < 4; ++y)
            {
                u8* dest = output;
                output += stride;

                int R = y * RV + 4 * RO + 2;
                int G = y * GV + 4 * GO + 2;
                int B = y * BV + 4 * BO + 2;

                for (int x = 0; x < 4; ++x)
                {
                    dest[0] = byteclamp(R >> 2);
                    dest[1] = byteclamp(G >> 2);
                    dest[2] = byteclamp(B >> 2);
                    dest[3] = 255;
                    dest += 4;

                    R += RH;
                    G += GH;
                    B += BH;
                }
            }
#endif
        }
    }

    void decompress_block_eac8(u8* output, size_t stride, u64 src)
    {
        static const int modifierTable[16][8] =
        {
            {-3,  -6,  -9, -15,  2,  5,  8, 14},
            {-3,  -7, -10, -13,  2,  6,  9, 12},
            {-2,  -5,  -8, -13,  1,  4,  7, 12},
            {-2,  -4,  -6, -13,  1,  3,  5, 12},
            {-3,  -6,  -8, -12,  2,  5,  7, 11},
            {-3,  -7,  -9, -11,  2,  6,  8, 10},
            {-4,  -7,  -8, -11,  3,  6,  7, 10},
            {-3,  -5,  -8, -11,  2,  4,  7, 10},
            {-2,  -6,  -8, -10,  1,  5,  7,  9},
            {-2,  -5,  -8, -10,  1,  4,  7,  9},
            {-2,  -4,  -8, -10,  1,  3,  7,  9},
            {-2,  -5,  -7, -10,  1,  4,  6,  9},
            {-3,  -4,  -7, -10,  2,  3,  6,  9},
            {-1,  -2,  -3, -10,  0,  1,  2,  9},
            {-4,  -6,  -8,  -9,  3,  5,  7,  8},
            {-3,  -5,  -7,  -9,  2,  4,  6,  8}
        };

        const int baseCodeword = getBits(src, 56, 8);
        const int multiplier   = getBits(src, 52, 4);
        const u32 tableNdx     = getBits(src, 48, 4);

        for (int y = 0; y < 4; ++y)
        {
            u8* dest = output;
            output += stride;

            for (int x = 0; x < 4; ++x)
            {
                const int pixelNdx    = x * 4 + y;
                const int pixelBitNdx = 45 - 3 * pixelNdx;
                const u32 modifierNdx = getBits(src, pixelBitNdx, 3);
                const int modifier    = modifierTable[tableNdx][modifierNdx];

                dest[3] = byteclamp(baseCodeword + multiplier * modifier);
                dest += 4;
            }
        }
    }

    void decompress_block_eac11(u8* output, size_t xstride, size_t ystride, u64 src, bool signedMode)
    {
        static const int modifierTable[16][8] =
        {
            {-3,  -6,  -9, -15,  2,  5,  8, 14},
            {-3,  -7, -10, -13,  2,  6,  9, 12},
            {-2,  -5,  -8, -13,  1,  4,  7, 12},
            {-2,  -4,  -6, -13,  1,  3,  5, 12},
            {-3,  -6,  -8, -12,  2,  5,  7, 11},
            {-3,  -7,  -9, -11,  2,  6,  8, 10},
            {-4,  -7,  -8, -11,  3,  6,  7, 10},
            {-3,  -5,  -8, -11,  2,  4,  7, 10},
            {-2,  -6,  -8, -10,  1,  5,  7,  9},
            {-2,  -5,  -8, -10,  1,  4,  7,  9},
            {-2,  -4,  -8, -10,  1,  3,  7,  9},
            {-2,  -5,  -7, -10,  1,  4,  6,  9},
            {-3,  -4,  -7, -10,  2,  3,  6,  9},
            {-1,  -2,  -3, -10,  0,  1,  2,  9},
            {-4,  -6,  -8,  -9,  3,  5,  7,  8},
            {-3,  -5,  -7,  -9,  2,  4,  6,  8}
        };

        const s32 multiplier = s32(getBits(src, 52, 4));
        const s32 tableNdx   = s32(getBits(src, 48, 4));
        s32 baseCodeword     = s32(getBits(src, 56, 8));

        if (signedMode)
        {
            if (baseCodeword > 127)
                baseCodeword -= 256;
            if (baseCodeword == -128)
                baseCodeword = -127;

            for (int y = 0; y < 4; ++y)
            {
                s16* dest = reinterpret_cast<s16*>(output);
                output += ystride;

                for (int x = 0; x < 4; ++x)
                {
                    const int pixelNdx    = x * 4 + y;
                    const int pixelBitNdx = 45 - 3 * pixelNdx;
                    const u32 modifierNdx = getBits(src, pixelBitNdx, 3);
                    const int modifier    = modifierTable[tableNdx][modifierNdx];

                    s16 sample;
                    if (multiplier != 0)
                        sample = s16(clamp(baseCodeword*8 + multiplier*modifier*8, -1023, 1023));
                    else
                        sample = s16(clamp(baseCodeword*8 + modifier, -1023, 1023));

                    dest[0] = extend11To16WithSign(sample);
                    dest += xstride;
                }
            }
        }
        else
        {
            for (int y = 0; y < 4; ++y)
            {
                u16* dest = reinterpret_cast<u16*>(output);
                output += ystride;

                for (int x = 0; x < 4; ++x)
                {
                    const int pixelNdx    = x * 4 + y;
                    const int pixelBitNdx = 45 - 3 * pixelNdx;
                    const u32 modifierNdx = getBits(src, pixelBitNdx, 3);
                    const int modifier    = modifierTable[tableNdx][modifierNdx];

                    u16 sample;
                    if (multiplier != 0)
                        sample = u16(clamp(baseCodeword*8 + 4 + multiplier*modifier*8, 0, 2047));
                    else
                        sample = u16(clamp(baseCodeword*8 + 4 + modifier, 0, 2047));

                    dest[0] = u16_extend(sample, 11, 16);
                    dest += xstride;
                }
            }
        }
    }

} // namespace

namespace mango::image
{

    void decode_block_etc1(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        const u64 color = uload64be(input);
        decompress_block_etc1(output, stride, color);
    }

    void decode_block_etc2(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        const bool alphaMode = info.compression == TextureCompression::ETC2_RGB_ALPHA1 ||
                               info.compression == TextureCompression::ETC2_SRGB_ALPHA1;
        const u64 color = uload64be(input);
        decompress_block_etc2(output, stride, color, alphaMode);
    }

    void decode_block_etc2_eac(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        const u64 alpha = uload64be(input + 0);
        const u64 color = uload64be(input + 8);
        decompress_block_etc2(output, stride, color, false);
        decompress_block_eac8(output, stride, alpha);
    }

    void decode_block_eac_r11(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        const bool signedMode = info.compression == TextureCompression::EAC_SIGNED_R11;
        const u64 red = uload64be(input);
        decompress_block_eac11(output, 1, stride, red, signedMode);
    }

    void decode_block_eac_rg11(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        const bool signedMode = info.compression == TextureCompression::EAC_SIGNED_RG11;
        const u64 red = uload64be(input + 0);
        const u64 green = uload64be(input + 8);
        decompress_block_eac11(output + 0, 2, stride, red, signedMode);
        decompress_block_eac11(output + 2, 2, stride, green, signedMode);
    }

} // namespace mango::image
