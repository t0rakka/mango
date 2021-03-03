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
#include "../../../include/mango/core/endian.hpp"
#include "../../../include/mango/math/vector.hpp"
#include "../../../include/mango/image/compression.hpp"

namespace
{
    using namespace mango;
    using namespace mango::math;

    enum
    {
        ASTC_MAX_BLOCK_WIDTH	= 12,
        ASTC_MAX_BLOCK_HEIGHT	= 12
    };

    inline int divRoundUp (int a, int b)
    {
        return a/b + ((a%b) ? 1 : 0);
    }

    inline u32 getBit (u32 src, int ndx)
    {
        return (src >> ndx) & 1;
    }

    inline u32 getBits (u32 src, int low, int high)
    {
        const int numBits = (high-low) + 1;
        return (src >> low) & ((1u<<numBits)-1);
    }

    inline bool isBitSet (u32 src, int ndx)
    {
        return getBit(src, ndx) != 0;
    }

    inline u32 reverseBits (u32 src, int numBits)
    {
        u32 result = 0;
        for (int i = 0; i < numBits; i++)
            result |= ((src >> i) & 1) << (numBits-1-i);
        return result;
    }

    inline u32 bitReplicationScale (u32 src, int numSrcBits, int numDstBits)
    {
        u32 dst = 0;
        for (int shift = numDstBits-numSrcBits; shift > -numSrcBits; shift -= numSrcBits)
            dst |= shift >= 0 ? src << shift : src >> -shift;
        return dst;
    }

    inline s32 signExtend (s32 src, int numSrcBits)
    {
        const bool negative = (src & (1 << (numSrcBits-1))) != 0;
        return src | (negative ? ~((1 << numSrcBits) - 1) : 0);
    }

    inline bool isFloat16InfOrNan (float16 v)
    {
        return getBits(v.u, 10, 14) == 31;
    }

    // A helper for getting bits from a 128-bit block.
    class Block128
    {
    private:
        enum
        {
            WORD_BYTES	= sizeof(u64),
            WORD_BITS	= 8 * WORD_BYTES,
            NUM_WORDS	= 128 / WORD_BITS
        };

    public:
        Block128 (const u8* src)
        {
            for (int wordNdx = 0; wordNdx < NUM_WORDS; wordNdx++)
            {
                m_words[wordNdx] = 0;
                for (int byteNdx = 0; byteNdx < WORD_BYTES; byteNdx++)
                    m_words[wordNdx] |= (u64)src[wordNdx*WORD_BYTES + byteNdx] << (8*byteNdx);
            }
        }

        u32 getBit (int ndx) const
        {
            return (m_words[ndx / WORD_BITS] >> (ndx % WORD_BITS)) & 1;
        }

        u32 getBits (int low, int high) const
        {
            if (high-low+1 == 0)
                return 0;
            
            const int word0Ndx = low / WORD_BITS;
            const int word1Ndx = high / WORD_BITS;
            
            if (word0Ndx == word1Ndx)
            {
                u64 temp = (m_words[word0Ndx] & ((((u64)1 << high%WORD_BITS << 1) - 1))) >> ((u64)low % WORD_BITS);
                return u32(temp);
            }
            else
            {
                return (u32)(m_words[word0Ndx] >> (low%WORD_BITS)) |
                       (u32)((m_words[word1Ndx] & (((u64)1 << high%WORD_BITS << 1) - 1)) << (high-low - high%WORD_BITS));
            }
        }
        
        bool isBitSet (int ndx) const
        {
            return getBit(ndx) != 0;
        }
        
    private:
        u64 m_words[NUM_WORDS];
    };
    
    // A helper for sequential access into a Block128.
    class BitAccessStream
    {
    protected:
        BitAccessStream& operator = (const BitAccessStream&) { return *this; }

    public:
        BitAccessStream (const Block128& src, int startNdxInSrc, int length, bool forward)
        : m_src				(src)
        , m_startNdxInSrc	(startNdxInSrc)
        , m_length			(length)
        , m_forward			(forward)
        , m_ndx				(0)
        {
        }
        
        // Get the next num bits. Bits at positions greater than or equal to m_length are zeros.
        u32 getNext (int num)
        {
            if (num == 0 || m_ndx >= m_length)
                return 0;
            
            const int end				= m_ndx + num;
            const int numBitsFromSrc	= std::max(0, std::min(m_length, end) - m_ndx);
            const int low				= m_ndx;
            const int high				= m_ndx + numBitsFromSrc - 1;
            
            m_ndx += num;
            
            return m_forward ?			   m_src.getBits(m_startNdxInSrc + low,  m_startNdxInSrc + high)
                             : reverseBits(m_src.getBits(m_startNdxInSrc - high, m_startNdxInSrc - low), numBitsFromSrc);
        }
        
    private:
        const Block128&		m_src;
        const int			m_startNdxInSrc;
        const int			m_length;
        const bool			m_forward;
        
        int					m_ndx;
    };
    
    enum ISEMode
    {
        ISEMODE_TRIT = 0,
        ISEMODE_QUINT,
        ISEMODE_PLAIN_BIT,
        
        ISEMODE_LAST
    };
    
    struct ISEParams
    {
        ISEMode		mode;
        int			numBits;
        
        ISEParams (ISEMode mode_, int numBits_) : mode(mode_), numBits(numBits_) {}
    };
    
    static inline int computeNumRequiredBits (const ISEParams& iseParams, int numValues)
    {
        switch (iseParams.mode)
        {
            case ISEMODE_TRIT:			return divRoundUp(numValues*8, 5) + numValues*iseParams.numBits;
            case ISEMODE_QUINT:			return divRoundUp(numValues*7, 3) + numValues*iseParams.numBits;
            case ISEMODE_PLAIN_BIT:		return numValues*iseParams.numBits;
            default:
                return -1;
        }
    }
    
    struct ISEDecodedResult
    {
        u32 m;
        u32 tq; //!< Trit or quint value, depending on ISE mode.
        u32 v;
    };
    
    // Data from an ASTC block's "block mode" part (i.e. bits [0,10]).
    struct ASTCBlockMode
    {
        bool		isError;
        // \note Following fields only relevant if !isError.
        bool		isVoidExtent;
        // \note Following fields only relevant if !isVoidExtent.
        bool		isDualPlane;
        int			weightGridWidth;
        int			weightGridHeight;
        ISEParams	weightISEParams;
        
        ASTCBlockMode (void)
        : isError			(true)
        , isVoidExtent		(true)
        , isDualPlane		(true)
        , weightGridWidth	(-1)
        , weightGridHeight	(-1)
        , weightISEParams	(ISEMODE_LAST, -1)
        {
        }
    };
    
    inline int computeNumWeights (const ASTCBlockMode& mode)
    {
        return mode.weightGridWidth * mode.weightGridHeight * (mode.isDualPlane ? 2 : 1);
    }
    
    struct ColorEndpointPair
    {
        uint32x4 e0;
        uint32x4 e1;
    };

    struct TexelWeightPair
    {
        u32 w[2];
    };
    
