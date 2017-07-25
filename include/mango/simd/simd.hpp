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

    template <typename ScalarType, int VectorSize, typename MaskType>
    struct hardware_mask
    {
        MaskType mask;

        hardware_mask() = default;
        hardware_mask(MaskType mask)
        : mask(mask)
        {
        }

        operator MaskType () const
        {
            return mask;
        }
    };

    template <typename ScalarType, int VectorSize, typename VectorType, typename MaskType>
    struct hardware_vector
    {
        using scalar = ScalarType;
        using vector = VectorType;
        using mask = hardware_mask<ScalarType, VectorSize, MaskType>;

        enum
        {
            scalar_bits = sizeof(ScalarType) * 8,
            vector_bits = sizeof(VectorType) * 8,
            size = VectorSize
        };

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

    template <typename ScalarType, int VectorSize>
    struct scalar_mask
    {
        uint32 mask;

        scalar_mask() = default;
        scalar_mask(uint32 mask)
        : mask(mask)
        {
        }

        operator uint32 () const
        {
            return mask;
        }
    };

    template <typename ScalarType, int VectorSize>
    struct scalar_vector
    {
        using scalar = ScalarType;
        using vector = void;
        using mask = scalar_mask<ScalarType, VectorSize>;

        enum
        {
            scalar_bits = sizeof(ScalarType) * 8,
            vector_bits = sizeof(ScalarType) * VectorSize * 8,
            size = VectorSize
        };

        ScalarType data[VectorSize];

        ScalarType & operator [] (int index)
        {
            return data[index];
        }

        const ScalarType & operator [] (int index) const
        {
            return data[index];
        }
    };

    // Vector types implemented as separate low/high parts, which typically have hardware support

    template <typename VectorType>
    struct composite_mask
    {
        typename VectorType::mask lo;
        typename VectorType::mask hi;
    };

    template <typename VectorType>
    struct composite_vector
    {
        using scalar = typename VectorType::scalar;
        using vector = void;
        using mask = composite_mask<VectorType>;

        enum
        {
            scalar_bits = sizeof(scalar) * 8,
            vector_bits = sizeof(VectorType) * 2 * 8,
            size = VectorType::size * 2
        };

        VectorType lo;
        VectorType hi;

        composite_vector() = default;
        composite_vector(VectorType lo, VectorType hi)
        : lo(lo), hi(hi)
        {
        }
    };

} // namespace simd
} // namespace mango

#if defined(MANGO_ENABLE_AVX512)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // Intel AVX512 vector intrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    using int8x16    = hardware_vector<int8, 16, __m128i, __m128i>;
    using int16x8    = hardware_vector<int16, 8, __m128i, __m128i>;
    using int32x4    = hardware_vector<int32, 4, __m128i, __m128i>;
    using int64x2    = hardware_vector<int64, 2, __m128i, __m128i>;
    using uint8x16   = hardware_vector<uint8, 16, __m128i, __m128i>;
    using uint16x8   = hardware_vector<uint16, 8, __m128i, __m128i>;
    using uint32x4   = hardware_vector<uint32, 4, __m128i, __m128i>;
    using uint64x2   = hardware_vector<uint64, 2, __m128i, __m128i>;
    using float32x4  = hardware_vector<float, 4, __m128, __m128>;
    using float64x2  = hardware_vector<double, 2, __m128d, __m128d>;

    using int8x32    = hardware_vector<int8, 32, __m256i, __m256i>;
    using int16x16   = hardware_vector<int16, 16, __m256i, __m256i>;
    using int32x8    = hardware_vector<int32, 8, __m256i, __m256i>;
    using int64x4    = hardware_vector<int64, 4, __m256i, __m256i>;
    using uint8x32   = hardware_vector<uint8, 32, __m256i, __m256i>;
    using uint16x16  = hardware_vector<uint16, 16, __m256i, __m256i>;
    using uint32x8   = hardware_vector<uint32, 8, __m256i, __m256i>;
    using uint64x4   = hardware_vector<uint64, 4, __m256i, __m256i>;
    using float32x8  = hardware_vector<float, 8, __m256, __m256>;
    using float64x4  = hardware_vector<double, 4, __m256d, __m256d>;

    using int8x64    = hardware_vector<int8, 64, __m512i, __mmask64>;
    using int16x32   = hardware_vector<int16, 32, __m512i, __mmask32>;
    using int32x16   = hardware_vector<int32, 16, __m512i, __mmask16>;
    using int64x8    = hardware_vector<int64, 8, __m512i, __mmask8>;
    using uint8x64   = hardware_vector<uint8, 64, __m512i, __mmask64>;
    using uint16x32  = hardware_vector<uint16, 32, __m512i, __mmask32>;
    using uint32x16  = hardware_vector<uint32, 16, __m512i, __mmask16>;
    using uint64x8   = hardware_vector<uint64, 8, __m512i, __mmask8>;
    using float32x16 = hardware_vector<float, 16, __m512, __mmask16>;
    using float64x8  = hardware_vector<double, 8, __m512d, __mmask8>;

    using float16x4  = scalar_vector<half, 4>;
    using float32x2  = scalar_vector<float, 2>;

} // namespace simd
} // namespace mango

