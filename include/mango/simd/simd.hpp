/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
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

    template <int ScalarBits, int VectorSize, typename MaskType>
    struct hardware_mask
    {
        using vector = MaskType;

        MaskType data;

        hardware_mask() = default;
        hardware_mask(MaskType mask)
            : data(mask)
        {
        }

        operator MaskType () const
        {
            return data;
        }
    };

    template <typename ScalarType, int VectorSize, typename VectorType>
    struct hardware_vector
    {
        using scalar = ScalarType;
        using vector = VectorType;

        enum
        {
            scalar_bits = sizeof(ScalarType) * 8,
            vector_bits = sizeof(VectorType) * 8,
            size = VectorSize,
            is_hardware = 1,
            is_composite = 0
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

    template <int ScalarBits, int VectorSize>
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

        enum
        {
            scalar_bits = sizeof(ScalarType) * 8,
            vector_bits = sizeof(ScalarType) * VectorSize * 8,
            size = VectorSize,
            is_hardware = 0,
            is_composite = 0
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
        VectorType lo;
        VectorType hi;
    };

    template <typename VectorType>
    struct composite_vector
    {
        using scalar = typename VectorType::scalar;
        using vector = void;

        enum
        {
            scalar_bits = sizeof(scalar) * 8,
            vector_bits = sizeof(VectorType) * 2 * 8,
            size = VectorType::size * 2,
            is_hardware = VectorType::is_hardware,
            is_composite = 1
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
    #define MANGO_ENABLE_SIMD128
    #define MANGO_ENABLE_SIMD256
    #define MANGO_ENABLE_SIMD512

    // 64 bit vector
    using float16x4  = scalar_vector<half, 4>;

    // 128 bit vector
    using int8x16    = hardware_vector<int8, 16, __m128i>;
    using int16x8    = hardware_vector<int16, 8, __m128i>;
    using int32x4    = hardware_vector<int32, 4, __m128i>;
    using int64x2    = hardware_vector<int64, 2, __m128i>;
    using uint8x16   = hardware_vector<uint8, 16, __m128i>;
    using uint16x8   = hardware_vector<uint16, 8, __m128i>;
    using uint32x4   = hardware_vector<uint32, 4, __m128i>;
    using uint64x2   = hardware_vector<uint64, 2, __m128i>;
    using float32x4  = hardware_vector<float, 4, __m128>;
    using float64x2  = hardware_vector<double, 2, __m128d>;

    // 256 bit vector
    using int8x32    = hardware_vector<int8, 32, __m256i>;
    using int16x16   = hardware_vector<int16, 16, __m256i>;
    using int32x8    = hardware_vector<int32, 8, __m256i>;
    using int64x4    = hardware_vector<int64, 4, __m256i>;
    using uint8x32   = hardware_vector<uint8, 32, __m256i>;
    using uint16x16  = hardware_vector<uint16, 16, __m256i>;
    using uint32x8   = hardware_vector<uint32, 8, __m256i>;
    using uint64x4   = hardware_vector<uint64, 4, __m256i>;
    using float32x8  = hardware_vector<float, 8, __m256>;
    using float64x4  = hardware_vector<double, 4, __m256d>;

    // 512 bit vector
    using int8x64    = hardware_vector<int8, 64, __m512i>;
    using int16x32   = hardware_vector<int16, 32, __m512i>;
    using int32x16   = hardware_vector<int32, 16, __m512i>;
    using int64x8    = hardware_vector<int64, 8, __m512i>;
    using uint8x64   = hardware_vector<uint8, 64, __m512i>;
    using uint16x32  = hardware_vector<uint16, 32, __m512i>;
    using uint32x16  = hardware_vector<uint32, 16, __m512i>;
    using uint64x8   = hardware_vector<uint64, 8, __m512i>;
    using float32x16 = hardware_vector<float, 16, __m512>;
    using float64x8  = hardware_vector<double, 8, __m512d>;

    // 128 bit vector mask
    using mask8x16   = hardware_mask<8, 16, __mmask16>;
    using mask16x8   = hardware_mask<16, 8, __mmask8>;
    using mask32x4   = hardware_mask<32, 4, __mmask8>;
    using mask64x2   = hardware_mask<64, 2, __mmask8>;

    // 256 bit vector mask
    using mask8x32   = hardware_mask<8, 32, __mmask32>;
    using mask16x16  = hardware_mask<16, 16, __mmask16>;
    using mask32x8   = hardware_mask<32, 8, __mmask8>;
    using mask64x4   = hardware_mask<64, 4, __mmask8>;

    // 512 bit vector mask
    using mask8x64   = hardware_mask<8, 64, __mmask64>;
    using mask16x32  = hardware_mask<16, 32, __mmask32>;
    using mask32x16  = hardware_mask<32, 16, __mmask16>;
    using mask64x8   = hardware_mask<64, 8, __mmask8>;

} // namespace simd
} // namespace mango

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
#include "avx512_gather.hpp"

#elif defined(MANGO_ENABLE_AVX2)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // Intel AVX2 vector intrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD
    #define MANGO_ENABLE_SIMD128
    #define MANGO_ENABLE_SIMD256

    // 64 bit vector
    using float16x4  = scalar_vector<half, 4>;

    // 128 bit vector
    using int8x16    = hardware_vector<int8, 16, __m128i>;
    using int16x8    = hardware_vector<int16, 8, __m128i>;
    using int32x4    = hardware_vector<int32, 4, __m128i>;
    using int64x2    = hardware_vector<int64, 2, __m128i>;
    using uint8x16   = hardware_vector<uint8, 16, __m128i>;
    using uint16x8   = hardware_vector<uint16, 8, __m128i>;
    using uint32x4   = hardware_vector<uint32, 4, __m128i>;
    using uint64x2   = hardware_vector<uint64, 2, __m128i>;
    using float32x4  = hardware_vector<float, 4, __m128>;
    using float64x2  = hardware_vector<double, 2, __m128d>;

    // 256 bit vector
    using int8x32    = hardware_vector<int8, 32, __m256i>;
    using int16x16   = hardware_vector<int16, 16, __m256i>;
    using int32x8    = hardware_vector<int32, 8, __m256i>;
    using int64x4    = hardware_vector<int64, 4, __m256i>;
    using uint8x32   = hardware_vector<uint8, 32, __m256i>;
    using uint16x16  = hardware_vector<uint16, 16, __m256i>;
    using uint32x8   = hardware_vector<uint32, 8, __m256i>;
    using uint64x4   = hardware_vector<uint64, 4, __m256i>;
    using float32x8  = hardware_vector<float, 8, __m256>;
    using float64x4  = hardware_vector<double, 4, __m256d>;

    // 512 bit vector
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

    // 128 bit vector mask
    using mask8x16   = hardware_mask<8, 16, __m128i>;
    using mask16x8   = hardware_mask<16, 8, __m128i>;
    using mask32x4   = hardware_mask<32, 4, __m128i>;
    using mask64x2   = hardware_mask<64, 2, __m128i>;

    // 256 bit vector mask
    using mask8x32   = hardware_mask<8, 32, __m256i>;
    using mask16x16  = hardware_mask<16, 16, __m256i>;
    using mask32x8   = hardware_mask<32, 8, __m256i>;
    using mask64x4   = hardware_mask<64, 4, __m256i>;

    // 512 bit vector mask
    using mask8x64   = composite_mask<mask8x32>;
    using mask16x32  = composite_mask<mask16x16>;
    using mask32x16  = composite_mask<mask32x8>;
    using mask64x8   = composite_mask<mask64x4>;

} // namespace simd
} // namespace mango

