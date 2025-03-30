/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include <type_traits>
#include <mango/core/configure.hpp>
#include <mango/core/half.hpp>
#include <mango/core/bits.hpp>

// ------------------------------------------------------------------
// Configure SIMD implementation
// ------------------------------------------------------------------

namespace mango::simd
{

    using f16 = float16;
    using f32 = float32;
    using f64 = float64;

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
        enum
        {
            alignment = sizeof(MaskType),
        };

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
            alignment = sizeof(VectorType),
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
        enum
        {
            alignment = sizeof(u32),
        };

        u32 mask;

        scalar_mask() = default;
        scalar_mask(u32 mask)
            : mask(mask)
        {
        }

        operator u32 () const
        {
            return mask;
        }
    };

    template <typename ScalarType, int VectorSize>
    struct scalar_vector
    {
        using scalar = ScalarType;
        using vector = ScalarType[VectorSize];

        enum
        {
            scalar_bits = sizeof(ScalarType) * 8,
            vector_bits = sizeof(ScalarType) * VectorSize * 8,
            alignment = sizeof(ScalarType),
            size = VectorSize,
            is_hardware = 0,
            is_composite = 0
        };

        vector data;

        ScalarType& operator [] (int index)
        {
            return data[index];
        }

        const ScalarType& operator [] (int index) const
        {
            return data[index];
        }
    };

    // Vector types implemented as separate low/high parts, which typically have hardware support

    template <typename MaskType>
    struct composite_mask
    {
        enum
        {
            alignment = MaskType::alignment,
        };

        alignas(alignment) MaskType data[2];
    };

    template <typename VectorType>
    struct composite_vector
    {
        using scalar = typename VectorType::scalar;
        using vector = std::array<VectorType, 2>;

        enum
        {
            scalar_bits = sizeof(scalar) * 8,
            vector_bits = sizeof(VectorType) * 2 * 8,
            alignment = VectorType::alignment,
            size = VectorType::size * 2,
            is_hardware = VectorType::is_hardware,
            is_composite = 1
        };

        alignas(VectorType::alignment) vector data;

        composite_vector() = default;
        composite_vector(VectorType lo, VectorType hi)
            : data{lo, hi}
        {
        }
    };

} // namespace mango::simd

#if defined(MANGO_ENABLE_AVX512)

// --------------------------------------------------------------
// Intel AVX512 vector intrinsics
// --------------------------------------------------------------

namespace mango::simd
{

    #define MANGO_ENABLE_SIMD
    #define MANGO_SIMD_VECTOR_SIZE 512

    // 64 bit vector
    using s32x2   = scalar_vector<s32, 2>;
    using u32x2   = scalar_vector<u32, 2>;
    using f32x2   = scalar_vector<f32, 2>;
    using f16x4   = hardware_vector<f16, 4, u64>;

    // 128 bit vector
    using s8x16   = hardware_vector<s8, 16, __m128i>;
    using s16x8   = hardware_vector<s16, 8, __m128i>;
    using s32x4   = hardware_vector<s32, 4, __m128i>;
    using s64x2   = hardware_vector<s64, 2, __m128i>;
    using u8x16   = hardware_vector<u8, 16, __m128i>;
    using u16x8   = hardware_vector<u16, 8, __m128i>;
    using u32x4   = hardware_vector<u32, 4, __m128i>;
    using u64x2   = hardware_vector<u64, 2, __m128i>;
    using f32x4   = hardware_vector<f32, 4, __m128>;
    using f64x2   = hardware_vector<f64, 2, __m128d>;

    // 256 bit vector
    using s8x32   = hardware_vector<s8, 32, __m256i>;
    using s16x16  = hardware_vector<s16, 16, __m256i>;
    using s32x8   = hardware_vector<s32, 8, __m256i>;
    using s64x4   = hardware_vector<s64, 4, __m256i>;
    using u8x32   = hardware_vector<u8, 32, __m256i>;
    using u16x16  = hardware_vector<u16, 16, __m256i>;
    using u32x8   = hardware_vector<u32, 8, __m256i>;
    using u64x4   = hardware_vector<u64, 4, __m256i>;
    using f32x8   = hardware_vector<f32, 8, __m256>;
    using f64x4   = hardware_vector<f64, 4, __m256d>;

    // 512 bit vector
    using s8x64   = hardware_vector<s8, 64, __m512i>;
    using s16x32  = hardware_vector<s16, 32, __m512i>;
    using s32x16  = hardware_vector<s32, 16, __m512i>;
    using s64x8   = hardware_vector<s64, 8, __m512i>;
    using u8x64   = hardware_vector<u8, 64, __m512i>;
    using u16x32  = hardware_vector<u16, 32, __m512i>;
    using u32x16  = hardware_vector<u32, 16, __m512i>;
    using u64x8   = hardware_vector<u64, 8, __m512i>;
    using f32x16  = hardware_vector<f32, 16, __m512>;
    using f64x8   = hardware_vector<f64, 8, __m512d>;

    // 128 bit vector mask
    using mask8x16  = hardware_mask<8, 16, __mmask16>;
    using mask16x8  = hardware_mask<16, 8, __mmask8>;
    using mask32x4  = hardware_mask<32, 4, __mmask8>;
    using mask64x2  = hardware_mask<64, 2, __mmask8>;

