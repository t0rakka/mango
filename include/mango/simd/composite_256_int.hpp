/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/simd/simd.hpp>

namespace mango::simd
{

#define SIMD_SET_COMPONENT(vec, value, mask, index) \
    if (index <= mask) vec.data[0] = set_component<index & mask>(vec.data[0], value); \
    else               vec.data[1] = set_component<index & mask>(vec.data[1], value)

#define SIMD_GET_COMPONENT(vec, mask, index) \
        index <= mask ? get_component<index & mask>(vec.data[0]) \
                      : get_component<index & mask>(vec.data[1])

#define SIMD_COMPOSITE_FUNC1(R, A, FUNC) \
    static inline R FUNC(A a) \
    { \
        auto lo = FUNC(a.data[0]); \
        auto hi = FUNC(a.data[1]); \
        return { lo, hi }; \
    }

#define SIMD_COMPOSITE_FUNC2(R, AB, FUNC) \
    static inline R FUNC(AB a, AB b) \
    { \
        auto lo = FUNC(a.data[0], b.data[0]); \
        auto hi = FUNC(a.data[1], b.data[1]); \
        return { lo, hi }; \
    }

#define SIMD_COMPOSITE_SELECT(MASK, AB, FUNC) \
    static inline AB select(MASK mask, AB a, AB b) \
    { \
        auto lo = select(mask.data[0], a.data[0], b.data[0]); \
        auto hi = select(mask.data[1], a.data[1], b.data[1]); \
        return { lo, hi }; \
    }

#define SIMD_COMPOSITE_ZEROMASK_FUNC1(R, A, MASK, FUNC) \
    static inline R FUNC(A a, MASK mask) \
    { \
        auto lo = FUNC(a.data[0], mask.data[0]); \
        auto hi = FUNC(a.data[1], mask.data[1]); \
        return { lo, hi }; \
    }

#define SIMD_COMPOSITE_ZEROMASK_FUNC2(R, AB, MASK, FUNC) \
    static inline R FUNC(AB a, AB b, MASK mask) \
    { \
        auto lo = FUNC(a.data[0], b.data[0], mask.data[0]); \
        auto hi = FUNC(a.data[1], b.data[1], mask.data[1]); \
        return { lo, hi }; \
    }

#define SIMD_COMPOSITE_MASK_FUNC1(R, A, MASK, FUNC) \
    static inline R FUNC(A a, MASK mask, R value) \
    { \
        auto lo = FUNC(a.data[0], mask.data[0], value.data[0]); \
        auto hi = FUNC(a.data[1], mask.data[1], value.data[1]); \
        return { lo, hi }; \
    }

#define SIMD_COMPOSITE_MASK_FUNC2(R, AB, MASK, FUNC) \
    static inline R FUNC(AB a, AB b, MASK mask, R value) \
    { \
        auto lo = FUNC(a.data[0], b.data[0], mask.data[0], value.data[0]); \
        auto hi = FUNC(a.data[1], b.data[1], mask.data[1], value.data[1]); \
        return { lo, hi }; \
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
        auto lo = u8x16_zero();
        auto hi = u8x16_zero();
        return { lo, hi };
    }

    static inline u8x32 u8x32_set(u8 s)
    {
        auto lo = u8x16_set(s);
        auto hi = u8x16_set(s);
        return { lo, hi };
    }

    static inline u8x32 u8x32_set(
        u8 v00, u8 v01, u8 v02, u8 v03, u8 v04, u8 v05, u8 v06, u8 v07,
        u8 v08, u8 v09, u8 v10, u8 v11, u8 v12, u8 v13, u8 v14, u8 v15,
        u8 v16, u8 v17, u8 v18, u8 v19, u8 v20, u8 v21, u8 v22, u8 v23,
        u8 v24, u8 v25, u8 v26, u8 v27, u8 v28, u8 v29, u8 v30, u8 v31)
    {
        auto lo = u8x16_set(v00, v01, v02, v03, v04, v05, v06, v07,
                            v08, v09, v10, v11, v12, v13, v14, v15);
        auto hi = u8x16_set(v16, v17, v18, v19, v20, v21, v22, v23,
                            v24, v25, v26, v27, v28, v29, v30, v31);
        return { lo, hi };
    }

    static inline u8x32 u8x32_uload(const void* source)
    {
        auto lo = u8x16_uload(reinterpret_cast<const u8*>(source) +  0);
        auto hi = u8x16_uload(reinterpret_cast<const u8*>(source) + 16);
        return { lo, hi };
    }

