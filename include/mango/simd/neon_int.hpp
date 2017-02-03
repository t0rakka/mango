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
    // uint8x16
    // -----------------------------------------------------------------

    static inline uint8x16 uint8x16_min(uint8x16 a, uint8x16 b)
    {
        return vminq_u8(a, b);
    }

    static inline uint8x16 uint8x16_max(uint8x16 a, uint8x16 b)
    {
        return vmaxq_u8(a, b);
    }

    // -----------------------------------------------------------------
    // uint16x8
    // -----------------------------------------------------------------

    static inline uint16x8 uint16x8_min(uint16x8 a, uint16x8 b)
    {
        return vminq_u16(a, b);
    }

    static inline uint16x8 uint16x8_max(uint16x8 a, uint16x8 b)
    {
        return vmaxq_u16(a, b);
    }

    // -----------------------------------------------------------------
    // uint32x4
    // -----------------------------------------------------------------

    // shuffle

    template <int x, int y, int z, int w>
    static inline uint32x4 uint32x4_shuffle(uint32x4 v)
    {
        // TODO: optimize
        const uint32 *temp = reinterpret_cast<const uint32 *>(&v);
        return (uint32x4_t) { temp[x], temp[y], temp[z], temp[w] };
    }

    template <>
    inline uint32x4 uint32x4_shuffle<0, 1, 2, 3>(uint32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <int Index>
    static inline uint32x4 uint32x4_set_component(uint32x4 a, uint32 s)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        return vsetq_lane_u32(s, a, Index);
    }

    template <int Index>
    static inline uint32 uint32x4_get_component(uint32x4 a)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        return vgetq_lane_u32(a, Index);
    }

    static inline uint32x4 uint32x4_zero()
    {
        return vdupq_n_u32(0);
    }

    static inline uint32x4 uint32x4_set1(uint32 s)
    {
        return vdupq_n_u32(s);
    }

    static inline uint32x4 uint32x4_set4(uint32 x, uint32 y, uint32 z, uint32 w)
    {
        uint32x4_t temp = { x, y, z, w };
        return temp;
    }

    static inline uint32x4 uint32x4_uload(const uint32* source)
    {
        uint32x4_t temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline void uint32x4_ustore(uint32* dest, uint32x4 a)
    {
        dest[0] = vgetq_lane_u32(a, 0);
        dest[1] = vgetq_lane_u32(a, 1);
        dest[2] = vgetq_lane_u32(a, 2);
        dest[3] = vgetq_lane_u32(a, 3);
    }

    static inline uint32x4 uint32x4_add(uint32x4 a, uint32x4 b)
    {
        return vaddq_u32(a, b);
    }

    static inline uint32x4 uint32x4_sub(uint32x4 a, uint32x4 b)
    {
        return vsubq_u32(a, b);
    }

    // logical

    static inline uint32x4 uint32x4_and(uint32x4 a, uint32x4 b)
    {
        return vandq_u32(a, b);
    }

    static inline uint32x4 uint32x4_nand(uint32x4 a, uint32x4 b)
    {
        return vbicq_u32(b, a);
    }

    static inline uint32x4 uint32x4_or(uint32x4 a, uint32x4 b)
    {
        return vorrq_u32(a, b);
    }

    static inline uint32x4 uint32x4_xor(uint32x4 a, uint32x4 b)
    {
        return veorq_u32(a, b);
    }

    static inline uint32x4 uint32x4_not(uint32x4 a)
    {
        return vmvnq_u32(a);
    }

    // shift

    template <int Count> 
    static inline uint32x4 uint32x4_sll(uint32x4 a)
    {
        return vshlq_n_u32(a, Count);
    }

    template <int Count> 
    static inline uint32x4 uint32x4_srl(uint32x4 a)
    {
        return vshrq_n_u32(a, Count);
    }

    template <int Count> 
    static inline uint32x4 uint32x4_sra(uint32x4 a)
    {
        const int32x4 temp = vshrq_n_s32(vreinterpretq_s32_u32(a), Count);
        return vreinterpretq_u32_s32(temp);
    }

    // compare

    static inline uint32x4 uint32x4_compare_neq(uint32x4 a, uint32x4 b)
    {
        return vmvnq_u32(vceqq_u32(a, b));
    }

    static inline uint32x4 uint32x4_compare_lt(uint32x4 a, uint32x4 b)
    {
        return vcltq_u32(a, b);
    }

    static inline uint32x4 uint32x4_compare_le(uint32x4 a, uint32x4 b)
    {
        return vcleq_u32(a, b);
    }

    static inline uint32x4 uint32x4_compare_ge(uint32x4 a, uint32x4 b)
    {
        return vcgeq_u32(a, b);
    }

    static inline uint32x4 uint32x4_compare_eq(uint32x4 a, uint32x4 b)
    {
        return vceqq_u32(a, b);
    }

    static inline uint32x4 uint32x4_compare_gt(uint32x4 a, uint32x4 b)
    {
        return vcgtq_u32(a, b);
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
    // int8x16
    // -----------------------------------------------------------------

    static inline int8x16 int8x16_min(int8x16 a, int8x16 b)
    {
        return vminq_s8(a, b);
    }

    static inline int8x16 int8x16_max(int8x16 a, int8x16 b)
    {
        return vmaxq_s8(a, b);
    }

    // -----------------------------------------------------------------
    // int16x8
    // -----------------------------------------------------------------

    static inline int16x8 int16x8_min(int16x8 a, int16x8 b)
    {
        return vminq_s16(a, b);
    }

    static inline int16x8 int16x8_max(int16x8 a, int16x8 b)
    {
        return vmaxq_s16(a, b);
    }

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    // shuffle

    template <int x, int y, int z, int w>
    static inline int32x4 int32x4_shuffle(int32x4 v)
    {
        // TODO: optimize
        const int32* temp = reinterpret_cast<const int32 *>(&v);
        return (int32x4_t) { temp[x], temp[y], temp[z], temp[w] };
    }

    template <>
    inline int32x4 int32x4_shuffle<0, 1, 2, 3>(int32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <int Index>
    static inline int32x4 int32x4_set_component(int32x4 a, int32 s)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        return vsetq_lane_s32(s, a, Index);
    }

    template <int Index>
    static inline int32 int32x4_get_component(int32x4 a)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        return vgetq_lane_s32(a, Index);
    }

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

    static inline int32x4 int32x4_uload(const int* source)
    {
        int32x4_t temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline void int32x4_ustore(int* dest, int32x4 a)
    {
        dest[0] = vgetq_lane_s32(a, 0);
        dest[1] = vgetq_lane_s32(a, 1);
        dest[2] = vgetq_lane_s32(a, 2);
        dest[3] = vgetq_lane_s32(a, 3);
    }

    static inline int32x4 int32x4_abs(int32x4 a)
    {
        return vabsq_s32(a);
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

    static inline int32x4 int32x4_not(int32x4 a)
    {
        return vmvnq_s32(a);
    }

    // shift

    template <int Count> 
    static inline int32x4 int32x4_sll(int32x4 a)
    {
        const uint32x4 temp = vshlq_n_u32(vreinterpretq_u32_s32(a), Count);
        return vreinterpretq_s32_u32(temp);
    }

    template <int Count> 
    static inline int32x4 int32x4_srl(int32x4 a)
    {
        const uint32x4 temp = vshrq_n_u32(vreinterpretq_u32_s32(a), Count);
        return vreinterpretq_s32_u32(temp);
    }

    template <int Count> 
    static inline int32x4 int32x4_sra(int32x4 a)
    {
        return vshrq_n_s32(a, Count);
    }

    // compare

    static inline int32x4 int32x4_compare_neq(int32x4 a, int32x4 b)
    {
        return vreinterpretq_s32_u32(vmvnq_u32(vceqq_s32(a, b)));
    }

    static inline int32x4 int32x4_compare_lt(int32x4 a, int32x4 b)
    {
        return vreinterpretq_s32_u32(vcltq_s32(a, b));
    }

    static inline int32x4 int32x4_compare_le(int32x4 a, int32x4 b)
    {
        return vreinterpretq_s32_u32(vcleq_s32(a, b));
    }

    static inline int32x4 int32x4_compare_ge(int32x4 a, int32x4 b)
    {
        return vreinterpretq_s32_u32(vcgeq_s32(a, b));
    }

    static inline int32x4 int32x4_compare_eq(int32x4 a, int32x4 b)
    {
        return vreinterpretq_s32_u32(vceqq_s32(a, b));
    }

    static inline int32x4 int32x4_compare_gt(int32x4 a, int32x4 b)
    {
        return vreinterpretq_s32_u32(vcgtq_s32(a, b));
    }

    static inline int32x4 int32x4_select(int32x4 mask, int32x4 a, int32x4 b)
    {
        return vbslq_s32(vreinterpretq_u32_s32(mask), a, b);
    }

    static inline int32x4 int32x4_min(int32x4 a, int32x4 b)
    {
        return vminq_s32(a, b);
    }

    static inline int32x4 int32x4_max(int32x4 a, int32x4 b)
    {
        return vmaxq_s32(a, b);
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
        const uint8x8_t a = vreinterpret_u8_u32(vdup_n_u32(s));
        const uint16x4_t b = vget_low_u16(vmovl_u8(a));
        return vreinterpretq_s32_u32(vmovl_u16(b));
    }

} // namespace simd
} // namespace mango

#endif // MANGO_SIMD_INT_NEON
