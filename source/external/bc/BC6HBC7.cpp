//-------------------------------------------------------------------------------------
// BC6HBC7.cpp
//
// Block-compression (BC) functionality for BC6H and BC7 (DirectX 11 texture compression)
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
//-------------------------------------------------------------------------------------

#include "BC.h"

using namespace DirectX;

//-------------------------------------------------------------------------------------
// Macros
//-------------------------------------------------------------------------------------

#define SIGN_EXTEND(x,nb) ((((x)&(1<<((nb)-1)))?((~0)^((1<<(nb))-1)):0)|(x))

// Because these are used in SAL annotations, they need to remain macros rather than const values
#define BC6H_MAX_REGIONS 2
#define BC6H_MAX_INDICES 16
#define BC7_MAX_REGIONS 3
#define BC7_MAX_INDICES 16

namespace
{
    //-------------------------------------------------------------------------------------
    // Constants
    //-------------------------------------------------------------------------------------

    constexpr uint16_t F16S_MASK = 0x8000;   // f16 sign mask
    constexpr uint16_t F16EM_MASK = 0x7fff;   // f16 exp & mantissa mask
    constexpr uint16_t F16MAX = 0x7bff;   // MAXFLT bit pattern for XMHALF

    constexpr size_t BC6H_NUM_CHANNELS = 3;
    constexpr size_t BC6H_MAX_SHAPES = 32;

    constexpr size_t BC7_NUM_CHANNELS = 4;
    constexpr size_t BC7_MAX_SHAPES = 64;

    constexpr int32_t BC67_WEIGHT_MAX = 64;
    constexpr uint32_t BC67_WEIGHT_SHIFT = 6;
    constexpr int32_t BC67_WEIGHT_ROUND = 32;

    constexpr float fEpsilon = (0.25f / 64.0f) * (0.25f / 64.0f);
    constexpr float pC3[] = { 2.0f / 2.0f, 1.0f / 2.0f, 0.0f / 2.0f };
    constexpr float pD3[] = { 0.0f / 2.0f, 1.0f / 2.0f, 2.0f / 2.0f };
    constexpr float pC4[] = { 3.0f / 3.0f, 2.0f / 3.0f, 1.0f / 3.0f, 0.0f / 3.0f };
    constexpr float pD4[] = { 0.0f / 3.0f, 1.0f / 3.0f, 2.0f / 3.0f, 3.0f / 3.0f };

