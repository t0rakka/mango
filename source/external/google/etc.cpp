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

#include "etc.hpp"
#include <mango/core/endian.hpp>

#ifdef MANGO_ENABLE_LICENSE_APACHE

namespace
{
    using namespace mango;

    inline u32 getBit (u64 src, int bit)
    {
        return (src >> bit) & 1;
    }

    inline u32 getBits (u64 src, int low, int high)
    {
        const int numBits = (high - low) + 1;
        return (src >> low) & ((1 << numBits) - 1);
    }

    constexpr u32 u32_extend(u32 value, int from, int to)
    {
        return (value << (to - from)) | (value >> (from * 2 - to));
    }

    inline int extendSigned3To8 (int src)
    {
        const bool isNeg = (src & (1<<2)) != 0;
        return ((isNeg ? ~((1<<3)-1) : 0) | src);
    }

    inline u8 extend5Delta3To8 (int base5, int delta3)
    {
        const int t = base5 + extendSigned3To8(delta3);
        return u32_extend(t, 5, 8);
    }

    inline u16 extend11To16 (u16 src)
    {
        return (src << 5) | (src >> 6);
    }

    inline s16 extend11To16WithSign (s16 src)
    {
        if (src < 0)
            return -(s16)extend11To16(-src);
        else
            return (s16)extend11To16(src);
    }

    inline int clamp32(int value, int vmin, int vmax)
    {
        if (value < vmin) value = vmin;
        else if (value > vmax) value = vmax;
        return value;
    }

    inline bool inRange(int value, int vmin, int vmax)
    {
        return value >= vmin && value <= vmax;
    }

