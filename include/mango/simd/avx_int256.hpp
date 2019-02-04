/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // helpers
    // -----------------------------------------------------------------

#define SET_COMPONENT(vec, value, mask, index) \
    if (index <= mask) vec.lo = set_component<index & mask>(vec.lo, value); \
    else               vec.hi = set_component<index & mask>(vec.hi, value)

#define GET_COMPONENT(vec, mask, index) \
        Index <= mask ? get_component<index & mask>(vec.lo) \
                      : get_component<index & mask>(vec.hi)

namespace detail {

    static inline mask8x16 get_low(mask8x32 a)
    {
        return _mm256_extractf128_si256(a, 0);
    }

    static inline mask8x16 get_high(mask8x32 a)
    {
        return _mm256_extractf128_si256(a, 1);
    }

    static inline mask16x8 get_low(mask16x16 a)
    {
        return _mm256_extractf128_si256(a, 0);
    }
    
    static inline mask16x8 get_high(mask16x16 a)
    {
        return _mm256_extractf128_si256(a, 1);
    }

    static inline mask32x4 get_low(mask32x8 a)
    {
        return _mm256_extractf128_si256(a, 0);
    }
    
    static inline mask32x4 get_high(mask32x8 a)
    {
        return _mm256_extractf128_si256(a, 1);
    }

    static inline mask64x2 get_low(mask64x4 a)
    {
        return _mm256_extractf128_si256(a, 0);
    }
    
    static inline mask64x2 get_high(mask64x4 a)
    {
        return _mm256_extractf128_si256(a, 1);
    }

    static inline mask8x32 combine(mask8x16 lo, mask8x16 hi)
    {
        return _mm256_insertf128_si256(_mm256_castsi128_si256(lo), hi, 1);
    }

    static inline mask16x16 combine(mask16x8 lo, mask16x8 hi)
    {
        return _mm256_insertf128_si256(_mm256_castsi128_si256(lo), hi, 1);
    }