    // Partition, Shape, Pixel (index into 4x4 block)
    const uint8_t g_aPartitionTable[3][64][16] =
    {
        {   // 1 Region case has no subsets (all 0)
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
        },

        {   // BC6H/BC7 Partition Set for 2 Subsets
            { 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1 }, // Shape 0
            { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // Shape 1
            { 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1 }, // Shape 2
            { 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1 }, // Shape 3
            { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1 }, // Shape 4
            { 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1 }, // Shape 5
            { 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1 }, // Shape 6
            { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1 }, // Shape 7
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1 }, // Shape 8
            { 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 9
            { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1 }, // Shape 10
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1 }, // Shape 11
            { 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 12
            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 13
            { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 14
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 }, // Shape 15
            { 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1 }, // Shape 16
            { 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, // Shape 17
            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0 }, // Shape 18
            { 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0 }, // Shape 19
            { 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, // Shape 20
            { 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0 }, // Shape 21
            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0 }, // Shape 22
            { 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1 }, // Shape 23
            { 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0 }, // Shape 24
            { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0 }, // Shape 25
            { 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0 }, // Shape 26
            { 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0 }, // Shape 27
            { 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0 }, // Shape 28
            { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, // Shape 29
            { 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0 }, // Shape 30
            { 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0 }, // Shape 31

                                                                // BC7 Partition Set for 2 Subsets (second-half)
            { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // Shape 32
            { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1 }, // Shape 33
            { 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0 }, // Shape 34
            { 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0 }, // Shape 35
            { 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0 }, // Shape 36
            { 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0 }, // Shape 37
            { 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1 }, // Shape 38
            { 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1 }, // Shape 39
            { 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0 }, // Shape 40
            { 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0 }, // Shape 41
            { 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0 }, // Shape 42
            { 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0 }, // Shape 43
            { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 }, // Shape 44
            { 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1 }, // Shape 45
            { 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1 }, // Shape 46
            { 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0 }, // Shape 47
            { 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 }, // Shape 48
            { 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0 }, // Shape 49
            { 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0 }, // Shape 50
            { 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0 }, // Shape 51
            { 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1 }, // Shape 52
            { 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1 }, // Shape 53
            { 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0 }, // Shape 54
            { 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0 }, // Shape 55
            { 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1 }, // Shape 56
            { 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1 }, // Shape 57
            { 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 }, // Shape 58
            { 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1 }, // Shape 59
            { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1 }, // Shape 60
            { 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, // Shape 61
            { 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // Shape 62
            { 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1 }  // Shape 63
        },

        {   // BC7 Partition Set for 3 Subsets
            { 0, 0, 1, 1, 0, 0, 1, 1, 0, 2, 2, 1, 2, 2, 2, 2 }, // Shape 0
            { 0, 0, 0, 1, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 2, 1 }, // Shape 1
            { 0, 0, 0, 0, 2, 0, 0, 1, 2, 2, 1, 1, 2, 2, 1, 1 }, // Shape 2
            { 0, 2, 2, 2, 0, 0, 2, 2, 0, 0, 1, 1, 0, 1, 1, 1 }, // Shape 3
            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2 }, // Shape 4
            { 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 2, 2, 0, 0, 2, 2 }, // Shape 5
            { 0, 0, 2, 2, 0, 0, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 6
            { 0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1 }, // Shape 7
            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2 }, // Shape 8
            { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2 }, // Shape 9
            { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2 }, // Shape 10
            { 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2 }, // Shape 11
            { 0, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1, 2 }, // Shape 12
            { 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 2 }, // Shape 13
            { 0, 0, 1, 1, 0, 1, 1, 2, 1, 1, 2, 2, 1, 2, 2, 2 }, // Shape 14
            { 0, 0, 1, 1, 2, 0, 0, 1, 2, 2, 0, 0, 2, 2, 2, 0 }, // Shape 15
            { 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 2, 1, 1, 2, 2 }, // Shape 16
            { 0, 1, 1, 1, 0, 0, 1, 1, 2, 0, 0, 1, 2, 2, 0, 0 }, // Shape 17
            { 0, 0, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1, 2, 2 }, // Shape 18
            { 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 2, 2, 1, 1, 1, 1 }, // Shape 19
            { 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2, 0, 2, 2, 2 }, // Shape 20
            { 0, 0, 0, 1, 0, 0, 0, 1, 2, 2, 2, 1, 2, 2, 2, 1 }, // Shape 21
            { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 2, 2, 0, 1, 2, 2 }, // Shape 22
            { 0, 0, 0, 0, 1, 1, 0, 0, 2, 2, 1, 0, 2, 2, 1, 0 }, // Shape 23
            { 0, 1, 2, 2, 0, 1, 2, 2, 0, 0, 1, 1, 0, 0, 0, 0 }, // Shape 24
            { 0, 0, 1, 2, 0, 0, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2 }, // Shape 25
            { 0, 1, 1, 0, 1, 2, 2, 1, 1, 2, 2, 1, 0, 1, 1, 0 }, // Shape 26
            { 0, 0, 0, 0, 0, 1, 1, 0, 1, 2, 2, 1, 1, 2, 2, 1 }, // Shape 27
            { 0, 0, 2, 2, 1, 1, 0, 2, 1, 1, 0, 2, 0, 0, 2, 2 }, // Shape 28
            { 0, 1, 1, 0, 0, 1, 1, 0, 2, 0, 0, 2, 2, 2, 2, 2 }, // Shape 29
            { 0, 0, 1, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 0, 1, 1 }, // Shape 30
            { 0, 0, 0, 0, 2, 0, 0, 0, 2, 2, 1, 1, 2, 2, 2, 1 }, // Shape 31
            { 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 2, 1, 2, 2, 2 }, // Shape 32
            { 0, 2, 2, 2, 0, 0, 2, 2, 0, 0, 1, 2, 0, 0, 1, 1 }, // Shape 33
            { 0, 0, 1, 1, 0, 0, 1, 2, 0, 0, 2, 2, 0, 2, 2, 2 }, // Shape 34
            { 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0 }, // Shape 35
            { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 0, 0, 0, 0 }, // Shape 36
            { 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0 }, // Shape 37
            { 0, 1, 2, 0, 2, 0, 1, 2, 1, 2, 0, 1, 0, 1, 2, 0 }, // Shape 38
            { 0, 0, 1, 1, 2, 2, 0, 0, 1, 1, 2, 2, 0, 0, 1, 1 }, // Shape 39
            { 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 0, 0, 0, 0, 1, 1 }, // Shape 40
            { 0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2 }, // Shape 41
            { 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 1, 2, 1, 2, 1 }, // Shape 42
            { 0, 0, 2, 2, 1, 1, 2, 2, 0, 0, 2, 2, 1, 1, 2, 2 }, // Shape 43
            { 0, 0, 2, 2, 0, 0, 1, 1, 0, 0, 2, 2, 0, 0, 1, 1 }, // Shape 44
            { 0, 2, 2, 0, 1, 2, 2, 1, 0, 2, 2, 0, 1, 2, 2, 1 }, // Shape 45
            { 0, 1, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 1 }, // Shape 46
            { 0, 0, 0, 0, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1 }, // Shape 47
            { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 2, 2 }, // Shape 48
            { 0, 2, 2, 2, 0, 1, 1, 1, 0, 2, 2, 2, 0, 1, 1, 1 }, // Shape 49
            { 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 2, 1, 1, 1, 2 }, // Shape 50
            { 0, 0, 0, 0, 2, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1, 2 }, // Shape 51
            { 0, 2, 2, 2, 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2 }, // Shape 52
            { 0, 0, 0, 2, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0, 0, 2 }, // Shape 53
            { 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 2, 2, 2, 2 }, // Shape 54
            { 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 2, 1, 1, 2 }, // Shape 55
            { 0, 1, 1, 0, 0, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2 }, // Shape 56
            { 0, 0, 2, 2, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 2, 2 }, // Shape 57
            { 0, 0, 2, 2, 1, 1, 2, 2, 1, 1, 2, 2, 0, 0, 2, 2 }, // Shape 58
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2 }, // Shape 59
            { 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1 }, // Shape 60
            { 0, 2, 2, 2, 1, 2, 2, 2, 0, 2, 2, 2, 1, 2, 2, 2 }, // Shape 61
            { 0, 1, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 }, // Shape 62
            { 0, 1, 1, 1, 2, 0, 1, 1, 2, 2, 0, 1, 2, 2, 2, 0 }  // Shape 63
        }
    };

    // Partition, Shape, Fixup
    const uint8_t g_aFixUp[3][64][3] =
    {
        {   // No fix-ups for 1st subset for BC6H or BC7
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 }
        },

        {   // BC6H/BC7 Partition Set Fixups for 2 Subsets
            { 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
            { 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
            { 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
            { 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
            { 0,15, 0 },{ 0, 2, 0 },{ 0, 8, 0 },{ 0, 2, 0 },
            { 0, 2, 0 },{ 0, 8, 0 },{ 0, 8, 0 },{ 0,15, 0 },
            { 0, 2, 0 },{ 0, 8, 0 },{ 0, 2, 0 },{ 0, 2, 0 },
            { 0, 8, 0 },{ 0, 8, 0 },{ 0, 2, 0 },{ 0, 2, 0 },

            // BC7 Partition Set Fixups for 2 Subsets (second-half)
            { 0,15, 0 },{ 0,15, 0 },{ 0, 6, 0 },{ 0, 8, 0 },
            { 0, 2, 0 },{ 0, 8, 0 },{ 0,15, 0 },{ 0,15, 0 },
            { 0, 2, 0 },{ 0, 8, 0 },{ 0, 2, 0 },{ 0, 2, 0 },
            { 0, 2, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0, 6, 0 },
            { 0, 6, 0 },{ 0, 2, 0 },{ 0, 6, 0 },{ 0, 8, 0 },
            { 0,15, 0 },{ 0,15, 0 },{ 0, 2, 0 },{ 0, 2, 0 },
            { 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
            { 0,15, 0 },{ 0, 2, 0 },{ 0, 2, 0 },{ 0,15, 0 }
        },

        {   // BC7 Partition Set Fixups for 3 Subsets
            { 0, 3,15 },{ 0, 3, 8 },{ 0,15, 8 },{ 0,15, 3 },
            { 0, 8,15 },{ 0, 3,15 },{ 0,15, 3 },{ 0,15, 8 },
            { 0, 8,15 },{ 0, 8,15 },{ 0, 6,15 },{ 0, 6,15 },
            { 0, 6,15 },{ 0, 5,15 },{ 0, 3,15 },{ 0, 3, 8 },
            { 0, 3,15 },{ 0, 3, 8 },{ 0, 8,15 },{ 0,15, 3 },
            { 0, 3,15 },{ 0, 3, 8 },{ 0, 6,15 },{ 0,10, 8 },
            { 0, 5, 3 },{ 0, 8,15 },{ 0, 8, 6 },{ 0, 6,10 },
            { 0, 8,15 },{ 0, 5,15 },{ 0,15,10 },{ 0,15, 8 },
            { 0, 8,15 },{ 0,15, 3 },{ 0, 3,15 },{ 0, 5,10 },
            { 0, 6,10 },{ 0,10, 8 },{ 0, 8, 9 },{ 0,15,10 },
            { 0,15, 6 },{ 0, 3,15 },{ 0,15, 8 },{ 0, 5,15 },
            { 0,15, 3 },{ 0,15, 6 },{ 0,15, 6 },{ 0,15, 8 },
            { 0, 3,15 },{ 0,15, 3 },{ 0, 5,15 },{ 0, 5,15 },
            { 0, 5,15 },{ 0, 8,15 },{ 0, 5,15 },{ 0,10,15 },
            { 0, 5,15 },{ 0,10,15 },{ 0, 8,15 },{ 0,13,15 },
            { 0,15, 3 },{ 0,12,15 },{ 0, 3,15 },{ 0, 3, 8 }
        }
    };

    const int g_aWeights2[] = { 0, 21, 43, 64 };
    const int g_aWeights3[] = { 0, 9, 18, 27, 37, 46, 55, 64 };
    const int g_aWeights4[] = { 0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64 };
}

namespace DirectX
{
    class LDRColorA
    {
    public:
        uint8_t r, g, b, a;

        LDRColorA() = default;
        LDRColorA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) noexcept : r(_r), g(_g), b(_b), a(_a) {}

        const uint8_t& operator [] (size_t uElement) const noexcept
        {
            switch (uElement)
            {
            case 0: return r;
            case 1: return g;
            case 2: return b;
            case 3: return a;
            default: assert(false); return r;
            }
        }

        uint8_t& operator [] (size_t uElement) noexcept
        {
            switch (uElement)
            {
            case 0: return r;
            case 1: return g;
            case 2: return b;
            case 3: return a;
            default: assert(false); return r;
            }
        }

        operator u32 () const
        {
            u32 temp;
            std::memcpy(&temp, this, 4);
            return temp;
        }

        operator float32x4 () const
        {
            int32x4 value;
            value.unpack(*this);
            return convert<float32x4>(value);
        }

        LDRColorA operator = (const HDRColorA& c) noexcept
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

        static void InterpolateRGB(const LDRColorA& c0, const LDRColorA& c1, size_t wc, size_t wcprec, LDRColorA& out) noexcept
        {
            const int* aWeights = nullptr;
            switch (wcprec)
            {
            case 2: aWeights = g_aWeights2; break;
            case 3: aWeights = g_aWeights3; break;
            case 4: aWeights = g_aWeights4; break;
            default: assert(false); out.r = out.g = out.b = 0; return;
            }
            out.r = uint8_t((uint32_t(c0.r) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.r) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
            out.g = uint8_t((uint32_t(c0.g) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.g) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
            out.b = uint8_t((uint32_t(c0.b) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.b) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
        }

        static void InterpolateA(const LDRColorA& c0, const LDRColorA& c1, size_t wa, size_t waprec, LDRColorA& out) noexcept
        {
            const int* aWeights = nullptr;
            switch (waprec)
            {
            case 2: aWeights = g_aWeights2; break;
            case 3: aWeights = g_aWeights3; break;
            case 4: aWeights = g_aWeights4; break;
            default: assert(false); out.a = 0; return;
            }
            out.a = uint8_t((uint32_t(c0.a) * uint32_t(BC67_WEIGHT_MAX - aWeights[wa]) + uint32_t(c1.a) * uint32_t(aWeights[wa]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
        }

        static void Interpolate(const LDRColorA& c0, const LDRColorA& c1, size_t wc, size_t wa, size_t wcprec, size_t waprec, LDRColorA& out) noexcept
        {
            InterpolateRGB(c0, c1, wc, wcprec, out);
            InterpolateA(c0, c1, wa, waprec, out);
        }
    };

    static_assert(sizeof(LDRColorA) == 4, "Unexpected packing");

    struct LDREndPntPair
    {
        LDRColorA A;
        LDRColorA B;
    };

    inline HDRColorA::HDRColorA(const LDRColorA& c) noexcept
    {
        r = float(c.r) * (1.0f / 255.0f);
        g = float(c.g) * (1.0f / 255.0f);
        b = float(c.b) * (1.0f / 255.0f);
        a = float(c.a) * (1.0f / 255.0f);
    }

    inline HDRColorA& HDRColorA::operator = (const LDRColorA& c) noexcept
    {
        r = static_cast<float>(c.r);
        g = static_cast<float>(c.g);
        b = static_cast<float>(c.b);
        a = static_cast<float>(c.a);
        return *this;
    }

    inline LDRColorA HDRColorA::ToLDRColorA() const noexcept
    {
        return LDRColorA(
            static_cast<uint8_t>(r + 0.01f), 
            static_cast<uint8_t>(g + 0.01f), 
            static_cast<uint8_t>(b + 0.01f), 
            static_cast<uint8_t>(a + 0.01f));
    }
}

namespace
{
    static
    float16 INT2F16(int input, bool bSigned) noexcept
    {
        float16 h;
        if (bSigned)
        {
            int s = 0;
            if (input < 0)
            {
                s = F16S_MASK;
                input = -input;
            }
            h.u = u16(s | input);
        }
        else
        {
            assert(input >= 0 && input <= F16MAX);
            h.u = u16(input);
        }

        return h;
    }

    class INTColor
    {
    public:
        int r, g, b;
        int pad;

    public:
        INTColor() = default;
        INTColor(int nr, int ng, int nb) noexcept
            : r(nr)
            , g(ng)
            , b(nb)
            , pad(0)
        {
        }

        INTColor& operator += (const INTColor& c) noexcept
        {
            r += c.r;
            g += c.g;
            b += c.b;
            return *this;
        }

        INTColor& operator -= (const INTColor& c) noexcept
        {
            r -= c.r;
            g -= c.g;
            b -= c.b;
            return *this;
        }

        INTColor& operator &= (const INTColor& c) noexcept
        {
            r &= c.r;
            g &= c.g;
            b &= c.b;
            return *this;
        }

        int& operator [] (uint8_t i) noexcept
        {
            assert(i < sizeof(INTColor) / sizeof(int));
            return reinterpret_cast<int*>(this)[i];
        }

        operator float32x4 () const
        {
            const int32x4* p = reinterpret_cast<const int32x4*>(this);
            return convert<float32x4>(*p);
        }

        void Set(const HDRColorA& c, bool bSigned) noexcept
        {
            float16x4 aF16;

            const float32x4 v = simd::f32x4_uload(reinterpret_cast<const float*>(&c));
            aF16 = v; //XMStoreHalf4(&aF16, v);

            r = F16ToINT(aF16[0], bSigned);
            g = F16ToINT(aF16[1], bSigned);
            b = F16ToINT(aF16[2], bSigned);
        }

        INTColor& Clamp(int iMin, int iMax) noexcept
        {
            r = std::min<int>(iMax, std::max<int>(iMin, r));
            g = std::min<int>(iMax, std::max<int>(iMin, g));
            b = std::min<int>(iMax, std::max<int>(iMin, b));
            return *this;
        }

        INTColor& SignExtend(const LDRColorA& Prec) noexcept
        {
            r = SIGN_EXTEND(r, int(Prec.r));
            g = SIGN_EXTEND(g, int(Prec.g));
            b = SIGN_EXTEND(b, int(Prec.b));
            return *this;
        }

    private:
        static int F16ToINT(const float16& f, bool bSigned) noexcept
        {
            uint16_t input = *reinterpret_cast<const uint16_t*>(&f);
            int out, s;
            if (bSigned)
            {
                s = input & F16S_MASK;
                input &= F16EM_MASK;
                if (input > F16MAX) out = F16MAX;
                else out = input;
                out = s ? -out : out;
            }
            else
            {
                if (input & F16S_MASK) out = 0;
                else out = input;
            }
            return out;
        }
    };

    static_assert(sizeof(INTColor) == 16, "Unexpected packing");

    struct INTEndPntPair
    {
        INTColor A;
        INTColor B;
    };

    template< size_t SizeInBytes >
    class CBits
    {
    public:
        uint8_t GetBit(size_t& uStartBit) const noexcept
        {
            assert(uStartBit < 128);
            const size_t uIndex = uStartBit >> 3;
            auto const ret = static_cast<uint8_t>((m_uBits[uIndex] >> (uStartBit - (uIndex << 3))) & 0x01);
            uStartBit++;
            return ret;
        }

        uint8_t GetBits(size_t& uStartBit, size_t uNumBits) const noexcept
        {
            if (uNumBits == 0) return 0;
            assert(uStartBit + uNumBits <= 128 && uNumBits <= 8);
            uint8_t ret;
            const size_t uIndex = uStartBit >> 3;
            const size_t uBase = uStartBit - (uIndex << 3);
            if (uBase + uNumBits > 8)
            {
                const size_t uFirstIndexBits = 8 - uBase;
                const size_t uNextIndexBits = uNumBits - uFirstIndexBits;
                ret = static_cast<uint8_t>((unsigned(m_uBits[uIndex]) >> uBase) | ((unsigned(m_uBits[uIndex + 1]) & ((1u << uNextIndexBits) - 1)) << uFirstIndexBits));
            }
            else
            {
                ret = static_cast<uint8_t>((m_uBits[uIndex] >> uBase) & ((1 << uNumBits) - 1));
            }
            assert(ret < (1 << uNumBits));
            uStartBit += uNumBits;
            return ret;
        }

        void SetBit(size_t& uStartBit, uint8_t uValue) noexcept
        {
            assert(uStartBit < 128 && uValue < 2);
            size_t uIndex = uStartBit >> 3;
            const size_t uBase = uStartBit - (uIndex << 3);
            m_uBits[uIndex] &= ~(1 << uBase);
            m_uBits[uIndex] |= uValue << uBase;
            uStartBit++;
        }

        void SetBits(size_t& uStartBit, size_t uNumBits, uint8_t uValue) noexcept
        {
            if (uNumBits == 0)
                return;
            assert(uStartBit + uNumBits <= 128 && uNumBits <= 8);
            assert(uValue < (1 << uNumBits));
            size_t uIndex = uStartBit >> 3;
            const size_t uBase = uStartBit - (uIndex << 3);
            if (uBase + uNumBits > 8)
            {
                const size_t uFirstIndexBits = 8 - uBase;
                const size_t uNextIndexBits = uNumBits - uFirstIndexBits;
                m_uBits[uIndex] &= ~(((1 << uFirstIndexBits) - 1) << uBase);
                m_uBits[uIndex] |= uValue << uBase;
                m_uBits[uIndex + 1] &= ~((1 << uNextIndexBits) - 1);
                m_uBits[uIndex + 1] |= uValue >> uFirstIndexBits;
            }
            else
            {
                m_uBits[uIndex] &= ~(((1 << uNumBits) - 1) << uBase);
                m_uBits[uIndex] |= uValue << uBase;
            }
            uStartBit += uNumBits;
        }

    private:
        uint8_t m_uBits[SizeInBytes];
    };

    // BC6H compression (16 bits per texel)
    class D3DX_BC6H : private CBits< 16 >
    {
    public:
        void Decode(bool bSigned, u8* output, size_t stride) const noexcept;
        void Encode(bool bSigned, const u8* input, size_t stride) noexcept;

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

        struct EncodeParams
        {
            float fBestErr;
            const bool bSigned;
            uint8_t uMode;
            uint8_t uShape;
            HDRColorA aHDRPixels[NUM_PIXELS_PER_BLOCK];;
            INTEndPntPair aUnqEndPts[BC6H_MAX_SHAPES][BC6H_MAX_REGIONS];
            INTColor aIPixels[NUM_PIXELS_PER_BLOCK];

            EncodeParams(const u8* input, size_t stride, bool bSignedFormat) noexcept
                : fBestErr(FLT_MAX)
                , bSigned(bSignedFormat)
                , uMode(0)
                , uShape(0)
                , aUnqEndPts{}
                , aIPixels{}
            {
                for (int y = 0; y < 4; ++y)
                {
                    const float16x4* src = reinterpret_cast<const float16x4*>(input + y * stride);
                    float32x4* dest = reinterpret_cast<float32x4*>(aHDRPixels + y * 4);

                    for (int x = 0; x < 4; ++x)
                    {
                        dest[x] = src[x];
                    }
                }

                for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
                {
                    aIPixels[i].Set(aHDRPixels[i], bSigned);
                }
            }
        };

        static int Quantize(int iValue, int prec, bool bSigned) noexcept;
        static int Unquantize(int comp, uint8_t uBitsPerComp, bool bSigned) noexcept;
        static int FinishUnquantize(int comp, bool bSigned) noexcept;

        static bool EndPointsFit(const EncodeParams* pEP, const INTEndPntPair aEndPts[]) noexcept;

        void GeneratePaletteQuantized(const EncodeParams* pEP, const INTEndPntPair& endPts,
            INTColor aPalette[]) const noexcept;
        float MapColorsQuantized(const EncodeParams* pEP, const INTColor aColors[], size_t np, const INTEndPntPair &endPts) const noexcept;
        float PerturbOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, uint8_t ch,
            const INTEndPntPair& oldEndPts, INTEndPntPair& newEndPts, float fOldErr, int do_b) const noexcept;
        void OptimizeOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, float aOrgErr,
            const INTEndPntPair &aOrgEndPts, INTEndPntPair &aOptEndPts) const noexcept;
        void OptimizeEndPoints(const EncodeParams* pEP, const float aOrgErr[],
            const INTEndPntPair aOrgEndPts[],
            INTEndPntPair aOptEndPts[]) const noexcept;
        static void SwapIndices(const EncodeParams* pEP, INTEndPntPair aEndPts[],
            size_t aIndices[]) noexcept;
        void AssignIndices(const EncodeParams* pEP, const INTEndPntPair aEndPts[],
            size_t aIndices[],
            float aTotErr[]) const noexcept;
        void QuantizeEndPts(const EncodeParams* pEP, INTEndPntPair* qQntEndPts) const noexcept;
        void EmitBlock(const EncodeParams* pEP, const INTEndPntPair aEndPts[],
            const size_t aIndices[]) noexcept;
        void Refine(EncodeParams* pEP) noexcept;

        static void GeneratePaletteUnquantized(const EncodeParams* pEP, size_t uRegion, INTColor aPalette[]) noexcept;
        float MapColors(const EncodeParams* pEP, size_t uRegion, size_t np, const size_t* auIndex) const noexcept;
        float RoughMSE(EncodeParams* pEP) const noexcept;

    private:
        static constexpr uint8_t c_NumModes = 14;
        static constexpr uint8_t c_NumModeInfo = 32;

        static const ModeDescriptor ms_aDesc[c_NumModes][82];
        static const ModeInfo ms_aInfo[c_NumModes];
        static const int ms_aModeToInfo[c_NumModeInfo];
    };

    // BC67 compression (16b bits per texel)
    class D3DX_BC7 : private CBits< 16 >
    {
    public:
        void Decode(u8* output, size_t stride) const noexcept;
        void Encode(uint32_t flags, const u8* input, size_t stride) noexcept;

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

        struct EncodeParams
        {
            uint8_t uMode;
            LDREndPntPair aEndPts[BC7_MAX_SHAPES][BC7_MAX_REGIONS];
            LDRColorA aLDRPixels[NUM_PIXELS_PER_BLOCK];
            const HDRColorA* const aHDRPixels;

            EncodeParams(const HDRColorA* const aOriginal) noexcept
                : uMode(0)
                , aEndPts{}
                , aLDRPixels{}
                , aHDRPixels(aOriginal)
            {
            }
        };

        static uint8_t Quantize(uint8_t comp, uint8_t uPrec) noexcept
        {
            assert(0 < uPrec && uPrec <= 8);
            const uint8_t rnd = std::min<uint8_t>(255u, static_cast<uint8_t>(unsigned(comp) + (1u << (7 - uPrec))));
            return uint8_t(rnd >> (8u - uPrec));
        }

        static LDRColorA Quantize(const LDRColorA& c, const LDRColorA& RGBAPrec) noexcept
        {
            LDRColorA q;
            q.r = Quantize(c.r, RGBAPrec.r);
            q.g = Quantize(c.g, RGBAPrec.g);
            q.b = Quantize(c.b, RGBAPrec.b);
            if (RGBAPrec.a)
                q.a = Quantize(c.a, RGBAPrec.a);
            else
                q.a = 255;
            return q;
        }

        static uint8_t Unquantize(uint8_t comp, size_t uPrec) noexcept
        {
            assert(0 < uPrec && uPrec <= 8);
            comp = static_cast<uint8_t>(unsigned(comp) << (8 - uPrec));
            return uint8_t(comp | (comp >> uPrec));
        }

        static LDRColorA Unquantize(const LDRColorA& c, const LDRColorA& RGBAPrec) noexcept
        {
            LDRColorA q;
            q.r = Unquantize(c.r, RGBAPrec.r);
            q.g = Unquantize(c.g, RGBAPrec.g);
            q.b = Unquantize(c.b, RGBAPrec.b);
            q.a = RGBAPrec.a > 0 ? Unquantize(c.a, RGBAPrec.a) : 255u;
            return q;
        }

        void GeneratePaletteQuantized(const EncodeParams* pEP, size_t uIndexMode, const LDREndPntPair& endpts,
            LDRColorA aPalette[]) const noexcept;
        float PerturbOne(const EncodeParams* pEP, const LDRColorA colors[], size_t np, size_t uIndexMode,
            size_t ch, const LDREndPntPair &old_endpts,
            LDREndPntPair &new_endpts, float old_err, uint8_t do_b) const noexcept;
        void Exhaustive(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode,
            size_t ch, float& fOrgErr, LDREndPntPair& optEndPt) const noexcept;
        void OptimizeOne(const EncodeParams* pEP, const LDRColorA colors[], size_t np, size_t uIndexMode,
            float orig_err, const LDREndPntPair &orig_endpts, LDREndPntPair &opt_endpts) const noexcept;
        void OptimizeEndPoints(const EncodeParams* pEP, size_t uShape, size_t uIndexMode,
            const float orig_err[],
            const LDREndPntPair orig_endpts[],
            LDREndPntPair opt_endpts[]) const noexcept;
        void AssignIndices(const EncodeParams* pEP, size_t uShape, size_t uIndexMode,
            LDREndPntPair endpts[],
            size_t aIndices[], size_t aIndices2[],
            float afTotErr[]) const noexcept;
        void EmitBlock(const EncodeParams* pEP, size_t uShape, size_t uRotation, size_t uIndexMode,
            const LDREndPntPair aEndPts[],
            const size_t aIndex[],
            const size_t aIndex2[]) noexcept;
        void FixEndpointPBits(const EncodeParams* pEP, const LDREndPntPair *pOrigEndpoints, LDREndPntPair *pFixedEndpoints) noexcept;
        float Refine(const EncodeParams* pEP, size_t uShape, size_t uRotation, size_t uIndexMode) noexcept;

        float MapColors(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode,
            const LDREndPntPair& endPts, float fMinErr) const noexcept;
        static float RoughMSE(EncodeParams* pEP, size_t uShape, size_t uIndexMode) noexcept;

    private:
        static constexpr uint8_t c_NumModes = 8;

        static const ModeInfo ms_aInfo[c_NumModes];
    };
}

// BC6H Compression
const D3DX_BC6H::ModeDescriptor D3DX_BC6H::ms_aDesc[D3DX_BC6H::c_NumModes][82] =
{
    {   // Mode 1 (0x00) - 10 5 5 5
        { M, 0}, { M, 1}, {GY, 4}, {BY, 4}, {BZ, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 2 (0x01) - 7 6 6 6
        { M, 0}, { M, 1}, {GY, 5}, {GZ, 4}, {GZ, 5}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {BZ, 0}, {BZ, 1}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {BY, 5}, {BZ, 2}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BZ, 3}, {BZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {RY, 5}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {RZ, 5}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 3 (0x02) - 11 5 4 4
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RW,10}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GW,10},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BW,10},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 4 (0x06) - 11 4 5 4
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RW,10},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GW,10}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BW,10},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {BZ, 0},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {GY, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 5 (0x0a) - 11 4 4 5
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RW,10},
        {BY, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GW,10},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BW,10}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {BZ, 1},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {BZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 6 (0x0e) - 9 5 5 5
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 7 (0x12) - 8 6 5 5
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {GZ, 4}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {BZ, 2}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BZ, 3}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {RY, 5}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {RZ, 5}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 8 (0x16) - 8 5 6 5
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {BZ, 0}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GY, 5}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {GZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 9 (0x1a) - 8 5 5 6
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {BZ, 1}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {BY, 5}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 10 (0x1e) - 6 6 6 6
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {GZ, 4}, {BZ, 0}, {BZ, 1}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GY, 5}, {BY, 5}, {BZ, 2}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {GZ, 5}, {BZ, 3}, {BZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {RY, 5}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {RZ, 5}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 11 (0x03) - 10 10
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {RX, 6}, {RX, 7}, {RX, 8}, {RX, 9}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GX, 6}, {GX, 7}, {GX, 8}, {GX, 9}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BX, 6}, {BX, 7}, {BX, 8}, {BX, 9}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0},
    },

    {   // Mode 12 (0x07) - 11 9
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {RX, 6}, {RX, 7}, {RX, 8}, {RW,10}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GX, 6}, {GX, 7}, {GX, 8}, {GW,10}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BX, 6}, {BX, 7}, {BX, 8}, {BW,10}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0},
    },

    {   // Mode 13 (0x0b) - 12 8
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {RX, 6}, {RX, 7}, {RW,11}, {RW,10}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GX, 6}, {GX, 7}, {GW,11}, {GW,10}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BX, 6}, {BX, 7}, {BW,11}, {BW,10}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0},
    },

    {   // Mode 14 (0x0f) - 16 4
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RW,15},
        {RW,14}, {RW,13}, {RW,12}, {RW,11}, {RW,10}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GW,15},
        {GW,14}, {GW,13}, {GW,12}, {GW,11}, {GW,10}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BW,15},
        {BW,14}, {BW,13}, {BW,12}, {BW,11}, {BW,10}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0},
    },
};

