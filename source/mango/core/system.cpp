/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/system.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/core/thread.hpp>
#include <mango/simd/simd.hpp>
#include <sstream>

namespace mango
{

	// ----------------------------------------------------------------------------
	// getSystemInfo()
	// ----------------------------------------------------------------------------

    std::string getSystemInfo()
    {
        std::stringstream info;

        info << "Platform: \"" << MANGO_PLATFORM_NAME << "\"" << std::endl;

        info << "CPU: \"" << MANGO_CPU_NAME << "\", ";
#ifdef MANGO_CPU_64BIT
        info << "Bits: 64, ";
#else
        info << "Bits: 32, ";
#endif
#ifdef MANGO_LITTLE_ENDIAN
        info << "Endian: LITTLE ";
#else
        info << "Endian: BIG ";
#endif
        info << std::endl;

        u64 flags = getCPUFlags();

        info << "Detected CPU Features: ";
        if (!flags) info << "N/A";
        if (flags & CPU_MMX) info << "MMX ";
        if (flags & CPU_MMX_PLUS) info << "MMX+ ";
        if (flags & CPU_SSE) info << "SSE ";
        if (flags & CPU_SSE2) info << "SSE2 ";
        if (flags & CPU_SSE3) info << "SSE3 ";
        if (flags & CPU_SSSE3) info << "SSSE3 ";
        if (flags & CPU_SSE4_1) info << "SSE4.1 ";
        if (flags & CPU_SSE4_2) info << "SSE4.2 ";
        if (flags & CPU_SSE4A) info << "SSE4A ";
        if (flags & CPU_XOP) info << "XOP ";
        if (flags & CPU_3DNOW) info << "3DNOW ";
        if (flags & CPU_3DNOW_EXT) info << "3DNOW+ ";
        if (flags & CPU_AVX) info << "AVX ";
        if (flags & CPU_AVX2) info << "AVX2 ";
        if (flags & CPU_ARM_NEON) info << "NEON ";
        if (flags & CPU_AES) info << "AES ";
        if (flags & CPU_CLMUL) info << "CLMUL ";
        if (flags & CPU_FMA3) info << "FMA3 ";
        if (flags & CPU_MOVBE) info << "MOVBE ";
        if (flags & CPU_POPCNT) info << "POPCNT ";
        if (flags & CPU_F16C) info << "F16C ";
        if (flags & CPU_RDRAND) info << "RDRAND ";
        if (flags & CPU_CMOV) info << "CMOV ";
        if (flags & CPU_CMPXCHG16B) info << "CMPXCHG16B ";
        if (flags & CPU_FMA4) info << "FMA4 ";
        if (flags & CPU_BMI1) info << "BMI1 ";
        if (flags & CPU_BMI2) info << "BMI2 ";
        if (flags & CPU_SHA) info << "SHA ";
        if (flags & CPU_AVX512F) info << "AVX512F ";
        if (flags & CPU_AVX512PFI) info << "AVX512PFI ";
        if (flags & CPU_AVX512ERI) info << "AVX512ERI ";
        if (flags & CPU_AVX512CDI) info << "AVX512CDI ";
        if (flags & CPU_AVX512BW) info << "AVX512BW ";
        if (flags & CPU_AVX512VL) info << "AVX512VL ";
        if (flags & CPU_AVX512DQ) info << "AVX512DQ ";
        if (flags & CPU_AVX512IFMA) info << "AVX512IFMA ";
        if (flags & CPU_AVX512VBMI) info << "AVX512VBMI ";
        info << std::endl;

        info << "Compiled SIMD Features: ";
#if defined(MANGO_ENABLE_SIMD)
    #if defined(MANGO_ENABLE_SSE)
        info << "SSE ";
    #endif

    #if defined(MANGO_ENABLE_SSE2)
        info << "SSE2 ";
    #endif

    #if defined(MANGO_ENABLE_SSE3)
        info << "SSE3 ";
    #endif

    #if defined(MANGO_ENABLE_SSSE3)
        info << "SSSE3 ";
    #endif

    #if defined(MANGO_ENABLE_SSE4_1)
        info << "SSE4.1 ";
    #endif

    #if defined(MANGO_ENABLE_SSE4_2)
        info << "SSE4.2 ";
    #endif

    #if defined(MANGO_ENABLE_AVX)
        info << "AVX ";
    #endif

    #if defined(MANGO_ENABLE_AVX2)
        info << "AVX2 ";
    #endif

    #if defined(MANGO_ENABLE_AVX512)
        info << "AVX512 ";
    #endif

    #if defined(MANGO_ENABLE_XOP)
        info << "XOP ";
    #endif

    #if defined(MANGO_ENABLE_FMA4)
        info << "FMA4 ";
    #endif

    #if defined(MANGO_ENABLE_FMA3)
        info << "FMA3 ";
    #endif

    #if defined(MANGO_ENABLE_F16C)
        info << "F16C ";
    #endif

    #if defined(MANGO_ENABLE_NEON)
        info << "NEON ";
    #endif
#else
        info << "N/A";
#endif
        info << std::endl;

        info << "Hardware threads: " << std::thread::hardware_concurrency() << std::endl;
        info << "Build: " << __DATE__ << "  " << __TIME__ << std::endl;

        return info.str();
    }

} // namespace mango
