/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "../core/configure.hpp"
#include "../core/half.hpp"

// ------------------------------------------------------------------
// Configure SIMD implementation
// ------------------------------------------------------------------

#define MANGO_INCLUDE_SIMD

#if defined(MANGO_ENABLE_AVX)

    // Intel AVX vector intrinsics
    #define MANGO_ENABLE_SIMD
    #include "simd4f_sse2.hpp"
    #include "simd4d_avx.hpp"

#elif defined(MANGO_ENABLE_SSE2)

    // Intel SSE2 vector intrinsics
    #define MANGO_ENABLE_SIMD
    #include "simd4f_sse2.hpp"
    #include "simd4d_sse2.hpp"

#elif defined(MANGO_ENABLE_NEON)

    // ARM NEON vector instrinsics
    #define MANGO_ENABLE_SIMD
    #include "simd4f_neon.hpp"
    #include "simd4d_fpu.hpp"

#elif defined(MANGO_ENABLE_ALTIVEC)

    // PowerPC Altivec / AVX128
    #define MANGO_ENABLE_SIMD
    #include "simd4f_altivec.hpp"
    #include "simd4d_fpu.hpp"

#elif defined(MANGO_ENABLE_SPU)

    // Cell BE SPU
    #define MANGO_ENABLE_SIMD
    #include "simd4f_spu.hpp"
    #include "simd4d_fpu.hpp"

#else

    // FPU SIMD emulation
    #include "simd4f_fpu.hpp"
    #include "simd4d_fpu.hpp"

#endif

namespace mango
{

    // ------------------------------------------------------------------
    // trigonometric functions
    // ------------------------------------------------------------------

    simd4f simd4f_sin(__simd4f a);
    simd4f simd4f_cos(__simd4f a);
    simd4f simd4f_tan(__simd4f a);
    simd4f simd4f_asin(__simd4f a);
    simd4f simd4f_acos(__simd4f a);
    simd4f simd4f_atan(__simd4f a);
    simd4f simd4f_exp(__simd4f a);
    simd4f simd4f_log(__simd4f a);
    simd4f simd4f_exp2(__simd4f a);
    simd4f simd4f_log2(__simd4f a);
    simd4f simd4f_pow(__simd4f a, __simd4f b);
    simd4f simd4f_atan2(__simd4f a, __simd4f b);

    simd4d simd4d_sin(__simd4d a);
    simd4d simd4d_cos(__simd4d a);
    simd4d simd4d_tan(__simd4d a);
    simd4d simd4d_asin(__simd4d a);
    simd4d simd4d_acos(__simd4d a);
    simd4d simd4d_atan(__simd4d a);
#if 0 // TODO
    simd4d simd4d_exp(__simd4d a);
    simd4d simd4d_log(__simd4d a);
    simd4d simd4d_exp2(__simd4d a);
    simd4d simd4d_log2(__simd4d a);
    simd4d simd4d_pow(__simd4d a, __simd4d b);
#endif
    simd4d simd4d_atan2(__simd4d a, __simd4d b);

    // ------------------------------------------------------------------
    // Common scalar variations
    // ------------------------------------------------------------------

    // simd4i

    static inline simd4i simd4i_add(int a, __simd4i b)
    {
        return simd4i_add(simd4i_set1(a), b);
    }

    static inline simd4i simd4i_add(__simd4i a, int b)
    {
        return simd4i_add(a, simd4i_set1(b));
    }

    static inline simd4i simd4i_sub(int a, __simd4i b)
    {
        return simd4i_sub(simd4i_set1(a), b);
    }

    static inline simd4i simd4i_sub(__simd4i a, int b)
    {
        return simd4i_sub(a, simd4i_set1(b));
    }

    static inline simd4i simd4i_and(int a, __simd4i b)
    {
        return simd4i_and(simd4i_set1(a), b);
    }

    static inline simd4i simd4i_and(__simd4i a, int b)
    {
        return simd4i_and(a, simd4i_set1(b));
    }

