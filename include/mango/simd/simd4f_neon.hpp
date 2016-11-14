/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_INCLUDE_SIMD
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

#include <cmath>
#include <algorithm>

namespace mango
{

    // -----------------------------------------------------------------
    // ARM NEON
    // -----------------------------------------------------------------

#ifdef MANGO_ENABLE_FP16

    typedef float16x4_t simd4h;
    typedef const float16x4_t __simd4h;

#else

    // fp16 software emulation
    struct simd4h
    {
        half x, y, z, w;
    };

    typedef const simd4h __simd4h;

#endif

    typedef float32x4_t simd4f;
    typedef const float32x4_t __simd4f;

    typedef int32x4_t simd4i;
    typedef const int32x4_t __simd4i;

    // -----------------------------------------------------------------
    // conversions
    // -----------------------------------------------------------------

    static inline simd4f simd4f_cast(__simd4i s)
    {
        return vreinterpretq_f32_s32(s);
    }

    static inline simd4i simd4i_cast(__simd4f s)
    {
        return vreinterpretq_s32_f32(s);
    }

    static inline simd4f simd4f_convert(__simd4i s)
    {
        return vcvtq_f32_s32(s);
    }

    static inline simd4f simd4f_unsigned_convert(__simd4i s)
    {
        const uint32x4_t u = vreinterpretq_u32_s32(s);
        return vcvtq_f32_u32(u);
    }

    static inline simd4i simd4i_convert(__simd4f s)
    {
        const uint32x4_t temp = vandq_u32((uint32x4_t)s, (uint32x4_t)vdupq_n_f32(-0.0f));
        const uint32x4_t half = (uint32x4_t)vdupq_n_f32(0.5f);
        return vcvtq_s32_f32(vaddq_f32(s, (float32x4_t)vorrq_u32(temp, half)));
    }

    static inline simd4i simd4i_truncate(__simd4f s)
    {
        return vcvtq_s32_f32(s);
    }

    // -----------------------------------------------------------------
    // simd4i
    // -----------------------------------------------------------------

    // set

    static inline simd4i simd4i_set_x(__simd4i a, int x)
    {
        return vsetq_lane_s32(x, a, 0);
    }

    static inline simd4i simd4i_set_y(__simd4i a, int y)
    {
        return vsetq_lane_s32(y, a, 1);
    }

    static inline simd4i simd4i_set_z(__simd4i a, int z)
    {
        return vsetq_lane_s32(z, a, 2);
    }

    static inline simd4i simd4i_set_w(__simd4i a, int w)
    {
        return vsetq_lane_s32(w, a, 3);
    }

    // get

    static inline int simd4i_get_x(__simd4i a)
    {
        return vgetq_lane_s32(a, 0);
    }

    static inline int simd4i_get_y(__simd4i a)
    {
        return vgetq_lane_s32(a, 1);
    }

    static inline int simd4i_get_z(__simd4i a)
    {
        return vgetq_lane_s32(a, 2);
    }

    static inline int simd4i_get_w(__simd4i a)
    {
        return vgetq_lane_s32(a, 3);
    }

    static inline simd4i simd4i_load(const int* source)
    {
        return vld1q_s32(source);
    }

