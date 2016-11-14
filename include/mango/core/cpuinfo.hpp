/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"

namespace mango
{

	// ----------------------------------------------------------------------------
	// getCPUFlags()
	// ----------------------------------------------------------------------------

    enum : uint64
    {
        // Intel
        CPU_MMX        = 0x0000000000000001,
        CPU_MMX_PLUS   = 0x0000000000000002,
        CPU_SSE        = 0x0000000000000004,
        CPU_SSE2       = 0x0000000000000008,
        CPU_SSE3       = 0x0000000000000010,
        CPU_SSSE3      = 0x0000000000000020,
        CPU_SSE4_1     = 0x0000000000000040,
        CPU_SSE4_2     = 0x0000000000000080,
        CPU_SSE4A      = 0x0000000000000100,
        CPU_XOP        = 0x0000000000000200,
        CPU_3DNOW      = 0x0000000000000400,
        CPU_3DNOW_EXT  = 0x0000000000000800,
        CPU_AVX        = 0x0000000000001000,
        CPU_AVX2       = 0x0000000000002000,
        CPU_AES        = 0x0000000000004000,
        CPU_CLMUL      = 0x0000000000008000,
        CPU_FMA3       = 0x0000000000010000,
        CPU_MOVBE      = 0x0000000000020000,
        CPU_POPCNT     = 0x0000000000040000,
        CPU_F16C       = 0x0000000000080000,
        CPU_RDRAND     = 0x0000000000100000,
        CPU_CMOV       = 0x0000000000200000,
        CPU_CMPXCHG16B = 0x0000000000400000,
        CPU_FMA4       = 0x0000000000800000,
        CPU_BMI1       = 0x0000000001000000,
        CPU_BMI2       = 0x0000000002000000,
        CPU_SHA        = 0x0000000004000000,
        CPU_AVX512F    = 0x0000000008000000,
        CPU_AVX512PFI  = 0x0000000010000000,
        CPU_AVX512ERI  = 0x0000000020000000,
        CPU_AVX512CDI  = 0x0000000040000000,
        CPU_AVX512BW   = 0x0000000080000000,
        CPU_AVX512VL   = 0x0000000100000000,
        // ARM
        CPU_NEON       = 0x0001000000000000
    };

	uint64 getCPUFlags();

} // namespace mango
