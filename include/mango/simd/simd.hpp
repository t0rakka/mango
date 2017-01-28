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

} // namespace simd
} // namespace mango

#if defined(MANGO_ENABLE_AVX)

namespace mango {
namespace simd {

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

} // namespace simd
} // namespace mango

    #define MANGO_SIMD_INT_SSE
    #define MANGO_SIMD_FLOAT_SSE
    #define MANGO_SIMD_DOUBLE_AVX
    #define MANGO_SIMD_CONVERT_AVX
    #include "sse_int.hpp"
    #include "sse_float.hpp"
    #include "avx_double.hpp"
    #include "avx_convert.hpp"

#elif defined(MANGO_ENABLE_SSE2)

namespace mango {
namespace simd {

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

} // namespace simd
} // namespace mango

    #define MANGO_SIMD_INT_SSE
    #define MANGO_SIMD_FLOAT_SSE
    #define MANGO_SIMD_DOUBLE_SSE
    #define MANGO_SIMD_CONVERT_SSE
    #include "sse_int.hpp"
    #include "sse_float.hpp"
    #include "sse_double.hpp"
    #include "sse_convert.hpp"

#elif defined(MANGO_ENABLE_NEON)

namespace mango {
namespace simd {

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

} // namespace simd
} // namespace mango

    #define MANGO_SIMD_INT_NEON
    #define MANGO_SIMD_FLOAT_NEON
    #define MANGO_SIMD_DOUBLE_SCALAR
    #define MANGO_SIMD_CONVERT_NEON
    #include "neon_int.hpp"
    #include "neon_float.hpp"
    #include "scalar_double.hpp"
    #include "neon_convert.hpp"

#elif defined(MANGO_ENABLE_ALTIVEC)

namespace mango {
namespace simd {

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

} // namespace simd
} // namespace mango

    #define MANGO_SIMD_INT_ALTIVEC
    #define MANGO_SIMD_FLOAT_ALTIVEC
    #define MANGO_SIMD_DOUBLE_SCALAR
    #define MANGO_SIMD_CONVERT_ALTIVEC
    #include "altivec_int.hpp"
    #include "altivec_float.hpp"
    #include "scalar_double.hpp"
    #include "altivec_convert.hpp"

#elif defined(MANGO_ENABLE_SPU)

namespace mango {
namespace simd {

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

} // namespace simd
} // namespace mango

    #define MANGO_SIMD_INT_SPU
    #define MANGO_SIMD_FLOAT_SPU
    #define MANGO_SIMD_DOUBLE_SCALAR
    #define MANGO_SIMD_CONVERT_SPU
    #include "spu_int.hpp"
    #include "spu_float.hpp"
    #include "scalar_double.hpp"
    #include "spu_convert.hpp"

#else

namespace mango {
namespace simd {

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

} // namespace simd
} // namespace mango

    #define MANGO_SIMD_INT_SCALAR
    #define MANGO_SIMD_FLOAT_SCALAR
    #define MANGO_SIMD_DOUBLE_SCALAR
    #define MANGO_SIMD_CONVERT_SCALAR
    #include "scalar_int.hpp"
    #include "scalar_float.hpp"
    #include "scalar_double.hpp"
    #include "scalar_convert.hpp"

#endif
