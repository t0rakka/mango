#ifndef _RAR_RARCOMMON_
#define _RAR_RARCOMMON_

#include <cstddef>
#include <cstring>
#include <cmath>
#include <cstdlib>

#if defined(__WIN32__) || defined(_WIN32)
#define _WIN_ALL // Defined for all Windows platforms, 32 and 64 bit, mobile and desktop.
#define _CRT_SECURE_NO_WARNINGS // mango: deprecate warnings
#ifdef _M_X64
#define _WIN_64
#else
#define _WIN_32
#endif
#endif

#ifdef __APPLE__
#define _UNIX
#define _APPLE
#endif

#if !defined(_WIN_ALL) && !defined(_APPLE)
#define _UNIX
#endif


#if defined(_WIN_ALL)

#define LITTLE_ENDIAN
#define NM  1024

#endif


#ifdef _UNIX

#define  NM  1024

#ifdef _APPLE
#if defined(__BIG_ENDIAN__) && !defined(BIG_ENDIAN)
#define BIG_ENDIAN
#undef LITTLE_ENDIAN
#endif
#if defined(__i386__) && !defined(LITTLE_ENDIAN)
#define LITTLE_ENDIAN
#undef BIG_ENDIAN
#endif
#endif

#if defined(__sparc) || defined(sparc) || defined(__hpux)
#ifndef BIG_ENDIAN
#define BIG_ENDIAN
#endif
#endif

#endif

// ARM64 / x86_64
#if defined(__aarch64__) || defined(__x86_64__)
#if !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
#define LITTLE_ENDIAN
#endif
#elif defined(__arm__)
#if !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
#define LITTLE_ENDIAN
#endif
#endif

// MIPS
#if defined(__mips__)
#if !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
    #if defined(MIPSEL) || (__MIPSEL__)
    #define LITTLE_ENDIAN
    #else
    #define BIG_ENDIAN
    #endif
#endif
#endif

// RISCV
#if defined(__riscv)
    #define LITTLE_ENDIAN
#endif

#if !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
#if defined(__i386) || defined(i386) || defined(__i386__) || defined(EMSCRIPTEN)
#define LITTLE_ENDIAN
#elif defined(BYTE_ORDER) && BYTE_ORDER == LITTLE_ENDIAN
#define LITTLE_ENDIAN
#elif defined(BYTE_ORDER) && BYTE_ORDER == BIG_ENDIAN
#define BIG_ENDIAN
#else
#error "Neither LITTLE_ENDIAN nor BIG_ENDIAN are defined. Define one of them."
#endif
#endif

#if defined(LITTLE_ENDIAN) && defined(BIG_ENDIAN)
#if defined(BYTE_ORDER) && BYTE_ORDER == BIG_ENDIAN
#undef LITTLE_ENDIAN
#elif defined(BYTE_ORDER) && BYTE_ORDER == LITTLE_ENDIAN
#undef BIG_ENDIAN
#else
#error "Both LITTLE_ENDIAN and BIG_ENDIAN are defined. Undef one of them."
#endif
#endif

#if !defined(BIG_ENDIAN) && !defined(_WIN_CE) && defined(_WIN_ALL)
// Allow not aligned integer access, increases speed in some operations.
#define ALLOW_NOT_ALIGNED_INT
#endif

#if defined(__sparc) || defined(sparc) || defined(__sparcv9)
// Prohibit not aligned access to data structures in text compression
// algorithm, increases memory requirements
#define STRICT_ALIGNMENT_REQUIRED
#endif

typedef unsigned char    byte;   // unsigned 8 bits
typedef unsigned short   ushort; // preferably 16 bits, but can be more
typedef unsigned int     uint;   // 32 bits or more

#define PRESENT_INT32 // undefine if signed 32 bits is not available

typedef unsigned int     uint32; // 32 bits exactly
typedef   signed int     int32;  // signed 32 bits exactly

// If compiler does not support 64 bit variables, we can define
// uint64 and int64 as 32 bit, but it will limit the maximum processed
// file size to 2 GB.
#if defined(__BORLANDC__) || defined(_MSC_VER)
typedef   unsigned __int64 uint64; // unsigned 64 bits
typedef     signed __int64  int64; // signed 64 bits
#else
typedef unsigned long long uint64; // unsigned 64 bits
typedef   signed long long  int64; // signed 64 bits
#endif


#define GET_SHORT16(x) (sizeof(ushort)==2 ? (ushort)(x):((x)&0xffff))
#define GET_UINT32(x)  (sizeof(uint32)==4 ? (uint32)(x):((x)&0xffffffff))


#define  Min(x,y) (((x)<(y)) ? (x):(y))
#define  Max(x,y) (((x)>(y)) ? (x):(y))

#define  ASIZE(x) (sizeof(x)/sizeof(x[0]))

// MAXPASSWORD is expected to be multiple of CRYPTPROTECTMEMORY_BLOCK_SIZE (16)
// for CryptProtectMemory in SecPassword.
#define  MAXPASSWORD       128

#include "array.hpp"
#include "crc.hpp"
#include "rijndael.hpp"
#include "crypt.hpp"
#include "getbits.hpp"
#include "rdwrfn.hpp"
#include "compress.hpp"
#include "rarvm.hpp"
#include "unpack.hpp"

#endif
