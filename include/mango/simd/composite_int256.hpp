/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

#define SIMD_SET_COMPONENT(vec, value, mask, index) \
    if (index <= mask) vec.lo = set_component<index & mask>(vec.lo, value); \
    else               vec.hi = set_component<index & mask>(vec.hi, value)

#define SIMD_GET_COMPONENT(vec, mask, index) \
        Index <= mask ? get_component<index & mask>(vec.lo) \
                      : get_component<index & mask>(vec.hi)

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
    // u8x32
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u8x32 set_component(u8x32 a, u8 b)
    {
        static_assert(Index < 32, "Index out of range.");
        SIMD_SET_COMPONENT(a, b, 15, Index);
        return a;
    }

    template <unsigned int Index>
    static inline u8 get_component(u8x32 a)
    {
        static_assert(Index < 32, "Index out of range.");
        return SIMD_GET_COMPONENT(a, 15, Index);
    }

    static inline u8x32 u8x32_zero()
    {
        u8x32 result;
        result.lo = u8x16_zero();
        result.hi = u8x16_zero();
        return result;
    }

    static inline u8x32 u8x32_set1(u8 s)
    {
        u8x32 result;
        result.lo = u8x16_set1(s);
        result.hi = u8x16_set1(s);
        return result;
    }

    static inline u8x32 u8x32_uload(const u8* source)
    {
        u8x32 result;
        result.lo = u8x16_uload(source + 0);
        result.hi = u8x16_uload(source + 16);
        return result;
    }

    static inline void u8x32_ustore(u8* dest, u8x32 a)
    {
        u8x16_ustore(dest + 0, a.lo);
        u8x16_ustore(dest + 16, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, unpacklo)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, unpackhi)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, add)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, sub)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, adds)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, subs)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, avg)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, ravg)

    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, bitwise_and)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, bitwise_or)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(u8x32, u8x32, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask8x32, u8x32, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask8x32, u8x32, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask8x32, u8x32, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask8x32, u8x32, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask8x32, u8x32, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask8x32, u8x32, compare_le)
    SIMD_COMPOSITE_SELECT(mask8x32, u8x32, select)

    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, min)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, max)

    // -----------------------------------------------------------------
    // u16x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u16x16 set_component(u16x16 a, u16 b)
    {
        static_assert(Index < 16, "Index out of range.");
        SIMD_SET_COMPONENT(a, b, 7, Index);
        return a;
    }

    template <unsigned int Index>
    static inline u16 get_component(u16x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return SIMD_GET_COMPONENT(a, 7, Index);
    }

    static inline u16x16 u16x16_zero()
    {
        u16x16 result;
        result.lo = u16x8_zero();
        result.hi = u16x8_zero();
        return result;
    }

    static inline u16x16 u16x16_set1(u16 s)
    {
        u16x16 result;
        result.lo = u16x8_set1(s);
        result.hi = u16x8_set1(s);
        return result;
    }

    static inline u16x16 u16x16_uload(const u16* source)
    {
        u16x16 result;
        result.lo = u16x8_uload(source + 0);
        result.hi = u16x8_uload(source + 8);
        return result;
    }

    static inline void u16x16_ustore(u16* dest, u16x16 a)
    {
        u16x8_ustore(dest + 0, a.lo);
        u16x8_ustore(dest + 8, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, unpacklo)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, unpackhi)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, add)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, sub)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, mullo)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, adds)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, subs)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, avg)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, ravg)

    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, bitwise_and)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, bitwise_or)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(u16x16, u16x16, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask16x16, u16x16, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask16x16, u16x16, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask16x16, u16x16, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask16x16, u16x16, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask16x16, u16x16, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask16x16, u16x16, compare_le)
    SIMD_COMPOSITE_SELECT(mask16x16, u16x16, select)

    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, min)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, max)

    // shift by constant

    template <int Count>
    static inline u16x16 slli(u16x16 a)
    {
        u16x16 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline u16x16 srli(u16x16 a)
    {
        u16x16 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline u16x16 srai(u16x16 a)
    {
        u16x16 result;
        result.lo = srai<Count>(a.lo);
        result.hi = srai<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline u16x16 sll(u16x16 a, int count)
    {
        u16x16 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline u16x16 srl(u16x16 a, int count)
    {
        u16x16 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    static inline u16x16 sra(u16x16 a, int count)
    {
        u16x16 result;
        result.lo = sra(a.lo, count);
        result.hi = sra(a.hi, count);
        return result;
    }

    // -----------------------------------------------------------------
    // u32x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u32x8 set_component(u32x8 a, u32 b)
    {
        static_assert(Index < 8, "Index out of range.");
        SIMD_SET_COMPONENT(a, b, 3, Index);
        return a;
    }

    template <unsigned int Index>
    static inline u32 get_component(u32x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return SIMD_GET_COMPONENT(a, 3, Index);
    }

    static inline u32x8 u32x8_zero()
    {
        u32x8 result;
        result.lo = u32x4_zero();
        result.hi = u32x4_zero();
        return result;
    }

    static inline u32x8 u32x8_set1(u32 s)
    {
        u32x8 result;
        result.lo = u32x4_set1(s);
        result.hi = u32x4_set1(s);
        return result;
    }

    static inline u32x8 u32x8_set8(u32 s0, u32 s1, u32 s2, u32 s3, u32 s4, u32 s5, u32 s6, u32 s7)
    {
        u32x8 result;
        result.lo = u32x4_set4(s0, s1, s2, s3);
        result.hi = u32x4_set4(s4, s5, s6, s7);
        return result;
    }

    static inline u32x8 u32x8_uload(const u32* source)
    {
        u32x8 result;
        result.lo = u32x4_uload(source + 0);
        result.hi = u32x4_uload(source + 4);
        return result;
    }

    static inline void u32x8_ustore(u32* dest, u32x8 a)
    {
        u32x4_ustore(dest + 0, a.lo);
        u32x4_ustore(dest + 4, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, unpacklo)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, unpackhi)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, add)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, sub)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, mullo)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, adds)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, subs)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, avg)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, ravg)

    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, bitwise_and)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, bitwise_or)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(u32x8, u32x8, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask32x8, u32x8, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask32x8, u32x8, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask32x8, u32x8, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask32x8, u32x8, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask32x8, u32x8, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask32x8, u32x8, compare_le)
    SIMD_COMPOSITE_SELECT(mask32x8, u32x8, select)

    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, min)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, max)

    // shift by constant

    template <int Count>
    static inline u32x8 slli(u32x8 a)
    {
        u32x8 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline u32x8 srli(u32x8 a)
    {
        u32x8 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline u32x8 srai(u32x8 a)
    {
        u32x8 result;
        result.lo = srai<Count>(a.lo);
        result.hi = srai<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline u32x8 sll(u32x8 a, int count)
    {
        u32x8 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline u32x8 srl(u32x8 a, int count)
    {
        u32x8 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    static inline u32x8 sra(u32x8 a, int count)
    {
        u32x8 result;
        result.lo = sra(a.lo, count);
        result.hi = sra(a.hi, count);
        return result;
    }

    // shift by vector

    static inline u32x8 sll(u32x8 a, u32x8 count)
    {
        u32x8 result;
        result.lo = sll(a.lo, count.lo);
        result.hi = sll(a.hi, count.hi);
        return result;
    }

    static inline u32x8 srl(u32x8 a, u32x8 count)
    {
        u32x8 result;
        result.lo = srl(a.lo, count.lo);
        result.hi = srl(a.hi, count.hi);
        return result;
    }

    static inline u32x8 sra(u32x8 a, u32x8 count)
    {
        u32x8 result;
        result.lo = sra(a.lo, count.lo);
        result.hi = sra(a.hi, count.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // u64x4
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u64x4 set_component(u64x4 a, u64 b)
    {
        static_assert(Index < 4, "Index out of range.");
        SIMD_SET_COMPONENT(a, b, 1, Index);
        return a;
    }

    template <unsigned int Index>
    static inline u64 get_component(u64x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return SIMD_GET_COMPONENT(a, 1, Index);
    }

    static inline u64x4 u64x4_zero()
    {
        u64x4 result;
        result.lo = u64x2_zero();
        result.hi = u64x2_zero();
        return result;
    }

    static inline u64x4 u64x4_set1(u64 s)
    {
        u64x4 result;
        result.lo = u64x2_set1(s);
        result.hi = u64x2_set1(s);
        return result;
    }

    static inline u64x4 u64x4_set4(u64 x, u64 y, u64 z, u64 w)
    {
        u64x4 result;
        result.lo = u64x2_set2(x, y);
        result.hi = u64x2_set2(z, w);
        return result;
    }

    static inline u64x4 u64x4_uload(const u64* source)
    {
        u64x4 result;
        result.lo = u64x2_uload(source + 0);
        result.hi = u64x2_uload(source + 2);
        return result;
    }

    static inline void u64x4_ustore(u64* dest, u64x4 a)
    {
        u64x2_ustore(dest + 0, a.lo);
        u64x2_ustore(dest + 2, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, unpacklo)
    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, unpackhi)
    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, add)
    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, sub)
    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, avg)
    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, ravg)

    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, bitwise_and)
    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, bitwise_or)
    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(u64x4, u64x4, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask64x4, u64x4, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask64x4, u64x4, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask64x4, u64x4, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask64x4, u64x4, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask64x4, u64x4, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask64x4, u64x4, compare_le)
    SIMD_COMPOSITE_SELECT(mask64x4, u64x4, select)

    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, min)
    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, max)

    // shift by constant

    template <int Count>
    static inline u64x4 slli(u64x4 a)
    {
        u64x4 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline u64x4 srli(u64x4 a)
    {
        u64x4 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline u64x4 sll(u64x4 a, int count)
    {
        u64x4 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline u64x4 srl(u64x4 a, int count)
    {
        u64x4 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    // -----------------------------------------------------------------
    // s8x32
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s8x32 set_component(s8x32 a, s8 b)
    {
        static_assert(Index < 32, "Index out of range.");
        SIMD_SET_COMPONENT(a, b, 15, Index);
        return a;
    }

    template <unsigned int Index>
    static inline s8 get_component(s8x32 a)
    {
        static_assert(Index < 32, "Index out of range.");
        return SIMD_GET_COMPONENT(a, 15, Index);
    }

    static inline s8x32 s8x32_zero()
    {
        s8x32 result;
        result.lo = s8x16_zero();
        result.hi = s8x16_zero();
        return result;
    }

    static inline s8x32 s8x32_set1(s8 s)
    {
        s8x32 result;
        result.lo = s8x16_set1(s);
        result.hi = s8x16_set1(s);
        return result;
    }

    static inline s8x32 s8x32_uload(const s8* source)
    {
        s8x32 result;
        result.lo = s8x16_uload(source + 0);
        result.hi = s8x16_uload(source + 16);
        return result;
    }

    static inline void s8x32_ustore(s8* dest, s8x32 a)
    {
        s8x16_ustore(dest + 0, a.lo);
        s8x16_ustore(dest + 16, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, unpacklo)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, unpackhi)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, add)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, sub)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, adds)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, subs)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, avg)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, ravg)
    SIMD_COMPOSITE_FUNC1(s8x32, s8x32, abs)
    SIMD_COMPOSITE_FUNC1(s8x32, s8x32, neg)

    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, bitwise_and)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, bitwise_or)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(s8x32, s8x32, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask8x32, s8x32, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask8x32, s8x32, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask8x32, s8x32, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask8x32, s8x32, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask8x32, s8x32, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask8x32, s8x32, compare_le)
    SIMD_COMPOSITE_SELECT(mask8x32, s8x32, select)

    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, min)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, max)

    // -----------------------------------------------------------------
    // s16x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s16x16 set_component(s16x16 a, s16 b)
    {
        static_assert(Index < 16, "Index out of range.");
        SIMD_SET_COMPONENT(a, b, 7, Index);
        return a;
    }

    template <unsigned int Index>
    static inline s16 get_component(s16x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return SIMD_GET_COMPONENT(a, 7, Index);
    }

    static inline s16x16 s16x16_zero()
    {
        s16x16 result;
        result.lo = s16x8_zero();
        result.hi = s16x8_zero();
        return result;
    }

    static inline s16x16 s16x16_set1(s16 s)
    {
        s16x16 result;
        result.lo = s16x8_set1(s);
        result.hi = s16x8_set1(s);
        return result;
    }

    static inline s16x16 s16x16_uload(const s16* source)
    {
        s16x16 result;
        result.lo = s16x8_uload(source + 0);
        result.hi = s16x8_uload(source + 8);
        return result;
    }

    static inline void s16x16_ustore(s16* dest, s16x16 a)
    {
        s16x8_ustore(dest + 0, a.lo);
        s16x8_ustore(dest + 8, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, unpacklo)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, unpackhi)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, add)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, sub)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, adds)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, subs)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, hadd)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, hsub)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, hadds)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, hsubs)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, avg)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, ravg)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, mullo)
    SIMD_COMPOSITE_FUNC1(s16x16, s16x16, abs)
    SIMD_COMPOSITE_FUNC1(s16x16, s16x16, neg)

    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, bitwise_and)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, bitwise_or)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(s16x16, s16x16, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask16x16, s16x16, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask16x16, s16x16, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask16x16, s16x16, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask16x16, s16x16, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask16x16, s16x16, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask16x16, s16x16, compare_le)
    SIMD_COMPOSITE_SELECT(mask16x16, s16x16, select)

    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, min)
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, max)

    // shift by constant

    template <int Count>
    static inline s16x16 slli(s16x16 a)
    {
        s16x16 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline s16x16 srli(s16x16 a)
    {
        s16x16 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline s16x16 srai(s16x16 a)
    {
        s16x16 result;
        result.lo = srai<Count>(a.lo);
        result.hi = srai<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline s16x16 sll(s16x16 a, int count)
    {
        s16x16 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline s16x16 srl(s16x16 a, int count)
    {
        s16x16 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    static inline s16x16 sra(s16x16 a, int count)
    {
        s16x16 result;
        result.lo = sra(a.lo, count);
        result.hi = sra(a.hi, count);
        return result;
    }

    // -----------------------------------------------------------------
    // s32x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s32x8 set_component(s32x8 a, s32 b)
    {
        static_assert(Index < 8, "Index out of range.");
        SIMD_SET_COMPONENT(a, b, 3, Index);
        return a;
    }

    template <unsigned int Index>
    static inline s32 get_component(s32x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return SIMD_GET_COMPONENT(a, 3, Index);
    }

    static inline s32x8 s32x8_zero()
    {
        s32x8 result;
        result.lo = s32x4_zero();
        result.hi = s32x4_zero();
        return result;
    }

    static inline s32x8 s32x8_set1(s32 s)
    {
        s32x8 result;
        result.lo = s32x4_set1(s);
        result.hi = s32x4_set1(s);
        return result;
    }

    static inline s32x8 s32x8_set8(s32 s0, s32 s1, s32 s2, s32 s3, s32 s4, s32 s5, s32 s6, s32 s7)
    {
        s32x8 result;
        result.lo = s32x4_set4(s0, s1, s2, s3);
        result.hi = s32x4_set4(s4, s5, s6, s7);
        return result;
    }

    static inline s32x8 s32x8_uload(const s32* source)
    {
        s32x8 result;
        result.lo = s32x4_uload(source + 0);
        result.hi = s32x4_uload(source + 4);
        return result;
    }

    static inline void s32x8_ustore(s32* dest, s32x8 a)
    {
        s32x4_ustore(dest + 0, a.lo);
        s32x4_ustore(dest + 4, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, unpacklo)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, unpackhi)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, add)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, sub)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, adds)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, subs)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, hadd)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, hsub)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, avg)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, ravg)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, mullo)
    SIMD_COMPOSITE_FUNC1(s32x8, s32x8, abs)
    SIMD_COMPOSITE_FUNC1(s32x8, s32x8, neg)

    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, bitwise_and)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, bitwise_or)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(s32x8, s32x8, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask32x8, s32x8, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask32x8, s32x8, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask32x8, s32x8, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask32x8, s32x8, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask32x8, s32x8, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask32x8, s32x8, compare_le)
    SIMD_COMPOSITE_SELECT(mask32x8, s32x8, select)

    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, min)
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, max)

    // shift by constant

    template <int Count>
    static inline s32x8 slli(s32x8 a)
    {
        s32x8 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline s32x8 srli(s32x8 a)
    {
        s32x8 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline s32x8 srai(s32x8 a)
    {
        s32x8 result;
        result.lo = srai<Count>(a.lo);
        result.hi = srai<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline s32x8 sll(s32x8 a, int count)
    {
        s32x8 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline s32x8 srl(s32x8 a, int count)
    {
        s32x8 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    static inline s32x8 sra(s32x8 a, int count)
    {
        s32x8 result;
        result.lo = sra(a.lo, count);
        result.hi = sra(a.hi, count);
        return result;
    }

    // shift by vector

    static inline s32x8 sll(s32x8 a, u32x8 count)
    {
        s32x8 result;
        result.lo = sll(a.lo, count.lo);
        result.hi = sll(a.hi, count.hi);
        return result;
    }

    static inline s32x8 srl(s32x8 a, u32x8 count)
    {
        s32x8 result;
        result.lo = srl(a.lo, count.lo);
        result.hi = srl(a.hi, count.hi);
        return result;
    }

    static inline s32x8 sra(s32x8 a, u32x8 count)
    {
        s32x8 result;
        result.lo = sra(a.lo, count.lo);
        result.hi = sra(a.hi, count.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // s64x4
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s64x4 set_component(s64x4 a, s64 b)
    {
        static_assert(Index < 4, "Index out of range.");
        SIMD_SET_COMPONENT(a, b, 1, Index);
        return a;
    }

    template <unsigned int Index>
    static inline s64 get_component(s64x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return SIMD_GET_COMPONENT(a, 1, Index);
    }

    static inline s64x4 s64x4_zero()
    {
        s64x4 result;
        result.lo = s64x2_zero();
        result.hi = s64x2_zero();
        return result;
    }

    static inline s64x4 s64x4_set1(s64 s)
    {
        s64x4 result;
        result.lo = s64x2_set1(s);
        result.hi = s64x2_set1(s);
        return result;
    }

    static inline s64x4 s64x4_set4(s64 x, s64 y, s64 z, s64 w)
    {
        s64x4 result;
        result.lo = s64x2_set2(x, y);
        result.hi = s64x2_set2(z, w);
        return result;
    }

    static inline s64x4 s64x4_uload(const s64* source)
    {
        s64x4 result;
        result.lo = s64x2_uload(source + 0);
        result.hi = s64x2_uload(source + 2);
        return result;
    }

    static inline void s64x4_ustore(s64* dest, s64x4 a)
    {
        s64x2_ustore(dest + 0, a.lo);
        s64x2_ustore(dest + 2, a.hi);
    }

    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, unpacklo)
    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, unpackhi)
    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, add)
    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, sub)
    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, avg)
    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, ravg)

    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, bitwise_and)
    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, bitwise_or)
    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(s64x4, s64x4, bitwise_not)

    SIMD_COMPOSITE_FUNC2(mask64x4, s64x4, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask64x4, s64x4, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask64x4, s64x4, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask64x4, s64x4, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask64x4, s64x4, compare_ge)
    SIMD_COMPOSITE_FUNC2(mask64x4, s64x4, compare_le)
    SIMD_COMPOSITE_SELECT(mask64x4, s64x4, select)

    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, min)
    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, max)

    // shift by constant

    template <int Count>
    static inline s64x4 slli(s64x4 a)
    {
        s64x4 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline s64x4 srli(s64x4 a)
    {
        s64x4 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline s64x4 sll(s64x4 a, int count)
    {
        s64x4 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline s64x4 srl(s64x4 a, int count)
    {
        s64x4 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    // -----------------------------------------------------------------
    // mask8x32
    // -----------------------------------------------------------------

    static inline mask8x32 operator & (mask8x32 a, mask8x32 b)
    {
        mask8x32 result;
        result.lo = a.lo & b.lo;
        result.hi = a.hi & b.hi;
        return result;
    }

    static inline mask8x32 operator | (mask8x32 a, mask8x32 b)
    {
        mask8x32 result;
        result.lo = a.lo | b.lo;
        result.hi = a.hi | b.hi;
        return result;
    }

    static inline mask8x32 operator ^ (mask8x32 a, mask8x32 b)
    {
        mask8x32 result;
        result.lo = a.lo ^ b.lo;
        result.hi = a.hi ^ b.hi;
        return result;
    }

    static inline mask8x32 operator ! (mask8x32 a)
    {
        mask8x32 result;
        result.lo = !a.lo;
        result.hi = !a.hi;
        return result;
    }

    static inline u32 get_mask(mask8x32 a)
    {
        u32 mask = get_mask(a.lo) | (get_mask(a.hi) << 16);
        return mask;
    }

    static inline bool none_of(mask8x32 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask8x32 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask8x32 a)
    {
        return get_mask(a) == 0xffffffff;
    }

    // -----------------------------------------------------------------
    // mask16x16
    // -----------------------------------------------------------------

    static inline mask16x16 operator & (mask16x16 a, mask16x16 b)
    {
        mask16x16 result;
        result.lo = a.lo & b.lo;
        result.hi = a.hi & b.hi;
        return result;
    }

    static inline mask16x16 operator | (mask16x16 a, mask16x16 b)
    {
        mask16x16 result;
        result.lo = a.lo | b.lo;
        result.hi = a.hi | b.hi;
        return result;
    }

    static inline mask16x16 operator ^ (mask16x16 a, mask16x16 b)
    {
        mask16x16 result;
        result.lo = a.lo ^ b.lo;
        result.hi = a.hi ^ b.hi;
        return result;
    }

    static inline mask16x16 operator ! (mask16x16 a)
    {
        mask16x16 result;
        result.lo = !a.lo;
        result.hi = !a.hi;
        return result;
    }

    static inline u32 get_mask(mask16x16 a)
    {
        u32 mask = get_mask(a.lo) | (get_mask(a.hi) << 8);
        return mask;
    }

    static inline bool none_of(mask16x16 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask16x16 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask16x16 a)
    {
        return get_mask(a) == 0xffff;
    }

    // -----------------------------------------------------------------
    // mask32x8
    // -----------------------------------------------------------------

    static inline mask32x8 operator & (mask32x8 a, mask32x8 b)
    {
        mask32x8 result;
        result.lo = a.lo & b.lo;
        result.hi = a.hi & b.hi;
        return result;
    }

    static inline mask32x8 operator | (mask32x8 a, mask32x8 b)
    {
        mask32x8 result;
        result.lo = a.lo | b.lo;
        result.hi = a.hi | b.hi;
        return result;
    }

    static inline mask32x8 operator ^ (mask32x8 a, mask32x8 b)
    {
        mask32x8 result;
        result.lo = a.lo ^ b.lo;
        result.hi = a.hi ^ b.hi;
        return result;
    }

    static inline mask32x8 operator ! (mask32x8 a)
    {
        mask32x8 result;
        result.lo = !a.lo;
        result.hi = !a.hi;
        return result;
    }

    static inline u32 get_mask(mask32x8 a)
    {
        u32 mask = get_mask(a.lo) | (get_mask(a.hi) << 4);
        return mask;
    }

    static inline bool none_of(mask32x8 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask32x8 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask32x8 a)
    {
        return get_mask(a) == 0xff;
    }

    // -----------------------------------------------------------------
    // mask64x4
    // -----------------------------------------------------------------

    static inline mask64x4 operator & (mask64x4 a, mask64x4 b)
    {
        mask64x4 result;
        result.lo = a.lo & b.lo;
        result.hi = a.hi & b.hi;
        return result;
    }

    static inline mask64x4 operator | (mask64x4 a, mask64x4 b)
    {
        mask64x4 result;
        result.lo = a.lo | b.lo;
        result.hi = a.hi | b.hi;
        return result;
    }

    static inline mask64x4 operator ^ (mask64x4 a, mask64x4 b)
    {
        mask64x4 result;
        result.lo = a.lo ^ b.lo;
        result.hi = a.hi ^ b.hi;
        return result;
    }

    static inline mask64x4 operator ! (mask64x4 a)
    {
        mask64x4 result;
        result.lo = !a.lo;
        result.hi = !a.hi;
        return result;
    }

    static inline u32 get_mask(mask64x4 a)
    {
        u32 mask = get_mask(a.lo) | (get_mask(a.hi) << 2);
        return mask;
    }

    static inline bool none_of(mask64x4 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask64x4 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask64x4 a)
    {
        return get_mask(a) == 0xf;
    }

#undef SIMD_SET_COMPONENT
#undef SIMD_GET_COMPONENT
#undef SIMD_COMPOSITE_FUNC1
#undef SIMD_COMPOSITE_FUNC2
#undef SIMD_COMPOSITE_SELECT

} // namespace simd
} // namespace mango
