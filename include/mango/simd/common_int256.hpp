/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // uint8x32
    // -----------------------------------------------------------------

    static inline uint8x32 uint8x32_zero()
    {
        uint8x32 result;
        result.lo = uint8x16_zero();
        result.hi = uint8x16_zero();
        return result;
    }

    static inline uint8x32 uint8x32_set1(uint8 s)
    {
        uint8x32 result;
        result.lo = uint8x16_set1(s);
        result.hi = uint8x16_set1(s);
        return result;
    }

    static inline uint8x32 unpacklo(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline uint8x32 unpackhi(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline uint8x32 add(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline uint8x32 sub(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline uint8x32 mullo(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline uint8x32 adds(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline uint8x32 subs(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline uint8x32 bitwise_nand(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline uint8x32 bitwise_and(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline uint8x32 bitwise_or(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline uint8x32 bitwise_xor(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    // compare

    static inline uint8x32 compare_eq(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline uint8x32 compare_gt(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline uint8x32 select(uint8x32 mask, uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    static inline uint8x32 min(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline uint8x32 max(uint8x32 a, uint8x32 b)
    {
        uint8x32 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // uint16x16
    // -----------------------------------------------------------------

    static inline uint16x16 uint16x16_zero()
    {
        uint16x16 result;
        result.lo = uint16x8_zero();
        result.hi = uint16x8_zero();
        return result;
    }

    static inline uint16x16 uint16x16_set1(uint16 s)
    {
        uint16x16 result;
        result.lo = uint16x8_set1(s);
        result.hi = uint16x8_set1(s);
        return result;
    }

    static inline uint16x16 unpacklo(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline uint16x16 unpackhi(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline uint16x16 add(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline uint16x16 sub(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline uint16x16 mullo(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline uint16x16 adds(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline uint16x16 subs(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline uint16x16 bitwise_nand(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline uint16x16 bitwise_and(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline uint16x16 bitwise_or(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline uint16x16 bitwise_xor(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    // compare

    static inline uint16x16 compare_eq(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline uint16x16 compare_gt(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline uint16x16 select(uint16x16 mask, uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    static inline uint16x16 min(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline uint16x16 max(uint16x16 a, uint16x16 b)
    {
        uint16x16 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // uint32x8
    // -----------------------------------------------------------------

    static inline uint32x8 uint32x8_zero()
    {
        uint32x8 result;
        result.lo = uint32x4_zero();
        result.hi = uint32x4_zero();
        return result;
    }

    static inline uint32x8 uint32x8_set1(uint32 s)
    {
        uint32x8 result;
        result.lo = uint32x4_set1(s);
        result.hi = uint32x4_set1(s);
        return result;
    }

    static inline uint32x8 uint32x8_set8(uint32 s0, uint32 s1, uint32 s2, uint32 s3, uint32 s4, uint32 s5, uint32 s6, uint32 s7)
    {
        uint32x8 result;
        result.lo = uint32x4_set4(s0, s1, s2, s3);
        result.hi = uint32x4_set4(s4, s5, s6, s7);
        return result;
    }

    static inline uint32x8 uint32x8_uload(const uint32* source)
    {
        uint32x8 result;
        result.lo = uint32x4_uload(source + 0);
        result.hi = uint32x4_uload(source + 4);
        return result;
    }

    static inline void uint32x8_ustore(uint32* dest, uint32x8 a)
    {
        uint32x4_ustore(dest + 0, a.lo);
        uint32x4_ustore(dest + 4, a.hi);
    }

    static inline uint32x8 unpacklo(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline uint32x8 unpackhi(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline uint32x8 add(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline uint32x8 sub(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline uint32x8 mullo(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline uint32x8 adds(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline uint32x8 subs(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline uint32x8 bitwise_nand(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline uint32x8 bitwise_and(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline uint32x8 bitwise_or(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline uint32x8 bitwise_xor(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    // shift

    template <int Count> 
    static inline uint32x8 sll(uint32x8 a)
    {
        uint32x8 result;
        result.lo = sll<Count>(a.lo);
        result.hi = sll<Count>(a.hi);
        return result;
    }

    template <int Count> 
    static inline uint32x8 srl(uint32x8 a)
    {
        uint32x8 result;
        result.lo = srl<Count>(a.lo);
        result.hi = srl<Count>(a.hi);
        return result;
    }

    template <int Count> 
    static inline uint32x8 sra(uint32x8 a)
    {
        uint32x8 result;
        result.lo = sra<Count>(a.lo);
        result.hi = sra<Count>(a.hi);
        return result;
    }

    // compare

    static inline uint32x8 compare_eq(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline uint32x8 compare_gt(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline uint32x8 select(uint32x8 mask, uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    static inline uint32x8 min(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline uint32x8 max(uint32x8 a, uint32x8 b)
    {
        uint32x8 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // int8x32
    // -----------------------------------------------------------------

    static inline int8x32 int8x32_zero()
    {
        int8x32 result;
        result.lo = int8x16_zero();
        result.hi = int8x16_zero();
        return result;
    }

    static inline int8x32 int8x32_set1(int8 s)
    {
        int8x32 result;
        result.lo = int8x16_set1(s);
        result.hi = int8x16_set1(s);
        return result;
    }

    static inline int8x32 unpacklo(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline int8x32 unpackhi(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline int8x32 add(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline int8x32 sub(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline int8x32 mullo(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline int8x32 adds(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline int8x32 subs(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    static inline int8x32 abs(int8x32 a)
    {
        int8x32 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline int8x32 neg(int8x32 a)
    {
        int8x32 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    // bitwise

    static inline int8x32 bitwise_nand(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline int8x32 bitwise_and(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline int8x32 bitwise_or(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline int8x32 bitwise_xor(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    // compare

    static inline int8x32 compare_eq(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline int8x32 compare_gt(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline int8x32 select(int8x32 mask, int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    static inline int8x32 min(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline int8x32 max(int8x32 a, int8x32 b)
    {
        int8x32 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // int16x16
    // -----------------------------------------------------------------

    static inline int16x16 int16x16_zero()
    {
        int16x16 result;
        result.lo = int16x8_zero();
        result.hi = int16x8_zero();
        return result;
    }

    static inline int16x16 int16x16_set1(int16 s)
    {
        int16x16 result;
        result.lo = int16x8_set1(s);
        result.hi = int16x8_set1(s);
        return result;
    }

    static inline int16x16 unpacklo(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline int16x16 unpackhi(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline int16x16 add(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline int16x16 sub(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline int16x16 mullo(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline int16x16 adds(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline int16x16 subs(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    static inline int16x16 abs(int16x16 a)
    {
        int16x16 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline int16x16 neg(int16x16 a)
    {
        int16x16 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    // bitwise

    static inline int16x16 bitwise_nand(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline int16x16 bitwise_and(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline int16x16 bitwise_or(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline int16x16 bitwise_xor(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    // compare

    static inline int16x16 compare_eq(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline int16x16 compare_gt(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline int16x16 select(int16x16 mask, int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    static inline int16x16 min(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline int16x16 max(int16x16 a, int16x16 b)
    {
        int16x16 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // int32x8
    // -----------------------------------------------------------------

    static inline int32x8 int32x8_zero()
    {
        int32x8 result;
        result.lo = int32x4_zero();
        result.hi = int32x4_zero();
        return result;
    }

    static inline int32x8 int32x8_set1(int32 s)
    {
        int32x8 result;
        result.lo = int32x4_set1(s);
        result.hi = int32x4_set1(s);
        return result;
    }

    static inline int32x8 int32x8_set8(int32 s0, int32 s1, int32 s2, int32 s3, int32 s4, int32 s5, int32 s6, int32 s7)
    {
        int32x8 result;
        result.lo = int32x4_set4(s0, s1, s2, s3);
        result.hi = int32x4_set4(s4, s5, s6, s7);
        return result;
    }

    static inline int32x8 int32x8_uload(const int32* source)
    {
        int32x8 result;
        result.lo = int32x4_uload(source + 0);
        result.hi = int32x4_uload(source + 4);
        return result;
    }

    static inline void int32x8_ustore(int32* dest, int32x8 a)
    {
        int32x4_ustore(dest + 0, a.lo);
        int32x4_ustore(dest + 4, a.hi);
    }

    static inline int32x8 unpacklo(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = unpacklo(a.lo, b.lo);
        result.hi = unpacklo(a.hi, b.hi);
        return result;
    }

    static inline int32x8 unpackhi(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = unpackhi(a.lo, b.lo);
        result.hi = unpackhi(a.hi, b.hi);
        return result;
    }

    static inline int32x8 abs(int32x8 a)
    {
        int32x8 result;
        result.lo = abs(a.lo);
        result.hi = abs(a.hi);
        return result;
    }

    static inline int32x8 neg(int32x8 a)
    {
        int32x8 result;
        result.lo = neg(a.lo);
        result.hi = neg(a.hi);
        return result;
    }

    static inline int32x8 add(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = add(a.lo, b.lo);
        result.hi = add(a.hi, b.hi);
        return result;
    }

    static inline int32x8 sub(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = sub(a.lo, b.lo);
        result.hi = sub(a.hi, b.hi);
        return result;
    }

    static inline int32x8 mullo(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = mullo(a.lo, b.lo);
        result.hi = mullo(a.hi, b.hi);
        return result;
    }

    // saturated

    static inline int32x8 adds(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = adds(a.lo, b.lo);
        result.hi = adds(a.hi, b.hi);
        return result;
    }

    static inline int32x8 subs(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = subs(a.lo, b.lo);
        result.hi = subs(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline int32x8 bitwise_nand(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = bitwise_nand(a.lo, b.lo);
        result.hi = bitwise_nand(a.hi, b.hi);
        return result;
    }

    static inline int32x8 bitwise_and(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = bitwise_and(a.lo, b.lo);
        result.hi = bitwise_and(a.hi, b.hi);
        return result;
    }

    static inline int32x8 bitwise_or(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = bitwise_or(a.lo, b.lo);
        result.hi = bitwise_or(a.hi, b.hi);
        return result;
    }

    static inline int32x8 bitwise_xor(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = bitwise_xor(a.lo, b.lo);
        result.hi = bitwise_xor(a.hi, b.hi);
        return result;
    }

    // shift

    template <int Count>
    static inline int32x8 sll(int32x8 a)
    {
        int32x8 result;
        result.lo = sll<Count>(a.lo);
        result.hi = sll<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline int32x8 srl(int32x8 a)
    {
        int32x8 result;
        result.lo = srl<Count>(a.lo);
        result.hi = srl<Count>(a.hi);
        return result;
    }

    template <int Count>
    static inline int32x8 sra(int32x8 a)
    {
        int32x8 result;
        result.lo = sra<Count>(a.lo);
        result.hi = sra<Count>(a.hi);
        return result;
    }

    // compare

    static inline int32x8 compare_eq(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = compare_eq(a.lo, b.lo);
        result.hi = compare_eq(a.hi, b.hi);
        return result;
    }

    static inline int32x8 compare_gt(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = compare_gt(a.lo, b.lo);
        result.hi = compare_gt(a.hi, b.hi);
        return result;
    }

    static inline int32x8 select(int32x8 mask, int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    static inline int32x8 min(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = min(a.lo, b.lo);
        result.hi = min(a.hi, b.hi);
        return result;
    }

    static inline int32x8 max(int32x8 a, int32x8 b)
    {
        int32x8 result;
        result.lo = max(a.lo, b.lo);
        result.hi = max(a.hi, b.hi);
        return result;
    }

} // namespace simd
} // namespace mango