#include "scalar_float64.hpp"
#include "avx512_float128.hpp"
#include "avx512_float256.hpp"
#include "avx512_float512.hpp"
#include "avx512_double128.hpp"
#include "avx512_double256.hpp"
#include "avx512_double512.hpp"
#include "avx512_int128.hpp"
#include "avx512_int256.hpp"
#include "avx512_int512.hpp"
#include "avx512_convert.hpp"
#include "sse_matrix.hpp"

#elif defined(MANGO_ENABLE_AVX2)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // Intel AVX2 vector intrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    using int8x16    = hardware_vector<int8, 16, __m128i, __m128i>;
    using int16x8    = hardware_vector<int16, 8, __m128i, __m128i>;
    using int32x4    = hardware_vector<int32, 4, __m128i, __m128i>;
    using int64x2    = hardware_vector<int64, 2, __m128i, __m128i>;
    using uint8x16   = hardware_vector<uint8, 16, __m128i, __m128i>;
    using uint16x8   = hardware_vector<uint16, 8, __m128i, __m128i>;
    using uint32x4   = hardware_vector<uint32, 4, __m128i, __m128i>;
    using uint64x2   = hardware_vector<uint64, 2, __m128i, __m128i>;
    using float32x4  = hardware_vector<float, 4, __m128, __m128>;
    using float64x2  = hardware_vector<double, 2, __m128d, __m128d>;

    using int8x32    = hardware_vector<int8, 32, __m256i, __m256i>;
    using int16x16   = hardware_vector<int16, 16, __m256i, __m256i>;
    using int32x8    = hardware_vector<int32, 8, __m256i, __m256i>;
    using int64x4    = hardware_vector<int64, 4, __m256i, __m256i>;
    using uint8x32   = hardware_vector<uint8, 32, __m256i, __m256i>;
    using uint16x16  = hardware_vector<uint16, 16, __m256i, __m256i>;
    using uint32x8   = hardware_vector<uint32, 8, __m256i, __m256i>;
    using uint64x4   = hardware_vector<uint64, 4, __m256i, __m256i>;
    using float32x8  = hardware_vector<float, 8, __m256, __m256>;
    using float64x4  = hardware_vector<double, 4, __m256d, __m256d>;

    using float16x4 = scalar_vector<half, 4>;
    using float32x2 = scalar_vector<float, 2>;

    using int8x64    = composite_vector<int8x32>;
    using int16x32   = composite_vector<int16x16>;
    using int32x16   = composite_vector<int32x8>;
    using int64x8    = composite_vector<int64x4>;
    using uint8x64   = composite_vector<uint8x32>;
    using uint16x32  = composite_vector<uint16x16>;
    using uint32x16  = composite_vector<uint32x8>;
    using uint64x8   = composite_vector<uint64x4>;
    using float32x16 = composite_vector<float32x8>;
    using float64x8  = composite_vector<float64x4>;

} // namespace simd
} // namespace mango

#include "scalar_float64.hpp"
#include "sse_int128.hpp"
#include "sse_float128.hpp"
#include "sse_double128.hpp"
#include "avx_float256.hpp"
#include "avx_double256.hpp"
#include "avx2_int256.hpp"
#include "common_int512.hpp"
#include "common_float512.hpp"
#include "common_double512.hpp"
#include "avx_convert.hpp"
#include "sse_matrix.hpp"