    // 256 bit vector mask
    using mask8x32  = hardware_mask<8, 32, __mmask32>;
    using mask16x16 = hardware_mask<16, 16, __mmask16>;
    using mask32x8  = hardware_mask<32, 8, __mmask8>;
    using mask64x4  = hardware_mask<64, 4, __mmask8>;

    // 512 bit vector mask
    using mask8x64  = hardware_mask<8, 64, __mmask64>;
    using mask16x32 = hardware_mask<16, 32, __mmask32>;
    using mask32x16 = hardware_mask<32, 16, __mmask16>;
    using mask64x8  = hardware_mask<64, 8, __mmask8>;

} // namespace mango::simd

#include <mango/simd/scalar_64_float.hpp>
#include <mango/simd/scalar_64_int.hpp>
#include <mango/simd/avx512_128_float.hpp>
#include <mango/simd/avx512_128_int.hpp>
#include <mango/simd/avx512_256_float.hpp>
#include <mango/simd/avx512_256_int.hpp>
#include <mango/simd/avx512_512_float.hpp>
#include <mango/simd/avx512_512_int.hpp>
#include <mango/simd/avx512_convert.hpp>
#include <mango/simd/avx512_gather.hpp>

#elif defined(MANGO_ENABLE_AVX2)

// --------------------------------------------------------------
// Intel AVX2 vector intrinsics
// --------------------------------------------------------------

namespace mango::simd
{

    #define MANGO_ENABLE_SIMD
    #define MANGO_SIMD_VECTOR_SIZE 256

    // 64 bit vector
    using s32x2   = scalar_vector<s32, 2>;
    using u32x2   = scalar_vector<u32, 2>;
    using f32x2   = scalar_vector<f32, 2>;
    using f16x4   = hardware_vector<f16, 4, u64>;

    // 128 bit vector
    using s8x16   = hardware_vector<s8, 16, __m128i>;
    using s16x8   = hardware_vector<s16, 8, __m128i>;
    using s32x4   = hardware_vector<s32, 4, __m128i>;
    using s64x2   = hardware_vector<s64, 2, __m128i>;
    using u8x16   = hardware_vector<u8, 16, __m128i>;
    using u16x8   = hardware_vector<u16, 8, __m128i>;
    using u32x4   = hardware_vector<u32, 4, __m128i>;
    using u64x2   = hardware_vector<u64, 2, __m128i>;
    using f32x4   = hardware_vector<f32, 4, __m128>;
    using f64x2   = hardware_vector<f64, 2, __m128d>;

    // 256 bit vector
    using s8x32   = hardware_vector<s8, 32, __m256i>;
    using s16x16  = hardware_vector<s16, 16, __m256i>;
    using s32x8   = hardware_vector<s32, 8, __m256i>;
    using s64x4   = hardware_vector<s64, 4, __m256i>;
    using u8x32   = hardware_vector<u8, 32, __m256i>;
    using u16x16  = hardware_vector<u16, 16, __m256i>;
    using u32x8   = hardware_vector<u32, 8, __m256i>;
    using u64x4   = hardware_vector<u64, 4, __m256i>;
    using f32x8   = hardware_vector<f32, 8, __m256>;
    using f64x4   = hardware_vector<f64, 4, __m256d>;

    // 512 bit vector
    using s8x64   = composite_vector<s8x32>;
    using s16x32  = composite_vector<s16x16>;
    using s32x16  = composite_vector<s32x8>;
    using s64x8   = composite_vector<s64x4>;
    using u8x64   = composite_vector<u8x32>;
    using u16x32  = composite_vector<u16x16>;
    using u32x16  = composite_vector<u32x8>;
    using u64x8   = composite_vector<u64x4>;
    using f32x16  = composite_vector<f32x8>;
    using f64x8   = composite_vector<f64x4>;

    // 128 bit vector mask
    using mask8x16  = hardware_mask<8, 16, __m128i>;
    using mask16x8  = hardware_mask<16, 8, __m128i>;
    using mask32x4  = hardware_mask<32, 4, __m128i>;
    using mask64x2  = hardware_mask<64, 2, __m128i>;

    // 256 bit vector mask
    using mask8x32  = hardware_mask<8, 32, __m256i>;
    using mask16x16 = hardware_mask<16, 16, __m256i>;
    using mask32x8  = hardware_mask<32, 8, __m256i>;
    using mask64x4  = hardware_mask<64, 4, __m256i>;

    // 512 bit vector mask
    using mask8x64  = composite_mask<mask8x32>;
    using mask16x32 = composite_mask<mask16x16>;
    using mask32x16 = composite_mask<mask32x8>;
    using mask64x8  = composite_mask<mask64x4>;

} // namespace mango::simd

#include <mango/simd/scalar_64_int.hpp>
#include <mango/simd/scalar_64_float.hpp>
#include <mango/simd/sse2_128_int.hpp>
#include <mango/simd/sse2_128_float.hpp>
#include <mango/simd/avx2_256_int.hpp>
#include <mango/simd/avx_256_float.hpp>
#include <mango/simd/composite_512_int.hpp>
#include <mango/simd/composite_512_float.hpp>
#include <mango/simd/avx_convert.hpp>
#include <mango/simd/avx2_gather.hpp>