#include "sse2_int128.hpp"
#include "sse2_float128.hpp"
#include "sse2_double128.hpp"
#include "avx2_int256.hpp"
#include "avx_float256.hpp"
#include "avx_double256.hpp"
#include "composite_int512.hpp"
#include "composite_float512.hpp"
#include "composite_double512.hpp"
#include "avx_convert.hpp"
#include "avx2_gather.hpp"

#elif defined(MANGO_ENABLE_AVX)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // Intel AVX vector intrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD
    #define MANGO_ENABLE_SIMD128
    #define MANGO_ENABLE_SIMD256
    
    // 64 bit vector
    using float16x4  = scalar_vector<half, 4>;

    // 128 bit vector
    using int8x16    = hardware_vector<int8, 16, __m128i>;
    using int16x8    = hardware_vector<int16, 8, __m128i>;
    using int32x4    = hardware_vector<int32, 4, __m128i>;
    using int64x2    = hardware_vector<int64, 2, __m128i>;
    using uint8x16   = hardware_vector<uint8, 16, __m128i>;
    using uint16x8   = hardware_vector<uint16, 8, __m128i>;
    using uint32x4   = hardware_vector<uint32, 4, __m128i>;
    using uint64x2   = hardware_vector<uint64, 2, __m128i>;
    using float32x4  = hardware_vector<float, 4, __m128>;
    using float64x2  = hardware_vector<double, 2, __m128d>;

    // 256 bit vector
    using int8x32    = composite_vector<int8x16>;
    using int16x16   = composite_vector<int16x8>;
    using int32x8    = composite_vector<int32x4>;
    using int64x4    = composite_vector<int64x2>;
    using uint8x32   = composite_vector<uint8x16>;
    using uint16x16  = composite_vector<uint16x8>;
    using uint32x8   = composite_vector<uint32x4>;
    using uint64x4   = composite_vector<uint64x2>;
    using float32x8  = hardware_vector<float, 8, __m256>;
    using float64x4  = hardware_vector<double, 4, __m256d>;

    // 512 bit vector
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

    // 128 bit vector mask
    using mask8x16   = hardware_mask<8, 16, __m128i>;
    using mask16x8   = hardware_mask<16, 8, __m128i>;
    using mask32x4   = hardware_mask<32, 4, __m128i>;
    using mask64x2   = hardware_mask<64, 2, __m128i>;

    // 256 bit vector mask
    using mask8x32   = hardware_mask<8, 32, __m256i>;
    using mask16x16  = hardware_mask<16, 16, __m256i>;
    using mask32x8   = hardware_mask<32, 8, __m256i>;
    using mask64x4   = hardware_mask<64, 4, __m256i>;

    // 512 bit vector mask
    using mask8x64   = composite_mask<mask8x32>;
    using mask16x32  = composite_mask<mask16x16>;
    using mask32x16  = composite_mask<mask32x8>;
    using mask64x8   = composite_mask<mask64x4>;

} // namespace simd
} // namespace mango

