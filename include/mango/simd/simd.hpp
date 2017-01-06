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

    struct float16x4
    {
        half x, y, z, w;
    };

    typedef __m128i int32x4;
    typedef __m128  float32x4;
    typedef __m256d float64x4;

#if defined(MANGO_COMPILER_MICROSOFT) || defined(MANGO_COMPILER_INTEL)
    typedef const __m128i&   int32x4__;
    typedef const float16x4& float16x4__;
    typedef const __m128&    float32x4__;
    typedef const __m256d&   float64x4__;
#else
    typedef const __m128i    int32x4__;
    typedef const float16x4& float16x4__;
    typedef const __m128     float32x4__;
    typedef const __m256d    float64x4__;
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

    struct float16x4
    {
        half x, y, z, w;
    };

    struct float64x4
    {
        __m128d xy;
        __m128d zw;
    };

    typedef __m128i int32x4;
    typedef __m128 float32x4;

#if defined(MANGO_COMPILER_MICROSOFT) || defined(MANGO_COMPILER_INTEL)
    typedef const __m128i&   int32x4__;
    typedef const float16x4& float16x4__;
    typedef const __m128&    float32x4__;
    typedef const float64x4& float64x4__;
#else
    typedef const __m128i    int32x4__;
    typedef const float16x4& float16x4__;
    typedef const __m128     float32x4__;
    typedef const float64x4& float64x4__;
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

    typedef float16x4_t float16x4;

#else

    struct float16x4
    {
        half x, y, z, w;
    };

#endif

    struct float64x4
    {
        double x, y, z, w;
    };

    typedef int32x4_t int32x4;
    typedef float32x4_t float32x4;

    typedef const int32x4_t   int32x4__;
    typedef const float16x4   float16x4__;
    typedef const float32x4_t float32x4__;
    typedef const float64x4&  float64x4__;

    #define MANGO_SIMD_INT
    #define MANGO_SIMD_FLOAT
    #define MANGO_SCALAR_DOUBLE

    #include "int_neon.hpp"
    #include "float_neon.hpp"
    #include "double_scalar.hpp"

#elif defined(MANGO_ENABLE_ALTIVEC)

    // --------------------------------------------------------------
    // PowerPC Altivec / AVX128
    // --------------------------------------------------------------

    struct float16x4
    {
        half x, y, z, w;
    };

    struct float64x4
    {
        double x, y, z, w;
    };

    typedef vector signed int int32x4;
    typedef vector float float32x4;

    typedef const vector signed int int32x4__;
    typedef const float16x4&        float16x4__;
    typedef const vector float      float32x4__;
    typedef const float64x4&        float64x4__;

    #define MANGO_SIMD_INT
    #define MANGO_SIMD_FLOAT
    #define MANGO_SCALAR_DOUBLE

    #include "int_altivec.hpp"
    #include "float_altivec.hpp"
    #include "double_scalar.hpp"

#elif defined(MANGO_ENABLE_SPU)

    // --------------------------------------------------------------
    // Cell BE SPU
    // --------------------------------------------------------------

    struct float16x4
    {
        half x, y, z, w;
    };

    struct float64x4
    {
        double x, y, z, w;
    };

    typedef vector signed int int32x4;
    typedef vector float      float32x4;

    typedef const vector signed int int32x4__;
    typedef const float16x4&        float16x4__;
    typedef const vector float      float32x4__;
    typedef const float64x4&        float64x4__;

    #define MANGO_SIMD_INT
    #define MANGO_SIMD_FLOAT
    #define MANGO_SCALAR_DOUBLE

    #include "int_spu.hpp"
    #include "float_spu.hpp"
    #include "double_scalar.hpp"

#else

    // --------------------------------------------------------------
    // SIMD emulation
    // --------------------------------------------------------------

    struct int32x4
    {
        int32 x, y, z, w;
    };

    struct float16x4
    {
        half x, y, z, w;
    };

    struct float32x4
    {
        float x, y, z, w;
    };

    struct float64x4
    {
        double x, y, z, w;
    };

    typedef const int32x4&   int32x4__;
    typedef const float16x4& float16x4__;
    typedef const float32x4& float32x4__;
    typedef const float64x4& float64x4__;

    #define MANGO_SCALAR_INT
    #define MANGO_SCALAR_FLOAT
    #define MANGO_SCALAR_DOUBLE

    #include "int_scalar.hpp"
    #include "float_scalar.hpp"
    #include "double_scalar.hpp"

#endif

} // namespace simd
} // namespace mango

#include "common.hpp"
