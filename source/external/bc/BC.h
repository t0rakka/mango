//-------------------------------------------------------------------------------------
// BC.h
//
// Block-compression (BC) functionality
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
//-------------------------------------------------------------------------------------

#pragma once

#include <assert.h>
#include <algorithm>
#include <float.h>
#include <stdint.h>
#include "../../../include/mango/mango.hpp"

namespace DirectX
{
    using namespace mango;
    using namespace mango::math;

//-------------------------------------------------------------------------------------
// Macros
//-------------------------------------------------------------------------------------

// Because these are used in SAL annotations, they need to remain macros rather than const values
#define NUM_PIXELS_PER_BLOCK 16

//-------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------

    enum BC_FLAGS : uint32_t
    {
        BC_FLAGS_NONE = 0x0,

        BC_FLAGS_DITHER_RGB = 0x10000,
        // Enables dithering for RGB colors for BC1-3

        BC_FLAGS_DITHER_A = 0x20000,
        // Enables dithering for Alpha channel for BC1-3

        BC_FLAGS_UNIFORM = 0x40000,
        // By default, uses perceptual weighting for BC1-3; this flag makes it a uniform weighting

        BC_FLAGS_USE_3SUBSETS = 0x80000,
        // By default, BC7 skips mode 0 & 2; this flag adds those modes back

        BC_FLAGS_FORCE_BC7_MODE6 = 0x100000,
        // BC7 should only use mode 6; skip other modes
    };

    //-------------------------------------------------------------------------------------
    // Structures
    //-------------------------------------------------------------------------------------
    class LDRColorA;

    class HDRColorA
    {
    public:
        float r, g, b, a;

    public:
        HDRColorA() = default;
        HDRColorA(float r, float g, float b, float a) noexcept
            : r(r)
            , g(g)
            , b(b)
            , a(a)
        {
        }

        // binary operators
        HDRColorA operator + (const HDRColorA& c) const noexcept
        {
            return HDRColorA(r + c.r, g + c.g, b + c.b, a + c.a);
        }

        HDRColorA operator - (const HDRColorA& c) const noexcept
        {
            return HDRColorA(r - c.r, g - c.g, b - c.b, a - c.a);
        }

        HDRColorA operator * (float f) const noexcept
        {
            return HDRColorA(r * f, g * f, b * f, a * f);
        }

        HDRColorA operator / (float f) const noexcept
        {
            const float fInv = 1.0f / f;
            return HDRColorA(r * fInv, g * fInv, b * fInv, a * fInv);
        }

        float operator * (const HDRColorA& c) const noexcept
        {
            return r * c.r + g * c.g + b * c.b + a * c.a;
        }

        // assignment operators
        HDRColorA& operator += (const HDRColorA& c) noexcept
        {
            r += c.r;
            g += c.g;
            b += c.b;
            a += c.a;
            return *this;
        }

        HDRColorA& operator -= (const HDRColorA& c) noexcept
        {
            r -= c.r;
            g -= c.g;
            b -= c.b;
            a -= c.a;
            return *this;
        }

        HDRColorA& operator *= (float f) noexcept
        {
            r *= f;
            g *= f;
            b *= f;
            a *= f;
            return *this;
        }

        HDRColorA& operator /= (float f) noexcept
        {
            const float fInv = 1.0f / f;
            r *= fInv;
            g *= fInv;
            b *= fInv;
            a *= fInv;
            return *this;
        }

        HDRColorA& Clamp(float fMin, float fMax) noexcept
        {
            r = std::min<float>(fMax, std::max<float>(fMin, r));
            g = std::min<float>(fMax, std::max<float>(fMin, g));
            b = std::min<float>(fMax, std::max<float>(fMin, b));
            a = std::min<float>(fMax, std::max<float>(fMin, a));
            return *this;
        }

        HDRColorA(const LDRColorA& c) noexcept;
        HDRColorA& operator = (const LDRColorA& c) noexcept;
        LDRColorA ToLDRColorA() const noexcept;
    };

    static inline
    void HDRColorALerp(HDRColorA& pOut, const HDRColorA& pC1, const HDRColorA& pC2, float s) noexcept
    {
        pOut.r = pC1.r + s * (pC2.r - pC1.r);
        pOut.g = pC1.g + s * (pC2.g - pC1.g);
        pOut.b = pC1.b + s * (pC2.b - pC1.b);
        pOut.a = pC1.a + s * (pC2.a - pC1.a);
    }

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

//-------------------------------------------------------------------------------------
// Templates
//-------------------------------------------------------------------------------------