#elif defined(MANGO_ENABLE_AVX)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // Intel AVX vector intrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    using int8x16    = hardware_vector<int8, 16, __m128i, __m128i>;
    using int16x8    = hardware_vector<int16, 8, __m128i, __m128i>;
    using int32x4    = hardware_vector<int32, 4, __m128i, __m128i>;
    using int64x2    = hardware_vector<int64, 2, __m128i, __m128i>;
    using uint8x16   = hardware_vector<uint8, 16, __m128i, __m128i>;
    using uint16x8   = hardware_vector<uint16, 8, __m128i, __m128i>;
    using uint32x4   = hardware_vector<uint32, 4, __m128i, __m128i>;
    using uint64x2   = hardware_vector<uint64, 2, __m128i, __m128i>;
    using float32x4  = hardware_vector<float, 4, __m128, __m128>;
    using float64x2  = hardware_vector<double, 2, __m128d, __m128d>;

    using float32x8  = hardware_vector<float, 8, __m256, __m256>;
    using float64x4  = hardware_vector<double, 4, __m256d, __m256d>;

    using float16x4  = scalar_vector<half, 4>;
    using float32x2  = scalar_vector<float, 2>;

    using int8x32    = composite_vector<int8x16>;
    using int16x16   = composite_vector<int16x8>;
    using int32x8    = composite_vector<int32x4>;
    using int64x4    = composite_vector<int64x2>;
    using uint8x32   = composite_vector<uint8x16>;
    using uint16x16  = composite_vector<uint16x8>;
    using uint32x8   = composite_vector<uint32x4>;
    using uint64x4   = composite_vector<uint64x2>;

    using int8x64    = composite_vector<int8x32>;
    using int16x32   = composite_vector<int16x16>;
    using int32x16   = composite_vector<int32x8>;
    using int64x8    = composite_vector<int64x4>;
    using uint8x64   = composite_vector<uint8x32>;
    using uint16x32  = composite_vector<uint16x16>;
    using uint32x16  = composite_vector<uint32x8>;
    using uint64x8   = composite_vector<uint64x4>;
    using float32x16 = composite_vector<float32x8>;
    using float64x8  = composite_vector<float64x4>;

} // namespace simd
} // namespace mango

#include "scalar_float64.hpp"
#include "sse_int128.hpp"
#include "sse_float128.hpp"
#include "sse_double128.hpp"
#include "common_int256.hpp"
#include "avx_float256.hpp"
#include "avx_double256.hpp"
#include "common_int512.hpp"
#include "common_float512.hpp"
#include "common_double512.hpp"
#include "avx_convert.hpp"
#include "sse_matrix.hpp"

#elif defined(MANGO_ENABLE_SSE2)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // Intel SSE vector intrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    using int8x16    = hardware_vector<int8, 16, __m128i, __m128i>;
    using int16x8    = hardware_vector<int16, 8, __m128i, __m128i>;
    using int32x4    = hardware_vector<int32, 4, __m128i, __m128i>;
    using int64x2    = hardware_vector<int64, 2, __m128i, __m128i>;
    using uint8x16   = hardware_vector<uint8, 16, __m128i, __m128i>;
    using uint16x8   = hardware_vector<uint16, 8, __m128i, __m128i>;
    using uint32x4   = hardware_vector<uint32, 4, __m128i, __m128i>;
    using uint64x2   = hardware_vector<uint64, 2, __m128i, __m128i>;
    using float32x4  = hardware_vector<float, 4, __m128, __m128>;
    using float64x2  = hardware_vector<double, 2, __m128d, __m128d>;

    using float16x4  = scalar_vector<half, 4>;
    using float32x2  = scalar_vector<float, 2>;

    using int8x32    = composite_vector<int8x16>;
    using int16x16   = composite_vector<int16x8>;
    using int32x8    = composite_vector<int32x4>;
    using int64x4    = composite_vector<int64x2>;
    using uint8x32   = composite_vector<uint8x16>;
    using uint16x16  = composite_vector<uint16x8>;
    using uint32x8   = composite_vector<uint32x4>;
    using uint64x4   = composite_vector<uint64x2>;
    using float32x8  = composite_vector<float32x4>;
    using float64x4  = composite_vector<float64x2>;

    using int8x64    = composite_vector<int8x32>;
    using int16x32   = composite_vector<int16x16>;
    using int32x16   = composite_vector<int32x8>;
    using int64x8    = composite_vector<int64x4>;
    using uint8x64   = composite_vector<uint8x32>;
    using uint16x32  = composite_vector<uint16x16>;
    using uint32x16  = composite_vector<uint32x8>;
    using uint64x8   = composite_vector<uint64x4>;
    using float32x16 = composite_vector<float32x8>;
    using float64x8  = composite_vector<float64x4>;

} // namespace simd
} // namespace mango