    ASTCBlockMode getASTCBlockMode (u32 blockModeData)
    {
        ASTCBlockMode blockMode;
        blockMode.isError = true; // \note Set to false later, if not error.
        
        blockMode.isVoidExtent = getBits(blockModeData, 0, 8) == 0x1fc;
        
        if (!blockMode.isVoidExtent)
        {
            if ((getBits(blockModeData, 0, 1) == 0 && getBits(blockModeData, 6, 8) == 7) || getBits(blockModeData, 0, 3) == 0)
                return blockMode; // Invalid ("reserved").
            
            u32 r = (u32)-1; // \note Set in the following branches.
            
            if (getBits(blockModeData, 0, 1) == 0)
            {
                const u32 r0	= getBit(blockModeData, 4);
                const u32 r1	= getBit(blockModeData, 2);
                const u32 r2	= getBit(blockModeData, 3);
                const u32 i78	= getBits(blockModeData, 7, 8);
                
                r = (r2 << 2) | (r1 << 1) | (r0 << 0);
                
                if (i78 == 3)
                {
                    const bool i5 = isBitSet(blockModeData, 5);
                    blockMode.weightGridWidth	= i5 ? 10 : 6;
                    blockMode.weightGridHeight	= i5 ? 6  : 10;
                }
                else
                {
                    const u32 a = getBits(blockModeData, 5, 6);
                    switch (i78)
                    {
                        case 0:	 blockMode.weightGridWidth = 12;	blockMode.weightGridHeight = a + 2;									break;
                        case 1:	 blockMode.weightGridWidth = a + 2;	blockMode.weightGridHeight = 12;									break;
                        case 2:	 blockMode.weightGridWidth = a + 6;	blockMode.weightGridHeight = getBits(blockModeData, 9, 10) + 6;		break;
                        default: break;
                    }
                }
            }
            else
            {
                const u32 r0	= getBit(blockModeData, 4);
                const u32 r1	= getBit(blockModeData, 0);
                const u32 r2	= getBit(blockModeData, 1);
                const u32 i23	= getBits(blockModeData, 2, 3);
                const u32 a	= getBits(blockModeData, 5, 6);
                
                r = (r2 << 2) | (r1 << 1) | (r0 << 0);
                
                if (i23 == 3)
                {
                    const u32	b	= getBit(blockModeData, 7);
                    const bool		i8	= isBitSet(blockModeData, 8);
                    blockMode.weightGridWidth	= i8 ? b+2 : a+2;
                    blockMode.weightGridHeight	= i8 ? a+2 : b+6;
                }
                else
                {
                    const u32 b = getBits(blockModeData, 7, 8);
                    
                    switch (i23)
                    {
                        case 0:	blockMode.weightGridWidth = b + 4;	blockMode.weightGridHeight = a + 2;	break;
                        case 1:	blockMode.weightGridWidth = b + 8;	blockMode.weightGridHeight = a + 2;	break;
                        case 2:	blockMode.weightGridWidth = a + 2;	blockMode.weightGridHeight = b + 8;	break;
                        default: break;
                    }
                }
            }
            
            const bool	zeroDH		= getBits(blockModeData, 0, 1) == 0 && getBits(blockModeData, 7, 8) == 2;
            const bool	h			= zeroDH ? 0 : isBitSet(blockModeData, 9);
            blockMode.isDualPlane	= zeroDH ? 0 : isBitSet(blockModeData, 10);
            
            {
                ISEMode&	m	= blockMode.weightISEParams.mode;
                int&		b	= blockMode.weightISEParams.numBits;
                m = ISEMODE_PLAIN_BIT;
                b = 0;
                
                if (h)
                {
                    switch (r)
                    {
                        case 2:							m = ISEMODE_QUINT;	b = 1;	break;
                        case 3:		m = ISEMODE_TRIT;						b = 2;	break;
                        case 4:												b = 4;	break;
                        case 5:							m = ISEMODE_QUINT;	b = 2;	break;
                        case 6:		m = ISEMODE_TRIT;						b = 3;	break;
                        case 7:												b = 5;	break;
                        default: break;
                    }
                }
                else
                {
                    switch (r)
                    {
                        case 2: 											b = 1;	break;
                        case 3: 	m = ISEMODE_TRIT;								break;
                        case 4: 											b = 2;	break;
                        case 5: 						m = ISEMODE_QUINT;			break;
                        case 6: 	m = ISEMODE_TRIT;						b = 1;	break;
                        case 7: 											b = 3;	break;
                        default: break;
                    }
                }
            }
        }

        blockMode.isError = false;
        return blockMode;
    }

    inline void setASTCErrorColorBlock (void* dst, int blockWidth, int blockHeight, bool isSRGB)
    {
        if (isSRGB)
        {
            u8* const dstU = (u8*)dst;

            for (int i = 0; i < blockWidth*blockHeight; i++)
            {
                dstU[4*i + 0] = 0xff;
                dstU[4*i + 1] = 0;
                dstU[4*i + 2] = 0xff;
                dstU[4*i + 3] = 0xff;
            }
        }
        else
        {
            float16* const dstF = (float16*)dst;

            for (int i = 0; i < blockWidth*blockHeight; i++)
            {
                dstF[4*i + 0] = float16(1.0f);
                dstF[4*i + 1] = float16(0.0f);
                dstF[4*i + 2] = float16(1.0f);
                dstF[4*i + 3] = float16(1.0f);
            }
        }
    }

    void decodeVoidExtentBlock (void* dst, const Block128& blockData, int blockWidth, int blockHeight, bool isSRGB, bool isLDRMode)
    {
        const u32	minSExtent			= blockData.getBits(12, 24);
        const u32	maxSExtent			= blockData.getBits(25, 37);
        const u32	minTExtent			= blockData.getBits(38, 50);
        const u32	maxTExtent			= blockData.getBits(51, 63);
        const bool	allExtentsAllOnes	= minSExtent == 0x1fff && maxSExtent == 0x1fff && minTExtent == 0x1fff && maxTExtent == 0x1fff;
        const bool	isHDRBlock			= blockData.isBitSet(9);

        if ((isLDRMode && isHDRBlock) || (!allExtentsAllOnes && (minSExtent >= maxSExtent || minTExtent >= maxTExtent)))
        {
            setASTCErrorColorBlock(dst, blockWidth, blockHeight, isSRGB);
            return;
        }

        const u32 rgba[4] =
        {
            blockData.getBits(64,  79),
            blockData.getBits(80,  95),
            blockData.getBits(96,  111),
            blockData.getBits(112, 127)
        };

        if (isSRGB)
        {
            u8* const dstU = (u8*)dst;
            for (int i = 0; i < blockWidth*blockHeight; i++)
                for (int c = 0; c < 4; c++)
                {
                    dstU[i*4 + c] = (rgba[c] & 0xff00) >> 8;
                }
        }
        else
        {
            float16* const dstF = (float16*)dst;

            if (isHDRBlock)
            {
                for (int c = 0; c < 4; c++)
                {
                    float16 h = u16(rgba[c]);
                    if (isFloat16InfOrNan(h))
                    {
                        //throw tcu::InternalError("Infinity or NaN color component in HDR void extent block in ASTC texture (behavior undefined by ASTC specification)");
                    }
                }

                for (int i = 0; i < blockWidth*blockHeight; i++)
                    for (int c = 0; c < 4; c++)
                    {
                        float16 h = u16(rgba[c]);
                        dstF[i*4 + c] = h;
                    }
            }
            else
            {
                for (int i = 0; i < blockWidth*blockHeight; i++)
                    for (int c = 0; c < 4; c++)
                    {
                        dstF[i*4 + c] = float16(rgba[c] == 65535 ? 1.0f : float(rgba[c] / 65536.0f));
                    }
            }
        }

        return;
    }

    void decodeColorEndpointModes (u32* endpointModesDst, const Block128& blockData, int numPartitions, int extraCemBitsStart)
    {
        if (numPartitions == 1)
        {
            endpointModesDst[0] = blockData.getBits(13, 16);
        }
        else
        {
            const u32 highLevelSelector = blockData.getBits(23, 24);
            
            if (highLevelSelector == 0)
            {
                const u32 mode = blockData.getBits(25, 28);
                for (int i = 0; i < numPartitions; i++)
                    endpointModesDst[i] = mode;
            }
            else
            {
                for (int partNdx = 0; partNdx < numPartitions; partNdx++)
                {
                    const u32 cemClass		= highLevelSelector - (blockData.isBitSet(25 + partNdx) ? 0 : 1);
                    const u32 lowBit0Ndx	= numPartitions + 2*partNdx;
                    const u32 lowBit1Ndx	= numPartitions + 2*partNdx + 1;
                    const u32 lowBit0		= blockData.getBit(lowBit0Ndx < 4 ? 25+lowBit0Ndx : extraCemBitsStart+lowBit0Ndx-4);
                    const u32 lowBit1		= blockData.getBit(lowBit1Ndx < 4 ? 25+lowBit1Ndx : extraCemBitsStart+lowBit1Ndx-4);
                    
                    endpointModesDst[partNdx] = (cemClass << 2) | (lowBit1 << 1) | lowBit0;
                }
            }
        }
    }
    
    inline int computeNumColorEndpointValues (u32 endpointMode)
    {
        return (endpointMode/4 + 1) * 2;
    }
    
    int computeNumColorEndpointValues (const u32* endpointModes, int numPartitions)
    {
        int result = 0;
        for (int i = 0; i < numPartitions; i++)
            result += computeNumColorEndpointValues(endpointModes[i]);
        return result;
    }
    
