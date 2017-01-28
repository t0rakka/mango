/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

#ifdef MANGO_SIMD_INT_NEON

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // uint32x4
    // -----------------------------------------------------------------

    // set

    static inline uint32x4 uint32x4_set1(uint32 s)
    {
        return vdupq_n_u32(s);
    }

    static inline uint32x4 uint32x4_set4(uint32 x, uint32 y, uint32 z, uint32 w)
    {
        uint32x4_t temp = { x, y, z, w };
        return temp;
    }

    static inline uint32x4 uint32x4_set_x(uint32x4 a, int x)
    {
        return vsetq_lane_u32(x, a, 0);
    }

    static inline uint32x4 uint32x4_set_y(uint32x4 a, int y)
    {
        return vsetq_lane_u32(y, a, 1);
    }

    static inline uint32x4 uint32x4_set_z(uint32x4 a, int z)
    {
        return vsetq_lane_u32(z, a, 2);
    }

    static inline uint32x4 uint32x4_set_w(uint32x4 a, int w)
    {
        return vsetq_lane_u32(w, a, 3);
    }

    // get

    static inline int uint32x4_get_x(uint32x4 a)
    {
        return vgetq_lane_u32(a, 0);
    }

    static inline int uint32x4_get_y(uint32x4 a)
    {
        return vgetq_lane_u32(a, 1);
    }

    static inline int uint32x4_get_z(uint32x4 a)
    {
        return vgetq_lane_u32(a, 2);
    }

    static inline int uint32x4_get_w(uint32x4 a)
    {
        return vgetq_lane_u32(a, 3);
    }

    static inline uint32x4 uint32x4_compare_eq(uint32x4 a, uint32x4 b)
    {
        return vceqq_u32(a, b);
    }

    static inline uint32x4 uint32x4_compare_gt(uint32x4 a, uint32x4 b)
    {
        return vcgeq_u32(a, b);
    }

    static inline uint32x4 uint32x4_select(uint32x4 mask, uint32x4 a, uint32x4 b)
    {
        return vbslq_u32(mask, a, b);
    }

    static inline uint32x4 uint32x4_min(uint32x4 a, uint32x4 b)
    {
        return vminq_u32(a, b);
    }

    static inline uint32x4 uint32x4_max(uint32x4 a, uint32x4 b)
    {
        return vmaxq_u32(a, b);
    }

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    // set

    static inline int32x4 int32x4_zero()
    {
        return vdupq_n_s32(0);
    }

    static inline int32x4 int32x4_set1(int s)
    {
        return vdupq_n_s32(s);
    }

    static inline int32x4 int32x4_set4(int x, int y, int z, int w)
    {
        int32x4_t temp = { x, y, z, w };
        return temp;
    }

    static inline int32x4 int32x4_set_x(int32x4 a, int x)
    {
        return vsetq_lane_s32(x, a, 0);
    }

    static inline int32x4 int32x4_set_y(int32x4 a, int y)
    {
        return vsetq_lane_s32(y, a, 1);
    }

    static inline int32x4 int32x4_set_z(int32x4 a, int z)
    {
        return vsetq_lane_s32(z, a, 2);
    }

    static inline int32x4 int32x4_set_w(int32x4 a, int w)
    {
        return vsetq_lane_s32(w, a, 3);
    }

    // get

    static inline int int32x4_get_x(int32x4 a)
    {
        return vgetq_lane_s32(a, 0);
    }

    static inline int int32x4_get_y(int32x4 a)
    {
        return vgetq_lane_s32(a, 1);
    }

    static inline int int32x4_get_z(int32x4 a)
    {
        return vgetq_lane_s32(a, 2);
    }

    static inline int int32x4_get_w(int32x4 a)
    {
        return vgetq_lane_s32(a, 3);
    }

    static inline int32x4 int32x4_load(const int* source)
    {
        return vld1q_s32(source);
    }

    static inline int32x4 int32x4_uload(const int* source)
    {
        int32x4_t temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline void int32x4_store(int* dest, int32x4 a)
    {
        vst1q_s32(dest, a);
    }

    static inline void int32x4_ustore(int* dest, int32x4 a)
    {
        dest[0] = vgetq_lane_s32(a, 0);
        dest[1] = vgetq_lane_s32(a, 1);
        dest[2] = vgetq_lane_s32(a, 2);
        dest[3] = vgetq_lane_s32(a, 3);
    }

    static inline int32x4 int32x4_neg(int32x4 a)
    {
        return vnegq_s32(a);
    }

    static inline int32x4 int32x4_add(int32x4 a, int32x4 b)
    {
        return vaddq_s32(a, b);
    }

    static inline int32x4 int32x4_sub(int32x4 a, int32x4 b)
    {
        return vsubq_s32(a, b);
    }

    // logical

    static inline int32x4 int32x4_and(int32x4 a, int32x4 b)
    {
        return vandq_s32(a, b);
    }

    static inline int32x4 int32x4_nand(int32x4 a, int32x4 b)
    {
        return vbicq_s32(b, a);
    }

    static inline int32x4 int32x4_or(int32x4 a, int32x4 b)
    {
        return vorrq_s32(a, b);
    }

    static inline int32x4 int32x4_xor(int32x4 a, int32x4 b)
    {
        return veorq_s32(a, b);
    }

    // shift

#ifdef MANGO_COMPILER_CLANG

    // error: argument to '__builtin_neon_vshrq_n_v' must be a constant integer

    #define int32x4_sll(a, b) \
        (int32x4) vreinterpretq_s32_u32(vshlq_n_u32(vreinterpretq_u32_s32(a), b))

    #define int32x4_srl(a, b) \
        (int32x4) vreinterpretq_s32_u32(vshrq_n_u32(vreinterpretq_u32_s32(a), b))

    #define int32x4_sra(a, b) \
        (int32x4) vshrq_n_s32(a, b)

#else

    static inline int32x4 int32x4_sll(int32x4 a, int b)
    {
        return vreinterpretq_s32_u32(vshlq_n_u32(vreinterpretq_u32_s32(a), b));
    }

    static inline int32x4 int32x4_srl(int32x4 a, int b)
    {
        return vreinterpretq_s32_u32(vshrq_n_u32(vreinterpretq_u32_s32(a), b));
    }

    static inline int32x4 int32x4_sra(int32x4 a, int b)
    {
        return vshrq_n_s32(a, b);
    }

#endif

    // compare

    static inline int32x4 int32x4_compare_eq(int32x4 a, int32x4 b)
    {
        return vreinterpretq_s32_u32(vceqq_s32(a, b));
    }

    static inline int32x4 int32x4_compare_gt(int32x4 a, int32x4 b)
    {
        return vreinterpretq_s32_u32(vcgeq_s32(a, b));
    }

    static inline int32x4 int32x4_select(int32x4 mask, int32x4 a, int32x4 b)
    {
        return vbslq_s32(vreinterpretq_u32_s32(mask), a, b);
    }

    static inline uint32 int32x4_get_mask(int32x4 a)
    {
        const int32x4_t mask = { 1, 2, 4, 8 };

        const int32x4_t masked = vandq_s32(a, mask);
        const int32x2_t high = vget_high_s32(masked);
        const int32x2_t low = vget_low_s32(masked);
        const int32x2_t d0 = vorr_s32(high, low);
        const int32x2_t d1 = vpadd_s32(d0, d0);
        return vget_lane_s32(d1, 0);
    }

    static inline int32x4 int32x4_min(int32x4 a, int32x4 b)
    {
        return vminq_s32(a, b);
    }

    static inline int32x4 int32x4_max(int32x4 a, int32x4 b)
    {
        return vmaxq_s32(a, b);
    }

    static inline uint32 int32x4_pack(int32x4 s)
    {
        const uint16x4_t a = vqmovun_s32(s);
        const uint16x8_t b = vcombine_u16(a, a);
        const uint8x8_t c = vqmovn_u16(b);
        const uint32x2_t d = vreinterpret_u32_u8(c);
        return vget_lane_u32(d, 0);
    }

    static inline int32x4 int32x4_unpack(uint32 s)
    {
        const uint32x2_t a = vdup_n_u32(s);
        const uint8x8_t b = vreinterpret_u8_u32(a);
        const uint16x8_t c = vshll_n_u8(b, 1); // shift by one!
        const uint16x4_t c_lo = vget_low_u16(c);
        const uint32x4_t d = vshll_n_u16(c_lo, 1); // another, compiler hangs on valid input of 0
        return vreinterpretq_s32_u32(vshrq_n_u32(d, 2)); // fix the scale here.. (>> 2)
    }

} // namespace simd
} // namespace mango

#endif // MANGO_SIMD_INT_NEON