#include "sse2_int128.hpp"
#include "sse2_float128.hpp"
#include "sse2_double128.hpp"
#include "avx_int256.hpp"
#include "avx_float256.hpp"
#include "avx_double256.hpp"
#include "composite_int512.hpp"
#include "composite_float512.hpp"
#include "composite_double512.hpp"
#include "avx_convert.hpp"
#include "common_gather.hpp"

#elif defined(MANGO_ENABLE_SSE2)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // Intel SSE2 vector intrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD
    #define MANGO_ENABLE_SIMD128
    
    // 64 bit vector
    using float16x4  = scalar_vector<half, 4>;

    // 128 bit vector
    using int8x16    = hardware_vector<int8, 16, __m128i>;
    using int16x8    = hardware_vector<int16, 8, __m128i>;
    using int32x4    = hardware_vector<int32, 4, __m128i>;
    using int64x2    = hardware_vector<int64, 2, __m128i>;
    using uint8x16   = hardware_vector<uint8, 16, __m128i>;
    using uint16x8   = hardware_vector<uint16, 8, __m128i>;
    using uint32x4   = hardware_vector<uint32, 4, __m128i>;
    using uint64x2   = hardware_vector<uint64, 2, __m128i>;
    using float32x4  = hardware_vector<float, 4, __m128>;
    using float64x2  = hardware_vector<double, 2, __m128d>;

    // 256 bit vector
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

    // 512 bit vector
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

    // 128 bit vector mask
    using mask8x16   = hardware_mask<8, 16, __m128i>;
    using mask16x8   = hardware_mask<16, 8, __m128i>;
    using mask32x4   = hardware_mask<32, 4, __m128i>;
    using mask64x2   = hardware_mask<64, 2, __m128i>;

    // 256 bit vector mask
    using mask8x32   = composite_mask<mask8x16>;
    using mask16x16  = composite_mask<mask16x8>;
    using mask32x8   = composite_mask<mask32x4>;
    using mask64x4   = composite_mask<mask64x2>;

    // 512 bit vector mask
    using mask8x64   = composite_mask<mask8x32>;
    using mask16x32  = composite_mask<mask16x16>;
    using mask32x16  = composite_mask<mask32x8>;
    using mask64x8   = composite_mask<mask64x4>;

} // namespace simd
} // namespace mango

