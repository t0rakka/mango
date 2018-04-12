/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // uint8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint8x16 set_component(uint8x16 a, uint8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return vsetq_lane_u8(s, a, Index);
    }

    template <unsigned int Index>
    static inline uint8 get_component(uint8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return vgetq_lane_u8(a, Index);
    }

    static inline uint8x16 uint8x16_zero()
    {
        return vdupq_n_u8(0);
    }

    static inline uint8x16 uint8x16_set1(uint8 s)
    {
        return vdupq_n_u8(s);
    }

    static inline uint8x16 uint8x16_set16(
        uint8 s0, uint8 s1, uint8 s2, uint8 s3, uint8 s4, uint8 s5, uint8 s6, uint8 s7,
        uint8 s8, uint8 s9, uint8 s10, uint8 s11, uint8 s12, uint8 s13, uint8 s14, uint8 s15)
    {
        uint8x16_t temp = { s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15 };
        return temp;
    }

    static inline uint8x16 uint8x16_load_low(const uint8* source)
    {
        const uint8x8_t low = vld1_u8(source);
        const uint8x8_t high = vdup_n_u8(0);
        return vcombine_u8(low, high);
    }

    static inline void uint8x16_store_low(uint8* dest, uint8x16 a)
    {
        vst1_u8(dest, vget_low_u8(a));
    }

    static inline uint8x16 unpacklo(uint8x16 a, uint8x16 b)
    {
	    const uint8x8x2_t temp = vzip_u8(vget_low_u8(a), vget_low_u8(b));
	    return vcombine_u8(temp.val[0], temp.val[1]);
    }

    static inline uint8x16 unpackhi(uint8x16 a, uint8x16 b)
    {
	    const uint8x8x2_t temp = vzip_u8(vget_high_u8(a), vget_high_u8(b));
	    return vcombine_u8(temp.val[0], temp.val[1]);
    }

    static inline uint8x16 add(uint8x16 a, uint8x16 b)
    {
        return vaddq_u8(a, b);
    }

    static inline uint8x16 sub(uint8x16 a, uint8x16 b)
    {
        return vsubq_u8(a, b);
    }

    // bitwise

    static inline uint8x16 bitwise_nand(uint8x16 a, uint8x16 b)
    {
        return vbicq_u8(b, a);
    }

    static inline uint8x16 bitwise_and(uint8x16 a, uint8x16 b)
    {
        return vandq_u8(a, b);
    }

    static inline uint8x16 bitwise_or(uint8x16 a, uint8x16 b)
    {
        return vorrq_u8(a, b);
    }

    static inline uint8x16 bitwise_xor(uint8x16 a, uint8x16 b)
    {
        return veorq_u8(a, b);
    }

    static inline uint8x16 bitwise_not(uint8x16 a)
    {
        return vmvnq_u8(a);
    }

    // saturated

    static inline uint8x16 adds(uint8x16 a, uint8x16 b)
    {
        return vqaddq_u8(a, b);
    }

    static inline uint8x16 subs(uint8x16 a, uint8x16 b)
    {
        return vqsubq_u8(a, b);
    }

    // compare

    static inline mask8x16 compare_eq(uint8x16 a, uint8x16 b)
    {
        return vceqq_u8(a, b);
    }

    static inline mask8x16 compare_gt(uint8x16 a, uint8x16 b)
    {
        return vcgtq_u8(a, b);
    }

    static inline mask8x16 compare_neq(uint8x16 a, uint8x16 b)
    {
        return vmvnq_u8(vceqq_u8(a, b));
    }

    static inline mask8x16 compare_lt(uint8x16 a, uint8x16 b)
    {
        return vcltq_u8(a, b);
    }

    static inline mask8x16 compare_le(uint8x16 a, uint8x16 b)
    {
        return vcleq_u8(a, b);
    }

    static inline mask8x16 compare_ge(uint8x16 a, uint8x16 b)
    {
        return vcgeq_u8(a, b);
    }

    static inline uint8x16 select(mask8x16 mask, uint8x16 a, uint8x16 b)
    {
        return vbslq_u8(mask, a, b);
    }

    static inline uint8x16 min(uint8x16 a, uint8x16 b)
    {
        return vminq_u8(a, b);
    }

    static inline uint8x16 max(uint8x16 a, uint8x16 b)
    {
        return vmaxq_u8(a, b);
    }

    // -----------------------------------------------------------------
    // uint16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint16x8 set_component(uint16x8 a, uint16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return vsetq_lane_u16(s, a, Index);
    }

    template <unsigned int Index>
    static inline uint16 get_component(uint16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return vgetq_lane_u16(a, Index);
    }

    static inline uint16x8 uint16x8_zero()
    {
        return vdupq_n_u16(0);
    }

    static inline uint16x8 uint16x8_set1(uint16 s)
    {
        return vdupq_n_u16(s);
    }

    static inline uint16x8 uint16x8_set8(uint16 s0, uint16 s1, uint16 s2, uint16 s3, uint16 s4, uint16 s5, uint16 s6, uint16 s7)
    {
        uint16x8_t temp = { s0, s1, s2, s3, s4, s5, s6, s7 };
        return temp;
    }

    static inline uint16x8 uint16x8_load_low(const uint16* source)
    {
        const uint16x4_t low = vld1_u16(source);
        const uint16x4_t high = vdup_n_u16(0);
        return vcombine_u16(low, high);
    }

    static inline void uint16x8_store_low(uint16* dest, uint16x8 a)
    {
        vst1_u16(dest, vget_low_u16(a));
    }

    static inline uint16x8 unpacklo(uint16x8 a, uint16x8 b)
    {
	    const uint16x4x2_t temp = vzip_u16(vget_low_u16(a), vget_low_u16(b));
	    return vcombine_u16(temp.val[0], temp.val[1]);
    }

    static inline uint16x8 unpackhi(uint16x8 a, uint16x8 b)
    {
	    const uint16x4x2_t temp = vzip_u16(vget_high_u16(a), vget_high_u16(b));
	    return vcombine_u16(temp.val[0], temp.val[1]);
    }

    static inline uint16x8 add(uint16x8 a, uint16x8 b)
    {
        return vaddq_u16(a, b);
    }

    static inline uint16x8 sub(uint16x8 a, uint16x8 b)
    {
        return vsubq_u16(a, b);
    }

    static inline uint16x8 mullo(uint16x8 a, uint16x8 b)
    {
        return vmulq_u16(a, b);
    }

    // saturated

    static inline uint16x8 adds(uint16x8 a, uint16x8 b)
    {
        return vqaddq_u16(a, b);
    }

    static inline uint16x8 subs(uint16x8 a, uint16x8 b)
    {
        return vqsubq_u16(a, b);
    }

    // bitwise

    static inline uint16x8 bitwise_nand(uint16x8 a, uint16x8 b)
    {
        return vbicq_u16(b, a);
    }

    static inline uint16x8 bitwise_and(uint16x8 a, uint16x8 b)
    {
        return vandq_u16(a, b);
    }

    static inline uint16x8 bitwise_or(uint16x8 a, uint16x8 b)
    {
        return vorrq_u16(a, b);
    }

    static inline uint16x8 bitwise_xor(uint16x8 a, uint16x8 b)
    {
        return veorq_u16(a, b);
    }

    static inline uint16x8 bitwise_not(uint16x8 a)
    {
        return vmvnq_u16(a);
    }

    // compare

    static inline mask16x8 compare_eq(uint16x8 a, uint16x8 b)
    {
        return vceqq_u16(a, b);
    }

    static inline mask16x8 compare_gt(uint16x8 a, uint16x8 b)
    {
        return vcgtq_u16(a, b);
    }

    static inline mask16x8 compare_neq(uint16x8 a, uint16x8 b)
    {
        return vmvnq_u16(vceqq_u16(a, b));
    }

    static inline mask16x8 compare_lt(uint16x8 a, uint16x8 b)
    {
        return vcltq_u16(a, b);
    }

    static inline mask16x8 compare_le(uint16x8 a, uint16x8 b)
    {
        return vcleq_u16(a, b);
    }

    static inline mask16x8 compare_ge(uint16x8 a, uint16x8 b)
    {
        return vcgeq_u16(a, b);
    }

    static inline uint16x8 select(mask16x8 mask, uint16x8 a, uint16x8 b)
    {
        return vbslq_u16(mask, a, b);
    }

    // shift by constant

    template <int Count>
    static inline uint16x8 slli(uint16x8 a)
    {
        return vshlq_n_u16(a, Count);
    }

    template <int Count>
    static inline uint16x8 srli(uint16x8 a)
    {
        return vshrq_n_u16(a, Count);
    }

    template <int Count>
    static inline uint16x8 srai(uint16x8 a)
    {
        const int16x8 temp = vshrq_n_s16(vreinterpretq_s16_u16(a), Count);
        return vreinterpretq_u16_s16(temp);
    }

    // shift by scalar

    static inline uint16x8 sll(uint16x8 a, int count)
    {
        return vshlq_u16(a, vdupq_n_s16(count));
    }

    static inline uint16x8 srl(uint16x8 a, int count)
    {
        return vshlq_u16(a, vdupq_n_s16(-count));
    }

    static inline uint16x8 sra(uint16x8 a, int count)
    {
        const int16x8 temp = vshlq_s16(vreinterpretq_s16_u16(a), vdupq_n_s16(-count));
        return vreinterpretq_u16_s16(temp);
    }

    static inline uint16x8 min(uint16x8 a, uint16x8 b)
    {
        return vminq_u16(a, b);
    }

    static inline uint16x8 max(uint16x8 a, uint16x8 b)
    {
        return vmaxq_u16(a, b);
    }

    // -----------------------------------------------------------------
    // uint32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline uint32x4 shuffle(uint32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        // TODO: optimize
        const uint32 *temp = reinterpret_cast<const uint32 *>(&v);
        return (uint32x4_t) { temp[x], temp[y], temp[z], temp[w] };
    }

    template <>
    inline uint32x4 shuffle<0, 1, 2, 3>(uint32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline uint32x4 set_component(uint32x4 a, uint32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return vsetq_lane_u32(s, a, Index);
    }

    template <unsigned int Index>
    static inline uint32 get_component(uint32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
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

    static inline uint32x4 uint32x4_load_low(const uint32* source)
    {
        const uint32x2_t low = vld1_u32(source);
        const uint32x2_t high = vdup_n_u32(0);
        return vcombine_u32(low, high);
    }

    static inline void uint32x4_store_low(uint32* dest, uint32x4 a)
    {
        vst1_u32(dest, vget_low_u32(a));
    }

    static inline uint32x4 unpacklo(uint32x4 a, uint32x4 b)
    {
	    const uint32x2x2_t temp = vzip_u32(vget_low_u32(a), vget_low_u32(b));
	    return vcombine_u32(temp.val[0], temp.val[1]);
    }

    static inline uint32x4 unpackhi(uint32x4 a, uint32x4 b)
    {
	    const uint32x2x2_t temp = vzip_u32(vget_high_u32(a), vget_high_u32(b));
	    return vcombine_u32(temp.val[0], temp.val[1]);
    }

    static inline uint32x4 add(uint32x4 a, uint32x4 b)
    {
        return vaddq_u32(a, b);
    }

    static inline uint32x4 sub(uint32x4 a, uint32x4 b)
    {
        return vsubq_u32(a, b);
    }

    static inline uint32x4 mullo(uint32x4 a, uint32x4 b)
    {
        return vmulq_u32(a, b);
    }

    // saturated

    static inline uint32x4 adds(uint32x4 a, uint32x4 b)
    {
        return vqaddq_u32(a, b);
    }

    static inline uint32x4 subs(uint32x4 a, uint32x4 b)
    {
        return vqsubq_u32(a, b);
    }

    // bitwise

    static inline uint32x4 bitwise_nand(uint32x4 a, uint32x4 b)
    {
        return vbicq_u32(b, a);
    }

    static inline uint32x4 bitwise_and(uint32x4 a, uint32x4 b)
    {
        return vandq_u32(a, b);
    }

    static inline uint32x4 bitwise_or(uint32x4 a, uint32x4 b)
    {
        return vorrq_u32(a, b);
    }

    static inline uint32x4 bitwise_xor(uint32x4 a, uint32x4 b)
    {
        return veorq_u32(a, b);
    }

    static inline uint32x4 bitwise_not(uint32x4 a)
    {
        return vmvnq_u32(a);
    }

    // compare

    static inline mask32x4 compare_eq(uint32x4 a, uint32x4 b)
    {
        return vceqq_u32(a, b);
    }

    static inline mask32x4 compare_gt(uint32x4 a, uint32x4 b)
    {
        return vcgtq_u32(a, b);
    }

    static inline mask32x4 compare_neq(uint32x4 a, uint32x4 b)
    {
        return vmvnq_u32(vceqq_u32(a, b));
    }

    static inline mask32x4 compare_lt(uint32x4 a, uint32x4 b)
    {
        return vcltq_u32(a, b);
    }

    static inline mask32x4 compare_le(uint32x4 a, uint32x4 b)
    {
        return vcleq_u32(a, b);
    }

    static inline mask32x4 compare_ge(uint32x4 a, uint32x4 b)
    {
        return vcgeq_u32(a, b);
    }

    static inline uint32x4 select(mask32x4 mask, uint32x4 a, uint32x4 b)
    {
        return vbslq_u32(mask, a, b);
    }

    // shift by constant

    template <int Count>
    static inline uint32x4 slli(uint32x4 a)
    {
        return vshlq_n_u32(a, Count);
    }

    template <int Count>
    static inline uint32x4 srli(uint32x4 a)
    {
        return vshrq_n_u32(a, Count);
    }

    template <int Count>
    static inline uint32x4 srai(uint32x4 a)
    {
        const int32x4 temp = vshrq_n_s32(vreinterpretq_s32_u32(a), Count);
        return vreinterpretq_u32_s32(temp);
    }

    // shift by scalar

    static inline uint32x4 sll(uint32x4 a, int count)
    {
        return vshlq_u32(a, vdupq_n_s32(count));
    }

    static inline uint32x4 srl(uint32x4 a, int count)
    {
        return vshlq_u32(a, vdupq_n_s32(-count));
    }

    static inline uint32x4 sra(uint32x4 a, int count)
    {
        const int32x4 temp = vshlq_s32(vreinterpretq_s32_u32(a), vdupq_n_s32(-count));
        return vreinterpretq_u32_s32(temp);
    }

    // shift by vector

    static inline uint32x4 sll(uint32x4 a, uint32x4 count)
    {
        return vshlq_u32(a, vreinterpretq_s32_u32(count));
    }

    static inline uint32x4 srl(uint32x4 a, uint32x4 count)
    {
        const int32x4 shift = vnegq_s32(vreinterpretq_s32_u32(count));
        return vshlq_u32(a, shift);
    }

    static inline uint32x4 sra(uint32x4 a, uint32x4 count)
    {
        const int32x4 shift = vnegq_s32(vreinterpretq_s32_u32(count));
        return vreinterpretq_u32_s32(vshlq_s32(vreinterpretq_s32_u32(a), shift));
    }

    static inline uint32x4 min(uint32x4 a, uint32x4 b)
    {
        return vminq_u32(a, b);
    }

    static inline uint32x4 max(uint32x4 a, uint32x4 b)
    {
        return vmaxq_u32(a, b);
    }

    // -----------------------------------------------------------------
    // uint64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint64x2 set_component(uint64x2 a, uint64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return vsetq_lane_u64(s, a, Index);
    }

    template <unsigned int Index>
    static inline uint64 get_component(uint64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return vgetq_lane_u64(a, Index);
    }

    static inline uint64x2 uint64x2_zero()
    {
        return vdupq_n_u64(0);
    }

    static inline uint64x2 uint64x2_set1(uint64 s)
    {
        return vdupq_n_u64(s);
    }

    static inline uint64x2 uint64x2_set2(uint64 x, uint64 y)
    {
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline uint64x2 unpacklo(uint64x2 a, uint64x2 b)
    {
        return vsetq_lane_u64(vgetq_lane_u64(b, 0), a, 1);
    }

    static inline uint64x2 unpackhi(uint64x2 a, uint64x2 b)
    {
        return vsetq_lane_u64(vgetq_lane_u64(a, 1), b, 0);
    }

    static inline uint64x2 add(uint64x2 a, uint64x2 b)
    {
        return vaddq_u64(a, b);
    }

    static inline uint64x2 sub(uint64x2 a, uint64x2 b)
    {
        return vsubq_u64(a, b);
    }

    static inline uint64x2 bitwise_nand(uint64x2 a, uint64x2 b)
    {
        return vbicq_u64(a, b);
    }

    static inline uint64x2 bitwise_and(uint64x2 a, uint64x2 b)
    {
        return vandq_u64(a, b);
    }

    static inline uint64x2 bitwise_or(uint64x2 a, uint64x2 b)
    {
        return vorrq_u64(a, b);
    }

    static inline uint64x2 bitwise_xor(uint64x2 a, uint64x2 b)
    {
        return veorq_u64(a, b);
    }

    static inline uint64x2 bitwise_not(uint64x2 a)
    {
        return veorq_u64(a, vdupq_n_u64(0xffffffffffffffffull));
    }

    static inline uint64x2 select(mask64x2 mask, uint64x2 a, uint64x2 b)
    {
        return vbslq_u64(mask, a, b);
    }

    // shift by constant

    template <int Count>
    static inline uint64x2 slli(uint64x2 a)
    {
        return vshlq_n_u64(a, Count);
    }

    template <int Count>
    static inline uint64x2 srli(uint64x2 a)
    {
        return vshrq_n_u64(a, Count);
    }

    // shift by scalar

    static inline uint64x2 sll(uint64x2 a, int count)
    {
        return vshlq_u64(a, vdupq_n_s64(count));
    }

    static inline uint64x2 srl(uint64x2 a, int count)
    {
        return vshlq_u64(a, vdupq_n_s64(-count));
    }

    // -----------------------------------------------------------------
    // int8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int8x16 set_component(int8x16 a, int8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return vsetq_lane_s8(s, a, Index);
    }

    template <unsigned int Index>
    static inline int8 get_component(int8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return vgetq_lane_s8(a, Index);
    }

    static inline int8x16 int8x16_zero()
    {
        return vdupq_n_s8(0);
    }

    static inline int8x16 int8x16_set1(int8 s)
    {
        return vdupq_n_s8(s);
    }

    static inline int8x16 int8x16_set16(
        int8 s0, int8 s1, int8 s2, int8 s3, int8 s4, int8 s5, int8 s6, int8 s7,
        int8 s8, int8 s9, int8 s10, int8 s11, int8 s12, int8 s13, int8 s14, int8 s15)
    {
        int8x16_t temp = { s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15 };
        return temp;
    }

    static inline int8x16 int8x16_load_low(const int8* source)
    {
        const int8x8_t low = vld1_s8(source);
        const int8x8_t high = vdup_n_s8(0);
        return vcombine_s8(low, high);
    }

    static inline void int8x16_store_low(int8* dest, int8x16 a)
    {
        vst1_s8(dest, vget_low_s8(a));
    }

    static inline int8x16 unpacklo(int8x16 a, int8x16 b)
    {
	    const int8x8x2_t temp = vzip_s8(vget_low_s8(a), vget_low_s8(b));
	    return vcombine_s8(temp.val[0], temp.val[1]);
    }

    static inline int8x16 unpackhi(int8x16 a, int8x16 b)
    {
	    const int8x8x2_t temp = vzip_s8(vget_high_s8(a), vget_high_s8(b));
	    return vcombine_s8(temp.val[0], temp.val[1]);
    }

    static inline int8x16 add(int8x16 a, int8x16 b)
    {
        return vaddq_s8(a, b);
    }

    static inline int8x16 sub(int8x16 a, int8x16 b)
    {
        return vsubq_s8(a, b);
    }

    // saturated

    static inline int8x16 adds(int8x16 a, int8x16 b)
    {
        return vqaddq_s8(a, b);
    }

    static inline int8x16 subs(int8x16 a, int8x16 b)
    {
        return vqsubq_s8(a, b);
    }

    static inline int8x16 abs(int8x16 a)
    {
        return vabsq_s8(a);
    }

    static inline int8x16 neg(int8x16 a)
    {
        return vnegq_s8(a);
    }

    // bitwise

    static inline int8x16 bitwise_nand(int8x16 a, int8x16 b)
    {
        return vbicq_s8(b, a);
    }

    static inline int8x16 bitwise_and(int8x16 a, int8x16 b)
    {
        return vandq_s8(a, b);
    }

    static inline int8x16 bitwise_or(int8x16 a, int8x16 b)
    {
        return vorrq_s8(a, b);
    }

    static inline int8x16 bitwise_xor(int8x16 a, int8x16 b)
    {
        return veorq_s8(a, b);
    }

    static inline int8x16 bitwise_not(int8x16 a)
    {
        return vmvnq_s8(a);
    }

    // compare

    static inline mask8x16 compare_eq(int8x16 a, int8x16 b)
    {
        return vceqq_s8(a, b);
    }

    static inline mask8x16 compare_gt(int8x16 a, int8x16 b)
    {
        return vcgtq_s8(a, b);
    }

    static inline mask8x16 compare_neq(int8x16 a, int8x16 b)
    {
        return vmvnq_u8(vceqq_s8(a, b));
    }

    static inline mask8x16 compare_lt(int8x16 a, int8x16 b)
    {
        return vcltq_s8(a, b);
    }

    static inline mask8x16 compare_le(int8x16 a, int8x16 b)
    {
        return vcleq_s8(a, b);
    }

    static inline mask8x16 compare_ge(int8x16 a, int8x16 b)
    {
        return vcgeq_s8(a, b);
    }

    static inline int8x16 select(mask8x16 mask, int8x16 a, int8x16 b)
    {
        return vbslq_s8(mask, a, b);
    }

    static inline int8x16 min(int8x16 a, int8x16 b)
    {
        return vminq_s8(a, b);
    }

    static inline int8x16 max(int8x16 a, int8x16 b)
    {
        return vmaxq_s8(a, b);
    }

    // -----------------------------------------------------------------
    // int16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int16x8 set_component(int16x8 a, int16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return vsetq_lane_s16(s, a, Index);
    }

    template <unsigned int Index>
    static inline int16 get_component(int16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return vgetq_lane_s16(a, Index);
    }

    static inline int16x8 int16x8_zero()
    {
        return vdupq_n_s16(0);
    }

    static inline int16x8 int16x8_set1(int16 s)
    {
        return vdupq_n_s16(s);
    }

    static inline int16x8 int16x8_set8(int16 s0, int16 s1, int16 s2, int16 s3, int16 s4, int16 s5, int16 s6, int16 s7)
    {
        int16x8_t temp = { s0, s1, s2, s3, s4, s5, s6, s7 };
        return temp;
    }

    static inline int16x8 int16x8_load_low(const int16* source)
    {
        const int16x4_t low = vld1_s16(source);
        const int16x4_t high = vdup_n_s16(0);
        return vcombine_s16(low, high);
    }

    static inline void int16x8_store_low(int16* dest, int16x8 a)
    {
        vst1_s16(dest, vget_low_s16(a));
    }

    static inline int16x8 unpacklo(int16x8 a, int16x8 b)
    {
	    const int16x4x2_t temp = vzip_s16(vget_low_s16(a), vget_low_s16(b));
	    return vcombine_s16(temp.val[0], temp.val[1]);
    }

    static inline int16x8 unpackhi(int16x8 a, int16x8 b)
    {
	    const int16x4x2_t temp = vzip_s16(vget_high_s16(a), vget_high_s16(b));
	    return vcombine_s16(temp.val[0], temp.val[1]);
    }

    static inline int16x8 add(int16x8 a, int16x8 b)
    {
        return vaddq_s16(a, b);
    }

    static inline int16x8 sub(int16x8 a, int16x8 b)
    {
        return vsubq_s16(a, b);
    }

    static inline int16x8 mullo(int16x8 a, int16x8 b)
    {
        return vmulq_s16(a, b);
    }

    // saturated

    static inline int16x8 adds(int16x8 a, int16x8 b)
    {
        return vqaddq_s16(a, b);
    }

    static inline int16x8 subs(int16x8 a, int16x8 b)
    {
        return vqsubq_s16(a, b);
    }

    static inline int16x8 abs(int16x8 a)
    {
        return vabsq_s16(a);
    }

    static inline int16x8 neg(int16x8 a)
    {
        return vnegq_s16(a);
    }

    // bitwise

    static inline int16x8 bitwise_nand(int16x8 a, int16x8 b)
    {
        return vbicq_s16(b, a);
    }

    static inline int16x8 bitwise_and(int16x8 a, int16x8 b)
    {
        return vandq_s16(a, b);
    }

    static inline int16x8 bitwise_or(int16x8 a, int16x8 b)
    {
        return vorrq_s16(a, b);
    }

    static inline int16x8 bitwise_xor(int16x8 a, int16x8 b)
    {
        return veorq_s16(a, b);
    }

    static inline int16x8 bitwise_not(int16x8 a)
    {
        return vmvnq_s16(a);
    }

    // compare

    static inline mask16x8 compare_eq(int16x8 a, int16x8 b)
    {
        return vceqq_s16(a, b);
    }

    static inline mask16x8 compare_gt(int16x8 a, int16x8 b)
    {
        return vcgtq_s16(a, b);
    }

    static inline mask16x8 compare_neq(int16x8 a, int16x8 b)
    {
        return vmvnq_u16(vceqq_s16(a, b));
    }

    static inline mask16x8 compare_lt(int16x8 a, int16x8 b)
    {
        return vcltq_s16(a, b);
    }

    static inline mask16x8 compare_le(int16x8 a, int16x8 b)
    {
        return vcleq_s16(a, b);
    }

    static inline mask16x8 compare_ge(int16x8 a, int16x8 b)
    {
        return vcgeq_s16(a, b);
    }

    static inline int16x8 select(mask16x8 mask, int16x8 a, int16x8 b)
    {
        return vbslq_s16(mask, a, b);
    }

    // shift by constant

    template <int Count>
    static inline int16x8 slli(int16x8 a)
    {
        const uint16x8 temp = vshlq_n_u16(vreinterpretq_u16_s16(a), Count);
        return vreinterpretq_s16_u16(temp);
    }

    template <int Count>
    static inline int16x8 srli(int16x8 a)
    {
        const uint16x8 temp = vshrq_n_u16(vreinterpretq_u16_s16(a), Count);
        return vreinterpretq_s16_u16(temp);
    }

    template <int Count>
    static inline int16x8 srai(int16x8 a)
    {
        return vshrq_n_s16(a, Count);
    }

    // shift by scalar

    static inline int16x8 sll(int16x8 a, int count)
    {
        const uint16x8 temp = vshlq_u16(vreinterpretq_u16_s16(a), vdupq_n_s16(count));
        return vreinterpretq_s16_u16(temp);
    }

    static inline int16x8 srl(int16x8 a, int count)
    {
        const uint16x8 temp = vshlq_u16(vreinterpretq_u16_s16(a), vdupq_n_s16(-count));
        return vreinterpretq_s16_u16(temp);
    }

    static inline int16x8 sra(int16x8 a, int count)
    {
        return vshlq_s16(a, vdupq_n_s16(-count));
    }

    static inline int16x8 min(int16x8 a, int16x8 b)
    {
        return vminq_s16(a, b);
    }

    static inline int16x8 max(int16x8 a, int16x8 b)
    {
        return vmaxq_s16(a, b);
    }

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline int32x4 shuffle(int32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        // TODO: optimize
        const int32* temp = reinterpret_cast<const int32 *>(&v);
        return (int32x4_t) { temp[x], temp[y], temp[z], temp[w] };
    }

    template <>
    inline int32x4 shuffle<0, 1, 2, 3>(int32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline int32x4 set_component(int32x4 a, int32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return vsetq_lane_s32(s, a, Index);
    }

    template <unsigned int Index>
    static inline int32 get_component(int32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
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

    static inline int32x4 int32x4_load_low(const int32* source)
    {
        const int32x2_t low = vld1_s32(source);
        const int32x2_t high = vdup_n_s32(0);
        return vcombine_s32(low, high);
    }

    static inline void int32x4_store_low(int32* dest, int32x4 a)
    {
        vst1_s32(dest, vget_low_s32(a));
    }

    static inline int32x4 unpacklo(int32x4 a, int32x4 b)
    {
	    const int32x2x2_t temp = vzip_s32(vget_low_s32(a), vget_low_s32(b));
	    return vcombine_s32(temp.val[0], temp.val[1]);
    }

    static inline int32x4 unpackhi(int32x4 a, int32x4 b)
    {
	    const int32x2x2_t temp = vzip_s32(vget_high_s32(a), vget_high_s32(b));
	    return vcombine_s32(temp.val[0], temp.val[1]);
    }

    static inline int32x4 abs(int32x4 a)
    {
        return vabsq_s32(a);
    }

    static inline int32x4 neg(int32x4 a)
    {
        return vnegq_s32(a);
    }

    static inline int32x4 add(int32x4 a, int32x4 b)
    {
        return vaddq_s32(a, b);
    }

    static inline int32x4 sub(int32x4 a, int32x4 b)
    {
        return vsubq_s32(a, b);
    }

    static inline int32x4 mullo(int32x4 a, int32x4 b)
    {
        return vmulq_s32(a, b);
    }

    // saturated

    static inline int32x4 adds(int32x4 a, int32x4 b)
    {
        return vqaddq_s32(a, b);
    }

    static inline int32x4 subs(int32x4 a, int32x4 b)
    {
        return vqsubq_s32(a, b);
    }

    // bitwise

    static inline int32x4 bitwise_nand(int32x4 a, int32x4 b)
    {
        return vbicq_s32(b, a);
    }

    static inline int32x4 bitwise_and(int32x4 a, int32x4 b)
    {
        return vandq_s32(a, b);
    }

    static inline int32x4 bitwise_or(int32x4 a, int32x4 b)
    {
        return vorrq_s32(a, b);
    }

    static inline int32x4 bitwise_xor(int32x4 a, int32x4 b)
    {
        return veorq_s32(a, b);
    }

    static inline int32x4 bitwise_not(int32x4 a)
    {
        return vmvnq_s32(a);
    }

    // compare

    static inline mask32x4 compare_eq(int32x4 a, int32x4 b)
    {
        return vceqq_s32(a, b);
    }

    static inline mask32x4 compare_gt(int32x4 a, int32x4 b)
    {
        return vcgtq_s32(a, b);
    }

    static inline mask32x4 compare_neq(int32x4 a, int32x4 b)
    {
        return vmvnq_u32(vceqq_s32(a, b));
    }

    static inline mask32x4 compare_lt(int32x4 a, int32x4 b)
    {
        return vcltq_s32(a, b);
    }

    static inline mask32x4 compare_le(int32x4 a, int32x4 b)
    {
        return vcleq_s32(a, b);
    }

    static inline mask32x4 compare_ge(int32x4 a, int32x4 b)
    {
        return vcgeq_s32(a, b);
    }

    static inline int32x4 select(mask32x4 mask, int32x4 a, int32x4 b)
    {
        return vbslq_s32(mask, a, b);
    }

    // shift by constant

    template <int Count>
    static inline int32x4 slli(int32x4 a)
    {
        const uint32x4 temp = vshlq_n_u32(vreinterpretq_u32_s32(a), Count);
        return vreinterpretq_s32_u32(temp);
    }

    template <int Count>
    static inline int32x4 srli(int32x4 a)
    {
        const uint32x4 temp = vshrq_n_u32(vreinterpretq_u32_s32(a), Count);
        return vreinterpretq_s32_u32(temp);
    }

    template <int Count>
    static inline int32x4 srai(int32x4 a)
    {
        return vshrq_n_s32(a, Count);
    }

    // shift by scalar

    static inline int32x4 sll(int32x4 a, int count)
    {
        const uint32x4 temp = vshlq_u32(vreinterpretq_u32_s32(a), vdupq_n_s32(count));
        return vreinterpretq_s32_u32(temp);
    }

    static inline int32x4 srl(int32x4 a, int count)
    {
        const uint32x4 temp = vshlq_u32(vreinterpretq_u32_s32(a), vdupq_n_s32(-count));
        return vreinterpretq_s32_u32(temp);
    }

    static inline int32x4 sra(int32x4 a, int count)
    {
        return vshlq_s32(a, vdupq_n_s32(-count));
    }

    // shift by vector

    static inline int32x4 sll(int32x4 a, uint32x4 count)
    {
        return vreinterpretq_s32_u32(vshlq_u32(vreinterpretq_u32_s32(a), vreinterpretq_s32_u32(count)));
    }

    static inline int32x4 srl(int32x4 a, uint32x4 count)
    {
        const int32x4 shift = vnegq_s32(vreinterpretq_s32_u32(count));
        return vreinterpretq_s32_u32(vshlq_u32(vreinterpretq_u32_s32(a), shift));
    }

    static inline int32x4 sra(int32x4 a, uint32x4 count)
    {
        const int32x4 shift = vnegq_s32(vreinterpretq_s32_u32(count));
        return vshlq_s32(a, shift);
    }

    static inline int32x4 min(int32x4 a, int32x4 b)
    {
        return vminq_s32(a, b);
    }

    static inline int32x4 max(int32x4 a, int32x4 b)
    {
        return vmaxq_s32(a, b);
    }

    static inline uint32 pack(int32x4 s)
    {
        const uint16x4_t a = vqmovun_s32(s);
        const uint16x8_t b = vcombine_u16(a, a);
        const uint8x8_t c = vqmovn_u16(b);
        const uint32x2_t d = vreinterpret_u32_u8(c);
        return vget_lane_u32(d, 0);
    }

    static inline int32x4 unpack(uint32 s)
    {
        const uint8x8_t a = vreinterpret_u8_u32(vdup_n_u32(s));
        const uint16x4_t b = vget_low_u16(vmovl_u8(a));
        return vreinterpretq_s32_u32(vmovl_u16(b));
    }

    // -----------------------------------------------------------------
    // int64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int64x2 set_component(int64x2 a, int64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return vsetq_lane_s64(s, a, Index);
    }

    template <unsigned int Index>
    static inline int64 get_component(int64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return vgetq_lane_s64(a, Index);
    }

    static inline int64x2 int64x2_zero()
    {
        return vdupq_n_s64(0);
    }

    static inline int64x2 int64x2_set1(int64 s)
    {
        return vdupq_n_s64(s);
    }

    static inline int64x2 int64x2_set2(int64 x, int64 y)
    {
        int64x2_t temp = { x, y };
        return temp;
    }

    static inline int64x2 unpacklo(int64x2 a, int64x2 b)
    {
        return vsetq_lane_s64(vgetq_lane_s64(b, 0), a, 1);
    }

    static inline int64x2 unpackhi(int64x2 a, int64x2 b)
    {
        return vsetq_lane_s64(vgetq_lane_s64(a, 1), b, 0);
    }

    static inline int64x2 add(int64x2 a, int64x2 b)
    {
        return vaddq_s64(a, b);
    }

    static inline int64x2 sub(int64x2 a, int64x2 b)
    {
        return vsubq_s64(a, b);
    }

    static inline int64x2 bitwise_nand(int64x2 a, int64x2 b)
    {
        return vbicq_s64(a, b);
    }

    static inline int64x2 bitwise_and(int64x2 a, int64x2 b)
    {
        return vandq_s64(a, b);
    }

    static inline int64x2 bitwise_or(int64x2 a, int64x2 b)
    {
        return vorrq_s64(a, b);
    }

    static inline int64x2 bitwise_xor(int64x2 a, int64x2 b)
    {
        return veorq_s64(a, b);
    }

    static inline int64x2 bitwise_not(int64x2 a)
    {
        return veorq_s64(a, vdupq_n_s64(0xffffffffffffffffull));
    }

    static inline int64x2 select(mask64x2 mask, int64x2 a, int64x2 b)
    {
        return vbslq_s64(mask, a, b);
    }

    // shift by constant

    template <int Count>
    static inline int64x2 slli(int64x2 a)
    {
        const uint64x2 temp = vshlq_n_u64(vreinterpretq_u64_s64(a), Count);
        return vreinterpretq_s64_u64(temp);
    }

    template <int Count>
    static inline int64x2 srli(int64x2 a)
    {
        const uint64x2 temp = vshrq_n_u64(vreinterpretq_u64_s64(a), Count);
        return vreinterpretq_s64_u64(temp);
    }

    // shift by scalar

    static inline int64x2 sll(int64x2 a, int count)
    {
        const uint64x2 temp = vshlq_u64(vreinterpretq_u64_s64(a), vdupq_n_s64(count));
        return vreinterpretq_s64_u64(temp);
    }

    static inline int64x2 srl(int64x2 a, int count)
    {
        const uint64x2 temp = vshlq_u64(vreinterpretq_u64_s64(a), vdupq_n_s64(-count));
        return vreinterpretq_s64_u64(temp);
    }

    // -----------------------------------------------------------------
    // mask8x16
    // -----------------------------------------------------------------

    static inline mask8x16 operator & (mask8x16 a, mask8x16 b)
    {
        return vandq_u8(a, b);
    }

    static inline mask8x16 operator | (mask8x16 a, mask8x16 b)
    {
        return vorrq_u8(a, b);
    }

    static inline mask8x16 operator ^ (mask8x16 a, mask8x16 b)
    {
        return veorq_u8(a, b);
    }

    static inline uint32 get_mask(mask8x16 a)
    {
        // TODO: optimize
        uint32 mask = 0;
        mask |= vgetq_lane_u8(a, 0x0) & 0x0001;
        mask |= vgetq_lane_u8(a, 0x1) & 0x0002;
        mask |= vgetq_lane_u8(a, 0x2) & 0x0004;
        mask |= vgetq_lane_u8(a, 0x3) & 0x0008;
        mask |= vgetq_lane_u8(a, 0x4) & 0x0010;
        mask |= vgetq_lane_u8(a, 0x5) & 0x0020;
        mask |= vgetq_lane_u8(a, 0x6) & 0x0040;
        mask |= vgetq_lane_u8(a, 0x7) & 0x0080;
        mask |= vgetq_lane_u8(a, 0x8) & 0x0100;
        mask |= vgetq_lane_u8(a, 0x9) & 0x0200;
        mask |= vgetq_lane_u8(a, 0xa) & 0x0400;
        mask |= vgetq_lane_u8(a, 0xb) & 0x0800;
        mask |= vgetq_lane_u8(a, 0xc) & 0x1000;
        mask |= vgetq_lane_u8(a, 0xd) & 0x2000;
        mask |= vgetq_lane_u8(a, 0xe) & 0x4000;
        mask |= vgetq_lane_u8(a, 0xf) & 0x8000;
        return mask;
    }

    // -----------------------------------------------------------------
    // mask16x8
    // -----------------------------------------------------------------

    static inline mask16x8 operator & (mask16x8 a, mask16x8 b)
    {
        return vandq_u16(a, b);
    }

    static inline mask16x8 operator | (mask16x8 a, mask16x8 b)
    {
        return vorrq_u16(a, b);
    }

    static inline mask16x8 operator ^ (mask16x8 a, mask16x8 b)
    {
        return veorq_u16(a, b);
    }

    static inline uint32 get_mask(mask16x8 a)
    {
        // TODO: optimize
        uint32 mask = 0;
        mask |= vgetq_lane_u16(a, 0x0) & 0x01;
        mask |= vgetq_lane_u16(a, 0x1) & 0x02;
        mask |= vgetq_lane_u16(a, 0x2) & 0x04;
        mask |= vgetq_lane_u16(a, 0x3) & 0x08;
        mask |= vgetq_lane_u16(a, 0x4) & 0x10;
        mask |= vgetq_lane_u16(a, 0x5) & 0x20;
        mask |= vgetq_lane_u16(a, 0x6) & 0x40;
        mask |= vgetq_lane_u16(a, 0x7) & 0x80;
        return mask;
    }

    // -----------------------------------------------------------------
    // mask32x4
    // -----------------------------------------------------------------

    static inline mask32x4 operator & (mask32x4 a, mask32x4 b)
    {
        return vandq_u32(a, b);
    }

    static inline mask32x4 operator | (mask32x4 a, mask32x4 b)
    {
        return vorrq_u32(a, b);
    }

    static inline mask32x4 operator ^ (mask32x4 a, mask32x4 b)
    {
        return veorq_u32(a, b);
    }

    static inline uint32 get_mask(mask32x4 a)
    {
        const uint32x4_t mask = { 1, 2, 4, 8 };
        const uint32x4_t masked = vandq_u32(a, mask);
        const uint32x2_t high = vget_high_u32(masked);
        const uint32x2_t low = vget_low_u32(masked);
        const uint32x2_t d0 = vorr_u32(high, low);
        const uint32x2_t d1 = vpadd_u32(d0, d0);
        return vget_lane_u32(d1, 0);
    }

    // -----------------------------------------------------------------
    // mask64x2
    // -----------------------------------------------------------------

    static inline mask64x2 operator & (mask64x2 a, mask64x2 b)
    {
        return vandq_u64(a, b);
    }

    static inline mask64x2 operator | (mask64x2 a, mask64x2 b)
    {
        return vorrq_u64(a, b);
    }

    static inline mask64x2 operator ^ (mask64x2 a, mask64x2 b)
    {
        return veorq_u64(a, b);
    }

    static inline uint32 get_mask(mask64x2 a)
    {
        uint32 x = uint32(vgetq_lane_u64(a, 0)) & 1;
        uint32 y = uint32(vgetq_lane_u64(a, 1)) & 2;
        return x | y;
    }

} // namespace simd
} // namespace mango