// Mode, Partitions, Transformed, IndexPrec, RGBAPrec
const D3DX_BC6H::ModeInfo D3DX_BC6H::ms_aInfo[D3DX_BC6H::c_NumModes] =
{
    {0x00, 1, true,  3, { { LDRColorA(10,10,10,0), LDRColorA(5, 5, 5,0) }, { LDRColorA(5,5,5,0), LDRColorA(5,5,5,0) } } }, // Mode 1
    {0x01, 1, true,  3, { { LDRColorA(7, 7, 7,0), LDRColorA(6, 6, 6,0) }, { LDRColorA(6,6,6,0), LDRColorA(6,6,6,0) } } }, // Mode 2
    {0x02, 1, true,  3, { { LDRColorA(11,11,11,0), LDRColorA(5, 4, 4,0) }, { LDRColorA(5,4,4,0), LDRColorA(5,4,4,0) } } }, // Mode 3
    {0x06, 1, true,  3, { { LDRColorA(11,11,11,0), LDRColorA(4, 5, 4,0) }, { LDRColorA(4,5,4,0), LDRColorA(4,5,4,0) } } }, // Mode 4
    {0x0a, 1, true,  3, { { LDRColorA(11,11,11,0), LDRColorA(4, 4, 5,0) }, { LDRColorA(4,4,5,0), LDRColorA(4,4,5,0) } } }, // Mode 5
    {0x0e, 1, true,  3, { { LDRColorA(9, 9, 9,0), LDRColorA(5, 5, 5,0) }, { LDRColorA(5,5,5,0), LDRColorA(5,5,5,0) } } }, // Mode 6
    {0x12, 1, true,  3, { { LDRColorA(8, 8, 8,0), LDRColorA(6, 5, 5,0) }, { LDRColorA(6,5,5,0), LDRColorA(6,5,5,0) } } }, // Mode 7
    {0x16, 1, true,  3, { { LDRColorA(8, 8, 8,0), LDRColorA(5, 6, 5,0) }, { LDRColorA(5,6,5,0), LDRColorA(5,6,5,0) } } }, // Mode 8
    {0x1a, 1, true,  3, { { LDRColorA(8, 8, 8,0), LDRColorA(5, 5, 6,0) }, { LDRColorA(5,5,6,0), LDRColorA(5,5,6,0) } } }, // Mode 9
    {0x1e, 1, false, 3, { { LDRColorA(6, 6, 6,0), LDRColorA(6, 6, 6,0) }, { LDRColorA(6,6,6,0), LDRColorA(6,6,6,0) } } }, // Mode 10
    {0x03, 0, false, 4, { { LDRColorA(10,10,10,0), LDRColorA(10,10,10,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 11
    {0x07, 0, true,  4, { { LDRColorA(11,11,11,0), LDRColorA(9, 9, 9,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 12
    {0x0b, 0, true,  4, { { LDRColorA(12,12,12,0), LDRColorA(8, 8, 8,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 13
    {0x0f, 0, true,  4, { { LDRColorA(16,16,16,0), LDRColorA(4, 4, 4,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 14
};

const int D3DX_BC6H::ms_aModeToInfo[D3DX_BC6H::c_NumModeInfo] =
{
     0, // Mode 1   - 0x00
     1, // Mode 2   - 0x01
     2, // Mode 3   - 0x02
    10, // Mode 11  - 0x03
    -1, // Invalid  - 0x04
    -1, // Invalid  - 0x05
     3, // Mode 4   - 0x06
    11, // Mode 12  - 0x07
    -1, // Invalid  - 0x08
    -1, // Invalid  - 0x09
     4, // Mode 5   - 0x0a
    12, // Mode 13  - 0x0b
    -1, // Invalid  - 0x0c
    -1, // Invalid  - 0x0d
     5, // Mode 6   - 0x0e
    13, // Mode 14  - 0x0f
    -1, // Invalid  - 0x10
    -1, // Invalid  - 0x11
     6, // Mode 7   - 0x12
    -1, // Reserved - 0x13
    -1, // Invalid  - 0x14
    -1, // Invalid  - 0x15
     7, // Mode 8   - 0x16
    -1, // Reserved - 0x17
    -1, // Invalid  - 0x18
    -1, // Invalid  - 0x19
     8, // Mode 9   - 0x1a
    -1, // Reserved - 0x1b
    -1, // Invalid  - 0x1c
    -1, // Invalid  - 0x1d
     9, // Mode 10  - 0x1e
    -1, // Resreved - 0x1f
};

// BC7 compression: uPartitions, uPartitionBits, uPBits, uRotationBits, uIndexModeBits, uIndexPrec, uIndexPrec2, RGBAPrec, RGBAPrecWithP
const D3DX_BC7::ModeInfo D3DX_BC7::ms_aInfo[D3DX_BC7::c_NumModes] =
{
    {2, 4, 6, 0, 0, 3, 0, LDRColorA(4,4,4,0), LDRColorA(5,5,5,0)},
        // Mode 0: Color only, 3 Subsets, RGBP 4441 (unique P-bit), 3-bit indecies, 16 partitions
    {1, 6, 2, 0, 0, 3, 0, LDRColorA(6,6,6,0), LDRColorA(7,7,7,0)},
        // Mode 1: Color only, 2 Subsets, RGBP 6661 (shared P-bit), 3-bit indecies, 64 partitions
    {2, 6, 0, 0, 0, 2, 0, LDRColorA(5,5,5,0), LDRColorA(5,5,5,0)},
        // Mode 2: Color only, 3 Subsets, RGB 555, 2-bit indecies, 64 partitions
    {1, 6, 4, 0, 0, 2, 0, LDRColorA(7,7,7,0), LDRColorA(8,8,8,0)},
        // Mode 3: Color only, 2 Subsets, RGBP 7771 (unique P-bit), 2-bits indecies, 64 partitions
    {0, 0, 0, 2, 1, 2, 3, LDRColorA(5,5,5,6), LDRColorA(5,5,5,6)},
        // Mode 4: Color w/ Separate Alpha, 1 Subset, RGB 555, A6, 16x2/16x3-bit indices, 2-bit rotation, 1-bit index selector
    {0, 0, 0, 2, 0, 2, 2, LDRColorA(7,7,7,8), LDRColorA(7,7,7,8)},
        // Mode 5: Color w/ Separate Alpha, 1 Subset, RGB 777, A8, 16x2/16x2-bit indices, 2-bit rotation
    {0, 0, 2, 0, 0, 4, 0, LDRColorA(7,7,7,7), LDRColorA(8,8,8,8)},
        // Mode 6: Color+Alpha, 1 Subset, RGBAP 77771 (unique P-bit), 16x4-bit indecies
    {1, 6, 4, 0, 0, 2, 0, LDRColorA(5,5,5,5), LDRColorA(6,6,6,6)}
        // Mode 7: Color+Alpha, 2 Subsets, RGBAP 55551 (unique P-bit), 2-bit indices, 64 partitions
};


namespace
{
    //-------------------------------------------------------------------------------------
    // Helper functions
    //-------------------------------------------------------------------------------------
    inline bool IsFixUpOffset(size_t uPartitions, size_t uShape, size_t uOffset) noexcept
    {
        assert(uPartitions < 3 && uShape < 64 && uOffset < 16);
        for (size_t p = 0; p <= uPartitions; p++)
        {
            if (uOffset == g_aFixUp[uPartitions][uShape][p])
            {
                return true;
            }
        }
        return false;
    }

    inline void TransformForward(INTEndPntPair aEndPts[]) noexcept
    {
        aEndPts[0].B -= aEndPts[0].A;
        aEndPts[1].A -= aEndPts[0].A;
        aEndPts[1].B -= aEndPts[0].A;
    }

    inline void TransformInverse(INTEndPntPair aEndPts[], const LDRColorA& Prec, bool bSigned) noexcept
    {
        const INTColor WrapMask((1 << Prec.r) - 1, (1 << Prec.g) - 1, (1 << Prec.b) - 1);
        aEndPts[0].B += aEndPts[0].A; aEndPts[0].B &= WrapMask;
        aEndPts[1].A += aEndPts[0].A; aEndPts[1].A &= WrapMask;
        aEndPts[1].B += aEndPts[0].A; aEndPts[1].B &= WrapMask;
        if (bSigned)
        {
            aEndPts[0].B.SignExtend(Prec);
            aEndPts[1].A.SignExtend(Prec);
            aEndPts[1].B.SignExtend(Prec);
        }
    }

    inline float Norm(const INTColor& a, const INTColor& b) noexcept
    {
        const float dr = float(a.r) - float(b.r);
        const float dg = float(a.g) - float(b.g);
        const float db = float(a.b) - float(b.b);
        return dr * dr + dg * dg + db * db;
    }

    // return # of bits needed to store n. handle signed or unsigned cases properly
    inline int NBits(int n, bool bIsSigned) noexcept
    {
        int nb;
        if (n == 0)
        {
            return 0;	// no bits needed for 0, signed or not
        }
        else if (n > 0)
        {
            for (nb = 0; n; ++nb, n >>= 1);
            return nb + (bIsSigned ? 1 : 0);
        }
        else
        {
            assert(bIsSigned);
            for (nb = 0; n < -1; ++nb, n >>= 1);
            return nb + 1;
        }
    }


    //-------------------------------------------------------------------------------------
    float OptimizeRGB(
        const HDRColorA* const pPoints,
        HDRColorA* pX,
        HDRColorA* pY,
        uint32_t cSteps,
        size_t cPixels,
        const size_t* pIndex) noexcept
    {
        constexpr float fError = FLT_MAX;
        const float *pC = (3 == cSteps) ? pC3 : pC4;
        const float *pD = (3 == cSteps) ? pD3 : pD4;

        // Find Min and Max points, as starting point
        HDRColorA X(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
        HDRColorA Y(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

        for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
        {
            if (pPoints[pIndex[iPoint]].r < X.r) X.r = pPoints[pIndex[iPoint]].r;
            if (pPoints[pIndex[iPoint]].g < X.g) X.g = pPoints[pIndex[iPoint]].g;
            if (pPoints[pIndex[iPoint]].b < X.b) X.b = pPoints[pIndex[iPoint]].b;
            if (pPoints[pIndex[iPoint]].r > Y.r) Y.r = pPoints[pIndex[iPoint]].r;
            if (pPoints[pIndex[iPoint]].g > Y.g) Y.g = pPoints[pIndex[iPoint]].g;
            if (pPoints[pIndex[iPoint]].b > Y.b) Y.b = pPoints[pIndex[iPoint]].b;
        }

        // Diagonal axis
        HDRColorA AB;
        AB.r = Y.r - X.r;
        AB.g = Y.g - X.g;
        AB.b = Y.b - X.b;

        const float fAB = AB.r * AB.r + AB.g * AB.g + AB.b * AB.b;

        // Single color block.. no need to root-find
        if (fAB < FLT_MIN)
        {
            pX->r = X.r; pX->g = X.g; pX->b = X.b;
            pY->r = Y.r; pY->g = Y.g; pY->b = Y.b;
            return 0.0f;
        }

        // Try all four axis directions, to determine which diagonal best fits data
        const float fABInv = 1.0f / fAB;

        HDRColorA Dir;
        Dir.r = AB.r * fABInv;
        Dir.g = AB.g * fABInv;
        Dir.b = AB.b * fABInv;

        HDRColorA Mid;
        Mid.r = (X.r + Y.r) * 0.5f;
        Mid.g = (X.g + Y.g) * 0.5f;
        Mid.b = (X.b + Y.b) * 0.5f;

        float fDir[4];
        fDir[0] = fDir[1] = fDir[2] = fDir[3] = 0.0f;

        for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
        {
            HDRColorA Pt;
            Pt.r = (pPoints[pIndex[iPoint]].r - Mid.r) * Dir.r;
            Pt.g = (pPoints[pIndex[iPoint]].g - Mid.g) * Dir.g;
            Pt.b = (pPoints[pIndex[iPoint]].b - Mid.b) * Dir.b;

            float f;
            f = Pt.r + Pt.g + Pt.b; fDir[0] += f * f;
            f = Pt.r + Pt.g - Pt.b; fDir[1] += f * f;
            f = Pt.r - Pt.g + Pt.b; fDir[2] += f * f;
            f = Pt.r - Pt.g - Pt.b; fDir[3] += f * f;
        }

        float fDirMax = fDir[0];
        size_t  iDirMax = 0;

        for (size_t iDir = 1; iDir < 4; iDir++)
        {
            if (fDir[iDir] > fDirMax)
            {
                fDirMax = fDir[iDir];
                iDirMax = iDir;
            }
        }

        if (iDirMax & 2) std::swap(X.g, Y.g);
        if (iDirMax & 1) std::swap(X.b, Y.b);

        // Two color block.. no need to root-find
        if (fAB < 1.0f / 4096.0f)
        {
            pX->r = X.r; pX->g = X.g; pX->b = X.b;
            pY->r = Y.r; pY->g = Y.g; pY->b = Y.b;
            return 0.0f;
        }

        // Use Newton's Method to find local minima of sum-of-squares error.
        auto const fSteps = static_cast<float>(cSteps - 1);

        for (size_t iIteration = 0; iIteration < 8; iIteration++)
        {
            // Calculate new steps
            HDRColorA pSteps[4] = {};

            for (size_t iStep = 0; iStep < cSteps; iStep++)
            {
                pSteps[iStep].r = X.r * pC[iStep] + Y.r * pD[iStep];
                pSteps[iStep].g = X.g * pC[iStep] + Y.g * pD[iStep];
                pSteps[iStep].b = X.b * pC[iStep] + Y.b * pD[iStep];
            }

            // Calculate color direction
            Dir.r = Y.r - X.r;
            Dir.g = Y.g - X.g;
            Dir.b = Y.b - X.b;

            const float fLen = (Dir.r * Dir.r + Dir.g * Dir.g + Dir.b * Dir.b);

            if (fLen < (1.0f / 4096.0f))
                break;

            const float fScale = fSteps / fLen;

            Dir.r *= fScale;
            Dir.g *= fScale;
            Dir.b *= fScale;

            // Evaluate function, and derivatives
            float d2X = 0.0f, d2Y = 0.0f;
            HDRColorA dX(0.0f, 0.0f, 0.0f, 0.0f), dY(0.0f, 0.0f, 0.0f, 0.0f);

            for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
            {
                const float fDot = 
                    (pPoints[pIndex[iPoint]].r - X.r) * Dir.r +
                    (pPoints[pIndex[iPoint]].g - X.g) * Dir.g +
                    (pPoints[pIndex[iPoint]].b - X.b) * Dir.b;

                uint32_t iStep;
                if (fDot <= 0.0f)
                    iStep = 0;
                else if (fDot >= fSteps)
                    iStep = cSteps - 1;
                else
                    iStep = uint32_t(fDot + 0.5f);

                HDRColorA Diff;
                Diff.r = pSteps[iStep].r - pPoints[pIndex[iPoint]].r;
                Diff.g = pSteps[iStep].g - pPoints[pIndex[iPoint]].g;
                Diff.b = pSteps[iStep].b - pPoints[pIndex[iPoint]].b;

                const float fC = pC[iStep] * (1.0f / 8.0f);
                const float fD = pD[iStep] * (1.0f / 8.0f);

                d2X += fC * pC[iStep];
                dX.r += fC * Diff.r;
                dX.g += fC * Diff.g;
                dX.b += fC * Diff.b;

                d2Y += fD * pD[iStep];
                dY.r += fD * Diff.r;
                dY.g += fD * Diff.g;
                dY.b += fD * Diff.b;
            }

            // Move endpoints
            if (d2X > 0.0f)
            {
                const float f = -1.0f / d2X;

                X.r += dX.r * f;
                X.g += dX.g * f;
                X.b += dX.b * f;
            }

            if (d2Y > 0.0f)
            {
                const float f = -1.0f / d2Y;

                Y.r += dY.r * f;
                Y.g += dY.g * f;
                Y.b += dY.b * f;
            }

            if ((dX.r * dX.r < fEpsilon) && (dX.g * dX.g < fEpsilon) && (dX.b * dX.b < fEpsilon) &&
                (dY.r * dY.r < fEpsilon) && (dY.g * dY.g < fEpsilon) && (dY.b * dY.b < fEpsilon))
            {
                break;
            }
        }

        pX->r = X.r; pX->g = X.g; pX->b = X.b;
        pY->r = Y.r; pY->g = Y.g; pY->b = Y.b;
        return fError;
    }

    //-------------------------------------------------------------------------------------

    float OptimizeRGBA(
        const HDRColorA* const pPoints,
        HDRColorA* pX,
        HDRColorA* pY,
        uint32_t cSteps,
        size_t cPixels,
        const size_t* pIndex) noexcept
    {
        constexpr float fError = FLT_MAX;
        const float *pC = (3 == cSteps) ? pC3 : pC4;
        const float *pD = (3 == cSteps) ? pD3 : pD4;

        // Find Min and Max points, as starting point
        HDRColorA X(1.0f, 1.0f, 1.0f, 1.0f);
        HDRColorA Y(0.0f, 0.0f, 0.0f, 0.0f);

        for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
        {
            if (pPoints[pIndex[iPoint]].r < X.r) X.r = pPoints[pIndex[iPoint]].r;
            if (pPoints[pIndex[iPoint]].g < X.g) X.g = pPoints[pIndex[iPoint]].g;
            if (pPoints[pIndex[iPoint]].b < X.b) X.b = pPoints[pIndex[iPoint]].b;
            if (pPoints[pIndex[iPoint]].a < X.a) X.a = pPoints[pIndex[iPoint]].a;
            if (pPoints[pIndex[iPoint]].r > Y.r) Y.r = pPoints[pIndex[iPoint]].r;
            if (pPoints[pIndex[iPoint]].g > Y.g) Y.g = pPoints[pIndex[iPoint]].g;
            if (pPoints[pIndex[iPoint]].b > Y.b) Y.b = pPoints[pIndex[iPoint]].b;
            if (pPoints[pIndex[iPoint]].a > Y.a) Y.a = pPoints[pIndex[iPoint]].a;
        }

        // Diagonal axis
        const HDRColorA AB = Y - X;
        const float fAB = AB * AB;

        // Single color block.. no need to root-find
        if (fAB < FLT_MIN)
        {
            *pX = X;
            *pY = Y;
            return 0.0f;
        }

        // Try all four axis directions, to determine which diagonal best fits data
        const float fABInv = 1.0f / fAB;
        HDRColorA Dir = AB * fABInv;
        const HDRColorA Mid = (X + Y) * 0.5f;

        float fDir[8];
        fDir[0] = fDir[1] = fDir[2] = fDir[3] = fDir[4] = fDir[5] = fDir[6] = fDir[7] = 0.0f;

        for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
        {
            HDRColorA Pt;
            Pt.r = (pPoints[pIndex[iPoint]].r - Mid.r) * Dir.r;
            Pt.g = (pPoints[pIndex[iPoint]].g - Mid.g) * Dir.g;
            Pt.b = (pPoints[pIndex[iPoint]].b - Mid.b) * Dir.b;
            Pt.a = (pPoints[pIndex[iPoint]].a - Mid.a) * Dir.a;

            float f;
            f = Pt.r + Pt.g + Pt.b + Pt.a; fDir[0] += f * f;
            f = Pt.r + Pt.g + Pt.b - Pt.a; fDir[1] += f * f;
            f = Pt.r + Pt.g - Pt.b + Pt.a; fDir[2] += f * f;
            f = Pt.r + Pt.g - Pt.b - Pt.a; fDir[3] += f * f;
            f = Pt.r - Pt.g + Pt.b + Pt.a; fDir[4] += f * f;
            f = Pt.r - Pt.g + Pt.b - Pt.a; fDir[5] += f * f;
            f = Pt.r - Pt.g - Pt.b + Pt.a; fDir[6] += f * f;
            f = Pt.r - Pt.g - Pt.b - Pt.a; fDir[7] += f * f;
        }

        float fDirMax = fDir[0];
        size_t  iDirMax = 0;

        for (size_t iDir = 1; iDir < 8; iDir++)
        {
            if (fDir[iDir] > fDirMax)
            {
                fDirMax = fDir[iDir];
                iDirMax = iDir;
            }
        }

        if (iDirMax & 4) std::swap(X.g, Y.g);
        if (iDirMax & 2) std::swap(X.b, Y.b);
        if (iDirMax & 1) std::swap(X.a, Y.a);

        // Two color block.. no need to root-find
        if (fAB < 1.0f / 4096.0f)
        {
            *pX = X;
            *pY = Y;
            return 0.0f;
        }

        // Use Newton's Method to find local minima of sum-of-squares error.
        const auto fSteps = static_cast<float>(cSteps - 1u);

        for (size_t iIteration = 0; iIteration < 8 && fError > 0.0f; iIteration++)
        {
            // Calculate new steps
            HDRColorA pSteps[BC7_MAX_INDICES];

            LDRColorA lX, lY;
            lX = (X * 255.0f).ToLDRColorA();
            lY = (Y * 255.0f).ToLDRColorA();

            for (size_t iStep = 0; iStep < cSteps; iStep++)
            {
                pSteps[iStep] = X * pC[iStep] + Y * pD[iStep];
                //LDRColorA::Interpolate(lX, lY, i, i, wcprec, waprec, aSteps[i]);
            }

            // Calculate color direction
            Dir = Y - X;
            const float fLen = Dir * Dir;
            if (fLen < (1.0f / 4096.0f))
                break;

            const float fScale = fSteps / fLen;
            Dir *= fScale;

            // Evaluate function, and derivatives
            float d2X = 0.0f, d2Y = 0.0f;
            HDRColorA dX(0.0f, 0.0f, 0.0f, 0.0f), dY(0.0f, 0.0f, 0.0f, 0.0f);

            for (size_t iPoint = 0; iPoint < cPixels; ++iPoint)
            {
                const float fDot = (pPoints[pIndex[iPoint]] - X) * Dir;

                uint32_t iStep;
                if (fDot <= 0.0f)
                    iStep = 0;
                else if (fDot >= fSteps)
                    iStep = cSteps - 1;
                else
                    iStep = uint32_t(fDot + 0.5f);

                const HDRColorA Diff = pSteps[iStep] - pPoints[pIndex[iPoint]];
                const float fC = pC[iStep] * (1.0f / 8.0f);
                const float fD = pD[iStep] * (1.0f / 8.0f);

                d2X += fC * pC[iStep];
                dX += Diff * fC;

                d2Y += fD * pD[iStep];
                dY += Diff * fD;
            }

            // Move endpoints
            if (d2X > 0.0f)
            {
                const float f = -1.0f / d2X;
                X += dX * f;
            }

            if (d2Y > 0.0f)
            {
                const float f = -1.0f / d2Y;
                Y += dY * f;
            }

            if ((dX * dX < fEpsilon) && (dY * dY < fEpsilon))
                break;
        }

        *pX = X;
        *pY = Y;
        return fError;
    }

    //-------------------------------------------------------------------------------------

    float ComputeError(
        const LDRColorA& pixel,
        const LDRColorA aPalette[],
        uint8_t uIndexPrec,
        uint8_t uIndexPrec2,
        size_t* pBestIndex = nullptr,
        size_t* pBestIndex2 = nullptr) noexcept
    {
        const size_t uNumIndices = size_t(1) << uIndexPrec;
        const size_t uNumIndices2 = size_t(1) << uIndexPrec2;
        float fTotalErr = 0;
        float fBestErr = FLT_MAX;

        if (pBestIndex)
            *pBestIndex = 0;
        if (pBestIndex2)
            *pBestIndex2 = 0;

        const float32x4 vpixel = pixel; // conversion

        if (uIndexPrec2 == 0)
        {
            for (size_t i = 0; i < uNumIndices && fBestErr > 0; i++)
            {
                float32x4 tpixel = aPalette[i]; // conversion
                // Compute ErrorMetric
                tpixel = vpixel - tpixel;
                const float fErr = dot(tpixel, tpixel);
                if (fErr > fBestErr)	// error increased, so we're done searching
                    break;
                if (fErr < fBestErr)
                {
                    fBestErr = fErr;
                    if (pBestIndex)
                        *pBestIndex = i;
                }
            }
            fTotalErr += fBestErr;
        }
        else
        {
            for (size_t i = 0; i < uNumIndices && fBestErr > 0; i++)
            {
                float32x4 tpixel = aPalette[i]; // conversion
                // Compute ErrorMetricRGB
                tpixel = vpixel - tpixel;
                const float fErr = dot(tpixel, tpixel);
                if (fErr > fBestErr)	// error increased, so we're done searching
                    break;
                if (fErr < fBestErr)
                {
                    fBestErr = fErr;
                    if (pBestIndex)
                        *pBestIndex = i;
                }
            }
            fTotalErr += fBestErr;
            fBestErr = FLT_MAX;
            for (size_t i = 0; i < uNumIndices2 && fBestErr > 0; i++)
            {
                // Compute ErrorMetricAlpha
                const float ea = float(pixel.a) - float(aPalette[i].a);
                const float fErr = ea*ea;
                if (fErr > fBestErr)	// error increased, so we're done searching
                    break;
                if (fErr < fBestErr)
                {
                    fBestErr = fErr;
                    if (pBestIndex2)
                        *pBestIndex2 = i;
                }
            }
            fTotalErr += fBestErr;
        }

        return fTotalErr;
    }

    void FillColorF64(u8* output, size_t stride, float16x4 color) noexcept
    {
        for (int y = 0; y < 4; ++y)
        {
            float16x4* dest = reinterpret_cast<float16x4*>(output + y * stride);
            for (int x = 0; x < 4; ++x)
            {
                dest[x] = color;
            }
        }
    }

    void FillWithErrorColorF64(u8* output, size_t stride) noexcept
    {
    #ifdef _DEBUG
        // Use Magenta in debug as a highly-visible error color
        float16x4 color = float16x4(1.0f, 0.0f, 1.0f, 1.0f);
    #else
        // In production use, default to black
        float16x4 color = float16x4(0.0f, 0.0f, 0.0f, 1.0f);
    #endif

        FillColorF64(output, stride, color);
    }

    void FillColorU32(u8* output, size_t stride, u32 color) noexcept
    {
        for (int y = 0; y < 4; ++y)
        {
            u32* dest = reinterpret_cast<u32*>(output + y * stride);
            for (int x = 0; x < 4; ++x)
            {
                dest[x] = color;
            }
        }
    }

    void FillWithErrorColorU32(u8* output, size_t stride) noexcept
    {
    #ifdef _DEBUG
        // Use Magenta in debug as a highly-visible error color
        u32 color = 0xffff00ff;
    #else
        // In production use, default to black
        u32 color = 0xff000000;
    #endif

        FillColorU32(output, stride, color);
    }

} // namespace

//-------------------------------------------------------------------------------------
// BC6H Compression
//-------------------------------------------------------------------------------------

void D3DX_BC6H::Decode(bool bSigned, u8* output, size_t stride) const noexcept
{
    assert(output);

    size_t uStartBit = 0;
    u8 uMode = GetBits(uStartBit, 2u);
    if (uMode != 0x00 && uMode != 0x01)
    {
        uMode = u8((unsigned(GetBits(uStartBit, 3)) << 2) | uMode);
    }

    assert(uMode < c_NumModeInfo);

    if (ms_aModeToInfo[uMode] >= 0)
    {
        assert(static_cast<unsigned int>(ms_aModeToInfo[uMode]) < c_NumModes);

        const ModeDescriptor* desc = ms_aDesc[ms_aModeToInfo[uMode]];
        const ModeInfo& info = ms_aInfo[ms_aModeToInfo[uMode]];

        INTEndPntPair aEndPts[BC6H_MAX_REGIONS] = {};
        uint32_t uShape = 0;

        // Read header
        const size_t uHeaderBits = info.uPartitions > 0 ? 82u : 65u;
        while (uStartBit < uHeaderBits)
        {
            const size_t uCurBit = uStartBit;
            if (GetBit(uStartBit))
            {
                switch (desc[uCurBit].m_eField)
                {
                case D:  uShape |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case RW: aEndPts[0].A.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case RX: aEndPts[0].B.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case RY: aEndPts[1].A.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case RZ: aEndPts[1].B.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case GW: aEndPts[0].A.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case GX: aEndPts[0].B.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case GY: aEndPts[1].A.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case GZ: aEndPts[1].B.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case BW: aEndPts[0].A.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case BX: aEndPts[0].B.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case BY: aEndPts[1].A.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case BZ: aEndPts[1].B.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                default:
                    {
                        debugPrint("BC6H: Invalid header bits encountered during decoding\n");
                        FillWithErrorColorF64(output, stride);
                        return;
                    }
                }
            }
        }

        assert(uShape < 64);

        // Sign extend necessary end points
        if (bSigned)
        {
            aEndPts[0].A.SignExtend(info.RGBAPrec[0][0]);
        }
        if (bSigned || info.bTransformed)
        {
            assert(info.uPartitions < BC6H_MAX_REGIONS);

            for (size_t p = 0; p <= info.uPartitions; ++p)
            {
                if (p != 0)
                {
                    aEndPts[p].A.SignExtend(info.RGBAPrec[p][0]);
                }
                aEndPts[p].B.SignExtend(info.RGBAPrec[p][1]);
            }
        }

        // Inverse transform the end points
        if (info.bTransformed)
        {
            TransformInverse(aEndPts, info.RGBAPrec[0][0], bSigned);
        }

        // Read indices
        for (int y = 0; y < 4; ++y)
        {
            float16x4* dest = reinterpret_cast<float16x4*>(output + y * stride);

            for (int x = 0; x < 4; ++x)
            {
                int idx = y * 4 + x;

                const size_t uNumBits = IsFixUpOffset(info.uPartitions, uShape, idx) ? info.uIndexPrec - 1u : info.uIndexPrec;
                if (uStartBit + uNumBits > 128)
                {
                    debugPrint("BC6H: Invalid block encountered during decoding\n");
                    FillWithErrorColorF64(output, stride);
                    return;
                }
                const uint8_t uIndex = GetBits(uStartBit, uNumBits);

                if (uIndex >= ((info.uPartitions > 0) ? 8 : 16))
                {
                    debugPrint("BC6H: Invalid index encountered during decoding\n");
                    FillWithErrorColorF64(output, stride);
                    return;
                }

                const size_t uRegion = g_aPartitionTable[info.uPartitions][uShape][idx];
                assert(uRegion < BC6H_MAX_REGIONS);

                // Unquantize endpoints and interpolate
                const int r1 = Unquantize(aEndPts[uRegion].A.r, info.RGBAPrec[0][0].r, bSigned);
                const int g1 = Unquantize(aEndPts[uRegion].A.g, info.RGBAPrec[0][0].g, bSigned);
                const int b1 = Unquantize(aEndPts[uRegion].A.b, info.RGBAPrec[0][0].b, bSigned);
                const int r2 = Unquantize(aEndPts[uRegion].B.r, info.RGBAPrec[0][0].r, bSigned);
                const int g2 = Unquantize(aEndPts[uRegion].B.g, info.RGBAPrec[0][0].g, bSigned);
                const int b2 = Unquantize(aEndPts[uRegion].B.b, info.RGBAPrec[0][0].b, bSigned);
                const int* aWeights = info.uPartitions > 0 ? g_aWeights3 : g_aWeights4;

                int c0 = FinishUnquantize((r1 * (BC67_WEIGHT_MAX - aWeights[uIndex]) + r2 * aWeights[uIndex] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT, bSigned);
                int c1 = FinishUnquantize((g1 * (BC67_WEIGHT_MAX - aWeights[uIndex]) + g2 * aWeights[uIndex] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT, bSigned);
                int c2 = FinishUnquantize((b1 * (BC67_WEIGHT_MAX - aWeights[uIndex]) + b2 * aWeights[uIndex] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT, bSigned);

                float16 r = INT2F16(c0, bSigned);
                float16 g = INT2F16(c1, bSigned);
                float16 b = INT2F16(c2, bSigned);

                dest[x] = float16x4(r, g, b, float16(1.0f));
            }
        }
    }
    else
    {
        const char* warnstr = "BC6H: Invalid mode encountered during decoding\n";
        switch (uMode)
        {
        case 0x13:  warnstr = "BC6H: Reserved mode 10011 encountered during decoding\n"; break;
        case 0x17:  warnstr = "BC6H: Reserved mode 10111 encountered during decoding\n"; break;
        case 0x1B:  warnstr = "BC6H: Reserved mode 11011 encountered during decoding\n"; break;
        case 0x1F:  warnstr = "BC6H: Reserved mode 11111 encountered during decoding\n"; break;
        }
        debugPrint(warnstr);

        // Per the BC6H format spec, we must return opaque black
        FillColorF64(output, stride, float16x4(0.0f, 0.0f, 0.0f, 1.0f));
    }
}

void D3DX_BC6H::Encode(bool bSigned, const u8* input, size_t stride) noexcept
{
    assert(input);

    EncodeParams EP(input, stride, bSigned);

    for (EP.uMode = 0; EP.uMode < c_NumModes && EP.fBestErr > 0; ++EP.uMode)
    {
        const uint8_t uShapes = ms_aInfo[EP.uMode].uPartitions ? 32u : 1u;
        // Number of rough cases to look at. reasonable values of this are 1, uShapes/4, and uShapes
        // uShapes/4 gets nearly all the cases; you can increase that a bit (say by 3 or 4) if you really want to squeeze the last bit out
        const size_t uItems = std::max<size_t>(1u, size_t(uShapes >> 2));
        float afRoughMSE[BC6H_MAX_SHAPES];
        uint8_t auShape[BC6H_MAX_SHAPES];

        // pick the best uItems shapes and refine these.
        for (EP.uShape = 0; EP.uShape < uShapes; ++EP.uShape)
        {
            size_t uShape = EP.uShape;
            afRoughMSE[uShape] = RoughMSE(&EP);
            auShape[uShape] = static_cast<uint8_t>(uShape);
        }

        // Bubble up the first uItems items
        for (size_t i = 0; i < uItems; i++)
        {
            for (size_t j = i + 1; j < uShapes; j++)
            {
                if (afRoughMSE[i] > afRoughMSE[j])
                {
                    std::swap(afRoughMSE[i], afRoughMSE[j]);
                    std::swap(auShape[i], auShape[j]);
                }
            }
        }

        for (size_t i = 0; i < uItems && EP.fBestErr > 0; i++)
        {
            EP.uShape = auShape[i];
            Refine(&EP);
        }
    }
}

//-------------------------------------------------------------------------------------

int D3DX_BC6H::Quantize(int iValue, int prec, bool bSigned) noexcept
{
    assert(prec > 1);	// didn't bother to make it work for 1
    int q, s = 0;
    if (bSigned)
    {
        assert(iValue >= -F16MAX && iValue <= F16MAX);
        if (iValue < 0)
        {
            s = 1;
            iValue = -iValue;
        }
        q = (prec >= 16) ? iValue : (iValue << (prec - 1)) / (F16MAX + 1);
        if (s)
            q = -q;
        assert(q > -(1 << (prec - 1)) && q < (1 << (prec - 1)));
    }
    else
    {
        assert(iValue >= 0 && iValue <= F16MAX);
        q = (prec >= 15) ? iValue : (iValue << prec) / (F16MAX + 1);
        assert(q >= 0 && q < (1 << prec));
    }

    return q;
}

int D3DX_BC6H::Unquantize(int comp, uint8_t uBitsPerComp, bool bSigned) noexcept
{
    int unq = 0, s = 0;
    if (bSigned)
    {
        if (uBitsPerComp >= 16)
        {
            unq = comp;
        }
        else
        {
            if (comp < 0)
            {
                s = 1;
                comp = -comp;
            }

            if (comp == 0) unq = 0;
            else if (comp >= ((1 << (uBitsPerComp - 1)) - 1)) unq = 0x7FFF;
            else unq = ((comp << 15) + 0x4000) >> (uBitsPerComp - 1);

            if (s) unq = -unq;
        }
    }
    else
    {
        if (uBitsPerComp >= 15) unq = comp;
        else if (comp == 0) unq = 0;
        else if (comp == ((1 << uBitsPerComp) - 1)) unq = 0xFFFF;
        else unq = ((comp << 16) + 0x8000) >> uBitsPerComp;
    }

    return unq;
}

int D3DX_BC6H::FinishUnquantize(int comp, bool bSigned) noexcept
{
    if (bSigned)
    {
        return (comp < 0) ? -(((-comp) * 31) >> 5) : (comp * 31) >> 5;  // scale the magnitude by 31/32
    }
    else
    {
        return (comp * 31) >> 6;                                        // scale the magnitude by 31/64
    }
}

//-------------------------------------------------------------------------------------

bool D3DX_BC6H::EndPointsFit(const EncodeParams* pEP, const INTEndPntPair aEndPts[]) noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const bool bTransformed = ms_aInfo[pEP->uMode].bTransformed;
    const bool bIsSigned = pEP->bSigned;
    const LDRColorA& Prec0 = ms_aInfo[pEP->uMode].RGBAPrec[0][0];
    const LDRColorA& Prec1 = ms_aInfo[pEP->uMode].RGBAPrec[0][1];
    const LDRColorA& Prec2 = ms_aInfo[pEP->uMode].RGBAPrec[1][0];
    const LDRColorA& Prec3 = ms_aInfo[pEP->uMode].RGBAPrec[1][1];

    INTColor aBits[4];
    aBits[0].r = NBits(aEndPts[0].A.r, bIsSigned);
    aBits[0].g = NBits(aEndPts[0].A.g, bIsSigned);
    aBits[0].b = NBits(aEndPts[0].A.b, bIsSigned);
    aBits[1].r = NBits(aEndPts[0].B.r, bTransformed || bIsSigned);
    aBits[1].g = NBits(aEndPts[0].B.g, bTransformed || bIsSigned);
    aBits[1].b = NBits(aEndPts[0].B.b, bTransformed || bIsSigned);
    if (aBits[0].r > Prec0.r || aBits[1].r > Prec1.r ||
        aBits[0].g > Prec0.g || aBits[1].g > Prec1.g ||
        aBits[0].b > Prec0.b || aBits[1].b > Prec1.b)
        return false;

    if (ms_aInfo[pEP->uMode].uPartitions)
    {
        aBits[2].r = NBits(aEndPts[1].A.r, bTransformed || bIsSigned);
        aBits[2].g = NBits(aEndPts[1].A.g, bTransformed || bIsSigned);
        aBits[2].b = NBits(aEndPts[1].A.b, bTransformed || bIsSigned);
        aBits[3].r = NBits(aEndPts[1].B.r, bTransformed || bIsSigned);
        aBits[3].g = NBits(aEndPts[1].B.g, bTransformed || bIsSigned);
        aBits[3].b = NBits(aEndPts[1].B.b, bTransformed || bIsSigned);

        if (aBits[2].r > Prec2.r || aBits[3].r > Prec3.r ||
            aBits[2].g > Prec2.g || aBits[3].g > Prec3.g ||
            aBits[2].b > Prec2.b || aBits[3].b > Prec3.b)
            return false;
    }

    return true;
}

void D3DX_BC6H::GeneratePaletteQuantized(const EncodeParams* pEP, const INTEndPntPair& endPts, INTColor aPalette[]) const noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const size_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uNumIndices = size_t(1) << uIndexPrec;
    assert(uNumIndices > 0);
    const LDRColorA& Prec = ms_aInfo[pEP->uMode].RGBAPrec[0][0];

    // scale endpoints
    INTEndPntPair unqEndPts;
    unqEndPts.A.r = Unquantize(endPts.A.r, Prec.r, pEP->bSigned);
    unqEndPts.A.g = Unquantize(endPts.A.g, Prec.g, pEP->bSigned);
    unqEndPts.A.b = Unquantize(endPts.A.b, Prec.b, pEP->bSigned);
    unqEndPts.B.r = Unquantize(endPts.B.r, Prec.r, pEP->bSigned);
    unqEndPts.B.g = Unquantize(endPts.B.g, Prec.g, pEP->bSigned);
    unqEndPts.B.b = Unquantize(endPts.B.b, Prec.b, pEP->bSigned);

    // interpolate
    const int* aWeights = nullptr;
    switch (uIndexPrec)
    {
    case 3: aWeights = g_aWeights3; assert(uNumIndices <= 8); break;
    case 4: aWeights = g_aWeights4; assert(uNumIndices <= 16); break;
    default:
        assert(false);
        for (size_t i = 0; i < uNumIndices; ++i)
        {
            aPalette[i] = INTColor(0, 0, 0);
        }
        return;
    }

    for (size_t i = 0; i < uNumIndices; ++i)
    {
        aPalette[i].r = FinishUnquantize(
            (unqEndPts.A.r * (BC67_WEIGHT_MAX - aWeights[i]) + unqEndPts.B.r * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT,
            pEP->bSigned);
        aPalette[i].g = FinishUnquantize(
            (unqEndPts.A.g * (BC67_WEIGHT_MAX - aWeights[i]) + unqEndPts.B.g * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT,
            pEP->bSigned);
        aPalette[i].b = FinishUnquantize(
            (unqEndPts.A.b * (BC67_WEIGHT_MAX - aWeights[i]) + unqEndPts.B.b * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT,
            pEP->bSigned);
    }
}

// given a collection of colors and quantized endpoints, generate a palette, choose best entries, and return a single toterr
float D3DX_BC6H::MapColorsQuantized(const EncodeParams* pEP, const INTColor aColors[], size_t np, const INTEndPntPair &endPts) const noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const uint8_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    auto const uNumIndices = static_cast<const uint8_t>(1u << uIndexPrec);
    INTColor aPalette[BC6H_MAX_INDICES];
    GeneratePaletteQuantized(pEP, endPts, aPalette);

    float fTotErr = 0;
    for (size_t i = 0; i < np; ++i)
    {
        const float32x4 vcolors = aColors[i]; // conversion

        // Compute ErrorMetricRGB
        float32x4 tpal = aPalette[0]; // conversion
        tpal = vcolors - tpal;
        float fBestErr = dot(tpal, tpal);

        for (int j = 1; j < uNumIndices && fBestErr > 0; ++j)
        {
            // Compute ErrorMetricRGB
            tpal = aPalette[j]; // conversion
            tpal = vcolors - tpal;
            const float fErr = dot(tpal, tpal);
            if (fErr > fBestErr) break;     // error increased, so we're done searching
            if (fErr < fBestErr) fBestErr = fErr;
        }
        fTotErr += fBestErr;
    }
    return fTotErr;
}

float D3DX_BC6H::PerturbOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, uint8_t ch,
    const INTEndPntPair& oldEndPts, INTEndPntPair& newEndPts, float fOldErr, int do_b) const noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    uint8_t uPrec;
    switch (ch)
    {
    case 0: uPrec = ms_aInfo[pEP->uMode].RGBAPrec[0][0].r; break;
    case 1: uPrec = ms_aInfo[pEP->uMode].RGBAPrec[0][0].g; break;
    case 2: uPrec = ms_aInfo[pEP->uMode].RGBAPrec[0][0].b; break;
    default: assert(false); newEndPts = oldEndPts; return FLT_MAX;
    }
    INTEndPntPair tmpEndPts;
    float fMinErr = fOldErr;
    int beststep = 0;

    // copy real endpoints so we can perturb them
    tmpEndPts = newEndPts = oldEndPts;

    // do a logarithmic search for the best error for this endpoint (which)
    for (int step = 1 << (uPrec - 1); step; step >>= 1)
    {
        bool bImproved = false;
        for (int sign = -1; sign <= 1; sign += 2)
        {
            if (do_b == 0)
            {
                tmpEndPts.A[ch] = newEndPts.A[ch] + sign * step;
                if (tmpEndPts.A[ch] < 0 || tmpEndPts.A[ch] >= (1 << uPrec))
                    continue;
            }
            else
            {
                tmpEndPts.B[ch] = newEndPts.B[ch] + sign * step;
                if (tmpEndPts.B[ch] < 0 || tmpEndPts.B[ch] >= (1 << uPrec))
                    continue;
            }

            const float fErr = MapColorsQuantized(pEP, aColors, np, tmpEndPts);

            if (fErr < fMinErr)
            {
                bImproved = true;
                fMinErr = fErr;
                beststep = sign * step;
            }
        }
        // if this was an improvement, move the endpoint and continue search from there
        if (bImproved)
        {
            if (do_b == 0)
                newEndPts.A[ch] += beststep;
            else
                newEndPts.B[ch] += beststep;
        }
    }
    return fMinErr;
}

void D3DX_BC6H::OptimizeOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, float aOrgErr,
    const INTEndPntPair &aOrgEndPts, INTEndPntPair &aOptEndPts) const noexcept
{
    assert(pEP);
    float aOptErr = aOrgErr;
    aOptEndPts.A = aOrgEndPts.A;
    aOptEndPts.B = aOrgEndPts.B;

    INTEndPntPair new_a, new_b;
    INTEndPntPair newEndPts;
    int do_b;

    // now optimize each channel separately
    for (uint8_t ch = 0; ch < BC6H_NUM_CHANNELS; ++ch)
    {
        // figure out which endpoint when perturbed gives the most improvement and start there
        // if we just alternate, we can easily end up in a local minima
        const float fErr0 = PerturbOne(pEP, aColors, np, ch, aOptEndPts, new_a, aOptErr, 0);	// perturb endpt A
        const float fErr1 = PerturbOne(pEP, aColors, np, ch, aOptEndPts, new_b, aOptErr, 1);	// perturb endpt B

        if (fErr0 < fErr1)
        {
            if (fErr0 >= aOptErr) continue;
            aOptEndPts.A[ch] = new_a.A[ch];
            aOptErr = fErr0;
            do_b = 1;		// do B next
        }
        else
        {
            if (fErr1 >= aOptErr) continue;
            aOptEndPts.B[ch] = new_b.B[ch];
            aOptErr = fErr1;
            do_b = 0;		// do A next
        }

        // now alternate endpoints and keep trying until there is no improvement
        for (;;)
        {
            const float fErr = PerturbOne(pEP, aColors, np, ch, aOptEndPts, newEndPts, aOptErr, do_b);
            if (fErr >= aOptErr)
                break;
            if (do_b == 0)
                aOptEndPts.A[ch] = newEndPts.A[ch];
            else
                aOptEndPts.B[ch] = newEndPts.B[ch];
            aOptErr = fErr;
            do_b = 1 - do_b;	// now move the other endpoint
        }
    }
}

void D3DX_BC6H::OptimizeEndPoints(const EncodeParams* pEP, const float aOrgErr[], const INTEndPntPair aOrgEndPts[], INTEndPntPair aOptEndPts[]) const noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC6H_MAX_REGIONS);
    INTColor aPixels[NUM_PIXELS_PER_BLOCK];

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        // collect the pixels in the region
        size_t np = 0;
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            if (g_aPartitionTable[p][pEP->uShape][i] == p)
            {
                aPixels[np++] = pEP->aIPixels[i];
            }
        }

        OptimizeOne(pEP, aPixels, np, aOrgErr[p], aOrgEndPts[p], aOptEndPts[p]);
    }
}


// Swap endpoints as needed to ensure that the indices at fix up have a 0 high-order bit

void D3DX_BC6H::SwapIndices(const EncodeParams* pEP, INTEndPntPair aEndPts[], size_t aIndices[]) noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const size_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    const size_t uNumIndices = size_t(1) << ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uHighIndexBit = uNumIndices >> 1;

    assert(uPartitions < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        const size_t i = g_aFixUp[uPartitions][pEP->uShape][p];
        assert(g_aPartitionTable[uPartitions][pEP->uShape][i] == p);
        if (aIndices[i] & uHighIndexBit)
        {
            // high bit is set, swap the aEndPts and indices for this region
            std::swap(aEndPts[p].A, aEndPts[p].B);

            for (size_t j = 0; j < NUM_PIXELS_PER_BLOCK; ++j)
                if (g_aPartitionTable[uPartitions][pEP->uShape][j] == p)
                    aIndices[j] = uNumIndices - 1 - aIndices[j];
        }
    }
}


// assign indices given a tile, shape, and quantized endpoints, return toterr for each region

void D3DX_BC6H::AssignIndices(const EncodeParams* pEP, const INTEndPntPair aEndPts[], size_t aIndices[], float aTotErr[]) const noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    auto const uNumIndices = static_cast<const uint8_t>(1u << ms_aInfo[pEP->uMode].uIndexPrec);

    assert(uPartitions < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);

    // build list of possibles
    INTColor aPalette[BC6H_MAX_REGIONS][BC6H_MAX_INDICES];

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        GeneratePaletteQuantized(pEP, aEndPts[p], aPalette[p]);
        aTotErr[p] = 0;
    }

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        const uint8_t uRegion = g_aPartitionTable[uPartitions][pEP->uShape][i];
        assert(uRegion < BC6H_MAX_REGIONS);
        float fBestErr = Norm(pEP->aIPixels[i], aPalette[uRegion][0]);
        aIndices[i] = 0;

        for (uint8_t j = 1; j < uNumIndices && fBestErr > 0; ++j)
        {
            const float fErr = Norm(pEP->aIPixels[i], aPalette[uRegion][j]);
            if (fErr > fBestErr) break;	// error increased, so we're done searching
            if (fErr < fBestErr)
            {
                fBestErr = fErr;
                aIndices[i] = j;
            }
        }
        aTotErr[uRegion] += fBestErr;
    }
}

void D3DX_BC6H::QuantizeEndPts(const EncodeParams* pEP, INTEndPntPair* aQntEndPts) const noexcept
{
    assert(pEP && aQntEndPts);
    assert(pEP->uMode < c_NumModes);

    const INTEndPntPair* aUnqEndPts = pEP->aUnqEndPts[pEP->uShape];
    const LDRColorA& Prec = ms_aInfo[pEP->uMode].RGBAPrec[0][0];
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC6H_MAX_REGIONS);

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        aQntEndPts[p].A.r = Quantize(aUnqEndPts[p].A.r, Prec.r, pEP->bSigned);
        aQntEndPts[p].A.g = Quantize(aUnqEndPts[p].A.g, Prec.g, pEP->bSigned);
        aQntEndPts[p].A.b = Quantize(aUnqEndPts[p].A.b, Prec.b, pEP->bSigned);
        aQntEndPts[p].B.r = Quantize(aUnqEndPts[p].B.r, Prec.r, pEP->bSigned);
        aQntEndPts[p].B.g = Quantize(aUnqEndPts[p].B.g, Prec.g, pEP->bSigned);
        aQntEndPts[p].B.b = Quantize(aUnqEndPts[p].B.b, Prec.b, pEP->bSigned);
    }
}

void D3DX_BC6H::EmitBlock(const EncodeParams* pEP, const INTEndPntPair aEndPts[], const size_t aIndices[]) noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const uint8_t uRealMode = ms_aInfo[pEP->uMode].uMode;
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    const uint8_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uHeaderBits = uPartitions > 0 ? 82u : 65u;
    const ModeDescriptor* desc = ms_aDesc[pEP->uMode];
    size_t uStartBit = 0;

    while (uStartBit < uHeaderBits)
    {
        switch (desc[uStartBit].m_eField)
        {
        case M:  SetBit(uStartBit, uint8_t(uRealMode >> desc[uStartBit].m_uBit) & 0x01u); break;
        case D:  SetBit(uStartBit, uint8_t(pEP->uShape >> desc[uStartBit].m_uBit) & 0x01u); break;
        case RW: SetBit(uStartBit, uint8_t(aEndPts[0].A.r >> desc[uStartBit].m_uBit) & 0x01u); break;
        case RX: SetBit(uStartBit, uint8_t(aEndPts[0].B.r >> desc[uStartBit].m_uBit) & 0x01u); break;
        case RY: SetBit(uStartBit, uint8_t(aEndPts[1].A.r >> desc[uStartBit].m_uBit) & 0x01u); break;
        case RZ: SetBit(uStartBit, uint8_t(aEndPts[1].B.r >> desc[uStartBit].m_uBit) & 0x01u); break;
        case GW: SetBit(uStartBit, uint8_t(aEndPts[0].A.g >> desc[uStartBit].m_uBit) & 0x01u); break;
        case GX: SetBit(uStartBit, uint8_t(aEndPts[0].B.g >> desc[uStartBit].m_uBit) & 0x01u); break;
        case GY: SetBit(uStartBit, uint8_t(aEndPts[1].A.g >> desc[uStartBit].m_uBit) & 0x01u); break;
        case GZ: SetBit(uStartBit, uint8_t(aEndPts[1].B.g >> desc[uStartBit].m_uBit) & 0x01u); break;
        case BW: SetBit(uStartBit, uint8_t(aEndPts[0].A.b >> desc[uStartBit].m_uBit) & 0x01u); break;
        case BX: SetBit(uStartBit, uint8_t(aEndPts[0].B.b >> desc[uStartBit].m_uBit) & 0x01u); break;
        case BY: SetBit(uStartBit, uint8_t(aEndPts[1].A.b >> desc[uStartBit].m_uBit) & 0x01u); break;
        case BZ: SetBit(uStartBit, uint8_t(aEndPts[1].B.b >> desc[uStartBit].m_uBit) & 0x01u); break;
        default: assert(false);
        }
    }

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        if (IsFixUpOffset(ms_aInfo[pEP->uMode].uPartitions, pEP->uShape, i))
            SetBits(uStartBit, uIndexPrec - 1u, static_cast<uint8_t>(aIndices[i]));
        else
            SetBits(uStartBit, uIndexPrec, static_cast<uint8_t>(aIndices[i]));
    }
    assert(uStartBit == 128);
}

void D3DX_BC6H::Refine(EncodeParams* pEP) noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC6H_MAX_REGIONS);

    const bool bTransformed = ms_aInfo[pEP->uMode].bTransformed;
    float aOrgErr[BC6H_MAX_REGIONS], aOptErr[BC6H_MAX_REGIONS];
    INTEndPntPair aOrgEndPts[BC6H_MAX_REGIONS], aOptEndPts[BC6H_MAX_REGIONS];
    size_t aOrgIdx[NUM_PIXELS_PER_BLOCK], aOptIdx[NUM_PIXELS_PER_BLOCK];

    QuantizeEndPts(pEP, aOrgEndPts);
    AssignIndices(pEP, aOrgEndPts, aOrgIdx, aOrgErr);
    SwapIndices(pEP, aOrgEndPts, aOrgIdx);

    if (bTransformed) TransformForward(aOrgEndPts);
    if (EndPointsFit(pEP, aOrgEndPts))
    {
        if (bTransformed) TransformInverse(aOrgEndPts, ms_aInfo[pEP->uMode].RGBAPrec[0][0], pEP->bSigned);
        OptimizeEndPoints(pEP, aOrgErr, aOrgEndPts, aOptEndPts);
        AssignIndices(pEP, aOptEndPts, aOptIdx, aOptErr);
        SwapIndices(pEP, aOptEndPts, aOptIdx);

        float fOrgTotErr = 0.0f, fOptTotErr = 0.0f;
        for (size_t p = 0; p <= uPartitions; ++p)
        {
            fOrgTotErr += aOrgErr[p];
            fOptTotErr += aOptErr[p];
        }

        if (bTransformed) TransformForward(aOptEndPts);
        if (EndPointsFit(pEP, aOptEndPts) && fOptTotErr < fOrgTotErr && fOptTotErr < pEP->fBestErr)
        {
            pEP->fBestErr = fOptTotErr;
            EmitBlock(pEP, aOptEndPts, aOptIdx);
        }
        else if (fOrgTotErr < pEP->fBestErr)
        {
            // either it stopped fitting when we optimized it, or there was no improvement
            // so go back to the unoptimized endpoints which we know will fit
            if (bTransformed) TransformForward(aOrgEndPts);
            pEP->fBestErr = fOrgTotErr;
            EmitBlock(pEP, aOrgEndPts, aOrgIdx);
        }
    }
}

void D3DX_BC6H::GeneratePaletteUnquantized(const EncodeParams* pEP, size_t uRegion, INTColor aPalette[]) noexcept
{
    assert(pEP);
    assert(uRegion < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);
    assert(pEP->uMode < c_NumModes);

    const INTEndPntPair& endPts = pEP->aUnqEndPts[pEP->uShape][uRegion];
    const uint8_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    auto const uNumIndices = static_cast<const uint8_t>(1u << uIndexPrec);
    assert(uNumIndices > 0);

    const int* aWeights = nullptr;
    switch (uIndexPrec)
    {
    case 3: aWeights = g_aWeights3; assert(uNumIndices <= 8); break;
    case 4: aWeights = g_aWeights4; assert(uNumIndices <= 16); break;
    default:
        assert(false);
        for (size_t i = 0; i < uNumIndices; ++i)
        {
            aPalette[i] = INTColor(0, 0, 0);
        }
        return;
    }

    for (size_t i = 0; i < uNumIndices; ++i)
    {
        aPalette[i].r = (endPts.A.r * (BC67_WEIGHT_MAX - aWeights[i]) + endPts.B.r * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT;
        aPalette[i].g = (endPts.A.g * (BC67_WEIGHT_MAX - aWeights[i]) + endPts.B.g * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT;
        aPalette[i].b = (endPts.A.b * (BC67_WEIGHT_MAX - aWeights[i]) + endPts.B.b * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT;
    }
}

float D3DX_BC6H::MapColors(const EncodeParams* pEP, size_t uRegion, size_t np, const size_t* auIndex) const noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const uint8_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    auto const uNumIndices = static_cast<const uint8_t>(1u << uIndexPrec);
    INTColor aPalette[BC6H_MAX_INDICES];
    GeneratePaletteUnquantized(pEP, uRegion, aPalette);

    float fTotalErr = 0.0f;
    for (size_t i = 0; i < np; ++i)
    {
        float fBestErr = Norm(pEP->aIPixels[auIndex[i]], aPalette[0]);
        for (uint8_t j = 1; j < uNumIndices && fBestErr > 0.0f; ++j)
        {
            const float fErr = Norm(pEP->aIPixels[auIndex[i]], aPalette[j]);
            if (fErr > fBestErr) break;      // error increased, so we're done searching
            if (fErr < fBestErr) fBestErr = fErr;
        }
        fTotalErr += fBestErr;
    }

    return fTotalErr;
}

float D3DX_BC6H::RoughMSE(EncodeParams* pEP) const noexcept
{
    assert(pEP);
    assert(pEP->uShape < BC6H_MAX_SHAPES);
    assert(pEP->uMode < c_NumModes);

    INTEndPntPair* aEndPts = pEP->aUnqEndPts[pEP->uShape];

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC6H_MAX_REGIONS);

    size_t auPixIdx[NUM_PIXELS_PER_BLOCK];

    float fError = 0.0f;
    for (size_t p = 0; p <= uPartitions; ++p)
    {
        size_t np = 0;
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            if (g_aPartitionTable[uPartitions][pEP->uShape][i] == p)
            {
                auPixIdx[np++] = i;
            }
        }

        // handle simple cases
        assert(np > 0);
        if (np == 1)
        {
            aEndPts[p].A = pEP->aIPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aIPixels[auPixIdx[0]];
            continue;
        }
        else if (np == 2)
        {
            aEndPts[p].A = pEP->aIPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aIPixels[auPixIdx[1]];
            continue;
        }

        HDRColorA epA, epB;
        OptimizeRGB(pEP->aHDRPixels, &epA, &epB, 4, np, auPixIdx);
        aEndPts[p].A.Set(epA, pEP->bSigned);
        aEndPts[p].B.Set(epB, pEP->bSigned);
        if (pEP->bSigned)
        {
            aEndPts[p].A.Clamp(-F16MAX, F16MAX);
            aEndPts[p].B.Clamp(-F16MAX, F16MAX);
        }
        else
        {
            aEndPts[p].A.Clamp(0, F16MAX);
            aEndPts[p].B.Clamp(0, F16MAX);
        }

        fError += MapColors(pEP, p, np, auPixIdx);
    }

    return fError;
}

//-------------------------------------------------------------------------------------
// BC7 Compression
//-------------------------------------------------------------------------------------

void D3DX_BC7::Decode(u8* output, size_t stride) const noexcept
{
    assert(output);

    size_t uFirst = 0;
    while (uFirst < 128 && !GetBit(uFirst)) {}
    const uint8_t uMode = uint8_t(uFirst - 1);

    if (uMode < 8)
    {
        const uint8_t uPartitions = ms_aInfo[uMode].uPartitions;
        assert(uPartitions < BC7_MAX_REGIONS);

        auto const uNumEndPts = static_cast<const uint8_t>((unsigned(uPartitions) + 1u) << 1);
        const uint8_t uIndexPrec = ms_aInfo[uMode].uIndexPrec;
        const uint8_t uIndexPrec2 = ms_aInfo[uMode].uIndexPrec2;
        size_t uStartBit = size_t(uMode) + 1;
        uint8_t P[6];
        const uint8_t uShape = GetBits(uStartBit, ms_aInfo[uMode].uPartitionBits);
        assert(uShape < BC7_MAX_SHAPES);

        const uint8_t uRotation = GetBits(uStartBit, ms_aInfo[uMode].uRotationBits);
        assert(uRotation < 4);

        const uint8_t uIndexMode = GetBits(uStartBit, ms_aInfo[uMode].uIndexModeBits);
        assert(uIndexMode < 2);

        LDRColorA c[BC7_MAX_REGIONS << 1];
        const LDRColorA RGBAPrec = ms_aInfo[uMode].RGBAPrec;
        const LDRColorA RGBAPrecWithP = ms_aInfo[uMode].RGBAPrecWithP;

        assert(uNumEndPts <= (BC7_MAX_REGIONS << 1));

        // Red channel
        for (size_t i = 0; i < uNumEndPts; i++)
        {
            if (uStartBit + RGBAPrec.r > 128)
            {
                debugPrint("BC7: Invalid block encountered during decoding\n");
                FillWithErrorColorU32(output, stride);
                return;
            }

            c[i].r = GetBits(uStartBit, RGBAPrec.r);
        }

        // Green channel
        for (size_t i = 0; i < uNumEndPts; i++)
        {
            if (uStartBit + RGBAPrec.g > 128)
            {
                debugPrint("BC7: Invalid block encountered during decoding\n");
                FillWithErrorColorU32(output, stride);
                return;
            }

            c[i].g = GetBits(uStartBit, RGBAPrec.g);
        }

        // Blue channel
        for (size_t i = 0; i < uNumEndPts; i++)
        {
            if (uStartBit + RGBAPrec.b > 128)
            {
                debugPrint("BC7: Invalid block encountered during decoding\n");
                FillWithErrorColorU32(output, stride);
                return;
            }

            c[i].b = GetBits(uStartBit, RGBAPrec.b);
        }

        // Alpha channel
        for (size_t i = 0; i < uNumEndPts; i++)
        {
            if (uStartBit + RGBAPrec.a > 128)
            {
                debugPrint("BC7: Invalid block encountered during decoding\n");
                FillWithErrorColorU32(output, stride);
                return;
            }

            c[i].a = RGBAPrec.a ? GetBits(uStartBit, RGBAPrec.a) : 255u;
        }

        // P-bits
        assert(ms_aInfo[uMode].uPBits <= 6);
        for (size_t i = 0; i < ms_aInfo[uMode].uPBits; i++)
        {
            if (uStartBit > 127)
            {
                debugPrint("BC7: Invalid block encountered during decoding\n");
                FillWithErrorColorU32(output, stride);
                return;
            }

            P[i] = GetBit(uStartBit);
        }

        if (ms_aInfo[uMode].uPBits)
        {
            for (size_t i = 0; i < uNumEndPts; i++)
            {
                size_t pi = i * ms_aInfo[uMode].uPBits / uNumEndPts;
                for (uint8_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
                {
                    if (RGBAPrec[ch] != RGBAPrecWithP[ch])
                    {
                        c[i][ch] = static_cast<uint8_t>((unsigned(c[i][ch]) << 1) | P[pi]);
                    }
                }
            }
        }

        for (size_t i = 0; i < uNumEndPts; i++)
        {
            c[i] = Unquantize(c[i], RGBAPrecWithP);
        }

        u8 w1[NUM_PIXELS_PER_BLOCK];
        u8 w2[NUM_PIXELS_PER_BLOCK];

        // read color indices
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
        {
            const size_t uNumBits = IsFixUpOffset(ms_aInfo[uMode].uPartitions, uShape, i) ? uIndexPrec - 1u : uIndexPrec;
            if (uStartBit + uNumBits > 128)
            {
                debugPrint("BC7: Invalid block encountered during decoding\n");
                FillWithErrorColorU32(output, stride);
                return;
            }
            w1[i] = GetBits(uStartBit, uNumBits);
        }

        // read alpha indices
        if (uIndexPrec2)
        {
            for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
            {
                const size_t uNumBits = i ? uIndexPrec2 : uIndexPrec2 - 1u;
                if (uStartBit + uNumBits > 128)
                {
                    debugPrint("BC7: Invalid block encountered during decoding\n");
                    FillWithErrorColorU32(output, stride);
                    return;
                }
                w2[i] = GetBits(uStartBit, uNumBits);
            }
        }

        for (int y = 0; y < 4; ++y)
        {
            u32* dest = reinterpret_cast<u32*>(output + y * stride);

            for (int x = 0; x < 4; ++x)
            {
                const int idx = y * 4 + x;

                const uint8_t uRegion = g_aPartitionTable[uPartitions][uShape][idx];
                LDRColorA outPixel;
                if (uIndexPrec2 == 0)
                {
                    LDRColorA::Interpolate(c[uRegion << 1], c[(uRegion << 1) + 1], w1[idx], w1[idx], uIndexPrec, uIndexPrec, outPixel);
                }
                else
                {
                    if (uIndexMode == 0)
                    {
                        LDRColorA::Interpolate(c[uRegion << 1], c[(uRegion << 1) + 1], w1[idx], w2[idx], uIndexPrec, uIndexPrec2, outPixel);
                    }
                    else
                    {
                        LDRColorA::Interpolate(c[uRegion << 1], c[(uRegion << 1) + 1], w2[idx], w1[idx], uIndexPrec2, uIndexPrec, outPixel);
                    }
                }

                switch (uRotation)
                {
                case 1: std::swap(outPixel.r, outPixel.a); break;
                case 2: std::swap(outPixel.g, outPixel.a); break;
                case 3: std::swap(outPixel.b, outPixel.a); break;
                }

                dest[x] = outPixel;
            }
        }
    }
    else
    {
        debugPrint("BC7: Reserved mode 8 encountered during decoding\n");
        // Per the BC7 format spec, we must return transparent black
        FillColorU32(output, stride, 0xff000000);
    }
}

void D3DX_BC7::Encode(uint32_t flags, const u8* input, size_t stride) noexcept
{
    assert(input);

    HDRColorA temp[16];
    unpack_block(temp, input, stride);

    EncodeParams EP(temp);

    for (int y = 0; y < 4; ++y)
    {
        std::memcpy(EP.aLDRPixels + y * 4, input + y * stride, 16);
    }

    u32 alphaMask = 0xFF;
    for (int i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        alphaMask &= EP.aLDRPixels[i].a;
    }

    const bool bHasAlpha = (alphaMask != 0xFF);

    D3DX_BC7 final = *this;
    float fMSEBest = FLT_MAX;

    for (EP.uMode = 0; EP.uMode < 8 && fMSEBest > 0; ++EP.uMode)
    {
        if (!(flags & BC_FLAGS_USE_3SUBSETS) && (EP.uMode == 0 || EP.uMode == 2))
        {
            // 3 subset modes tend to be used rarely and add significant compression time
            continue;
        }

        /*
        if ((flags & TEX_COMPRESS_BC7_QUICK) && (EP.uMode != 6))
        {
            // Use only mode 6
            continue;
        }
        */

        if ((!bHasAlpha) && (EP.uMode == 7))
        {
            // There is no value in using mode 7 for completely opaque blocks (the other 2 subset modes handle this case for opaque blocks), so skip it for a small perf win.
            continue;
        }

        const size_t uShapes = size_t(1) << ms_aInfo[EP.uMode].uPartitionBits;
        assert(uShapes <= BC7_MAX_SHAPES);

        const size_t uNumRots = size_t(1) << ms_aInfo[EP.uMode].uRotationBits;
        const size_t uNumIdxMode = size_t(1) << ms_aInfo[EP.uMode].uIndexModeBits;
        // Number of rough cases to look at. reasonable values of this are 1, uShapes/4, and uShapes
        // uShapes/4 gets nearly all the cases; you can increase that a bit (say by 3 or 4) if you really want to squeeze the last bit out
        const size_t uItems = std::max<size_t>(1, uShapes >> 2);
        float afRoughMSE[BC7_MAX_SHAPES];
        size_t auShape[BC7_MAX_SHAPES];

        for (size_t r = 0; r < uNumRots && fMSEBest > 0; ++r)
        {
            switch (r)
            {
            case 1: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].r, EP.aLDRPixels[i].a); break;
            case 2: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].g, EP.aLDRPixels[i].a); break;
            case 3: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].b, EP.aLDRPixels[i].a); break;
            }

            for (size_t im = 0; im < uNumIdxMode && fMSEBest > 0; ++im)
            {
                // pick the best uItems shapes and refine these.
                for (size_t s = 0; s < uShapes; s++)
                {
                    afRoughMSE[s] = RoughMSE(&EP, s, im);
                    auShape[s] = s;
                }

                // Bubble up the first uItems items
                for (size_t i = 0; i < uItems; i++)
                {
                    for (size_t j = i + 1; j < uShapes; j++)
                    {
                        if (afRoughMSE[i] > afRoughMSE[j])
                        {
                            std::swap(afRoughMSE[i], afRoughMSE[j]);
                            std::swap(auShape[i], auShape[j]);
                        }
                    }
                }

                for (size_t i = 0; i < uItems && fMSEBest > 0; i++)
                {
                    const float fMSE = Refine(&EP, auShape[i], r, im);
                    if (fMSE < fMSEBest)
                    {
                        final = *this;
                        fMSEBest = fMSE;
                    }
                }
            }

            switch (r)
            {
            case 1: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].r, EP.aLDRPixels[i].a); break;
            case 2: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].g, EP.aLDRPixels[i].a); break;
            case 3: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].b, EP.aLDRPixels[i].a); break;
            }
        }
    }

    *this = final;
}