#include "sse2_int128.hpp"
#include "sse2_float128.hpp"
#include "sse2_double128.hpp"
#include "composite_int256.hpp"
#include "composite_float256.hpp"
#include "composite_double256.hpp"
#include "composite_int512.hpp"
#include "composite_float512.hpp"
#include "composite_double512.hpp"
#include "sse2_convert.hpp"
#include "common_gather.hpp"

#elif defined(MANGO_ENABLE_NEON)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // ARM NEON vector instrinsics
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD
    #define MANGO_ENABLE_SIMD128

    struct float64x2_t
    {
        double data[2];
        float64x2_t() = default;
        float64x2_t(double x, double y) { data[0] = x; data[1] = y; }
        double & operator [] (int index) { return data[index]; }
        const double & operator [] (int index) const { return data[index]; }
    };

    // 64 bit vector
#ifdef MANGO_ENABLE_FP16
    using float16x4  = hardware_vector<half, 4, float16x4_t>;
#else
    using float16x4  = scalar_vector<half, 4>;
#endif

    // 128 bit vector
    using int8x16    = hardware_vector<int8, 16, int8x16_t>;
    using int16x8    = hardware_vector<int16, 8, int16x8_t>;
    using int32x4    = hardware_vector<int32, 4, int32x4_t>;
    using int64x2    = hardware_vector<int64, 2, int64x2_t>;
    using uint8x16   = hardware_vector<uint8, 16, uint8x16_t>;
    using uint16x8   = hardware_vector<uint16, 8, uint16x8_t>;
    using uint32x4   = hardware_vector<uint32, 4, uint32x4_t>;
    using uint64x2   = hardware_vector<uint64, 2, uint64x2_t>;
    using float32x4  = hardware_vector<float, 4, float32x4_t>;
    using float64x2  = hardware_vector<double, 2, float64x2_t>;

    // 256 bit vector
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

    // 512 bit vector
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

    // 128 bit vector mask
    using mask8x16   = hardware_mask<8, 16, uint8x16_t>;
    using mask16x8   = hardware_mask<16, 8, uint16x8_t>;
    using mask32x4   = hardware_mask<32, 4, uint32x4_t>;
    using mask64x2   = hardware_mask<64, 2, uint64x2_t>;

    // 256 bit vector mask
    using mask8x32   = composite_mask<mask8x16>;
    using mask16x16  = composite_mask<mask16x8>;
    using mask32x8   = composite_mask<mask32x4>;
    using mask64x4   = composite_mask<mask64x2>;

    // 512 bit vector mask
    using mask8x64   = composite_mask<mask8x32>;
    using mask16x32  = composite_mask<mask16x16>;
    using mask32x16  = composite_mask<mask32x8>;
    using mask64x8   = composite_mask<mask64x4>;

} // namespace simd
} // namespace mango

