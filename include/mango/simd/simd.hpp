/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cmath>
#include <algorithm>
#include "../core/configure.hpp"
#include "../core/half.hpp"
#include "../core/bits.hpp"

// ------------------------------------------------------------------
// Configure SIMD implementation
// ------------------------------------------------------------------

namespace mango {
namespace simd {

#if defined(MANGO_ENABLE_AVX)

    // --------------------------------------------------------------
    // Intel AVX vector intrinsics
    // --------------------------------------------------------------

    struct simd4h
    {
        half x, y, z, w;
    };

    typedef __m128i simd4i;
    typedef __m128 simd4f;
    typedef __m256d simd4d;

#if defined(MANGO_COMPILER_MICROSOFT) || defined(MANGO_COMPILER_INTEL)
    typedef const __m128i& __simd4i;
    typedef const simd4h& __simd4h;
    typedef const __m128& __simd4f;
    typedef const __m256d& __simd4d;
#else
    typedef const __m128i __simd4i;
    typedef const simd4h& __simd4h;
    typedef const __m128 __simd4f;
    typedef const __m256d __simd4d;
#endif

    #define MANGO_SIMD_INT
    #define MANGO_SIMD_FLOAT
    #define MANGO_SIMD_DOUBLE

    #include "int_sse.hpp"
    #include "float_sse.hpp"
    #include "double_avx.hpp"

#elif defined(MANGO_ENABLE_SSE2)

    // --------------------------------------------------------------
    // Intel SSE vector intrinsics
    // --------------------------------------------------------------

    struct simd4h
    {
        half x, y, z, w;
    };

    struct simd4d
    {
        __m128d xy;
        __m128d zw;
    };

    typedef __m128i simd4i;
    typedef __m128 simd4f;

#if defined(MANGO_COMPILER_MICROSOFT) || defined(MANGO_COMPILER_INTEL)
    typedef const __m128i& __simd4i;
    typedef const simd4h& __simd4h;
    typedef const __m128& __simd4f;
    typedef const simd4d& __simd4d;
#else
    typedef const __m128i __simd4i;
    typedef const simd4h& __simd4h;
    typedef const __m128 __simd4f;
    typedef const simd4d& __simd4d;
#endif

    #define MANGO_SIMD_INT
    #define MANGO_SIMD_FLOAT
    #define MANGO_SIMD_DOUBLE

    #include "int_sse.hpp"
    #include "float_sse.hpp"
    #include "double_sse.hpp"

#elif defined(MANGO_ENABLE_NEON)

    // --------------------------------------------------------------
    // ARM NEON vector instrinsics
    // --------------------------------------------------------------

#ifdef MANGO_ENABLE_FP16

    typedef float16x4_t simd4h;

#else

    struct simd4h
    {
        half x, y, z, w;
    };

#endif

    struct simd4d
    {
        double x, y, z, w;
    };

    typedef int32x4_t simd4i;
    typedef float32x4_t simd4f;

    typedef const int32x4_t __simd4i;
    typedef const simd4h __simd4h;
    typedef const float32x4_t __simd4f;
    typedef const simd4d& __simd4d;

    #define MANGO_SIMD_INT
    #define MANGO_SIMD_FLOAT

    #include "int_neon.hpp"
    #include "float_neon.hpp"
    #include "double_scalar.hpp"

#elif defined(MANGO_ENABLE_ALTIVEC)

    // --------------------------------------------------------------
    // PowerPC Altivec / AVX128
    // --------------------------------------------------------------

    struct simd4h
    {
        half x, y, z, w;
    };

    struct simd4d
    {
        double x, y, z, w;
    };

    typedef vector signed int simd4i;
    typedef vector float simd4f;

    typedef const vector signed int __simd4i;
    typedef const simd4h& __simd4h;
    typedef const vector float __simd4f;
    typedef const simd4d& __simd4d;

    #define MANGO_SIMD_INT
    #define MANGO_SIMD_FLOAT

    #include "int_altivec.hpp"
    #include "float_altivec.hpp"
    #include "double_scalar.hpp"

#elif defined(MANGO_ENABLE_SPU)

    // --------------------------------------------------------------
    // Cell BE SPU
    // --------------------------------------------------------------

    struct simd4h
    {
        half x, y, z, w;
    };

    struct simd4d
    {
        double x, y, z, w;
    };

    typedef vector signed int simd4i;
    typedef vector float simd4f;

    typedef const vector signed int __simd4i;
    typedef const simd4h& __simd4h;
    typedef const vector float __simd4f;
    typedef const simd4d& __simd4d;

    #define MANGO_SIMD_INT
    #define MANGO_SIMD_FLOAT

    #include "int_spu.hpp"
    #include "float_spu.hpp"
    #include "double_scalar.hpp"

#endif

    // --------------------------------------------------------------
    // SIMD emulation
    // --------------------------------------------------------------

    // Implement emulation for types which don't have support in hardware

#ifndef MANGO_SIMD_INT

    #define MANGO_SCALAR_INT

    struct simd4i
    {
        int32 x, y, z, w;
    };

    typedef const simd4i& __simd4i;

#endif

#ifndef MANGO_SIMD_FLOAT

    #define MANGO_SCALAR_FLOAT

    struct simd4h
    {
        half x, y, z, w;
    };

    struct simd4f
    {
        float x, y, z, w;
    };

    typedef const simd4h& __simd4h;
    typedef const simd4f& __simd4f;

#endif

#ifndef MANGO_SIMD_DOUBLE

    #define MANGO_SCALAR_DOUBLE

    struct simd4d
    {
        double x, y, z, w;
    };

    typedef const simd4d& __simd4d;

#endif

    // Include the headers only after all of the types have been defined

#ifdef MANGO_SCALAR_INT
    #include "int_scalar.hpp"
#endif

#ifdef MANGO_SCALAR_FLOAT
    #include "float_scalar.hpp"
#endif

#ifdef MANGO_SCALAR_DOUBLE
    #include "double_scalar.hpp"
#endif

} // namespace simd
} // namespace mango

// TODO: remove this hack after the refactoring is done
using namespace mango::simd;

#include "common.hpp"