    static inline simd4i simd4i_nand(int a, __simd4i b)
    {
        return simd4i_nand(simd4i_set1(a), b);
    }

    static inline simd4i simd4i_nand(__simd4i a, int b)
    {
        return simd4i_nand(a, simd4i_set1(b));
    }

    static inline simd4i simd4i_or(int a, __simd4i b)
    {
        return simd4i_or(simd4i_set1(a), b);
    }

    static inline simd4i simd4i_or(__simd4i a, int b)
    {
        return simd4i_or(a, simd4i_set1(b));
    }

    static inline simd4i simd4i_xor(int a, __simd4i b)
    {
        return simd4i_xor(simd4i_set1(a), b);
    }

    static inline simd4i simd4i_xor(__simd4i a, int b)
    {
        return simd4i_xor(a, simd4i_set1(b));
    }

    static inline simd4i simd4i_compare_eq(__simd4i a, int b)
    {
        return simd4i_compare_eq(a, simd4i_set1(b));
    }

    static inline simd4i simd4i_compare_gt(__simd4i a, int b)
    {
        return simd4i_compare_gt(a, simd4i_set1(b));
    }

    static inline simd4i simd4i_select(__simd4i mask, int a, __simd4i b)
    {
        return simd4i_select(mask, simd4i_set1(a), b);
    }

    static inline simd4i simd4i_select(__simd4i mask, __simd4i a, int b)
    {
        return simd4i_select(mask, a, simd4i_set1(b));
    }

    static inline simd4i simd4i_select(__simd4i mask, int a, int b)
    {
        return simd4i_select(mask, simd4i_set1(a), simd4i_set1(b));
    }

    // simd4f

    static inline simd4f simd4f_add(float a, __simd4f b)
    {
        return simd4f_add(simd4f_set1(a), b);
    }

    static inline simd4f simd4f_add(__simd4f a, float b)
    {
        return simd4f_add(a, simd4f_set1(b));
    }

    static inline simd4f simd4f_sub(float a, __simd4f b)
    {
        return simd4f_sub(simd4f_set1(a), b);
    }

    static inline simd4f simd4f_sub(__simd4f a, float b)
    {
        return simd4f_sub(a, simd4f_set1(b));
    }

    static inline simd4f simd4f_mul(__simd4f a, float b)
    {
        return simd4f_mul(a, simd4f_set1(b));
    }

    static inline simd4f simd4f_mul(float a, __simd4f b)
    {
        return simd4f_mul(simd4f_set1(a), b);
    }

    static inline simd4f simd4f_div(float a, __simd4f b)
    {
        return simd4f_div(simd4f_set1(a), b);
    }

    static inline simd4f simd4f_madd(float a, __simd4f b, __simd4f c)
    {
        return simd4f_madd(simd4f_set1(a), b, c);
    }

    static inline simd4f simd4f_madd(__simd4f a, float b, __simd4f c)
    {
        return simd4f_madd(a, simd4f_set1(b), c);
    }

    static inline simd4f simd4f_madd(__simd4f a, __simd4f b, float c)
    {
        return simd4f_madd(a, b, simd4f_set1(c));
    }

    static inline simd4f simd4f_min(float a, __simd4f b)
    {
        return simd4f_min(simd4f_set1(a), b);
    }

    static inline simd4f simd4f_min(__simd4f a, float b)
    {
        return simd4f_min(a, simd4f_set1(b));
    }

    static inline simd4f simd4f_max(float a, __simd4f b)
    {
        return simd4f_max(simd4f_set1(a), b);
    }

    static inline simd4f simd4f_max(__simd4f a, float b)
    {
        return simd4f_max(a, simd4f_set1(b));
    }

    static inline simd4f simd4f_clamp(__simd4f v, float vmin, float vmax)
    {
        return simd4f_clamp(v, simd4f_set1(vmin), simd4f_set1(vmax));
    }