#include "neon_int128.hpp"
#include "neon_float128.hpp"
#include "neon_double128.hpp"
#include "composite_double256.hpp"
#include "composite_int256.hpp"
#include "composite_float256.hpp"
#include "composite_int512.hpp"
#include "composite_float512.hpp"
#include "composite_double512.hpp"
#include "neon_convert.hpp"
#include "common_gather.hpp"

#elif defined(MANGO_ENABLE_ALTIVEC) && defined(MANGO_ENABLE_VSX)

#include <altivec.h>

namespace mango {
namespace simd {
    
    // --------------------------------------------------------------
    // Altivec / VSX
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD
    #define MANGO_ENABLE_SIMD128
    
    // 64 bit vector
    using float16x4  = scalar_vector<half, 4>;

    // 128 bit vector
    using int8x16    = hardware_vector<int8, 16, __vector signed char>;
    using int16x8    = hardware_vector<int16, 8, __vector signed short>;
    using int32x4    = hardware_vector<int32, 4, __vector signed int>;
    using int64x2    = hardware_vector<int64, 2, __vector signed long long>;
    using uint8x16   = hardware_vector<uint8, 16, __vector unsigned char>;
    using uint16x8   = hardware_vector<uint16, 8, __vector unsigned short>;
    using uint32x4   = hardware_vector<uint32, 4, __vector unsigned int>;
    using uint64x2   = hardware_vector<uint64, 2, __vector unsigned long long>;
    using float32x4  = hardware_vector<float, 4, __vector float>;
    using float64x2  = hardware_vector<double, 2, __vector double>;

    // 256 bit vector
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

    // 512 bit vector
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

    // 128 bit vector mask
    using mask8x16   = hardware_mask<8, 16, __vector bool char>;
    using mask16x8   = hardware_mask<16, 8, __vector bool short>;
    using mask32x4   = hardware_mask<32, 4, __vector bool int>;
    using mask64x2   = hardware_mask<64, 2, __vector bool long long>;

    // 256 bit vector mask
    using mask8x32   = composite_mask<mask8x16>;
    using mask16x16  = composite_mask<mask16x8>;
    using mask32x8   = composite_mask<mask32x4>;
    using mask64x4   = composite_mask<mask64x2>;

    // 512 bit vector mask
    using mask8x64   = composite_mask<mask8x32>;
    using mask16x32  = composite_mask<mask16x16>;
    using mask32x16  = composite_mask<mask32x8>;
    using mask64x8   = composite_mask<mask64x4>;

} // namespace simd
} // namespace mango

// <altivec.h> defined these; clean up
#undef bool
#undef vector
#undef pixel
#undef __bool
#undef __vector
#undef __pixel

#include "altivec_int128.hpp"
#include "altivec_float128.hpp"
#include "altivec_double128.hpp"
#include "composite_int256.hpp"
#include "composite_float256.hpp"
#include "composite_double256.hpp"
#include "composite_int512.hpp"
#include "composite_float512.hpp"
#include "composite_double512.hpp"
#include "altivec_convert.hpp"
#include "common_gather.hpp"