//-------------------------------------------------------------------------------------

void D3DX_BC7::GeneratePaletteQuantized(const EncodeParams* pEP, size_t uIndexMode, const LDREndPntPair& endPts, LDRColorA aPalette[]) const noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const size_t uIndexPrec = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec2 : ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uIndexPrec2 = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec : ms_aInfo[pEP->uMode].uIndexPrec2;
    const size_t uNumIndices = size_t(1) << uIndexPrec;
    const size_t uNumIndices2 = size_t(1) << uIndexPrec2;
    assert(uNumIndices > 0 && uNumIndices2 > 0);
    assert((uNumIndices <= BC7_MAX_INDICES) && (uNumIndices2 <= BC7_MAX_INDICES));

    const LDRColorA a = Unquantize(endPts.A, ms_aInfo[pEP->uMode].RGBAPrecWithP);
    const LDRColorA b = Unquantize(endPts.B, ms_aInfo[pEP->uMode].RGBAPrecWithP);
    if (uIndexPrec2 == 0)
    {
        for (size_t i = 0; i < uNumIndices; i++)
            LDRColorA::Interpolate(a, b, i, i, uIndexPrec, uIndexPrec, aPalette[i]);
    }
    else
    {
        for (size_t i = 0; i < uNumIndices; i++)
            LDRColorA::InterpolateRGB(a, b, i, uIndexPrec, aPalette[i]);
        for (size_t i = 0; i < uNumIndices2; i++)
            LDRColorA::InterpolateA(a, b, i, uIndexPrec2, aPalette[i]);
    }
}

