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
    struct hardware_vector
    {
        using type = VectorType;
        VectorType data;

        hardware_vector() = default;

        hardware_vector(VectorType v)
        : data(v)
        {
        }

        operator VectorType () const
        {
            return data;
        }
    };

    // Scalar Emulated vector types not supported by hardware instructions

    template <typename ScalarType, int Size>
    struct scalar_vector
    {
        enum
        {
            bits = sizeof(ScalarType) * 8,
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

    // Vector types implemented as separate low/high parts, which typically have hardware support

    template <typename VectorType>
    struct composite_vector
    {
        VectorType lo, hi;
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

    using int8x16   = hardware_vector<int8, 16, __m128i>;
    using int16x8   = hardware_vector<int16, 8, __m128i>;
    using int32x4   = hardware_vector<int32, 4, __m128i>;
    using int64x2   = hardware_vector<int64, 2, __m128i>;
    using uint8x16  = hardware_vector<uint8, 16, __m128i>;
    using uint16x8  = hardware_vector<uint16, 8, __m128i>;
    using uint32x4  = hardware_vector<uint32, 4, __m128i>;
    using uint64x2  = hardware_vector<uint64, 2, __m128i>;
    using int8x32   = hardware_vector<int8, 32, __m256i>;
    using int16x16  = hardware_vector<int16, 16, __m256i>;
    using int32x8   = hardware_vector<int32, 8, __m256i>;
    using int64x4   = hardware_vector<int64, 4, __m256i>;
    using uint8x32  = hardware_vector<uint8, 32, __m256i>;
    using uint16x16 = hardware_vector<uint16, 16, __m256i>;
    using uint32x8  = hardware_vector<uint32, 8, __m256i>;
    using uint64x4  = hardware_vector<uint64, 4, __m256i>;
    using float32x4 = hardware_vector<float, 4, __m128>;
    using float32x8 = hardware_vector<float, 8, __m256>;
    using float64x2 = hardware_vector<double, 2, __m128d>;
    using float64x4 = hardware_vector<double, 4, __m256d>;

    using float16x4 = scalar_vector<half, 4>;
    using float32x2 = scalar_vector<float, 2>;

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

    using int8x16   = hardware_vector<int8, 16, __m128i>;
    using int16x8   = hardware_vector<int16, 8, __m128i>;
    using int32x4   = hardware_vector<int32, 4, __m128i>;
    using int64x2   = hardware_vector<int64, 2, __m128i>;
    using uint8x16  = hardware_vector<uint8, 16, __m128i>;
    using uint16x8  = hardware_vector<uint16, 8, __m128i>;
    using uint32x4  = hardware_vector<uint32, 4, __m128i>;
    using uint64x2  = hardware_vector<uint64, 2, __m128i>;
    using float32x4 = hardware_vector<float, 4, __m128>;
    using float32x8 = hardware_vector<float, 8, __m256>;
    using float64x2 = hardware_vector<double, 2, __m128d>;
    using float64x4 = hardware_vector<double, 4, __m256d>;

    using float16x4 = scalar_vector<half, 4>;
    using float32x2 = scalar_vector<float, 2>;

    using int8x32   = composite_vector<int8x16>;
    using int16x16  = composite_vector<int16x8>;
    using int32x8   = composite_vector<int32x4>;
    using int64x4   = composite_vector<int64x2>;
    using uint8x32  = composite_vector<uint8x16>;
    using uint16x16 = composite_vector<uint16x8>;
    using uint32x8  = composite_vector<uint32x4>;
    using uint64x4  = composite_vector<uint64x2>;

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

    using int8x16   = hardware_vector<int8, 16, __m128i>;
    using int16x8   = hardware_vector<int16, 8, __m128i>;
    using int32x4   = hardware_vector<int32, 4, __m128i>;
    using int64x2   = hardware_vector<int64, 2, __m128i>;
    using uint8x16  = hardware_vector<uint8, 16, __m128i>;
    using uint16x8  = hardware_vector<uint16, 8, __m128i>;
    using uint32x4  = hardware_vector<uint32, 4, __m128i>;
    using uint64x2  = hardware_vector<uint64, 2, __m128i>;
    using float32x4 = hardware_vector<float, 4, __m128>;
    using float64x2 = hardware_vector<double, 2, __m128d>;

    using float16x4 = scalar_vector<half, 4>;
    using float32x2 = scalar_vector<float, 2>;

    using int8x32   = composite_vector<int8x16>;
    using int16x16  = composite_vector<int16x8>;
    using int32x8   = composite_vector<int32x4>;
    using int64x4   = composite_vector<int64x2>;
    using uint8x32  = composite_vector<uint8x16>;
    using uint16x16 = composite_vector<uint16x8>;
    using uint32x8  = composite_vector<uint32x4>;
    using uint64x4  = composite_vector<uint64x2>;
    using float32x8 = composite_vector<float32x4>;
    using float64x4 = composite_vector<float64x2>;

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

    using int8x16   = hardware_vector<int8, 16, int8x16_t>;
    using int16x8   = hardware_vector<int16, 8, int16x8_t>;
    using int32x4   = hardware_vector<int32, 4, int32x4_t>;
    using int64x2   = hardware_vector<int64, 2, int64x2_t>;
    using uint8x16  = hardware_vector<uint8, 16, uint8x16_t>;
    using uint16x8  = hardware_vector<uint16, 8, uint16x8_t>;
    using uint32x4  = hardware_vector<uint32, 4, uint32x4_t>;
    using uint64x2  = hardware_vector<uint64, 2, uint64x2_t>;
    using float32x2 = hardware_vector<float, 2, float32x2_t>;
    using float32x4 = hardware_vector<float, 4, float32x4_t>;

    using float64x2 = scalar_vector<double, 2> ;
    using float64x4 = scalar_vector<double, 4>;

#ifdef MANGO_ENABLE_FP16

    using float16x4 = hardware_vector<half, 4, float16x4_t>;

#else

    using float16x4 = scalar_vector<half, 4>;

#endif

    using int8x32   = composite_vector<int8x16>;
    using int16x16  = composite_vector<int16x8>;
    using int32x8   = composite_vector<int32x4>;
    using int64x4   = composite_vector<int64x2>;
    using uint8x32  = composite_vector<uint8x16>;
    using uint16x16 = composite_vector<uint16x8>;
    using uint32x8  = composite_vector<uint32x4>;
    using uint64x4  = composite_vector<uint64x2>;
    using float32x8 = composite_vector<float32x4>;

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

    using int8x16   = hardware_vector<int8, 16, vector signed char>;
    using int16x8   = hardware_vector<int16, 8, vector signed short>;
    using int32x4   = hardware_vector<int32, 4, vector signed int>;
    using uint8x16  = hardware_vector<uint8, 16, vector unsigned char>;
    using uint16x8  = hardware_vector<uint16, 8, vector unsigned short>;
    using uint32x4  = hardware_vector<uint32, 4, vector unsigned int>;
    using float32x4 = hardware_vector<float, 4, vector float>;

    using int64x2   = scalar_vector<int64, 2>;
    using uint64x2  = scalar_vector<uint64, 2>;
    using float16x4 = scalar_vector<half, 4>;
    using float32x2 = scalar_vector<float, 2>;
    using float64x2 = scalar_vector<double, 2>;
    using float64x4 = scalar_vector<double, 4>;

    using int8x32   = composite_vector<int8x16>;
    using int16x16  = composite_vector<int16x8>;
    using int32x8   = composite_vector<int32x4>;
    using int64x4   = composite_vector<int64x2>;
    using uint8x32  = composite_vector<uint8x16>;
    using uint16x16 = composite_vector<uint16x8>;
    using uint32x8  = composite_vector<uint32x4>;
    using uint64x4  = composite_vector<uint64x2>;
    using float32x8 = composite_vector<float32x4>;

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

    using int8x16   = hardware_vector<int8, 16, vector signed char>;
    using int16x8   = hardware_vector<int16, 8, vector signed short>;
    using int32x4   = hardware_vector<int32, 4, vector signed int>;
    using uint8x16  = hardware_vector<uint8, 16, vector unsigned char>;
    using uint16x8  = hardware_vector<uint16, 8, vector unsigned short>;
    using uint32x4  = hardware_vector<uint32, 4, vector unsigned int>;
    using float32x4 = hardware_vector<float, 4, vector float>;

    using int64x2   = scalar_vector<int64, 2>;
    using uint64x2  = scalar_vector<uint64, 2>;
    using float16x4 = scalar_vector<half, 4>;
    using float32x2 = scalar_vector<float, 2>;
    using float64x2 = scalar_vector<double, 2>;
    using float64x4 = scalar_vector<double, 4>;

    using int8x32   = composite_vector<int8x16>;
    using int16x16  = composite_vector<int16x8>;
    using int32x8   = composite_vector<int32x4>;
    using int64x4   = composite_vector<int64x2>;
    using uint8x32  = composite_vector<uint8x16>;
    using uint16x16 = composite_vector<uint16x8>;
    using uint32x8  = composite_vector<uint32x4>;
    using uint64x4  = composite_vector<uint64x2>;
    using float32x8 = composite_vector<float32x4>;

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

    using int8x16   = scalar_vector<int8, 16>;
    using int16x8   = scalar_vector<int16, 8>;
    using int32x4   = scalar_vector<int32, 4>;
    using int64x2   = scalar_vector<int64, 2>;
    using uint8x16  = scalar_vector<uint8, 16>;
    using uint16x8  = scalar_vector<uint16, 8>;
    using uint32x4  = scalar_vector<uint32, 4>;
    using uint64x2  = scalar_vector<uint64, 2>;
    using float16x4 = scalar_vector<half, 4>;
    using float32x2 = scalar_vector<float, 2>;
    using float32x4 = scalar_vector<float, 4>;
    using float64x2 = scalar_vector<double, 2>;
    using float64x4 = scalar_vector<double, 4>;

    using int8x32   = composite_vector<int8x16>;
    using int16x16  = composite_vector<int16x8>;
    using int32x8   = composite_vector<int32x4>;
    using int64x4   = composite_vector<int64x2>;
    using uint8x32  = composite_vector<uint8x16>;
    using uint16x16 = composite_vector<uint16x8>;
    using uint32x8  = composite_vector<uint32x4>;
    using uint64x4  = composite_vector<uint64x2>;
    using float32x8 = composite_vector<float32x4>;

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