#elif defined(MANGO_ENABLE_AVX)

// --------------------------------------------------------------
// Intel AVX vector intrinsics
// --------------------------------------------------------------

namespace mango::simd
{

    #define MANGO_ENABLE_SIMD
    #define MANGO_SIMD_VECTOR_SIZE 128 /* Sorry AVX, nice try.. */

    // 64 bit vector
    using s32x2   = scalar_vector<s32, 2>;
    using u32x2   = scalar_vector<u32, 2>;
    using f32x2   = scalar_vector<f32, 2>;
    using f16x4   = hardware_vector<f16, 4, u64>;

    // 128 bit vector
    using s8x16   = hardware_vector<s8, 16, __m128i>;
    using s16x8   = hardware_vector<s16, 8, __m128i>;
    using s32x4   = hardware_vector<s32, 4, __m128i>;
    using s64x2   = hardware_vector<s64, 2, __m128i>;
    using u8x16   = hardware_vector<u8, 16, __m128i>;
    using u16x8   = hardware_vector<u16, 8, __m128i>;
    using u32x4   = hardware_vector<u32, 4, __m128i>;
    using u64x2   = hardware_vector<u64, 2, __m128i>;
    using f32x4   = hardware_vector<f32, 4, __m128>;
    using f64x2   = hardware_vector<f64, 2, __m128d>;

    // 256 bit vector
    using s8x32   = composite_vector<s8x16>;
    using s16x16  = composite_vector<s16x8>;
    using s32x8   = composite_vector<s32x4>;
    using s64x4   = composite_vector<s64x2>;
    using u8x32   = composite_vector<u8x16>;
    using u16x16  = composite_vector<u16x8>;
    using u32x8   = composite_vector<u32x4>;
    using u64x4   = composite_vector<u64x2>;
    using f32x8   = hardware_vector<f32, 8, __m256>;
    using f64x4   = hardware_vector<f64, 4, __m256d>;

    // 512 bit vector
    using s8x64   = composite_vector<s8x32>;
    using s16x32  = composite_vector<s16x16>;
    using s32x16  = composite_vector<s32x8>;
    using s64x8   = composite_vector<s64x4>;
    using u8x64   = composite_vector<u8x32>;
    using u16x32  = composite_vector<u16x16>;
    using u32x16  = composite_vector<u32x8>;
    using u64x8   = composite_vector<u64x4>;
    using f32x16  = composite_vector<f32x8>;
    using f64x8   = composite_vector<f64x4>;

    // 128 bit vector mask
    using mask8x16  = hardware_mask<8, 16, __m128i>;
    using mask16x8  = hardware_mask<16, 8, __m128i>;
    using mask32x4  = hardware_mask<32, 4, __m128i>;
    using mask64x2  = hardware_mask<64, 2, __m128i>;

    // 256 bit vector mask
    using mask8x32  = hardware_mask<8, 32, __m256i>;
    using mask16x16 = hardware_mask<16, 16, __m256i>;
    using mask32x8  = hardware_mask<32, 8, __m256i>;
    using mask64x4  = hardware_mask<64, 4, __m256i>;

    // 512 bit vector mask
    using mask8x64  = composite_mask<mask8x32>;
    using mask16x32 = composite_mask<mask16x16>;
    using mask32x16 = composite_mask<mask32x8>;
    using mask64x8  = composite_mask<mask64x4>;

} // namespace mango::simd

#include <mango/simd/scalar_64_int.hpp>
#include <mango/simd/scalar_64_float.hpp>
#include <mango/simd/sse2_128_int.hpp>
#include <mango/simd/sse2_128_float.hpp>
#include <mango/simd/avx_256_int.hpp>
#include <mango/simd/avx_256_float.hpp>
#include <mango/simd/composite_512_int.hpp>
#include <mango/simd/composite_512_float.hpp>
#include <mango/simd/avx_convert.hpp>
#include <mango/simd/common_gather.hpp>

#elif defined(MANGO_ENABLE_SSE2)

// --------------------------------------------------------------
// Intel SSE2 vector intrinsics
// --------------------------------------------------------------

namespace mango::simd
{

    #define MANGO_ENABLE_SIMD
    #define MANGO_SIMD_VECTOR_SIZE 128

    // 64 bit vector
    using s32x2   = scalar_vector<s32, 2>;
    using u32x2   = scalar_vector<u32, 2>;
    using f32x2   = scalar_vector<f32, 2>;
    using f16x4   = hardware_vector<f16, 4, u64>;

    // 128 bit vector
    using s8x16   = hardware_vector<s8, 16, __m128i>;
    using s16x8   = hardware_vector<s16, 8, __m128i>;
    using s32x4   = hardware_vector<s32, 4, __m128i>;
    using s64x2   = hardware_vector<s64, 2, __m128i>;
    using u8x16   = hardware_vector<u8, 16, __m128i>;
    using u16x8   = hardware_vector<u16, 8, __m128i>;
    using u32x4   = hardware_vector<u32, 4, __m128i>;
    using u64x2   = hardware_vector<u64, 2, __m128i>;
    using f32x4   = hardware_vector<f32, 4, __m128>;
    using f64x2   = hardware_vector<f64, 2, __m128d>;