    static inline simd4f simd4f_compare_neq(__simd4f a, float b)
    {
        return simd4f_compare_neq(a, simd4f_set1(b));
    }

    static inline simd4f simd4f_compare_eq(__simd4f a, float b)
    {
        return simd4f_compare_eq(a, simd4f_set1(b));
    }

    static inline simd4f simd4f_compare_lt(__simd4f a, float b)
    {
        return simd4f_compare_lt(a, simd4f_set1(b));
    }

    static inline simd4f simd4f_compare_le(__simd4f a, float b)
    {
        return simd4f_compare_le(a, simd4f_set1(b));
    }

    static inline simd4f simd4f_compare_gt(__simd4f a, float b)
    {
        return simd4f_compare_gt(a, simd4f_set1(b));
    }

    static inline simd4f simd4f_compare_ge(__simd4f a, float b)
    {
        return simd4f_compare_ge(a, simd4f_set1(b));
    }

    static inline simd4f simd4f_select(__simd4f mask, float a, __simd4f b)
    {
        return simd4f_select(mask, simd4f_set1(a), b);
    }

    static inline simd4f simd4f_select(__simd4f mask, __simd4f a, float b)
    {
        return simd4f_select(mask, a, simd4f_set1(b));
    }

    static inline simd4f simd4f_select(__simd4f mask, float a, float b)
    {
        return simd4f_select(mask, simd4f_set1(a), simd4f_set1(b));
    }

    static inline simd4f simd4f_mod(__simd4f a, __simd4f b)
    {
        simd4f temp = simd4f_floor(simd4f_div(a, b));
        return simd4f_sub(a, simd4f_mul(b, temp));
    }

    static inline simd4f simd4f_sign(__simd4f a)
    {
        const simd4f zero_mask = simd4f_compare_neq(a, simd4f_zero());
        const simd4f sign_bits = simd4f_and(a, simd4f_set1(-0.0f));
        const simd4f signed_one = simd4f_or(sign_bits, simd4f_set1(1.0f));
        return simd4f_and(signed_one, zero_mask);
    }

    static inline simd4f simd4f_radians(__simd4f a)
    {
        static const simd4f s = simd4f_set1(0.01745329251f);
        return simd4f_mul(a, s);
    }

    static inline simd4f simd4f_degrees(__simd4f a)
    {
        static const simd4f s = simd4f_set1(57.2957795131f);
        return simd4f_mul(a, s);
    }

    static inline simd4f simd4f_square(__simd4f a)
    {
        return simd4f_dot4(a, a);
    }

    static inline simd4f simd4f_length(__simd4f a)
    {
        return simd4f_sqrt(simd4f_dot4(a, a));
    }

    static inline simd4f simd4f_normalize(__simd4f a)
    {
        return simd4f_mul(a, simd4f_rsqrt(simd4f_dot4(a, a)));
    }

    // simd4d

    static inline simd4d simd4d_add(double a, __simd4d b)
    {
        return simd4d_add(simd4d_set1(a), b);
    }

    static inline simd4d simd4d_add(__simd4d a, double b)
    {
        return simd4d_add(a, simd4d_set1(b));
    }

    static inline simd4d simd4d_sub(double a, __simd4d b)
    {
        return simd4d_sub(simd4d_set1(a), b);
    }

    static inline simd4d simd4d_sub(__simd4d a, double b)
    {
        return simd4d_sub(a, simd4d_set1(b));
    }

    static inline simd4d simd4d_mul(__simd4d a, double b)
    {
        return simd4d_mul(a, simd4d_set1(b));
    }

    static inline simd4d simd4d_mul(double a, __simd4d b)
    {
        return simd4d_mul(simd4d_set1(a), b);
    }

    static inline simd4d simd4d_div(double a, __simd4d b)
    {
        return simd4d_div(simd4d_set1(a), b);
    }

    static inline simd4d simd4d_madd(double a, __simd4d b, __simd4d c)
    {
        return simd4d_madd(simd4d_set1(a), b, c);
    }