    void decodeISETritBlock (ISEDecodedResult* dst, int numValues, BitAccessStream& data, int numBits)
    {
        u32 m[5];

        m[0]	= data.getNext(numBits);
        u32 T01	= data.getNext(2);
        m[1]	= data.getNext(numBits);
        u32 T23	= data.getNext(2);
        m[2]	= data.getNext(numBits);
        u32 T4	= data.getNext(1);
        m[3]	= data.getNext(numBits);
        u32 T56	= data.getNext(2);
        m[4]	= data.getNext(numBits);
        u32 T7	= data.getNext(1);

        switch (numValues)
        {
            // note Fall-throughs.
            case 1: T23		= 0;
            case 2: T4		= 0;
            case 3: T56		= 0;
            case 4: T7		= 0;
            case 5: break;
            default:
                break;
        }

        const u32 T = (T7 << 7) | (T56 << 5) | (T4 << 4) | (T23 << 2) | (T01 << 0);

        static const u32 tritsFromT[256][5] =
        {
            { 0,0,0,0,0 }, { 1,0,0,0,0 }, { 2,0,0,0,0 }, { 0,0,2,0,0 }, { 0,1,0,0,0 }, { 1,1,0,0,0 }, { 2,1,0,0,0 }, { 1,0,2,0,0 }, { 0,2,0,0,0 }, { 1,2,0,0,0 }, { 2,2,0,0,0 }, { 2,0,2,0,0 }, { 0,2,2,0,0 }, { 1,2,2,0,0 }, { 2,2,2,0,0 }, { 2,0,2,0,0 },
            { 0,0,1,0,0 }, { 1,0,1,0,0 }, { 2,0,1,0,0 }, { 0,1,2,0,0 }, { 0,1,1,0,0 }, { 1,1,1,0,0 }, { 2,1,1,0,0 }, { 1,1,2,0,0 }, { 0,2,1,0,0 }, { 1,2,1,0,0 }, { 2,2,1,0,0 }, { 2,1,2,0,0 }, { 0,0,0,2,2 }, { 1,0,0,2,2 }, { 2,0,0,2,2 }, { 0,0,2,2,2 },
            { 0,0,0,1,0 }, { 1,0,0,1,0 }, { 2,0,0,1,0 }, { 0,0,2,1,0 }, { 0,1,0,1,0 }, { 1,1,0,1,0 }, { 2,1,0,1,0 }, { 1,0,2,1,0 }, { 0,2,0,1,0 }, { 1,2,0,1,0 }, { 2,2,0,1,0 }, { 2,0,2,1,0 }, { 0,2,2,1,0 }, { 1,2,2,1,0 }, { 2,2,2,1,0 }, { 2,0,2,1,0 },
            { 0,0,1,1,0 }, { 1,0,1,1,0 }, { 2,0,1,1,0 }, { 0,1,2,1,0 }, { 0,1,1,1,0 }, { 1,1,1,1,0 }, { 2,1,1,1,0 }, { 1,1,2,1,0 }, { 0,2,1,1,0 }, { 1,2,1,1,0 }, { 2,2,1,1,0 }, { 2,1,2,1,0 }, { 0,1,0,2,2 }, { 1,1,0,2,2 }, { 2,1,0,2,2 }, { 1,0,2,2,2 },
            { 0,0,0,2,0 }, { 1,0,0,2,0 }, { 2,0,0,2,0 }, { 0,0,2,2,0 }, { 0,1,0,2,0 }, { 1,1,0,2,0 }, { 2,1,0,2,0 }, { 1,0,2,2,0 }, { 0,2,0,2,0 }, { 1,2,0,2,0 }, { 2,2,0,2,0 }, { 2,0,2,2,0 }, { 0,2,2,2,0 }, { 1,2,2,2,0 }, { 2,2,2,2,0 }, { 2,0,2,2,0 },
            { 0,0,1,2,0 }, { 1,0,1,2,0 }, { 2,0,1,2,0 }, { 0,1,2,2,0 }, { 0,1,1,2,0 }, { 1,1,1,2,0 }, { 2,1,1,2,0 }, { 1,1,2,2,0 }, { 0,2,1,2,0 }, { 1,2,1,2,0 }, { 2,2,1,2,0 }, { 2,1,2,2,0 }, { 0,2,0,2,2 }, { 1,2,0,2,2 }, { 2,2,0,2,2 }, { 2,0,2,2,2 },
            { 0,0,0,0,2 }, { 1,0,0,0,2 }, { 2,0,0,0,2 }, { 0,0,2,0,2 }, { 0,1,0,0,2 }, { 1,1,0,0,2 }, { 2,1,0,0,2 }, { 1,0,2,0,2 }, { 0,2,0,0,2 }, { 1,2,0,0,2 }, { 2,2,0,0,2 }, { 2,0,2,0,2 }, { 0,2,2,0,2 }, { 1,2,2,0,2 }, { 2,2,2,0,2 }, { 2,0,2,0,2 },
            { 0,0,1,0,2 }, { 1,0,1,0,2 }, { 2,0,1,0,2 }, { 0,1,2,0,2 }, { 0,1,1,0,2 }, { 1,1,1,0,2 }, { 2,1,1,0,2 }, { 1,1,2,0,2 }, { 0,2,1,0,2 }, { 1,2,1,0,2 }, { 2,2,1,0,2 }, { 2,1,2,0,2 }, { 0,2,2,2,2 }, { 1,2,2,2,2 }, { 2,2,2,2,2 }, { 2,0,2,2,2 },
            { 0,0,0,0,1 }, { 1,0,0,0,1 }, { 2,0,0,0,1 }, { 0,0,2,0,1 }, { 0,1,0,0,1 }, { 1,1,0,0,1 }, { 2,1,0,0,1 }, { 1,0,2,0,1 }, { 0,2,0,0,1 }, { 1,2,0,0,1 }, { 2,2,0,0,1 }, { 2,0,2,0,1 }, { 0,2,2,0,1 }, { 1,2,2,0,1 }, { 2,2,2,0,1 }, { 2,0,2,0,1 },
            { 0,0,1,0,1 }, { 1,0,1,0,1 }, { 2,0,1,0,1 }, { 0,1,2,0,1 }, { 0,1,1,0,1 }, { 1,1,1,0,1 }, { 2,1,1,0,1 }, { 1,1,2,0,1 }, { 0,2,1,0,1 }, { 1,2,1,0,1 }, { 2,2,1,0,1 }, { 2,1,2,0,1 }, { 0,0,1,2,2 }, { 1,0,1,2,2 }, { 2,0,1,2,2 }, { 0,1,2,2,2 },
            { 0,0,0,1,1 }, { 1,0,0,1,1 }, { 2,0,0,1,1 }, { 0,0,2,1,1 }, { 0,1,0,1,1 }, { 1,1,0,1,1 }, { 2,1,0,1,1 }, { 1,0,2,1,1 }, { 0,2,0,1,1 }, { 1,2,0,1,1 }, { 2,2,0,1,1 }, { 2,0,2,1,1 }, { 0,2,2,1,1 }, { 1,2,2,1,1 }, { 2,2,2,1,1 }, { 2,0,2,1,1 },
            { 0,0,1,1,1 }, { 1,0,1,1,1 }, { 2,0,1,1,1 }, { 0,1,2,1,1 }, { 0,1,1,1,1 }, { 1,1,1,1,1 }, { 2,1,1,1,1 }, { 1,1,2,1,1 }, { 0,2,1,1,1 }, { 1,2,1,1,1 }, { 2,2,1,1,1 }, { 2,1,2,1,1 }, { 0,1,1,2,2 }, { 1,1,1,2,2 }, { 2,1,1,2,2 }, { 1,1,2,2,2 },
            { 0,0,0,2,1 }, { 1,0,0,2,1 }, { 2,0,0,2,1 }, { 0,0,2,2,1 }, { 0,1,0,2,1 }, { 1,1,0,2,1 }, { 2,1,0,2,1 }, { 1,0,2,2,1 }, { 0,2,0,2,1 }, { 1,2,0,2,1 }, { 2,2,0,2,1 }, { 2,0,2,2,1 }, { 0,2,2,2,1 }, { 1,2,2,2,1 }, { 2,2,2,2,1 }, { 2,0,2,2,1 },
            { 0,0,1,2,1 }, { 1,0,1,2,1 }, { 2,0,1,2,1 }, { 0,1,2,2,1 }, { 0,1,1,2,1 }, { 1,1,1,2,1 }, { 2,1,1,2,1 }, { 1,1,2,2,1 }, { 0,2,1,2,1 }, { 1,2,1,2,1 }, { 2,2,1,2,1 }, { 2,1,2,2,1 }, { 0,2,1,2,2 }, { 1,2,1,2,2 }, { 2,2,1,2,2 }, { 2,1,2,2,2 },
            { 0,0,0,1,2 }, { 1,0,0,1,2 }, { 2,0,0,1,2 }, { 0,0,2,1,2 }, { 0,1,0,1,2 }, { 1,1,0,1,2 }, { 2,1,0,1,2 }, { 1,0,2,1,2 }, { 0,2,0,1,2 }, { 1,2,0,1,2 }, { 2,2,0,1,2 }, { 2,0,2,1,2 }, { 0,2,2,1,2 }, { 1,2,2,1,2 }, { 2,2,2,1,2 }, { 2,0,2,1,2 },
            { 0,0,1,1,2 }, { 1,0,1,1,2 }, { 2,0,1,1,2 }, { 0,1,2,1,2 }, { 0,1,1,1,2 }, { 1,1,1,1,2 }, { 2,1,1,1,2 }, { 1,1,2,1,2 }, { 0,2,1,1,2 }, { 1,2,1,1,2 }, { 2,2,1,1,2 }, { 2,1,2,1,2 }, { 0,2,2,2,2 }, { 1,2,2,2,2 }, { 2,2,2,2,2 }, { 2,1,2,2,2 }
        };

        const u32 (& trits)[5] = tritsFromT[T];

        for (int i = 0; i < numValues; i++)
        {
            dst[i].m	= m[i];
            dst[i].tq	= trits[i];
            dst[i].v	= (trits[i] << numBits) + m[i];
        }
    }

