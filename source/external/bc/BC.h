//-------------------------------------------------------------------------------------
// BC.h
//
// Block-compression (BC) functionality
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
//-------------------------------------------------------------------------------------

#if defined(_MSC_VER) && (_MSC_VER > 1000)
#pragma once
#endif

#include <assert.h>
#include <algorithm>
#include <float.h>
#include <stdint.h>
#include "../../../include/mango/mango.hpp"

// Reference:
// https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#19.5.9%20BC4U:%202(+6%20Derived)%20Single%20Component%20UNORM%20Values

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

namespace DirectX
{
    using namespace mango;
    using namespace mango::math;

//-------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------

const uint16_t F16S_MASK    = 0x8000;   // f16 sign mask
const uint16_t F16EM_MASK   = 0x7fff;   // f16 exp & mantissa mask
const uint16_t F16MAX       = 0x7bff;   // MAXFLT bit pattern for float16

#define SIGN_EXTEND(x,nb) ((((x)&(1<<((nb)-1)))?((~0)<<(nb)):0)|(x))

// Because these are used in SAL annotations, they need to remain macros rather than const values
#define NUM_PIXELS_PER_BLOCK 16
#define BC6H_MAX_REGIONS 2
#define BC6H_MAX_INDICES 16
#define BC7_MAX_REGIONS 3
#define BC7_MAX_INDICES 16

const size_t BC6H_NUM_CHANNELS = 3;
const size_t BC6H_MAX_SHAPES = 32;

const size_t BC7_NUM_CHANNELS = 4;
const size_t BC7_MAX_SHAPES = 64;

const uint32_t BC67_WEIGHT_MAX = 64;
const uint32_t BC67_WEIGHT_SHIFT = 6;
const uint32_t BC67_WEIGHT_ROUND = 32;

extern const int g_aWeights2[4];
extern const int g_aWeights3[8];
extern const int g_aWeights4[16];

enum BC_FLAGS
{
    BC_FLAGS_NONE       = 0x0,
    BC_FLAGS_DITHER_RGB = 0x10000,  // Enables dithering for RGB colors for BC1-3
    BC_FLAGS_DITHER_A   = 0x20000,  // Enables dithering for Alpha channel for BC1-3
    BC_FLAGS_UNIFORM    = 0x40000,  // By default, uses perceptual weighting for BC1-3; this flag makes it a uniform weighting
};

//-------------------------------------------------------------------------------------
// Structures
//-------------------------------------------------------------------------------------
class HDRColorA;

class LDRColorA
{
public:
    uint8_t r, g, b, a;

    LDRColorA() {}
    LDRColorA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) : r(_r), g(_g), b(_b), a(_a) {}

    const uint8_t& operator [] (size_t uElement) const
    {
        switch(uElement)
        {
        case 0: return r;
        case 1: return g;
        case 2: return b;
        case 3: return a;
        default: assert(false); return r;
        }
    }

    uint8_t& operator [] (size_t uElement)
    {
        switch(uElement)
        {
        case 0: return r;
        case 1: return g;
        case 2: return b;
        case 3: return a;
        default: assert(false); return r;
        }
    }

    LDRColorA operator = (const HDRColorA& c);

