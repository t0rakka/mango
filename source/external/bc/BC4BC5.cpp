//-------------------------------------------------------------------------------------
// BC4BC5.cpp
//
// Block-compression (BC) functionality for BC4 and BC5 (DirectX 10 texture compression)
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
//-------------------------------------------------------------------------------------

#include "BC.h"

using namespace DirectX;

//------------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------------

// Because these are used in SAL annotations, they need to remain macros rather than const values
#define BLOCK_LEN 4
// length of each block in texel

#define BLOCK_SIZE (BLOCK_LEN * BLOCK_LEN)
// total texels in a 4x4 block.

namespace
{
    //------------------------------------------------------------------------------------
    // Structures
    //-------------------------------------------------------------------------------------

    // BC4U/BC5U
    struct BC4_UNORM
    {
        float R(size_t uOffset) const noexcept
        {
            const size_t uIndex = GetIndex(uOffset);
            return DecodeFromIndex(uIndex);
        }

        float DecodeFromIndex(size_t uIndex) const noexcept
        {
            if (uIndex == 0)
                return float(red_0) / 255.0f;
            if (uIndex == 1)
                return float(red_1) / 255.0f;
            const float fred_0 = float(red_0) / 255.0f;
            const float fred_1 = float(red_1) / 255.0f;
            if (red_0 > red_1)
            {
                uIndex -= 1;
                return (fred_0 * float(7u - uIndex) + fred_1 * float(uIndex)) / 7.0f;
            }
            else
            {
                if (uIndex == 6)
                    return 0.0f;
                if (uIndex == 7)
                    return 1.0f;
                uIndex -= 1;
                return (fred_0 * float(5u - uIndex) + fred_1 * float(uIndex)) / 5.0f;
            }
        }

        size_t GetIndex(size_t uOffset) const noexcept
        {
            return static_cast<size_t>((data >> (3 * uOffset + 16)) & 0x07);
        }

        void SetIndex(size_t uOffset, size_t uIndex) noexcept
        {
            data &= ~(uint64_t(0x07) << (3 * uOffset + 16));
            data |= (uint64_t(uIndex) << (3 * uOffset + 16));
        }

        union
        {
            struct
            {
                uint8_t red_0;
                uint8_t red_1;
                uint8_t indices[6];
            };
            uint64_t data;
        };
    };

    // BC4S/BC5S
    struct BC4_SNORM
    {
        float R(size_t uOffset) const noexcept
        {
            const size_t uIndex = GetIndex(uOffset);
            return DecodeFromIndex(uIndex);
        }

        float DecodeFromIndex(size_t uIndex) const noexcept
        {
            const int8_t sred_0 = (red_0 == -128) ? -127 : red_0;
            const int8_t sred_1 = (red_1 == -128) ? -127 : red_1;

            if (uIndex == 0)
                return float(sred_0) / 127.0f;
            if (uIndex == 1)
                return float(sred_1) / 127.0f;
            const float fred_0 = float(sred_0) / 127.0f;
            const float fred_1 = float(sred_1) / 127.0f;
            if (red_0 > red_1)
            {
                uIndex -= 1;
                return (fred_0 * float(7u - uIndex) + fred_1 * float(uIndex)) / 7.0f;
            }
            else
            {
                if (uIndex == 6)
                    return -1.0f;
                if (uIndex == 7)
                    return 1.0f;
                uIndex -= 1;
                return (fred_0 * float(5u - uIndex) + fred_1 * float(uIndex)) / 5.0f;
            }
        }

        size_t GetIndex(size_t uOffset) const noexcept
        {
            return static_cast<size_t>((data >> (3 * uOffset + 16)) & 0x07);
        }

        void SetIndex(size_t uOffset, size_t uIndex) noexcept
        {
            data &= ~(uint64_t(0x07) << (3 * uOffset + 16));
            data |= (uint64_t(uIndex) << (3 * uOffset + 16));
        }

        union
        {
            struct
            {
                int8_t red_0;
                int8_t red_1;
                uint8_t indices[6];
            };
            uint64_t data;
        };
    };

