/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <new>

// -----------------------------------------------------------------------
// platform
// -----------------------------------------------------------------------

#if defined(_XBOX_VER) && (_XBOX_VER < 200)

    // Microsoft XBOX
    #define MANGO_PLATFORM_XBOX
    #define MANGO_PLATFORM_NAME "Xbox"

#elif (defined(_XBOX_VER) && (_XBOX_VER >= 200)) || defined(_XENON)

	// Microsoft XBOX 360
    #define MANGO_PLATFORM_XBOX360
    #define MANGO_PLATFORM_NAME "Xbox 360"

#elif defined(_DURANGO)

	// Microsoft XBOX ONE
    #define MANGO_PLATFORM_XBOXONE
    #define MANGO_PLATFORM_NAME "Xbox One"

#elif defined(__CELLOS_LV2__)

	// SONY Playstation 3
    #define MANGO_PLATFORM_PS3
    #define MANGO_PLATFORM_NAME "Playstation 3"

#elif defined(__ORBIS__)

	// SONY Playstation 4
    #define MANGO_PLATFORM_PS4
    #define MANGO_PLATFORM_NAME "Playstation 4"

#elif defined(_WIN32) || defined(_WINDOWS_)

    // Microsoft Windows
    #define MANGO_PLATFORM_WINDOWS
    #define MANGO_PLATFORM_NAME "Windows"

    #ifndef NOMINMAX
    #define NOMINMAX
    #endif

    #include <windows.h>

#elif defined(__MINGW32__) || defined(__MINGW64__)

    // MinGW
    #define MANGO_PLATFORM_MINGW
    #define MANGO_PLATFORM_WINDOWS
    #define MANGO_PLATFORM_NAME "MinGW"

#elif defined(__APPLE__)

    #include "TargetConditionals.h"

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

        // Apple iOS
        #define MANGO_PLATFORM_IOS
        #define MANGO_PLATFORM_UNIX
        #define MANGO_PLATFORM_NAME "iOS"

    #else

        // Apple OS X
        #define MANGO_PLATFORM_OSX
        #define MANGO_PLATFORM_UNIX
        #define MANGO_PLATFORM_NAME "OS X"

    #endif

#elif defined(__ANDROID__)

    // Google Android
    #define MANGO_PLATFORM_ANDROID
    #define MANGO_PLATFORM_UNIX
    #define MANGO_PLATFORM_NAME "Android"

    #include <stdint.h>
    #include <malloc.h>

#elif defined(__linux__)

    // Linux
    #define MANGO_PLATFORM_LINUX
    #define MANGO_PLATFORM_UNIX
    #define MANGO_PLATFORM_NAME "Linux"

    #include <stdint.h>
    #include <malloc.h>

#elif defined(__CYGWIN__)

    // Cygwin
    #define MANGO_PLATFORM_CYGWIN
    #define MANGO_PLATFORM_UNIX
    #define MANGO_PLATFORM_NAME "Cygwin"

    #include <stdint.h>
    #include <malloc.h>

#elif defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)

    // BSD
    #define MANGO_PLATFORM_BSD
    #define MANGO_PLATFORM_UNIX
    #define MANGO_PLATFORM_NAME "BSD"

    #include <inttypes.h>
    #include <malloc.h>

#elif defined(sun) || defined(__sun)

    // SUN
    #define MANGO_PLATFORM_SUN
    #define MANGO_PLATFORM_UNIX
    #define MANGO_PLATFORM_NAME "SUN"

    #include <inttypes.h>
    #include <malloc.h>

#elif defined(__hpux)

    // HPUX
    #define MANGO_PLATFORM_HPUX
    #define MANGO_PLATFORM_UNIX
    #define MANGO_PLATFORM_NAME "HPUX"

    #include <inttypes.h>
    #include <malloc.h>

#elif defined(__sgi) || defined(__sgi__)

    // Silicon Graphics IRIX
    #define MANGO_PLATFORM_IRIX
    #define MANGO_PLATFORM_UNIX
    #define MANGO_PLATFORM_NAME "SGI IRIX"

#else

    // unsupported
    #error "Platform not supported."

#endif

// -----------------------------------------------------------------------
// compiler
// -----------------------------------------------------------------------

