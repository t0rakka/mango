/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // u8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u8x16 set_component(u8x16 a, u8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return vsetq_lane_u8(s, a, Index);
    }

    template <unsigned int Index>
    static inline u8 get_component(u8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return vgetq_lane_u8(a, Index);
    }

    static inline u8x16 u8x16_zero()
    {
        return vdupq_n_u8(0);
    }

    static inline u8x16 u8x16_set1(u8 s)
    {
        return vdupq_n_u8(s);
    }

    static inline u8x16 u8x16_set16(
        u8 s0, u8 s1, u8 s2, u8 s3, u8 s4, u8 s5, u8 s6, u8 s7,
        u8 s8, u8 s9, u8 s10, u8 s11, u8 s12, u8 s13, u8 s14, u8 s15)
    {
        uint8x16_t temp = { s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15 };
        return temp;
    }

    static inline u8x16 u8x16_uload(const u8* source)
    {
        return vld1q_u8(source);
    }

    static inline void u8x16_ustore(u8* dest, u8x16 a)
    {
        vst1q_u8(dest, a);
    }

    static inline u8x16 u8x16_load_low(const u8* source)
    {
        const uint8x8_t low = vld1_u8(source);
        const uint8x8_t high = vdup_n_u8(0);
        return vcombine_u8(low, high);
    }

    static inline void u8x16_store_low(u8* dest, u8x16 a)
    {
        vst1_u8(dest, vget_low_u8(a));
    }

    static inline u8x16 unpacklo(u8x16 a, u8x16 b)
    {
	    const uint8x8x2_t temp = vzip_u8(vget_low_u8(a), vget_low_u8(b));
	    return vcombine_u8(temp.val[0], temp.val[1]);
    }

    static inline u8x16 unpackhi(u8x16 a, u8x16 b)
    {
	    const uint8x8x2_t temp = vzip_u8(vget_high_u8(a), vget_high_u8(b));
	    return vcombine_u8(temp.val[0], temp.val[1]);
    }

    static inline u8x16 add(u8x16 a, u8x16 b)
    {
        return vaddq_u8(a, b);
    }

    static inline u8x16 sub(u8x16 a, u8x16 b)
    {
        return vsubq_u8(a, b);
    }

    static inline u8x16 adds(u8x16 a, u8x16 b)
    {
        return vqaddq_u8(a, b);
    }

    static inline u8x16 subs(u8x16 a, u8x16 b)
    {
        return vqsubq_u8(a, b);
    }

    static inline u8x16 avg(u8x16 a, u8x16 b)
    {
        return vhaddq_u8(a, b);
    }

    static inline u8x16 ravg(u8x16 a, u8x16 b)
    {
        return vrhaddq_u8(a, b);
    }

    // bitwise

    static inline u8x16 bitwise_nand(u8x16 a, u8x16 b)
    {
        return vbicq_u8(b, a);
    }

    static inline u8x16 bitwise_and(u8x16 a, u8x16 b)
    {
        return vandq_u8(a, b);
    }

    static inline u8x16 bitwise_or(u8x16 a, u8x16 b)
    {
        return vorrq_u8(a, b);
    }

    static inline u8x16 bitwise_xor(u8x16 a, u8x16 b)
    {
        return veorq_u8(a, b);
    }

    static inline u8x16 bitwise_not(u8x16 a)
    {
        return vmvnq_u8(a);
    }

    // compare

    static inline mask8x16 compare_eq(u8x16 a, u8x16 b)
    {
        return vceqq_u8(a, b);
    }

    static inline mask8x16 compare_gt(u8x16 a, u8x16 b)
    {
        return vcgtq_u8(a, b);
    }

    static inline mask8x16 compare_neq(u8x16 a, u8x16 b)
    {
        return vmvnq_u8(vceqq_u8(a, b));
    }

    static inline mask8x16 compare_lt(u8x16 a, u8x16 b)
    {
        return vcltq_u8(a, b);
    }

    static inline mask8x16 compare_le(u8x16 a, u8x16 b)
    {
        return vcleq_u8(a, b);
    }

    static inline mask8x16 compare_ge(u8x16 a, u8x16 b)
    {
        return vcgeq_u8(a, b);
    }

    static inline u8x16 select(mask8x16 mask, u8x16 a, u8x16 b)
    {
        return vbslq_u8(mask, a, b);
    }

    static inline u8x16 min(u8x16 a, u8x16 b)
    {
        return vminq_u8(a, b);
    }

    static inline u8x16 max(u8x16 a, u8x16 b)
    {
        return vmaxq_u8(a, b);
    }

    // -----------------------------------------------------------------
    // u16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u16x8 set_component(u16x8 a, u16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return vsetq_lane_u16(s, a, Index);
    }

    template <unsigned int Index>
    static inline u16 get_component(u16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return vgetq_lane_u16(a, Index);
    }

    static inline u16x8 u16x8_zero()
    {
        return vdupq_n_u16(0);
    }

    static inline u16x8 u16x8_set1(u16 s)
    {
        return vdupq_n_u16(s);
    }

    static inline u16x8 u16x8_set8(u16 s0, u16 s1, u16 s2, u16 s3, u16 s4, u16 s5, u16 s6, u16 s7)
    {
        uint16x8_t temp = { s0, s1, s2, s3, s4, s5, s6, s7 };
        return temp;
    }

    static inline u16x8 u16x8_uload(const u16* source)
    {
        return vld1q_u16(source);
    }

    static inline void u16x8_ustore(u16* dest, u16x8 a)
    {
        vst1q_u16(dest, a);
    }

    static inline u16x8 u16x8_load_low(const u16* source)
    {
        const uint16x4_t low = vld1_u16(source);
        const uint16x4_t high = vdup_n_u16(0);
        return vcombine_u16(low, high);
    }

    static inline void u16x8_store_low(u16* dest, u16x8 a)
    {
        vst1_u16(dest, vget_low_u16(a));
    }

    static inline u16x8 unpacklo(u16x8 a, u16x8 b)
    {
	    const uint16x4x2_t temp = vzip_u16(vget_low_u16(a), vget_low_u16(b));
	    return vcombine_u16(temp.val[0], temp.val[1]);
    }

    static inline u16x8 unpackhi(u16x8 a, u16x8 b)
    {
	    const uint16x4x2_t temp = vzip_u16(vget_high_u16(a), vget_high_u16(b));
	    return vcombine_u16(temp.val[0], temp.val[1]);
    }

    static inline u16x8 add(u16x8 a, u16x8 b)
    {
        return vaddq_u16(a, b);
    }

    static inline u16x8 sub(u16x8 a, u16x8 b)
    {
        return vsubq_u16(a, b);
    }

    static inline u16x8 adds(u16x8 a, u16x8 b)
    {
        return vqaddq_u16(a, b);
    }

    static inline u16x8 subs(u16x8 a, u16x8 b)
    {
        return vqsubq_u16(a, b);
    }

    static inline u16x8 avg(u16x8 a, u16x8 b)
    {
        return vhaddq_u16(a, b);
    }

    static inline u16x8 ravg(u16x8 a, u16x8 b)
    {
        return vrhaddq_u16(a, b);
    }

    static inline u16x8 mullo(u16x8 a, u16x8 b)
    {
        return vmulq_u16(a, b);
    }

    // bitwise

    static inline u16x8 bitwise_nand(u16x8 a, u16x8 b)
    {
        return vbicq_u16(b, a);
    }

    static inline u16x8 bitwise_and(u16x8 a, u16x8 b)
    {
        return vandq_u16(a, b);
    }

    static inline u16x8 bitwise_or(u16x8 a, u16x8 b)
    {
        return vorrq_u16(a, b);
    }

    static inline u16x8 bitwise_xor(u16x8 a, u16x8 b)
    {
        return veorq_u16(a, b);
    }

    static inline u16x8 bitwise_not(u16x8 a)
    {
        return vmvnq_u16(a);
    }

    // compare

    static inline mask16x8 compare_eq(u16x8 a, u16x8 b)
    {
        return vceqq_u16(a, b);
    }

    static inline mask16x8 compare_gt(u16x8 a, u16x8 b)
    {
        return vcgtq_u16(a, b);
    }

    static inline mask16x8 compare_neq(u16x8 a, u16x8 b)
    {
        return vmvnq_u16(vceqq_u16(a, b));
    }

    static inline mask16x8 compare_lt(u16x8 a, u16x8 b)
    {
        return vcltq_u16(a, b);
    }

    static inline mask16x8 compare_le(u16x8 a, u16x8 b)
    {
        return vcleq_u16(a, b);
    }

    static inline mask16x8 compare_ge(u16x8 a, u16x8 b)
    {
        return vcgeq_u16(a, b);
    }

    static inline u16x8 select(mask16x8 mask, u16x8 a, u16x8 b)
    {
        return vbslq_u16(mask, a, b);
    }

    static inline u16x8 min(u16x8 a, u16x8 b)
    {
        return vminq_u16(a, b);
    }

    static inline u16x8 max(u16x8 a, u16x8 b)
    {
        return vmaxq_u16(a, b);
    }

    // shift by constant

    template <int Count>
    static inline u16x8 slli(u16x8 a)
    {
        return vshlq_n_u16(a, Count);
    }

    template <int Count>
    static inline u16x8 srli(u16x8 a)
    {
        return vshrq_n_u16(a, Count);
    }

    template <int Count>
    static inline u16x8 srai(u16x8 a)
    {
        const s16x8 temp = vshrq_n_s16(vreinterpretq_s16_u16(a), Count);
        return vreinterpretq_u16_s16(temp);
    }

    // shift by scalar

    static inline u16x8 sll(u16x8 a, int count)
    {
        return vshlq_u16(a, vdupq_n_s16(count));
    }

    static inline u16x8 srl(u16x8 a, int count)
    {
        return vshlq_u16(a, vdupq_n_s16(-count));
    }

    static inline u16x8 sra(u16x8 a, int count)
    {
        const s16x8 temp = vshlq_s16(vreinterpretq_s16_u16(a), vdupq_n_s16(-count));
        return vreinterpretq_u16_s16(temp);
    }

    // -----------------------------------------------------------------
    // u32x4
    // -----------------------------------------------------------------

    // shuffle

    template <u32 x, u32 y, u32 z, u32 w>
    static inline u32x4 shuffle(u32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        // TODO: optimize
        const u32 *temp = reinterpret_cast<const u32 *>(&v);
        return (uint32x4_t) { temp[x], temp[y], temp[z], temp[w] };
    }

    template <>
    inline u32x4 shuffle<0, 1, 2, 3>(u32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline u32x4 set_component(u32x4 a, u32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return vsetq_lane_u32(s, a, Index);
    }

    template <unsigned int Index>
    static inline u32 get_component(u32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return vgetq_lane_u32(a, Index);
    }

    static inline u32x4 u32x4_zero()
    {
        return vdupq_n_u32(0);
    }

    static inline u32x4 u32x4_set1(u32 s)
    {
        return vdupq_n_u32(s);
    }

    static inline u32x4 u32x4_set4(u32 x, u32 y, u32 z, u32 w)
    {
        uint32x4_t temp = { x, y, z, w };
        return temp;
    }

    static inline u32x4 u32x4_uload(const u32* source)
    {
        return vld1q_u32(source);
    }

    static inline void u32x4_ustore(u32* dest, u32x4 a)
    {
        vst1q_u32(dest, a);
    }

    static inline u32x4 u32x4_load_low(const u32* source)
    {
        const uint32x2_t low = vld1_u32(source);
        const uint32x2_t high = vdup_n_u32(0);
        return vcombine_u32(low, high);
    }

    static inline void u32x4_store_low(u32* dest, u32x4 a)
    {
        vst1_u32(dest, vget_low_u32(a));
    }

    static inline u32x4 unpacklo(u32x4 a, u32x4 b)
    {
	    const uint32x2x2_t temp = vzip_u32(vget_low_u32(a), vget_low_u32(b));
	    return vcombine_u32(temp.val[0], temp.val[1]);
    }

    static inline u32x4 unpackhi(u32x4 a, u32x4 b)
    {
	    const uint32x2x2_t temp = vzip_u32(vget_high_u32(a), vget_high_u32(b));
	    return vcombine_u32(temp.val[0], temp.val[1]);
    }

    static inline u32x4 add(u32x4 a, u32x4 b)
    {
        return vaddq_u32(a, b);
    }

    static inline u32x4 sub(u32x4 a, u32x4 b)
    {
        return vsubq_u32(a, b);
    }

    static inline u32x4 adds(u32x4 a, u32x4 b)
    {
        return vqaddq_u32(a, b);
    }

    static inline u32x4 subs(u32x4 a, u32x4 b)
    {
        return vqsubq_u32(a, b);
    }

    static inline u32x4 avg(u32x4 a, u32x4 b)
    {
        return vhaddq_u32(a, b);
    }

    static inline u32x4 ravg(u32x4 a, u32x4 b)
    {
        return vrhaddq_u32(a, b);
    }

    static inline u32x4 mullo(u32x4 a, u32x4 b)
    {
        return vmulq_u32(a, b);
    }

    // bitwise

    static inline u32x4 bitwise_nand(u32x4 a, u32x4 b)
    {
        return vbicq_u32(b, a);
    }

    static inline u32x4 bitwise_and(u32x4 a, u32x4 b)
    {
        return vandq_u32(a, b);
    }

    static inline u32x4 bitwise_or(u32x4 a, u32x4 b)
    {
        return vorrq_u32(a, b);
    }

    static inline u32x4 bitwise_xor(u32x4 a, u32x4 b)
    {
        return veorq_u32(a, b);
    }

    static inline u32x4 bitwise_not(u32x4 a)
    {
        return vmvnq_u32(a);
    }

    // compare

    static inline mask32x4 compare_eq(u32x4 a, u32x4 b)
    {
        return vceqq_u32(a, b);
    }

    static inline mask32x4 compare_gt(u32x4 a, u32x4 b)
    {
        return vcgtq_u32(a, b);
    }

    static inline mask32x4 compare_neq(u32x4 a, u32x4 b)
    {
        return vmvnq_u32(vceqq_u32(a, b));
    }

    static inline mask32x4 compare_lt(u32x4 a, u32x4 b)
    {
        return vcltq_u32(a, b);
    }

    static inline mask32x4 compare_le(u32x4 a, u32x4 b)
    {
        return vcleq_u32(a, b);
    }

    static inline mask32x4 compare_ge(u32x4 a, u32x4 b)
    {
        return vcgeq_u32(a, b);
    }

    static inline u32x4 select(mask32x4 mask, u32x4 a, u32x4 b)
    {
        return vbslq_u32(mask, a, b);
    }

    static inline u32x4 min(u32x4 a, u32x4 b)
    {
        return vminq_u32(a, b);
    }

    static inline u32x4 max(u32x4 a, u32x4 b)
    {
        return vmaxq_u32(a, b);
    }

    // shift by constant

    template <int Count>
    static inline u32x4 slli(u32x4 a)
    {
        return vshlq_n_u32(a, Count);
    }

    template <int Count>
    static inline u32x4 srli(u32x4 a)
    {
        return vshrq_n_u32(a, Count);
    }

    template <int Count>
    static inline u32x4 srai(u32x4 a)
    {
        const s32x4 temp = vshrq_n_s32(vreinterpretq_s32_u32(a), Count);
        return vreinterpretq_u32_s32(temp);
    }

    // shift by scalar

    static inline u32x4 sll(u32x4 a, int count)
    {
        return vshlq_u32(a, vdupq_n_s32(count));
    }

    static inline u32x4 srl(u32x4 a, int count)
    {
        return vshlq_u32(a, vdupq_n_s32(-count));
    }

    static inline u32x4 sra(u32x4 a, int count)
    {
        const s32x4 temp = vshlq_s32(vreinterpretq_s32_u32(a), vdupq_n_s32(-count));
        return vreinterpretq_u32_s32(temp);
    }

    // shift by vector

    static inline u32x4 sll(u32x4 a, u32x4 count)
    {
        return vshlq_u32(a, vreinterpretq_s32_u32(count));
    }

    static inline u32x4 srl(u32x4 a, u32x4 count)
    {
        const s32x4 shift = vnegq_s32(vreinterpretq_s32_u32(count));
        return vshlq_u32(a, shift);
    }

    static inline u32x4 sra(u32x4 a, u32x4 count)
    {
        const s32x4 shift = vnegq_s32(vreinterpretq_s32_u32(count));
        return vreinterpretq_u32_s32(vshlq_s32(vreinterpretq_s32_u32(a), shift));
    }

    // -----------------------------------------------------------------
    // u64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u64x2 set_component(u64x2 a, u64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return vsetq_lane_u64(s, a, Index);
    }

    template <unsigned int Index>
    static inline u64 get_component(u64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return vgetq_lane_u64(a, Index);
    }

    static inline u64x2 u64x2_zero()
    {
        return vdupq_n_u64(0);
    }

    static inline u64x2 u64x2_set1(u64 s)
    {
        return vdupq_n_u64(s);
    }

    static inline u64x2 u64x2_set2(u64 x, u64 y)
    {
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline u64x2 u64x2_uload(const u64* source)
    {
        return vld1q_u64(source);
    }

    static inline void u64x2_ustore(u64* dest, u64x2 a)
    {
        vst1q_u64(dest, a);
    }

    static inline u64x2 unpacklo(u64x2 a, u64x2 b)
    {
        return vsetq_lane_u64(vgetq_lane_u64(b, 0), a, 1);
    }

    static inline u64x2 unpackhi(u64x2 a, u64x2 b)
    {
        return vsetq_lane_u64(vgetq_lane_u64(a, 1), b, 0);
    }

    static inline u64x2 add(u64x2 a, u64x2 b)
    {
        return vaddq_u64(a, b);
    }

    static inline u64x2 sub(u64x2 a, u64x2 b)
    {
        return vsubq_u64(a, b);
    }

    static inline u64x2 avg(u64x2 a, u64x2 b)
    {
        // unsigned average
        int64x2_t sa = vreinterpretq_s64_u64(a);
        int64x2_t sb = vreinterpretq_s64_u64(b);
        int64x2_t axb = veorq_s64(sa, sb);
        int64x2_t temp = vaddq_s64(vandq_s64(sa, sb), vshrq_n_s64(axb, 1));
        return vreinterpretq_u64_s64(temp);
    }

    static inline u64x2 ravg(u64x2 a, u64x2 b)
    {
        a = vaddq_u64(a, vdupq_n_u64(1));
        return avg(a, b);
    }

    // bitwise

    static inline u64x2 bitwise_nand(u64x2 a, u64x2 b)
    {
        return vbicq_u64(a, b);
    }

    static inline u64x2 bitwise_and(u64x2 a, u64x2 b)
    {
        return vandq_u64(a, b);
    }

    static inline u64x2 bitwise_or(u64x2 a, u64x2 b)
    {
        return vorrq_u64(a, b);
    }

    static inline u64x2 bitwise_xor(u64x2 a, u64x2 b)
    {
        return veorq_u64(a, b);
    }

#ifdef __aarch64__

    static inline u64x2 bitwise_not(u64x2 a)
    {
        return veorq_u64(a, vceqq_u64(a, a));
    }

#else

    static inline u64x2 bitwise_not(u64x2 a)
    {
        return veorq_u64(a, vdupq_n_u64(0xffffffffffffffffull));
    }

#endif

    // compare

    static inline mask64x2 compare_eq(u64x2 a, u64x2 b)
    {
        u64 x = 0 - (vgetq_lane_u64(a, 0) == vgetq_lane_u64(b, 0));
        u64 y = 0 - (vgetq_lane_u64(a, 1) == vgetq_lane_u64(b, 1));
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline mask64x2 compare_gt(u64x2 a, u64x2 b)
    {
        u64 x = 0 - (vgetq_lane_u64(a, 0) > vgetq_lane_u64(b, 0));
        u64 y = 0 - (vgetq_lane_u64(a, 1) > vgetq_lane_u64(b, 1));
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline mask64x2 compare_neq(u64x2 a, u64x2 b)
    {
        u64 x = 0 - (vgetq_lane_u64(a, 0) != vgetq_lane_u64(b, 0));
        u64 y = 0 - (vgetq_lane_u64(a, 1) != vgetq_lane_u64(b, 1));
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline mask64x2 compare_lt(u64x2 a, u64x2 b)
    {
        u64 x = 0 - (vgetq_lane_u64(a, 0) < vgetq_lane_u64(b, 0));
        u64 y = 0 - (vgetq_lane_u64(a, 1) < vgetq_lane_u64(b, 1));
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline mask64x2 compare_le(u64x2 a, u64x2 b)
    {
        u64 x = 0 - (vgetq_lane_u64(a, 0) <= vgetq_lane_u64(b, 0));
        u64 y = 0 - (vgetq_lane_u64(a, 1) <= vgetq_lane_u64(b, 1));
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline mask64x2 compare_ge(u64x2 a, u64x2 b)
    {
        u64 x = 0 - (vgetq_lane_u64(a, 0) >= vgetq_lane_u64(b, 0));
        u64 y = 0 - (vgetq_lane_u64(a, 1) >= vgetq_lane_u64(b, 1));
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline u64x2 select(mask64x2 mask, u64x2 a, u64x2 b)
    {
        return vbslq_u64(mask, a, b);
    }

    static inline u64x2 min(u64x2 a, u64x2 b)
    {
        return select(compare_gt(a, b), b, a);
    }

    static inline u64x2 max(u64x2 a, u64x2 b)
    {
        return select(compare_gt(a, b), a, b);
    }

    // shift by constant

    template <int Count>
    static inline u64x2 slli(u64x2 a)
    {
        return vshlq_n_u64(a, Count);
    }

    template <int Count>
    static inline u64x2 srli(u64x2 a)
    {
        return vshrq_n_u64(a, Count);
    }

    // shift by scalar

    static inline u64x2 sll(u64x2 a, int count)
    {
        return vshlq_u64(a, vdupq_n_s64(count));
    }

    static inline u64x2 srl(u64x2 a, int count)
    {
        return vshlq_u64(a, vdupq_n_s64(-count));
    }

    // -----------------------------------------------------------------
    // s8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s8x16 set_component(s8x16 a, s8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return vsetq_lane_s8(s, a, Index);
    }

    template <unsigned int Index>
    static inline s8 get_component(s8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return vgetq_lane_s8(a, Index);
    }

    static inline s8x16 s8x16_zero()
    {
        return vdupq_n_s8(0);
    }

    static inline s8x16 s8x16_set1(s8 s)
    {
        return vdupq_n_s8(s);
    }

    static inline s8x16 s8x16_set16(
        s8 v0, s8 v1, s8 v2, s8 v3, s8 v4, s8 v5, s8 v6, s8 v7,
        s8 v8, s8 v9, s8 v10, s8 v11, s8 v12, s8 v13, s8 v14, s8 v15)
    {
        int8x16_t temp = { v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15 };
        return temp;
    }

    static inline s8x16 s8x16_uload(const s8* source)
    {
        return vld1q_s8(source);
    }

    static inline void s8x16_ustore(s8* dest, s8x16 a)
    {
        vst1q_s8(dest, a);
    }

    static inline s8x16 s8x16_load_low(const s8* source)
    {
        const int8x8_t low = vld1_s8(source);
        const int8x8_t high = vdup_n_s8(0);
        return vcombine_s8(low, high);
    }

    static inline void s8x16_store_low(s8* dest, s8x16 a)
    {
        vst1_s8(dest, vget_low_s8(a));
    }

    static inline s8x16 unpacklo(s8x16 a, s8x16 b)
    {
	    const int8x8x2_t temp = vzip_s8(vget_low_s8(a), vget_low_s8(b));
	    return vcombine_s8(temp.val[0], temp.val[1]);
    }

    static inline s8x16 unpackhi(s8x16 a, s8x16 b)
    {
	    const int8x8x2_t temp = vzip_s8(vget_high_s8(a), vget_high_s8(b));
	    return vcombine_s8(temp.val[0], temp.val[1]);
    }

    static inline s8x16 add(s8x16 a, s8x16 b)
    {
        return vaddq_s8(a, b);
    }

    static inline s8x16 sub(s8x16 a, s8x16 b)
    {
        return vsubq_s8(a, b);
    }

    static inline s8x16 adds(s8x16 a, s8x16 b)
    {
        return vqaddq_s8(a, b);
    }

    static inline s8x16 subs(s8x16 a, s8x16 b)
    {
        return vqsubq_s8(a, b);
    }

    static inline s8x16 avg(s8x16 a, s8x16 b)
    {
        return vhaddq_s8(a, b);
    }

    static inline s8x16 ravg(s8x16 a, s8x16 b)
    {
        return vrhaddq_s8(a, b);
    }

    static inline s8x16 abs(s8x16 a)
    {
        return vabsq_s8(a);
    }

    static inline s8x16 neg(s8x16 a)
    {
        return vnegq_s8(a);
    }

    // bitwise

    static inline s8x16 bitwise_nand(s8x16 a, s8x16 b)
    {
        return vbicq_s8(b, a);
    }

    static inline s8x16 bitwise_and(s8x16 a, s8x16 b)
    {
        return vandq_s8(a, b);
    }

    static inline s8x16 bitwise_or(s8x16 a, s8x16 b)
    {
        return vorrq_s8(a, b);
    }

    static inline s8x16 bitwise_xor(s8x16 a, s8x16 b)
    {
        return veorq_s8(a, b);
    }

    static inline s8x16 bitwise_not(s8x16 a)
    {
        return vmvnq_s8(a);
    }

    // compare

    static inline mask8x16 compare_eq(s8x16 a, s8x16 b)
    {
        return vceqq_s8(a, b);
    }

    static inline mask8x16 compare_gt(s8x16 a, s8x16 b)
    {
        return vcgtq_s8(a, b);
    }

    static inline mask8x16 compare_neq(s8x16 a, s8x16 b)
    {
        return vmvnq_u8(vceqq_s8(a, b));
    }

    static inline mask8x16 compare_lt(s8x16 a, s8x16 b)
    {
        return vcltq_s8(a, b);
    }

    static inline mask8x16 compare_le(s8x16 a, s8x16 b)
    {
        return vcleq_s8(a, b);
    }

    static inline mask8x16 compare_ge(s8x16 a, s8x16 b)
    {
        return vcgeq_s8(a, b);
    }

    static inline s8x16 select(mask8x16 mask, s8x16 a, s8x16 b)
    {
        return vbslq_s8(mask, a, b);
    }

    static inline s8x16 min(s8x16 a, s8x16 b)
    {
        return vminq_s8(a, b);
    }

    static inline s8x16 max(s8x16 a, s8x16 b)
    {
        return vmaxq_s8(a, b);
    }

    // -----------------------------------------------------------------
    // s16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s16x8 set_component(s16x8 a, s16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return vsetq_lane_s16(s, a, Index);
    }

    template <unsigned int Index>
    static inline s16 get_component(s16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return vgetq_lane_s16(a, Index);
    }

    static inline s16x8 s16x8_zero()
    {
        return vdupq_n_s16(0);
    }

    static inline s16x8 s16x8_set1(s16 s)
    {
        return vdupq_n_s16(s);
    }

    static inline s16x8 s16x8_set8(s16 s0, s16 s1, s16 s2, s16 s3, s16 s4, s16 s5, s16 s6, s16 s7)
    {
        int16x8_t temp = { s0, s1, s2, s3, s4, s5, s6, s7 };
        return temp;
    }

    static inline s16x8 s16x8_uload(const s16* source)
    {
        return vld1q_s16(source);
    }

    static inline void s16x8_ustore(s16* dest, s16x8 a)
    {
        vst1q_s16(dest, a);
    }

    static inline s16x8 s16x8_load_low(const s16* source)
    {
        const int16x4_t low = vld1_s16(source);
        const int16x4_t high = vdup_n_s16(0);
        return vcombine_s16(low, high);
    }

    static inline void s16x8_store_low(s16* dest, s16x8 a)
    {
        vst1_s16(dest, vget_low_s16(a));
    }

    static inline s16x8 unpacklo(s16x8 a, s16x8 b)
    {
	    const int16x4x2_t temp = vzip_s16(vget_low_s16(a), vget_low_s16(b));
	    return vcombine_s16(temp.val[0], temp.val[1]);
    }

    static inline s16x8 unpackhi(s16x8 a, s16x8 b)
    {
	    const int16x4x2_t temp = vzip_s16(vget_high_s16(a), vget_high_s16(b));
	    return vcombine_s16(temp.val[0], temp.val[1]);
    }

    static inline s16x8 add(s16x8 a, s16x8 b)
    {
        return vaddq_s16(a, b);
    }

    static inline s16x8 sub(s16x8 a, s16x8 b)
    {
        return vsubq_s16(a, b);
    }

    static inline s16x8 adds(s16x8 a, s16x8 b)
    {
        return vqaddq_s16(a, b);
    }

    static inline s16x8 subs(s16x8 a, s16x8 b)
    {
        return vqsubq_s16(a, b);
    }

#if defined(__aarch64__)

    static inline s16x8 hadd(s16x8 a, s16x8 b)
    {
        return vpaddq_s16(a, b);
    }

    static inline s16x8 hsub(s16x8 a, s16x8 b)
    {
        b = vnegq_s16(b);
        return vpaddq_s16(a, b);
    }

#else

    static inline s16x8 hadd(s16x8 a, s16x8 b)
    {
        s16x8 temp_a = unpacklo(a, b);
        s16x8 temp_b = unpackhi(a, b);
        a = unpacklo(temp_a, temp_b);
        b = unpackhi(temp_a, temp_b);
        temp_a = unpacklo(a, b);
        temp_b = unpackhi(a, b);
        return add(temp_a, temp_b);
    }

    static inline s16x8 hsub(s16x8 a, s16x8 b)
    {
        s16x8 temp_a = unpacklo(a, b);
        s16x8 temp_b = unpackhi(a, b);
        a = unpacklo(temp_a, temp_b);
        b = unpackhi(temp_a, temp_b);
        temp_a = unpacklo(a, b);
        temp_b = unpackhi(a, b);
        return sub(temp_a, temp_b);
    }

#endif

    static inline s16x8 hadds(s16x8 a, s16x8 b)
    {
        s16x8 temp_a = unpacklo(a, b);
        s16x8 temp_b = unpackhi(a, b);
        a = unpacklo(temp_a, temp_b);
        b = unpackhi(temp_a, temp_b);
        temp_a = unpacklo(a, b);
        temp_b = unpackhi(a, b);
        return adds(temp_a, temp_b);
    }

    static inline s16x8 hsubs(s16x8 a, s16x8 b)
    {
        s16x8 temp_a = unpacklo(a, b);
        s16x8 temp_b = unpackhi(a, b);
        a = unpacklo(temp_a, temp_b);
        b = unpackhi(temp_a, temp_b);
        temp_a = unpacklo(a, b);
        temp_b = unpackhi(a, b);
        return subs(temp_a, temp_b);
    }

    static inline s16x8 avg(s16x8 a, s16x8 b)
    {
        return vhaddq_s16(a, b);
    }

    static inline s16x8 ravg(s16x8 a, s16x8 b)
    {
        return vrhaddq_s16(a, b);
    }

    static inline s16x8 mullo(s16x8 a, s16x8 b)
    {
        return vmulq_s16(a, b);
    }

    static inline s16x8 abs(s16x8 a)
    {
        return vabsq_s16(a);
    }

    static inline s16x8 neg(s16x8 a)
    {
        return vnegq_s16(a);
    }

    // bitwise

    static inline s16x8 bitwise_nand(s16x8 a, s16x8 b)
    {
        return vbicq_s16(b, a);
    }

    static inline s16x8 bitwise_and(s16x8 a, s16x8 b)
    {
        return vandq_s16(a, b);
    }

    static inline s16x8 bitwise_or(s16x8 a, s16x8 b)
    {
        return vorrq_s16(a, b);
    }

    static inline s16x8 bitwise_xor(s16x8 a, s16x8 b)
    {
        return veorq_s16(a, b);
    }

    static inline s16x8 bitwise_not(s16x8 a)
    {
        return vmvnq_s16(a);
    }

    // compare

    static inline mask16x8 compare_eq(s16x8 a, s16x8 b)
    {
        return vceqq_s16(a, b);
    }

    static inline mask16x8 compare_gt(s16x8 a, s16x8 b)
    {
        return vcgtq_s16(a, b);
    }

    static inline mask16x8 compare_neq(s16x8 a, s16x8 b)
    {
        return vmvnq_u16(vceqq_s16(a, b));
    }

    static inline mask16x8 compare_lt(s16x8 a, s16x8 b)
    {
        return vcltq_s16(a, b);
    }

    static inline mask16x8 compare_le(s16x8 a, s16x8 b)
    {
        return vcleq_s16(a, b);
    }

    static inline mask16x8 compare_ge(s16x8 a, s16x8 b)
    {
        return vcgeq_s16(a, b);
    }

    static inline s16x8 select(mask16x8 mask, s16x8 a, s16x8 b)
    {
        return vbslq_s16(mask, a, b);
    }

    static inline s16x8 min(s16x8 a, s16x8 b)
    {
        return vminq_s16(a, b);
    }

    static inline s16x8 max(s16x8 a, s16x8 b)
    {
        return vmaxq_s16(a, b);
    }

    // shift by constant

    template <int Count>
    static inline s16x8 slli(s16x8 a)
    {
        const u16x8 temp = vshlq_n_u16(vreinterpretq_u16_s16(a), Count);
        return vreinterpretq_s16_u16(temp);
    }

    template <int Count>
    static inline s16x8 srli(s16x8 a)
    {
        const u16x8 temp = vshrq_n_u16(vreinterpretq_u16_s16(a), Count);
        return vreinterpretq_s16_u16(temp);
    }

    template <int Count>
    static inline s16x8 srai(s16x8 a)
    {
        return vshrq_n_s16(a, Count);
    }

    // shift by scalar

    static inline s16x8 sll(s16x8 a, int count)
    {
        const u16x8 temp = vshlq_u16(vreinterpretq_u16_s16(a), vdupq_n_s16(count));
        return vreinterpretq_s16_u16(temp);
    }

    static inline s16x8 srl(s16x8 a, int count)
    {
        const u16x8 temp = vshlq_u16(vreinterpretq_u16_s16(a), vdupq_n_s16(-count));
        return vreinterpretq_s16_u16(temp);
    }

    static inline s16x8 sra(s16x8 a, int count)
    {
        return vshlq_s16(a, vdupq_n_s16(-count));
    }

    // -----------------------------------------------------------------
    // s32x4
    // -----------------------------------------------------------------

    // shuffle

    template <u32 x, u32 y, u32 z, u32 w>
    static inline s32x4 shuffle(s32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        // TODO: optimize
        const s32* temp = reinterpret_cast<const s32 *>(&v);
        return (int32x4_t) { temp[x], temp[y], temp[z], temp[w] };
    }

    template <>
    inline s32x4 shuffle<0, 1, 2, 3>(s32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline s32x4 set_component(s32x4 a, s32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return vsetq_lane_s32(s, a, Index);
    }

    template <unsigned int Index>
    static inline s32 get_component(s32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return vgetq_lane_s32(a, Index);
    }

    // set

    static inline s32x4 s32x4_zero()
    {
        return vdupq_n_s32(0);
    }

    static inline s32x4 s32x4_set1(s32 s)
    {
        return vdupq_n_s32(s);
    }

    static inline s32x4 s32x4_set4(s32 x, s32 y, s32 z, s32 w)
    {
        int32x4_t temp = { x, y, z, w };
        return temp;
    }

    static inline s32x4 s32x4_uload(const s32* source)
    {
        return vld1q_s32(source);
    }

    static inline void s32x4_ustore(s32* dest, s32x4 a)
    {
        vst1q_s32(dest, a);
    }

    static inline s32x4 s32x4_load_low(const s32* source)
    {
        const int32x2_t low = vld1_s32(source);
        const int32x2_t high = vdup_n_s32(0);
        return vcombine_s32(low, high);
    }

    static inline void s32x4_store_low(s32* dest, s32x4 a)
    {
        vst1_s32(dest, vget_low_s32(a));
    }

    static inline s32x4 unpacklo(s32x4 a, s32x4 b)
    {
	    const int32x2x2_t temp = vzip_s32(vget_low_s32(a), vget_low_s32(b));
	    return vcombine_s32(temp.val[0], temp.val[1]);
    }

    static inline s32x4 unpackhi(s32x4 a, s32x4 b)
    {
	    const int32x2x2_t temp = vzip_s32(vget_high_s32(a), vget_high_s32(b));
	    return vcombine_s32(temp.val[0], temp.val[1]);
    }

    static inline s32x4 abs(s32x4 a)
    {
        return vabsq_s32(a);
    }

    static inline s32x4 neg(s32x4 a)
    {
        return vnegq_s32(a);
    }

    static inline s32x4 add(s32x4 a, s32x4 b)
    {
        return vaddq_s32(a, b);
    }

    static inline s32x4 sub(s32x4 a, s32x4 b)
    {
        return vsubq_s32(a, b);
    }

    static inline s32x4 adds(s32x4 a, s32x4 b)
    {
        return vqaddq_s32(a, b);
    }

    static inline s32x4 subs(s32x4 a, s32x4 b)
    {
        return vqsubq_s32(a, b);
    }

#if defined(__aarch64__)

    static inline s32x4 hadd(s32x4 a, s32x4 b)
    {
        return vpaddq_s32(a, b);
    }

    static inline s32x4 hsub(s32x4 a, s32x4 b)
    {
        b = vnegq_s32(b);
        return vpaddq_s32(a, b);
    }

#else

    static inline s32x4 hadd(s32x4 a, s32x4 b)
    {
        s32x4 temp_a = unpacklo(a, b);
        s32x4 temp_b = unpackhi(a, b);
        a = unpacklo(temp_a, temp_b);
        b = unpackhi(temp_a, temp_b);
        return add(a, b);
    }

    static inline s32x4 hsub(s32x4 a, s32x4 b)
    {
        s32x4 temp_a = unpacklo(a, b);
        s32x4 temp_b = unpackhi(a, b);
        a = unpacklo(temp_a, temp_b);
        b = unpackhi(temp_a, temp_b);
        return sub(a, b);
    }

#endif

    static inline s32x4 avg(s32x4 a, s32x4 b)
    {
        return vhaddq_s32(a, b);
    }

    static inline s32x4 ravg(s32x4 a, s32x4 b)
    {
        return vrhaddq_s32(a, b);
    }

    static inline s32x4 mullo(s32x4 a, s32x4 b)
    {
        return vmulq_s32(a, b);
    }

    // bitwise

    static inline s32x4 bitwise_nand(s32x4 a, s32x4 b)
    {
        return vbicq_s32(b, a);
    }

    static inline s32x4 bitwise_and(s32x4 a, s32x4 b)
    {
        return vandq_s32(a, b);
    }

    static inline s32x4 bitwise_or(s32x4 a, s32x4 b)
    {
        return vorrq_s32(a, b);
    }

    static inline s32x4 bitwise_xor(s32x4 a, s32x4 b)
    {
        return veorq_s32(a, b);
    }

    static inline s32x4 bitwise_not(s32x4 a)
    {
        return vmvnq_s32(a);
    }

    // compare

    static inline mask32x4 compare_eq(s32x4 a, s32x4 b)
    {
        return vceqq_s32(a, b);
    }

    static inline mask32x4 compare_gt(s32x4 a, s32x4 b)
    {
        return vcgtq_s32(a, b);
    }

    static inline mask32x4 compare_neq(s32x4 a, s32x4 b)
    {
        return vmvnq_u32(vceqq_s32(a, b));
    }

    static inline mask32x4 compare_lt(s32x4 a, s32x4 b)
    {
        return vcltq_s32(a, b);
    }

    static inline mask32x4 compare_le(s32x4 a, s32x4 b)
    {
        return vcleq_s32(a, b);
    }

    static inline mask32x4 compare_ge(s32x4 a, s32x4 b)
    {
        return vcgeq_s32(a, b);
    }

    static inline s32x4 select(mask32x4 mask, s32x4 a, s32x4 b)
    {
        return vbslq_s32(mask, a, b);
    }

    static inline s32x4 min(s32x4 a, s32x4 b)
    {
        return vminq_s32(a, b);
    }

    static inline s32x4 max(s32x4 a, s32x4 b)
    {
        return vmaxq_s32(a, b);
    }

    // shift by constant

    template <int Count>
    static inline s32x4 slli(s32x4 a)
    {
        const u32x4 temp = vshlq_n_u32(vreinterpretq_u32_s32(a), Count);
        return vreinterpretq_s32_u32(temp);
    }

    template <int Count>
    static inline s32x4 srli(s32x4 a)
    {
        const u32x4 temp = vshrq_n_u32(vreinterpretq_u32_s32(a), Count);
        return vreinterpretq_s32_u32(temp);
    }

    template <int Count>
    static inline s32x4 srai(s32x4 a)
    {
        return vshrq_n_s32(a, Count);
    }

    // shift by scalar

    static inline s32x4 sll(s32x4 a, int count)
    {
        const u32x4 temp = vshlq_u32(vreinterpretq_u32_s32(a), vdupq_n_s32(count));
        return vreinterpretq_s32_u32(temp);
    }

    static inline s32x4 srl(s32x4 a, int count)
    {
        const u32x4 temp = vshlq_u32(vreinterpretq_u32_s32(a), vdupq_n_s32(-count));
        return vreinterpretq_s32_u32(temp);
    }

    static inline s32x4 sra(s32x4 a, int count)
    {
        return vshlq_s32(a, vdupq_n_s32(-count));
    }

    // shift by vector

    static inline s32x4 sll(s32x4 a, u32x4 count)
    {
        return vreinterpretq_s32_u32(vshlq_u32(vreinterpretq_u32_s32(a), vreinterpretq_s32_u32(count)));
    }

    static inline s32x4 srl(s32x4 a, u32x4 count)
    {
        const s32x4 shift = vnegq_s32(vreinterpretq_s32_u32(count));
        return vreinterpretq_s32_u32(vshlq_u32(vreinterpretq_u32_s32(a), shift));
    }

    static inline s32x4 sra(s32x4 a, u32x4 count)
    {
        const s32x4 shift = vnegq_s32(vreinterpretq_s32_u32(count));
        return vshlq_s32(a, shift);
    }

    static inline u32 pack(s32x4 s)
    {
        const uint16x4_t a = vqmovun_s32(s);
        const uint16x8_t b = vcombine_u16(a, a);
        const uint8x8_t c = vqmovn_u16(b);
        const uint32x2_t d = vreinterpret_u32_u8(c);
        return vget_lane_u32(d, 0);
    }

    static inline s32x4 unpack(u32 s)
    {
        const uint8x8_t a = vreinterpret_u8_u32(vdup_n_u32(s));
        const uint16x4_t b = vget_low_u16(vmovl_u8(a));
        return vreinterpretq_s32_u32(vmovl_u16(b));
    }

    // -----------------------------------------------------------------
    // s64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s64x2 set_component(s64x2 a, s64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return vsetq_lane_s64(s, a, Index);
    }

    template <unsigned int Index>
    static inline s64 get_component(s64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return vgetq_lane_s64(a, Index);
    }

    static inline s64x2 s64x2_zero()
    {
        return vdupq_n_s64(0);
    }

    static inline s64x2 s64x2_set1(s64 s)
    {
        return vdupq_n_s64(s);
    }

    static inline s64x2 s64x2_set2(s64 x, s64 y)
    {
        int64x2_t temp = { x, y };
        return temp;
    }

    static inline s64x2 s64x2_uload(const s64* source)
    {
        return vld1q_s64(source);
    }

    static inline void s64x2_ustore(s64* dest, s64x2 a)
    {
        vst1q_s64(dest, a);
    }

    static inline s64x2 unpacklo(s64x2 a, s64x2 b)
    {
        return vsetq_lane_s64(vgetq_lane_s64(b, 0), a, 1);
    }

    static inline s64x2 unpackhi(s64x2 a, s64x2 b)
    {
        return vsetq_lane_s64(vgetq_lane_s64(a, 1), b, 0);
    }

    static inline s64x2 add(s64x2 a, s64x2 b)
    {
        return vaddq_s64(a, b);
    }

    static inline s64x2 sub(s64x2 a, s64x2 b)
    {
        return vsubq_s64(a, b);
    }

    static inline s64x2 avg(s64x2 a, s64x2 b)
    {
        // unsigned average
        int64x2_t axb = veorq_s64(a, b);
        int64x2_t temp = vaddq_s64(vandq_s64(a, b), vshrq_n_s64(axb, 1));

        // signed rounding
        int64x2_t sign = vshrq_n_s64(temp, 63);
        temp = vaddq_s64(temp, vandq_s64(sign, axb));

        return temp;
    }

    static inline s64x2 ravg(s64x2 a, s64x2 b)
    {
        a = vaddq_s64(a, vdupq_n_s64(1));
        return avg(a, b);
    }

    // bitwise

    static inline s64x2 bitwise_nand(s64x2 a, s64x2 b)
    {
        return vbicq_s64(a, b);
    }

    static inline s64x2 bitwise_and(s64x2 a, s64x2 b)
    {
        return vandq_s64(a, b);
    }

    static inline s64x2 bitwise_or(s64x2 a, s64x2 b)
    {
        return vorrq_s64(a, b);
    }

    static inline s64x2 bitwise_xor(s64x2 a, s64x2 b)
    {
        return veorq_s64(a, b);
    }

#ifdef __aarch64__

    static inline s64x2 bitwise_not(s64x2 a)
    {
        return veorq_s64(a, vreinterpretq_s64_u64(vceqq_s64(a, a)));
    }

#else

    static inline s64x2 bitwise_not(s64x2 a)
    {
        return veorq_s64(a, vdupq_n_s64(0xffffffffffffffffull));
    }

#endif

    // compare

    static inline mask64x2 compare_eq(s64x2 a, s64x2 b)
    {
        u64 x = 0 - (vgetq_lane_s64(a, 0) == vgetq_lane_s64(b, 0));
        u64 y = 0 - (vgetq_lane_s64(a, 1) == vgetq_lane_s64(b, 1));
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline mask64x2 compare_gt(s64x2 a, s64x2 b)
    {
        u64 x = 0 - (vgetq_lane_s64(a, 0) > vgetq_lane_s64(b, 0));
        u64 y = 0 - (vgetq_lane_s64(a, 1) > vgetq_lane_s64(b, 1));
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline mask64x2 compare_neq(s64x2 a, s64x2 b)
    {
        u64 x = 0 - (vgetq_lane_s64(a, 0) != vgetq_lane_s64(b, 0));
        u64 y = 0 - (vgetq_lane_s64(a, 1) != vgetq_lane_s64(b, 1));
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline mask64x2 compare_lt(s64x2 a, s64x2 b)
    {
        u64 x = 0 - (vgetq_lane_s64(a, 0) < vgetq_lane_s64(b, 0));
        u64 y = 0 - (vgetq_lane_s64(a, 1) < vgetq_lane_s64(b, 1));
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline mask64x2 compare_le(s64x2 a, s64x2 b)
    {
        u64 x = 0 - (vgetq_lane_s64(a, 0) <= vgetq_lane_s64(b, 0));
        u64 y = 0 - (vgetq_lane_s64(a, 1) <= vgetq_lane_s64(b, 1));
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline mask64x2 compare_ge(s64x2 a, s64x2 b)
    {
        u64 x = 0 - (vgetq_lane_s64(a, 0) >= vgetq_lane_s64(b, 0));
        u64 y = 0 - (vgetq_lane_s64(a, 1) >= vgetq_lane_s64(b, 1));
        uint64x2_t temp = { x, y };
        return temp;
    }

    static inline s64x2 select(mask64x2 mask, s64x2 a, s64x2 b)
    {
        return vbslq_s64(mask, a, b);
    }

    static inline s64x2 min(s64x2 a, s64x2 b)
    {
        return select(compare_gt(a, b), b, a);
    }

    static inline s64x2 max(s64x2 a, s64x2 b)
    {
        return select(compare_gt(a, b), a, b);
    }

    // shift by constant

    template <int Count>
    static inline s64x2 slli(s64x2 a)
    {
        const u64x2 temp = vshlq_n_u64(vreinterpretq_u64_s64(a), Count);
        return vreinterpretq_s64_u64(temp);
    }

    template <int Count>
    static inline s64x2 srli(s64x2 a)
    {
        const u64x2 temp = vshrq_n_u64(vreinterpretq_u64_s64(a), Count);
        return vreinterpretq_s64_u64(temp);
    }

    // shift by scalar

    static inline s64x2 sll(s64x2 a, int count)
    {
        const u64x2 temp = vshlq_u64(vreinterpretq_u64_s64(a), vdupq_n_s64(count));
        return vreinterpretq_s64_u64(temp);
    }

    static inline s64x2 srl(s64x2 a, int count)
    {
        const u64x2 temp = vshlq_u64(vreinterpretq_u64_s64(a), vdupq_n_s64(-count));
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

    static inline mask8x16 operator ! (mask8x16 a)
    {
        return bitwise_not(u8x16(a)).data;
    }

    static inline u32 get_mask(mask8x16 a)
    {
        const uint8x8_t weights = { 1, 2, 4, 8, 16, 32, 64, 128 };

        uint8x8_t lo = vget_low_u8(a);
        lo = vand_u8(lo, weights);
        lo = vpadd_u8(lo, lo);
        lo = vpadd_u8(lo, lo);
        lo = vpadd_u8(lo, lo);

        uint8x8_t hi = vget_high_u8(a);
        hi = vand_u8(hi, weights);
        hi = vpadd_u8(hi, hi);
        hi = vpadd_u8(hi, hi);
        hi = vpadd_u8(hi, hi);

        return vget_lane_u8(lo, 0) | (vget_lane_u8(hi, 0) << 8);
    }

#ifdef __aarch64__

    static inline bool none_of(mask8x16 a)
    {
        return vmaxvq_u8(a) == 0;
    }

    static inline bool any_of(mask8x16 a)
    {
        return vmaxvq_u8(a) != 0;
    }

    static inline bool all_of(mask8x16 a)
    {
        return vminvq_u8(a) != 0;
    }

#else

    static inline bool none_of(mask8x16 a)
    {
        uint8x8_t b = vpadd_u8(vget_low_u8(a), vget_high_u8(a));
        return vget_lane_u8(vpmax_u8(b, b), 0) == 0;
    }

    static inline bool any_of(mask8x16 a)
    {
        uint8x8_t b = vpadd_u8(vget_low_u8(a), vget_high_u8(a));
        return vget_lane_u8(vpmax_u8(b, b), 0) != 0;
    }

    static inline bool all_of(mask8x16 a)
    {
        uint8x8_t b = vpadd_u8(vget_low_u8(a), vget_high_u8(a));
        return vget_lane_u8(vpmin_u8(b, b), 0) != 0;
    }

#endif

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

    static inline mask16x8 operator ! (mask16x8 a)
    {
        return bitwise_not(u16x8(a)).data;
    }

#ifdef __aarch64__

    static inline u32 get_mask(mask16x8 a)
    {
        const uint16x8_t weights = { 1, 2, 4, 8, 16, 32, 64, 128 };
        a = vandq_u16(a, weights);
        a = vpaddq_u16(a, a);
        a = vpaddq_u16(a, a);
        a = vpaddq_u16(a, a);
        return vgetq_lane_u16(a, 0);;
    }

    static inline bool none_of(mask16x8 a)
    {
        return vmaxvq_u16(a) == 0;
    }

    static inline bool any_of(mask16x8 a)
    {
        return vmaxvq_u16(a) != 0;
    }

    static inline bool all_of(mask16x8 a)
    {
        return vminvq_u16(a) != 0;
    }

#else

    static inline u32 get_mask(mask16x8 a)
    {
        const uint16x8_t weights = { 1, 2, 4, 8, 16, 32, 64, 128 };
        a = vandq_u16(a, weights);
        uint16x4_t b = vpadd_u16(vget_low_u16(a), vget_high_u16(a));
        b = vpadd_u16(b, b);
        b = vpadd_u16(b, b);
        return vget_lane_u16(b, 0);
    }

    static inline bool none_of(mask16x8 a)
    {
        uint16x4_t b = vpadd_u16(vget_low_u16(a), vget_high_u16(a));
        return vget_lane_u16(vpmax_u16(b, b), 0) == 0;
    }

    static inline bool any_of(mask16x8 a)
    {
        uint16x4_t b = vpadd_u16(vget_low_u16(a), vget_high_u16(a));
        return vget_lane_u16(vpmax_u16(b, b), 0) != 0;
    }

    static inline bool all_of(mask16x8 a)
    {
        uint16x4_t b = vpadd_u16(vget_low_u16(a), vget_high_u16(a));
        return vget_lane_u16(vpmin_u16(b, b), 0) != 0;
    }

#endif

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

    static inline mask32x4 operator ! (mask32x4 a)
    {
        return bitwise_not(u32x4(a)).data;
    }

#ifdef __aarch64__

    static inline u32 get_mask(mask32x4 a)
    {
        const uint32x4_t weights = { 1, 2, 4, 8 };
        a = vandq_u32(a, weights);
        a = vpaddq_u32(a, a);
        a = vpaddq_u32(a, a);
        return vgetq_lane_u32(a, 0);
    }

    static inline bool none_of(mask32x4 a)
    {
        return vmaxvq_u32(a) == 0;
    }

    static inline bool any_of(mask32x4 a)
    {
        return vmaxvq_u32(a) != 0;
    }

    static inline bool all_of(mask32x4 a)
    {
        return vminvq_u32(a) != 0;
    }

#else

    static inline u32 get_mask(mask32x4 a)
    {
        const uint32x4_t weights = { 1, 2, 4, 8 };
        a = vandq_u32(a, weights);
        uint32x2_t b = vpadd_u32(vget_low_u32(a), vget_high_u32(a));
        b = vpadd_u32(b, b);
        return vget_lane_u32(b, 0);
    }

    static inline bool none_of(mask32x4 a)
    {
        uint32x2_t b = vorr_u32(vget_low_u32(a), vget_high_u32(a));
        return vget_lane_u32(vpmax_u32(b, b), 0) == 0;
    }

    static inline bool any_of(mask32x4 a)
    {
        uint32x2_t b = vorr_u32(vget_low_u32(a), vget_high_u32(a));
        return vget_lane_u32(vpmax_u32(b, b), 0) != 0;
    }

    static inline bool all_of(mask32x4 a)
    {
        uint32x2_t b = vand_u32(vget_low_u32(a), vget_high_u32(a));
        return vget_lane_u32(vpmin_u32(b, b), 0) != 0;
    }

#endif

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

    static inline mask64x2 operator ! (mask64x2 a)
    {
        return bitwise_not(u64x2(a)).data;
    }

#ifdef __aarch64__

    static inline u32 get_mask(mask64x2 a)
    {
        const uint64x2_t weights = { 1, 2 };
        a = vandq_u64(a, weights);
        a = vpaddq_u64(a, a);
        return u32(vgetq_lane_u64(a, 0));
    }

#else

    static inline u32 get_mask(mask64x2 a)
    {
        const uint32x2_t weights = { 1, 2 };
        uint32x2_t b = vmovn_u64(a);
        b = vand_u32(b, weights);
        b = vpadd_u32(b, b);
        return vget_lane_u32(b, 0);
    }

#endif

    static inline bool none_of(mask64x2 a)
    {
        uint32x2_t b = vmovn_u64(a);
        b = vpmax_u32(b, b);
        return vget_lane_u32(b, 0) == 0;
    }

    static inline bool any_of(mask64x2 a)
    {
        uint32x2_t b = vmovn_u64(a);
        b = vpmax_u32(b, b);
        return vget_lane_u32(b, 0) != 0;
    }

    static inline bool all_of(mask64x2 a)
    {
        uint32x2_t b = vmovn_u64(a);
        b = vpmin_u32(b, b);
        return vget_lane_u32(b, 0) != 0;
    }

} // namespace simd
} // namespace mango