    template <bool bRange>
    void OptimizeAlpha(float *pX, float *pY, const float *pPoints, uint32_t cSteps) noexcept
    {
        static const float pC6[] = { 5.0f / 5.0f, 4.0f / 5.0f, 3.0f / 5.0f, 2.0f / 5.0f, 1.0f / 5.0f, 0.0f / 5.0f };
        static const float pD6[] = { 0.0f / 5.0f, 1.0f / 5.0f, 2.0f / 5.0f, 3.0f / 5.0f, 4.0f / 5.0f, 5.0f / 5.0f };
        static const float pC8[] = { 7.0f / 7.0f, 6.0f / 7.0f, 5.0f / 7.0f, 4.0f / 7.0f, 3.0f / 7.0f, 2.0f / 7.0f, 1.0f / 7.0f, 0.0f / 7.0f };
        static const float pD8[] = { 0.0f / 7.0f, 1.0f / 7.0f, 2.0f / 7.0f, 3.0f / 7.0f, 4.0f / 7.0f, 5.0f / 7.0f, 6.0f / 7.0f, 7.0f / 7.0f };

        const float *pC = (6 == cSteps) ? pC6 : pC8;
        const float *pD = (6 == cSteps) ? pD6 : pD8;

        constexpr float MAX_VALUE = 1.0f;
        constexpr float MIN_VALUE = (bRange) ? -1.0f : 0.0f;

        // Find Min and Max points, as starting point
        float fX = MAX_VALUE;
        float fY = MIN_VALUE;

        if (8 == cSteps)
        {
            for (size_t iPoint = 0; iPoint < NUM_PIXELS_PER_BLOCK; iPoint++)
            {
                if (pPoints[iPoint] < fX)
                    fX = pPoints[iPoint];

                if (pPoints[iPoint] > fY)
                    fY = pPoints[iPoint];
            }
        }
        else
        {
            for (size_t iPoint = 0; iPoint < NUM_PIXELS_PER_BLOCK; iPoint++)
            {
                if (pPoints[iPoint] < fX && pPoints[iPoint] > MIN_VALUE)
                    fX = pPoints[iPoint];

                if (pPoints[iPoint] > fY && pPoints[iPoint] < MAX_VALUE)
                    fY = pPoints[iPoint];
            }

            if (fX == fY)
            {
                fY = MAX_VALUE;
            }
        }

        // Use Newton's Method to find local minima of sum-of-squares error.
        auto const fSteps = static_cast<float>(cSteps - 1);

        for (size_t iIteration = 0; iIteration < 8; iIteration++)
        {
            if ((fY - fX) < (1.0f / 256.0f))
                break;

            float const fScale = fSteps / (fY - fX);

            // Calculate new steps
            float pSteps[8];

            for (size_t iStep = 0; iStep < cSteps; iStep++)
                pSteps[iStep] = pC[iStep] * fX + pD[iStep] * fY;

            if (6 == cSteps)
            {
                pSteps[6] = MIN_VALUE;
                pSteps[7] = MAX_VALUE;
            }

            // Evaluate function, and derivatives
            float dX = 0.0f;
            float dY = 0.0f;
            float d2X = 0.0f;
            float d2Y = 0.0f;

            for (size_t iPoint = 0; iPoint < NUM_PIXELS_PER_BLOCK; iPoint++)
            {
                const float fDot = (pPoints[iPoint] - fX) * fScale;

                uint32_t iStep;
                if (fDot <= 0.0f)
                {
                    // D3DX10 / D3DX11 didn't take into account the proper minimum value for the bRange (BC4S/BC5S) case
                    iStep = ((6 == cSteps) && (pPoints[iPoint] <= (fX + MIN_VALUE) * 0.5f)) ? 6u : 0u;
                }
                else if (fDot >= fSteps)
                {
                    iStep = ((6 == cSteps) && (pPoints[iPoint] >= (fY + MAX_VALUE) * 0.5f)) ? 7u : (cSteps - 1);
                }
                else
                {
                    iStep = uint32_t(fDot + 0.5f);
                }

                if (iStep < cSteps)
                {
                    // D3DX had this computation backwards (pPoints[iPoint] - pSteps[iStep])
                    // this fix improves RMS of the alpha component
                    const float fDiff = pSteps[iStep] - pPoints[iPoint];

                    dX += pC[iStep] * fDiff;
                    d2X += pC[iStep] * pC[iStep];

                    dY += pD[iStep] * fDiff;
                    d2Y += pD[iStep] * pD[iStep];
                }
            }

            // Move endpoints
            if (d2X > 0.0f)
                fX -= dX / d2X;

            if (d2Y > 0.0f)
                fY -= dY / d2Y;

            if (fX > fY)
            {
                const float f = fX; fX = fY; fY = f;
            }

            if ((dX * dX < (1.0f / 64.0f)) && (dY * dY < (1.0f / 64.0f)))
                break;
        }

        *pX = (fX < MIN_VALUE) ? MIN_VALUE : (fX > MAX_VALUE) ? MAX_VALUE : fX;
        *pY = (fY < MIN_VALUE) ? MIN_VALUE : (fY > MAX_VALUE) ? MAX_VALUE : fY;
    }

    static inline
    void unpack_block(HDRColorA* output, const u8* input, size_t stride)
    {
        float32x4* dest = reinterpret_cast<float32x4*>(output);

        const float32x4 scale(1.0f / 255.0f);

        for (int y = 0; y < 4; ++y)
        {
            uint32x4 color = uint32x4::uload(input);
            float32x4 r = convert<float32x4>((color >>  0) & 0xff) * scale;
            float32x4 g = convert<float32x4>((color >>  8) & 0xff) * scale;
            float32x4 b = convert<float32x4>((color >> 16) & 0xff) * scale;
            float32x4 a = convert<float32x4>((color >> 24) & 0xff) * scale;
            transpose(dest, r, g, b, a);

            input += stride;
            dest += 4;
        }
    }

} // namespace DirectX