#if defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC)

    // Intel C/C++ Compiler
    #define MANGO_COMPILER_INTEL
	#define MANGO_PACKED(STRUCT) \
		__pragma( pack(push, 1) ) \
		STRUCT \
		__pragma( pack(pop) )

#elif defined(_MSC_VER)

    // Microsoft Visual C++
    #define MANGO_COMPILER_MICROSOFT
	#define MANGO_PACKED(STRUCT) \
		__pragma( pack(push, 1) ) \
		STRUCT \
		__pragma( pack(pop) )

	// noexcept specifier support was added in Visual Studio 2015
	#if _MSC_VER < 1900
		#define noexcept
	#endif

    // Fix <cmath> macros
    #define _USE_MATH_DEFINES

    // SSE2 is always supported on x64
    #if defined(_M_X64 ) || defined(_M_AMD64)
        #ifndef __SSE2__
        #define __SSE2__
        #endif
    #endif

    // AVX and AVX2 include support for these
    #if defined(__AVX__) || defined(__AVX2__)
        #ifndef __SSE3__
        #define __SSE3__
        #endif

        #ifndef __SSSE3__
        #define __SSSE3__
        #endif

        #ifndef __SSE4_1__
        #define __SSE4_1__
        #endif

        #ifndef __SSE4_2__
        #define __SSE4_2__
        #endif
    #endif

    #pragma warning(disable : 4996 4201)

#elif defined(__llvm__) || defined(__clang__)

    // LLVM / Clang
    #define MANGO_COMPILER_CLANG
	#define MANGO_PACKED(STRUCT) STRUCT __attribute__((__packed__))

#elif defined(__GNUC__)

    // GNU C/C++ Compiler
    #define MANGO_COMPILER_GCC
	#define MANGO_PACKED(STRUCT) STRUCT __attribute__((__packed__))

#elif defined(__MWERKS__)

    // Metrowerks CodeWarrior
	#define MANGO_PACKED(STRUCT) STRUCT

#elif defined(__COMO__)

    // Comeau C++
	#define MANGO_PACKED(STRUCT) STRUCT

#else

    // generic
	#define MANGO_PACKED(STRUCT) STRUCT

#endif

// -----------------------------------------------------------------------
// CPU
// -----------------------------------------------------------------------

#if defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)

    // 64 bit Intel
    #define MANGO_CPU_INTEL
    #define MANGO_CPU_64BIT
    #define MANGO_LITTLE_ENDIAN
    #define MANGO_UNALIGNED_MEMORY
    #define MANGO_CPU_NAME "x86_64"

#elif defined(_M_IX86) || defined(__i386__)

    // 32 bit Intel
    #define MANGO_CPU_INTEL
    #define MANGO_LITTLE_ENDIAN
    #define MANGO_UNALIGNED_MEMORY
    #define MANGO_CPU_NAME "x86"

#elif defined(__ia64__) || defined(__itanium__) || defined(_M_IA64)

	// Intel Itanium (IA-64)
	#define MANGO_CPU_INTEL
	#define MANGO_LITTLE_ENDIAN /* bi-endian; depends on OS */
	#define MANGO_CPU_NAME "Itanium"

#elif defined(__aarch64__)

    // 64 bit ARM
    #define MANGO_CPU_ARM
    #define MANGO_CPU_64BIT
    #define MANGO_LITTLE_ENDIAN /* bi-endian; depends on OS */
    #define MANGO_CPU_NAME "ARM64"

    #if defined(__ARM_FEATURE_UNALIGNED)
        #define MANGO_UNALIGNED_MEMORY
    #endif

#elif defined(__arm__)

    // 32 bit ARM
    #define MANGO_CPU_ARM
    #define MANGO_LITTLE_ENDIAN /* bi-endian; depends on OS */
    #define MANGO_CPU_NAME "ARM"

    #if defined(__ARM_FEATURE_UNALIGNED)
        #define MANGO_UNALIGNED_MEMORY
    #endif

#elif defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) || defined(__powerpc64le__) || defined(__ppc64le__) || defined(__PPC64LE__)

    // 64 bit PowerPC
    #define MANGO_CPU_PPC
    #define MANGO_CPU_64BIT

	#ifdef defined(__powerpc64le__) || defined(__ppc64le__) || defined(__PPC64LE__)
    	#define MANGO_LITTLE_ENDIAN
	#else
    	#define MANGO_BIG_ENDIAN /* bi-endian; depends on OS */
	#endif

    #define MANGO_CPU_NAME "PowerPC"