    static inline void u8x32_ustore(void* dest, u8x32 a)
    {
        u8x16_ustore(reinterpret_cast<u8*>(dest) +  0, a.data[0]);
        u8x16_ustore(reinterpret_cast<u8*>(dest) + 16, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, unpacklo)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, unpackhi)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, add)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, sub)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, adds)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, subs)
    SIMD_COMPOSITE_FUNC2(u8x32, u8x32, avg)

    SIMD_COMPOSITE_ZEROMASK_FUNC2(u8x32, u8x32, mask8x32, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u8x32, u8x32, mask8x32, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u8x32, u8x32, mask8x32, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u8x32, u8x32, mask8x32, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u8x32, u8x32, mask8x32, adds)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u8x32, u8x32, mask8x32, subs)
    SIMD_COMPOSITE_MASK_FUNC2(u8x32, u8x32, mask8x32, min)
    SIMD_COMPOSITE_MASK_FUNC2(u8x32, u8x32, mask8x32, max)
    SIMD_COMPOSITE_MASK_FUNC2(u8x32, u8x32, mask8x32, add)
    SIMD_COMPOSITE_MASK_FUNC2(u8x32, u8x32, mask8x32, sub)
    SIMD_COMPOSITE_MASK_FUNC2(u8x32, u8x32, mask8x32, adds)
    SIMD_COMPOSITE_MASK_FUNC2(u8x32, u8x32, mask8x32, subs)

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
        auto lo = u16x8_zero();
        auto hi = u16x8_zero();
        return { lo, hi };
    }

    static inline u16x16 u16x16_set(u16 s)
    {
        auto lo = u16x8_set(s);
        auto hi = u16x8_set(s);
        return { lo, hi };
    }

    static inline u16x16 u16x16_set(
        u16 v00, u16 v01, u16 v02, u16 v03, u16 v04, u16 v05, u16 v06, u16 v07,
        u16 v08, u16 v09, u16 v10, u16 v11, u16 v12, u16 v13, u16 v14, u16 v15)
    {
        auto lo = u16x8_set(v00, v01, v02, v03, v04, v05, v06, v07);
        auto hi = u16x8_set(v08, v09, v10, v11, v12, v13, v14, v15);
        return { lo, hi };
    }

    static inline u16x16 u16x16_uload(const void* source)
    {
        auto lo = u16x8_uload(reinterpret_cast<const u16*>(source) + 0);
        auto hi = u16x8_uload(reinterpret_cast<const u16*>(source) + 8);
        return { lo, hi };
    }

    static inline void u16x16_ustore(void* dest, u16x16 a)
    {
        u16x8_ustore(reinterpret_cast<u16*>(dest) + 0, a.data[0]);
        u16x8_ustore(reinterpret_cast<u16*>(dest) + 8, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, unpacklo)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, unpackhi)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, add)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, sub)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, mullo)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, adds)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, subs)
    SIMD_COMPOSITE_FUNC2(u16x16, u16x16, avg)

    SIMD_COMPOSITE_ZEROMASK_FUNC2(u16x16, u16x16, mask16x16, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u16x16, u16x16, mask16x16, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u16x16, u16x16, mask16x16, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u16x16, u16x16, mask16x16, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u16x16, u16x16, mask16x16, adds)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u16x16, u16x16, mask16x16, subs)
    SIMD_COMPOSITE_MASK_FUNC2(u16x16, u16x16, mask16x16, min)
    SIMD_COMPOSITE_MASK_FUNC2(u16x16, u16x16, mask16x16, max)
    SIMD_COMPOSITE_MASK_FUNC2(u16x16, u16x16, mask16x16, add)
    SIMD_COMPOSITE_MASK_FUNC2(u16x16, u16x16, mask16x16, sub)
    SIMD_COMPOSITE_MASK_FUNC2(u16x16, u16x16, mask16x16, adds)
    SIMD_COMPOSITE_MASK_FUNC2(u16x16, u16x16, mask16x16, subs)

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
        auto lo = slli<Count>(a.data[0]);
        auto hi = slli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline u16x16 srli(u16x16 a)
    {
        auto lo = srli<Count>(a.data[0]);
        auto hi = srli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline u16x16 srai(u16x16 a)
    {
        auto lo = srai<Count>(a.data[0]);
        auto hi = srai<Count>(a.data[1]);
        return { lo, hi };
    }

    // shift by scalar

    static inline u16x16 sll(u16x16 a, int count)
    {
        auto lo = sll(a.data[0], count);
        auto hi = sll(a.data[1], count);
        return { lo, hi };
    }

    static inline u16x16 srl(u16x16 a, int count)
    {
        auto lo = srl(a.data[0], count);
        auto hi = srl(a.data[1], count);
        return { lo, hi };
    }

    static inline u16x16 sra(u16x16 a, int count)
    {
        auto lo = sra(a.data[0], count);
        auto hi = sra(a.data[1], count);
        return { lo, hi };
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
        auto lo = u32x4_zero();
        auto hi = u32x4_zero();
        return { lo, hi };
    }

    static inline u32x8 u32x8_set(u32 s)
    {
        auto lo = u32x4_set(s);
        auto hi = u32x4_set(s);
        return { lo, hi };
    }

    static inline u32x8 u32x8_set(u32 v0, u32 v1, u32 v2, u32 v3, u32 v4, u32 v5, u32 v6, u32 v7)
    {
        auto lo = u32x4_set(v0, v1, v2, v3);
        auto hi = u32x4_set(v4, v5, v6, v7);
        return { lo, hi };
    }

    static inline u32x8 u32x8_uload(const void* source)
    {
        auto lo = u32x4_uload(reinterpret_cast<const u32*>(source) + 0);
        auto hi = u32x4_uload(reinterpret_cast<const u32*>(source) + 4);
        return { lo, hi };
    }

    static inline void u32x8_ustore(void* dest, u32x8 a)
    {
        u32x4_ustore(reinterpret_cast<u32*>(dest) + 0, a.data[0]);
        u32x4_ustore(reinterpret_cast<u32*>(dest) + 4, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, unpacklo)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, unpackhi)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, add)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, sub)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, mullo)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, adds)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, subs)
    SIMD_COMPOSITE_FUNC2(u32x8, u32x8, avg)

    SIMD_COMPOSITE_ZEROMASK_FUNC2(u32x8, u32x8, mask32x8, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u32x8, u32x8, mask32x8, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u32x8, u32x8, mask32x8, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u32x8, u32x8, mask32x8, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u32x8, u32x8, mask32x8, adds)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u32x8, u32x8, mask32x8, subs)
    SIMD_COMPOSITE_MASK_FUNC2(u32x8, u32x8, mask32x8, min)
    SIMD_COMPOSITE_MASK_FUNC2(u32x8, u32x8, mask32x8, max)
    SIMD_COMPOSITE_MASK_FUNC2(u32x8, u32x8, mask32x8, add)
    SIMD_COMPOSITE_MASK_FUNC2(u32x8, u32x8, mask32x8, sub)
    SIMD_COMPOSITE_MASK_FUNC2(u32x8, u32x8, mask32x8, adds)
    SIMD_COMPOSITE_MASK_FUNC2(u32x8, u32x8, mask32x8, subs)

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
        auto lo = slli<Count>(a.data[0]);
        auto hi = slli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline u32x8 srli(u32x8 a)
    {
        auto lo = srli<Count>(a.data[0]);
        auto hi = srli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline u32x8 srai(u32x8 a)
    {
        auto lo = srai<Count>(a.data[0]);
        auto hi = srai<Count>(a.data[1]);
        return { lo, hi };
    }

    // shift by scalar

    static inline u32x8 sll(u32x8 a, int count)
    {
        auto lo = sll(a.data[0], count);
        auto hi = sll(a.data[1], count);
        return { lo, hi };
    }

    static inline u32x8 srl(u32x8 a, int count)
    {
        auto lo = srl(a.data[0], count);
        auto hi = srl(a.data[1], count);
        return { lo, hi };
    }

    static inline u32x8 sra(u32x8 a, int count)
    {
        auto lo = sra(a.data[0], count);
        auto hi = sra(a.data[1], count);
        return { lo, hi };
    }

    // shift by vector

    static inline u32x8 sll(u32x8 a, u32x8 count)
    {
        auto lo = sll(a.data[0], count.data[0]);
        auto hi = sll(a.data[1], count.data[1]);
        return { lo, hi };
    }

    static inline u32x8 srl(u32x8 a, u32x8 count)
    {
        auto lo = srl(a.data[0], count.data[0]);
        auto hi = srl(a.data[1], count.data[1]);
        return { lo, hi };
    }

    static inline u32x8 sra(u32x8 a, u32x8 count)
    {
        auto lo = sra(a.data[0], count.data[0]);
        auto hi = sra(a.data[1], count.data[1]);
        return { lo, hi };
    }

    // -----------------------------------------------------------------
    // u64x4
    // -----------------------------------------------------------------

    template <u32 x_, u32 y_, u32 z_, u32 w_>
    static inline u64x4 shuffle(u64x4 v)
    {
        static_assert(x_ < 4 && y_ < 4 && z_ < 4 && w_ < 4, "Index out of range.");
        const u64 x = SIMD_GET_COMPONENT(v, 1, x_);
        const u64 y = SIMD_GET_COMPONENT(v, 1, y_);
        const u64 z = SIMD_GET_COMPONENT(v, 1, z_);
        const u64 w = SIMD_GET_COMPONENT(v, 1, w_);
        const u64x2 lo = u64x2_set(x, y);
        const u64x2 hi = u64x2_set(z, w);
        return { lo, hi };
    }

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
        auto lo = u64x2_zero();
        auto hi = u64x2_zero();
        return { lo, hi };
    }

    static inline u64x4 u64x4_set(u64 s)
    {
        auto lo = u64x2_set(s);
        auto hi = u64x2_set(s);
        return { lo, hi };
    }

    static inline u64x4 u64x4_set(u64 x, u64 y, u64 z, u64 w)
    {
        auto lo = u64x2_set(x, y);
        auto hi = u64x2_set(z, w);
        return { lo, hi };
    }

    static inline u64x4 u64x4_uload(const void* source)
    {
        auto lo = u64x2_uload(reinterpret_cast<const u64*>(source) + 0);
        auto hi = u64x2_uload(reinterpret_cast<const u64*>(source) + 2);
        return { lo, hi };
    }

    static inline void u64x4_ustore(void* dest, u64x4 a)
    {
        u64x2_ustore(reinterpret_cast<u64*>(dest) + 0, a.data[0]);
        u64x2_ustore(reinterpret_cast<u64*>(dest) + 2, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, unpacklo)
    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, unpackhi)
    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, add)
    SIMD_COMPOSITE_FUNC2(u64x4, u64x4, sub)
    
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u64x4, u64x4, mask64x4, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u64x4, u64x4, mask64x4, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u64x4, u64x4, mask64x4, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u64x4, u64x4, mask64x4, sub)
    SIMD_COMPOSITE_MASK_FUNC2(u64x4, u64x4, mask64x4, min)
    SIMD_COMPOSITE_MASK_FUNC2(u64x4, u64x4, mask64x4, max)
    SIMD_COMPOSITE_MASK_FUNC2(u64x4, u64x4, mask64x4, add)
    SIMD_COMPOSITE_MASK_FUNC2(u64x4, u64x4, mask64x4, sub)

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
        auto lo = slli<Count>(a.data[0]);
        auto hi = slli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline u64x4 srli(u64x4 a)
    {
        auto lo = srli<Count>(a.data[0]);
        auto hi = srli<Count>(a.data[1]);
        return { lo, hi };
    }

    // shift by scalar

    static inline u64x4 sll(u64x4 a, int count)
    {
        auto lo = sll(a.data[0], count);
        auto hi = sll(a.data[1], count);
        return { lo, hi };
    }

    static inline u64x4 srl(u64x4 a, int count)
    {
        auto lo = srl(a.data[0], count);
        auto hi = srl(a.data[1], count);
        return { lo, hi };
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
        auto lo = s8x16_zero();
        auto hi = s8x16_zero();
        return { lo, hi };
    }

    static inline s8x32 s8x32_set(s8 s)
    {
        auto lo = s8x16_set(s);
        auto hi = s8x16_set(s);
        return { lo, hi };
    }

    static inline s8x32 s8x32_set(
        s8 v00, s8 v01, s8 v02, s8 v03, s8 v04, s8 v05, s8 v06, s8 v07,
        s8 v08, s8 v09, s8 v10, s8 v11, s8 v12, s8 v13, s8 v14, s8 v15,
        s8 v16, s8 v17, s8 v18, s8 v19, s8 v20, s8 v21, s8 v22, s8 v23,
        s8 v24, s8 v25, s8 v26, s8 v27, s8 v28, s8 v29, s8 v30, s8 v31)
    {
        auto lo = s8x16_set(v00, v01, v02, v03, v04, v05, v06, v07,
                            v08, v09, v10, v11, v12, v13, v14, v15);
        auto hi = s8x16_set(v16, v17, v18, v19, v20, v21, v22, v23,
                            v24, v25, v26, v27, v28, v29, v30, v31);
        return { lo, hi };
    }

    static inline s8x32 s8x32_uload(const void* source)
    {
        auto lo = s8x16_uload(reinterpret_cast<const s8*>(source) +  0);
        auto hi = s8x16_uload(reinterpret_cast<const s8*>(source) + 16);
        return { lo, hi };
    }

    static inline void s8x32_ustore(void* dest, s8x32 a)
    {
        s8x16_ustore(reinterpret_cast<s8*>(dest) +  0, a.data[0]);
        s8x16_ustore(reinterpret_cast<s8*>(dest) + 16, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, unpacklo)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, unpackhi)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, add)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, sub)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, adds)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, subs)
    SIMD_COMPOSITE_FUNC2(s8x32, s8x32, avg)
    SIMD_COMPOSITE_FUNC1(s8x32, s8x32, abs)
    SIMD_COMPOSITE_FUNC1(s8x32, s8x32, neg)

    SIMD_COMPOSITE_ZEROMASK_FUNC1(s8x32, s8x32, mask8x32, abs)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s8x32, s8x32, mask8x32, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s8x32, s8x32, mask8x32, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s8x32, s8x32, mask8x32, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s8x32, s8x32, mask8x32, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s8x32, s8x32, mask8x32, adds)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s8x32, s8x32, mask8x32, subs)
    SIMD_COMPOSITE_MASK_FUNC1(s8x32, s8x32, mask8x32, abs)
    SIMD_COMPOSITE_MASK_FUNC2(s8x32, s8x32, mask8x32, min)
    SIMD_COMPOSITE_MASK_FUNC2(s8x32, s8x32, mask8x32, max)
    SIMD_COMPOSITE_MASK_FUNC2(s8x32, s8x32, mask8x32, add)
    SIMD_COMPOSITE_MASK_FUNC2(s8x32, s8x32, mask8x32, sub)
    SIMD_COMPOSITE_MASK_FUNC2(s8x32, s8x32, mask8x32, adds)
    SIMD_COMPOSITE_MASK_FUNC2(s8x32, s8x32, mask8x32, subs)

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
        auto lo = s16x8_zero();
        auto hi = s16x8_zero();
        return { lo, hi };
    }

    static inline s16x16 s16x16_set(s16 s)
    {
        auto lo = s16x8_set(s);
        auto hi = s16x8_set(s);
        return { lo, hi };
    }

    static inline s16x16 s16x16_set(
        s16 v00, s16 v01, s16 v02, s16 v03, s16 v04, s16 v05, s16 v06, s16 v07,
        s16 v08, s16 v09, s16 v10, s16 v11, s16 v12, s16 v13, s16 v14, s16 v15)
    {
        auto lo = s16x8_set(v00, v01, v02, v03, v04, v05, v06, v07);
        auto hi = s16x8_set(v08, v09, v10, v11, v12, v13, v14, v15);
        return { lo, hi };
    }

    static inline s16x16 s16x16_uload(const void* source)
    {
        auto lo = s16x8_uload(reinterpret_cast<const s16*>(source) + 0);
        auto hi = s16x8_uload(reinterpret_cast<const s16*>(source) + 8);
        return { lo, hi };
    }

    static inline void s16x16_ustore(void* dest, s16x16 a)
    {
        s16x8_ustore(reinterpret_cast<s16*>(dest) + 0, a.data[0]);
        s16x8_ustore(reinterpret_cast<s16*>(dest) + 8, a.data[1]);
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
    SIMD_COMPOSITE_FUNC2(s16x16, s16x16, mullo)
    SIMD_COMPOSITE_FUNC2(s32x8, s16x16, madd)
    SIMD_COMPOSITE_FUNC1(s16x16, s16x16, abs)
    SIMD_COMPOSITE_FUNC1(s16x16, s16x16, neg)

    SIMD_COMPOSITE_ZEROMASK_FUNC1(s16x16, s16x16, mask16x16, abs)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s16x16, s16x16, mask16x16, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s16x16, s16x16, mask16x16, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s16x16, s16x16, mask16x16, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s16x16, s16x16, mask16x16, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s16x16, s16x16, mask16x16, adds)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s16x16, s16x16, mask16x16, subs)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x8, s16x16, mask32x8, madd)
    SIMD_COMPOSITE_MASK_FUNC1(s16x16, s16x16, mask16x16, abs)
    SIMD_COMPOSITE_MASK_FUNC2(s16x16, s16x16, mask16x16, min)
    SIMD_COMPOSITE_MASK_FUNC2(s16x16, s16x16, mask16x16, max)
    SIMD_COMPOSITE_MASK_FUNC2(s16x16, s16x16, mask16x16, add)
    SIMD_COMPOSITE_MASK_FUNC2(s16x16, s16x16, mask16x16, sub)
    SIMD_COMPOSITE_MASK_FUNC2(s16x16, s16x16, mask16x16, adds)
    SIMD_COMPOSITE_MASK_FUNC2(s16x16, s16x16, mask16x16, subs)
    SIMD_COMPOSITE_MASK_FUNC2(s32x8, s16x16, mask32x8, madd)

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
        auto lo = slli<Count>(a.data[0]);
        auto hi = slli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline s16x16 srli(s16x16 a)
    {
        auto lo = srli<Count>(a.data[0]);
        auto hi = srli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline s16x16 srai(s16x16 a)
    {
        auto lo = srai<Count>(a.data[0]);
        auto hi = srai<Count>(a.data[1]);
        return { lo, hi };
    }

    // shift by scalar

    static inline s16x16 sll(s16x16 a, int count)
    {
        auto lo = sll(a.data[0], count);
        auto hi = sll(a.data[1], count);
        return { lo, hi };
    }

    static inline s16x16 srl(s16x16 a, int count)
    {
        auto lo = srl(a.data[0], count);
        auto hi = srl(a.data[1], count);
        return { lo, hi };
    }

    static inline s16x16 sra(s16x16 a, int count)
    {
        auto lo = sra(a.data[0], count);
        auto hi = sra(a.data[1], count);
        return { lo, hi };
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
        auto lo = s32x4_zero();
        auto hi = s32x4_zero();
        return { lo, hi };
    }

    static inline s32x8 s32x8_set(s32 s)
    {
        auto lo = s32x4_set(s);
        auto hi = s32x4_set(s);
        return { lo, hi };
    }

    static inline s32x8 s32x8_set(s32 v0, s32 v1, s32 v2, s32 v3, s32 v4, s32 v5, s32 v6, s32 v7)
    {
        auto lo = s32x4_set(v0, v1, v2, v3);
        auto hi = s32x4_set(v4, v5, v6, v7);
        return { lo, hi };
    }

    static inline s32x8 s32x8_uload(const void* source)
    {
        auto lo = s32x4_uload(reinterpret_cast<const s32*>(source) + 0);
        auto hi = s32x4_uload(reinterpret_cast<const s32*>(source) + 4);
        return { lo, hi };
    }

    static inline void s32x8_ustore(void* dest, s32x8 a)
    {
        s32x4_ustore(reinterpret_cast<s32*>(dest) + 0, a.data[0]);
        s32x4_ustore(reinterpret_cast<s32*>(dest) + 4, a.data[1]);
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
    SIMD_COMPOSITE_FUNC2(s32x8, s32x8, mullo)
    SIMD_COMPOSITE_FUNC1(s32x8, s32x8, abs)
    SIMD_COMPOSITE_FUNC1(s32x8, s32x8, neg)

    SIMD_COMPOSITE_ZEROMASK_FUNC1(s32x8, s32x8, mask32x8, abs)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x8, s32x8, mask32x8, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x8, s32x8, mask32x8, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x8, s32x8, mask32x8, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x8, s32x8, mask32x8, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x8, s32x8, mask32x8, adds)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x8, s32x8, mask32x8, subs)
    SIMD_COMPOSITE_MASK_FUNC1(s32x8, s32x8, mask32x8, abs)
    SIMD_COMPOSITE_MASK_FUNC2(s32x8, s32x8, mask32x8, min)
    SIMD_COMPOSITE_MASK_FUNC2(s32x8, s32x8, mask32x8, max)
    SIMD_COMPOSITE_MASK_FUNC2(s32x8, s32x8, mask32x8, add)
    SIMD_COMPOSITE_MASK_FUNC2(s32x8, s32x8, mask32x8, sub)
    SIMD_COMPOSITE_MASK_FUNC2(s32x8, s32x8, mask32x8, adds)
    SIMD_COMPOSITE_MASK_FUNC2(s32x8, s32x8, mask32x8, subs)

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
        auto lo = slli<Count>(a.data[0]);
        auto hi = slli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline s32x8 srli(s32x8 a)
    {
        auto lo = srli<Count>(a.data[0]);
        auto hi = srli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline s32x8 srai(s32x8 a)
    {
        auto lo = srai<Count>(a.data[0]);
        auto hi = srai<Count>(a.data[1]);
        return { lo, hi };
    }

    // shift by scalar

    static inline s32x8 sll(s32x8 a, int count)
    {
        auto lo = sll(a.data[0], count);
        auto hi = sll(a.data[1], count);
        return { lo, hi };
    }

    static inline s32x8 srl(s32x8 a, int count)
    {
        auto lo = srl(a.data[0], count);
        auto hi = srl(a.data[1], count);
        return { lo, hi };
    }

    static inline s32x8 sra(s32x8 a, int count)
    {
        auto lo = sra(a.data[0], count);
        auto hi = sra(a.data[1], count);
        return { lo, hi };
    }

    // shift by vector

    static inline s32x8 sll(s32x8 a, u32x8 count)
    {
        auto lo = sll(a.data[0], count.data[0]);
        auto hi = sll(a.data[1], count.data[1]);
        return { lo, hi };
    }

    static inline s32x8 srl(s32x8 a, u32x8 count)
    {
        auto lo = srl(a.data[0], count.data[0]);
        auto hi = srl(a.data[1], count.data[1]);
        return { lo, hi };
    }

    static inline s32x8 sra(s32x8 a, u32x8 count)
    {
        auto lo = sra(a.data[0], count.data[0]);
        auto hi = sra(a.data[1], count.data[1]);
        return { lo, hi };
    }

    // -----------------------------------------------------------------
    // s64x4
    // -----------------------------------------------------------------

    template <u32 x_, u32 y_, u32 z_, u32 w_>
    static inline s64x4 shuffle(s64x4 v)
    {
        static_assert(x_ < 4 && y_ < 4 && z_ < 4 && w_ < 4, "Index out of range.");
        const s64 x = SIMD_GET_COMPONENT(v, 1, x_);
        const s64 y = SIMD_GET_COMPONENT(v, 1, y_);
        const s64 z = SIMD_GET_COMPONENT(v, 1, z_);
        const s64 w = SIMD_GET_COMPONENT(v, 1, w_);
        const s64x2 lo = s64x2_set(x, y);
        const s64x2 hi = s64x2_set(z, w);
        return { lo, hi };
    }

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
        auto lo = s64x2_zero();
        auto hi = s64x2_zero();
        return { lo, hi };
    }

    static inline s64x4 s64x4_set(s64 s)
    {
        auto lo = s64x2_set(s);
        auto hi = s64x2_set(s);
        return { lo, hi };
    }

    static inline s64x4 s64x4_set(s64 x, s64 y, s64 z, s64 w)
    {
        auto lo = s64x2_set(x, y);
        auto hi = s64x2_set(z, w);
        return { lo, hi };
    }

    static inline s64x4 s64x4_uload(const void* source)
    {
        auto lo = s64x2_uload(reinterpret_cast<const s64*>(source) + 0);
        auto hi = s64x2_uload(reinterpret_cast<const s64*>(source) + 2);
        return { lo, hi };
    }

    static inline void s64x4_ustore(void* dest, s64x4 a)
    {
        s64x2_ustore(reinterpret_cast<s64*>(dest) + 0, a.data[0]);
        s64x2_ustore(reinterpret_cast<s64*>(dest) + 2, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, unpacklo)
    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, unpackhi)
    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, add)
    SIMD_COMPOSITE_FUNC2(s64x4, s64x4, sub)
    SIMD_COMPOSITE_FUNC1(s64x4, s64x4, neg)

    SIMD_COMPOSITE_ZEROMASK_FUNC2(s64x4, s64x4, mask64x4, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s64x4, s64x4, mask64x4, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s64x4, s64x4, mask64x4, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s64x4, s64x4, mask64x4, sub)
    SIMD_COMPOSITE_MASK_FUNC2(s64x4, s64x4, mask64x4, min)
    SIMD_COMPOSITE_MASK_FUNC2(s64x4, s64x4, mask64x4, max)
    SIMD_COMPOSITE_MASK_FUNC2(s64x4, s64x4, mask64x4, add)
    SIMD_COMPOSITE_MASK_FUNC2(s64x4, s64x4, mask64x4, sub)

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
        auto lo = slli<Count>(a.data[0]);
        auto hi = slli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline s64x4 srli(s64x4 a)
    {
        auto lo = srli<Count>(a.data[0]);
        auto hi = srli<Count>(a.data[1]);
        return { lo, hi };
    }

    // shift by scalar

    static inline s64x4 sll(s64x4 a, int count)
    {
        auto lo = sll(a.data[0], count);
        auto hi = sll(a.data[1], count);
        return { lo, hi };
    }

    static inline s64x4 srl(s64x4 a, int count)
    {
        auto lo = srl(a.data[0], count);
        auto hi = srl(a.data[1], count);
        return { lo, hi };
    }

    // -----------------------------------------------------------------
    // mask8x32
    // -----------------------------------------------------------------

    static inline mask8x32 mask_and(mask8x32 a, mask8x32 b)
    {
        auto lo = mask_and(a.data[0], b.data[0]);
        auto hi = mask_and(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask8x32 mask_or(mask8x32 a, mask8x32 b)
    {
        auto lo = mask_or(a.data[0], b.data[0]);
        auto hi = mask_or(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask8x32 mask_xor(mask8x32 a, mask8x32 b)
    {
        auto lo = mask_xor(a.data[0], b.data[0]);
        auto hi = mask_xor(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask8x32 mask_not(mask8x32 a)
    {
        auto lo = mask_not(a.data[0]);
        auto hi = mask_not(a.data[1]);
        return { lo, hi };
    }

    static inline u32 get_mask(mask8x32 a)
    {
        u32 mask = get_mask(a.data[0]) | (get_mask(a.data[1]) << 16);
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

    static inline mask16x16 mask_and(mask16x16 a, mask16x16 b)
    {
        auto lo = mask_and(a.data[0], b.data[0]);
        auto hi = mask_and(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask16x16 mask_or(mask16x16 a, mask16x16 b)
    {
        auto lo = mask_or(a.data[0], b.data[0]);
        auto hi = mask_or(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask16x16 mask_xor(mask16x16 a, mask16x16 b)
    {
        auto lo = mask_xor(a.data[0], b.data[0]);
        auto hi = mask_xor(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask16x16 mask_not(mask16x16 a)
    {
        auto lo = mask_not(a.data[0]);
        auto hi = mask_not(a.data[1]);
        return { lo, hi };
    }

    static inline u32 get_mask(mask16x16 a)
    {
        u32 mask = get_mask(a.data[0]) | (get_mask(a.data[1]) << 8);
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

    static inline mask32x8 mask_and(mask32x8 a, mask32x8 b)
    {
        auto lo = mask_and(a.data[0], b.data[0]);
        auto hi = mask_and(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask32x8 mask_or(mask32x8 a, mask32x8 b)
    {
        auto lo = mask_or(a.data[0], b.data[0]);
        auto hi = mask_or(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask32x8 mask_xor(mask32x8 a, mask32x8 b)
    {
        auto lo = mask_xor(a.data[0], b.data[0]);
        auto hi = mask_xor(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask32x8 mask_not(mask32x8 a)
    {
        auto lo = mask_not(a.data[0]);
        auto hi = mask_not(a.data[1]);
        return { lo, hi };
    }

    static inline u32 get_mask(mask32x8 a)
    {
        u32 mask = get_mask(a.data[0]) | (get_mask(a.data[1]) << 4);
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

    static inline mask64x4 mask_and(mask64x4 a, mask64x4 b)
    {
        auto lo = mask_and(a.data[0], b.data[0]);
        auto hi = mask_and(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask64x4 mask_or(mask64x4 a, mask64x4 b)
    {
        auto lo = mask_or(a.data[0], b.data[0]);
        auto hi = mask_or(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask64x4 mask_xor(mask64x4 a, mask64x4 b)
    {
        auto lo = mask_xor(a.data[0], b.data[0]);
        auto hi = mask_xor(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask64x4 mask_not(mask64x4 a)
    {
        auto lo = mask_not(a.data[0]);
        auto hi = mask_not(a.data[1]);
        return { lo, hi };
    }

    static inline u32 get_mask(mask64x4 a)
    {
        u32 mask = get_mask(a.data[0]) | (get_mask(a.data[1]) << 2);
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
#undef SIMD_COMPOSITE_ZEROMASK_FUNC1
#undef SIMD_COMPOSITE_ZEROMASK_FUNC2
#undef SIMD_COMPOSITE_MASK_FUNC1
#undef SIMD_COMPOSITE_MASK_FUNC2

} // namespace mango::simd