    // 256 bit vector
    using s8x32   = composite_vector<s8x16>;
    using s16x16  = composite_vector<s16x8>;
    using s32x8   = composite_vector<s32x4>;
    using s64x4   = composite_vector<s64x2>;
    using u8x32   = composite_vector<u8x16>;
    using u16x16  = composite_vector<u16x8>;
    using u32x8   = composite_vector<u32x4>;
    using u64x4   = composite_vector<u64x2>;
    using f32x8   = composite_vector<f32x4>;
    using f64x4   = composite_vector<f64x2>;

    // 512 bit vector
    using s8x64    = composite_vector<s8x32>;
    using s16x32   = composite_vector<s16x16>;
    using s32x16   = composite_vector<s32x8>;
    using s64x8    = composite_vector<s64x4>;
    using u8x64    = composite_vector<u8x32>;
    using u16x32   = composite_vector<u16x16>;
    using u32x16   = composite_vector<u32x8>;
    using u64x8    = composite_vector<u64x4>;
    using f32x16   = composite_vector<f32x8>;
    using f64x8    = composite_vector<f64x4>;

    // 128 bit vector mask
    using mask8x16  = hardware_mask<8, 16, __m128i>;
    using mask16x8  = hardware_mask<16, 8, __m128i>;
    using mask32x4  = hardware_mask<32, 4, __m128i>;
    using mask64x2  = hardware_mask<64, 2, __m128i>;

    // 256 bit vector mask
    using mask8x32  = composite_mask<mask8x16>;
    using mask16x16 = composite_mask<mask16x8>;
    using mask32x8  = composite_mask<mask32x4>;
    using mask64x4  = composite_mask<mask64x2>;

    // 512 bit vector mask
    using mask8x64  = composite_mask<mask8x32>;
    using mask16x32 = composite_mask<mask16x16>;
    using mask32x16 = composite_mask<mask32x8>;
    using mask64x8  = composite_mask<mask64x4>;

} // namespace mango::simd

#include <mango/simd/scalar_64_int.hpp>
#include <mango/simd/scalar_64_float.hpp>
#include <mango/simd/sse2_128_int.hpp>
#include <mango/simd/sse2_128_float.hpp>
#include <mango/simd/composite_256_int.hpp>
#include <mango/simd/composite_256_float.hpp>
#include <mango/simd/composite_512_int.hpp>
#include <mango/simd/composite_512_float.hpp>
#include <mango/simd/sse2_convert.hpp>
#include <mango/simd/common_gather.hpp>

#elif defined(MANGO_ENABLE_NEON)

// --------------------------------------------------------------
// ARM NEON vector instrinsics
// --------------------------------------------------------------

namespace mango::simd
{

    // http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0491f/BABDFJCI.html
    // http://infocenter.arm.com/help/topic/com.arm.doc.ihi0073a/IHI0073A_arm_neon_intrinsics_ref.pdf
    // http://kib.kiev.ua/x86docs/ARMARM/DDI0487A_e_armv8_arm.pdf

    #define MANGO_ENABLE_SIMD
    #define MANGO_SIMD_VECTOR_SIZE 128

#ifndef __aarch64__

    struct float64x2_t
    {
        f64 data[2];
        float64x2_t() = default;
        float64x2_t(f64 x, f64 y) { data[0] = x; data[1] = y; }
        f64 & operator [] (int index) { return data[index]; }
        const f64 & operator [] (int index) const { return data[index]; }
    };

#endif

    // 64 bit vector
    using s32x2   = hardware_vector<s32, 2, int32x2_t>;
    using u32x2   = hardware_vector<u32, 2, uint32x2_t>;
    using f32x2   = hardware_vector<f32, 2, float32x2_t>;

#ifdef MANGO_ENABLE_ARM_FP16
    using f16x4   = hardware_vector<f16, 4, float16x4_t>;
#else
    using f16x4   = hardware_vector<f16, 4, u64>;
#endif

    // 128 bit vector
    using s8x16   = hardware_vector<s8, 16, int8x16_t>;
    using s16x8   = hardware_vector<s16, 8, int16x8_t>;
    using s32x4   = hardware_vector<s32, 4, int32x4_t>;
    using s64x2   = hardware_vector<s64, 2, int64x2_t>;
    using u8x16   = hardware_vector<u8, 16, uint8x16_t>;
    using u16x8   = hardware_vector<u16, 8, uint16x8_t>;
    using u32x4   = hardware_vector<u32, 4, uint32x4_t>;
    using u64x2   = hardware_vector<u64, 2, uint64x2_t>;
    using f32x4   = hardware_vector<f32, 4, float32x4_t>;
    using f64x2   = hardware_vector<f64, 2, float64x2_t>;

    // 256 bit vector
    using s8x32   = composite_vector<s8x16>;
    using s16x16  = composite_vector<s16x8>;
    using s32x8   = composite_vector<s32x4>;
    using s64x4   = composite_vector<s64x2>;
    using u8x32   = composite_vector<u8x16>;
    using u16x16  = composite_vector<u16x8>;
    using u32x8   = composite_vector<u32x4>;
    using u64x4   = composite_vector<u64x2>;
    using f32x8   = composite_vector<f32x4>;
    using f64x4   = composite_vector<f64x2>;

