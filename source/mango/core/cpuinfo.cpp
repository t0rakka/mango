/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <mango/core/cpuinfo.hpp>

namespace
{
    using namespace mango;

#if defined(MANGO_CPU_INTEL)

    // ----------------------------------------------------------------------------
    // cpuid()
    // ----------------------------------------------------------------------------

#if defined(MANGO_PLATFORM_WINDOWS)

#include "intrin.h"

    void cpuid(int* info, int id)
    {
        __cpuid(info, id);
    }

#elif defined(MANGO_PLATFORM_UNIX)

#include "cpuid.h"

    void cpuid(int* info, int id)
    {
        unsigned int regs[4] = { 0, 0, 0, 0 };
        __cpuid_count(id, 0, regs[0], regs[1], regs[2], regs[3]);

        info[0] = regs[0];
        info[1] = regs[1];
        info[2] = regs[2];
        info[3] = regs[3];
    }

#else

    #error "cpuid() not implemented."

#endif

    // ----------------------------------------------------------------------------
    // getCPUFlagsInternal()
    // ----------------------------------------------------------------------------

    u64 getCPUFlagsInternal()
    {
        u64 flags = 0;

        int cpuInfo[4] = { 0, 0, 0, 0 };

        // Get CPU information
        cpuid(cpuInfo, 0);
        unsigned int nIds = cpuInfo[0];

        for (unsigned int i = 0; i <= nIds; ++i)
        {
            cpuid(cpuInfo, i);

            switch (i)
            {
                case 1:
                    // edx
                    if ((cpuInfo[3] & 0x00800000) != 0) flags |= CPU_MMX;
                    if ((cpuInfo[3] & 0x02000000) != 0) flags |= CPU_SSE;
                    if ((cpuInfo[3] & 0x04000000) != 0) flags |= CPU_SSE2;
                    if ((cpuInfo[3] & 0x00008000) != 0) flags |= CPU_CMOV;
                    // ecx
                    if ((cpuInfo[2] & 0x00000001) != 0) flags |= CPU_SSE3;
                    if ((cpuInfo[2] & 0x00000200) != 0) flags |= CPU_SSSE3;
                    if ((cpuInfo[2] & 0x00080000) != 0) flags |= CPU_SSE4_1;
                    if ((cpuInfo[2] & 0x00100000) != 0) flags |= CPU_SSE4_2;
                    if ((cpuInfo[2] & 0x10000000) != 0) flags |= CPU_AVX;
                    if ((cpuInfo[2] & 0x02000000) != 0) flags |= CPU_AES;
                    if ((cpuInfo[2] & 0x00000002) != 0) flags |= CPU_CLMUL;
                    if ((cpuInfo[2] & 0x00001000) != 0) flags |= CPU_FMA3;
                    if ((cpuInfo[2] & 0x00400000) != 0) flags |= CPU_MOVBE;
                    if ((cpuInfo[2] & 0x00800000) != 0) flags |= CPU_POPCNT;
                    if ((cpuInfo[2] & 0x20000000) != 0) flags |= CPU_F16C;
                    if ((cpuInfo[2] & 0x40000000) != 0) flags |= CPU_RDRAND;
                    if ((cpuInfo[2] & 0x00002000) != 0) flags |= CPU_CMPXCHG16B;
                    break;
                case 7:
                    // ebx
                    if ((cpuInfo[1] & 0x00000002) != 0) flags |= CPU_AVX512VBMI;
                    if ((cpuInfo[1] & 0x00000008) != 0) flags |= (CPU_BMI1 | CPU_LZCNT);
                    if ((cpuInfo[1] & 0x00000020) != 0) flags |= CPU_AVX2;
                    if ((cpuInfo[1] & 0x00000100) != 0) flags |= CPU_BMI2;
                    if ((cpuInfo[1] & 0x00010000) != 0) flags |= CPU_AVX512F;
                    if ((cpuInfo[1] & 0x00020000) != 0) flags |= CPU_AVX512DQ;
                    if ((cpuInfo[1] & 0x00200000) != 0) flags |= CPU_AVX512IFMA;
                    if ((cpuInfo[1] & 0x04000000) != 0) flags |= CPU_AVX512PFI;
                    if ((cpuInfo[1] & 0x08000000) != 0) flags |= CPU_AVX512ERI;
                    if ((cpuInfo[1] & 0x10000000) != 0) flags |= CPU_AVX512CDI;
                    if ((cpuInfo[1] & 0x20000000) != 0) flags |= CPU_SHA;
                    if ((cpuInfo[1] & 0x40000000) != 0) flags |= CPU_AVX512BW;
                    if ((cpuInfo[1] & 0x80000000) != 0) flags |= CPU_AVX512VL;
                    break;
            }
        }

        // Get extended CPU information
        cpuid(cpuInfo, 0x80000000);
        unsigned int nExtIds = cpuInfo[0];

        for (unsigned int i = 0x80000000; i <= nExtIds; ++i)
        {
            if (i == 0x80000001)
            {
                cpuid(cpuInfo, i);

                // ecx
                if ((cpuInfo[2] & 0x00000020) != 0) flags |= (CPU_POPCNT | CPU_LZCNT); // ABM
                if ((cpuInfo[2] & 0x00000040) != 0) flags |= CPU_SSE4A;
                if ((cpuInfo[2] & 0x00010000) != 0) flags |= CPU_FMA4;
                if ((cpuInfo[2] & 0x00000800) != 0) flags |= CPU_XOP;
                // edx
                if ((cpuInfo[3] & 0x00800000) != 0) flags |= CPU_MMX;
                if ((cpuInfo[3] & 0x00400000) != 0) flags |= CPU_MMX_PLUS;
                if ((cpuInfo[3] & 0x80000000) != 0) flags |= CPU_3DNOW;
                if ((cpuInfo[3] & 0x40000000) != 0) flags |= CPU_3DNOW_EXT;
            }
        }

        return flags;
    }

#elif defined(MANGO_CPU_ARM) && defined(MANGO_PLATFORM_ANDROID)

#include <cpu-features.h>