    void decodeISEQuintBlock (ISEDecodedResult* dst, int numValues, BitAccessStream& data, int numBits)
    {
        u32 m[3];

        m[0]	 = data.getNext(numBits);
        u32 Q012 = data.getNext(3);
        m[1]	 = data.getNext(numBits);
        u32 Q34	 = data.getNext(2);
        m[2]	 = data.getNext(numBits);
        u32 Q56	 = data.getNext(2);

        switch (numValues)
        {
            // note Fall-throughs.
            case 1: Q34		= 0;
            case 2: Q56		= 0;
            case 3: break;
            default:
                break;
        }

        const u32 Q = (Q56 << 5) | (Q34 << 3) | (Q012 << 0);

        static const u32 quintsFromQ[256][3] =
        {
            { 0,0,0 }, { 1,0,0 }, { 2,0,0 }, { 3,0,0 }, { 4,0,0 }, { 0,4,0 }, { 4,4,0 }, { 4,4,4 }, { 0,1,0 }, { 1,1,0 }, { 2,1,0 }, { 3,1,0 }, { 4,1,0 }, { 1,4,0 }, { 4,4,1 }, { 4,4,4 },
            { 0,2,0 }, { 1,2,0 }, { 2,2,0 }, { 3,2,0 }, { 4,2,0 }, { 2,4,0 }, { 4,4,2 }, { 4,4,4 }, { 0,3,0 }, { 1,3,0 }, { 2,3,0 }, { 3,3,0 }, { 4,3,0 }, { 3,4,0 }, { 4,4,3 }, { 4,4,4 },
            { 0,0,1 }, { 1,0,1 }, { 2,0,1 }, { 3,0,1 }, { 4,0,1 }, { 0,4,1 }, { 4,0,4 }, { 0,4,4 }, { 0,1,1 }, { 1,1,1 }, { 2,1,1 }, { 3,1,1 }, { 4,1,1 }, { 1,4,1 }, { 4,1,4 }, { 1,4,4 },
            { 0,2,1 }, { 1,2,1 }, { 2,2,1 }, { 3,2,1 }, { 4,2,1 }, { 2,4,1 }, { 4,2,4 }, { 2,4,4 }, { 0,3,1 }, { 1,3,1 }, { 2,3,1 }, { 3,3,1 }, { 4,3,1 }, { 3,4,1 }, { 4,3,4 }, { 3,4,4 },
            { 0,0,2 }, { 1,0,2 }, { 2,0,2 }, { 3,0,2 }, { 4,0,2 }, { 0,4,2 }, { 2,0,4 }, { 3,0,4 }, { 0,1,2 }, { 1,1,2 }, { 2,1,2 }, { 3,1,2 }, { 4,1,2 }, { 1,4,2 }, { 2,1,4 }, { 3,1,4 },
            { 0,2,2 }, { 1,2,2 }, { 2,2,2 }, { 3,2,2 }, { 4,2,2 }, { 2,4,2 }, { 2,2,4 }, { 3,2,4 }, { 0,3,2 }, { 1,3,2 }, { 2,3,2 }, { 3,3,2 }, { 4,3,2 }, { 3,4,2 }, { 2,3,4 }, { 3,3,4 },
            { 0,0,3 }, { 1,0,3 }, { 2,0,3 }, { 3,0,3 }, { 4,0,3 }, { 0,4,3 }, { 0,0,4 }, { 1,0,4 }, { 0,1,3 }, { 1,1,3 }, { 2,1,3 }, { 3,1,3 }, { 4,1,3 }, { 1,4,3 }, { 0,1,4 }, { 1,1,4 },
            { 0,2,3 }, { 1,2,3 }, { 2,2,3 }, { 3,2,3 }, { 4,2,3 }, { 2,4,3 }, { 0,2,4 }, { 1,2,4 }, { 0,3,3 }, { 1,3,3 }, { 2,3,3 }, { 3,3,3 }, { 4,3,3 }, { 3,4,3 }, { 0,3,4 }, { 1,3,4 }
        };

        const u32 (& quints)[3] = quintsFromQ[Q];

        for (int i = 0; i < numValues; i++)
        {
            dst[i].m	= m[i];
            dst[i].tq	= quints[i];
            dst[i].v	= (quints[i] << numBits) + m[i];
        }
    }

    inline void decodeISEBitBlock (ISEDecodedResult* dst, BitAccessStream& data, int numBits)
    {
        dst[0].m = data.getNext(numBits);
        dst[0].v = dst[0].m;
    }

    void decodeISE (ISEDecodedResult* dst, int numValues, BitAccessStream& data, const ISEParams& params)
    {
        if (params.mode == ISEMODE_TRIT)
        {
            const int numBlocks = divRoundUp(numValues, 5);
            for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
            {
                const int numValuesInBlock = blockNdx == numBlocks-1 ? numValues - 5*(numBlocks-1) : 5;
                decodeISETritBlock(&dst[5*blockNdx], numValuesInBlock, data, params.numBits);
            }
        }
        else if (params.mode == ISEMODE_QUINT)
        {
            const int numBlocks = divRoundUp(numValues, 3);
            for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
            {
                const int numValuesInBlock = blockNdx == numBlocks-1 ? numValues - 3*(numBlocks-1) : 3;
                decodeISEQuintBlock(&dst[3*blockNdx], numValuesInBlock, data, params.numBits);
            }
        }
        else
        {
            for (int i = 0; i < numValues; i++)
                decodeISEBitBlock(&dst[i], data, params.numBits);
        }
    }
    
    ISEParams computeMaximumRangeISEParams (int numAvailableBits, int numValuesInSequence)
    {
        int curBitsForTritMode		= 6;
        int curBitsForQuintMode		= 5;
        int curBitsForPlainBitMode	= 8;
        
        for (;;)
        {
            const int tritRange			= curBitsForTritMode > 0		? (3 << curBitsForTritMode) - 1			: -1;
            const int quintRange		= curBitsForQuintMode > 0		? (5 << curBitsForQuintMode) - 1		: -1;
            const int plainBitRange		= curBitsForPlainBitMode > 0	? (1 << curBitsForPlainBitMode) - 1		: -1;
            const int maxRange			= std::max(std::max(tritRange, quintRange), plainBitRange);
            
            if (maxRange == tritRange)
            {
                const ISEParams params(ISEMODE_TRIT, curBitsForTritMode);
                if (computeNumRequiredBits(params, numValuesInSequence) <= numAvailableBits)
                    return ISEParams(ISEMODE_TRIT, curBitsForTritMode);
                curBitsForTritMode--;
            }
            else if (maxRange == quintRange)
            {
                const ISEParams params(ISEMODE_QUINT, curBitsForQuintMode);
                if (computeNumRequiredBits(params, numValuesInSequence) <= numAvailableBits)
                    return ISEParams(ISEMODE_QUINT, curBitsForQuintMode);
                curBitsForQuintMode--;
            }
            else
            {
                const ISEParams params(ISEMODE_PLAIN_BIT, curBitsForPlainBitMode);
                if (computeNumRequiredBits(params, numValuesInSequence) <= numAvailableBits)
                    return ISEParams(ISEMODE_PLAIN_BIT, curBitsForPlainBitMode);
                curBitsForPlainBitMode--;
            }
        }
    }
    
    void unquantizeColorEndpoints (u32* dst, const ISEDecodedResult* iseResults, int numEndpoints, const ISEParams& iseParams)
    {
        if (iseParams.mode == ISEMODE_TRIT || iseParams.mode == ISEMODE_QUINT)
        {
            const int rangeCase			= iseParams.numBits*2 - (iseParams.mode == ISEMODE_TRIT ? 2 : 1);
            static const u32	Ca[11]	= { 204, 113, 93, 54, 44, 26, 22, 13, 11, 6, 5 };
            const u32			C		= Ca[rangeCase];
            
            for (int endpointNdx = 0; endpointNdx < numEndpoints; endpointNdx++)
            {
                const u32 a = getBit(iseResults[endpointNdx].m, 0);
                const u32 b = getBit(iseResults[endpointNdx].m, 1);
                const u32 c = getBit(iseResults[endpointNdx].m, 2);
                const u32 d = getBit(iseResults[endpointNdx].m, 3);
                const u32 e = getBit(iseResults[endpointNdx].m, 4);
                const u32 f = getBit(iseResults[endpointNdx].m, 5);
                
                const u32 A = a == 0 ? 0 : (1<<9)-1;
                const u32 B = rangeCase == 0	? 0
                : rangeCase == 1	? 0
                : rangeCase == 2	? (b << 8) |									(b << 4) |				(b << 2) |	(b << 1)
                : rangeCase == 3	? (b << 8) |												(b << 3) |	(b << 2)
                : rangeCase == 4	? (c << 8) | (b << 7) |										(c << 3) |	(b << 2) |	(c << 1) |	(b << 0)
                : rangeCase == 5	? (c << 8) | (b << 7) |													(c << 2) |	(b << 1) |	(c << 0)
                : rangeCase == 6	? (d << 8) | (c << 7) | (b << 6) |										(d << 2) |	(c << 1) |	(b << 0)
                : rangeCase == 7	? (d << 8) | (c << 7) | (b << 6) |													(d << 1) |	(c << 0)
                : rangeCase == 8	? (e << 8) | (d << 7) | (c << 6) | (b << 5) |										(e << 1) |	(d << 0)
                : rangeCase == 9	? (e << 8) | (d << 7) | (c << 6) | (b << 5) |													(e << 0)
                : rangeCase == 10	? (f << 8) | (e << 7) | (d << 6) | (c << 5) |	(b << 4) |										(f << 0)
                : (u32)-1;
                
                dst[endpointNdx] = (((iseResults[endpointNdx].tq*C + B) ^ A) >> 2) | (A & 0x80);
            }
        }
        else
        {
            for (int endpointNdx = 0; endpointNdx < numEndpoints; endpointNdx++)
                dst[endpointNdx] = bitReplicationScale(iseResults[endpointNdx].v, iseParams.numBits, 8);
        }
    }
    