    // 512 bit vector
    using s8x64   = composite_vector<s8x32>;
    using s16x32  = composite_vector<s16x16>;
    using s32x16  = composite_vector<s32x8>;
    using s64x8   = composite_vector<s64x4>;
    using u8x64   = composite_vector<u8x32>;
    using u16x32  = composite_vector<u16x16>;
    using u32x16  = composite_vector<u32x8>;
    using u64x8   = composite_vector<u64x4>;
    using f32x16  = composite_vector<f32x8>;
    using f64x8   = composite_vector<f64x4>;

    // 128 bit vector mask
    using mask8x16  = hardware_mask<8, 16, uint8x16_t>;
    using mask16x8  = hardware_mask<16, 8, uint16x8_t>;
    using mask32x4  = hardware_mask<32, 4, uint32x4_t>;
    using mask64x2  = hardware_mask<64, 2, uint64x2_t>;

    // 256 bit vector mask
    using mask8x32  = composite_mask<mask8x16>;
    using mask16x16 = composite_mask<mask16x8>;
    using mask32x8  = composite_mask<mask32x4>;
    using mask64x4  = composite_mask<mask64x2>;

    // 512 bit vector mask
    using mask8x64  = composite_mask<mask8x32>;
    using mask16x32 = composite_mask<mask16x16>;
    using mask32x16 = composite_mask<mask32x8>;
    using mask64x8  = composite_mask<mask64x4>;

} // namespace mango::simd

#include <mango/simd/neon_64_int.hpp>
#include <mango/simd/neon_64_float.hpp>
#include <mango/simd/neon_128_int.hpp>
#include <mango/simd/neon_128_float.hpp>
#include <mango/simd/composite_256_int.hpp>
#include <mango/simd/composite_256_float.hpp>
#include <mango/simd/composite_512_int.hpp>
#include <mango/simd/composite_512_float.hpp>
#include <mango/simd/neon_convert.hpp>
#include <mango/simd/common_gather.hpp>

#elif defined(MANGO_ENABLE_ALTIVEC) && defined(MANGO_ENABLE_VSX)

// --------------------------------------------------------------
// Altivec / VSX
// --------------------------------------------------------------

#include <altivec.h>

namespace mango::simd
{

    // https://www.nxp.com/docs/en/reference-manual/ALTIVECPIM.pdf
    // https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.4.0/com.ibm.zos.v2r4.cbcpx01/vectorbltin.htm
    // https://www.ibm.com/support/knowledgecenter/SSGH2K_13.1.2/com.ibm.xlc131.aix.doc/compiler_ref/vec_intrin_cpp.html

    #define MANGO_ENABLE_SIMD
    #define MANGO_SIMD_VECTOR_SIZE 128

    // 64 bit vector
    using s32x2   = scalar_vector<s32, 2>;
    using u32x2   = scalar_vector<u32, 2>;
    using f32x2   = scalar_vector<f32, 2>;
    using f16x4   = hardware_vector<f16, 4, u64>;

    // 128 bit vector
    using s8x16   = hardware_vector<s8, 16, __vector signed char>;
    using s16x8   = hardware_vector<s16, 8, __vector signed short>;
    using s32x4   = hardware_vector<s32, 4, __vector signed int>;
    using s64x2   = hardware_vector<s64, 2, __vector signed long long>;
    using u8x16   = hardware_vector<u8, 16, __vector unsigned char>;
    using u16x8   = hardware_vector<u16, 8, __vector unsigned short>;
    using u32x4   = hardware_vector<u32, 4, __vector unsigned int>;
    using u64x2   = hardware_vector<u64, 2, __vector unsigned long long>;
    using f32x4   = hardware_vector<f32, 4, __vector float>;
    using f64x2   = hardware_vector<f64, 2, __vector double>;

    // 256 bit vector
    using s8x32   = composite_vector<s8x16>;
    using s16x16  = composite_vector<s16x8>;
    using s32x8   = composite_vector<s32x4>;
    using s64x4   = composite_vector<s64x2>;
    using u8x32   = composite_vector<u8x16>;
    using u16x16  = composite_vector<u16x8>;
    using u32x8   = composite_vector<u32x4>;
    using u64x4   = composite_vector<u64x2>;
    using f32x8   = composite_vector<f32x4>;
    using f64x4   = composite_vector<f64x2>;

    // 512 bit vector
    using s8x64   = composite_vector<s8x32>;
    using s16x32  = composite_vector<s16x16>;
    using s32x16  = composite_vector<s32x8>;
    using s64x8   = composite_vector<s64x4>;
    using u8x64   = composite_vector<u8x32>;
    using u16x32  = composite_vector<u16x16>;
    using u32x16  = composite_vector<u32x8>;
    using u64x8   = composite_vector<u64x4>;
    using f32x16  = composite_vector<f32x8>;
    using f64x8   = composite_vector<f64x4>;

