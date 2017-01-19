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

    // --------------------------------------------------------------
    // SIMD vector strong_type
    // --------------------------------------------------------------

    template <typename T, bool Signed>
    struct strong_type
    {
        typedef T type;
        T m;

        strong_type() = default;

        strong_type(T m) : m(m)
        {
        }

        operator T () const
        {
            return m;
        }
    };

#if defined(MANGO_ENABLE_AVX)

    // --------------------------------------------------------------
    // Intel AVX vector intrinsics
    // --------------------------------------------------------------

    typedef strong_type<__m128i, false> uint32x4;
    typedef strong_type<__m128i, true> int32x4;
    typedef strong_type<__m128, true> float32x4;
    typedef strong_type<__m256d, true> float64x4;

    struct float16x4
    {
        half x, y, z, w;
    };

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

    typedef strong_type<__m128i, false> uint32x4;
    typedef strong_type<__m128i, true> int32x4;
    typedef strong_type<__m128, true> float32x4;

    struct float16x4
    {
        half x, y, z, w;
    };

    struct float64x4
    {
        __m128d xy;
        __m128d zw;
    };

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

    typedef strong_type<uint32x4_t, false> uint32x4;
    typedef strong_type<int32x4_t, true> int32x4;
    typedef strong_type<float32x4_t, true> float32x4;

#ifdef MANGO_ENABLE_FP16

    typedef strong_type<float16x4_t, true> float16x4;

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

    typedef strong_type<vector unsigned int, false> uint32x4;
    typedef strong_type<vector signed int, true> int32x4;
    typedef strong_type<vector float, true> float32x4;

    struct float16x4
    {
        half x, y, z, w;
    };

    struct float64x4
    {
        double x, y, z, w;
    };

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

    typedef strong_type<vector unsigned int, false> uint32x4;
    typedef strong_type<vector signed int, true> int32x4;
    typedef strong_type<vector float, true> float32x4;

    struct float16x4
    {
        half x, y, z, w;
    };

    struct float64x4
    {
        double x, y, z, w;
    };

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

    struct uint32x4
    {
        uint32 x, y, z, w;
    };

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