    inline void bitTransferSigned (s32& a, s32& b)
    {
        b >>= 1;
        b |= a & 0x80;
        a >>= 1;
        a &= 0x3f;
        if (isBitSet(a, 5))
            a -= 0x40;
    }

    inline uint32x4 clampedRGBA(int32x4 rgba)
    {
        // TODO: optimize
        rgba = clamp(rgba, int32x4(0), int32x4(255));
        return uint32x4(rgba.x, rgba.y, rgba.z, rgba.w);
    }

    inline int32x4 blueContract (int r, int g, int b, int a)
    {
        return int32x4((r+b)>>1, (g+b)>>1, b, a);
    }
    
    inline bool isColorEndpointModeHDR (u32 mode)
    {
        return mode == 2	||
               mode == 3	||
               mode == 7	||
               mode == 11	||
               mode == 14	||
               mode == 15;
    }

    void decodeHDREndpointMode7 (uint32x4& e0, uint32x4& e1, u32 v0, u32 v1, u32 v2, u32 v3)
    {
        const u32 m10		= getBit(v1, 7) | (getBit(v2, 7) << 1);
        const u32 m23		= getBits(v0, 6, 7);
        const u32 majComp	= m10 != 3	? m10
            : m23 != 3	? m23
            :			  0;
        const u32 mode		= m10 != 3	? m23
            : m23 != 3	? 4
            :			  5;
        
        s32			red		= (s32)getBits(v0, 0, 5);
        s32			green	= (s32)getBits(v1, 0, 4);
        s32			blue	= (s32)getBits(v2, 0, 4);
        s32			scale	= (s32)getBits(v3, 0, 4);

        {
#define SHOR(DST_VAR, SHIFT, BIT_VAR) (DST_VAR) |= (BIT_VAR) << (SHIFT)
#define ASSIGN_X_BITS(V0,S0, V1,S1, V2,S2, V3,S3, V4,S4, V5,S5, V6,S6) { SHOR(V0,S0,x0); SHOR(V1,S1,x1); SHOR(V2,S2,x2); SHOR(V3,S3,x3); SHOR(V4,S4,x4); SHOR(V5,S5,x5); SHOR(V6,S6,x6); }
            
            const u32	x0	= getBit(v1, 6);
            const u32	x1	= getBit(v1, 5);
            const u32	x2	= getBit(v2, 6);
            const u32	x3	= getBit(v2, 5);
            const u32	x4	= getBit(v3, 7);
            const u32	x5	= getBit(v3, 6);
            const u32	x6	= getBit(v3, 5);
            
            s32&		R	= red;
            s32&		G	= green;
            s32&		B	= blue;
            s32&		S	= scale;
            
            switch (mode)
            {
                case 0: ASSIGN_X_BITS(R,9,  R,8,  R,7,  R,10,  R,6,  S,6,   S,5); break;
                case 1: ASSIGN_X_BITS(R,8,  G,5,  R,7,  B,5,   R,6,  R,10,  R,9); break;
                case 2: ASSIGN_X_BITS(R,9,  R,8,  R,7,  R,6,   S,7,  S,6,   S,5); break;
                case 3: ASSIGN_X_BITS(R,8,  G,5,  R,7,  B,5,   R,6,  S,6,   S,5); break;
                case 4: ASSIGN_X_BITS(G,6,  G,5,  B,6,  B,5,   R,6,  R,7,   S,5); break;
                case 5: ASSIGN_X_BITS(G,6,  G,5,  B,6,  B,5,   R,6,  S,6,   S,5); break;
                default:
                    break;
            }
            
#undef ASSIGN_X_BITS
#undef SHOR
        }
        
        static const int shiftAmounts[] = { 1, 1, 2, 3, 4, 5 };
        
        red		<<= shiftAmounts[mode];
        green	<<= shiftAmounts[mode];
        blue	<<= shiftAmounts[mode];
        scale	<<= shiftAmounts[mode];
        
        if (mode != 5)
        {
            green = red - green;
            blue  = red - blue;
        }
        
        if (majComp == 1)
            std::swap(red, green);
        else if (majComp == 2)
            std::swap(red, blue);

        const uint32x4 limit0(0);
        const uint32x4 limit1(0xfff);
        e0 = uint32x4(red - scale, green - scale, blue - scale, 0x780);
        e1 = uint32x4(red, green, blue, 0x780);
        e0 = clamp(e0, limit0, limit1);
        e1 = clamp(e1, limit0, limit1);
    }
    
    void decodeHDREndpointMode11 (uint32x4& e0, uint32x4& e1, u32 v0, u32 v1, u32 v2, u32 v3, u32 v4, u32 v5)
    {
        const u32 major = (getBit(v5, 7) << 1) | getBit(v4, 7);

        if (major == 3)
        {
            e0 = uint32x4(v0<<4, v2<<4, getBits(v4,0,6)<<5, 0x780);
            e1 = uint32x4(v1<<4, v3<<4, getBits(v5,0,6)<<5, 0x780);
        }
        else
        {
            const u32 mode = (getBit(v3, 7) << 2) | (getBit(v2, 7) << 1) | getBit(v1, 7);

            s32 a  = (s32)((getBit(v1, 6) << 8) | v0);
            s32 c  = (s32)(getBits(v1, 0, 5));
            s32 b0 = (s32)(getBits(v2, 0, 5));
            s32 b1 = (s32)(getBits(v3, 0, 5));
            s32 d0 = (s32)(getBits(v4, 0, 4));
            s32 d1 = (s32)(getBits(v5, 0, 4));

            {
#define SHOR(DST_VAR, SHIFT, BIT_VAR) (DST_VAR) |= (BIT_VAR) << (SHIFT)
#define ASSIGN_X_BITS(V0,S0, V1,S1, V2,S2, V3,S3, V4,S4, V5,S5) { SHOR(V0,S0,x0); SHOR(V1,S1,x1); SHOR(V2,S2,x2); SHOR(V3,S3,x3); SHOR(V4,S4,x4); SHOR(V5,S5,x5); }
                
                const u32 x0 = getBit(v2, 6);
                const u32 x1 = getBit(v3, 6);
                const u32 x2 = getBit(v4, 6);
                const u32 x3 = getBit(v5, 6);
                const u32 x4 = getBit(v4, 5);
                const u32 x5 = getBit(v5, 5);
                
                switch (mode)
                {
                    case 0: ASSIGN_X_BITS(b0,6,  b1,6,   d0,6,  d1,6,  d0,5,  d1,5); break;
                    case 1: ASSIGN_X_BITS(b0,6,  b1,6,   b0,7,  b1,7,  d0,5,  d1,5); break;
                    case 2: ASSIGN_X_BITS(a,9,   c,6,    d0,6,  d1,6,  d0,5,  d1,5); break;
                    case 3: ASSIGN_X_BITS(b0,6,  b1,6,   a,9,   c,6,   d0,5,  d1,5); break;
                    case 4: ASSIGN_X_BITS(b0,6,  b1,6,   b0,7,  b1,7,  a,9,   a,10); break;
                    case 5: ASSIGN_X_BITS(a,9,   a,10,   c,7,   c,6,   d0,5,  d1,5); break;
                    case 6: ASSIGN_X_BITS(b0,6,  b1,6,   a,11,  c,6,   a,9,   a,10); break;
                    case 7: ASSIGN_X_BITS(a,9,   a,10,   a,11,  c,6,   d0,5,  d1,5); break;
                    default:
                        break;
                }
                
#undef ASSIGN_X_BITS
#undef SHOR
            }

            static const int numDBits[] = { 7, 6, 7, 6, 5, 6, 5, 6 };

            d0 = signExtend(d0, numDBits[mode]);
            d1 = signExtend(d1, numDBits[mode]);

            const int shiftAmount = (mode >> 1) ^ 3;
            a	<<= shiftAmount;
            c	<<= shiftAmount;
            b0	<<= shiftAmount;
            b1	<<= shiftAmount;
            d0	<<= shiftAmount;
            d1	<<= shiftAmount;

            const uint32x4 limit0(0);
            const uint32x4 limit1(0xfff);
            e0 = uint32x4(a - c, a - b0 - c - d0, a - b1 - c - d1, 0x780);
            e1 = uint32x4(a, a - b0, a - b1, 0x780);
            e0 = clamp(e0, limit0, limit1);
            e1 = clamp(e1, limit0, limit1);

            if (major == 1) {
                e0 = e0.yxzw;
                e1 = e1.yxzw;
            }
            else if (major == 2) {
                e0 = e0.zyxw;
                e1 = e1.zyxw;
            }
        }
    }