#include "scalar_float64.hpp"
#include "sse_int128.hpp"
#include "sse_float128.hpp"
#include "sse_double128.hpp"
#include "common_int256.hpp"
#include "common_float256.hpp"
#include "common_double256.hpp"
#include "common_int512.hpp"
#include "common_float512.hpp"
#include "common_double512.hpp"
#include "sse_convert.hpp"
#include "sse_matrix.hpp"

#elif defined(MANGO_ENABLE_NEON)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // ARM NEON vector instrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    using int8x16    = hardware_vector<int8, 16, int8x16_t, uint8x16_t>;
    using int16x8    = hardware_vector<int16, 8, int16x8_t, uint16x8_t>;
    using int32x4    = hardware_vector<int32, 4, int32x4_t, uint32x4_t>;
    using int64x2    = hardware_vector<int64, 2, int64x2_t, uint64x2_t>;
    using uint8x16   = hardware_vector<uint8, 16, uint8x16_t, uint8x16_t>;
    using uint16x8   = hardware_vector<uint16, 8, uint16x8_t, uint16x8_t>;
    using uint32x4   = hardware_vector<uint32, 4, uint32x4_t, uint32x4_t>;
    using uint64x2   = hardware_vector<uint64, 2, uint64x2_t, uint64x2_t>;
    using float32x2  = hardware_vector<float, 2, float32x2_t, uint32x2_t>;
    using float32x4  = hardware_vector<float, 4, float32x4_t, uint32x4_t>;

#ifdef MANGO_ENABLE_FP16
    using float16x4  = hardware_vector<half, 4, float16x4_t, void>;
#else
    using float16x4  = scalar_vector<half, 4>;
#endif

    using float64x2  = scalar_vector<double, 2>;
    using float64x4  = scalar_vector<double, 4>;

    using int8x32    = composite_vector<int8x16>;
    using int16x16   = composite_vector<int16x8>;
    using int32x8    = composite_vector<int32x4>;
    using int64x4    = composite_vector<int64x2>;
    using uint8x32   = composite_vector<uint8x16>;
    using uint16x16  = composite_vector<uint16x8>;
    using uint32x8   = composite_vector<uint32x4>;
    using uint64x4   = composite_vector<uint64x2>;
    using float32x8  = composite_vector<float32x4>;

    using int8x64    = composite_vector<int8x32>;
    using int16x32   = composite_vector<int16x16>;
    using int32x16   = composite_vector<int32x8>;
    using int64x8    = composite_vector<int64x4>;
    using uint8x64   = composite_vector<uint8x32>;
    using uint16x32  = composite_vector<uint16x16>;
    using uint32x16  = composite_vector<uint32x8>;
    using uint64x8   = composite_vector<uint64x4>;
    using float32x16 = composite_vector<float32x8>;
    using float64x8  = composite_vector<float64x4>;

} // namespace simd
} // namespace mango

#include "neon_float64.hpp"
#include "neon_int128.hpp"
#include "neon_float128.hpp"
#include "scalar_double128.hpp"
#include "scalar_double256.hpp"
#include "common_int256.hpp"
#include "common_float256.hpp"
#include "common_int512.hpp"
#include "common_float512.hpp"
#include "common_double512.hpp"
#include "neon_convert.hpp"
#include "neon_matrix.hpp"

#elif defined(MANGO_ENABLE_ALTIVEC)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // PowerPC Altivec / AVX128
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    using int8x16    = hardware_vector<int8, 16, vector signed char, vector signed char>;
    using int16x8    = hardware_vector<int16, 8, vector signed short, vector signed short>;
    using int32x4    = hardware_vector<int32, 4, vector signed int, vector signed int>;
    using uint8x16   = hardware_vector<uint8, 16, vector unsigned char, vector unsigned char>;
    using uint16x8   = hardware_vector<uint16, 8, vector unsigned short, vector unsigned short>;
    using uint32x4   = hardware_vector<uint32, 4, vector unsigned int, vector unsigned int>;
    using float32x4  = hardware_vector<float, 4, vector float, vector float>;

    using int64x2    = scalar_vector<int64, 2>;
    using uint64x2   = scalar_vector<uint64, 2>;
    using float16x4  = scalar_vector<half, 4>;
    using float32x2  = scalar_vector<float, 2>;
    using float64x2  = scalar_vector<double, 2>;
    using float64x4  = scalar_vector<double, 4>;

    using int8x32    = composite_vector<int8x16>;
    using int16x16   = composite_vector<int16x8>;
    using int32x8    = composite_vector<int32x4>;
    using int64x4    = composite_vector<int64x2>;
    using uint8x32   = composite_vector<uint8x16>;
    using uint16x16  = composite_vector<uint16x8>;
    using uint32x8   = composite_vector<uint32x4>;
    using uint64x4   = composite_vector<uint64x2>;
    using float32x8  = composite_vector<float32x4>;

    using int8x64    = composite_vector<int8x32>;
    using int16x32   = composite_vector<int16x16>;
    using int32x16   = composite_vector<int32x8>;
    using int64x8    = composite_vector<int64x4>;
    using uint8x64   = composite_vector<uint8x32>;
    using uint16x32  = composite_vector<uint16x16>;
    using uint32x16  = composite_vector<uint32x8>;
    using uint64x8   = composite_vector<uint64x4>;
    using float32x16 = composite_vector<float32x8>;
    using float64x8  = composite_vector<float64x4>;

} // namespace simd
} // namespace mango