    //-------------------------------------------------------------------------------------
    // Convert a floating point value to an 8-bit SNORM
    //-------------------------------------------------------------------------------------
    void inline FloatToSNorm(float fVal, int8_t *piSNorm) noexcept
    {
        constexpr uint32_t dwMostNeg = (1 << (8 * sizeof(int8_t) - 1));

        if (std::isnan(fVal))
            fVal = 0;
        else
            if (fVal > 1)
                fVal = 1;    // Clamp to 1
            else
                if (fVal < -1)
                    fVal = -1;    // Clamp to -1

        fVal = fVal * static_cast<int8_t>(dwMostNeg - 1);

        if (fVal >= 0)
            fVal += .5f;
        else
            fVal -= .5f;

        *piSNorm = static_cast<int8_t>(fVal);
    }

    //------------------------------------------------------------------------------
    void FindEndPointsBC4U(
        const float theTexelsU[],
        uint8_t &endpointU_0,
        uint8_t &endpointU_1) noexcept
    {
        // The boundary of codec for signed/unsigned format
        constexpr float MIN_NORM = 0.f;
        constexpr float MAX_NORM = 1.f;

        // Find max/min of input texels
        float fBlockMax = theTexelsU[0];
        float fBlockMin = theTexelsU[0];
        for (size_t i = 0; i < BLOCK_SIZE; ++i)
        {
            if (theTexelsU[i] < fBlockMin)
            {
                fBlockMin = theTexelsU[i];
            }
            else if (theTexelsU[i] > fBlockMax)
            {
                fBlockMax = theTexelsU[i];
            }
        }

        //  If there are boundary values in input texels, should use 4 interpolated color values to guarantee
        //  the exact code of the boundary values.
        const bool bUsing4BlockCodec = (MIN_NORM == fBlockMin || MAX_NORM == fBlockMax);

        // Using Optimize
        float fStart, fEnd;

        if (!bUsing4BlockCodec)
        {
            // 6 interpolated color values
            OptimizeAlpha<false>(&fStart, &fEnd, theTexelsU, 8);

            auto iStart = static_cast<uint8_t>(fStart * 255.0f);
            auto iEnd = static_cast<uint8_t>(fEnd * 255.0f);

            endpointU_0 = iEnd;
            endpointU_1 = iStart;
        }
        else
        {
            // 4 interpolated color values
            OptimizeAlpha<false>(&fStart, &fEnd, theTexelsU, 6);

            auto iStart = static_cast<uint8_t>(fStart * 255.0f);
            auto iEnd = static_cast<uint8_t>(fEnd * 255.0f);

            endpointU_1 = iEnd;
            endpointU_0 = iStart;
        }
    }

    void FindEndPointsBC4S(
        const float theTexelsU[],
        int8_t &endpointU_0,
        int8_t &endpointU_1) noexcept
    {
        //  The boundary of codec for signed/unsigned format
        constexpr float MIN_NORM = -1.f;
        constexpr float MAX_NORM = 1.f;

        // Find max/min of input texels
        float fBlockMax = theTexelsU[0];
        float fBlockMin = theTexelsU[0];
        for (size_t i = 0; i < BLOCK_SIZE; ++i)
        {
            if (theTexelsU[i] < fBlockMin)
            {
                fBlockMin = theTexelsU[i];
            }
            else if (theTexelsU[i] > fBlockMax)
            {
                fBlockMax = theTexelsU[i];
            }
        }

        //  If there are boundary values in input texels, should use 4 interpolated color values to guarantee
        //  the exact code of the boundary values.
        const bool bUsing4BlockCodec = (MIN_NORM == fBlockMin || MAX_NORM == fBlockMax);

        // Using Optimize
        float fStart, fEnd;

        if (!bUsing4BlockCodec)
        {
            // 6 interpolated color values
            OptimizeAlpha<true>(&fStart, &fEnd, theTexelsU, 8);

            int8_t iStart, iEnd;
            FloatToSNorm(fStart, &iStart);
            FloatToSNorm(fEnd, &iEnd);

            endpointU_0 = iEnd;
            endpointU_1 = iStart;
        }
        else
        {
            // 4 interpolated color values
            OptimizeAlpha<true>(&fStart, &fEnd, theTexelsU, 6);

            int8_t iStart, iEnd;
            FloatToSNorm(fStart, &iStart);
            FloatToSNorm(fEnd, &iEnd);

            endpointU_1 = iEnd;
            endpointU_0 = iStart;
        }
    }

    //------------------------------------------------------------------------------

