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

    struct int32x4
    {
        __m128i m;

        int32x4() = default;

        int32x4(__m128i m) : m(m)
        {
        }

        operator __m128i () const
        {
            return m;
        }
    };

    struct float16x4
    {
        half x, y, z, w;
    };

    struct float32x4
    {
        __m128 m;

        float32x4() = default;

        float32x4(__m128 m) : m(m)
        {
        }

        operator __m128 () const
        {
            return m;
        }
    };

    struct float64x4
    {
        __m256d m;

        float64x4() = default;

        float64x4(__m256d m) : m(m)
        {
        }

        operator __m256d () const
        {
            return m;
        }
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

    struct int32x4
    {
        __m128i m;

        int32x4() = default;

        int32x4(__m128i m) : m(m)
        {
        }

        operator __m128i () const
        {
            return m;
        }
    };

    struct float16x4
    {
        half x, y, z, w;
    };

    struct float32x4
    {
        __m128 m;

        float32x4() = default;

        float32x4(__m128 m) : m(m)
        {
        }

        operator __m128 () const
        {
            return m;
        }
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

    struct int32x4
    {
        int32x4_t m;

        int32x4() = default;

        int32x4(int32x4_t m) : m(m)
        {
        }

        operator int32x4_t () const
        {
            return m;
        }
    };

#ifdef MANGO_ENABLE_FP16

    struct float16x4
    {
        float16x4_t m;

        float16x4() = default;

        float16x4(float16x4_t m) : m(m)
        {
        }

        operator float16x4_t () const
        {
            return m;
        }
    };

#else

    struct float16x4
    {
        half x, y, z, w;
    };

#endif

    struct float32x4
    {
        float32x4_t m;

        float32x4() = default;

        float32x4(float32x4_t m) : m(m)
        {
        }

        operator float32x4_t () const
        {
            return m;
        }
    };

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

    struct int32x4
    {
        typedef vector signed int vectype;
        vectype m;

        int32x4() = default;

        int32x4(vectype m) : m(m)
        {
        }

        operator vectype () const
        {
            return m;
        }
    };

    struct float16x4
    {
        half x, y, z, w;
    };

    struct float32x4
    {
        typedef vector float vectype;
        vectype m;

        float32x4() = default;

        float32x4(vectype m) : m(m)
        {
        }

        operator vectype () const
        {
            return m;
        }
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

    struct int32x4
    {
        typedef vector signed int vectype;
        vectype m;

        int32x4() = default;

        int32x4(vectype m) : m(m)
        {
        }

        operator vectype () const
        {
            return m;
        }
    };

    struct float16x4
    {
        half x, y, z, w;
    };

    struct float32x4
    {
        typedef vector float vectype;
        vectype m;

        float32x4() = default;

        float32x4(vectype m) : m(m)
        {
        }

        operator vectype () const
        {
            return m;
        }
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