    static inline simd4i simd4i_uload(const int* source)
    {
        simd4i temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline void simd4i_store(int* dest, __simd4i a)
    {
        vst1q_s32(dest, a);
    }

    static inline void simd4i_ustore(int* dest, __simd4i a)
    {
        dest[0] = vgetq_lane_s32(a, 0);
        dest[1] = vgetq_lane_s32(a, 1);
        dest[2] = vgetq_lane_s32(a, 2);
        dest[3] = vgetq_lane_s32(a, 3);
    }

    static inline simd4i simd4i_zero()
    {
        return vdupq_n_s32(0);
    }

    static inline simd4i simd4i_set1(int s)
    {
        return vdupq_n_s32(s);
    }

    static inline simd4i simd4i_set4(int x, int y, int z, int w)
    {
        simd4i temp = { x, y, z, w };
        return temp;
    }

    static inline simd4i simd4i_neg(__simd4i a)
    {
        return vnegq_s32(a);
    }

    static inline simd4i simd4i_add(__simd4i a, __simd4i b)
    {
        return vaddq_s32(a, b);
    }

    static inline simd4i simd4i_sub(__simd4i a, __simd4i b)
    {
        return vsubq_s32(a, b);
    }

    // logical

    static inline simd4i simd4i_and(__simd4i a, __simd4i b)
    {
        return (simd4i) vandq_s32(a, b);
    }

    static inline simd4i simd4i_nand(__simd4i a, __simd4i b)
    {
        return (simd4i) vbicq_s32(b, a);
    }

    static inline simd4i simd4i_or(__simd4i a, __simd4i b)
    {
        return (simd4i) vorrq_s32(a, b);
    }

    static inline simd4i simd4i_xor(__simd4i a, __simd4i b)
    {
        return (simd4i) veorq_u32((uint32x4_t)a, (uint32x4_t)b);
    }

    // shift

#ifdef MANGO_COMPILER_CLANG

	#define simd4i_sll(a, b) \
        ((simd4i) vshlq_n_u32((uint32x4_t)a, b))

	#define simd4i_srl(a, b) \
        ((simd4i) vshrq_n_u32((uint32x4_t)a, b))

	#define simd4i_sra(a, b) \
        vshrq_n_s32(a, b)

#else

    static inline simd4i simd4i_sll(__simd4i a, int b)
    {
        return (simd4i) vshlq_n_u32((uint32x4_t)a, b);
    }

    static inline simd4i simd4i_srl(__simd4i a, int b)
    {
        return (simd4i) vshrq_n_u32((uint32x4_t)a, b);
    }

    static inline simd4i simd4i_sra(__simd4i a, int b)
    {
        return vshrq_n_s32(a, b);
    }

#endif

    // compare

    static inline simd4i simd4i_compare_eq(__simd4i a, __simd4i b)
    {
        return (simd4i) vceqq_s32(a, b);
    }

    static inline simd4i simd4i_compare_gt(__simd4i a, __simd4i b)
    {
        return (simd4i) vcgeq_s32(a, b);
    }

    static inline simd4i simd4i_select(__simd4i mask, __simd4i a, __simd4i b)
    {
        return vbslq_s32((uint32x4_t)mask, a, b);
    }

    static inline uint32 simd4i_get_mask(__simd4i a)
    {
        const int32x4_t mask = { 1, 2, 4, 8 };

        const int32x4_t masked = vandq_s32(a, mask);
        const int32x2_t high = vget_high_s32(masked);
        const int32x2_t low = vget_low_s32(masked);
        const int32x2_t d0 = vorr_s32(high, low);
        const int32x2_t d1 = vpadd_s32(d0, d0);
        return vget_lane_s32(d1, 0);
    }

    static inline uint32 simd4i_pack(__simd4i s)
    {
        const uint16x4_t a = vqmovun_s32(s);
        const uint16x8_t b = vcombine_u16(a, a);
        const uint8x8_t c = vqmovn_u16(b);
        const uint32x2_t d = vreinterpret_u32_u8(c);
        return vget_lane_u32(d, 0);
    }

    static inline simd4i simd4i_unpack(uint32 s)
    {
        const uint32x2_t a = vdup_n_u32(s);
        const uint8x8_t b = vreinterpret_u8_u32(a);
        const uint16x8_t c = vshll_n_u8(b, 1); // shift by one!
        const uint16x4_t c_lo = vget_low_u16(c);
        const uint32x4_t d = vshll_n_u16(c_lo, 1); // another, compiler hangs on valid input of 0
        return vreinterpretq_s32_u32(vshrq_n_u32(d, 2)); // fix the scale here.. (>> 2)
    }

    // -----------------------------------------------------------------
    // simd4f
    // -----------------------------------------------------------------

    // shuffle

#ifdef MANGO_COMPILER_CLANG

    template <int x, int y, int z, int w>
    inline simd4f simd4f_shuffle(__simd4f v)
    {
        return __builtin_shufflevector(v, v, x, y, z, w);
    }

#else

    template <int x, int y, int z, int w>
    inline simd4f simd4f_shuffle(__simd4f v)
    {
#if __GNUC__ >= 5
        return (float32x4_t) __builtin_shuffle(v, (uint32x4_t) {x, y, z, w});
#else
        return (simd4f) { v[x], v[y], v[z], v[w] };
#endif
    }

#endif

    //
    // gcc 5 __builtin_shuffle(reg, low.x, low.y, high.z, high.w)
    // legend: generated instruction (-- means generic tbl load)
    // x-axis: low, y-axis: high
    //
    //     xx   xy   xz   xw   yx   yy   yz   yw   zx   zy   zz   zw   wx   wy   wz   ww
    // xx  dup  -    -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // xy  -    -    -    -    -    -    -    -    -    -    -    ext  -    -    -    -
    // xz  -    -    uzp1 -    -    -    -    -    -    -    -    -    -    -    -    -
    // xw  -    -    -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // yx  -    -    -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // yy  zip1 -    -    -    -    dup  -    -    -    -    -    -    -    -    -    -
    // yz  -    -    -    -    -    -    -    -    -    -    -    -    ext  -    -    -
    // yw  -    -    -    -    -    -    -    uzp2 -    -    -    -    -    -    -    -
    // zx  -    -    -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // zy  -    -    -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // zz  trn1 -    -    -    -    -    -    -    -    -    dup  -    -    -    -    -
    // zw  -    mov  -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // wx  -    -    -    -    -    -    ext  -    -    -    -    -    -    -    -    -
    // wy  -    -    -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // wz  -    -    -    -    rev64-    -    -    -    -    -    -    -    -    -    -
    // ww  -    -    -    -    -    trn2 -    -    -    -    zip2 -    -    -    -    dup

    template <>
    inline simd4f simd4f_shuffle<0, 1, 2, 3>(__simd4f v)
    {
        // .xyzw
        return v;
    }

    // indexed accessor

    template <int Index>
    static inline simd4f simd4f_set_component(__simd4f a, float s)
    {
        return vsetq_lane_f32(s, a, Index);
    }

    template <int Index>
    static inline float simd4f_get_component(__simd4f a)
    {
        return vgetq_lane_f32(a, Index);
    }

    // set

    static inline simd4f simd4f_set_x(__simd4f a, float x)
    {
        return vsetq_lane_f32(x, a, 0);
    }

    static inline simd4f simd4f_set_y(__simd4f a, float y)
    {
        return vsetq_lane_f32(y, a, 1);
    }

    static inline simd4f simd4f_set_z(__simd4f a, float z)
    {
        return vsetq_lane_f32(z, a, 2);
    }

    static inline simd4f simd4f_set_w(__simd4f a, float w)
    {
        return vsetq_lane_f32(w, a, 3);
    }

    // get

    static inline float simd4f_get_x(__simd4f a)
    {
        return vgetq_lane_f32(a, 0);
    }

    static inline float simd4f_get_y(__simd4f a)
    {
        return vgetq_lane_f32(a, 1);
    }

    static inline float simd4f_get_z(__simd4f a)
    {
        return vgetq_lane_f32(a, 2);
    }

    static inline float simd4f_get_w(__simd4f a)
    {
        return vgetq_lane_f32(a, 3);
    }

    static inline simd4f simd4f_splat_x(__simd4f a)
    {
        const float32x2_t xy = vget_low_f32(a);
        return vdupq_lane_f32(xy, 0);
    }
    
    static inline simd4f simd4f_splat_y(__simd4f a)
    {
        const float32x2_t xy = vget_low_f32(a);
        return vdupq_lane_f32(xy, 1);
    }
    
    static inline simd4f simd4f_splat_z(__simd4f a)
    {
        const float32x2_t zw = vget_high_f32(a);
        return vdupq_lane_f32(zw, 0);
    }
    
    static inline simd4f simd4f_splat_w(__simd4f a)
    {
        const float32x2_t zw = vget_high_f32(a);
        return vdupq_lane_f32(zw, 1);
    }

    static inline simd4f simd4f_zero()
    {
        return vdupq_n_f32(0.0f);
    }

    static inline simd4f simd4f_set1(float s)
    {
        return vdupq_n_f32(s);
    }

    static inline simd4f simd4f_set4(float x, float y, float z, float w)
    {
        simd4f temp = { x, y, z, w };
        return temp;
    }

    static inline simd4f simd4f_load(const float* source)
    {
        return vld1q_f32(source);
    }

    static inline simd4f simd4f_uload(const float* source)
    {
        simd4f temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline void simd4f_store(float* dest, __simd4f a)
    {
        vst1q_f32(dest, a);
    }

    static inline void simd4f_ustore(float* dest, __simd4f a)
    {
        dest[0] = vgetq_lane_f32(a, 0);
        dest[1] = vgetq_lane_f32(a, 1);
        dest[2] = vgetq_lane_f32(a, 2);
        dest[3] = vgetq_lane_f32(a, 3);
    }

    static inline simd4f simd4f_movelh(__simd4f a, __simd4f b)
    {
        return vcombine_f32(vget_low_f32(a), vget_low_f32(b));
    }

    static inline simd4f simd4f_movehl(__simd4f a, __simd4f b)
    {
        return vcombine_f32(vget_high_f32(b), vget_high_f32(a));
    }

    static inline simd4f simd4f_unpackhi(__simd4f a, __simd4f b)
    {
        float32x4x2_t v = vzipq_f32(a, b);
        return v.val[1];
    }

    static inline simd4f simd4f_unpacklo(__simd4f a, __simd4f b)
    {
        float32x4x2_t v = vzipq_f32(a, b);
        return v.val[0];
    }

    // logical

    static inline simd4f simd4f_and(__simd4f a, __simd4f b)
    {
        return simd4f_cast(simd4i_and(simd4i_cast(a), simd4i_cast(b)));
    }

    static inline simd4f simd4f_nand(__simd4f a, __simd4f b)
    {
        return simd4f_cast(simd4i_nand(simd4i_cast(a), simd4i_cast(b)));
    }

    static inline simd4f simd4f_or(__simd4f a, __simd4f b)
    {
        return simd4f_cast(simd4i_or(simd4i_cast(a), simd4i_cast(b)));
    }

    static inline simd4f simd4f_xor(__simd4f a, __simd4f b)
    {
        return simd4f_cast(simd4i_xor(simd4i_cast(a), simd4i_cast(b)));
    }

    static inline simd4f simd4f_min(__simd4f a, __simd4f b)
    {
        return vminq_f32(a, b);
    }

    static inline simd4f simd4f_max(__simd4f a, __simd4f b)
    {
        return vmaxq_f32(a, b);
    }

    static inline simd4f simd4f_clamp(__simd4f v, __simd4f vmin, __simd4f vmax)
    {
        return vminq_f32(vmax, vmaxq_f32(vmin, v));
    }

    static inline simd4f simd4f_abs(__simd4f a)
    {
        return vabsq_f32(a);
    }

    static inline simd4f simd4f_neg(__simd4f a)
    {
        return vnegq_f32(a);
    }

    static inline simd4f simd4f_add(__simd4f a, __simd4f b)
    {
        return vaddq_f32(a, b);
    }

    static inline simd4f simd4f_sub(__simd4f a, __simd4f b)
    {
        return vsubq_f32(a, b);
    }

    static inline simd4f simd4f_mul(__simd4f a, __simd4f b)
    {
        return vmulq_f32(a, b);
    }

#ifdef __aarch64__

    static inline simd4f simd4f_div(__simd4f a, __simd4f b)
    {
        return vdivq_f32(a, b);
    }

    static inline simd4f simd4f_div(__simd4f a, float b)
    {
        simd4f s = vdupq_n_f32(b);
        return vdivq_f32(a, s);
    }

#else

    static inline simd4f simd4f_div(__simd4f a, __simd4f b)
    {
        simd4f n = vrecpeq_f32(b);
        n = vmulq_f32(vrecpsq_f32(n, b), n);
        n = vmulq_f32(vrecpsq_f32(n, b), n);
        return vmulq_f32(a, n);
    }

    static inline simd4f simd4f_div(__simd4f a, float b)
    {
        simd4f s = vdupq_n_f32(b);
        simd4f n = vrecpeq_f32(s);
        n = vmulq_f32(vrecpsq_f32(n, s), n);
        n = vmulq_f32(vrecpsq_f32(n, s), n);
        return vmulq_f32(a, n);
    }

#endif

    static inline simd4f simd4f_madd(__simd4f a, __simd4f b, __simd4f c)
    {
        return vmlaq_f32(a, b, c);
    }

    static inline simd4f simd4f_msub(__simd4f a, __simd4f b, __simd4f c)
    {
        return vmlsq_f32(a, b, c);
    }

    static inline simd4f simd4f_fast_reciprocal(__simd4f a)
    {
        simd4f n = vrecpeq_f32(a);
        n = vmulq_f32(vrecpsq_f32(n, a), n);
        return n;
    }

    static inline simd4f simd4f_fast_rsqrt(__simd4f a)
    {
        simd4f n = vrsqrteq_f32(a);
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        return n;
    }

    static inline simd4f simd4f_fast_sqrt(__simd4f a)
    {
        simd4f n = vrsqrteq_f32(a);
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        return vmulq_f32(a, n);
    }

    static inline simd4f simd4f_reciprocal(__simd4f a)
    {
        simd4f n = vrecpeq_f32(a);
        n = vmulq_f32(vrecpsq_f32(n, a), n);
        n = vmulq_f32(vrecpsq_f32(n, a), n);
        return n;
    }

    static inline simd4f simd4f_rsqrt(__simd4f a)
    {
        simd4f n = vrsqrteq_f32(a);
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        return n;
    }

    static inline simd4f simd4f_sqrt(__simd4f a)
    {
        simd4f n = vrsqrteq_f32(a);
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        return vmulq_f32(a, n);
    }

    static inline simd4f simd4f_dot3(__simd4f a, __simd4f b)
    {
        const simd4f s = vmulq_f32(a, b);
        const float32x2_t xy = vget_low_f32(s);
        const float32x2_t zw = vget_high_f32(s);
        return vdupq_lane_f32(vadd_f32(vpadd_f32(xy, xy), zw), 0);
    }

    static inline simd4f simd4f_dot4(__simd4f a, __simd4f b)
    {
        simd4f m = vmulq_f32(a, b);
        float32x2_t s = vpadd_f32(vget_low_f32(m), vget_high_f32(m));
        s = vpadd_f32(s, s);
        return vdupq_lane_f32(s, 0);
    }

    static inline simd4f simd4f_cross3(__simd4f a, __simd4f b)
    {
        simd4f c = vmulq_f32(a, simd4f_shuffle<1, 2, 0, 3>(b));
        c = vmlsq_f32(c, b, simd4f_shuffle<1, 2, 0, 3>(a));
        return simd4f_shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline simd4f simd4f_compare_neq(__simd4f a, __simd4f b)
    {
        return (simd4f) vmvnq_u32(vceqq_f32(a, b));
    }

    static inline simd4f simd4f_compare_eq(__simd4f a, __simd4f b)
    {
        return (simd4f) vceqq_f32(a, b);
    }

    static inline simd4f simd4f_compare_lt(__simd4f a, __simd4f b)
    {
        return (simd4f) vcltq_f32(a, b);
    }

    static inline simd4f simd4f_compare_le(__simd4f a, __simd4f b)
    {
        return (simd4f) vcleq_f32(a, b);
    }

    static inline simd4f simd4f_compare_gt(__simd4f a, __simd4f b)
    {
        return (simd4f) vcgtq_f32(a, b);
    }

    static inline simd4f simd4f_compare_ge(__simd4f a, __simd4f b)
    {
        return (simd4f) vcgeq_f32(a, b);
    }

    static inline simd4f simd4f_select(__simd4f mask, __simd4f a, __simd4f b)
    {
        return vbslq_f32((uint32x4_t)mask, a, b);
    }

    // rounding

#if __ARM_ARCH >= 8 && !defined(MANGO_COMPILER_CLANG)

    // Disabled with clang until supported in NDK

    static inline simd4f simd4f_round(__simd4f s)
    {
        return vrndqa_f32(s);
    }

    static inline simd4f simd4f_trunc(__simd4f s)
    {
        return vrndq_f32(s);
    }

    static inline simd4f simd4f_floor(__simd4f s)
    {
        return vrndqm_f32(s);
    }

    static inline simd4f simd4f_ceil(__simd4f s)
    {
        return vrndqp_f32(s);
    }

#else

    static inline simd4f simd4f_round(__simd4f s)
    {
        const float32x4_t magic = vdupq_n_f32(12582912.0f); // 1.5 * (1 << 23)
        return vsubq_f32(vaddq_f32(s, magic), magic);
    }

    static inline simd4f simd4f_trunc(__simd4f s)
    {
        const int32x4_t truncated = vcvtq_s32_f32(s);
        return vcvtq_f32_s32(truncated);
    }

    static inline simd4f simd4f_floor(__simd4f s)
    {
        const simd4f temp = simd4f_round(s);
        const uint32x4_t mask = vcltq_f32(s, temp);
        const uint32x4_t one = vdupq_n_u32(0x3f800000);
        return vsubq_f32(temp, (simd4f) vandq_u32(mask, one));
    }

    static inline simd4f simd4f_ceil(__simd4f s)
    {
        const simd4f temp = simd4f_round(s);
        const uint32x4_t mask = vcgtq_f32(s, temp);
        const uint32x4_t one = vdupq_n_u32(0x3f800000);
        return vaddq_f32(temp, (simd4f) vandq_u32(mask, one));
    }

#endif

    static inline simd4f simd4f_fract(__simd4f s)
    {
        return simd4f_sub(s, simd4f_floor(s));
    }

    // -----------------------------------------------------------------
    // float <-> half conversions
    // -----------------------------------------------------------------

#ifdef MANGO_ENABLE_FP16

    static inline simd4f simd4f_convert(__simd4h s)
    {
        return vcvt_f32_f16(s);
    }

    static inline simd4h simd4h_convert(__simd4f s)
    {
        return vcvt_f16_f32(s);
    }

#else

    static inline simd4f simd4f_convert(__simd4h s)
    {
        float x = s.x;
        float y = s.y;
        float z = s.z;
        float w = s.w;
        return simd4f_set4(x, y, z, w);
    }

    static inline simd4h simd4h_convert(__simd4f s)
    {
        simd4h v;
        v.x = simd4f_get_x(s);
        v.y = simd4f_get_y(s);
        v.z = simd4f_get_z(s);
        v.w = simd4f_get_w(s);
        return v;
    }

#endif

    // -----------------------------------------------------------------
    // simd4f_matrix
    // -----------------------------------------------------------------

    static inline void simd4f_matrix_set_scale(simd4f* m, float s)
    {
        const simd4f zero = simd4f_zero();
        const float one = 1.0f;
        m[0] = vsetq_lane_f32(s, zero, 0);
        m[1] = vsetq_lane_f32(s, zero, 1);
        m[2] = vsetq_lane_f32(s, zero, 2);
        m[3] = vsetq_lane_f32(one, zero, 3);
    }

    static inline void simd4f_matrix_set_scale(simd4f* m, float x, float y, float z)
    {
        const simd4f zero = simd4f_zero();
        const float one = 1.0f;
        m[0] = vsetq_lane_f32(x, zero, 0);
        m[1] = vsetq_lane_f32(y, zero, 1);
        m[2] = vsetq_lane_f32(z, zero, 2);
        m[3] = vsetq_lane_f32(one, zero, 3);
    }

    static inline void simd4f_matrix_set_translate(simd4f* m, float x, float y, float z)
    {
        const simd4f zero = simd4f_zero();
        const float one = 1.0f;
        m[0] = vsetq_lane_f32(one, zero, 0);
        m[1] = vsetq_lane_f32(one, zero, 1);
        m[2] = vsetq_lane_f32(one, zero, 2);
        m[3] = simd4f_set4(x, y, z, one);
    }

    static inline void simd4f_matrix_scale(simd4f* m, float s)
    {
        const simd4f v = simd4f_set4(s, s, s, 1.0f);
        m[0] = simd4f_mul(m[0], v);
        m[1] = simd4f_mul(m[1], v);
        m[2] = simd4f_mul(m[2], v);
        m[3] = simd4f_mul(m[3], v);
    }

    static inline void simd4f_matrix_scale(simd4f* m, float x, float y, float z)
    {
        const simd4f v = simd4f_set4(x, y, z, 1.0f);
        m[0] = simd4f_mul(m[0], v);
        m[1] = simd4f_mul(m[1], v);
        m[2] = simd4f_mul(m[2], v);
        m[3] = simd4f_mul(m[3], v);
    }

    static inline void simd4f_matrix_translate(simd4f* m, float x, float y, float z)
    {
        const simd4f v = simd4f_set4(x, y, z, 0.0f);
        m[0] = simd4f_madd(m[0], simd4f_shuffle<3, 3, 3, 3>(m[0]), v);
        m[1] = simd4f_madd(m[1], simd4f_shuffle<3, 3, 3, 3>(m[1]), v);
        m[2] = simd4f_madd(m[2], simd4f_shuffle<3, 3, 3, 3>(m[2]), v);
        m[3] = simd4f_madd(m[3], simd4f_shuffle<3, 3, 3, 3>(m[3]), v);
    }

    static inline void simd4f_matrix_transpose(simd4f* result, const simd4f* m)
    {
#if 1
        const float32_t* src = reinterpret_cast<const float32_t*>(m);
        float32_t* dest = reinterpret_cast<float32_t*>(result);
        float32x4x4_t temp = vld4q_f32(src);
        vst1q_f32(dest + 0, temp.val[0]);
        vst1q_f32(dest + 4, temp.val[1]);
        vst1q_f32(dest + 8, temp.val[2]);
        vst1q_f32(dest + 12, temp.val[3]);
#else
        asm volatile (
            "vldmia     %1, { q0-q3 }   \n\t"
            "vtrn.32    q0, q1          \n\t"
            "vtrn.32    q2, q3          \n\t"
            "vswp       d1, d4          \n\t"
            "vswp       d3, d6          \n\t"
            "vstmia     %0, { q0-q3 }   \n\t"
            :
            : "r" (result), "r" (m)
            : "q0", "q1", "q2","q3", "memory"
        );
#endif
    }

    static inline void simd4f_matrix_inverse(simd4f* result, const simd4f* m)
    {
        float32x2_t low0 = vget_low_f32(m[0]);
        float32x2_t low1 = vget_low_f32(m[1]);
        float32x2_t low2 = vget_low_f32(m[2]);
        float32x2_t low3 = vget_low_f32(m[3]);
        float32x2_t high0 = vget_high_f32(m[0]);
        float32x2_t high1 = vget_high_f32(m[1]);
        float32x2_t high2 = vget_high_f32(m[2]);
        float32x2_t high3 = vget_high_f32(m[3]);

        float32x2x2_t n0 = vzip_f32(low0, low1);   // x0, x1, y0, y1
        float32x2x2_t n1 = vzip_f32(high0, high1); // z0, z1, w0, w1
        float32x2x2_t n2 = vzip_f32(low2, low3);   // x2, x3, y2, y3
        float32x2x2_t n3 = vzip_f32(high2, high3); // z2, z3, w2, w3

        float32x4_t row0 = vcombine_f32(n0.val[0], n2.val[0]); // x0, x1, x2, x3
        float32x4_t row1 = vcombine_f32(n2.val[1], n0.val[1]); // y2, y3, y0, y1
        float32x4_t row2 = vcombine_f32(n1.val[0], n3.val[0]); // z0, z1, z2, z3
       	float32x4_t row3 = vcombine_f32(n3.val[1], n1.val[1]); // w2, w3, w0, w1

        float32x4_t temp;
        float32x4_t res0;
        float32x4_t res1;
        float32x4_t res2;
        float32x4_t res3;

        temp = vmulq_f32(row2, row3);
        temp = simd4f_shuffle<1, 0, 3, 2>(temp);
        res0 = vmulq_f32(row1, temp);
        res1 = vmulq_f32(row0, temp);
        temp = simd4f_shuffle<2, 3, 0, 1>(temp);
        res0 = vsubq_f32(vmulq_f32(row1, temp), res0);
        res1 = vsubq_f32(vmulq_f32(row0, temp), res1);
        res1 = simd4f_shuffle<2, 3, 0, 1>(res1);
        temp = vmulq_f32(row1, row2);
        temp = simd4f_shuffle<1, 0, 3, 2>(temp);
        res0 = vmlaq_f32(res0, row3, temp);
        res3 = vmulq_f32(row0, temp);
        temp = simd4f_shuffle<2, 3, 0, 1>(temp);
        res0 = vmlsq_f32(res0, row3, temp);
        res3 = vsubq_f32(vmulq_f32(row0, temp), res3);
        res3 = simd4f_shuffle<2, 3, 0, 1>(res3);
        temp = vmulq_f32(simd4f_shuffle<2, 3, 0, 1>(row1), row3);
        temp = simd4f_shuffle<1, 0, 3, 2>(temp);
        row2 = simd4f_shuffle<2, 3, 0, 1>(row2);
        res0 = vmlaq_f32(res0, row2, temp);
        res2 = vmulq_f32(row0, temp);
        temp = simd4f_shuffle<2, 3, 0, 1>(temp);
        res0 = vmlsq_f32(res0, row2, temp);
        res2 = vsubq_f32(vmulq_f32(row0, temp), res2);
        res2 = simd4f_shuffle<2, 3, 0, 1>(res2);
        temp = vmulq_f32(row0, row1);
        temp = simd4f_shuffle<1, 0, 3, 2>(temp);
        res2 = vmlaq_f32(res2, row3, temp);
        res3 = vsubq_f32(vmulq_f32(row2, temp), res3);
        temp = simd4f_shuffle<2, 3, 0, 1>(temp);
        res2 = vsubq_f32(vmulq_f32(row3, temp), res2);
        res3 = vmlsq_f32(res3, row2, temp);
        temp = vmulq_f32(row0, row3);
        temp = simd4f_shuffle<1, 0, 3,2>(temp);
        res1 = vmlsq_f32(res1, row2, temp);
        res2 = vmlaq_f32(res2, row1, temp);
        temp = simd4f_shuffle<2, 3, 0, 1>(temp);
        res1 = vmlaq_f32(res1, row2, temp);
        res2 = vmlsq_f32(res2, row1, temp);
        temp = vmulq_f32(row0, row2);
        temp = simd4f_shuffle<1, 0, 3, 2>(temp);
        res1 = vmlaq_f32(res1, row3, temp);
        res3 = vmlsq_f32(res3, row1, temp);
        temp = simd4f_shuffle<2, 3, 0, 1>(temp);
        res1 = vmlsq_f32(res1, row3, temp);
        res3 = vmlaq_f32(res3, row1, temp);

        float32x4_t det = vmulq_f32(row0, res0);
        det = vaddq_f32(det, simd4f_shuffle<1, 0, 3, 2>(det));
        det = vaddq_f32(det, simd4f_set1(simd4f_get_z(det)));
        temp = vdupq_n_f32(1.0f / vgetq_lane_f32(det, 0));

        result[0] = vmulq_f32(res0, temp);
        result[1] = vmulq_f32(res1, temp);
        result[2] = vmulq_f32(res2, temp);
        result[3] = vmulq_f32(res3, temp);
    }

    static inline void simd4f_matrix_inverse_transpose(simd4f* result, const simd4f* m)
    {
        simd4f temp[4];
        simd4f_matrix_inverse(temp, m);
        simd4f_matrix_transpose(result, temp);
    }

    static inline simd4f simd4f_vector_matrix_multiply(__simd4f v, const simd4f* m)
    {
#if 1
        const simd4f x = vdupq_n_f32(vgetq_lane_f32(v, 0));
        const simd4f y = vdupq_n_f32(vgetq_lane_f32(v, 1));
        const simd4f z = vdupq_n_f32(vgetq_lane_f32(v, 2));
        const simd4f w = vdupq_n_f32(vgetq_lane_f32(v, 3));
        simd4f temp = vmulq_f32(x, m[0]);
        temp = vmlaq_f32(temp, y, m[1]);
        temp = vmlaq_f32(temp, z, m[2]);
        temp = vmlaq_f32(temp, w, m[3]);
        return temp;
#else
        simd4f temp;
        asm volatile (
            "vldmia    %2, { q8-q11 }    \n\t"
            "vmul.f32  %q0, q8, %e1[0]   \n\t"
            "vmla.f32  %q0, q9, %e1[1]   \n\t"
            "vmla.f32  %q0, q10, %f1[0]  \n\t"
            "vmla.f32  %q0, q11, %f1[1]  \n\t"
            : "=w" (temp)
            : "x" (v), "r" (m)
            : "memory", "q8", "q9", "q10", "q11"
        );
        return temp;
#endif
    }

    static inline void simd4f_matrix_matrix_multiply(simd4f* result, const simd4f* a, const simd4f* b)
    {
#if 1
        result[0] = simd4f_vector_matrix_multiply(a[0], b);
        result[1] = simd4f_vector_matrix_multiply(a[1], b);
        result[2] = simd4f_vector_matrix_multiply(a[2], b);
        result[3] = simd4f_vector_matrix_multiply(a[3], b);
#else
        asm volatile (
            "vldmia    %1, { q0-q3 }    \n\t"
            "vldmia    %2, { q8-q11 }   \n\t"
            "vmul.f32  q12, q8, d0[0]   \n\t"
            "vmul.f32  q13, q8, d2[0]   \n\t"
            "vmul.f32  q14, q8, d4[0]   \n\t"
            "vmul.f32  q15, q8, d6[0]   \n\t"
            "vmla.f32  q12, q9, d0[1]   \n\t"
            "vmla.f32  q13, q9, d2[1]   \n\t"
            "vmla.f32  q14, q9, d4[1]   \n\t"
            "vmla.f32  q15, q9, d6[1]   \n\t"
            "vmla.f32  q12, q10, d1[0]  \n\t"
            "vmla.f32  q13, q10, d3[0]  \n\t"
            "vmla.f32  q14, q10, d5[0]  \n\t"
            "vmla.f32  q15, q10, d7[0]  \n\t"
            "vmla.f32  q12, q11, d1[1]  \n\t"
            "vmla.f32  q13, q11, d3[1]  \n\t"
            "vmla.f32  q14, q11, d5[1]  \n\t"
            "vmla.f32  q15, q11, d7[1]  \n\t"
            "vstmia    %0, { q12-q15 }  \n\t"
            :
            : "r" (result), "r" (a), "r" (b)
            : "memory", "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
        );
#endif
    }

} // namespace mango