    static inline mask32x8 combine(mask32x4 lo, mask32x4 hi)
    {
        return _mm256_insertf128_si256(_mm256_castsi128_si256(lo), hi, 1);
    }

} // namespace detail

    // -----------------------------------------------------------------
    // u8x32
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u8x32 set_component(u8x32 a, u8 b)
    {
        static_assert(Index < 32, "Index out of range.");
        SET_COMPONENT(a, b, 15, Index);
        return a;
    }

    template <unsigned int Index>
    static inline u8 get_component(u8x32 a)
    {
        static_assert(Index < 32, "Index out of range.");
        return GET_COMPONENT(a, 15, Index);
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

    static inline u8x32 unpacklo(u8x32 a, u8x32 b)
    {
        u8x32 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline u8x32 unpackhi(u8x32 a, u8x32 b)
    {
        u8x32 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline u8x32 add(u8x32 a, u8x32 b)
    {
        u8x32 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline u8x32 sub(u8x32 a, u8x32 b)
    {
        u8x32 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline u8x32 adds(u8x32 a, u8x32 b)
    {
        u8x32 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline u8x32 subs(u8x32 a, u8x32 b)
    {
        u8x32 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline u8x32 bitwise_nand(u8x32 a, u8x32 b)
    {
        u8x32 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline u8x32 bitwise_and(u8x32 a, u8x32 b)
    {
        u8x32 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline u8x32 bitwise_or(u8x32 a, u8x32 b)
    {
        u8x32 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline u8x32 bitwise_xor(u8x32 a, u8x32 b)
    {
        u8x32 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline u8x32 bitwise_not(u8x32 a)
    {
        u8x32 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    // compare

    static inline mask8x32 compare_eq(u8x32 a, u8x32 b)
    {
        mask8x16 lo = compare_eq(a.lo, b.lo);
        mask8x16 hi = compare_eq(a.hi, b.hi);
        return detail::combine(lo, hi);
    }

    static inline mask8x32 compare_gt(u8x32 a, u8x32 b)
    {
        mask8x16 lo = compare_gt(a.lo, b.lo);
        mask8x16 hi = compare_gt(a.hi, b.hi);
        return detail::combine(lo, hi);
    }

    static inline u8x32 select(mask8x32 mask, u8x32 a, u8x32 b)
    {
        u8x16 lo = select(detail::get_low (mask), a.lo, b.lo);
        u8x16 hi = select(detail::get_high(mask), a.hi, b.hi);
        return u8x32(lo, hi);
    }

    static inline u8x32 min(u8x32 a, u8x32 b)
    {
        u8x32 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline u8x32 max(u8x32 a, u8x32 b)
    {
        u8x32 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // u16x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u16x16 set_component(u16x16 a, u16 b)
    {
        static_assert(Index < 16, "Index out of range.");
        SET_COMPONENT(a, b, 7, Index);
        return a;
    }

    template <unsigned int Index>
    static inline u16 get_component(u16x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return GET_COMPONENT(a, 7, Index);
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

    static inline u16x16 unpacklo(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline u16x16 unpackhi(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline u16x16 add(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline u16x16 sub(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline u16x16 mullo(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline u16x16 adds(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline u16x16 subs(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline u16x16 bitwise_nand(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline u16x16 bitwise_and(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline u16x16 bitwise_or(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline u16x16 bitwise_xor(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline u16x16 bitwise_not(u16x16 a)
    {
        u16x16 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    // compare

    static inline mask16x16 compare_eq(u16x16 a, u16x16 b)
    {
        mask16x8 lo = compare_eq(a.lo, b.lo);
        mask16x8 hi = compare_eq(a.hi, b.hi);
        return detail::combine(lo, hi);
    }

    static inline mask16x16 compare_gt(u16x16 a, u16x16 b)
    {
        mask16x8 lo = compare_gt(a.lo, b.lo);
        mask16x8 hi = compare_gt(a.hi, b.hi);
        return detail::combine(lo, hi);
    }

    static inline u16x16 select(mask16x16 mask, u16x16 a, u16x16 b)
    {
        u16x8 lo = select(detail::get_low (mask), a.lo, b.lo);
        u16x8 hi = select(detail::get_high(mask), a.hi, b.hi);
        return u16x16(lo, hi);
    }

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

    static inline u16x16 min(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline u16x16 max(u16x16 a, u16x16 b)
    {
        u16x16 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // u32x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u32x8 set_component(u32x8 a, u32 b)
    {
        static_assert(Index < 8, "Index out of range.");
        SET_COMPONENT(a, b, 3, Index);
        return a;
    }

    template <unsigned int Index>
    static inline u32 get_component(u32x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return GET_COMPONENT(a, 3, Index);
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

    static inline u32x8 unpacklo(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline u32x8 unpackhi(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline u32x8 add(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline u32x8 sub(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline u32x8 mullo(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline u32x8 adds(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline u32x8 subs(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline u32x8 bitwise_nand(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline u32x8 bitwise_and(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline u32x8 bitwise_or(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline u32x8 bitwise_xor(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline u32x8 bitwise_not(u32x8 a)
    {
        u32x8 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    // compare

    static inline mask32x8 compare_eq(u32x8 a, u32x8 b)
    {
        mask32x4 lo = compare_eq(a.lo, b.lo);
        mask32x4 hi = compare_eq(a.hi, b.hi);
        return detail::combine(lo, hi);
    }

    static inline mask32x8 compare_gt(u32x8 a, u32x8 b)
    {
        mask32x4 lo = compare_gt(a.lo, b.lo);
        mask32x4 hi = compare_gt(a.hi, b.hi);
        return detail::combine(lo, hi);
    }

    static inline u32x8 select(mask32x8 mask, u32x8 a, u32x8 b)
    {
        u32x4 lo = select(detail::get_low (mask), a.lo, b.lo);
        u32x4 hi = select(detail::get_high(mask), a.hi, b.hi);
        return u32x8(lo, hi);
    }

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

    static inline u32x8 min(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline u32x8 max(u32x8 a, u32x8 b)
    {
        u32x8 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // u64x4
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline u64x4 set_component(u64x4 a, u64 b)
    {
        static_assert(Index < 4, "Index out of range.");
        SET_COMPONENT(a, b, 1, Index);
        return a;
    }

    template <unsigned int Index>
    static inline u64 get_component(u64x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return GET_COMPONENT(a, 1, Index);
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

    static inline u64x4 unpacklo(u64x4 a, u64x4 b)
    {
        u64x4 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline u64x4 unpackhi(u64x4 a, u64x4 b)
    {
        u64x4 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline u64x4 add(u64x4 a, u64x4 b)
    {
        u64x4 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline u64x4 sub(u64x4 a, u64x4 b)
    {
        u64x4 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline u64x4 bitwise_nand(u64x4 a, u64x4 b)
    {
        u64x4 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline u64x4 bitwise_and(u64x4 a, u64x4 b)
    {
        u64x4 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline u64x4 bitwise_or(u64x4 a, u64x4 b)
    {
        u64x4 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline u64x4 bitwise_xor(u64x4 a, u64x4 b)
    {
        u64x4 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline u64x4 bitwise_not(u64x4 a)
    {
        u64x4 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    static inline u64x4 select(mask64x4 mask, u64x4 a, u64x4 b)
    {
        u64x2 lo = select(detail::get_low (mask), a.lo, b.lo);
        u64x2 hi = select(detail::get_high(mask), a.hi, b.hi);
        return u64x4(lo, hi);
    }

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
        SET_COMPONENT(a, b, 15, Index);
        return a;
    }

    template <unsigned int Index>
    static inline s8 get_component(s8x32 a)
    {
        static_assert(Index < 32, "Index out of range.");
        return GET_COMPONENT(a, 15, Index);
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

    static inline s8x32 unpacklo(s8x32 a, s8x32 b)
    {
        s8x32 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline s8x32 unpackhi(s8x32 a, s8x32 b)
    {
        s8x32 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline s8x32 add(s8x32 a, s8x32 b)
    {
        s8x32 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline s8x32 sub(s8x32 a, s8x32 b)
    {
        s8x32 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline s8x32 adds(s8x32 a, s8x32 b)
    {
        s8x32 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline s8x32 subs(s8x32 a, s8x32 b)
    {
        s8x32 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    static inline s8x32 abs(s8x32 a)
    {
        s8x32 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline s8x32 neg(s8x32 a)
    {
        s8x32 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    // bitwise

    static inline s8x32 bitwise_nand(s8x32 a, s8x32 b)
    {
        s8x32 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline s8x32 bitwise_and(s8x32 a, s8x32 b)
    {
        s8x32 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline s8x32 bitwise_or(s8x32 a, s8x32 b)
    {
        s8x32 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline s8x32 bitwise_xor(s8x32 a, s8x32 b)
    {
        s8x32 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline s8x32 bitwise_not(s8x32 a)
    {
        s8x32 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    // compare

    static inline mask8x32 compare_eq(s8x32 a, s8x32 b)
    {
        mask8x16 lo = compare_eq(a.lo, b.lo);
        mask8x16 hi = compare_eq(a.hi, b.hi);
        return detail::combine(lo, hi);
    }

    static inline mask8x32 compare_gt(s8x32 a, s8x32 b)
    {
        mask8x16 lo = compare_gt(a.lo, b.lo);
        mask8x16 hi = compare_gt(a.hi, b.hi);
        return detail::combine(lo, hi);
    }

    static inline s8x32 select(mask8x32 mask, s8x32 a, s8x32 b)
    {
        s8x16 lo = select(detail::get_low (mask), a.lo, b.lo);
        s8x16 hi = select(detail::get_high(mask), a.hi, b.hi);
        return s8x32(lo, hi);
    }

    static inline s8x32 min(s8x32 a, s8x32 b)
    {
        s8x16 lo = min(a.lo, b.lo);
        s8x16 hi = min(a.hi, b.hi);
        return s8x32(lo, hi);
    }

    static inline s8x32 max(s8x32 a, s8x32 b)
    {
        s8x16 lo = max(a.lo, b.lo);
        s8x16 hi = max(a.hi, b.hi);
        return s8x32(lo, hi);
    }

    // -----------------------------------------------------------------
    // s16x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s16x16 set_component(s16x16 a, s16 b)
    {
        static_assert(Index < 16, "Index out of range.");
        SET_COMPONENT(a, b, 7, Index);
        return a;
    }

    template <unsigned int Index>
    static inline s16 get_component(s16x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return GET_COMPONENT(a, 7, Index);
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

    static inline s16x16 unpacklo(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline s16x16 unpackhi(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline s16x16 add(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline s16x16 sub(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline s16x16 mullo(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline s16x16 adds(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline s16x16 subs(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    static inline s16x16 abs(s16x16 a)
    {
        s16x16 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline s16x16 neg(s16x16 a)
    {
        s16x16 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    // bitwise

    static inline s16x16 bitwise_nand(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline s16x16 bitwise_and(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline s16x16 bitwise_or(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline s16x16 bitwise_xor(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline s16x16 bitwise_not(s16x16 a)
    {
        s16x16 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    // compare

    static inline mask16x16 compare_eq(s16x16 a, s16x16 b)
    {
        mask16x8 lo = compare_eq(a.lo, b.lo);
        mask16x8 hi = compare_eq(a.hi, b.hi);
        return detail::combine(lo, hi);
    }

    static inline mask16x16 compare_gt(s16x16 a, s16x16 b)
    {
        mask16x8 lo = compare_gt(a.lo, b.lo);
        mask16x8 hi = compare_gt(a.hi, b.hi);
        return detail::combine(lo, hi);
    }

    static inline s16x16 select(mask16x16 mask, s16x16 a, s16x16 b)
    {
        s16x8 lo = select(detail::get_low (mask), a.lo, b.lo);
        s16x8 hi = select(detail::get_high(mask), a.hi, b.hi);
        return s16x16(lo, hi);
    }

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

    static inline s16x16 min(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline s16x16 max(s16x16 a, s16x16 b)
    {
        s16x16 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // s32x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s32x8 set_component(s32x8 a, s32 b)
    {
        static_assert(Index < 8, "Index out of range.");
        SET_COMPONENT(a, b, 3, Index);
        return a;
    }

    template <unsigned int Index>
    static inline s32 get_component(s32x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return GET_COMPONENT(a, 3, Index);
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

    static inline s32x8 unpacklo(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline s32x8 unpackhi(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline s32x8 abs(s32x8 a)
    {
        s32x8 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline s32x8 neg(s32x8 a)
    {
        s32x8 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    static inline s32x8 add(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline s32x8 sub(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline s32x8 mullo(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline s32x8 adds(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline s32x8 subs(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline s32x8 bitwise_nand(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline s32x8 bitwise_and(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline s32x8 bitwise_or(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline s32x8 bitwise_xor(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline s32x8 bitwise_not(s32x8 a)
    {
        s32x8 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    // compare

    static inline mask32x8 compare_eq(s32x8 a, s32x8 b)
    {
        mask32x4 lo = compare_eq(a.lo, b.lo);
        mask32x4 hi = compare_eq(a.hi, b.hi);
        return detail::combine(lo, hi);
    }

    static inline mask32x8 compare_gt(s32x8 a, s32x8 b)
    {
        mask32x4 lo = compare_gt(a.lo, b.lo);
        mask32x4 hi = compare_gt(a.hi, b.hi);
        return detail::combine(lo, hi);
    }

    static inline s32x8 select(mask32x8 mask, s32x8 a, s32x8 b)
    {
        s32x4 lo = select(detail::get_low (mask), a.lo, b.lo);
        s32x4 hi = select(detail::get_high(mask), a.hi, b.hi);
        return s32x8(lo, hi);
    }

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

    static inline s32x8 min(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline s32x8 max(s32x8 a, s32x8 b)
    {
        s32x8 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // s64x4
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline s64x4 set_component(s64x4 a, s64 b)
    {
        static_assert(Index < 4, "Index out of range.");
        SET_COMPONENT(a, b, 1, Index);
        return a;
    }

    template <unsigned int Index>
    static inline s64 get_component(s64x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return GET_COMPONENT(a, 1, Index);
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

    static inline s64x4 unpacklo(s64x4 a, s64x4 b)
    {
        s64x4 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline s64x4 unpackhi(s64x4 a, s64x4 b)
    {
        s64x4 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline s64x4 add(s64x4 a, s64x4 b)
    {
        s64x4 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline s64x4 sub(s64x4 a, s64x4 b)
    {
        s64x4 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline s64x4 bitwise_nand(s64x4 a, s64x4 b)
    {
        s64x4 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline s64x4 bitwise_and(s64x4 a, s64x4 b)
    {
        s64x4 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline s64x4 bitwise_or(s64x4 a, s64x4 b)
    {
        s64x4 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline s64x4 bitwise_xor(s64x4 a, s64x4 b)
    {
        s64x4 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline s64x4 bitwise_not(s64x4 a)
    {
        s64x4 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    static inline s64x4 select(mask64x4 mask, s64x4 a, s64x4 b)
    {
        s64x2 lo = select(detail::get_low (mask), a.lo, b.lo);
        s64x2 hi = select(detail::get_high(mask), a.hi, b.hi);
        return s64x4(lo, hi);
    }

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
        return _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
    }

    static inline mask8x32 operator | (mask8x32 a, mask8x32 b)
    {
        return _mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
    }

    static inline mask8x32 operator ^ (mask8x32 a, mask8x32 b)
    {
        return _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
    }

    static inline u32 get_mask(mask8x32 a)
    {
        return _mm256_movemask_ps(_mm256_castsi256_ps(a));
    }

    static inline bool none_of(mask8x32 a)
    {
        return _mm256_testz_si256(a, a) != 0;
    }

    static inline bool any_of(mask8x32 a)
    {
        return _mm256_testz_si256(a, a) == 0;
    }

    static inline bool all_of(mask8x32 a)
    {
        return _mm256_testc_si256(a, _mm256_set1_epi8(0xff));
    }

    // -----------------------------------------------------------------
    // mask16x16
    // -----------------------------------------------------------------

    static inline mask16x16 operator & (mask16x16 a, mask16x16 b)
    {
        return _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
    }

    static inline mask16x16 operator | (mask16x16 a, mask16x16 b)
    {
        return _mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
    }

    static inline mask16x16 operator ^ (mask16x16 a, mask16x16 b)
    {
        return _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
    }

    static inline u32 get_mask(mask16x16 a)
    {
        u32 mask = get_mask(detail::get_low(a)) | (get_mask(detail::get_high(a)) << 8);
        return mask;
    }

    static inline bool none_of(mask16x16 a)
    {
        return _mm256_testz_si256(a, a) != 0;
    }

    static inline bool any_of(mask16x16 a)
    {
        return _mm256_testz_si256(a, a) == 0;
    }

    static inline bool all_of(mask16x16 a)
    {
        return _mm256_testc_si256(a, _mm256_set1_epi16(0xffff));
    }

    // -----------------------------------------------------------------
    // mask32x8
    // -----------------------------------------------------------------

    static inline mask32x8 operator & (mask32x8 a, mask32x8 b)
    {
        return _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
    }

    static inline mask32x8 operator | (mask32x8 a, mask32x8 b)
    {
        return _mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
    }

    static inline mask32x8 operator ^ (mask32x8 a, mask32x8 b)
    {
        return _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
    }

    static inline u32 get_mask(mask32x8 a)
    {
        u32 mask = get_mask(detail::get_low(a)) | (get_mask(detail::get_high(a)) << 4);
        return mask;
    }

    static inline bool none_of(mask32x8 a)
    {
        return _mm256_testz_si256(a, a) != 0;
    }

    static inline bool any_of(mask32x8 a)
    {
        return _mm256_testz_si256(a, a) == 0;
    }

    static inline bool all_of(mask32x8 a)
    {
        return _mm256_testc_si256(a, _mm256_set1_epi32(0xffffffff));
    }

    // -----------------------------------------------------------------
    // mask64x4
    // -----------------------------------------------------------------

    static inline mask64x4 operator & (mask64x4 a, mask64x4 b)
    {
        return _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
    }

    static inline mask64x4 operator | (mask64x4 a, mask64x4 b)
    {
        return _mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
    }

    static inline mask64x4 operator ^ (mask64x4 a, mask64x4 b)
    {
        return _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
    }

    static inline u32 get_mask(mask64x4 a)
    {
        u32 mask = get_mask(detail::get_low(a)) | (get_mask(detail::get_high(a)) << 2);
        return mask;
    }

    static inline bool none_of(mask64x4 a)
    {
        return _mm256_testz_si256(a, a) != 0;
    }

    static inline bool any_of(mask64x4 a)
    {
        return _mm256_testz_si256(a, a) == 0;
    }

    static inline bool all_of(mask64x4 a)
    {
        return _mm256_testc_si256(a, _mm256_set1_epi32(0xffffffff));
    }

#undef SET_COMPONENT
#undef GET_COMPONENT

} // namespace simd
} // namespace mango