#elif defined(__powerpc__) || defined(_M_PPC)

    // 32 bit PowerPC
    #define MANGO_CPU_PPC
    #define MANGO_BIG_ENDIAN /* bi-endian; depends on OS */
    #define MANGO_CPU_NAME "PowerPC"

#elif defined(__m68k__)

	#define MANGO_CPU_M68K
	#define MANGO_BIG_ENDIAN
	#define MANGO_CPU_NAME "Motorola 68k"

#elif defined(__sparc) || defined(sparc)

    // SUN Sparc
    #define MANGO_CPU_SPARC
    #define MANGO_BIG_ENDIAN /* bi-endian; depends on OS */
    #define MANGO_CPU_NAME "Sparc"

#elif defined(__mips__) || defined(__mips64)

    // MIPS
    #define MANGO_CPU_MIPS
    #define MANGO_CPU_NAME "MIPS"

    #if (defined(MIPSEL) || (__MIPSEL__)) && !defined(_MIPSEB)
        #define MANGO_LITTLE_ENDIAN
    #else
        #define MANGO_BIG_ENDIAN
    #endif

    #if (_MIPS_SIM == _ABI64) || defined(__mips64)
        #define MANGO_CPU_64BIT
    #endif

#elif defined(__alpha__) || defined(_M_ALPHA)

	// Alpha
	#define MANGO_CPU_ALPHA
	#define MANGO_BIG_ENDIAN /* bi-endian; depends on OS */
	#define MANGO_CPU_NAME "Alpha"

#else

    // generic CPU
    #define MANGO_CPU_NAME "Generic"

	// last chance to detect endianess
	#include <stdlib.h>

	#if defined (__GLIBC__)
		#include <endian.h>
		#if (__BYTE_ORDER == __BIG_ENDIAN)
			#define MANGO_BIG_ENDIAN
		#else
			#define MANGO_LITTLE_ENDIAN
		#endif
	#else
	    #error "CPU endianess not supported."
	#endif

#endif

// last chance to detect a 64 bit processor
#if !defined(MANGO_CPU_64BIT) && (defined(__LP64__) || defined(__MINGW64__))
    #define MANGO_CPU_64BIT
#endif

// compiling for little endian
#if defined(__LITTLE_ENDIAN__) && defined(MANGO_BIG_ENDIAN)
    #undef MANGO_BIG_ENDIAN
    #define MANGO_LITTLE_ENDIAN
#endif

// compiling for big endian
#if defined(__BIG_ENDIAN__) && defined(MANGO_LITTLE_ENDIAN)
    #undef MANGO_LITTLE_ENDIAN
    #define MANGO_BIG_ENDIAN
#endif

// -----------------------------------------------------------------------
// SIMD
// -----------------------------------------------------------------------

#if defined(MANGO_CPU_INTEL)

    // Intel SSE vector intrinsics
    #define MANGO_ENABLE_SSE
    #include <xmmintrin.h>

    #ifdef __SSE2__
        // Required minimum feature level
        #define MANGO_ENABLE_SSE2
        #include <emmintrin.h>
    #endif

    #ifdef __SSE3__
        #define MANGO_ENABLE_SSE3
        #include <pmmintrin.h>
    #endif

    #ifdef __SSSE3__
        #define MANGO_ENABLE_SSSE3
        #include <tmmintrin.h>
    #endif

    #ifdef __SSE4_1__
        #define MANGO_ENABLE_SSE4_1
        #include <smmintrin.h>
    #endif

    #ifdef __SSE4_2__
        #define MANGO_ENABLE_SSE4_2
        #include <nmmintrin.h>
    #endif

    #ifdef __AVX__
        #define MANGO_ENABLE_AVX
        #include <immintrin.h>
    #endif

    #ifdef __AVX2__
        #define MANGO_ENABLE_AVX2
        #include <immintrin.h>
    #endif

    #ifdef __XOP__
        #if defined(MANGO_COMPILER_MICROSOFT)
            #define MANGO_ENABLE_XOP
            #define MANGO_ENABLE_FMA4
            #include <ammintrin.h>
        #elif defined(MANGO_COMPILER_GCC) || defined(MANGO_COMPILER_CLANG)
            #define MANGO_ENABLE_XOP
            #define MANGO_ENABLE_FMA4
            #include <x86intrin.h>
        #endif
    #endif

    #ifdef __F16C__
        #define MANGO_ENABLE_F16C
        #include <immintrin.h>
    #endif

    #ifdef __POPCNT__
        #define MANGO_ENABLE_POPCNT
    #endif

    #ifdef __BMI__
        #define MANGO_ENABLE_BMI
    #endif

    #ifdef __BMI2__
        #define MANGO_ENABLE_BMI2
    #endif

    #if defined(__FMA__) && !defined(MANGO_ENABLE_FMA3)
        #define MANGO_ENABLE_FMA3
        #include <immintrin.h>
    #endif

    #if defined(__FMA4__) && !defined(MANGO_ENABLE_FMA4)
        #if defined(MANGO_COMPILER_MICROSOFT)
            #define MANGO_ENABLE_FMA4
            #include <intrin.h>
        #elif defined(MANGO_COMPILER_GCC) || defined(MANGO_COMPILER_CLANG)
            #define MANGO_ENABLE_FMA4
            #include <x86intrin.h>
        #endif
    #endif

