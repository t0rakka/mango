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

    // The C++ does not have strong typedef:
    // Wrap the hardware-supported SIMD vector types so that we get
    // strong types for the compiler to be able to resolve the overloads
    // when the underlying types are identical.

    template <typename ScalarType, int Size, typename VectorType>
    struct vector_type
    {
        typedef VectorType type;
        VectorType m;

        vector_type() = default;

        vector_type(VectorType m) : m(m)
        {
        }

        operator VectorType () const
        {
            return m;
        }
    };

    template <typename ScalarType, int Bits, int Size>
    struct scalar_type
    {
        typedef ScalarType type;
        enum {
            bits = Bits, 
            size = Size 
        };
        ScalarType data[Size];

        ScalarType & operator [] (int index)
        {
            return reinterpret_cast<ScalarType *>(this)[index];
        }

        const ScalarType & operator [] (int index) const
        {
            return reinterpret_cast<const ScalarType *>(this)[index];
        }
    };

} // namespace simd
} // namespace mango

#if defined(MANGO_ENABLE_AVX2)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // Intel AVX2 vector intrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    typedef vector_type<int8, 16, __m128i>    int8x16;
    typedef vector_type<int16, 8, __m128i>    int16x8;
    typedef vector_type<int32, 4, __m128i>    int32x4;
    typedef vector_type<int64, 2, __m128i>    int64x2;
    typedef vector_type<uint8, 16, __m128i>   uint8x16;
    typedef vector_type<uint16, 8, __m128i>   uint16x8;
    typedef vector_type<uint32, 4, __m128i>   uint32x4;
    typedef vector_type<uint64, 2, __m128i>   uint64x2;
    typedef vector_type<int8, 32, __m256i>    int8x32;
    typedef vector_type<int16, 16, __m256i>   int16x16;
    typedef vector_type<int32, 8, __m256i>    int32x8;
    typedef vector_type<int64, 4, __m256i>    int64x4;
    typedef vector_type<uint8, 32, __m256i>   uint8x32;
    typedef vector_type<uint16, 16, __m256i>  uint16x16;
    typedef vector_type<uint32, 8, __m256i>   uint32x8;
    typedef vector_type<uint64, 4, __m256i>   uint64x4;
    typedef scalar_type<half, 16, 4>          float16x4;
    typedef scalar_type<float, 32, 2>         float32x2;
    typedef vector_type<float, 4, __m128>     float32x4;
    typedef vector_type<float, 8, __m256>     float32x8;
    typedef vector_type<double, 2, __m128d>   float64x2;
    typedef vector_type<double, 4, __m256d>   float64x4;

} // namespace simd
} // namespace mango

    #include "sse_int128.hpp"
    #include "avx2_int256.hpp"
    #include "scalar_float64.hpp"
    #include "sse_float128.hpp"
    #include "avx_float256.hpp"
    #include "sse_double128.hpp"
    #include "avx_double256.hpp"
    #include "avx_convert.hpp"

#elif defined(MANGO_ENABLE_AVX)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // Intel AVX vector intrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    typedef vector_type<int8, 16, __m128i>   int8x16;
    typedef vector_type<int16, 8, __m128i>   int16x8;
    typedef vector_type<int32, 4, __m128i>   int32x4;
    typedef vector_type<int64, 2, __m128i>   int64x2;
    typedef vector_type<uint8, 16, __m128i>  uint8x16;
    typedef vector_type<uint16, 8, __m128i>  uint16x8;
    typedef vector_type<uint32, 4, __m128i>  uint32x4;
    typedef vector_type<uint64, 2, __m128i>  uint64x2;
    typedef scalar_type<half, 16, 4>         float16x4;
    typedef scalar_type<float, 32, 2>        float32x2;
    typedef vector_type<float, 4, __m128>    float32x4;
    typedef vector_type<float, 8, __m256>    float32x8;
    typedef vector_type<double, 2, __m128d>  float64x2;
    typedef vector_type<double, 4, __m256d>  float64x4;

    struct int8x32
    {
        int8x16 lo, hi;
    };

    struct int16x16
    {
        int16x8 lo, hi;
    };

    struct int32x8
    {
        int32x4 lo, hi;
    };

    struct int64x4
    {
        int64x2 lo, hi;
    };

    struct uint8x32
    {
        uint8x16 lo, hi;
    };

    struct uint16x16
    {
        uint16x8 lo, hi;
    };

    struct uint32x8
    {
        uint32x4 lo, hi;
    };

    struct uint64x4
    {
        uint64x2 lo, hi;
    };

} // namespace simd
} // namespace mango

    #include "sse_int128.hpp"
    #include "common_int256.hpp"
    #include "scalar_float64.hpp"
    #include "sse_float128.hpp"
    #include "avx_float256.hpp"
    #include "sse_double128.hpp"
    #include "avx_double256.hpp"
    #include "avx_convert.hpp"