float D3DX_BC7::PerturbOne(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode, size_t ch,
    const LDREndPntPair &oldEndPts, LDREndPntPair &newEndPts, float fOldErr, uint8_t do_b) const noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const int prec = ms_aInfo[pEP->uMode].RGBAPrecWithP[ch];
    LDREndPntPair tmp_endPts = newEndPts = oldEndPts;
    float fMinErr = fOldErr;
    uint8_t* pnew_c = (do_b ? &newEndPts.B[ch] : &newEndPts.A[ch]);
    uint8_t* ptmp_c = (do_b ? &tmp_endPts.B[ch] : &tmp_endPts.A[ch]);

    // do a logarithmic search for the best error for this endpoint (which)
    for (int step = 1 << (prec - 1); step; step >>= 1)
    {
        bool bImproved = false;
        int beststep = 0;
        for (int sign = -1; sign <= 1; sign += 2)
        {
            int tmp = int(*pnew_c) + sign * step;
            if (tmp < 0 || tmp >= (1 << prec))
                continue;
            else
                *ptmp_c = static_cast<uint8_t>(tmp);

            const float fTotalErr = MapColors(pEP, aColors, np, uIndexMode, tmp_endPts, fMinErr);
            if (fTotalErr < fMinErr)
            {
                bImproved = true;
                fMinErr = fTotalErr;
                beststep = sign * step;
            }
        }

        // if this was an improvement, move the endpoint and continue search from there
        if (bImproved)
            *pnew_c = uint8_t(int(*pnew_c) + beststep);
    }
    return fMinErr;
}