#elif defined(MANGO_ENABLE_MSA)

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // MIPS MSA
    // --------------------------------------------------------------

    #define MANGO_ENABLE_SIMD
    #define MANGO_ENABLE_SIMD128
    
    // 64 bit vector
    using float16x4  = scalar_vector<half, 4>;

    // 128 bit vector
    using int8x16    = hardware_vector<int8, 16, v16i8>;
    using int16x8    = hardware_vector<int16, 8, v8i16>;
    using int32x4    = hardware_vector<int32, 4, v4i32>;
    using int64x2    = hardware_vector<int64, 2, v2i64>;
    using uint8x16   = hardware_vector<uint8, 16, v16u8>;
    using uint16x8   = hardware_vector<uint16, 8, v8u16>;
    using uint32x4   = hardware_vector<uint32, 4, v4u32>;
    using uint64x2   = hardware_vector<uint64, 2, v2u64>;
    using float32x4  = hardware_vector<float, 4, v4f32>;
    using float64x2  = hardware_vector<double, 2, v2f64>;

    // 256 bit vector
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

    // 512 bit vector
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

    // 128 bit vector mask
    using mask8x16   = hardware_mask<8, 16, v16u8>;
    using mask16x8   = hardware_mask<16, 8, v8u16>;
    using mask32x4   = hardware_mask<32, 4, v4u32>;
    using mask64x2   = hardware_mask<64, 2, v2u64>;

    // 256 bit vector mask
    using mask8x32   = composite_mask<mask8x16>;
    using mask16x16  = composite_mask<mask16x8>;
    using mask32x8   = composite_mask<mask32x4>;
    using mask64x4   = composite_mask<mask64x2>;

    // 512 bit vector mask
    using mask8x64   = composite_mask<mask8x32>;
    using mask16x32  = composite_mask<mask16x16>;
    using mask32x16  = composite_mask<mask32x8>;
    using mask64x8   = composite_mask<mask64x4>;
    
} // namespace simd
} // namespace mango

#include "msa_int128.hpp"
#include "msa_float128.hpp"
#include "msa_double128.hpp"
#include "composite_int256.hpp"
#include "composite_float256.hpp"
#include "composite_double256.hpp"
#include "composite_int512.hpp"
#include "composite_float512.hpp"
#include "composite_double512.hpp"
#include "msa_convert.hpp"
#include "common_gather.hpp"

#else

namespace mango {
namespace simd {

    // --------------------------------------------------------------
    // SIMD emulation
    // --------------------------------------------------------------

    // 64 bit vector
    using float16x4  = scalar_vector<half, 4>;

    // 128 bit vector
    using int8x16    = scalar_vector<int8, 16>;
    using int16x8    = scalar_vector<int16, 8>;
    using int32x4    = scalar_vector<int32, 4>;
    using int64x2    = scalar_vector<int64, 2>;
    using uint8x16   = scalar_vector<uint8, 16>;
    using uint16x8   = scalar_vector<uint16, 8>;
    using uint32x4   = scalar_vector<uint32, 4>;
    using uint64x2   = scalar_vector<uint64, 2>;
    using float32x4  = scalar_vector<float, 4>;
    using float64x2  = scalar_vector<double, 2>;

    // 256 bit vector
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

    // 512 bit vector
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

    // 128 bit vector mask
    using mask8x16   = scalar_mask<8, 16>;
    using mask16x8   = scalar_mask<16, 8>;
    using mask32x4   = scalar_mask<32, 4>;
    using mask64x2   = scalar_mask<64, 2>;

    // 256 bit vector mask
    using mask8x32   = composite_mask<mask8x16>;
    using mask16x16  = composite_mask<mask16x8>;
    using mask32x8   = composite_mask<mask32x4>;
    using mask64x4   = composite_mask<mask64x2>;

    // 512 bit vector mask
    using mask8x64   = composite_mask<mask8x32>;
    using mask16x32  = composite_mask<mask16x16>;
    using mask32x16  = composite_mask<mask32x8>;
    using mask64x8   = composite_mask<mask64x4>;

} // namespace simd
} // namespace mango

#include "scalar_int128.hpp"
#include "scalar_float128.hpp"
#include "scalar_double128.hpp"
#include "composite_int256.hpp"
#include "composite_float256.hpp"
#include "composite_double256.hpp"
#include "composite_int512.hpp"
#include "composite_float512.hpp"
#include "composite_double512.hpp"
#include "scalar_convert.hpp"
#include "common_gather.hpp"

#endif