    void decompress_block_etc1(u8* output, int stride, const u64 src)
    {
        const int	flipBit		= getBit(src, 32);
        const int	diffBit		= getBit(src, 33);
        const u32	table[2]	= { getBits(src, 37, 39), getBits(src, 34, 36) };
        int         baseR[2];
        int         baseG[2];
        int         baseB[2];
        
        if (!diffBit)
        {
            // Individual mode.
            baseR[0] = u32_extend(getBits(src, 60, 63), 4, 8);
            baseR[1] = u32_extend(getBits(src, 56, 59), 4, 8);
            baseG[0] = u32_extend(getBits(src, 52, 55), 4, 8);
            baseG[1] = u32_extend(getBits(src, 48, 51), 4, 8);
            baseB[0] = u32_extend(getBits(src, 44, 47), 4, 8);
            baseB[1] = u32_extend(getBits(src, 40, 43), 4, 8);
        }
        else
        {
            // Differential mode.
            int bR = getBits(src, 59, 63);
            int dR = getBits(src, 56, 58);
            int bG = getBits(src, 51, 55);
            int dG = getBits(src, 48, 50);
            int bB = getBits(src, 43, 47);
            int dB = getBits(src, 40, 42);
            
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
        
        // Write final pixels.
        for (int y = 0; y < 4; ++y)
        {
            u8* dest = output;
            
            for (int x = 0; x < 4; ++x)
            {
                const int    pixelNdx    = x * 4 + y;
                const int    subBlock	= (flipBit ? y : x) >> 1;
                const u32    tableNdx	= table[subBlock];
                const u32    modifierNdx	= getBit(src, pixelNdx);
                const u32    modifierSgn = getBit(src, pixelNdx + 16);
                const int    modifier	= modifierTable[modifierNdx][tableNdx] ^ (0 - modifierSgn);
                
                dest[0] = (u8)byteclamp(baseR[subBlock] + modifier);
                dest[1] = (u8)byteclamp(baseG[subBlock] + modifier);
                dest[2] = (u8)byteclamp(baseB[subBlock] + modifier);
                dest[3] = 255;
                dest += 4;
            }
            
            output += stride;
        }
    }

    void decompress_block_etc2(u8* output, int stride, const u64 src, bool alphaMode)
    {
        enum Etc2Mode
        {
            MODE_INDIVIDUAL = 0,
            MODE_DIFFERENTIAL,
            MODE_T,
            MODE_H,
            MODE_PLANAR
        };
        
        const int	diffOpaqueBit	= getBit(src, 33);
        const s8	selBR			= (s8)getBits(src, 59, 63);	// 5 bits.
        const s8	selBG			= (s8)getBits(src, 51, 55);
        const s8	selBB			= (s8)getBits(src, 43, 47);
        const s8	selDR           = (s8)extendSigned3To8(getBits(src, 56, 58)); // 3 bits.
        const s8	selDG           = (s8)extendSigned3To8(getBits(src, 48, 50));
        const s8	selDB           = (s8)extendSigned3To8(getBits(src, 40, 42));
        Etc2Mode	mode;
        
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
            
            const int	flipBit		= getBit(src, 32);
            const u32	table[2]	= { getBits(src, 37, 39), getBits(src, 34, 36) };
            u8		    baseR[2];
            u8		    baseG[2];
            u8		    baseB[2];
            
            if (mode == MODE_INDIVIDUAL)
            {
                // Individual mode, initial values.
                baseR[0] = u32_extend(getBits(src, 60, 63), 4, 8);
                baseR[1] = u32_extend(getBits(src, 56, 59), 4, 8);
                baseG[0] = u32_extend(getBits(src, 52, 55), 4, 8);
                baseG[1] = u32_extend(getBits(src, 48, 51), 4, 8);
                baseB[0] = u32_extend(getBits(src, 44, 47), 4, 8);
                baseB[1] = u32_extend(getBits(src, 40, 43), 4, 8);
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
            
            // Write final pixels for individual or differential mode.
            for (int y = 0; y < 4; ++y)
            {
                u8* dest = output;
                
                for (int x = 0; x < 4; ++x)
                {
                    const int   pixelNdx    = x * 4 + y;
                    const int   subBlock	= (flipBit ? y : x) >> 1;
                    const u32	tableNdx	= table[subBlock];
                    const u32	modifierNdx	= (getBit(src, 16+pixelNdx) << 1) | getBit(src, pixelNdx);
                    
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
                        int modifier;
                        
                        // PUNCHTHROUGH version and opaque bit may also affect modifiers.
                        if (alphaMode && diffOpaqueBit == 0 && (modifierNdx == 0 || modifierNdx == 2))
                            modifier = 0;
                        else
                            modifier = modifierTable[tableNdx][modifierNdx];
                        
                        dest[0] = (u8)byteclamp(baseR[subBlock] + modifier);
                        dest[1] = (u8)byteclamp(baseG[subBlock] + modifier);
                        dest[2] = (u8)byteclamp(baseB[subBlock] + modifier);
                        dest[3] = 255;
                    }
                    
                    dest += 4;
                }
                
                output += stride;
            }
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
                const u8  R1a		= (u8)getBits(src, 59, 60);
                const u8  R1b		= (u8)getBits(src, 56, 57);
                const u8  G1		= (u8)getBits(src, 52, 55);
                const u8  B1		= (u8)getBits(src, 48, 51);
                const u8  R2		= (u8)getBits(src, 44, 47);
                const u8  G2		= (u8)getBits(src, 40, 43);
                const u8  B2		= (u8)getBits(src, 36, 39);
                const u32 distNdx	= (getBits(src, 34, 35) << 1) | getBit(src, 32);
                const int dist		= distTable[distNdx];
                
                paintR[0] = u32_extend((R1a << 2) | R1b, 4, 8);
                paintG[0] = u32_extend(G1, 4, 8);
                paintB[0] = u32_extend(B1, 4, 8);
                paintR[2] = u32_extend(R2, 4, 8);
                paintG[2] = u32_extend(G2, 4, 8);
                paintB[2] = u32_extend(B2, 4, 8);
                paintR[1] = (u8)byteclamp(paintR[2] + dist);
                paintG[1] = (u8)byteclamp(paintG[2] + dist);
                paintB[1] = (u8)byteclamp(paintB[2] + dist);
                paintR[3] = (u8)byteclamp(paintR[2] - dist);
                paintG[3] = (u8)byteclamp(paintG[2] - dist);
                paintB[3] = (u8)byteclamp(paintB[2] - dist);
            }
            else
            {
                // H mode, calculate paint values.
                const u8 R1		= (u8)getBits(src, 59, 62);
                const u8 G1a	= (u8)getBits(src, 56, 58);
                const u8 G1b	= (u8)getBit(src, 52);
                const u8 B1a	= (u8)getBit(src, 51);
                const u8 B1b	= (u8)getBits(src, 47, 49);
                const u8 R2		= (u8)getBits(src, 43, 46);
                const u8 G2		= (u8)getBits(src, 39, 42);
                const u8 B2		= (u8)getBits(src, 35, 38);
                u8		baseR[2];
                u8		baseG[2];
                u8		baseB[2];
                u32		baseValue[2];
                u32		distNdx;
                int		dist;
                
                baseR[0]		= u32_extend(R1, 4, 8);
                baseG[0]		= u32_extend((G1a << 1) | G1b, 4, 8);
                baseB[0]		= u32_extend((B1a << 3) | B1b, 4, 8);
                baseR[1]		= u32_extend(R2, 4, 8);
                baseG[1]		= u32_extend(G2, 4, 8);
                baseB[1]		= u32_extend(B2, 4, 8);
                baseValue[0]	= (((u32)baseR[0]) << 16) | (((u32)baseG[0]) << 8) | baseB[0];
                baseValue[1]	= (((u32)baseR[1]) << 16) | (((u32)baseG[1]) << 8) | baseB[1];
                distNdx			= (getBit(src, 34) << 2) | (getBit(src, 32) << 1) | (u32)(baseValue[0] >= baseValue[1]);
                dist			= distTable[distNdx];
                
                paintR[0]		= (u8)byteclamp(baseR[0] + dist);
                paintG[0]		= (u8)byteclamp(baseG[0] + dist);
                paintB[0]		= (u8)byteclamp(baseB[0] + dist);
                paintR[1]		= (u8)byteclamp(baseR[0] - dist);
                paintG[1]		= (u8)byteclamp(baseG[0] - dist);
                paintB[1]		= (u8)byteclamp(baseB[0] - dist);
                paintR[2]		= (u8)byteclamp(baseR[1] + dist);
                paintG[2]		= (u8)byteclamp(baseG[1] + dist);
                paintB[2]		= (u8)byteclamp(baseB[1] + dist);
                paintR[3]		= (u8)byteclamp(baseR[1] - dist);
                paintG[3]		= (u8)byteclamp(baseG[1] - dist);
                paintB[3]		= (u8)byteclamp(baseB[1] - dist);
            }
            
            // Write final pixels for T or H mode.
            for (int y = 0; y < 4; ++y)
            {
                u8* dest = output;

                for (int x = 0; x < 4; ++x)
                {
                    const int   pixelNdx    = x * 4 + y;
                    const u32	paintNdx	= (getBit(src, 16+pixelNdx) << 1) | getBit(src, pixelNdx);

                    if (alphaMode && diffOpaqueBit == 0 && paintNdx == 2)
                    {
                        dest[0] = 0;
                        dest[1] = 0;
                        dest[2] = 0;
                        dest[3] = 0;
                    }
                    else
                    {
                        dest[0] = (u8)byteclamp(paintR[paintNdx]);
                        dest[1] = (u8)byteclamp(paintG[paintNdx]);
                        dest[2] = (u8)byteclamp(paintB[paintNdx]);
                        dest[3] = 255;
                    }
                    
                    dest += 4;
                }
                
                output += stride;
            }
        }
        else
        {
            // Planar mode.
            const u8 GO1 = (u8)getBit(src, 56);
            const u8 GO2 = (u8)getBits(src, 49, 54);
            const u8 BO1 = (u8)getBit(src, 48);
            const u8 BO2 = (u8)getBits(src, 43, 44);
            const u8 BO3 = (u8)getBits(src, 39, 41);
            const u8 RH1 = (u8)getBits(src, 34, 38);
            const u8 RH2 = (u8)getBit(src, 32);
            const u8 RO	= u32_extend(getBits(src, 57, 62), 6, 8);
            const u8 GO	= u32_extend((GO1 << 6) | GO2, 7, 8);
            const u8 BO	= u32_extend((BO1 << 5) | (BO2 << 3) | BO3, 6, 8);
            const u8 RH	= u32_extend((RH1 << 1) | RH2, 6, 8);
            const u8 GH	= u32_extend(getBits(src, 25, 31), 7, 8);
            const u8 BH	= u32_extend(getBits(src, 19, 24), 6, 8);
            const u8 RV	= u32_extend(getBits(src, 13, 18), 6, 8);
            const u8 GV	= u32_extend(getBits(src, 6, 12), 7, 8);
            const u8 BV	= u32_extend(getBits(src, 0, 5), 6, 8);
            
            // Write final pixels for planar mode.
            for (int y = 0; y < 4; ++y)
            {
                u8* dest = output;
                
                for (int x = 0; x < 4; ++x)
                {
                    const int unclampedR = (x * ((int)RH-(int)RO) + y * ((int)RV-(int)RO) + 4*(int)RO + 2) >> 2;
                    const int unclampedG = (x * ((int)GH-(int)GO) + y * ((int)GV-(int)GO) + 4*(int)GO + 2) >> 2;
                    const int unclampedB = (x * ((int)BH-(int)BO) + y * ((int)BV-(int)BO) + 4*(int)BO + 2) >> 2;
                    
                    dest[0] = (u8)byteclamp(unclampedR);
                    dest[1] = (u8)byteclamp(unclampedG);
                    dest[2] = (u8)byteclamp(unclampedB);
                    dest[3] = 255;
                    dest += 4;
                }
                
                output += stride;
            }
        }
    }

    void decompress_block_eac8(u8* output, int stride, u64 src)
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

        const u8 baseCodeword	= (u8)getBits(src, 56, 63);
        const u8 multiplier		= (u8)getBits(src, 52, 55);
        const u32 tableNdx		= getBits(src, 48, 51);

        for (int y = 0; y < 4; ++y)
        {
            u8* dest = output;

            for (int x = 0; x < 4; ++x)
            {
                const int pixelNdx    = x * 4 + y;
                const int pixelBitNdx = 45 - 3 * pixelNdx;
                const u32 modifierNdx = getBits(src, pixelBitNdx, pixelBitNdx + 2);
                const int modifier    = modifierTable[tableNdx][modifierNdx];

                dest[3] = (u8)byteclamp(baseCodeword + multiplier * modifier);
                dest += 4;
            }

            output += stride;
        }
    }

    void decompress_block_eac11(u8* output, int xstride, int ystride, u64 src, bool signedMode)
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

        const s32 multiplier = (s32)getBits(src, 52, 55);
        const s32 tableNdx	 = (s32)getBits(src, 48, 51);
        s32 baseCodeword	 = (s32)getBits(src, 56, 63);

        if (signedMode)
        {
            if (baseCodeword > 127)
                baseCodeword -= 256;
            if (baseCodeword == -128)
                baseCodeword = -127;

            for (int y = 0; y < 4; ++y)
            {
                s16* dest = reinterpret_cast<s16*>(output);

                for (int x = 0; x < 4; ++x)
                {
                    const int pixelNdx    = x * 4 + y;
                    const int pixelBitNdx = 45 - 3 * pixelNdx;
                    const u32 modifierNdx = getBits(src, pixelBitNdx, pixelBitNdx + 2);
                    const int modifier    = modifierTable[tableNdx][modifierNdx];

                    s16 sample;
                    if (multiplier != 0)
                        sample = (s16)clamp32(baseCodeword*8 + multiplier*modifier*8, -1023, 1023);
                    else
                        sample = (s16)clamp32(baseCodeword*8 + modifier, -1023, 1023);

                    dest[0] = extend11To16WithSign(sample);
                    dest += xstride;
                }

                output += ystride;
            }
        }
        else
        {
            for (int y = 0; y < 4; ++y)
            {
                u16* dest = reinterpret_cast<u16*>(output);

                for (int x = 0; x < 4; ++x)
                {
                    const int pixelNdx    = x * 4 + y;
                    const int pixelBitNdx = 45 - 3 * pixelNdx;
                    const u32 modifierNdx = getBits(src, pixelBitNdx, pixelBitNdx + 2);
                    const int modifier    = modifierTable[tableNdx][modifierNdx];

                    u16 sample;
                    if (multiplier != 0)
                        sample = (u16)clamp32(baseCodeword*8 + 4 + multiplier*modifier*8, 0, 2047);
                    else
                        sample = (u16)clamp32(baseCodeword*8 + 4 + modifier, 0, 2047);

                    dest[0] = extend11To16(sample);
                    dest += xstride;
                }

                output += ystride;
            }
        }
    }

} // namespace