// perturb the endpoints at least -3 to 3.
// always ensure endpoint ordering is preserved (no need to overlap the scan)

void D3DX_BC7::Exhaustive(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode, size_t ch,
    float& fOrgErr, LDREndPntPair& optEndPt) const noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const uint8_t uPrec = ms_aInfo[pEP->uMode].RGBAPrecWithP[ch];
    LDREndPntPair tmpEndPt;
    if (fOrgErr == 0)
        return;

    constexpr int delta = 5;

    // ok figure out the range of A and B
    tmpEndPt = optEndPt;
    const int alow = std::max<int>(0, int(optEndPt.A[ch]) - delta);
    const int ahigh = std::min<int>((1 << uPrec) - 1, int(optEndPt.A[ch]) + delta);
    const int blow = std::max<int>(0, int(optEndPt.B[ch]) - delta);
    const int bhigh = std::min<int>((1 << uPrec) - 1, int(optEndPt.B[ch]) + delta);
    int amin = 0;
    int bmin = 0;

    float fBestErr = fOrgErr;
    if (optEndPt.A[ch] <= optEndPt.B[ch])
    {
        // keep a <= b
        for (int a = alow; a <= ahigh; ++a)
        {
            for (int b = std::max<int>(a, blow); b < bhigh; ++b)
            {
                tmpEndPt.A[ch] = static_cast<uint8_t>(a);
                tmpEndPt.B[ch] = static_cast<uint8_t>(b);

                const float fErr = MapColors(pEP, aColors, np, uIndexMode, tmpEndPt, fBestErr);
                if (fErr < fBestErr)
                {
                    amin = a;
                    bmin = b;
                    fBestErr = fErr;
                }
            }
        }
    }
    else
    {
        // keep b <= a
        for (int b = blow; b < bhigh; ++b)
        {
            for (int a = std::max<int>(b, alow); a <= ahigh; ++a)
            {
                tmpEndPt.A[ch] = static_cast<uint8_t>(a);
                tmpEndPt.B[ch] = static_cast<uint8_t>(b);

                const float fErr = MapColors(pEP, aColors, np, uIndexMode, tmpEndPt, fBestErr);
                if (fErr < fBestErr)
                {
                    amin = a;
                    bmin = b;
                    fBestErr = fErr;
                }
            }
        }
    }

    if (fBestErr < fOrgErr)
    {
        optEndPt.A[ch] = static_cast<uint8_t>(amin);
        optEndPt.B[ch] = static_cast<uint8_t>(bmin);
        fOrgErr = fBestErr;
    }
}

