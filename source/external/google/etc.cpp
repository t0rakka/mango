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

    inline uint32 getBit (uint64 src, int bit)
    {
        return (src >> bit) & 1;
    }

    inline uint32 getBits (uint64 src, int low, int high)
    {
        const int numBits = (high - low) + 1;
        return (src >> low) & ((1 << numBits) - 1);
    }

    inline uint8 extend4To8 (int src)
    {
        return uint8((src << 4) | src);
    }

    inline uint8 extend5To8 (int src)
    {
        return uint8((src << 3) | (src >> 2));
    }

    inline uint8 extend6To8 (int src)
    {
        return uint8((src << 2) | (src >> 4));
    }

    inline uint8 extend7To8 (int src)
    {
        return uint8((src << 1) | (src >> 6));
    }

    inline int extendSigned3To8 (int src)
    {
        const bool isNeg = (src & (1<<2)) != 0;
        return ((isNeg ? ~((1<<3)-1) : 0) | src);
    }

    inline uint8 extend5Delta3To8 (int base5, int delta3)
    {
        const int t = base5 + extendSigned3To8(delta3);
        return extend5To8(t);
    }

    inline uint16 extend11To16 (uint16 src)
    {
        return (src << 5) | (src >> 6);
    }

    inline int16 extend11To16WithSign (int16 src)
    {
        if (src < 0)
            return -(int16)extend11To16(-src);
        else
            return (int16)extend11To16(src);
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

    void decompress_block_etc1(uint8* output, int stride, const uint64 src)
    {
        const int		flipBit		= getBit(src, 32);
        const int		diffBit		= getBit(src, 33);
        const uint32	table[2]	= { getBits(src, 37, 39), getBits(src, 34, 36) };
        int             baseR[2];
        int             baseG[2];
        int             baseB[2];
        
        if (!diffBit)
        {
            // Individual mode.
            baseR[0] = extend4To8(getBits(src, 60, 63));
            baseR[1] = extend4To8(getBits(src, 56, 59));
            baseG[0] = extend4To8(getBits(src, 52, 55));
            baseG[1] = extend4To8(getBits(src, 48, 51));
            baseB[0] = extend4To8(getBits(src, 44, 47));
            baseB[1] = extend4To8(getBits(src, 40, 43));
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
            
            baseR[0] = extend5To8(bR);
            baseG[0] = extend5To8(bG);
            baseB[0] = extend5To8(bB);
            baseR[1] = extend5Delta3To8(bR, dR);
            baseG[1] = extend5Delta3To8(bG, dG);
            baseB[1] = extend5Delta3To8(bB, dB);
        }
        
        static const uint8 modifierTable[2][8] =
        {
            { 2, 5, 9, 13, 18, 24, 33, 47 },
            { 8, 17, 29, 42, 60, 80, 106, 183 }
        };
        
        // Write final pixels.
        for (int y = 0; y < 4; ++y)
        {
            uint8* dest = output;
            
            for (int x = 0; x < 4; ++x)
            {
                const int       pixelNdx    = x * 4 + y;
                const int       subBlock	= (flipBit ? y : x) >> 1;
                const uint32    tableNdx	= table[subBlock];
                const uint32    modifierNdx	= getBit(src, pixelNdx);
                const uint32    modifierSgn = getBit(src, pixelNdx + 16);
                const int       modifier	= modifierTable[modifierNdx][tableNdx] ^ (0 - modifierSgn);
                
                dest[0] = (uint8)byteclamp(baseR[subBlock] + modifier);
                dest[1] = (uint8)byteclamp(baseG[subBlock] + modifier);
                dest[2] = (uint8)byteclamp(baseB[subBlock] + modifier);
                dest[3] = 255;
                dest += 4;
            }
            
            output += stride;
        }
    }

    void decompress_block_etc2(uint8* output, int stride, const uint64 src, bool alphaMode)
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
        const int8	selBR			= (int8)getBits(src, 59, 63);	// 5 bits.
        const int8	selBG			= (int8)getBits(src, 51, 55);
        const int8	selBB			= (int8)getBits(src, 43, 47);
        const int8	selDR           = (int8)extendSigned3To8(getBits(src, 56, 58)); // 3 bits.
        const int8	selDG           = (int8)extendSigned3To8(getBits(src, 48, 50));
        const int8	selDB           = (int8)extendSigned3To8(getBits(src, 40, 42));
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
            
            const int		flipBit		= getBit(src, 32);
            const uint32	table[2]	= { getBits(src, 37, 39), getBits(src, 34, 36) };
            uint8			baseR[2];
            uint8			baseG[2];
            uint8			baseB[2];
            
            if (mode == MODE_INDIVIDUAL)
            {
                // Individual mode, initial values.
                baseR[0] = extend4To8(getBits(src, 60, 63));
                baseR[1] = extend4To8(getBits(src, 56, 59));
                baseG[0] = extend4To8(getBits(src, 52, 55));
                baseG[1] = extend4To8(getBits(src, 48, 51));
                baseB[0] = extend4To8(getBits(src, 44, 47));
                baseB[1] = extend4To8(getBits(src, 40, 43));
            }
            else
            {
                // Differential mode, initial values.
                baseR[0] = extend5To8(selBR);
                baseG[0] = extend5To8(selBG);
                baseB[0] = extend5To8(selBB);
                baseR[1] = extend5To8(selBR + selDR);
                baseG[1] = extend5To8(selBG + selDG);
                baseB[1] = extend5To8(selBB + selDB);
            }
            
            // Write final pixels for individual or differential mode.
            for (int y = 0; y < 4; ++y)
            {
                uint8* dest = output;
                
                for (int x = 0; x < 4; ++x)
                {
                    const int       pixelNdx    = x * 4 + y;
                    const int       subBlock	= (flipBit ? y : x) >> 1;
                    const uint32	tableNdx	= table[subBlock];
                    const uint32	modifierNdx	= (getBit(src, 16+pixelNdx) << 1) | getBit(src, pixelNdx);
                    
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
                        
                        dest[0] = (uint8)byteclamp(baseR[subBlock] + modifier);
                        dest[1] = (uint8)byteclamp(baseG[subBlock] + modifier);
                        dest[2] = (uint8)byteclamp(baseB[subBlock] + modifier);
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
            
            uint8 paintR[4];
            uint8 paintG[4];
            uint8 paintB[4];
            
            if (mode == MODE_T)
            {
                // T mode, calculate paint values.
                const uint8	R1a			= (uint8)getBits(src, 59, 60);
                const uint8	R1b			= (uint8)getBits(src, 56, 57);
                const uint8	G1			= (uint8)getBits(src, 52, 55);
                const uint8	B1			= (uint8)getBits(src, 48, 51);
                const uint8	R2			= (uint8)getBits(src, 44, 47);
                const uint8	G2			= (uint8)getBits(src, 40, 43);
                const uint8	B2			= (uint8)getBits(src, 36, 39);
                const uint32 distNdx	= (getBits(src, 34, 35) << 1) | getBit(src, 32);
                const int	 dist		= distTable[distNdx];
                
                paintR[0] = extend4To8((R1a << 2) | R1b);
                paintG[0] = extend4To8(G1);
                paintB[0] = extend4To8(B1);
                paintR[2] = extend4To8(R2);
                paintG[2] = extend4To8(G2);
                paintB[2] = extend4To8(B2);
                paintR[1] = (uint8)byteclamp(paintR[2] + dist);
                paintG[1] = (uint8)byteclamp(paintG[2] + dist);
                paintB[1] = (uint8)byteclamp(paintB[2] + dist);
                paintR[3] = (uint8)byteclamp(paintR[2] - dist);
                paintG[3] = (uint8)byteclamp(paintG[2] - dist);
                paintB[3] = (uint8)byteclamp(paintB[2] - dist);
            }
            else
            {
                // H mode, calculate paint values.
                const uint8	R1		= (uint8)getBits(src, 59, 62);
                const uint8	G1a		= (uint8)getBits(src, 56, 58);
                const uint8	G1b		= (uint8)getBit(src, 52);
                const uint8	B1a		= (uint8)getBit(src, 51);
                const uint8	B1b		= (uint8)getBits(src, 47, 49);
                const uint8	R2		= (uint8)getBits(src, 43, 46);
                const uint8	G2		= (uint8)getBits(src, 39, 42);
                const uint8	B2		= (uint8)getBits(src, 35, 38);
                uint8		baseR[2];
                uint8		baseG[2];
                uint8		baseB[2];
                uint32		baseValue[2];
                uint32		distNdx;
                int			dist;
                
                baseR[0]		= extend4To8(R1);
                baseG[0]		= extend4To8((G1a << 1) | G1b);
                baseB[0]		= extend4To8((B1a << 3) | B1b);
                baseR[1]		= extend4To8(R2);
                baseG[1]		= extend4To8(G2);
                baseB[1]		= extend4To8(B2);
                baseValue[0]	= (((uint32)baseR[0]) << 16) | (((uint32)baseG[0]) << 8) | baseB[0];
                baseValue[1]	= (((uint32)baseR[1]) << 16) | (((uint32)baseG[1]) << 8) | baseB[1];
                distNdx			= (getBit(src, 34) << 2) | (getBit(src, 32) << 1) | (uint32)(baseValue[0] >= baseValue[1]);
                dist			= distTable[distNdx];
                
                paintR[0]		= (uint8)byteclamp(baseR[0] + dist);
                paintG[0]		= (uint8)byteclamp(baseG[0] + dist);
                paintB[0]		= (uint8)byteclamp(baseB[0] + dist);
                paintR[1]		= (uint8)byteclamp(baseR[0] - dist);
                paintG[1]		= (uint8)byteclamp(baseG[0] - dist);
                paintB[1]		= (uint8)byteclamp(baseB[0] - dist);
                paintR[2]		= (uint8)byteclamp(baseR[1] + dist);
                paintG[2]		= (uint8)byteclamp(baseG[1] + dist);
                paintB[2]		= (uint8)byteclamp(baseB[1] + dist);
                paintR[3]		= (uint8)byteclamp(baseR[1] - dist);
                paintG[3]		= (uint8)byteclamp(baseG[1] - dist);
                paintB[3]		= (uint8)byteclamp(baseB[1] - dist);
            }
            
            // Write final pixels for T or H mode.
            for (int y = 0; y < 4; ++y)
            {
                uint8* dest = output;
                
                for (int x = 0; x < 4; ++x)
                {
                    const int       pixelNdx    = x * 4 + y;
                    const uint32	paintNdx	= (getBit(src, 16+pixelNdx) << 1) | getBit(src, pixelNdx);
                    
                    if (alphaMode && diffOpaqueBit == 0 && paintNdx == 2)
                    {
                        dest[0] = 0;
                        dest[1] = 0;
                        dest[2] = 0;
                        dest[3] = 0;
                    }
                    else
                    {
                        dest[0] = (uint8)byteclamp(paintR[paintNdx]);
                        dest[1] = (uint8)byteclamp(paintG[paintNdx]);
                        dest[2] = (uint8)byteclamp(paintB[paintNdx]);
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
            const uint8 GO1	= (uint8)getBit(src, 56);
            const uint8 GO2	= (uint8)getBits(src, 49, 54);
            const uint8 BO1	= (uint8)getBit(src, 48);
            const uint8 BO2	= (uint8)getBits(src, 43, 44);
            const uint8 BO3	= (uint8)getBits(src, 39, 41);
            const uint8 RH1	= (uint8)getBits(src, 34, 38);
            const uint8 RH2	= (uint8)getBit(src, 32);
            const uint8 RO	= extend6To8(getBits(src, 57, 62));
            const uint8 GO	= extend7To8((GO1 << 6) | GO2);
            const uint8 BO	= extend6To8((BO1 << 5) | (BO2 << 3) | BO3);
            const uint8 RH	= extend6To8((RH1 << 1) | RH2);
            const uint8 GH	= extend7To8(getBits(src, 25, 31));
            const uint8 BH	= extend6To8(getBits(src, 19, 24));
            const uint8 RV	= extend6To8(getBits(src, 13, 18));
            const uint8 GV	= extend7To8(getBits(src, 6, 12));
            const uint8 BV	= extend6To8(getBits(src, 0, 5));
            
            // Write final pixels for planar mode.
            for (int y = 0; y < 4; ++y)
            {
                uint8* dest = output;
                
                for (int x = 0; x < 4; ++x)
                {
                    const int unclampedR = (x * ((int)RH-(int)RO) + y * ((int)RV-(int)RO) + 4*(int)RO + 2) >> 2;
                    const int unclampedG = (x * ((int)GH-(int)GO) + y * ((int)GV-(int)GO) + 4*(int)GO + 2) >> 2;
                    const int unclampedB = (x * ((int)BH-(int)BO) + y * ((int)BV-(int)BO) + 4*(int)BO + 2) >> 2;
                    
                    dest[0] = (uint8)byteclamp(unclampedR);
                    dest[1] = (uint8)byteclamp(unclampedG);
                    dest[2] = (uint8)byteclamp(unclampedB);
                    dest[3] = 255;
                    dest += 4;
                }
                
                output += stride;
            }
        }
    }

    void decompress_block_eac8(uint8* output, int stride, uint64 src)
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

        const uint8	baseCodeword	= (uint8)getBits(src, 56, 63);
        const uint8	multiplier		= (uint8)getBits(src, 52, 55);
        const uint32 tableNdx		= getBits(src, 48, 51);

        for (int y = 0; y < 4; ++y)
        {
            uint8* dest = output;

            for (int x = 0; x < 4; ++x)
            {
                const int    pixelNdx    = x * 4 + y;
                const int    pixelBitNdx = 45 - 3 * pixelNdx;
                const uint32 modifierNdx = (getBit(src, pixelBitNdx + 2) << 2) | (getBit(src, pixelBitNdx + 1) << 1) | getBit(src, pixelBitNdx);
                const int    modifier    = modifierTable[tableNdx][modifierNdx];

                dest[3] = (uint8)byteclamp(baseCodeword + multiplier * modifier);
                dest += 4;
            }

            output += stride;
        }
    }

    void decompress_block_eac11(uint8* output, int xstride, int ystride, uint64 src, bool signedMode)
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

        const int32 multiplier	= (int32)getBits(src, 52, 55);
        const int32 tableNdx	= (int32)getBits(src, 48, 51);
        int32 baseCodeword		= (int32)getBits(src, 56, 63);

        if (signedMode)
        {
            if (baseCodeword > 127)
                baseCodeword -= 256;
            if (baseCodeword == -128)
                baseCodeword = -127;

            for (int y = 0; y < 4; ++y)
            {
                int16* dest = reinterpret_cast<int16*>(output);

                for (int x = 0; x < 4; ++x)
                {
                    const int    pixelNdx    = x * 4 + y;
                    const int    pixelBitNdx = 45 - 3 * pixelNdx;
                    const uint32 modifierNdx = (getBit(src, pixelBitNdx + 2) << 2) |
                    (getBit(src, pixelBitNdx + 1) << 1) |
                    getBit(src, pixelBitNdx);
                    const int    modifier    = modifierTable[tableNdx][modifierNdx];

                    int16 sample;
                    if (multiplier != 0)
                        sample = (int16)clamp32(baseCodeword*8 + multiplier*modifier*8, -1023, 1023);
                    else
                        sample = (int16)clamp32(baseCodeword*8 + modifier, -1023, 1023);

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
                uint16* dest = reinterpret_cast<uint16*>(output);

                for (int x = 0; x < 4; ++x)
                {
                    const int    pixelNdx    = x * 4 + y;
                    const int    pixelBitNdx = 45 - 3 * pixelNdx;
                    const uint32 modifierNdx = (getBit(src, pixelBitNdx + 2) << 2) |
                    (getBit(src, pixelBitNdx + 1) << 1) |
                    getBit(src, pixelBitNdx);
                    const int    modifier    = modifierTable[tableNdx][modifierNdx];

                    uint16 sample;
                    if (multiplier != 0)
                        sample = (uint16)clamp32(baseCodeword*8 + 4 + multiplier*modifier*8, 0, 2047);
                    else
                        sample = (uint16)clamp32(baseCodeword*8 + 4 + modifier, 0, 2047);

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

    void decode_block_etc1(const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride)
    {
        MANGO_UNREFERENCED_PARAMETER(info);
        const uint64 color = uload64be(input);
        decompress_block_etc1(output, stride, color);
    }

    void decode_block_etc2(const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride)
    {
        const bool alphaMode = info.compression == TextureCompression::ETC2_RGB_ALPHA1 ||
                               info.compression == TextureCompression::ETC2_SRGB_ALPHA1;
        const uint64 color = uload64be(input);
        decompress_block_etc2(output, stride, color, alphaMode);
    }

    void decode_block_etc2_eac(const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride)
    {
        MANGO_UNREFERENCED_PARAMETER(info);
        const uint64 alpha = uload64be(input + 0);
        const uint64 color = uload64be(input + 8);
        decompress_block_etc2(output, stride, color, false);
        decompress_block_eac8(output, stride, alpha);
    }

    void decode_block_eac_r11(const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride)
    {
        const bool signedMode = info.compression == TextureCompression::EAC_SIGNED_R11;
        const uint64 red = uload64be(input);
        decompress_block_eac11(output, 1, stride, red, signedMode);
    }

    void decode_block_eac_rg11(const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride)
    {
        const bool signedMode = info.compression == TextureCompression::EAC_SIGNED_RG11;
        const uint64 red = uload64be(input + 0);
        const uint64 green = uload64be(input + 8);
        decompress_block_eac11(output + 0, 2, stride, red, signedMode);
        decompress_block_eac11(output + 2, 2, stride, green, signedMode);
    }

} // namespace mango

#endif // MANGO_ENABLE_LICENSE_APACHE