    u64 getCPUFlagsInternal()
    {
        u64 flags = 0;

        if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM)
        {
            uint64_t features = android_getCpuFeatures();

            if ((features & ANDROID_CPU_ARM_FEATURE_NEON) != 0)
            {
                flags |= CPU_ARM_NEON;
            }

            if ((features & ANDROID_CPU_ARM_FEATURE_AES) != 0)
            {
                flags |= CPU_ARM_AES;
            }

            if ((features & ANDROID_CPU_ARM_FEATURE_SHA1) != 0)
            {
                flags |= CPU_ARM_SHA1;
            }

            if ((features & ANDROID_CPU_ARM_FEATURE_SHA2) != 0)
            {
                flags |= CPU_ARM_SHA2;
            }

            if ((features & ANDROID_CPU_ARM_FEATURE_CRC32) != 0)
            {
                flags |= CPU_ARM_CRC32;
            }
        }
        else if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM64)
        {
            uint64_t features = android_getCpuFeatures();

            flags |= CPU_ARM_NEON; // default for ARM64

            if ((features & ANDROID_CPU_ARM64_FEATURE_AES) != 0)
            {
                flags |= CPU_ARM_AES;
            }

            if ((features & ANDROID_CPU_ARM64_FEATURE_SHA1) != 0)
            {
                flags |= CPU_ARM_SHA1;
            }

            if ((features & ANDROID_CPU_ARM64_FEATURE_SHA2) != 0)
            {
                flags |= CPU_ARM_SHA2;
            }

            if ((features & ANDROID_CPU_ARM64_FEATURE_CRC32) != 0)
            {
                flags |= CPU_ARM_CRC32;
            }
        }

        return flags;
    }

#elif defined(MANGO_CPU_ARM) && defined(MANGO_PLATFORM_LINUX)

#include <sys/auxv.h>
#include <asm/hwcap.h>

// patch missing macros in some (older) implementations

#ifndef HWCAP_ASIMD
#define HWCAP_ASIMD		(1 << 1)
#endif

#ifndef HWCAP_AES
#define HWCAP_AES		(1 << 3)
#endif

#ifndef HWCAP_CRC32
#define HWCAP_CRC32		(1 << 7)
#endif

#ifndef HWCAP_SHA1
#define HWCAP_SHA1		(1 << 5)
#endif

#ifndef HWCAP_SHA2
#define HWCAP_SHA2		(1 << 6)
#endif

    u64 getCPUFlagsInternal()
    {
        u64 flags = 0;

        long hwcaps = getauxval(AT_HWCAP);

        if (hwcaps & HWCAP_ASIMD)
        {
            flags |= CPU_ARM_NEON;
        }

        if (hwcaps & HWCAP_AES)
        {
            flags |= CPU_ARM_AES;
        }

        if (hwcaps & HWCAP_CRC32)
        {
            flags |= CPU_ARM_CRC32;
        }

        if (hwcaps & HWCAP_SHA1)
        {
            flags |= CPU_ARM_SHA1;
        }

        if (hwcaps & HWCAP_SHA2)
        {
            flags |= CPU_ARM_SHA2;
        }

        return flags;
    }

#else

    u64 getCPUFlagsInternal()
    {
        return 0; // unsupported platform
    }

#endif

} // namespace

namespace mango
{

    u64 getCPUFlags()
    {
        static u64 flags = getCPUFlagsInternal(); // cache the value
        return flags;
    }

} // namespace mango