    // 128 bit vector mask
    using mask8x16  = hardware_mask<8, 16, __vector bool char>;
    using mask16x8  = hardware_mask<16, 8, __vector bool short>;
    using mask32x4  = hardware_mask<32, 4, __vector bool int>;
    using mask64x2  = hardware_mask<64, 2, __vector bool long long>;

    // 256 bit vector mask
    using mask8x32  = composite_mask<mask8x16>;
    using mask16x16 = composite_mask<mask16x8>;
    using mask32x8  = composite_mask<mask32x4>;
    using mask64x4  = composite_mask<mask64x2>;

    // 512 bit vector mask
    using mask8x64  = composite_mask<mask8x32>;
    using mask16x32 = composite_mask<mask16x16>;
    using mask32x16 = composite_mask<mask32x8>;
    using mask64x8  = composite_mask<mask64x4>;

} // namespace mango::simd

// <altivec.h> defined these; clean up
#undef bool
#undef vector
#undef pixel
#undef __bool
#undef __vector
#undef __pixel

#include <mango/simd/scalar_64_int.hpp>
#include <mango/simd/scalar_64_float.hpp>
#include <mango/simd/altivec_128_int.hpp>
#include <mango/simd/altivec_128_float.hpp>
#include <mango/simd/composite_256_int.hpp>
#include <mango/simd/composite_256_float.hpp>
#include <mango/simd/composite_512_int.hpp>
#include <mango/simd/composite_512_float.hpp>
#include <mango/simd/altivec_convert.hpp>
#include <mango/simd/common_gather.hpp>

#elif defined(MANGO_ENABLE_MSA)

// --------------------------------------------------------------
// MIPS MSA
// --------------------------------------------------------------

namespace mango::simd
{

    #define MANGO_ENABLE_SIMD
    #define MANGO_SIMD_VECTOR_SIZE 128

    // 64 bit vector
    using s32x2   = scalar_vector<s32, 2>;
    using u32x2   = scalar_vector<u32, 2>;
    using f32x2   = scalar_vector<f32, 2>;
    using f16x4   = hardware_vector<f16, 4, u64>;

    // 128 bit vector
    using s8x16   = hardware_vector<s8, 16, v16i8>;
    using s16x8   = hardware_vector<s16, 8, v8i16>;
    using s32x4   = hardware_vector<s32, 4, v4i32>;
    using s64x2   = hardware_vector<s64, 2, v2i64>;
    using u8x16   = hardware_vector<u8, 16, v16u8>;
    using u16x8   = hardware_vector<u16, 8, v8u16>;
    using u32x4   = hardware_vector<u32, 4, v4u32>;
    using u64x2   = hardware_vector<u64, 2, v2u64>;
    using f32x4   = hardware_vector<f32, 4, v4f32>;
    using f64x2   = hardware_vector<f64, 2, v2f64>;

    // 256 bit vector
    using s8x32   = composite_vector<s8x16>;
    using s16x16  = composite_vector<s16x8>;
    using s32x8   = composite_vector<s32x4>;
    using s64x4   = composite_vector<s64x2>;
    using u8x32   = composite_vector<u8x16>;
    using u16x16  = composite_vector<u16x8>;
    using u32x8   = composite_vector<u32x4>;
    using u64x4   = composite_vector<u64x2>;
    using f32x8   = composite_vector<f32x4>;
    using f64x4   = composite_vector<f64x2>;

    // 512 bit vector
    using s8x64   = composite_vector<s8x32>;
    using s16x32  = composite_vector<s16x16>;
    using s32x16  = composite_vector<s32x8>;
    using s64x8   = composite_vector<s64x4>;
    using u8x64   = composite_vector<u8x32>;
    using u16x32  = composite_vector<u16x16>;
    using u32x16  = composite_vector<u32x8>;
    using u64x8   = composite_vector<u64x4>;
    using f32x16  = composite_vector<f32x8>;
    using f64x8   = composite_vector<f64x4>;

    // 128 bit vector mask
    using mask8x16  = hardware_mask<8, 16, v16u8>;
    using mask16x8  = hardware_mask<16, 8, v8u16>;
    using mask32x4  = hardware_mask<32, 4, v4u32>;
    using mask64x2  = hardware_mask<64, 2, v2u64>;

    // 256 bit vector mask
    using mask8x32  = composite_mask<mask8x16>;
    using mask16x16 = composite_mask<mask16x8>;
    using mask32x8  = composite_mask<mask32x4>;
    using mask64x4  = composite_mask<mask64x2>;

    // 512 bit vector mask
    using mask8x64  = composite_mask<mask8x32>;
    using mask16x32 = composite_mask<mask16x16>;
    using mask32x16 = composite_mask<mask32x8>;
    using mask64x8  = composite_mask<mask64x4>;

} // namespace mango::simd

#include <mango/simd/scalar_64_int.hpp>
#include <mango/simd/scalar_64_float.hpp>
#include <mango/simd/msa_128_int.hpp>
#include <mango/simd/msa_128_float.hpp>
#include <mango/simd/composite_256_int.hpp>
#include <mango/simd/composite_256_float.hpp>
#include <mango/simd/composite_512_int.hpp>
#include <mango/simd/composite_512_float.hpp>
#include <mango/simd/msa_convert.hpp>
#include <mango/simd/common_gather.hpp>