    inline void FindEndPointsBC5U(
        const float theTexelsU[],
        const float theTexelsV[],
        uint8_t &endpointU_0,
        uint8_t &endpointU_1,
        uint8_t &endpointV_0,
        uint8_t &endpointV_1) noexcept
    {
        //Encoding the U and V channel by BC4 codec separately.
        FindEndPointsBC4U(theTexelsU, endpointU_0, endpointU_1);
        FindEndPointsBC4U(theTexelsV, endpointV_0, endpointV_1);
    }

    inline void FindEndPointsBC5S(
        const float theTexelsU[],
        const float theTexelsV[],
        int8_t &endpointU_0,
        int8_t &endpointU_1,
        int8_t &endpointV_0,
        int8_t &endpointV_1) noexcept
    {
        //Encoding the U and V channel by BC4 codec separately.
        FindEndPointsBC4S(theTexelsU, endpointU_0, endpointU_1);
        FindEndPointsBC4S(theTexelsV, endpointV_0, endpointV_1);
    }

    //------------------------------------------------------------------------------

    void FindClosestUNORM(
        BC4_UNORM* pBC,
        const float theTexelsU[]) noexcept
    {
        float rGradient[8];
        for (size_t i = 0; i < 8; ++i)
        {
            rGradient[i] = pBC->DecodeFromIndex(i);
        }

        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            size_t uBestIndex = 0;
            float fBestDelta = 100000;
            for (size_t uIndex = 0; uIndex < 8; uIndex++)
            {
                const float fCurrentDelta = fabsf(rGradient[uIndex] - theTexelsU[i]);
                if (fCurrentDelta < fBestDelta)
                {
                    uBestIndex = uIndex;
                    fBestDelta = fCurrentDelta;
                }
            }
            pBC->SetIndex(i, uBestIndex);
        }
    }

    void FindClosestSNORM(
        BC4_SNORM* pBC,
        const float theTexelsU[]) noexcept
    {
        float rGradient[8];
        for (size_t i = 0; i < 8; ++i)
        {
            rGradient[i] = pBC->DecodeFromIndex(i);
        }

        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            size_t uBestIndex = 0;
            float fBestDelta = 100000;
            for (size_t uIndex = 0; uIndex < 8; uIndex++)
            {
                const float fCurrentDelta = fabsf(rGradient[uIndex] - theTexelsU[i]);
                if (fCurrentDelta < fBestDelta)
                {
                    uBestIndex = uIndex;
                    fBestDelta = fCurrentDelta;
                }
            }
            pBC->SetIndex(i, uBestIndex);
        }
    }

} // namespace

namespace mango::image
{
    using namespace DirectX;

    void decode_block_bc4u(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(BC4_UNORM) == 8, "BC4_UNORM should be 8 bytes" );
        const BC4_UNORM* pBC4 = reinterpret_cast<const BC4_UNORM*>(input);