    void decodeHDREndpointMode15(uint32x4& e0, uint32x4& e1, u32 v0, u32 v1, u32 v2, u32 v3, u32 v4, u32 v5, u32 v6In, u32 v7In)
    {
        decodeHDREndpointMode11(e0, e1, v0, v1, v2, v3, v4, v5);

        const u32	mode	= (getBit(v7In, 7) << 1) | getBit(v6In, 7);
        s32			v6		= (s32)getBits(v6In, 0, 6);
        s32			v7		= (s32)getBits(v7In, 0, 6);

        if (mode == 3)
        {
            e0.w = v6 << 5;
            e1.w = v7 << 5;
        }
        else
        {
            v6 |= (v7 << (mode+1)) & 0x780;
            v7 &= (0x3f >> mode);
            v7 ^= 0x20 >> mode;
            v7 -= 0x20 >> mode;
            v6 <<= 4-mode;
            v7 <<= 4-mode;

            v7 += v6;
            v7 = clamp(v7, 0, 0xfff);
            e0.w = v6;
            e1.w = v7;
        }
    }

    void decodeColorEndpoints (ColorEndpointPair* dst, const u32* unquantizedEndpoints, const u32* endpointModes, int numPartitions)
    {
        int unquantizedNdx = 0;
        
        for (int partitionNdx = 0; partitionNdx < numPartitions; partitionNdx++)
        {
            const u32		endpointMode	= endpointModes[partitionNdx];
            const u32*		v				= &unquantizedEndpoints[unquantizedNdx];
            uint32x4&		e0				= dst[partitionNdx].e0;
            uint32x4&		e1				= dst[partitionNdx].e1;

            unquantizedNdx += computeNumColorEndpointValues(endpointMode);
            
            switch (endpointMode)
            {
                case 0:
                    e0 = uint32x4(v[0], v[0], v[0], 0xff);
                    e1 = uint32x4(v[1], v[1], v[1], 0xff);
                    break;
                    
                case 1:
                {
                    const u32 L0 = (v[0] >> 2) | (getBits(v[1], 6, 7) << 6);
                    const u32 L1 = std::min(0xffu, L0 + getBits(v[1], 0, 5));
                    e0 = uint32x4(L0, L0, L0, 0xff);
                    e1 = uint32x4(L1, L1, L1, 0xff);
                    break;
                }
                    
                case 2:
                {
                    const u32 v1Gr		= v[1] >= v[0];
                    const u32 y0		= v1Gr ? v[0]<<4 : (v[1]<<4) + 8;
                    const u32 y1		= v1Gr ? v[1]<<4 : (v[0]<<4) - 8;
                    
                    e0 = uint32x4(y0, y0, y0, 0x780);
                    e1 = uint32x4(y1, y1, y1, 0x780);
                    break;
                }
                    
                case 3:
                {
                    const bool		m	= isBitSet(v[0], 7);
                    const u32	y0	= m ? (getBits(v[1], 5, 7) << 9) | (getBits(v[0], 0, 6) << 2)
                    : (getBits(v[1], 4, 7) << 8) | (getBits(v[0], 0, 6) << 1);
                    const u32	d	= m ? getBits(v[1], 0, 4) << 2
                    : getBits(v[1], 0, 3) << 1;
                    const u32	y1	= std::min(0xfffu, y0+d);
                    
                    e0 = uint32x4(y0, y0, y0, 0x780);
                    e1 = uint32x4(y1, y1, y1, 0x780);
                    break;
                }
                    
                case 4:
                    e0 = uint32x4(v[0], v[0], v[0], v[2]);
                    e1 = uint32x4(v[1], v[1], v[1], v[3]);
                    break;
                    
                case 5:
                {
                    s32 v0 = (s32)v[0];
                    s32 v1 = (s32)v[1];
                    s32 v2 = (s32)v[2];
                    s32 v3 = (s32)v[3];
                    bitTransferSigned(v1, v0);
                    bitTransferSigned(v3, v2);
                    
                    e0 = clampedRGBA(int32x4(v0, v0, v0, v2));
                    e1 = clampedRGBA(int32x4(v0+v1, v0+v1, v0+v1, v2+v3));
                    break;
                }
                    
                case 6:
                    e0 = uint32x4((v[0]*v[3]) >> 8,	(v[1]*v[3]) >> 8,	(v[2]*v[3]) >> 8,	0xff);
                    e1 = uint32x4(v[0],				v[1],				v[2],				0xff);
                    break;
                    
                case 7:
                    decodeHDREndpointMode7(e0, e1, v[0], v[1], v[2], v[3]);
                    break;
                    
                case 8:
                    if (v[1]+v[3]+v[5] >= v[0]+v[2]+v[4])
                    {
                        e0 = uint32x4(v[0], v[2], v[4], 0xff);
                        e1 = uint32x4(v[1], v[3], v[5], 0xff);
                    }
                    else
                    {
                        int32x4 temp0 = blueContract(v[1], v[3], v[5], 0xff);
                        int32x4 temp1 = blueContract(v[0], v[2], v[4], 0xff);
                        e0 = uint32x4(temp0.x, temp0.y, temp0.z, temp0.w);
                        e1 = uint32x4(temp1.x, temp1.y, temp1.z, temp1.w);
                    }
                    break;

                case 9:
                {
                    s32 v0 = (s32)v[0];
                    s32 v1 = (s32)v[1];
                    s32 v2 = (s32)v[2];
                    s32 v3 = (s32)v[3];
                    s32 v4 = (s32)v[4];
                    s32 v5 = (s32)v[5];
                    bitTransferSigned(v1, v0);
                    bitTransferSigned(v3, v2);
                    bitTransferSigned(v5, v4);
                    
                    if (v1+v3+v5 >= 0)
                    {
                        e0 = clampedRGBA(int32x4(v0,		v2,		v4,		0xff));
                        e1 = clampedRGBA(int32x4(v0+v1,	v2+v3,	v4+v5,	0xff));
                    }
                    else
                    {
                        e0 = clampedRGBA(blueContract(v0+v1,	v2+v3,	v4+v5,	0xff));
                        e1 = clampedRGBA(blueContract(v0,		v2,		v4,		0xff));
                    }
                    break;
                }
                    
                case 10:
                    e0 = uint32x4((v[0]*v[3]) >> 8,	(v[1]*v[3]) >> 8,	(v[2]*v[3]) >> 8,	v[4]);
                    e1 = uint32x4(v[0],				v[1],				v[2],				v[5]);
                    break;
                    
                case 11:
                    decodeHDREndpointMode11(e0, e1, v[0], v[1], v[2], v[3], v[4], v[5]);
                    break;
                    
                case 12:
                    if (v[1]+v[3]+v[5] >= v[0]+v[2]+v[4])
                    {
                        e0 = uint32x4(v[0], v[2], v[4], v[6]);
                        e1 = uint32x4(v[1], v[3], v[5], v[7]);
                    }
                    else
                    {
                        e0 = clampedRGBA(blueContract(v[1], v[3], v[5], v[7]));
                        e1 = clampedRGBA(blueContract(v[0], v[2], v[4], v[6]));
                    }
                    break;
                    
                case 13:
                {
                    s32 v0 = (s32)v[0];
                    s32 v1 = (s32)v[1];
                    s32 v2 = (s32)v[2];
                    s32 v3 = (s32)v[3];
                    s32 v4 = (s32)v[4];
                    s32 v5 = (s32)v[5];
                    s32 v6 = (s32)v[6];
                    s32 v7 = (s32)v[7];
                    bitTransferSigned(v1, v0);
                    bitTransferSigned(v3, v2);
                    bitTransferSigned(v5, v4);
                    bitTransferSigned(v7, v6);

                    if (v1+v3+v5 >= 0)
                    {
                        e0 = clampedRGBA(int32x4(v0,		v2,		v4,		v6));
                        e1 = clampedRGBA(int32x4(v0+v1,	v2+v3,	v4+v5,	v6+v7));
                    }
                    else
                    {
                        e0 = clampedRGBA(blueContract(v0+v1,	v2+v3,	v4+v5,	v6+v7));
                        e1 = clampedRGBA(blueContract(v0,		v2,		v4,		v6));
                    }
                    
                    break;
                }
                    
                case 14:
                    decodeHDREndpointMode11(e0, e1, v[0], v[1], v[2], v[3], v[4], v[5]);
                    e0.w = v[6];
                    e1.w = v[7];
                    break;
                    
                case 15:
                    decodeHDREndpointMode15(e0, e1, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    void computeColorEndpoints (ColorEndpointPair* dst, const Block128& blockData, const u32* endpointModes, int numPartitions, int numColorEndpointValues, const ISEParams& iseParams, int numBitsAvailable)
    {
        const int			colorEndpointDataStart = numPartitions == 1 ? 17 : 29;
        ISEDecodedResult	colorEndpointData[18];

        {
            BitAccessStream dataStream(blockData, colorEndpointDataStart, numBitsAvailable, true);
            decodeISE(&colorEndpointData[0], numColorEndpointValues, dataStream, iseParams);
        }

        {
            u32 unquantizedEndpoints[18];
            unquantizeColorEndpoints(&unquantizedEndpoints[0], &colorEndpointData[0], numColorEndpointValues, iseParams);
            decodeColorEndpoints(dst, &unquantizedEndpoints[0], &endpointModes[0], numPartitions);
        }
    }

    void unquantizeWeights (u32* dst, const ISEDecodedResult* weightGrid, const ASTCBlockMode& blockMode)
    {
        const int			numWeights	= computeNumWeights(blockMode);
        const ISEParams&	iseParams	= blockMode.weightISEParams;

        if (iseParams.mode == ISEMODE_TRIT || iseParams.mode == ISEMODE_QUINT)
        {
            const int rangeCase = iseParams.numBits*2 + (iseParams.mode == ISEMODE_QUINT ? 1 : 0);
            
            if (rangeCase == 0 || rangeCase == 1)
            {
                static const u32 map0[3]	= { 0, 32, 63 };
                static const u32 map1[5]	= { 0, 16, 32, 47, 63 };
                const u32* const map		= rangeCase == 0 ? &map0[0] : &map1[0];
                for (int i = 0; i < numWeights; i++)
                {
                    dst[i] = map[weightGrid[i].v];
                }
            }
            else
            {
                static const u32	Ca[5]	= { 50, 28, 23, 13, 11 };
                const u32			C		= Ca[rangeCase-2];
                
                for (int weightNdx = 0; weightNdx < numWeights; weightNdx++)
                {
                    const u32 a = getBit(weightGrid[weightNdx].m, 0);
                    const u32 b = getBit(weightGrid[weightNdx].m, 1);
                    const u32 c = getBit(weightGrid[weightNdx].m, 2);
                    
                    const u32 A = a == 0 ? 0 : (1<<7)-1;
                    const u32 B = rangeCase == 2 ? 0
                    : rangeCase == 3 ? 0
                    : rangeCase == 4 ? (b << 6) |					(b << 2) |				(b << 0)
                    : rangeCase == 5 ? (b << 6) |								(b << 1)
                    : rangeCase == 6 ? (c << 6) | (b << 5) |					(c << 1) |	(b << 0)
                    : (u32)-1;
                    
                    dst[weightNdx] = (((weightGrid[weightNdx].tq*C + B) ^ A) >> 2) | (A & 0x20);
                }
            }
        }
        else
        {
            for (int weightNdx = 0; weightNdx < numWeights; weightNdx++)
                dst[weightNdx] = bitReplicationScale(weightGrid[weightNdx].v, iseParams.numBits, 6);
        }

        for (int weightNdx = 0; weightNdx < numWeights; weightNdx++)
            dst[weightNdx] += dst[weightNdx] > 32 ? 1 : 0;
    }
    
    void interpolateWeights (TexelWeightPair* dst, const u32* unquantizedWeights, int blockWidth, int blockHeight, const ASTCBlockMode& blockMode)
    {
        const int		numWeightsPerTexel	= blockMode.isDualPlane ? 2 : 1;
        const u32	scaleX				= (1024 + blockWidth/2) / (blockWidth-1);
        const u32	scaleY				= (1024 + blockHeight/2) / (blockHeight-1);
        
        for (int texelY = 0; texelY < blockHeight; texelY++)
        {
            for (int texelX = 0; texelX < blockWidth; texelX++)
            {
                const u32 gX	= (scaleX*texelX*(blockMode.weightGridWidth-1) + 32) >> 6;
                const u32 gY	= (scaleY*texelY*(blockMode.weightGridHeight-1) + 32) >> 6;
                const u32 jX	= gX >> 4;
                const u32 jY	= gY >> 4;
                const u32 fX	= gX & 0xf;
                const u32 fY	= gY & 0xf;
                const u32 w11	= (fX*fY + 8) >> 4;
                const u32 w10	= fY - w11;
                const u32 w01	= fX - w11;
                const u32 w00	= 16 - fX - fY + w11;
                const u32 v0	= jY*blockMode.weightGridWidth + jX;
                
                for (int texelWeightNdx = 0; texelWeightNdx < numWeightsPerTexel; texelWeightNdx++)
                {
                    const u32 p00	= unquantizedWeights[(v0)									* numWeightsPerTexel + texelWeightNdx];
                    const u32 p01	= unquantizedWeights[(v0 + 1)								* numWeightsPerTexel + texelWeightNdx];
                    const u32 p10	= unquantizedWeights[(v0 + blockMode.weightGridWidth)		* numWeightsPerTexel + texelWeightNdx];
                    const u32 p11	= unquantizedWeights[(v0 + blockMode.weightGridWidth + 1)	* numWeightsPerTexel + texelWeightNdx];
                    
                    dst[texelY*blockWidth + texelX].w[texelWeightNdx] = (p00*w00 + p01*w01 + p10*w10 + p11*w11 + 8) >> 4;
                }
            }
        }
    }
    
    void computeTexelWeights (TexelWeightPair* dst, const Block128& blockData, int blockWidth, int blockHeight, const ASTCBlockMode& blockMode)
    {
        ISEDecodedResult weightGrid[64];
        
        {
            BitAccessStream dataStream(blockData, 127, computeNumRequiredBits(blockMode.weightISEParams, computeNumWeights(blockMode)), false);
            decodeISE(&weightGrid[0], computeNumWeights(blockMode), dataStream, blockMode.weightISEParams);
        }
        
        {
            u32 unquantizedWeights[64];
            unquantizeWeights(&unquantizedWeights[0], &weightGrid[0], blockMode);
            interpolateWeights(dst, &unquantizedWeights[0], blockWidth, blockHeight, blockMode);
        }
    }
    
    inline u32 hash52 (u32 v)
    {
        u32 p = v;
        p ^= p >> 15;	p -= p << 17;	p += p << 7;	p += p << 4;
        p ^= p >>  5;	p += p << 16;	p ^= p >> 7;	p ^= p >> 3;
        p ^= p <<  6;	p ^= p >> 17;
        return p;
    }
    
    int computeTexelPartition (u32 seedIn, u32 xIn, u32 yIn, u32 zIn, int numPartitions, bool smallBlock)
    {
        const u32	x		= smallBlock ? xIn << 1 : xIn;
        const u32	y		= smallBlock ? yIn << 1 : yIn;
        const u32	z		= smallBlock ? zIn << 1 : zIn;
        const u32	seed	= seedIn + 1024*(numPartitions-1);
        const u32	rnum	= hash52(seed);
        u8			seed1	=  rnum							& 0xf;
        u8			seed2	= (rnum >>  4)					& 0xf;
        u8			seed3	= (rnum >>  8)					& 0xf;
        u8			seed4	= (rnum >> 12)					& 0xf;
        u8			seed5	= (rnum >> 16)					& 0xf;
        u8			seed6	= (rnum >> 20)					& 0xf;
        u8			seed7	= (rnum >> 24)					& 0xf;
        u8			seed8	= (rnum >> 28)					& 0xf;
        u8			seed9	= (rnum >> 18)					& 0xf;
        u8			seed10	= (rnum >> 22)					& 0xf;
        u8			seed11	= (rnum >> 26)					& 0xf;
        u8			seed12	= ((rnum >> 30) | (rnum << 2))	& 0xf;
        
        seed1 *= seed1;		seed5 *= seed5;		seed9  *= seed9;
        seed2 *= seed2;		seed6 *= seed6;		seed10 *= seed10;
        seed3 *= seed3;		seed7 *= seed7;		seed11 *= seed11;
        seed4 *= seed4;		seed8 *= seed8;		seed12 *= seed12;
        
        const int shA = (seed & 2) != 0		? 4		: 5;
        const int shB = numPartitions == 3	? 6		: 5;
        const int sh1 = (seed & 1) != 0		? shA	: shB;
        const int sh2 = (seed & 1) != 0		? shB	: shA;
        const int sh3 = (seed & 0x10) != 0	? sh1	: sh2;
        
        seed1 >>= sh1;		seed2  >>= sh2;		seed3  >>= sh1;		seed4  >>= sh2;
        seed5 >>= sh1;		seed6  >>= sh2;		seed7  >>= sh1;		seed8  >>= sh2;
        seed9 >>= sh3;		seed10 >>= sh3;		seed11 >>= sh3;		seed12 >>= sh3;
        
        const int a =						0x3f & (seed1*x + seed2*y + seed11*z + (rnum >> 14));
        const int b =						0x3f & (seed3*x + seed4*y + seed12*z + (rnum >> 10));
        const int c = numPartitions >= 3 ?	0x3f & (seed5*x + seed6*y + seed9*z  + (rnum >>  6))	: 0;
        const int d = numPartitions >= 4 ?	0x3f & (seed7*x + seed8*y + seed10*z + (rnum >>  2))	: 0;
        
        return a >= b && a >= c && a >= d	? 0
        : b >= c && b >= d				? 1
        : c >= d						? 2
        :								  3;
    }
    
    void setTexelColors (void* dst, ColorEndpointPair* colorEndpoints, TexelWeightPair* texelWeights, int ccs, u32 partitionIndexSeed,
                                int numPartitions, int blockWidth, int blockHeight, bool isSRGB, bool isLDRMode, const u32* colorEndpointModes)
    {
        const bool	smallBlock = blockWidth*blockHeight < 31;
        bool		isHDREndpoint[4];
        
        for (int i = 0; i < numPartitions; i++)
            isHDREndpoint[i] = isColorEndpointModeHDR(colorEndpointModes[i]);
        
        for (int texelY = 0; texelY < blockHeight; texelY++)
            for (int texelX = 0; texelX < blockWidth; texelX++)
            {
                const int				texelNdx			= texelY*blockWidth + texelX;
                const int				colorEndpointNdx	= numPartitions == 1 ? 0 : computeTexelPartition(partitionIndexSeed, texelX, texelY, 0, numPartitions, smallBlock);
                const uint32x4&			e0					= colorEndpoints[colorEndpointNdx].e0;
                const uint32x4&			e1					= colorEndpoints[colorEndpointNdx].e1;
                const TexelWeightPair&	weight				= texelWeights[texelNdx];
                
                if (isLDRMode && isHDREndpoint[colorEndpointNdx])
                {
                    if (isSRGB)
                    {
                        ((u8*)dst)[texelNdx*4 + 0] = 0xff;
                        ((u8*)dst)[texelNdx*4 + 1] = 0;
                        ((u8*)dst)[texelNdx*4 + 2] = 0xff;
                        ((u8*)dst)[texelNdx*4 + 3] = 0xff;
                    }
                    else
                    {
                        ((float16*)dst)[texelNdx*4 + 0] = float16(1.0f);
                        ((float16*)dst)[texelNdx*4 + 1] = float16(0.0f);
                        ((float16*)dst)[texelNdx*4 + 2] = float16(1.0f);
                        ((float16*)dst)[texelNdx*4 + 3] = float16(1.0f);
                    }
                }
                else
                {
                    for (int channelNdx = 0; channelNdx < 4; channelNdx++)
                    {
                        if (!isHDREndpoint[colorEndpointNdx] || (channelNdx == 3 && colorEndpointModes[colorEndpointNdx] == 14)) // \note Alpha for mode 14 is treated the same as LDR.
                        {
                            const u32 c0	= (e0[channelNdx] << 8) | (isSRGB ? 0x80 : e0[channelNdx]);
                            const u32 c1	= (e1[channelNdx] << 8) | (isSRGB ? 0x80 : e1[channelNdx]);
                            const u32 w	= weight.w[ccs == channelNdx ? 1 : 0];
                            const u32 c	= (c0*(64-w) + c1*w + 32) / 64;
                            
                            if (isSRGB)
                                ((u8*)dst)[texelNdx*4 + channelNdx] = (c & 0xff00) >> 8;
                            else
                                ((float16*)dst)[texelNdx*4 + channelNdx] = float16(c == 65535 ? 1.0f : float(c / 65536.0f));
                        }
                        else
                        {
                            const u32		c0	= e0[channelNdx] << 4;
                            const u32		c1	= e1[channelNdx] << 4;
                            const u32		w	= weight.w[ccs == channelNdx ? 1 : 0];
                            const u32		c	= (c0*(64-w) + c1*w + 32) / 64;
                            const u32		e	= getBits(c, 11, 15);
                            const u32		m	= getBits(c, 0, 10);
                            const u32		mt	= m < 512		? 3*m
                            : m >= 1536		? 5*m - 2048
                            :				  4*m - 512;

                            float16 cf;
                            cf.u = u16((e << 10) + (mt >> 3));
                            if (isFloat16InfOrNan(cf))
                                cf.u = 0x7bff;

                            ((float16*)dst)[texelNdx*4 + channelNdx] = cf;
                        }
                    }
                }
            }
    }

    void decompressASTCBlock (void* dst, const Block128& blockData, int blockWidth, int blockHeight, bool isSRGB, bool isLDR)
    {
        // Decode block mode.
        
        const ASTCBlockMode blockMode = getASTCBlockMode(blockData.getBits(0, 10));
        
        // Check for block mode errors.
        
        if (blockMode.isError)
        {
            setASTCErrorColorBlock(dst, blockWidth, blockHeight, isSRGB);
            return;
        }
        
        // Separate path for void-extent.
        
        if (blockMode.isVoidExtent)
        {
            decodeVoidExtentBlock(dst, blockData, blockWidth, blockHeight, isSRGB, isLDR);
            return;
        }
        
        // Compute weight grid values.
        
        const int numWeights			= computeNumWeights(blockMode);
        const int numWeightDataBits		= computeNumRequiredBits(blockMode.weightISEParams, numWeights);
        const int numPartitions			= (int)blockData.getBits(11, 12) + 1;
        
        // Check for errors in weight grid, partition and dual-plane parameters.
        
        if (numWeights > 64								||
            numWeightDataBits > 96						||
            numWeightDataBits < 24						||
            blockMode.weightGridWidth > blockWidth		||
            blockMode.weightGridHeight > blockHeight	||
            (numPartitions == 4 && blockMode.isDualPlane))
        {
            setASTCErrorColorBlock(dst, blockWidth, blockHeight, isSRGB);
            return;
        }

        // Compute number of bits available for color endpoint data.

        const bool	isSingleUniqueCem			= numPartitions == 1 || blockData.getBits(23, 24) == 0;
        const int	numConfigDataBits			= (numPartitions == 1 ? 17 : isSingleUniqueCem ? 29 : 25 + 3*numPartitions) +
                                                  (blockMode.isDualPlane ? 2 : 0);
        const int	numBitsForColorEndpoints	= 128 - numWeightDataBits - numConfigDataBits;
        const int	extraCemBitsStart			= 127 - numWeightDataBits - (isSingleUniqueCem		? -1
                                                                             : numPartitions == 4	? 7
                                                                             : numPartitions == 3	? 4
                                                                             : numPartitions == 2	? 1
                                                                             : 0);
        // Decode color endpoint modes.

        u32 colorEndpointModes[4];
        decodeColorEndpointModes(&colorEndpointModes[0], blockData, numPartitions, extraCemBitsStart);

        const int numColorEndpointValues = computeNumColorEndpointValues(colorEndpointModes, numPartitions);

        // Check for errors in color endpoint value count.

        if (numColorEndpointValues > 18 || numBitsForColorEndpoints < divRoundUp(13*numColorEndpointValues, 5))
        {
            setASTCErrorColorBlock(dst, blockWidth, blockHeight, isSRGB);
            return;
        }

        // Compute color endpoints.

        ColorEndpointPair colorEndpoints[4];
        computeColorEndpoints(&colorEndpoints[0], blockData, &colorEndpointModes[0], numPartitions, numColorEndpointValues,
                              computeMaximumRangeISEParams(numBitsForColorEndpoints, numColorEndpointValues), numBitsForColorEndpoints);

        // Compute texel weights.

        TexelWeightPair texelWeights[ASTC_MAX_BLOCK_WIDTH*ASTC_MAX_BLOCK_HEIGHT];
        computeTexelWeights(&texelWeights[0], blockData, blockWidth, blockHeight, blockMode);

        // Set texel colors.

        const int		ccs					= blockMode.isDualPlane ? (int)blockData.getBits(extraCemBitsStart-2, extraCemBitsStart-1) : -1;
        const u32	partitionIndexSeed		= numPartitions > 1 ? blockData.getBits(13, 22) : (u32)-1;

        setTexelColors(dst, &colorEndpoints[0], &texelWeights[0], ccs, partitionIndexSeed, numPartitions, blockWidth, blockHeight, isSRGB, isLDR, &colorEndpointModes[0]);
    }
    
} // namespace

namespace mango::image
{

    //
    // NOTE: 
    //
    // The isSRGB here simply means 8 bit UNORM target, whether it is or isn't interpreted as sRGB
    // by the client depends on the ASTC compression format being decoded.
    //
    // The isLDR is redundant and should reflect the value of isSRGB and exists only because the decoder
    // has flexibility our front-end doesn't require. Don't be confused about this. :D

    void decode_block_astc_srgb(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        const bool isSRGB = true;
        const bool isLDR = true;

        const int blockWidth = info.width;
        const int blockHeight = info.height;

        const Block128 blockData(input);

        u8 buffer[ASTC_MAX_BLOCK_WIDTH * ASTC_MAX_BLOCK_HEIGHT * 4];
        decompressASTCBlock(buffer, blockData, blockWidth, blockHeight, isSRGB, isLDR);

        for (int y = 0; y < blockHeight; ++y)
        {
            u8* dest = output + y * stride;
            u8* src = buffer + y * blockWidth * 4;

            for (int x = 0; x < blockWidth; ++x)
            {
                dest[0] = src[0];
                dest[1] = src[1];
                dest[2] = src[2];
                dest[3] = src[3];
                dest += 4;
                src += 4;
            }
        }
    }

    void decode_block_astc_fp16(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        const bool isSRGB = false;
        const bool isLDR = false;

        const int blockWidth = info.width;
        const int blockHeight = info.height;

        const Block128 blockData(input);

        float16 buffer[ASTC_MAX_BLOCK_WIDTH * ASTC_MAX_BLOCK_HEIGHT * 4];
        decompressASTCBlock(buffer, blockData, blockWidth, blockHeight, isSRGB, isLDR);

        for (int y = 0; y < blockHeight; ++y)
        {
            float16* dest = reinterpret_cast<float16*>(output + y * stride);
            float16* src = buffer + y * blockWidth * 4;

            for (int x = 0; x < blockWidth; ++x)
            {
                dest[0] = src[0];
                dest[1] = src[1];
                dest[2] = src[2];
                dest[3] = src[3];
                dest += 4;
                src += 4;
            }
        }
    }

} // namespace mango::image
