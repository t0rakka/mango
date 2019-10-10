/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

#define SIMD_COMPOSITE_FUNC1(R, A, FUNC) \
    static inline R FUNC(A a) \
    { \
        R result; \
        result.lo = FUNC(a.lo); \
        result.hi = FUNC(a.hi); \
        return result; \
    }

#define SIMD_COMPOSITE_FUNC2(R, AB, FUNC) \
    static inline R FUNC(AB a, AB b) \
    { \
        R result; \
        result.lo = FUNC(a.lo, b.lo); \
        result.hi = FUNC(a.hi, b.hi); \
        return result; \
    }

#define SIMD_COMPOSITE_SELECT(MASK, AB, FUNC) \
    static inline AB select(MASK mask, AB a, AB b) \
    { \
        AB result; \
        result.lo = select(mask.lo, a.lo, b.lo); \
        result.hi = select(mask.hi, a.hi, b.hi); \
        return result; \
    }

    // -----------------------------------------------------------------
    // u8x64
    // -----------------------------------------------------------------

    static inline u8x64 u8x64_zero()
    {
        u8x64 result;
        result.lo = u8x32_zero();
        result.hi = u8x32_zero();
        return result;
    }

    static inline u8x64 u8x64_set1(u8 s)
    {
        u8x64 result;
        result.lo = u8x32_set1(s);
        result.hi = u8x32_set1(s);
        return result;
    }

    static inline u8x64 u8x64_uload(const u8* source)
    {
        u8x64 result;
        result.lo = u8x32_uload(source + 0);
        result.hi = u8x32_uload(source + 32);
        return result;
    }

    static inline void u8x64_ustore(u8* dest, u8x64 a)
    {
        u8x32_ustore(dest + 0, a.lo);
        u8x32_ustore(dest + 32, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, unpacklo)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, unpackhi)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, add)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, sub)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, adds)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, subs)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, avg)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, ravg)

    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, bitwise_and)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, bitwise_or)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(u8x64, u8x64, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask8x64, u8x64, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask8x64, u8x64, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask8x64, u8x64, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask8x64, u8x64, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask8x64, u8x64, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask8x64, u8x64, compare_le)
    SIMD_COMPOSITE_SELECT(mask8x64, u8x64, select)

    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, min)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, max)

    // -----------------------------------------------------------------
    // u16x32
    // -----------------------------------------------------------------

    static inline u16x32 u16x32_zero()
    {
        u16x32 result;
        result.lo = u16x16_zero();
        result.hi = u16x16_zero();
        return result;
    }

    static inline u16x32 u16x32_set1(u16 s)
    {
        u16x32 result;
        result.lo = u16x16_set1(s);
        result.hi = u16x16_set1(s);
        return result;
    }

    static inline u16x32 u16x32_uload(const u16* source)
    {
        u16x32 result;
        result.lo = u16x16_uload(source + 0);
        result.hi = u16x16_uload(source + 16);
        return result;
    }

    static inline void u16x32_ustore(u16* dest, u16x32 a)
    {
        u16x16_ustore(dest + 0, a.lo);
        u16x16_ustore(dest + 16, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, unpacklo)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, unpackhi)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, add)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, sub)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, mullo)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, adds)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, subs)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, avg)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, ravg)

    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, bitwise_and)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, bitwise_or)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(u16x32, u16x32, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask16x32, u16x32, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask16x32, u16x32, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask16x32, u16x32, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask16x32, u16x32, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask16x32, u16x32, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask16x32, u16x32, compare_le)
    SIMD_COMPOSITE_SELECT(mask16x32, u16x32, select)

    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, min)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, max)

    // shift by constant

    template <int Count>
    static inline u16x32 slli(u16x32 a)
    {
        u16x32 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline u16x32 srli(u16x32 a)
    {
        u16x32 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline u16x32 srai(u16x32 a)
    {
        u16x32 result;
        result.lo = srai<Count>(a.lo);
        result.hi = srai<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline u16x32 sll(u16x32 a, int count)
    {
        u16x32 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline u16x32 srl(u16x32 a, int count)
    {
        u16x32 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    static inline u16x32 sra(u16x32 a, int count)
    {
        u16x32 result;
        result.lo = sra(a.lo, count);
        result.hi = sra(a.hi, count);
        return result;
    }

    // -----------------------------------------------------------------
    // u32x16
    // -----------------------------------------------------------------

    static inline u32x16 u32x16_zero()
    {
        u32x16 result;
        result.lo = u32x8_zero();
        result.hi = u32x8_zero();
        return result;
    }

    static inline u32x16 u32x16_set1(u32 s)
    {
        u32x16 result;
        result.lo = u32x8_set1(s);
        result.hi = u32x8_set1(s);
        return result;
    }

    static inline u32x16 u32x16_set16(u32 s0, u32 s1, u32 s2, u32 s3, u32 s4, u32 s5, u32 s6, u32 s7,
        u32 s8, u32 s9, u32 s10, u32 s11, u32 s12, u32 s13, u32 s14, u32 s15)
    {
        u32x16 result;
        result.lo = u32x8_set8(s0, s1, s2, s3, s4, s5, s6, s7);
        result.hi = u32x8_set8(s8, s9, s10, s11, s12, s13, s14, s15);
        return result;
    }

    static inline u32x16 u32x16_uload(const u32* source)
    {
        u32x16 result;
        result.lo = u32x8_uload(source + 0);
        result.hi = u32x8_uload(source + 8);
        return result;
    }

    static inline void u32x16_ustore(u32* dest, u32x16 a)
    {
        u32x8_ustore(dest + 0, a.lo);
        u32x8_ustore(dest + 8, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, unpacklo)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, unpackhi)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, add)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, sub)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, avg)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, ravg)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, mullo)

    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, bitwise_and)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, bitwise_or)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(u32x16, u32x16, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask32x16, u32x16, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask32x16, u32x16, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask32x16, u32x16, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask32x16, u32x16, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask32x16, u32x16, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask32x16, u32x16, compare_le)
    SIMD_COMPOSITE_SELECT(mask32x16, u32x16, select)

    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, min)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, max)

    // shift by constant

    template <int Count>
    static inline u32x16 slli(u32x16 a)
    {
        u32x16 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline u32x16 srli(u32x16 a)
    {
        u32x16 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline u32x16 srai(u32x16 a)
    {
        u32x16 result;
        result.lo = srai<Count>(a.lo);
        result.hi = srai<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline u32x16 sll(u32x16 a, int count)
    {
        u32x16 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline u32x16 srl(u32x16 a, int count)
    {
        u32x16 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    static inline u32x16 sra(u32x16 a, int count)
    {
        u32x16 result;
        result.lo = sra(a.lo, count);
        result.hi = sra(a.hi, count);
        return result;
    }

    // shift by vector

    static inline u32x16 sll(u32x16 a, u32x16 count)
    {
        a.lo = sll(a.lo, count.lo);
        a.hi = sll(a.hi, count.hi);
        return a;
    }

    static inline u32x16 srl(u32x16 a, u32x16 count)
    {
        a.lo = srl(a.lo, count.lo);
        a.hi = srl(a.hi, count.hi);
        return a;
    }

    static inline u32x16 sra(u32x16 a, u32x16 count)
    {
        a.lo = sra(a.lo, count.lo);
        a.hi = sra(a.hi, count.hi);
        return a;
    }

    // -----------------------------------------------------------------
    // u64x8
    // -----------------------------------------------------------------

    static inline u64x8 u64x8_zero()
    {
        u64x8 result;
        result.lo = u64x4_zero();
        result.hi = u64x4_zero();
        return result;
    }

    static inline u64x8 u64x8_set1(u64 s)
    {
        u64x8 result;
        result.lo = u64x4_set1(s);
        result.hi = u64x4_set1(s);
        return result;
    }

    static inline u64x8 u64x8_set8(u64 s0, u64 s1, u64 s2, u64 s3, u64 s4, u64 s5, u64 s6, u64 s7)
    {
        u64x8 result;
        result.lo = u64x4_set4(s0, s1, s2, s3);
        result.hi = u64x4_set4(s4, s5, s6, s7);
        return result;
    }

    static inline u64x8 u64x8_uload(const u64* source)
    {
        u64x8 result;
        result.lo = u64x4_uload(source + 0);
        result.hi = u64x4_uload(source + 4);
        return result;
    }

    static inline void u64x8_ustore(u64* dest, u64x8 a)
    {
        u64x4_ustore(dest + 0, a.lo);
        u64x4_ustore(dest + 4, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, unpacklo)
    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, unpackhi)
    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, add)
    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, sub)
    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, avg)
    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, ravg)

    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, bitwise_and)
    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, bitwise_or)
    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(u64x8, u64x8, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask64x8, u64x8, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask64x8, u64x8, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask64x8, u64x8, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask64x8, u64x8, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask64x8, u64x8, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask64x8, u64x8, compare_le)
    SIMD_COMPOSITE_SELECT(mask64x8, u64x8, select)

    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, min)
    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, max)

    // shift by constant

    template <int Count>
    static inline u64x8 slli(u64x8 a)
    {
        u64x8 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline u64x8 srli(u64x8 a)
    {
        u64x8 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline u64x8 sll(u64x8 a, int count)
    {
        u64x8 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline u64x8 srl(u64x8 a, int count)
    {
        u64x8 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    // -----------------------------------------------------------------
    // s8x64
    // -----------------------------------------------------------------

    static inline s8x64 s8x64_zero()
    {
        s8x64 result;
        result.lo = s8x32_zero();
        result.hi = s8x32_zero();
        return result;
    }

    static inline s8x64 s8x64_set1(s8 s)
    {
        s8x64 result;
        result.lo = s8x32_set1(s);
        result.hi = s8x32_set1(s);
        return result;
    }

    static inline s8x64 s8x64_uload(const s8* source)
    {
        s8x64 result;
        result.lo = s8x32_uload(source + 0);
        result.hi = s8x32_uload(source + 32);
        return result;
    }

    static inline void s8x64_ustore(s8* dest, s8x64 a)
    {
        s8x32_ustore(dest + 0, a.lo);
        s8x32_ustore(dest + 32, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, unpacklo)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, unpackhi)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, add)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, sub)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, adds)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, subs)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, avg)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, ravg)
    SIMD_COMPOSITE_FUNC1(s8x64, s8x64, abs)
    SIMD_COMPOSITE_FUNC1(s8x64, s8x64, neg)

    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, bitwise_and)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, bitwise_or)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(s8x64, s8x64, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask8x64, s8x64, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask8x64, s8x64, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask8x64, s8x64, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask8x64, s8x64, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask8x64, s8x64, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask8x64, s8x64, compare_le)
    SIMD_COMPOSITE_SELECT(mask8x64, s8x64, select)

    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, min)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, max)

    // -----------------------------------------------------------------
    // s16x32
    // -----------------------------------------------------------------

    static inline s16x32 s16x32_zero()
    {
        s16x32 result;
        result.lo = s16x16_zero();
        result.hi = s16x16_zero();
        return result;
    }

    static inline s16x32 s16x32_set1(s16 s)
    {
        s16x32 result;
        result.lo = s16x16_set1(s);
        result.hi = s16x16_set1(s);
        return result;
    }

    static inline s16x32 s16x32_uload(const s16* source)
    {
        s16x32 result;
        result.lo = s16x16_uload(source + 0);
        result.hi = s16x16_uload(source + 16);
        return result;
    }

    static inline void s16x32_ustore(s16* dest, s16x32 a)
    {
        s16x16_ustore(dest + 0, a.lo);
        s16x16_ustore(dest + 16, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, unpacklo)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, unpackhi)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, add)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, sub)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, mullo)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, adds)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, subs)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, avg)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, ravg)
    SIMD_COMPOSITE_FUNC1(s16x32, s16x32, abs)
    SIMD_COMPOSITE_FUNC1(s16x32, s16x32, neg)

    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, bitwise_and)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, bitwise_or)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(s16x32, s16x32, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask16x32, s16x32, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask16x32, s16x32, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask16x32, s16x32, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask16x32, s16x32, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask16x32, s16x32, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask16x32, s16x32, compare_le)
    SIMD_COMPOSITE_SELECT(mask16x32, s16x32, select)

    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, min)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, max)

    // shift by constant

    template <int Count>
    static inline s16x32 slli(s16x32 a)
    {
        s16x32 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline s16x32 srli(s16x32 a)
    {
        s16x32 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline s16x32 srai(s16x32 a)
    {
        s16x32 result;
        result.lo = srai<Count>(a.lo);
        result.hi = srai<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline s16x32 sll(s16x32 a, int count)
    {
        s16x32 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline s16x32 srl(s16x32 a, int count)
    {
        s16x32 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    static inline s16x32 sra(s16x32 a, int count)
    {
        s16x32 result;
        result.lo = sra(a.lo, count);
        result.hi = sra(a.hi, count);
        return result;
    }

    // -----------------------------------------------------------------
    // s32x16
    // -----------------------------------------------------------------

    static inline s32x16 s32x16_zero()
    {
        s32x16 result;
        result.lo = s32x8_zero();
        result.hi = s32x8_zero();
        return result;
    }

    static inline s32x16 s32x16_set1(s32 s)
    {
        s32x16 result;
        result.lo = s32x8_set1(s);
        result.hi = s32x8_set1(s);
        return result;
    }

    static inline s32x16 s32x16_set16(s32 v0, s32 v1, s32 v2, s32 v3, s32 v4, s32 v5, s32 v6, s32 v7,
        s32 v8, s32 v9, s32 v10, s32 v11, s32 v12, s32 v13, s32 v14, s32 v15)
    {
        s32x16 result;
        result.lo = s32x8_set8(v0, v1, v2, v3, v4, v5, v6, v7);
        result.hi = s32x8_set8(v8, v9, v10, v11, v12, v13, v14, v15);
        return result;
    }

    static inline s32x16 s32x16_uload(const s32* source)
    {
        s32x16 result;
        result.lo = s32x8_uload(source + 0);
        result.hi = s32x8_uload(source + 8);
        return result;
    }

    static inline void s32x16_ustore(s32* dest, s32x16 a)
    {
        s32x8_ustore(dest + 0, a.lo);
        s32x8_ustore(dest + 8, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, unpacklo)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, unpackhi)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, add)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, sub)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, avg)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, ravg)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, mullo)
    SIMD_COMPOSITE_FUNC1(s32x16, s32x16, abs)
    SIMD_COMPOSITE_FUNC1(s32x16, s32x16, neg)

    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, bitwise_and)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, bitwise_or)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(s32x16, s32x16, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask32x16, s32x16, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask32x16, s32x16, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask32x16, s32x16, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask32x16, s32x16, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask32x16, s32x16, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask32x16, s32x16, compare_le)
    SIMD_COMPOSITE_SELECT(mask32x16, s32x16, select)

    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, min)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, max)

    // shift by constant

    template <int Count>
    static inline s32x16 slli(s32x16 a)
    {
        s32x16 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline s32x16 srli(s32x16 a)
    {
        s32x16 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline s32x16 srai(s32x16 a)
    {
        s32x16 result;
        result.lo = srai<Count>(a.lo);
        result.hi = srai<Count>(a.hi);
        return result;
    }

    // shify by scalar

    static inline s32x16 sll(s32x16 a, int count)
    {
        s32x16 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline s32x16 srl(s32x16 a, int count)
    {
        s32x16 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    static inline s32x16 sra(s32x16 a, int count)
    {
        s32x16 result;
        result.lo = sra(a.lo, count);
        result.hi = sra(a.hi, count);
        return result;
    }

    // shift by vector

    static inline s32x16 sll(s32x16 a, u32x16 count)
    {
        a.lo = sll(a.lo, count.lo);
        a.hi = sll(a.hi, count.hi);
        return a;
    }

    static inline s32x16 srl(s32x16 a, u32x16 count)
    {
        a.lo = srl(a.lo, count.lo);
        a.hi = srl(a.hi, count.hi);
        return a;
    }

    static inline s32x16 sra(s32x16 a, u32x16 count)
    {
        a.lo = sra(a.lo, count.lo);
        a.hi = sra(a.hi, count.hi);
        return a;
    }

    // -----------------------------------------------------------------
    // s64x8
    // -----------------------------------------------------------------

    static inline s64x8 s64x8_zero()
    {
        s64x8 result;
        result.lo = s64x4_zero();
        result.hi = s64x4_zero();
        return result;
    }

    static inline s64x8 s64x8_set1(s64 s)
    {
        s64x8 result;
        result.lo = s64x4_set1(s);
        result.hi = s64x4_set1(s);
        return result;
    }

    static inline s64x8 s64x8_set8(s64 s0, s64 s1, s64 s2, s64 s3, s64 s4, s64 s5, s64 s6, s64 s7)
    {
        s64x8 result;
        result.lo = s64x4_set4(s0, s1, s2, s3);
        result.hi = s64x4_set4(s4, s5, s6, s7);
        return result;
    }

    static inline s64x8 s64x8_uload(const s64* source)
    {
        s64x8 result;
        result.lo = s64x4_uload(source + 0);
        result.hi = s64x4_uload(source + 4);
        return result;
    }

    static inline void s64x8_ustore(s64* dest, s64x8 a)
    {
        s64x4_ustore(dest + 0, a.lo);
        s64x4_ustore(dest + 4, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, unpacklo)
    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, unpackhi)
    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, add)
    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, sub)
    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, avg)
    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, ravg)

    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, bitwise_and)
    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, bitwise_or)
    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(s64x8, s64x8, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask64x8, s64x8, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask64x8, s64x8, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask64x8, s64x8, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask64x8, s64x8, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask64x8, s64x8, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask64x8, s64x8, compare_le)
    SIMD_COMPOSITE_SELECT(mask64x8, s64x8, select)

    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, min)
    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, max)

    // shift by constant

    template <int Count>
    static inline s64x8 slli(s64x8 a)
    {
        s64x8 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline s64x8 srli(s64x8 a)
    {
        s64x8 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline s64x8 sll(s64x8 a, int count)
    {
        s64x8 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline s64x8 srl(s64x8 a, int count)
    {
        s64x8 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    // -----------------------------------------------------------------
    // mask8x64
    // -----------------------------------------------------------------

    static inline mask8x64 operator & (mask8x64 a, mask8x64 b)
    {
        mask8x64 result;
        result.lo = a.lo & b.lo;
        result.hi = a.hi & b.hi;
        return result;
    }

    static inline mask8x64 operator | (mask8x64 a, mask8x64 b)
    {
        mask8x64 result;
        result.lo = a.lo | b.lo;
        result.hi = a.hi | b.hi;
        return result;
    }

    static inline mask8x64 operator ^ (mask8x64 a, mask8x64 b)
    {
        mask8x64 result;
        result.lo = a.lo ^ b.lo;
        result.hi = a.hi ^ b.hi;
        return result;
    }

    static inline mask8x64 operator ! (mask8x64 a)
    {
        mask8x64 result;
        result.lo = !a.lo;
        result.hi = !a.hi;
        return result;
    }

    static inline u64 get_mask(mask8x64 a)
    {
        u64 mask = get_mask(a.lo) | (u64(get_mask(a.hi)) << 32);
        return mask;
    }

    static inline bool none_of(mask8x64 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask8x64 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask8x64 a)
    {
        return get_mask(a) == 0xffffffffffffffffull;
    }

    // -----------------------------------------------------------------
    // mask16x32
    // -----------------------------------------------------------------

    static inline mask16x32 operator & (mask16x32 a, mask16x32 b)
    {
        mask16x32 result;
        result.lo = a.lo & b.lo;
        result.hi = a.hi & b.hi;
        return result;
    }

    static inline mask16x32 operator | (mask16x32 a, mask16x32 b)
    {
        mask16x32 result;
        result.lo = a.lo | b.lo;
        result.hi = a.hi | b.hi;
        return result;
    }

    static inline mask16x32 operator ^ (mask16x32 a, mask16x32 b)
    {
        mask16x32 result;
        result.lo = a.lo ^ b.lo;
        result.hi = a.hi ^ b.hi;
        return result;
    }

    static inline mask16x32 operator ! (mask16x32 a)
    {
        mask16x32 result;
        result.lo = !a.lo;
        result.hi = !a.hi;
        return result;
    }

    static inline u32 get_mask(mask16x32 a)
    {
        u32 mask = get_mask(a.lo) | (get_mask(a.hi) << 16);
        return mask;
    }

    static inline bool none_of(mask16x32 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask16x32 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask16x32 a)
    {
        return get_mask(a) == 0xffffffff;
    }

    // -----------------------------------------------------------------
    // mask32x16
    // -----------------------------------------------------------------

    static inline mask32x16 operator & (mask32x16 a, mask32x16 b)
    {
        mask32x16 result;
        result.lo = a.lo & b.lo;
        result.hi = a.hi & b.hi;
        return result;
    }

    static inline mask32x16 operator | (mask32x16 a, mask32x16 b)
    {
        mask32x16 result;
        result.lo = a.lo | b.lo;
        result.hi = a.hi | b.hi;
        return result;
    }

    static inline mask32x16 operator ^ (mask32x16 a, mask32x16 b)
    {
        mask32x16 result;
        result.lo = a.lo ^ b.lo;
        result.hi = a.hi ^ b.hi;
        return result;
    }

    static inline mask32x16 operator ! (mask32x16 a)
    {
        mask32x16 result;
        result.lo = !a.lo;
        result.hi = !a.hi;
        return result;
    }

    static inline u32 get_mask(mask32x16 a)
    {
        u32 mask = get_mask(a.lo) | (get_mask(a.hi) << 8);
        return mask;
    }

    static inline bool none_of(mask32x16 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask32x16 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask32x16 a)
    {
        return get_mask(a) == 0xffff;
    }

    // -----------------------------------------------------------------
    // mask64x8
    // -----------------------------------------------------------------

    static inline mask64x8 operator & (mask64x8 a, mask64x8 b)
    {
        mask64x8 result;
        result.lo = a.lo & b.lo;
        result.hi = a.hi & b.hi;
        return result;
    }

    static inline mask64x8 operator | (mask64x8 a, mask64x8 b)
    {
        mask64x8 result;
        result.lo = a.lo | b.lo;
        result.hi = a.hi | b.hi;
        return result;
    }

    static inline mask64x8 operator ^ (mask64x8 a, mask64x8 b)
    {
        mask64x8 result;
        result.lo = a.lo ^ b.lo;
        result.hi = a.hi ^ b.hi;
        return result;
    }

    static inline mask64x8 operator ! (mask64x8 a)
    {
        mask64x8 result;
        result.lo = !a.lo;
        result.hi = !a.hi;
        return result;
    }

    static inline u32 get_mask(mask64x8 a)
    {
        u32 mask = get_mask(a.lo) | (get_mask(a.hi) << 4);
        return mask;
    }

    static inline bool none_of(mask64x8 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask64x8 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask64x8 a)
    {
        return get_mask(a) == 0xff;
    }

#undef SIMD_COMPOSITE_FUNC1
#undef SIMD_COMPOSITE_FUNC2
#undef SIMD_COMPOSITE_SELECT

} // namespace simd
} // namespace mango