#include "scalar_float64.hpp"
#include "altivec_int128.hpp"
#include "altivec_float128.hpp"
#include "common_int256.hpp"
#include "common_float256.hpp"
#include "scalar_double128.hpp"
#include "scalar_double256.hpp"
#include "common_int512.hpp"
#include "common_float512.hpp"
#include "common_double512.hpp"
#include "altivec_convert.hpp"
#include "altivec_matrix.hpp"

#elif defined(MANGO_ENABLE_SPU)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // Cell BE SPU
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    using int8x16    = hardware_vector<int8, 16, vector signed char, vector signed char>;
    using int16x8    = hardware_vector<int16, 8, vector signed short, vector signed short>;
    using int32x4    = hardware_vector<int32, 4, vector signed int, vector signed int>;
    using uint8x16   = hardware_vector<uint8, 16, vector unsigned char, vector unsigned char>;
    using uint16x8   = hardware_vector<uint16, 8, vector unsigned short, vector unsigned short>;
    using uint32x4   = hardware_vector<uint32, 4, vector unsigned int, vector unsigned int>;
    using float32x4  = hardware_vector<float, 4, vector float, vector float>;

    using int64x2    = scalar_vector<int64, 2>;
    using uint64x2   = scalar_vector<uint64, 2>;
    using float16x4  = scalar_vector<half, 4>;
    using float32x2  = scalar_vector<float, 2>;
    using float64x2  = scalar_vector<double, 2>;
    using float64x4  = scalar_vector<double, 4>;

    using int8x32    = composite_vector<int8x16>;
    using int16x16   = composite_vector<int16x8>;
    using int32x8    = composite_vector<int32x4>;
    using int64x4    = composite_vector<int64x2>;
    using uint8x32   = composite_vector<uint8x16>;
    using uint16x16  = composite_vector<uint16x8>;
    using uint32x8   = composite_vector<uint32x4>;
    using uint64x4   = composite_vector<uint64x2>;
    using float32x8  = composite_vector<float32x4>;

    using int8x64    = composite_vector<int8x32>;
    using int16x32   = composite_vector<int16x16>;
    using int32x16   = composite_vector<int32x8>;
    using int64x8    = composite_vector<int64x4>;
    using uint8x64   = composite_vector<uint8x32>;
    using uint16x32  = composite_vector<uint16x16>;
    using uint32x16  = composite_vector<uint32x8>;
    using uint64x8   = composite_vector<uint64x4>;
    using float32x16 = composite_vector<float32x8>;
    using float64x8  = composite_vector<float64x4>;

} // namespace simd
} // namespace mango

#include "scalar_float64.hpp"
#include "spu_int128.hpp"
#include "spu_float128.hpp"
#include "common_int256.hpp"
#include "common_float256.hpp"
#include "scalar_double128.hpp"
#include "scalar_double256.hpp"
#include "common_int512.hpp"
#include "common_float512.hpp"
#include "common_double512.hpp"
#include "spu_convert.hpp"
#include "spu_matrix.hpp"