    static inline simd4d simd4d_madd(__simd4d a, double b, __simd4d c)
    {
        return simd4d_madd(a, simd4d_set1(b), c);
    }

    static inline simd4d simd4d_madd(__simd4d a, __simd4d b, double c)
    {
        return simd4d_madd(a, b, simd4d_set1(c));
    }

    static inline simd4d simd4d_min(double a, __simd4d b)
    {
        return simd4d_min(simd4d_set1(a), b);
    }

    static inline simd4d simd4d_min(__simd4d a, double b)
    {
        return simd4d_min(a, simd4d_set1(b));
    }

    static inline simd4d simd4d_max(double a, __simd4d b)
    {
        return simd4d_max(simd4d_set1(a), b);
    }

    static inline simd4d simd4d_max(__simd4d a, double b)
    {
        return simd4d_max(a, simd4d_set1(b));
    }

    static inline simd4d simd4d_clamp(__simd4d v, double vmin, double vmax)
    {
        return simd4d_clamp(v, simd4d_set1(vmin), simd4d_set1(vmax));
    }

    static inline simd4d simd4d_compare_neq(__simd4d a, double b)
    {
        return simd4d_compare_neq(a, simd4d_set1(b));
    }

    static inline simd4d simd4d_compare_eq(__simd4d a, double b)
    {
        return simd4d_compare_eq(a, simd4d_set1(b));
    }

    static inline simd4d simd4d_compare_lt(__simd4d a, double b)
    {
        return simd4d_compare_lt(a, simd4d_set1(b));
    }

    static inline simd4d simd4d_compare_le(__simd4d a, double b)
    {
        return simd4d_compare_le(a, simd4d_set1(b));
    }

    static inline simd4d simd4d_compare_gt(__simd4d a, double b)
    {
        return simd4d_compare_gt(a, simd4d_set1(b));
    }

    static inline simd4d simd4d_compare_ge(__simd4d a, double b)
    {
        return simd4d_compare_ge(a, simd4d_set1(b));
    }

    static inline simd4d simd4d_select(__simd4d mask, double a, __simd4d b)
    {
        return simd4d_select(mask, simd4d_set1(a), b);
    }

    static inline simd4d simd4d_select(__simd4d mask, __simd4d a, double b)
    {
        return simd4d_select(mask, a, simd4d_set1(b));
    }

    static inline simd4d simd4d_select(__simd4d mask, double a, double b)
    {
        return simd4d_select(mask, simd4d_set1(a), simd4d_set1(b));
    }

    static inline simd4d simd4d_mod(__simd4d a, __simd4d b)
    {
        simd4d temp = simd4d_floor(simd4d_div(a, b));
        return simd4d_sub(a, simd4d_mul(b, temp));
    }

    static inline simd4d simd4d_sign(__simd4d a)
    {
        const simd4d zero_mask = simd4d_compare_neq(a, simd4d_zero());
        const simd4d sign_bits = simd4d_and(a, simd4d_set1(-0.0));
        const simd4d signed_one = simd4d_or(sign_bits, simd4d_set1(1.0));
        return simd4d_and(signed_one, zero_mask);
    }

    static inline simd4d simd4d_radians(__simd4d a)
    {
        static const simd4d s = simd4d_set1(0.01745329251);
        return simd4d_mul(a, s);
    }

    static inline simd4d simd4d_degrees(__simd4d a)
    {
        static const simd4d s = simd4d_set1(57.2957795131);
        return simd4d_mul(a, s);
    }

    static inline simd4d simd4d_square(__simd4d a)
    {
        return simd4d_dot4(a, a);
    }

    static inline simd4d simd4d_length(__simd4d a)
    {
        return simd4d_sqrt(simd4d_dot4(a, a));
    }

    static inline simd4d simd4d_normalize(__simd4d a)
    {
        return simd4d_mul(a, simd4d_rsqrt(simd4d_dot4(a, a)));
    }

} // namespace mango

