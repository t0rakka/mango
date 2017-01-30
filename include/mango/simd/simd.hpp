/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cmath>
#include <algorithm>
#include <type_traits>
#include "../core/configure.hpp"
#include "../core/half.hpp"
#include "../core/bits.hpp"

// ------------------------------------------------------------------
// Configure SIMD implementation
// ------------------------------------------------------------------

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // SIMD vector base classes
    // --------------------------------------------------------------

    template <typename T, bool Signed>
    struct vector_type
    {
        typedef T type;
        T m;

        vector_type() = default;

        vector_type(T m) : m(m)
        {
        }

        operator T () const
        {
            return m;
        }
    };

    template <typename T, int Size>
    struct scalar_type
    {
        T data[Size];

        scalar_type() = default;

        T & operator [] (int index)
        {
            return reinterpret_cast<T *>(this)[index];
        }

        const T & operator [] (int index) const
        {
            return reinterpret_cast<const T *>(this)[index];
        }
    };

    template <typename T>
    struct scalar_type<T, 4>
    {
        T data[4];

        scalar_type() = default;

        scalar_type(T x, T y, T z, T w)
        {
            data[0] = x;
            data[1] = y;
            data[2] = z;
            data[3] = w;
        }

        T & operator [] (int index)
        {
            return reinterpret_cast<T *>(this)[index];
        }

        const T & operator [] (int index) const
        {
            return reinterpret_cast<const T *>(this)[index];
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

    typedef vector_type<__m128i, false> uint32x4;
    typedef vector_type<__m128i, true> int32x4;
    typedef vector_type<__m128, true> float32x4;
    typedef vector_type<__m256d, true> float64x4;
    typedef scalar_type<half, 4> float16x4;

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

    typedef vector_type<__m128i, false> uint32x4;
    typedef vector_type<__m128i, true> int32x4;
    typedef vector_type<__m128, true> float32x4;
    typedef scalar_type<half, 4> float16x4;

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

    typedef vector_type<uint32x4_t, false> uint32x4;
    typedef vector_type<int32x4_t, true> int32x4;
    typedef vector_type<float32x4_t, true> float32x4;

#ifdef MANGO_ENABLE_FP16

    typedef vector_type<float16x4_t, true> float16x4;

#else

    typedef scalar_type<half, 4> float16x4;

#endif

    typedef scalar_type<double, 4> float64x4;

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

    typedef vector_type<vector unsigned int, false> uint32x4;
    typedef vector_type<vector signed int, true> int32x4;
    typedef vector_type<vector float, true> float32x4;
    typedef scalar_type<double, 4> float64x4;
    typedef scalar_type<half, 4> float16x4;

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

    typedef vector_type<vector unsigned int, false> uint32x4;
    typedef vector_type<vector signed int, true> int32x4;
    typedef vector_type<vector float, true> float32x4;
    typedef scalar_type<double, 4> float64x4;
    typedef scalar_type<half, 4> float16x4;

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

    typedef scalar_type<uint32, 4> uint32x4;
    typedef scalar_type<int32, 4> int32x4;
    typedef scalar_type<float, 4> float32x4;
    typedef scalar_type<double, 4> float64x4;
    typedef scalar_type<half, 4> float16x4;

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