/* TODO:
#elif defined(MANGO_ENABLE_MSA)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // MIPS SIMD Architecture
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD

    using int8x16    = hardware_vector<int8, 16, v16i8, v16i8>;
    using int16x8    = hardware_vector<int16, 8, v8i16, v8i16>;
    using int32x4    = hardware_vector<int32, 4, v4i32, v4i32>;
    using int64x2    = hardware_vector<int64, 2, v2i64, v2i64>;
    using uint8x16   = hardware_vector<uint8, 16, v16u8, v16u8>;
    using uint16x8   = hardware_vector<uint16, 8, v8u16, v8u16>;
    using uint32x4   = hardware_vector<uint32, 4, v4u32, v4u32>;
    using uint64x2   = hardware_vector<uint64, 2, v2u64, v2u64>;
    using float32x4  = hardware_vector<float, 4, v4f32, v4f32>;
    using float64x2  = hardware_vector<double, 2, v2f64, v2f64>;

    using float16x4  = scalar_vector<half, 4>;
    using float32x2  = scalar_vector<float, 2>;

    using int8x32    = composite_vector<int8x16>;
    using int16x16   = composite_vector<int16x8>;
    using int32x8    = composite_vector<int32x4>;
    using int64x4    = composite_vector<int64x2>;
    using uint8x32   = composite_vector<uint8x16>;
    using uint16x16  = composite_vector<uint16x8>;
    using uint32x8   = composite_vector<uint32x4>;
    using uint64x4   = composite_vector<uint64x2>;
    using float32x8  = composite_vector<float32x4>;
    using float64x4  = composite_vector<float64x2>;

    using int8x64    = composite_vector<int8x32>;
    using int16x32   = composite_vector<int16x16>;
    using int32x16   = composite_vector<int32x8>;
    using int64x8    = composite_vector<int64x4>;
    using uint8x64   = composite_vector<uint8x32>;
    using uint16x32  = composite_vector<uint16x16>;
    using uint32x16  = composite_vector<uint32x8>;
    using uint64x8   = composite_vector<uint64x4>;
    using float32x16 = composite_vector<float32x8>;
    using float64x8  = composite_vector<float64x4>;

} // namespace simd
} // namespace mango

#include "scalar_float64.hpp"
#include "msa_int128.hpp"
#include "msa_float128.hpp"
#include "msa_double128.hpp"
#include "common_int256.hpp"
#include "common_float256.hpp"
#include "common_double256.hpp"
#include "common_int512.hpp"
#include "common_float512.hpp"
#include "common_double512.hpp"
#include "msa_convert.hpp"
#include "msa_matrix.hpp"
*/
#else

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // SIMD emulation
    // --------------------------------------------------------------

    using int8x16    = scalar_vector<int8, 16>;
    using int16x8    = scalar_vector<int16, 8>;
    using int32x4    = scalar_vector<int32, 4>;
    using int64x2    = scalar_vector<int64, 2>;
    using uint8x16   = scalar_vector<uint8, 16>;
    using uint16x8   = scalar_vector<uint16, 8>;
    using uint32x4   = scalar_vector<uint32, 4>;
    using uint64x2   = scalar_vector<uint64, 2>;
    using float16x4  = scalar_vector<half, 4>;
    using float32x2  = scalar_vector<float, 2>;
    using float32x4  = scalar_vector<float, 4>;
    using float64x2  = scalar_vector<double, 2>;
    using float64x4  = scalar_vector<double, 4>;

    using int8x32    = composite_vector<int8x16>;
    using int16x16   = composite_vector<int16x8>;
    using int32x8    = composite_vector<int32x4>;
    using int64x4    = composite_vector<int64x2>;
    using uint8x32   = composite_vector<uint8x16>;
    using uint16x16  = composite_vector<uint16x8>;
    using uint32x8   = composite_vector<uint32x4>;
    using uint64x4   = composite_vector<uint64x2>;
    using float32x8  = composite_vector<float32x4>;

    using int8x64    = composite_vector<int8x32>;
    using int16x32   = composite_vector<int16x16>;
    using int32x16   = composite_vector<int32x8>;
    using int64x8    = composite_vector<int64x4>;
    using uint8x64   = composite_vector<uint8x32>;
    using uint16x32  = composite_vector<uint16x16>;
    using uint32x16  = composite_vector<uint32x8>;
    using uint64x8   = composite_vector<uint64x4>;
    using float32x16 = composite_vector<float32x8>;
    using float64x8  = composite_vector<float64x4>;

} // namespace simd
} // namespace mango

#include "scalar_float64.hpp"
#include "scalar_int128.hpp"
#include "scalar_float128.hpp"
#include "scalar_double128.hpp"
#include "common_int256.hpp"
#include "common_float256.hpp"
#include "scalar_double256.hpp"
#include "common_int512.hpp"
#include "common_float512.hpp"
#include "common_double512.hpp"
#include "scalar_convert.hpp"
#include "scalar_matrix.hpp"

#endif

#include "gather.hpp"