#elif defined(MANGO_ENABLE_SSE2)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // Intel SSE vector intrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    typedef vector_type<int8, 16, __m128i>   int8x16;
    typedef vector_type<int16, 8, __m128i>   int16x8;
    typedef vector_type<int32, 4, __m128i>   int32x4;
    typedef vector_type<int64, 2, __m128i>   int64x2;
    typedef vector_type<uint8, 16, __m128i>  uint8x16;
    typedef vector_type<uint16, 8, __m128i>  uint16x8;
    typedef vector_type<uint32, 4, __m128i>  uint32x4;
    typedef vector_type<uint64, 2, __m128i>  uint64x2;
    typedef scalar_type<half, 16, 4>         float16x4;
    typedef scalar_type<float, 32, 2>        float32x2;
    typedef vector_type<float, 4, __m128>    float32x4;
    typedef vector_type<double, 2, __m128d>  float64x2;

    struct int8x32
    {
        int8x16 lo, hi;
    };

    struct int16x16
    {
        int16x8 lo, hi;
    };

    struct int32x8
    {
        int32x4 lo, hi;
    };

    struct int64x4
    {
        int64x2 lo, hi;
    };

    struct uint8x32
    {
        uint8x16 lo, hi;
    };

    struct uint16x16
    {
        uint16x8 lo, hi;
    };

    struct uint32x8
    {
        uint32x4 lo, hi;
    };

    struct uint64x4
    {
        uint64x2 lo, hi;
    };

    struct float32x8
    {
        float32x4 lo, hi;
    };

    struct float64x4
    {
        float64x2 lo, hi;
    };

} // namespace simd
} // namespace mango

    #include "sse_int128.hpp"
    #include "common_int256.hpp"
    #include "scalar_float64.hpp"
    #include "sse_float128.hpp"
    #include "common_float256.hpp"
    #include "sse_double128.hpp"
    #include "sse_double256.hpp"
    #include "sse_convert.hpp"

#elif defined(MANGO_ENABLE_NEON)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // ARM NEON vector instrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    typedef vector_type<int8, 16, int8x16_t>    int8x16;
    typedef vector_type<int16, 8, int16x8_t>    int16x8;
    typedef vector_type<int32, 4, int32x4_t>    int32x4;
    typedef vector_type<int64, 2, int64x2_t>    int64x2;
    typedef vector_type<uint8, 16, uint8x16_t>  uint8x16;
    typedef vector_type<uint16, 8, uint16x8_t>  uint16x8;
    typedef vector_type<uint32, 4, uint32x4_t>  uint32x4;
    typedef vector_type<uint64, 2, uint64x2_t>  uint64x2;
    typedef vector_type<float, 2, float32x2_t>  float32x2;
    typedef vector_type<float, 4, float32x4_t>  float32x4;

#ifdef MANGO_ENABLE_FP16

    typedef vector_type<half, 4, float16x4_t> float16x4;

#else

    typedef scalar_type<half, 16, 4> float16x4;

#endif

    struct int8x32
    {
        int8x16 lo, hi;
    };

    struct int16x16
    {
        int16x8 lo, hi;
    };

    struct int32x8
    {
        int32x4 lo, hi;
    };

    struct int64x4
    {
        int64x2 lo, hi;
    };

    struct uint8x32
    {
        uint8x16 lo, hi;
    };

    struct uint16x16
    {
        uint16x8 lo, hi;
    };

    struct uint32x8
    {
        uint32x4 lo, hi;
    };

    struct uint64x4
    {
        uint64x2 lo, hi;
    };

    struct float32x8
    {
        float32x4 lo, hi;
    };

    typedef scalar_type<double, 64, 2> float64x2;
    typedef scalar_type<double, 64, 4> float64x4;

} // namespace simd
} // namespace mango

    #include "neon_int128.hpp"
    #include "common_int256.hpp"
    #include "neon_float64.hpp"
    #include "neon_float128.hpp"
    #include "common_float256.hpp"
    #include "scalar_double128.hpp"
    #include "scalar_double256.hpp"
    #include "neon_convert.hpp"