        for (int y = 0; y < 4; ++y)
        {
            float32x4* pColor = reinterpret_cast<float32x4*>(output);
            const int blockIndex = y * 4;

            for (int x = 0; x < 4; ++x)
            {
                pColor[x] = float32x4(pBC4->R(blockIndex + x), 0.0f, 0.0f, 1.0f);
            }

            output += stride;
        }
    }

    void decode_block_bc4s(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(BC4_SNORM) == 8, "BC4_SNORM should be 8 bytes" );
        const BC4_SNORM* pBC4 = reinterpret_cast<const BC4_SNORM*>(input);

        for (int y = 0; y < 4; ++y)
        {
            float32x4* pColor = reinterpret_cast<float32x4*>(output);
            const int blockIndex = y * 4;

            for (int x = 0; x < 4; ++x)
            {
                pColor[x] = float32x4(pBC4->R(blockIndex + x), 0.0f, 0.0f, 1.0f);
            }

            output += stride;
        }
    }

    void decode_block_bc5u(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(BC4_UNORM) == 8, "BC4_UNORM should be 8 bytes" );
        const BC4_UNORM* pBC = reinterpret_cast<const BC4_UNORM*>(input);

        for (int y = 0; y < 4; ++y)
        {
            float32x4* pColor = reinterpret_cast<float32x4*>(output);
            const int blockIndex = y * 4;

            for (int x = 0; x < 4; ++x)
            {
                float red   = pBC[0].R(blockIndex + x);
                float green = pBC[1].R(blockIndex + x);
                pColor[x] = float32x4(red, green, 0.0f, 1.0f);
            }

            output += stride;
        }
    }

    void decode_block_bc5s(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(BC4_SNORM) == 8, "BC4_SNORM should be 8 bytes" );
        const BC4_SNORM* pBC = reinterpret_cast<const BC4_SNORM*>(input);

        for (int y = 0; y < 4; ++y)
        {
            float32x4* pColor = reinterpret_cast<float32x4*>(output);
            const int blockIndex = y * 4;

            for (int x = 0; x < 4; ++x)
            {
                float red   = pBC[0].R(blockIndex + x);
                float green = pBC[1].R(blockIndex + x);
                pColor[x] = float32x4(red, green, 0.0f, 1.0f);
            }

            output += stride;
        }
    }

    void encode_block_bc4u(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(BC4_UNORM) == 8, "BC4_UNORM should be 8 bytes" );
        BC4_UNORM* pBC4 = reinterpret_cast<BC4_UNORM*>(output);

        std::memset(output, 0, sizeof(BC4_UNORM));

        float theTexelsU[NUM_PIXELS_PER_BLOCK];

        for (int y = 0; y < 4; ++y)
        {
            const float* image = reinterpret_cast<const float*>(input + y * stride);
            float* u = theTexelsU + y * 4;

            for (int x = 0; x < 4; ++x)
            {
                u[x] = image[0];
                image += 4;
            }
        }

        FindEndPointsBC4U(theTexelsU, pBC4->red_0, pBC4->red_1);
        FindClosestUNORM(pBC4, theTexelsU);
    }

    void encode_block_bc4s(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(BC4_SNORM) == 8, "BC4_SNORM should be 8 bytes" );
        BC4_SNORM* pBC4 = reinterpret_cast<BC4_SNORM*>(output);

        std::memset(output, 0, sizeof(BC4_UNORM));

        float theTexelsU[NUM_PIXELS_PER_BLOCK];

        for (int y = 0; y < 4; ++y)
        {
            const float* image = reinterpret_cast<const float*>(input + y * stride);
            float* u = theTexelsU + y * 4;

            for (int x = 0; x < 4; ++x)
            {
                u[x] = image[0];
                image += 4;
            }
        }

        FindEndPointsBC4S(theTexelsU, pBC4->red_0, pBC4->red_1);
        FindClosestSNORM(pBC4, theTexelsU);
    }

    void encode_block_bc5u(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(BC4_UNORM) == 8, "BC4_UNORM should be 8 bytes" );
        BC4_UNORM* pBC = reinterpret_cast<BC4_UNORM*>(output);

        std::memset(output, 0, sizeof(BC4_UNORM) * 2);

        float theTexelsU[NUM_PIXELS_PER_BLOCK];
        float theTexelsV[NUM_PIXELS_PER_BLOCK];

        for (int y = 0; y < 4; ++y)
        {
            const float* image = reinterpret_cast<const float*>(input + y * stride);
            float* u = theTexelsU + y * 4;
            float* v = theTexelsV + y * 4;

            for (int x = 0; x < 4; ++x)
            {
                u[x] = image[0];
                v[x] = image[1];
                image += 4;
            }
        }

        FindEndPointsBC5U(theTexelsU, theTexelsV,
            pBC[0].red_0,
            pBC[0].red_1,
            pBC[1].red_0,
            pBC[1].red_1);

        FindClosestUNORM(pBC + 0, theTexelsU);
        FindClosestUNORM(pBC + 1, theTexelsV);
    }

    void encode_block_bc5s(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(BC4_SNORM) == 8, "BC4_SNORM should be 8 bytes" );
        BC4_SNORM* pBC = reinterpret_cast<BC4_SNORM*>(output);

        std::memset(output, 0, sizeof(BC4_UNORM) * 2);

        float theTexelsU[NUM_PIXELS_PER_BLOCK];
        float theTexelsV[NUM_PIXELS_PER_BLOCK];

        for (int y = 0; y < 4; ++y)
        {
            const float* image = reinterpret_cast<const float*>(input + y * stride);
            float* u = theTexelsU + y * 4;
            float* v = theTexelsV + y * 4;

            for (int x = 0; x < 4; ++x)
            {
                u[x] = image[0];
                v[x] = image[1];
                image += 4;
            }
        }

        FindEndPointsBC5S(theTexelsU, theTexelsV,
            pBC[0].red_0,
            pBC[0].red_1,
            pBC[1].red_0,
            pBC[1].red_1);

        FindClosestSNORM(pBC + 0, theTexelsU);
        FindClosestSNORM(pBC + 1, theTexelsV);
    }

} // namespace mango::image