#elif defined(MANGO_CPU_ARM) && defined(__ARM_NEON__)

    // ARM NEON vector instrinsics
    #define MANGO_ENABLE_NEON
    #include <arm_neon.h>

    // ARM FP feature bits
    #if ((__ARM_FP & 0x2) != 0)
        #define MANGO_ENABLE_FP16
    #endif

#elif defined(MANGO_CPU_PPC) && (defined(__VEC__) || defined(__PPU__))

    // PowerPC Altivec / AVX128
    #define MANGO_ENABLE_ALTIVEC
    #include <altivec.h>

#elif defined(MANGO_CPU_PPC) && defined(__SPU__)

    // Cell BE SPU
    #define MANGO_ENABLE_SPU
    #include <spu_intrinsics.h>

#endif

// -----------------------------------------------------------------------
// import / export
// -----------------------------------------------------------------------

#ifdef MANGO_PLATFORM_WINDOWS

    #define MANGO_IMPORT __declspec(dllimport)
    #define MANGO_EXPORT __declspec(dllexport)

#elif __GNUC__ >= 4

    #define MANGO_IMPORT __attribute__ ((__visibility__ ("default")))
    #define MANGO_EXPORT __attribute__ ((__visibility__ ("default")))

#else

    #define MANGO_IMPORT
    #define MANGO_EXPORT

#endif

// -----------------------------------------------------------------------
// misc
// -----------------------------------------------------------------------

#define MANGO_UNREFERENCED_PARAMETER(x) (void) x

// -----------------------------------------------------------------------
// options
// -----------------------------------------------------------------------

#define MANGO_MEMORY_ALIGNMENT 16

// -----------------------------------------------------------------------
// licenses
// -----------------------------------------------------------------------

#ifndef MANGO_DISABLE_LICENSE_ZLIB
    #define MANGO_ENABLE_LICENSE_ZLIB
    // bzip2
#endif

#ifndef MANGO_DISABLE_LICENSE_BSD
    #define MANGO_ENABLE_LICENSE_BSD
    // lz4, jpeg.arithmetic
#endif

#ifndef MANGO_DISABLE_LICENSE_GPL
    #define MANGO_ENABLE_LICENSE_GPL
    // unrar
#endif

#ifndef MANGO_DISABLE_LICENSE_MICROSOFT
    #define MANGO_ENABLE_LICENSE_MICROSOFT
    // BC4,5,6,7 texture compression
#endif

#ifndef MANGO_DISABLE_LICENSE_APACHE
    #define MANGO_ENABLE_LICENSE_APACHE
    // ETC1, ETC2, ASTC texture compression
#endif

// -----------------------------------------------------------------------
// typedefs
// -----------------------------------------------------------------------

namespace mango
{

    typedef std::int8_t    int8;
    typedef std::int16_t   int16;
    typedef std::int32_t   int32;
    typedef std::int64_t   int64;

    typedef std::uint8_t   uint8;
    typedef std::uint16_t  uint16;
    typedef std::uint32_t  uint32;
    typedef std::uint64_t  uint64;

} // namespace mango