namespace mango
{

    void decode_block_etc1(const TextureCompressionInfo& info, u8* output, const u8* input, int stride)
    {
        MANGO_UNREFERENCED_PARAMETER(info);
        const u64 color = uload64be(input);
        decompress_block_etc1(output, stride, color);
    }

    void decode_block_etc2(const TextureCompressionInfo& info, u8* output, const u8* input, int stride)
    {
        const bool alphaMode = info.compression == TextureCompression::ETC2_RGB_ALPHA1 ||
                               info.compression == TextureCompression::ETC2_SRGB_ALPHA1;
        const u64 color = uload64be(input);
        decompress_block_etc2(output, stride, color, alphaMode);
    }

    void decode_block_etc2_eac(const TextureCompressionInfo& info, u8* output, const u8* input, int stride)
    {
        MANGO_UNREFERENCED_PARAMETER(info);
        const u64 alpha = uload64be(input + 0);
        const u64 color = uload64be(input + 8);
        decompress_block_etc2(output, stride, color, false);
        decompress_block_eac8(output, stride, alpha);
    }

    void decode_block_eac_r11(const TextureCompressionInfo& info, u8* output, const u8* input, int stride)
    {
        const bool signedMode = info.compression == TextureCompression::EAC_SIGNED_R11;
        const u64 red = uload64be(input);
        decompress_block_eac11(output, 1, stride, red, signedMode);
    }

    void decode_block_eac_rg11(const TextureCompressionInfo& info, u8* output, const u8* input, int stride)
    {
        const bool signedMode = info.compression == TextureCompression::EAC_SIGNED_RG11;
        const u64 red = uload64be(input + 0);
        const u64 green = uload64be(input + 8);
        decompress_block_eac11(output + 0, 2, stride, red, signedMode);
        decompress_block_eac11(output + 2, 2, stride, green, signedMode);
    }

} // namespace mango

#endif // MANGO_ENABLE_LICENSE_APACHE
