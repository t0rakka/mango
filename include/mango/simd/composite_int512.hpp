/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // uint8x64
    // -----------------------------------------------------------------

    static inline uint8x64 uint8x64_zero()
    {
        uint8x64 result;
        result.lo = uint8x32_zero();
        result.hi = uint8x32_zero();
        return result;
    }

    static inline uint8x64 uint8x64_set1(u8 s)
    {
        uint8x64 result;
        result.lo = uint8x32_set1(s);
        result.hi = uint8x32_set1(s);
        return result;
    }

    static inline uint8x64 unpacklo(uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline uint8x64 unpackhi(uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline uint8x64 add(uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline uint8x64 sub(uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline uint8x64 adds(uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline uint8x64 subs(uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline uint8x64 bitwise_nand(uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline uint8x64 bitwise_and(uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline uint8x64 bitwise_or(uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline uint8x64 bitwise_xor(uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline uint8x64 bitwise_not(uint8x64 a)
    {
        uint8x64 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    // compare

    static inline mask8x64 compare_eq(uint8x64 a, uint8x64 b)
    {
        mask8x64 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline mask8x64 compare_gt(uint8x64 a, uint8x64 b)
    {
        mask8x64 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline uint8x64 select(mask8x64 mask, uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    static inline uint8x64 min(uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline uint8x64 max(uint8x64 a, uint8x64 b)
    {
        uint8x64 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // uint16x32
    // -----------------------------------------------------------------

    static inline uint16x32 uint16x32_zero()
    {
        uint16x32 result;
        result.lo = uint16x16_zero();
        result.hi = uint16x16_zero();
        return result;
    }

    static inline uint16x32 uint16x32_set1(u16 s)
    {
        uint16x32 result;
        result.lo = uint16x16_set1(s);
        result.hi = uint16x16_set1(s);
        return result;
    }

    static inline uint16x32 unpacklo(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline uint16x32 unpackhi(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline uint16x32 add(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline uint16x32 sub(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline uint16x32 mullo(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline uint16x32 adds(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline uint16x32 subs(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline uint16x32 bitwise_nand(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline uint16x32 bitwise_and(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline uint16x32 bitwise_or(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline uint16x32 bitwise_xor(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline uint16x32 bitwise_not(uint16x32 a)
    {
        uint16x32 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    // compare

    static inline mask16x32 compare_eq(uint16x32 a, uint16x32 b)
    {
        mask16x32 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline mask16x32 compare_gt(uint16x32 a, uint16x32 b)
    {
        mask16x32 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline uint16x32 select(mask16x32 mask, uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    // shift by constant

    template <int Count>
    static inline uint16x32 slli(uint16x32 a)
    {
        uint16x32 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline uint16x32 srli(uint16x32 a)
    {
        uint16x32 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline uint16x32 srai(uint16x32 a)
    {
        uint16x32 result;
        result.lo = srai<Count>(a.lo);
        result.hi = srai<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline uint16x32 sll(uint16x32 a, int count)
    {
        uint16x32 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline uint16x32 srl(uint16x32 a, int count)
    {
        uint16x32 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    static inline uint16x32 sra(uint16x32 a, int count)
    {
        uint16x32 result;
        result.lo = sra(a.lo, count);
        result.hi = sra(a.hi, count);
        return result;
    }

    static inline uint16x32 min(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline uint16x32 max(uint16x32 a, uint16x32 b)
    {
        uint16x32 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // uint32x16
    // -----------------------------------------------------------------

    static inline uint32x16 uint32x16_zero()
    {
        uint32x16 result;
        result.lo = uint32x8_zero();
        result.hi = uint32x8_zero();
        return result;
    }

    static inline uint32x16 uint32x16_set1(u32 s)
    {
        uint32x16 result;
        result.lo = uint32x8_set1(s);
        result.hi = uint32x8_set1(s);
        return result;
    }

    static inline uint32x16 uint32x16_set16(u32 s0, u32 s1, u32 s2, u32 s3, u32 s4, u32 s5, u32 s6, u32 s7,
        u32 s8, u32 s9, u32 s10, u32 s11, u32 s12, u32 s13, u32 s14, u32 s15)
    {
        uint32x16 result;
        result.lo = uint32x8_set8(s0, s1, s2, s3, s4, s5, s6, s7);
        result.hi = uint32x8_set8(s8, s9, s10, s11, s12, s13, s14, s15);
        return result;
    }

    static inline uint32x16 uint32x16_uload(const u32* source)
    {
        uint32x16 result;
        result.lo = uint32x8_uload(source + 0);
        result.hi = uint32x8_uload(source + 8);
        return result;
    }

    static inline void uint32x16_ustore(u32* dest, uint32x16 a)
    {
        uint32x8_ustore(dest + 0, a.lo);
        uint32x8_ustore(dest + 8, a.hi);
    }

    static inline uint32x16 unpacklo(uint32x16 a, uint32x16 b)
    {
        uint32x16 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline uint32x16 unpackhi(uint32x16 a, uint32x16 b)
    {
        uint32x16 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline uint32x16 add(uint32x16 a, uint32x16 b)
    {
        uint32x16 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline uint32x16 sub(uint32x16 a, uint32x16 b)
    {
        uint32x16 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline uint32x16 mullo(uint32x16 a, uint32x16 b)
    {
        uint32x16 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline uint32x16 bitwise_nand(uint32x16 a, uint32x16 b)
    {
        uint32x16 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline uint32x16 bitwise_and(uint32x16 a, uint32x16 b)
    {
        uint32x16 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline uint32x16 bitwise_or(uint32x16 a, uint32x16 b)
    {
        uint32x16 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline uint32x16 bitwise_xor(uint32x16 a, uint32x16 b)
    {
        uint32x16 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline uint32x16 bitwise_not(uint32x16 a)
    {
        uint32x16 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    // compare

    static inline mask32x16 compare_eq(uint32x16 a, uint32x16 b)
    {
        mask32x16 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline mask32x16 compare_gt(uint32x16 a, uint32x16 b)
    {
        mask32x16 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline uint32x16 select(mask32x16 mask, uint32x16 a, uint32x16 b)
    {
        uint32x16 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    // shift by constant

    template <int Count>
    static inline uint32x16 slli(uint32x16 a)
    {
        uint32x16 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline uint32x16 srli(uint32x16 a)
    {
        uint32x16 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline uint32x16 srai(uint32x16 a)
    {
        uint32x16 result;
        result.lo = srai<Count>(a.lo);
        result.hi = srai<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline uint32x16 sll(uint32x16 a, int count)
    {
        uint32x16 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline uint32x16 srl(uint32x16 a, int count)
    {
        uint32x16 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    static inline uint32x16 sra(uint32x16 a, int count)
    {
        uint32x16 result;
        result.lo = sra(a.lo, count);
        result.hi = sra(a.hi, count);
        return result;
    }

    // shift by vector

    static inline uint32x16 sll(uint32x16 a, uint32x16 count)
    {
        a.lo = sll(a.lo, count.lo);
        a.hi = sll(a.hi, count.hi);
        return a;
    }

    static inline uint32x16 srl(uint32x16 a, uint32x16 count)
    {
        a.lo = srl(a.lo, count.lo);
        a.hi = srl(a.hi, count.hi);
        return a;
    }

    static inline uint32x16 sra(uint32x16 a, uint32x16 count)
    {
        a.lo = sra(a.lo, count.lo);
        a.hi = sra(a.hi, count.hi);
        return a;
    }

    static inline uint32x16 min(uint32x16 a, uint32x16 b)
    {
        uint32x16 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline uint32x16 max(uint32x16 a, uint32x16 b)
    {
        uint32x16 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // uint64x8
    // -----------------------------------------------------------------

    static inline uint64x8 uint64x8_zero()
    {
        uint64x8 result;
        result.lo = uint64x4_zero();
        result.hi = uint64x4_zero();
        return result;
    }

    static inline uint64x8 uint64x8_set1(u64 s)
    {
        uint64x8 result;
        result.lo = uint64x4_set1(s);
        result.hi = uint64x4_set1(s);
        return result;
    }

    static inline uint64x8 uint64x8_set8(u64 s0, u64 s1, u64 s2, u64 s3, u64 s4, u64 s5, u64 s6, u64 s7)
    {
        uint64x8 result;
        result.lo = uint64x4_set4(s0, s1, s2, s3);
        result.hi = uint64x4_set4(s4, s5, s6, s7);
        return result;
    }

    static inline uint64x8 unpacklo(uint64x8 a, uint64x8 b)
    {
        uint64x8 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline uint64x8 unpackhi(uint64x8 a, uint64x8 b)
    {
        uint64x8 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline uint64x8 add(uint64x8 a, uint64x8 b)
    {
        uint64x8 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline uint64x8 sub(uint64x8 a, uint64x8 b)
    {
        uint64x8 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline uint64x8 bitwise_nand(uint64x8 a, uint64x8 b)
    {
        uint64x8 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline uint64x8 bitwise_and(uint64x8 a, uint64x8 b)
    {
        uint64x8 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline uint64x8 bitwise_or(uint64x8 a, uint64x8 b)
    {
        uint64x8 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline uint64x8 bitwise_xor(uint64x8 a, uint64x8 b)
    {
        uint64x8 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline uint64x8 bitwise_not(uint64x8 a)
    {
        uint64x8 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    static inline uint64x8 select(mask64x8 mask, uint64x8 a, uint64x8 b)
    {
        uint64x8 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    // shift by constant

    template <int Count>
    static inline uint64x8 slli(uint64x8 a)
    {
        uint64x8 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline uint64x8 srli(uint64x8 a)
    {
        uint64x8 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline uint64x8 sll(uint64x8 a, int count)
    {
        uint64x8 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline uint64x8 srl(uint64x8 a, int count)
    {
        uint64x8 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    // -----------------------------------------------------------------
    // int8x64
    // -----------------------------------------------------------------

    static inline int8x64 int8x64_zero()
    {
        int8x64 result;
        result.lo = int8x32_zero();
        result.hi = int8x32_zero();
        return result;
    }

    static inline int8x64 int8x64_set1(s8 s)
    {
        int8x64 result;
        result.lo = int8x32_set1(s);
        result.hi = int8x32_set1(s);
        return result;
    }

    static inline int8x64 unpacklo(int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline int8x64 unpackhi(int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline int8x64 add(int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline int8x64 sub(int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline int8x64 adds(int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline int8x64 subs(int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    static inline int8x64 abs(int8x64 a)
    {
        int8x64 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline int8x64 neg(int8x64 a)
    {
        int8x64 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    // bitwise

    static inline int8x64 bitwise_nand(int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline int8x64 bitwise_and(int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline int8x64 bitwise_or(int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline int8x64 bitwise_xor(int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline int8x64 bitwise_not(int8x64 a)
    {
        int8x64 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    // compare

    static inline mask8x64 compare_eq(int8x64 a, int8x64 b)
    {
        mask8x64 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline mask8x64 compare_gt(int8x64 a, int8x64 b)
    {
        mask8x64 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline int8x64 select(mask8x64 mask, int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    static inline int8x64 min(int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline int8x64 max(int8x64 a, int8x64 b)
    {
        int8x64 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // int16x32
    // -----------------------------------------------------------------

    static inline int16x32 int16x32_zero()
    {
        int16x32 result;
        result.lo = int16x16_zero();
        result.hi = int16x16_zero();
        return result;
    }

    static inline int16x32 int16x32_set1(s16 s)
    {
        int16x32 result;
        result.lo = int16x16_set1(s);
        result.hi = int16x16_set1(s);
        return result;
    }

    static inline int16x32 unpacklo(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline int16x32 unpackhi(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline int16x32 add(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline int16x32 sub(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline int16x32 mullo(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline int16x32 adds(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline int16x32 subs(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    static inline int16x32 abs(int16x32 a)
    {
        int16x32 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline int16x32 neg(int16x32 a)
    {
        int16x32 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    // bitwise

    static inline int16x32 bitwise_nand(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline int16x32 bitwise_and(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline int16x32 bitwise_or(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline int16x32 bitwise_xor(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline int16x32 bitwise_not(int16x32 a)
    {
        int16x32 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    // compare

    static inline mask16x32 compare_eq(int16x32 a, int16x32 b)
    {
        mask16x32 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline mask16x32 compare_gt(int16x32 a, int16x32 b)
    {
        mask16x32 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline int16x32 select(mask16x32 mask, int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    // shift by constant

    template <int Count>
    static inline int16x32 slli(int16x32 a)
    {
        int16x32 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline int16x32 srli(int16x32 a)
    {
        int16x32 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline int16x32 srai(int16x32 a)
    {
        int16x32 result;
        result.lo = srai<Count>(a.lo);
        result.hi = srai<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline int16x32 sll(int16x32 a, int count)
    {
        int16x32 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline int16x32 srl(int16x32 a, int count)
    {
        int16x32 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    static inline int16x32 sra(int16x32 a, int count)
    {
        int16x32 result;
        result.lo = sra(a.lo, count);
        result.hi = sra(a.hi, count);
        return result;
    }

    static inline int16x32 min(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline int16x32 max(int16x32 a, int16x32 b)
    {
        int16x32 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // int32x16
    // -----------------------------------------------------------------

    static inline int32x16 int32x16_zero()
    {
        int32x16 result;
        result.lo = int32x8_zero();
        result.hi = int32x8_zero();
        return result;
    }

    static inline int32x16 int32x16_set1(s32 s)
    {
        int32x16 result;
        result.lo = int32x8_set1(s);
        result.hi = int32x8_set1(s);
        return result;
    }

    static inline int32x16 int32x16_set16(s32 v0, s32 v1, s32 v2, s32 v3, s32 v4, s32 v5, s32 v6, s32 v7,
        s32 v8, s32 v9, s32 v10, s32 v11, s32 v12, s32 v13, s32 v14, s32 v15)
    {
        int32x16 result;
        result.lo = int32x8_set8(v0, v1, v2, v3, v4, v5, v6, v7);
        result.hi = int32x8_set8(v8, v9, v10, v11, v12, v13, v14, v15);
        return result;
    }

    static inline int32x16 int32x16_uload(const s32* source)
    {
        int32x16 result;
        result.lo = int32x8_uload(source + 0);
        result.hi = int32x8_uload(source + 8);
        return result;
    }

    static inline void int32x16_ustore(s32* dest, int32x16 a)
    {
        int32x8_ustore(dest + 0, a.lo);
        int32x8_ustore(dest + 8, a.hi);
    }

    static inline int32x16 unpacklo(int32x16 a, int32x16 b)
    {
        int32x16 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline int32x16 unpackhi(int32x16 a, int32x16 b)
    {
        int32x16 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline int32x16 abs(int32x16 a)
    {
        int32x16 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline int32x16 neg(int32x16 a)
    {
        int32x16 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    static inline int32x16 add(int32x16 a, int32x16 b)
    {
        int32x16 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline int32x16 sub(int32x16 a, int32x16 b)
    {
        int32x16 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline int32x16 mullo(int32x16 a, int32x16 b)
    {
        int32x16 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline int32x16 bitwise_nand(int32x16 a, int32x16 b)
    {
        int32x16 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline int32x16 bitwise_and(int32x16 a, int32x16 b)
    {
        int32x16 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline int32x16 bitwise_or(int32x16 a, int32x16 b)
    {
        int32x16 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline int32x16 bitwise_xor(int32x16 a, int32x16 b)
    {
        int32x16 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline int32x16 bitwise_not(int32x16 a)
    {
        int32x16 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    // compare

    static inline mask32x16 compare_eq(int32x16 a, int32x16 b)
    {
        mask32x16 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline mask32x16 compare_gt(int32x16 a, int32x16 b)
    {
        mask32x16 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline int32x16 select(mask32x16 mask, int32x16 a, int32x16 b)
    {
        int32x16 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    // shift by constant

    template <int Count>
    static inline int32x16 slli(int32x16 a)
    {
        int32x16 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline int32x16 srli(int32x16 a)
    {
        int32x16 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline int32x16 srai(int32x16 a)
    {
        int32x16 result;
        result.lo = srai<Count>(a.lo);
        result.hi = srai<Count>(a.hi);
        return result;
    }

    // shify by scalar

    static inline int32x16 sll(int32x16 a, int count)
    {
        int32x16 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline int32x16 srl(int32x16 a, int count)
    {
        int32x16 result;
        result.lo = srl(a.lo, count);
        result.hi = srl(a.hi, count);
        return result;
    }

    static inline int32x16 sra(int32x16 a, int count)
    {
        int32x16 result;
        result.lo = sra(a.lo, count);
        result.hi = sra(a.hi, count);
        return result;
    }

    // shift by vector

    static inline int32x16 sll(int32x16 a, uint32x16 count)
    {
        a.lo = sll(a.lo, count.lo);
        a.hi = sll(a.hi, count.hi);
        return a;
    }

    static inline int32x16 srl(int32x16 a, uint32x16 count)
    {
        a.lo = srl(a.lo, count.lo);
        a.hi = srl(a.hi, count.hi);
        return a;
    }

    static inline int32x16 sra(int32x16 a, uint32x16 count)
    {
        a.lo = sra(a.lo, count.lo);
        a.hi = sra(a.hi, count.hi);
        return a;
    }

    static inline int32x16 min(int32x16 a, int32x16 b)
    {
        int32x16 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline int32x16 max(int32x16 a, int32x16 b)
    {
        int32x16 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // int64x8
    // -----------------------------------------------------------------

    static inline int64x8 int64x8_zero()
    {
        int64x8 result;
        result.lo = int64x4_zero();
        result.hi = int64x4_zero();
        return result;
    }

    static inline int64x8 int64x8_set1(s64 s)
    {
        int64x8 result;
        result.lo = int64x4_set1(s);
        result.hi = int64x4_set1(s);
        return result;
    }

    static inline int64x8 int64x8_set8(s64 s0, s64 s1, s64 s2, s64 s3, s64 s4, s64 s5, s64 s6, s64 s7)
    {
        int64x8 result;
        result.lo = int64x4_set4(s0, s1, s2, s3);
        result.hi = int64x4_set4(s4, s5, s6, s7);
        return result;
    }

    static inline int64x8 unpacklo(int64x8 a, int64x8 b)
    {
        int64x8 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline int64x8 unpackhi(int64x8 a, int64x8 b)
    {
        int64x8 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline int64x8 add(int64x8 a, int64x8 b)
    {
        int64x8 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline int64x8 sub(int64x8 a, int64x8 b)
    {
        int64x8 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline int64x8 bitwise_nand(int64x8 a, int64x8 b)
    {
        int64x8 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline int64x8 bitwise_and(int64x8 a, int64x8 b)
    {
        int64x8 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline int64x8 bitwise_or(int64x8 a, int64x8 b)
    {
        int64x8 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline int64x8 bitwise_xor(int64x8 a, int64x8 b)
    {
        int64x8 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    static inline int64x8 bitwise_not(int64x8 a)
    {
        int64x8 result;
        result.lo = bitwise_not(a.lo);
        result.hi = bitwise_not(a.hi);
        return result;
    }

    static inline int64x8 select(mask64x8 mask, int64x8 a, int64x8 b)
    {
        int64x8 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    // shift by constant

    template <int Count>
    static inline int64x8 slli(int64x8 a)
    {
        int64x8 result;
        result.lo = slli<Count>(a.lo);
        result.hi = slli<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline int64x8 srli(int64x8 a)
    {
        int64x8 result;
        result.lo = srli<Count>(a.lo);
        result.hi = srli<Count>(a.hi);
        return result;
    }

    // shift by scalar

    static inline int64x8 sll(int64x8 a, int count)
    {
        int64x8 result;
        result.lo = sll(a.lo, count);
        result.hi = sll(a.hi, count);
        return result;
    }

    static inline int64x8 srl(int64x8 a, int count)
    {
        int64x8 result;
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

} // namespace simd
} // namespace mango