#elif defined(MANGO_ENABLE_WASM)

// --------------------------------------------------------------
// WASM
// --------------------------------------------------------------

#include <wasm_simd128.h>

namespace mango::simd
{

    #define MANGO_ENABLE_SIMD
    #define MANGO_SIMD_VECTOR_SIZE 128

    // 64 bit vector
    using s32x2   = scalar_vector<s32, 2>;
    using u32x2   = scalar_vector<u32, 2>;
    using f32x2   = scalar_vector<f32, 2>;
    using f16x4   = hardware_vector<f16, 4, u64>;

    // 128 bit vector
    using s8x16   = hardware_vector<s8, 16, v128_t>;
    using s16x8   = hardware_vector<s16, 8, v128_t>;
    using s32x4   = hardware_vector<s32, 4, v128_t>;
    using s64x2   = hardware_vector<s64, 2, v128_t>;
    using u8x16   = hardware_vector<u8, 16, v128_t>;
    using u16x8   = hardware_vector<u16, 8, v128_t>;
    using u32x4   = hardware_vector<u32, 4, v128_t>;
    using u64x2   = hardware_vector<u64, 2, v128_t>;
    using f32x4   = hardware_vector<f32, 4, v128_t>;
    using f64x2   = hardware_vector<f64, 2, v128_t>;

    // 256 bit vector
    using s8x32   = composite_vector<s8x16>;
    using s16x16  = composite_vector<s16x8>;
    using s32x8   = composite_vector<s32x4>;
    using s64x4   = composite_vector<s64x2>;
    using u8x32   = composite_vector<u8x16>;
    using u16x16  = composite_vector<u16x8>;
    using u32x8   = composite_vector<u32x4>;
    using u64x4   = composite_vector<u64x2>;
    using f32x8   = composite_vector<f32x4>;
    using f64x4   = composite_vector<f64x2>;

    // 512 bit vector
    using s8x64   = composite_vector<s8x32>;
    using s16x32  = composite_vector<s16x16>;
    using s32x16  = composite_vector<s32x8>;
    using s64x8   = composite_vector<s64x4>;
    using u8x64   = composite_vector<u8x32>;
    using u16x32  = composite_vector<u16x16>;
    using u32x16  = composite_vector<u32x8>;
    using u64x8   = composite_vector<u64x4>;
    using f32x16  = composite_vector<f32x8>;
    using f64x8   = composite_vector<f64x4>;

    // 128 bit vector mask
    using mask8x16  = hardware_mask<8, 16, v128_t>;
    using mask16x8  = hardware_mask<16, 8, v128_t>;
    using mask32x4  = hardware_mask<32, 4, v128_t>;
    using mask64x2  = hardware_mask<64, 2, v128_t>;

    // 256 bit vector mask
    using mask8x32  = composite_mask<mask8x16>;
    using mask16x16 = composite_mask<mask16x8>;
    using mask32x8  = composite_mask<mask32x4>;
    using mask64x4  = composite_mask<mask64x2>;

    // 512 bit vector mask
    using mask8x64  = composite_mask<mask8x32>;
    using mask16x32 = composite_mask<mask16x16>;
    using mask32x16 = composite_mask<mask32x8>;
    using mask64x8  = composite_mask<mask64x4>;

} // namespace mango::simd

#include <mango/simd/scalar_64_int.hpp>
#include <mango/simd/scalar_64_float.hpp>
#include <mango/simd/wasm_128_int.hpp>
#include <mango/simd/wasm_128_float.hpp>
#include <mango/simd/composite_256_int.hpp>
#include <mango/simd/composite_256_float.hpp>
#include <mango/simd/composite_512_int.hpp>
#include <mango/simd/composite_512_float.hpp>
#include <mango/simd/wasm_convert.hpp>
#include <mango/simd/common_gather.hpp>

#else

// --------------------------------------------------------------
// SIMD emulation
// --------------------------------------------------------------

namespace mango::simd
{

    #define MANGO_SIMD_VECTOR_SIZE 32

    // 64 bit vector
    using s32x2   = scalar_vector<s32, 2>;
    using u32x2   = scalar_vector<u32, 2>;
    using f32x2   = scalar_vector<f32, 2>;
    using f16x4   = hardware_vector<f16, 4, u64>;

    // 128 bit vector
    using s8x16   = scalar_vector<s8, 16>;
    using s16x8   = scalar_vector<s16, 8>;
    using s32x4   = scalar_vector<s32, 4>;
    using s64x2   = scalar_vector<s64, 2>;
    using u8x16   = scalar_vector<u8, 16>;
    using u16x8   = scalar_vector<u16, 8>;
    using u32x4   = scalar_vector<u32, 4>;
    using u64x2   = scalar_vector<u64, 2>;
    using f32x4   = scalar_vector<f32, 4>;
    using f64x2   = scalar_vector<f64, 2>;

    // 256 bit vector
    using s8x32   = composite_vector<s8x16>;
    using s16x16  = composite_vector<s16x8>;
    using s32x8   = composite_vector<s32x4>;
    using s64x4   = composite_vector<s64x2>;
    using u8x32   = composite_vector<u8x16>;
    using u16x16  = composite_vector<u16x8>;
    using u32x8   = composite_vector<u32x4>;
    using u64x4   = composite_vector<u64x2>;
    using f32x8   = composite_vector<f32x4>;
    using f64x4   = composite_vector<f64x2>;