void D3DX_BC7::OptimizeOne(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode,
    float fOrgErr, const LDREndPntPair& org, LDREndPntPair& opt) const noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    float fOptErr = fOrgErr;
    opt = org;

    LDREndPntPair new_a, new_b;
    LDREndPntPair newEndPts;
    uint8_t do_b;

    // now optimize each channel separately
    for (size_t ch = 0; ch < BC7_NUM_CHANNELS; ++ch)
    {
        if (ms_aInfo[pEP->uMode].RGBAPrecWithP[ch] == 0)
            continue;

        // figure out which endpoint when perturbed gives the most improvement and start there
        // if we just alternate, we can easily end up in a local minima
        const float fErr0 = PerturbOne(pEP, aColors, np, uIndexMode, ch, opt, new_a, fOptErr, 0);	// perturb endpt A
        const float fErr1 = PerturbOne(pEP, aColors, np, uIndexMode, ch, opt, new_b, fOptErr, 1);	// perturb endpt B

        uint8_t& copt_a = opt.A[ch];
        uint8_t& copt_b = opt.B[ch];
        uint8_t& cnew_a = new_a.A[ch];
        uint8_t& cnew_b = new_a.B[ch];

        if (fErr0 < fErr1)
        {
            if (fErr0 >= fOptErr)
                continue;
            copt_a = cnew_a;
            fOptErr = fErr0;
            do_b = 1;		// do B next
        }
        else
        {
            if (fErr1 >= fOptErr)
                continue;
            copt_b = cnew_b;
            fOptErr = fErr1;
            do_b = 0;		// do A next
        }

        // now alternate endpoints and keep trying until there is no improvement
        for (; ; )
        {
            const float fErr = PerturbOne(pEP, aColors, np, uIndexMode, ch, opt, newEndPts, fOptErr, do_b);
            if (fErr >= fOptErr)
                break;
            if (do_b == 0)
                copt_a = cnew_a;
            else
                copt_b = cnew_b;
            fOptErr = fErr;
            do_b = 1u - do_b;	// now move the other endpoint
        }
    }

    // finally, do a small exhaustive search around what we think is the global minima to be sure
    for (size_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
        Exhaustive(pEP, aColors, np, uIndexMode, ch, fOptErr, opt);
}

void D3DX_BC7::OptimizeEndPoints(const EncodeParams* pEP, size_t uShape, size_t uIndexMode, const float afOrgErr[],
    const LDREndPntPair aOrgEndPts[], LDREndPntPair aOptEndPts[]) const noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS && uShape < BC7_MAX_SHAPES);

    LDRColorA aPixels[NUM_PIXELS_PER_BLOCK];

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        // collect the pixels in the region
        size_t np = 0;
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
            if (g_aPartitionTable[uPartitions][uShape][i] == p)
                aPixels[np++] = pEP->aLDRPixels[i];

        OptimizeOne(pEP, aPixels, np, uIndexMode, afOrgErr[p], aOrgEndPts[p], aOptEndPts[p]);
    }
}

void D3DX_BC7::AssignIndices(const EncodeParams* pEP, size_t uShape, size_t uIndexMode, LDREndPntPair endPts[], size_t aIndices[], size_t aIndices2[],
    float afTotErr[]) const noexcept
{
    assert(pEP);
    assert(uShape < BC7_MAX_SHAPES);
    assert(pEP->uMode < c_NumModes);

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS);

    const uint8_t uIndexPrec = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec2 : ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uIndexPrec2 = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec : ms_aInfo[pEP->uMode].uIndexPrec2;
    auto const uNumIndices = static_cast<const uint8_t>(1u << uIndexPrec);
    auto const uNumIndices2 = static_cast<const uint8_t>(1u << uIndexPrec2);

    assert((uNumIndices <= BC7_MAX_INDICES) && (uNumIndices2 <= BC7_MAX_INDICES));

    const uint8_t uHighestIndexBit = uint8_t(uNumIndices >> 1);
    const uint8_t uHighestIndexBit2 = uint8_t(uNumIndices2 >> 1);
    LDRColorA aPalette[BC7_MAX_REGIONS][BC7_MAX_INDICES];

    // build list of possibles
    for (size_t p = 0; p <= uPartitions; p++)
    {
        GeneratePaletteQuantized(pEP, uIndexMode, endPts[p], aPalette[p]);
        afTotErr[p] = 0;
    }

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
    {
        uint8_t uRegion = g_aPartitionTable[uPartitions][uShape][i];
        assert(uRegion < BC7_MAX_REGIONS);
        afTotErr[uRegion] += ComputeError(pEP->aLDRPixels[i], aPalette[uRegion], uIndexPrec, uIndexPrec2, &(aIndices[i]), &(aIndices2[i]));
    }

    // swap endpoints as needed to ensure that the indices at index_positions have a 0 high-order bit
    if (uIndexPrec2 == 0)
    {
        for (size_t p = 0; p <= uPartitions; p++)
        {
            if (aIndices[g_aFixUp[uPartitions][uShape][p]] & uHighestIndexBit)
            {
                std::swap(endPts[p].A, endPts[p].B);
                for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
                    if (g_aPartitionTable[uPartitions][uShape][i] == p)
                        aIndices[i] = uNumIndices - 1 - aIndices[i];
            }
            assert((aIndices[g_aFixUp[uPartitions][uShape][p]] & uHighestIndexBit) == 0);
        }
    }
    else
    {
        for (size_t p = 0; p <= uPartitions; p++)
        {
            if (aIndices[g_aFixUp[uPartitions][uShape][p]] & uHighestIndexBit)
            {
                std::swap(endPts[p].A.r, endPts[p].B.r);
                std::swap(endPts[p].A.g, endPts[p].B.g);
                std::swap(endPts[p].A.b, endPts[p].B.b);
                for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
                    if (g_aPartitionTable[uPartitions][uShape][i] == p)
                        aIndices[i] = uNumIndices - 1 - aIndices[i];
            }
            assert((aIndices[g_aFixUp[uPartitions][uShape][p]] & uHighestIndexBit) == 0);

            if (aIndices2[0] & uHighestIndexBit2)
            {
                std::swap(endPts[p].A.a, endPts[p].B.a);
                for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
                    aIndices2[i] = uNumIndices2 - 1 - aIndices2[i];
            }
            assert((aIndices2[0] & uHighestIndexBit2) == 0);
        }
    }
}

