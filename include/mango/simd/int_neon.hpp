/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_SIMD_INT
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

    // -----------------------------------------------------------------
    // simd4i
    // -----------------------------------------------------------------

    // conversion

    static inline simd4i simd4i_cast(__simd4f s)
    {
        return vreinterpretq_s32_f32(s);
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