    // 512 bit vector
    using s8x64   = composite_vector<s8x32>;
    using s16x32  = composite_vector<s16x16>;
    using s32x16  = composite_vector<s32x8>;
    using s64x8   = composite_vector<s64x4>;
    using u8x64   = composite_vector<u8x32>;
    using u16x32  = composite_vector<u16x16>;
    using u32x16  = composite_vector<u32x8>;
    using u64x8   = composite_vector<u64x4>;
    using f32x16  = composite_vector<f32x8>;
    using f64x8   = composite_vector<f64x4>;

    // 128 bit vector mask
    using mask8x16  = scalar_mask<8, 16>;
    using mask16x8  = scalar_mask<16, 8>;
    using mask32x4  = scalar_mask<32, 4>;
    using mask64x2  = scalar_mask<64, 2>;

    // 256 bit vector mask
    using mask8x32  = composite_mask<mask8x16>;
    using mask16x16 = composite_mask<mask16x8>;
    using mask32x8  = composite_mask<mask32x4>;
    using mask64x4  = composite_mask<mask64x2>;

    // 512 bit vector mask
    using mask8x64  = composite_mask<mask8x32>;
    using mask16x32 = composite_mask<mask16x16>;
    using mask32x16 = composite_mask<mask32x8>;
    using mask64x8  = composite_mask<mask64x4>;

} // namespace mango::simd

#include <mango/simd/scalar_64_int.hpp>
#include <mango/simd/scalar_64_float.hpp>
#include <mango/simd/scalar_128_int.hpp>
#include <mango/simd/scalar_128_float.hpp>
#include <mango/simd/composite_256_int.hpp>
#include <mango/simd/composite_256_float.hpp>
#include <mango/simd/composite_512_int.hpp>
#include <mango/simd/composite_512_float.hpp>
#include <mango/simd/scalar_convert.hpp>
#include <mango/simd/common_gather.hpp>

#endif

namespace mango::simd
{

    // --------------------------------------------------------------
    // SIMD type traits
    // --------------------------------------------------------------

    // Default: not a mask type
    template <typename T>
    struct is_mask_type : std::false_type {};

    // Hardware mask type
    template <int ScalarBits, int VectorSize, typename MaskType>
    struct is_mask_type<hardware_mask<ScalarBits, VectorSize, MaskType>> : std::true_type {};

    // Scalar mask type
    template <int ScalarBits, int VectorSize>
    struct is_mask_type<scalar_mask<ScalarBits, VectorSize>> : std::true_type {};

    // Composite mask type
    template <typename MaskType>
    struct is_mask_type<composite_mask<MaskType>> : std::true_type {};

    template <typename T>
    concept is_mask = is_mask_type<T>::value;

    // Default: not a SIMD vector type
    template <typename T>
    struct is_vector_type : std::false_type {};

    // Hardware vector type
    template <typename ScalarType, int VectorSize, typename VectorType>
    struct is_vector_type<hardware_vector<ScalarType, VectorSize, VectorType>> : std::true_type {};

    // Scalar vector type
    template <typename ScalarType, int VectorSize>
    struct is_vector_type<scalar_vector<ScalarType, VectorSize>> : std::true_type {};

    // Composite vector type
    template <typename VectorType>
    struct is_vector_type<composite_vector<VectorType>> : std::true_type {};

    // Exception: f16x4 cannot do arithmetic operations
    template <>
    struct is_vector_type<f16x4> : std::false_type {};

    // Concept using the type trait
    template <typename T>
    concept is_vector = is_vector_type<T>::value && !is_mask_type<T>::value;

    // --------------------------------------------------------------
    // SIMD operators
    // --------------------------------------------------------------

    // vector operations

    template <typename T>
        requires is_vector<T>
    inline T nand(T a, T b)
    {
        return bitwise_nand(a, b);
    }

    template <typename T>
        requires is_vector<T>
    inline T operator & (T a, T b)
    {
        return bitwise_and(a, b);
    }

    template <typename T>
        requires is_vector<T>
    inline T operator | (T a, T b)
    {
        return bitwise_or(a, b);
    }

    template <typename T>
        requires is_vector<T>
    inline T operator ^ (T a, T b)
    {
        return bitwise_xor(a, b);
    }

    template <typename T>
        requires is_vector<T>
    inline T operator ~ (T a)
    {
        return bitwise_not(a);
    }

    // mask operations

    template <typename T>
        requires is_mask<T>
    inline T operator & (T a, T b)
    {
        return mask_and(a, b);
    }

    template <typename T>
        requires is_mask<T>
    inline T operator | (T a, T b)
    {
        return mask_or(a, b);
    }

    template <typename T>
        requires is_mask<T>
    inline T operator ^ (T a, T b)
    {
        return mask_xor(a, b);
    }

    template <typename T>
        requires is_mask<T>
    inline T operator ~ (T a)
    {
        return mask_not(a);
    }

} // namespace mango::simd