    static void InterpolateRGB(const LDRColorA& c0, const LDRColorA& c1, size_t wc, size_t wcprec, LDRColorA& out)
    {
        const int* aWeights = nullptr;
        switch(wcprec)
        {
        case 2: aWeights = g_aWeights2; assert( wc < 4 ); break;
        case 3: aWeights = g_aWeights3; assert( wc < 8 ); break;
        case 4: aWeights = g_aWeights4; assert( wc < 16 ); break;
        default: assert(false); out.r = out.g = out.b = 0; return;
        }
        out.r = uint8_t((uint32_t(c0.r) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.r) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
        out.g = uint8_t((uint32_t(c0.g) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.g) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
        out.b = uint8_t((uint32_t(c0.b) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.b) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
    }

    static void InterpolateA(const LDRColorA& c0, const LDRColorA& c1, size_t wa, size_t waprec, LDRColorA& out)
    {
        const int* aWeights = nullptr;
        switch(waprec)
        {
        case 2: aWeights = g_aWeights2; assert( wa < 4 ); break;
        case 3: aWeights = g_aWeights3; assert( wa < 8 ); break;
        case 4: aWeights = g_aWeights4; assert( wa < 16 ); break;
        default: assert(false); out.a = 0; return;
        }
        out.a = uint8_t((uint32_t(c0.a) * uint32_t(BC67_WEIGHT_MAX - aWeights[wa]) + uint32_t(c1.a) * uint32_t(aWeights[wa]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
    }

    static void Interpolate(const LDRColorA& c0, const LDRColorA& c1, size_t wc, size_t wa, size_t wcprec, size_t waprec, LDRColorA& out)
    {
        InterpolateRGB(c0, c1, wc, wcprec, out);
        InterpolateA(c0, c1, wa, waprec, out);
    }
};

class HDRColorA
{
public:
    float r, g, b, a;

public:
    HDRColorA() {}
    HDRColorA(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
    HDRColorA(const HDRColorA& c) : r(c.r), g(c.g), b(c.b), a(c.a) {}
    HDRColorA(const LDRColorA& c)
    {
        r = float(c.r) * (1.0f/255.0f);
        g = float(c.g) * (1.0f/255.0f);
        b = float(c.b) * (1.0f/255.0f);
        a = float(c.a) * (1.0f/255.0f);
    }

    // binary operators
    HDRColorA operator + ( const HDRColorA& c ) const
    {
        return HDRColorA(r + c.r, g + c.g, b + c.b, a + c.a);
    }

    HDRColorA operator - ( const HDRColorA& c ) const
    {
        return HDRColorA(r - c.r, g - c.g, b - c.b, a - c.a);
    }

    HDRColorA operator * ( float f ) const
    {
        return HDRColorA(r * f, g * f, b * f, a * f);
    }

    HDRColorA operator / ( float f ) const
    {
        float fInv = 1.0f / f;
        return HDRColorA(r * fInv, g * fInv, b * fInv, a * fInv);
    }

    float operator * ( const HDRColorA& c ) const
    {
        return r * c.r + g * c.g + b * c.b + a * c.a;
    }

    // assignment operators
    HDRColorA& operator += ( const HDRColorA& c )
    {
        r += c.r;
        g += c.g;
        b += c.b;
        a += c.a;
        return *this;
    }

    HDRColorA& operator -= ( const HDRColorA& c )
    {
        r -= c.r;
        g -= c.g;
        b -= c.b;
        a -= c.a;
        return *this;
    }

    HDRColorA& operator *= ( float f )
    {
        r *= f;
        g *= f;
        b *= f;
        a *= f;
        return *this;
    }

    HDRColorA& operator /= ( float f )
    {
        float fInv = 1.0f / f;
        r *= fInv;
        g *= fInv;
        b *= fInv;
        a *= fInv;
        return *this;
    }

    HDRColorA& operator = (const HDRColorA& c)
    {
        r = c.r;
        g = c.g;
        b = c.b;
        a = c.a;
        return *this;
    }

    HDRColorA& operator = (const LDRColorA& c)
    {
        r = float(c.r);
        g = float(c.g);
        b = float(c.b);
        a = float(c.a);
        return *this;
    }

    HDRColorA& Clamp(float fMin, float fMax)
    {
        r = mango::math::clamp(r, fMin, fMax);
        g = mango::math::clamp(g, fMin, fMax);
        b = mango::math::clamp(b, fMin, fMax);
        a = mango::math::clamp(a, fMin, fMax);
        return *this;
    }

    LDRColorA ToLDRColorA() const
    {
        return LDRColorA(uint8_t(r + 0.01f), 
                         uint8_t(g + 0.01f), 
                         uint8_t(b + 0.01f), 
                         uint8_t(a + 0.01f));
    }
};

inline LDRColorA LDRColorA::operator = (const HDRColorA& c)
{
    LDRColorA ret;
    HDRColorA tmp(c);
    tmp = tmp.Clamp(0.0f, 1.0f) * 255.0f;
    ret.r = uint8_t(tmp.r + 0.001f);
    ret.g = uint8_t(tmp.g + 0.001f);
    ret.b = uint8_t(tmp.b + 0.001f);
    ret.a = uint8_t(tmp.a + 0.001f);
    return ret;
}

struct LDREndPntPair
{
    LDRColorA A;
    LDRColorA B;
};

struct HDREndPntPair
{
    HDRColorA A;
    HDRColorA B;
};

inline HDRColorA* HDRColorALerp(HDRColorA *pOut, const HDRColorA *pC1, const HDRColorA *pC2, float s)
{
    pOut->r = pC1->r + s * (pC2->r - pC1->r);
    pOut->g = pC1->g + s * (pC2->g - pC1->g);
    pOut->b = pC1->b + s * (pC2->b - pC1->b);
    pOut->a = pC1->a + s * (pC2->a - pC1->a);
    return pOut;
}

#pragma pack(push,1)
// BC1/DXT1 compression (4 bits per texel)
struct D3DX_BC1
{
    uint16_t    rgb[2]; // 565 colors
    uint32_t    bitmap; // 2bpp rgb bitmap
};

// BC2/DXT2/3 compression (8 bits per texel)
struct D3DX_BC2
{
    uint32_t    bitmap[2];  // 4bpp alpha bitmap
    D3DX_BC1    bc1;        // BC1 rgb data
};

// BC3/DXT4/5 compression (8 bits per texel)
struct D3DX_BC3
{
    uint8_t     alpha[2];   // alpha values
    uint8_t     bitmap[6];  // 3bpp alpha bitmap
    D3DX_BC1    bc1;        // BC1 rgb data
};
#pragma pack(pop)

class INTColor
{
public:
    int r, g, b;

public:
    INTColor() {}
    INTColor(int nr, int ng, int nb) {r = nr; g = ng; b = nb;}
    INTColor(const INTColor& c) {r = c.r; g = c.g; b = c.b;}

    INTColor& operator = ( const INTColor& c )
    {
        r = c.r;
        g = c.g;
        b = c.b;
        return *this;
    }

    INTColor operator - ( const INTColor& c ) const
    {
        return INTColor(r - c.r, g - c.g, b - c.b);
    }

    INTColor& operator += ( const INTColor& c )
    {
        r += c.r;
        g += c.g;
        b += c.b;
        return *this;
    }

    INTColor& operator -= ( const INTColor& c )
    {
        r -= c.r;
        g -= c.g;
        b -= c.b;
        return *this;
    }

    INTColor& operator &= ( const INTColor& c )
    {
        r &= c.r;
        g &= c.g;
        b &= c.b;
        return *this;
    }

    int& operator [] ( uint8_t i )
    {
        return ((int*) this)[i];
    }

    void Set(const HDRColorA& c, bool bSigned)
    {
        // TODO: vectorize
        float16 x = c.r;
        float16 y = c.g;
        float16 z = c.b;
        r = F16ToINT(x, bSigned);
        g = F16ToINT(y, bSigned);
        b = F16ToINT(z, bSigned);
    }

    INTColor& Clamp(int iMin, int iMax)
    {
        r = std::min<int>(iMax, std::max<int>(iMin, r));
        g = std::min<int>(iMax, std::max<int>(iMin, g));
        b = std::min<int>(iMax, std::max<int>(iMin, b));
        return *this;
    }

    INTColor& SignExtend(const LDRColorA& Prec)
    {
        r = SIGN_EXTEND(r, Prec.r);
        g = SIGN_EXTEND(g, Prec.g);
        b = SIGN_EXTEND(b, Prec.b);
        return *this;
    }

    void ToF16(float16 aF16[3], bool bSigned) const
    {
        aF16[0] = INT2F16(r, bSigned);
        aF16[1] = INT2F16(g, bSigned);
        aF16[2] = INT2F16(b, bSigned);
    }

private:
    static int F16ToINT(const float16& f, bool bSigned)
    {
        uint16_t input = *((const uint16_t*) &f);
        int out, s;
        if(bSigned)
        {
            s = input & F16S_MASK;
            input &= F16EM_MASK;
            if(input > F16MAX) out = F16MAX;
            else out = input;
            out = s ? -out : out;
        }
        else
        {
            if(input & F16S_MASK) out = 0;
            else out = input;
        }
        return out;
    }

    static float16 INT2F16(int input, bool bSigned)
    {
        float16 h;
        uint16_t out;
        if(bSigned)
        {
            int s = 0;
            if(input < 0)
            {
                s = F16S_MASK;
                input = -input;
            }
            out = uint16_t(s | input);
        }
        else
        {
            assert(input >= 0 && input <= F16MAX);
            out = (uint16_t) input;
        }

        *((uint16_t*) &h) = out;
        return h;
    }
};

struct INTEndPntPair
{
    INTColor A;
    INTColor B;
};

template< size_t SizeInBytes >
class CBits
{
public:
    uint8_t GetBit(size_t& uStartBit) const
    {
        assert(uStartBit < 128);
        size_t uIndex = uStartBit >> 3;
        uint8_t ret = (m_uBits[uIndex] >> (uStartBit - (uIndex << 3))) & 0x01;
        uStartBit++;
        return ret;
    }

    uint8_t GetBits(size_t& uStartBit, size_t uNumBits) const
    {
        if(uNumBits == 0) return 0;
        assert(uStartBit + uNumBits <= 128 && uNumBits <= 8);
        uint8_t ret;
        size_t uIndex = uStartBit >> 3;
        size_t uBase = uStartBit - (uIndex << 3);
        if(uBase + uNumBits > 8)
        {
            size_t uFirstIndexBits = 8 - uBase;
            size_t uNextIndexBits = uNumBits - uFirstIndexBits;
            ret = (m_uBits[uIndex] >> uBase) | ((m_uBits[uIndex+1] & ((1 << uNextIndexBits) - 1)) << uFirstIndexBits);
        }
        else
        {
            ret = (m_uBits[uIndex] >> uBase) & ((1 << uNumBits) - 1);
        }
        assert(ret < (1 << uNumBits));
        uStartBit += uNumBits;
        return ret;
    }

    void SetBit(size_t& uStartBit, uint8_t uValue)
    {
        assert(uStartBit < 128 && uValue < 2);
        size_t uIndex = uStartBit >> 3;
        size_t uBase = uStartBit - (uIndex << 3);
        m_uBits[uIndex] &= ~(1 << uBase);
        m_uBits[uIndex] |= uValue << uBase;
        uStartBit++;
    }

    void SetBits(size_t& uStartBit, size_t uNumBits, uint8_t uValue)
    {
        if(uNumBits == 0)
            return;
        assert(uStartBit + uNumBits <= 128 && uNumBits <= 8);
        assert(uValue < (1 << uNumBits));
        size_t uIndex = uStartBit >> 3;
        size_t uBase = uStartBit - (uIndex << 3);
        if(uBase + uNumBits > 8)
        {
            size_t uFirstIndexBits = 8 - uBase;
            size_t uNextIndexBits = uNumBits - uFirstIndexBits;
            m_uBits[uIndex] &= ~(((1 << uFirstIndexBits) - 1) << uBase);
            m_uBits[uIndex] |= uValue << uBase;
            m_uBits[uIndex+1] &= ~((1 << uNextIndexBits) - 1);
            m_uBits[uIndex+1] |= uValue >> uFirstIndexBits;
        }
        else
        {
            m_uBits[uIndex] &= ~(((1 << uNumBits) - 1) << uBase);
            m_uBits[uIndex] |= uValue << uBase;
        }
        uStartBit += uNumBits;
    }

private:
    uint8_t m_uBits[ SizeInBytes ];
};

// BC6H compression (16 bits per texel)
class D3DX_BC6H : private CBits< 16 >
{
public:
    void Decode(bool bSigned, u8* output, size_t stride) const;
    void Encode(bool bSigned, const HDRColorA* const pIn);

private:
    enum EField : uint8_t
    {
        NA, // N/A
        M,  // Mode
        D,  // Shape
        RW,
        RX,
        RY,
        RZ,
        GW,
        GX,
        GY,
        GZ,
        BW,
        BX,
        BY,
        BZ,
    };

    struct ModeDescriptor
    {
        EField m_eField;
        uint8_t   m_uBit;
    };

    struct ModeInfo
    {
        uint8_t uMode;
        uint8_t uPartitions;
        bool bTransformed;
        uint8_t uIndexPrec;
        LDRColorA RGBAPrec[BC6H_MAX_REGIONS][2];
    };

    class EncodeParams : protected NonCopyable
    {
    public:
        float fBestErr;
        const bool bSigned;
        uint8_t uMode;
        uint8_t uShape;
        const HDRColorA* const aHDRPixels;
        INTEndPntPair aUnqEndPts[BC6H_MAX_SHAPES][BC6H_MAX_REGIONS];
        INTColor aIPixels[NUM_PIXELS_PER_BLOCK];

        EncodeParams(const HDRColorA* const aOriginal, bool bSignedFormat) :
            fBestErr(FLT_MAX),
            bSigned(bSignedFormat),
            aHDRPixels(aOriginal)
        {
            for(size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
            {
                aIPixels[i].Set(aOriginal[i], bSigned);
            }
        }
    };

    static int Quantize(int iValue, int prec, bool bSigned);
    static int Unquantize(int comp, uint8_t uBitsPerComp, bool bSigned);
    static int FinishUnquantize(int comp, bool bSigned);

    static bool EndPointsFit(const EncodeParams* pEP, const INTEndPntPair aEndPts[]);

    void GeneratePaletteQuantized(const EncodeParams* pEP, const INTEndPntPair& endPts,
                                  INTColor aPalette[]) const;
    float MapColorsQuantized(const EncodeParams* pEP, const INTColor aColors[], size_t np, const INTEndPntPair &endPts) const;
    float PerturbOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, uint8_t ch,
                     const INTEndPntPair& oldEndPts, INTEndPntPair& newEndPts, float fOldErr, int do_b) const;
    void OptimizeOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, float aOrgErr,
                     const INTEndPntPair &aOrgEndPts, INTEndPntPair &aOptEndPts) const;
    void OptimizeEndPoints(const EncodeParams* pEP, const float aOrgErr[],
                           const INTEndPntPair aOrgEndPts[], INTEndPntPair aOptEndPts[]) const;
    static void SwapIndices(const EncodeParams* pEP, INTEndPntPair aEndPts[], size_t aIndices[]);
    void AssignIndices(const EncodeParams* pEP, const INTEndPntPair aEndPts[],
                        size_t aIndices[], float aTotErr[]) const;
    void QuantizeEndPts(const EncodeParams* pEP, INTEndPntPair* qQntEndPts) const;
    void EmitBlock(const EncodeParams* pEP, const INTEndPntPair aEndPts[],
                   const size_t aIndices[]);
    void Refine(EncodeParams* pEP);

    static void GeneratePaletteUnquantized(const EncodeParams* pEP, size_t uRegion, INTColor aPalette[]);
    float MapColors(const EncodeParams* pEP, size_t uRegion, size_t np, const size_t* auIndex) const;
    float RoughMSE(EncodeParams* pEP) const;

private:
    const static ModeDescriptor ms_aDesc[][82];
    const static ModeInfo ms_aInfo[];
    const static int ms_aModeToInfo[];
};

// BC67 compression (16b bits per texel)
class D3DX_BC7 : private CBits< 16 >
{
public:
    void Decode(u8* output, size_t stride) const;
    void Encode(const HDRColorA* const pIn);

private:
    struct ModeInfo
    {
        uint8_t uPartitions;
        uint8_t uPartitionBits;
        uint8_t uPBits;
        uint8_t uRotationBits;
        uint8_t uIndexModeBits;
        uint8_t uIndexPrec;
        uint8_t uIndexPrec2;
        LDRColorA RGBAPrec;
        LDRColorA RGBAPrecWithP;
    };

    class EncodeParams : protected NonCopyable
    {
    public:
        uint8_t uMode;
        LDREndPntPair aEndPts[BC7_MAX_SHAPES][BC7_MAX_REGIONS];
        LDRColorA aLDRPixels[NUM_PIXELS_PER_BLOCK];
        const HDRColorA* const aHDRPixels;

        EncodeParams(const HDRColorA* const aOriginal) : aHDRPixels(aOriginal) {}
    };

    static uint8_t Quantize(uint8_t comp, uint8_t uPrec)
    {
        assert(0 < uPrec && uPrec <= 8);
        uint8_t rnd = (uint8_t) std::min<uint16_t>(255, uint16_t(comp) + (1 << (7 - uPrec)));
        return rnd >> (8 - uPrec);
    }

    static LDRColorA Quantize(const LDRColorA& c, const LDRColorA& RGBAPrec)
    {
        LDRColorA q;
        q.r = Quantize(c.r, RGBAPrec.r);
        q.g = Quantize(c.g, RGBAPrec.g);
        q.b = Quantize(c.b, RGBAPrec.b);
        if(RGBAPrec.a)
            q.a = Quantize(c.a, RGBAPrec.a);
        else
            q.a = 255;
        return q;
    }

    static uint8_t Unquantize(uint8_t comp, size_t uPrec)
    {
        assert(0 < uPrec && uPrec <= 8);
        comp = comp << (8 - uPrec);
        return comp | (comp >> uPrec);
    }

    static LDRColorA Unquantize(const LDRColorA& c, const LDRColorA& RGBAPrec)
    {
        LDRColorA q;
        q.r = Unquantize(c.r, RGBAPrec.r);
        q.g = Unquantize(c.g, RGBAPrec.g);
        q.b = Unquantize(c.b, RGBAPrec.b);
        q.a = RGBAPrec.a > 0 ? Unquantize(c.a, RGBAPrec.a) : 255;
        return q;
    }

    void GeneratePaletteQuantized(const EncodeParams* pEP, size_t uIndexMode, const LDREndPntPair& endpts,
                                  LDRColorA aPalette[]) const;
    float PerturbOne(const EncodeParams* pEP, const LDRColorA colors[], size_t np, size_t uIndexMode,
                     size_t ch, const LDREndPntPair &old_endpts,
                     LDREndPntPair &new_endpts, float old_err, uint8_t do_b) const;
    void Exhaustive(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode,
                    size_t ch, float& fOrgErr, LDREndPntPair& optEndPt) const;
    void OptimizeOne(const EncodeParams* pEP, const LDRColorA colors[], size_t np, size_t uIndexMode,
                     float orig_err, const LDREndPntPair &orig_endpts, LDREndPntPair &opt_endpts) const;
    void OptimizeEndPoints(const EncodeParams* pEP, size_t uShape, size_t uIndexMode,
                           const float orig_err[],
                           const LDREndPntPair orig_endpts[],
                           LDREndPntPair opt_endpts[]) const;
    void AssignIndices(const EncodeParams* pEP, size_t uShape, size_t uIndexMode,
                       LDREndPntPair endpts[],
                       size_t aIndices[], size_t aIndices2[],
                       float afTotErr[]) const;
    void EmitBlock(const EncodeParams* pEP, size_t uShape, size_t uRotation, size_t uIndexMode,
                   const LDREndPntPair aEndPts[],
                   const size_t aIndex[],
                   const size_t aIndex2[]);
    float Refine(const EncodeParams* pEP, size_t uShape, size_t uRotation, size_t uIndexMode);

    float MapColors(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode,
                    const LDREndPntPair& endPts, float fMinErr) const;
    static float RoughMSE(EncodeParams* pEP, size_t uShape, size_t uIndexMode);

private:
    const static ModeInfo ms_aInfo[];
};

//-------------------------------------------------------------------------------------

template <bool bRange> void OptimizeAlpha(float *pX, float *pY, const float *pPoints, size_t cSteps)
{
    static const float pC6[] = { 5.0f/5.0f, 4.0f/5.0f, 3.0f/5.0f, 2.0f/5.0f, 1.0f/5.0f, 0.0f/5.0f };
    static const float pD6[] = { 0.0f/5.0f, 1.0f/5.0f, 2.0f/5.0f, 3.0f/5.0f, 4.0f/5.0f, 5.0f/5.0f };
    static const float pC8[] = { 7.0f/7.0f, 6.0f/7.0f, 5.0f/7.0f, 4.0f/7.0f, 3.0f/7.0f, 2.0f/7.0f, 1.0f/7.0f, 0.0f/7.0f };
    static const float pD8[] = { 0.0f/7.0f, 1.0f/7.0f, 2.0f/7.0f, 3.0f/7.0f, 4.0f/7.0f, 5.0f/7.0f, 6.0f/7.0f, 7.0f/7.0f };

    const float *pC = (6 == cSteps) ? pC6 : pC8;
    const float *pD = (6 == cSteps) ? pD6 : pD8;

    float MAX_VALUE = 1.0f;
    float MIN_VALUE = bRange ? -1.0f : 0.0f;

    // Find Min and Max points, as starting point
    float fX = MAX_VALUE;
    float fY = MIN_VALUE;

    if(8 == cSteps)
    {
        for(size_t iPoint = 0; iPoint < NUM_PIXELS_PER_BLOCK; iPoint++)
        {
            if(pPoints[iPoint] < fX)
                fX = pPoints[iPoint];

            if(pPoints[iPoint] > fY)
                fY = pPoints[iPoint];
        }
    }
    else
    {
        for(size_t iPoint = 0; iPoint < NUM_PIXELS_PER_BLOCK; iPoint++)
        {
            if(pPoints[iPoint] < fX && pPoints[iPoint] > MIN_VALUE)
                fX = pPoints[iPoint];

            if(pPoints[iPoint] > fY && pPoints[iPoint] < MAX_VALUE)
                fY = pPoints[iPoint];
        }

        if (fX == fY)
        {
            fY = MAX_VALUE;
        }
    }

    // Use Newton's Method to find local minima of sum-of-squares error.
    float fSteps = (float) (cSteps - 1);

    for(size_t iIteration = 0; iIteration < 8; iIteration++)
    {
        float fScale;

        if((fY - fX) < (1.0f / 256.0f))
            break;

        fScale = fSteps / (fY - fX);

        // Calculate new steps
        float pSteps[8];

        for(size_t iStep = 0; iStep < cSteps; iStep++)
            pSteps[iStep] = pC[iStep] * fX + pD[iStep] * fY;

        if(6 == cSteps)
        {
            pSteps[6] = MIN_VALUE;
            pSteps[7] = MAX_VALUE;
        }

        // Evaluate function, and derivatives
        float dX  = 0.0f;
        float dY  = 0.0f;
        float d2X = 0.0f;
        float d2Y = 0.0f;

        for(size_t iPoint = 0; iPoint < NUM_PIXELS_PER_BLOCK; iPoint++)
        {
            float fDot = (pPoints[iPoint] - fX) * fScale;

            size_t iStep;

            if(fDot <= 0.0f)
                iStep = ((6 == cSteps) && (pPoints[iPoint] <= fX * 0.5f)) ? 6 : 0;
            else if(fDot >= fSteps)
                iStep = ((6 == cSteps) && (pPoints[iPoint] >= (fY + 1.0f) * 0.5f)) ? 7 : (cSteps - 1);
            else
                iStep = static_cast<int32_t>(fDot + 0.5f);


            if(iStep < cSteps)
            {
                // D3DX had this computation backwards (pPoints[iPoint] - pSteps[iStep])
                // this fix improves RMS of the alpha component
                float fDiff = pSteps[iStep] - pPoints[iPoint];

                dX  += pC[iStep] * fDiff;
                d2X += pC[iStep] * pC[iStep];

                dY  += pD[iStep] * fDiff;
                d2Y += pD[iStep] * pD[iStep];
            }
        }

        // Move endpoints
        if(d2X > 0.0f)
            fX -= dX / d2X;

        if(d2Y > 0.0f)
            fY -= dY / d2Y;

        if(fX > fY)
        {
            float f = fX; fX = fY; fY = f;
        }

        if((dX * dX < (1.0f / 64.0f)) && (dY * dY < (1.0f / 64.0f)))
            break;
    }

    *pX = mango::math::clamp(fX, MIN_VALUE, MAX_VALUE);
    *pY = mango::math::clamp(fY, MIN_VALUE, MAX_VALUE);
}

}; // namespace DirectX