#elif defined(MANGO_ENABLE_ALTIVEC)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // PowerPC Altivec / AVX128
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    typedef vector_type<int8, 16, vector signed char>     int8x16;
    typedef vector_type<int16, 8, vector signed short>    int16x8;
    typedef vector_type<int32, 4, vector signed int>      int32x4;
    typedef scalar_type<int64, 64, 2>                     int64x2;
    typedef vector_type<uint8, 16, vector unsigned char>  uint8x16;
    typedef vector_type<uint16, 8, vector unsigned short> uint16x8;
    typedef vector_type<uint32, 4, vector unsigned int>   uint32x4;
    typedef scalar_type<uint64, 64, 2>                    uint64x2;
    typedef scalar_type<half, 16, 4>                      float16x4;
    typedef scalar_type<float, 32, 2>                     float32x2;
    typedef vector_type<float, 4, vector float>           float32x4;
    typedef scalar_type<double, 64, 2>                    float64x2;
    typedef scalar_type<double, 64, 4>                    float64x4;

    struct int8x32
    {
        int8x16 lo, hi;
    };

    struct int16x16
    {
        int16x8 lo, hi;
    };

    struct int32x8
    {
        int32x4 lo, hi;
    };

    struct int64x4
    {
        int64x2 lo, hi;
    };

    struct uint8x32
    {
        uint8x16 lo, hi;
    };

    struct uint16x16
    {
        uint16x8 lo, hi;
    };

    struct uint32x8
    {
        uint32x4 lo, hi;
    };

    struct uint64x4
    {
        uint64x2 lo, hi;
    };

    struct float32x8
    {
        float32x4 lo, hi;
    };

} // namespace simd
} // namespace mango

    #include "altivec_int128.hpp"
    #include "common_int256.hpp"
    #include "scalar_float64.hpp"
    #include "altivec_float128.hpp"
    #include "common_float256.hpp"
    #include "scalar_double128.hpp"
    #include "scalar_double256.hpp"
    #include "altivec_convert.hpp"

#elif defined(MANGO_ENABLE_SPU)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // Cell BE SPU
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    typedef vector_type<int8, 16, vector signed char>     int8x16;
    typedef vector_type<int16, 8, vector signed short>    int16x8;
    typedef vector_type<int32, 4, vector signed int>      int32x4;
    typedef scalar_type<int64, 64, 2>                     int64x2;
    typedef vector_type<uint8, 16, vector unsigned char>  uint8x16;
    typedef vector_type<uint16, 8, vector unsigned short> uint16x8;
    typedef vector_type<uint32, 4, vector unsigned int>   uint32x4;
    typedef scalar_type<uint64, 64, 2>                    uint64x2;
    typedef scalar_type<half, 16, 4>                      float16x4;
    typedef scalar_type<float, 32, 2>                     float32x2;
    typedef vector_type<float, 4, vector float>           float32x4;
    typedef scalar_type<double, 64, 2>                    float64x2;
    typedef scalar_type<double, 64, 4>                    float64x4;

    struct int8x32
    {
        int8x16 lo, hi;
    };

    struct int16x16
    {
        int16x8 lo, hi;
    };

    struct int32x8
    {
        int32x4 lo, hi;
    };

    struct uint8x32
    {
        uint8x16 lo, hi;
    };

    struct int64x4
    {
        int64x2 lo, hi;
    };

    struct uint16x16
    {
        uint16x8 lo, hi;
    };

    struct uint32x8
    {
        uint32x4 lo, hi;
    };

    struct uint64x4
    {
        uint64x2 lo, hi;
    };

    struct float32x8
    {
        float32x4 lo, hi;
    };

} // namespace simd
} // namespace mango

    #include "spu_int128.hpp"
    #include "common_int256.hpp"
    #include "scalar_float64.hpp"
    #include "spu_float128.hpp"
    #include "common_float256.hpp"
    #include "scalar_double128.hpp"
    #include "scalar_double256.hpp"
    #include "spu_convert.hpp"

#else

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // SIMD emulation
    // --------------------------------------------------------------

    typedef scalar_type<int8, 8, 16>    int8x16;
    typedef scalar_type<int16, 16, 8>   int16x8;
    typedef scalar_type<int32, 32, 4>   int32x4;
    typedef scalar_type<int64, 64, 2>   int64x2;
    typedef scalar_type<uint8, 8, 16>   uint8x16;
    typedef scalar_type<uint16, 16, 8>  uint16x8;
    typedef scalar_type<uint32, 32, 4>  uint32x4;
    typedef scalar_type<uint64, 64, 2>  uint64x2;
    typedef scalar_type<half, 16, 4>    float16x4;
    typedef scalar_type<float, 32, 2>   float32x2;
    typedef scalar_type<float, 32, 4>   float32x4;
    typedef scalar_type<double, 64, 2>  float64x2;
    typedef scalar_type<double, 64, 4>  float64x4;

    struct int8x32
    {
        int8x16 lo, hi;
    };

    struct int16x16
    {
        int16x8 lo, hi;
    };

    struct int32x8
    {
        int32x4 lo, hi;
    };

    struct uint8x32
    {
        uint8x16 lo, hi;
    };

    struct int64x4
    {
        int64x2 lo, hi;
    };

    struct uint16x16
    {
        uint16x8 lo, hi;
    };

    struct uint32x8
    {
        uint32x4 lo, hi;
    };

    struct uint64x4
    {
        uint64x2 lo, hi;
    };

    struct float32x8
    {
        float32x4 lo, hi;
    };

} // namespace simd
} // namespace mango

    #include "scalar_int128.hpp"
    #include "common_int256.hpp"
    #include "scalar_float64.hpp"
    #include "scalar_float128.hpp"
    #include "common_float256.hpp"
    #include "scalar_double128.hpp"
    #include "scalar_double256.hpp"
    #include "scalar_convert.hpp"

#endif