void D3DX_BC7::EmitBlock(const EncodeParams* pEP, size_t uShape, size_t uRotation, size_t uIndexMode, const LDREndPntPair aEndPts[], const size_t aIndex[], const size_t aIndex2[]) noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS);

    const size_t uPBits = ms_aInfo[pEP->uMode].uPBits;
    const size_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uIndexPrec2 = ms_aInfo[pEP->uMode].uIndexPrec2;
    const LDRColorA RGBAPrec = ms_aInfo[pEP->uMode].RGBAPrec;
    const LDRColorA RGBAPrecWithP = ms_aInfo[pEP->uMode].RGBAPrecWithP;
    size_t i;
    size_t uStartBit = 0;
    SetBits(uStartBit, pEP->uMode, 0);
    SetBits(uStartBit, 1, 1);
    SetBits(uStartBit, ms_aInfo[pEP->uMode].uRotationBits, static_cast<uint8_t>(uRotation));
    SetBits(uStartBit, ms_aInfo[pEP->uMode].uIndexModeBits, static_cast<uint8_t>(uIndexMode));
    SetBits(uStartBit, ms_aInfo[pEP->uMode].uPartitionBits, static_cast<uint8_t>(uShape));

    if (uPBits)
    {
        const size_t uNumEP = (size_t(uPartitions) + 1) << 1;
        uint8_t aPVote[BC7_MAX_REGIONS << 1] = { 0,0,0,0,0,0 };
        uint8_t aCount[BC7_MAX_REGIONS << 1] = { 0,0,0,0,0,0 };
        for (uint8_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
        {
            uint8_t ep = 0;
            for (i = 0; i <= uPartitions; i++)
            {
                if (RGBAPrec[ch] == RGBAPrecWithP[ch])
                {
                    SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].A[ch]);
                    SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].B[ch]);
                }
                else
                {
                    SetBits(uStartBit, RGBAPrec[ch], uint8_t(aEndPts[i].A[ch] >> 1));
                    SetBits(uStartBit, RGBAPrec[ch], uint8_t(aEndPts[i].B[ch] >> 1));
                    size_t idx = ep++ * uPBits / uNumEP;
                    assert(idx < (BC7_MAX_REGIONS << 1));
                    aPVote[idx] += aEndPts[i].A[ch] & 0x01;
                    aCount[idx]++;
                    idx = ep++ * uPBits / uNumEP;
                    assert(idx < (BC7_MAX_REGIONS << 1));
                    aPVote[idx] += aEndPts[i].B[ch] & 0x01;
                    aCount[idx]++;
                }
            }
        }

        for (i = 0; i < uPBits; i++)
        {
            SetBits(uStartBit, 1, (aPVote[i] >(aCount[i] >> 1)) ? 1u : 0u);
        }
    }
    else
    {
        for (size_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
        {
            for (i = 0; i <= uPartitions; i++)
            {
                SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].A[ch]);
                SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].B[ch]);
            }
        }
    }

    const size_t* aI1 = uIndexMode ? aIndex2 : aIndex;
    const size_t* aI2 = uIndexMode ? aIndex : aIndex2;
    for (i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
    {
        if (IsFixUpOffset(ms_aInfo[pEP->uMode].uPartitions, uShape, i))
            SetBits(uStartBit, uIndexPrec - 1, static_cast<uint8_t>(aI1[i]));
        else
            SetBits(uStartBit, uIndexPrec, static_cast<uint8_t>(aI1[i]));
    }
    if (uIndexPrec2)
        for (i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
            SetBits(uStartBit, i ? uIndexPrec2 : uIndexPrec2 - 1, static_cast<uint8_t>(aI2[i]));

    assert(uStartBit == 128);
}

void D3DX_BC7::FixEndpointPBits(const EncodeParams* pEP, const LDREndPntPair *pOrigEndpoints, LDREndPntPair *pFixedEndpoints) noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const size_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS);

    pFixedEndpoints[0] = pOrigEndpoints[0];
    pFixedEndpoints[1] = pOrigEndpoints[1];
    pFixedEndpoints[2] = pOrigEndpoints[2];

    const size_t uPBits = ms_aInfo[pEP->uMode].uPBits;

    if (uPBits)
    {
        const size_t uNumEP = size_t(1 + uPartitions) << 1;
        uint8_t aPVote[BC7_MAX_REGIONS << 1] = { 0,0,0,0,0,0 };
        uint8_t aCount[BC7_MAX_REGIONS << 1] = { 0,0,0,0,0,0 };

        const LDRColorA RGBAPrec = ms_aInfo[pEP->uMode].RGBAPrec;
        const LDRColorA RGBAPrecWithP = ms_aInfo[pEP->uMode].RGBAPrecWithP;

        for (uint8_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
        {
            uint8_t ep = 0;
            for (size_t i = 0; i <= uPartitions; i++)
            {
                if (RGBAPrec[ch] == RGBAPrecWithP[ch])
                {
                    pFixedEndpoints[i].A[ch] = pOrigEndpoints[i].A[ch];
                    pFixedEndpoints[i].B[ch] = pOrigEndpoints[i].B[ch];
                }
                else
                {
                    pFixedEndpoints[i].A[ch] = uint8_t(pOrigEndpoints[i].A[ch] >> 1);
                    pFixedEndpoints[i].B[ch] = uint8_t(pOrigEndpoints[i].B[ch] >> 1);

                    size_t idx = ep++ * uPBits / uNumEP;
                    assert(idx < (BC7_MAX_REGIONS << 1));
                    aPVote[idx] += pOrigEndpoints[i].A[ch] & 0x01;
                    aCount[idx]++;
                    idx = ep++ * uPBits / uNumEP;
                    assert(idx < (BC7_MAX_REGIONS << 1));
                    aPVote[idx] += pOrigEndpoints[i].B[ch] & 0x01;
                    aCount[idx]++;
                }
            }
        }

        // Compute the actual pbits we'll use when we encode block. Note this is not
        // rounding the component indices correctly in cases the pbits != a component's LSB.
        int pbits[BC7_MAX_REGIONS << 1];
        for (size_t i = 0; i < uPBits; i++)
            pbits[i] = aPVote[i] >(aCount[i] >> 1) ? 1 : 0;

        // Now calculate the actual endpoints with proper pbits, so error calculations are accurate.
        if (pEP->uMode == 1)
        {
            // shared pbits
            for (uint8_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
            {
                for (size_t i = 0; i <= uPartitions; i++)
                {
                    pFixedEndpoints[i].A[ch] = static_cast<uint8_t>((pFixedEndpoints[i].A[ch] << 1) | pbits[i]);
                    pFixedEndpoints[i].B[ch] = static_cast<uint8_t>((pFixedEndpoints[i].B[ch] << 1) | pbits[i]);
                }
            }
        }
        else
        {
            for (uint8_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
            {
                for (size_t i = 0; i <= uPartitions; i++)
                {
                    pFixedEndpoints[i].A[ch] = static_cast<uint8_t>((pFixedEndpoints[i].A[ch] << 1) | pbits[i * 2 + 0]);
                    pFixedEndpoints[i].B[ch] = static_cast<uint8_t>((pFixedEndpoints[i].B[ch] << 1) | pbits[i * 2 + 1]);
                }
            }
        }
    }
}

float D3DX_BC7::Refine(const EncodeParams* pEP, size_t uShape, size_t uRotation, size_t uIndexMode) noexcept
{
    assert(pEP);
    assert(uShape < BC7_MAX_SHAPES);
    assert(pEP->uMode < c_NumModes);

    const size_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS);

    LDREndPntPair aOrgEndPts[BC7_MAX_REGIONS];
    LDREndPntPair aOptEndPts[BC7_MAX_REGIONS];
    size_t aOrgIdx[NUM_PIXELS_PER_BLOCK];
    size_t aOrgIdx2[NUM_PIXELS_PER_BLOCK];
    size_t aOptIdx[NUM_PIXELS_PER_BLOCK];
    size_t aOptIdx2[NUM_PIXELS_PER_BLOCK];
    float aOrgErr[BC7_MAX_REGIONS];
    float aOptErr[BC7_MAX_REGIONS];

    const LDREndPntPair* aEndPts = &pEP->aEndPts[uShape][0];

    for (size_t p = 0; p <= uPartitions; p++)
    {
        // NOTE: This silences compiler warning (GCC 12); it incorrectly analyses that
        //       we will have overflow. Checking it manually disables the compile-time check.
        if (p < BC7_MAX_REGIONS)
        {
            aOrgEndPts[p].A = Quantize(aEndPts[p].A, ms_aInfo[pEP->uMode].RGBAPrecWithP);
            aOrgEndPts[p].B = Quantize(aEndPts[p].B, ms_aInfo[pEP->uMode].RGBAPrecWithP);
        }
    }

    LDREndPntPair newEndPts1[BC7_MAX_REGIONS];
    FixEndpointPBits(pEP, aOrgEndPts, newEndPts1);

    AssignIndices(pEP, uShape, uIndexMode, newEndPts1, aOrgIdx, aOrgIdx2, aOrgErr);

    OptimizeEndPoints(pEP, uShape, uIndexMode, aOrgErr, newEndPts1, aOptEndPts);

    LDREndPntPair newEndPts2[BC7_MAX_REGIONS];
    FixEndpointPBits(pEP, aOptEndPts, newEndPts2);

    AssignIndices(pEP, uShape, uIndexMode, newEndPts2, aOptIdx, aOptIdx2, aOptErr);

    float fOrgTotErr = 0, fOptTotErr = 0;
    for (size_t p = 0; p <= uPartitions; p++)
    {
        fOrgTotErr += aOrgErr[p];
        fOptTotErr += aOptErr[p];
    }
    if (fOptTotErr < fOrgTotErr)
    {
        EmitBlock(pEP, uShape, uRotation, uIndexMode, newEndPts2, aOptIdx, aOptIdx2);
        return fOptTotErr;
    }
    else
    {
        EmitBlock(pEP, uShape, uRotation, uIndexMode, newEndPts1, aOrgIdx, aOrgIdx2);
        return fOrgTotErr;
    }
}

float D3DX_BC7::MapColors(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode, const LDREndPntPair& endPts, float fMinErr) const noexcept
{
    assert(pEP);
    assert(pEP->uMode < c_NumModes);

    const uint8_t uIndexPrec = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec2 : ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uIndexPrec2 = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec : ms_aInfo[pEP->uMode].uIndexPrec2;
    LDRColorA aPalette[BC7_MAX_INDICES];
    float fTotalErr = 0;

    GeneratePaletteQuantized(pEP, uIndexMode, endPts, aPalette);
    for (size_t i = 0; i < np; ++i)
    {
        fTotalErr += ComputeError(aColors[i], aPalette, uIndexPrec, uIndexPrec2);
        if (fTotalErr > fMinErr)   // check for early exit
        {
            fTotalErr = FLT_MAX;
            break;
        }
    }

    return fTotalErr;
}

float D3DX_BC7::RoughMSE(EncodeParams* pEP, size_t uShape, size_t uIndexMode) noexcept
{
    assert(pEP);
    assert(uShape < BC7_MAX_SHAPES);
    assert(pEP->uMode < c_NumModes);

    LDREndPntPair* aEndPts = pEP->aEndPts[uShape];

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS);

    const uint8_t uIndexPrec = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec2 : ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uIndexPrec2 = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec : ms_aInfo[pEP->uMode].uIndexPrec2;
    auto const uNumIndices = static_cast<const uint8_t>(1u << uIndexPrec);
    auto const uNumIndices2 = static_cast<const uint8_t>(1u << uIndexPrec2);
    size_t auPixIdx[NUM_PIXELS_PER_BLOCK];
    LDRColorA aPalette[BC7_MAX_REGIONS][BC7_MAX_INDICES];

    for (size_t p = 0; p <= uPartitions; p++)
    {
        size_t np = 0;
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
        {
            if (g_aPartitionTable[uPartitions][uShape][i] == p)
            {
                auPixIdx[np++] = i;
            }
        }

        // handle simple cases
        assert(np > 0);
        if (np == 1)
        {
            aEndPts[p].A = pEP->aLDRPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aLDRPixels[auPixIdx[0]];
            continue;
        }
        else if (np == 2)
        {
            aEndPts[p].A = pEP->aLDRPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aLDRPixels[auPixIdx[1]];
            continue;
        }

        if (uIndexPrec2 == 0)
        {
            HDRColorA epA, epB;
            OptimizeRGBA(pEP->aHDRPixels, &epA, &epB, 4, np, auPixIdx);
            epA.Clamp(0.0f, 1.0f);
            epB.Clamp(0.0f, 1.0f);
            epA *= 255.0f;
            epB *= 255.0f;
            aEndPts[p].A = epA.ToLDRColorA();
            aEndPts[p].B = epB.ToLDRColorA();
        }
        else
        {
            uint8_t uMinAlpha = 255, uMaxAlpha = 0;
            for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
            {
                uMinAlpha = std::min<uint8_t>(uMinAlpha, pEP->aLDRPixels[auPixIdx[i]].a);
                uMaxAlpha = std::max<uint8_t>(uMaxAlpha, pEP->aLDRPixels[auPixIdx[i]].a);
            }

            HDRColorA epA, epB;
            OptimizeRGB(pEP->aHDRPixels, &epA, &epB, 4, np, auPixIdx);
            epA.Clamp(0.0f, 1.0f);
            epB.Clamp(0.0f, 1.0f);
            epA *= 255.0f;
            epB *= 255.0f;
            aEndPts[p].A = epA.ToLDRColorA();
            aEndPts[p].B = epB.ToLDRColorA();
            aEndPts[p].A.a = uMinAlpha;
            aEndPts[p].B.a = uMaxAlpha;
        }
    }

    if (uIndexPrec2 == 0)
    {
        for (size_t p = 0; p <= uPartitions; p++)
            for (size_t i = 0; i < uNumIndices; i++)
                LDRColorA::Interpolate(aEndPts[p].A, aEndPts[p].B, i, i, uIndexPrec, uIndexPrec, aPalette[p][i]);
    }
    else
    {
        for (size_t p = 0; p <= uPartitions; p++)
        {
            for (size_t i = 0; i < uNumIndices; i++)
                LDRColorA::InterpolateRGB(aEndPts[p].A, aEndPts[p].B, i, uIndexPrec, aPalette[p][i]);
            for (size_t i = 0; i < uNumIndices2; i++)
                LDRColorA::InterpolateA(aEndPts[p].A, aEndPts[p].B, i, uIndexPrec2, aPalette[p][i]);
        }
    }

    float fTotalErr = 0;
    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
    {
        const uint8_t uRegion = g_aPartitionTable[uPartitions][uShape][i];
        fTotalErr += ComputeError(pEP->aLDRPixels[i], aPalette[uRegion], uIndexPrec, uIndexPrec2);
    }

    return fTotalErr;
}

namespace mango::image
{
    using namespace DirectX;

    // BC6

    void decode_block_bc6hu(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(D3DX_BC6H) == 16, "D3DX_BC6H should be 16 bytes");
        reinterpret_cast<const D3DX_BC6H*>(input)->Decode(false, output, stride);
    }

    void decode_block_bc6hs(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(D3DX_BC6H) == 16, "D3DX_BC6H should be 16 bytes");
        reinterpret_cast<const D3DX_BC6H*>(input)->Decode(true, output, stride);
    }

    void encode_block_bc6hu(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(D3DX_BC6H) == 16, "D3DX_BC6H should be 16 bytes");
        reinterpret_cast<D3DX_BC6H*>(output)->Encode(false, input, stride);
    }

    void encode_block_bc6hs(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(D3DX_BC6H) == 16, "D3DX_BC6H should be 16 bytes");
        reinterpret_cast<D3DX_BC6H*>(output)->Encode(true, input, stride);
    }

    // BC7

    void decode_block_bc7(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(D3DX_BC7) == 16, "D3DX_BC7 should be 16 bytes");
        reinterpret_cast<const D3DX_BC7*>(input)->Decode(output, stride);
    }

    void encode_block_bc7(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        static_assert(sizeof(D3DX_BC7) == 16, "D3DX_BC7 should be 16 bytes");
        reinterpret_cast<D3DX_BC7*>(output)->Encode(0, input, stride);
    }

} // namespace mango::image
