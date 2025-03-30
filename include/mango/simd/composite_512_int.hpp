/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/simd/simd.hpp>

namespace mango::simd
{

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

#define SIMD_COMPOSITE_ZEROMASK_FUNC1(R, A, MASK, FUNC) \
    static inline R FUNC(A a, MASK mask) \
    { \
        auto lo = FUNC(a.data[0], mask.data[0]); \
        auto hi = FUNC(a.data[1], mask.data[1]); \
        return { lo, hi }; \
    }

#define SIMD_COMPOSITE_MASK_FUNC1(R, A, MASK, FUNC) \
    static inline R FUNC(A a, MASK mask, R value) \
    { \
        auto lo = FUNC(a.data[0], mask.data[0], value.data[0]); \
        auto hi = FUNC(a.data[1], mask.data[1], value.data[1]); \
        return { lo, hi }; \
    }

#define SIMD_COMPOSITE_ZEROMASK_FUNC2(R, AB, MASK, FUNC) \
    static inline R FUNC(AB a, AB b, MASK mask) \
    { \
        auto lo = FUNC(a.data[0], b.data[0], mask.data[0]); \
        auto hi = FUNC(a.data[1], b.data[1], mask.data[1]); \
        return { lo, hi }; \
    }

#define SIMD_COMPOSITE_MASK_FUNC2(R, AB, MASK, FUNC) \
    static inline R FUNC(AB a, AB b, MASK mask, R value) \
    { \
        auto lo = FUNC(a.data[0], b.data[0], mask.data[0], value.data[0]); \
        auto hi = FUNC(a.data[1], b.data[1], mask.data[1], value.data[1]); \
        return { lo, hi }; \
    }

#define SIMD_COMPOSITE_SELECT(MASK, AB, FUNC) \
    static inline AB select(MASK mask, AB a, AB b) \
    { \
        auto lo = select(mask.data[0], a.data[0], b.data[0]); \
        auto hi = select(mask.data[1], a.data[1], b.data[1]); \
        return { lo, hi }; \
    }

    // -----------------------------------------------------------------
    // u8x64
    // -----------------------------------------------------------------

    static inline u8x64 u8x64_zero()
    {
        auto value = u8x32_zero();
        return { value, value };
    }

    static inline u8x64 u8x64_set(u8 s)
    {
        auto value = u8x32_set(s);
        return { value, value };
    }

    static inline u8x64 u8x64_set(
        u8 v00, u8 v01, u8 v02, u8 v03, u8 v04, u8 v05, u8 v06, u8 v07,
        u8 v08, u8 v09, u8 v10, u8 v11, u8 v12, u8 v13, u8 v14, u8 v15,
        u8 v16, u8 v17, u8 v18, u8 v19, u8 v20, u8 v21, u8 v22, u8 v23,
        u8 v24, u8 v25, u8 v26, u8 v27, u8 v28, u8 v29, u8 v30, u8 v31,
        u8 v32, u8 v33, u8 v34, u8 v35, u8 v36, u8 v37, u8 v38, u8 v39,
        u8 v40, u8 v41, u8 v42, u8 v43, u8 v44, u8 v45, u8 v46, u8 v47,
        u8 v48, u8 v49, u8 v50, u8 v51, u8 v52, u8 v53, u8 v54, u8 v55,
        u8 v56, u8 v57, u8 v58, u8 v59, u8 v60, u8 v61, u8 v62, u8 v63)
    {
        auto lo = u8x32_set(v00, v01, v02, v03, v04, v05, v06, v07,
                            v08, v09, v10, v11, v12, v13, v14, v15,
                            v16, v17, v18, v19, v20, v21, v22, v23,
                            v24, v25, v26, v27, v28, v29, v30, v31);
        auto hi = u8x32_set(v32, v33, v34, v35, v36, v37, v38, v39,
                            v40, v41, v42, v43, v44, v45, v46, v47,
                            v48, v49, v50, v51, v52, v53, v54, v55,
                            v56, v57, v58, v59, v60, v61, v62, v63);
        return { lo, hi };
    }

    static inline u8x64 u8x64_uload(const void* source)
    {
        auto lo = u8x32_uload(reinterpret_cast<const u8*>(source) +  0);
        auto hi = u8x32_uload(reinterpret_cast<const u8*>(source) + 32);
        return { lo, hi };
    }

    static inline void u8x64_ustore(void* dest, u8x64 a)
    {
        u8x32_ustore(reinterpret_cast<u8*>(dest) +  0, a.data[0]);
        u8x32_ustore(reinterpret_cast<u8*>(dest) + 32, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, unpacklo)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, unpackhi)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, add)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, sub)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, adds)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, subs)
    SIMD_COMPOSITE_FUNC2(u8x64, u8x64, avg)

    SIMD_COMPOSITE_ZEROMASK_FUNC2(u8x64, u8x64, mask8x64, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u8x64, u8x64, mask8x64, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u8x64, u8x64, mask8x64, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u8x64, u8x64, mask8x64, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u8x64, u8x64, mask8x64, adds)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u8x64, u8x64, mask8x64, subs)
    SIMD_COMPOSITE_MASK_FUNC2(u8x64, u8x64, mask8x64, min)
    SIMD_COMPOSITE_MASK_FUNC2(u8x64, u8x64, mask8x64, max)
    SIMD_COMPOSITE_MASK_FUNC2(u8x64, u8x64, mask8x64, add)
    SIMD_COMPOSITE_MASK_FUNC2(u8x64, u8x64, mask8x64, sub)
    SIMD_COMPOSITE_MASK_FUNC2(u8x64, u8x64, mask8x64, adds)
    SIMD_COMPOSITE_MASK_FUNC2(u8x64, u8x64, mask8x64, subs)

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
        auto lo = u16x16_zero();
        auto hi = u16x16_zero();
        return { lo, hi };
    }

    static inline u16x32 u16x32_set(u16 s)
    {
        auto lo = u16x16_set(s);
        auto hi = u16x16_set(s);
        return { lo, hi };
    }

    static inline u16x32 u16x32_set(
        u16 v00, u16 v01, u16 v02, u16 v03, u16 v04, u16 v05, u16 v06, u16 v07,
        u16 v08, u16 v09, u16 v10, u16 v11, u16 v12, u16 v13, u16 v14, u16 v15,
        u16 v16, u16 v17, u16 v18, u16 v19, u16 v20, u16 v21, u16 v22, u16 v23,
        u16 v24, u16 v25, u16 v26, u16 v27, u16 v28, u16 v29, u16 v30, u16 v31)
    {
        auto lo = u16x16_set(v00, v01, v02, v03, v04, v05, v06, v07,
                             v08, v09, v10, v11, v12, v13, v14, v15);
        auto hi = u16x16_set(v16, v17, v18, v19, v20, v21, v22, v23,
                             v24, v25, v26, v27, v28, v29, v30, v31);
        return { lo, hi };
    }

    static inline u16x32 u16x32_uload(const void* source)
    {
        auto lo = u16x16_uload(reinterpret_cast<const u16*>(source) +  0);
        auto hi = u16x16_uload(reinterpret_cast<const u16*>(source) + 16);
        return { lo, hi };
    }

    static inline void u16x32_ustore(void* dest, u16x32 a)
    {
        u16x16_ustore(reinterpret_cast<u16*>(dest) +  0, a.data[0]);
        u16x16_ustore(reinterpret_cast<u16*>(dest) + 16, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, unpacklo)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, unpackhi)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, add)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, sub)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, mullo)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, adds)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, subs)
    SIMD_COMPOSITE_FUNC2(u16x32, u16x32, avg)

    SIMD_COMPOSITE_ZEROMASK_FUNC2(u16x32, u16x32, mask16x32, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u16x32, u16x32, mask16x32, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u16x32, u16x32, mask16x32, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u16x32, u16x32, mask16x32, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u16x32, u16x32, mask16x32, adds)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u16x32, u16x32, mask16x32, subs)
    SIMD_COMPOSITE_MASK_FUNC2(u16x32, u16x32, mask16x32, min)
    SIMD_COMPOSITE_MASK_FUNC2(u16x32, u16x32, mask16x32, max)
    SIMD_COMPOSITE_MASK_FUNC2(u16x32, u16x32, mask16x32, add)
    SIMD_COMPOSITE_MASK_FUNC2(u16x32, u16x32, mask16x32, sub)
    SIMD_COMPOSITE_MASK_FUNC2(u16x32, u16x32, mask16x32, adds)
    SIMD_COMPOSITE_MASK_FUNC2(u16x32, u16x32, mask16x32, subs)

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
        auto lo = slli<Count>(a.data[0]);
        auto hi = slli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline u16x32 srli(u16x32 a)
    {
        auto lo = srli<Count>(a.data[0]);
        auto hi = srli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline u16x32 srai(u16x32 a)
    {
        auto lo = srai<Count>(a.data[0]);
        auto hi = srai<Count>(a.data[1]);
        return { lo, hi };
    }

    // shift by scalar

    static inline u16x32 sll(u16x32 a, int count)
    {
        auto lo = sll(a.data[0], count);
        auto hi = sll(a.data[1], count);
        return { lo, hi };
    }

    static inline u16x32 srl(u16x32 a, int count)
    {
        auto lo = srl(a.data[0], count);
        auto hi = srl(a.data[1], count);
        return { lo, hi };
    }

    static inline u16x32 sra(u16x32 a, int count)
    {
        auto lo = sra(a.data[0], count);
        auto hi = sra(a.data[1], count);
        return { lo, hi };
    }

    // -----------------------------------------------------------------
    // u32x16
    // -----------------------------------------------------------------

    static inline u32x16 u32x16_zero()
    {
        auto lo = u32x8_zero();
        auto hi = u32x8_zero();
        return { lo, hi };
    }

    static inline u32x16 u32x16_set(u32 s)
    {
        auto lo = u32x8_set(s);
        auto hi = u32x8_set(s);
        return { lo, hi };
    }

    static inline u32x16 u32x16_set(
        u32 v00, u32 v01, u32 v02, u32 v03, u32 v04, u32 v05, u32 v06, u32 v07,
        u32 v08, u32 v09, u32 v10, u32 v11, u32 v12, u32 v13, u32 v14, u32 v15)
    {
        auto lo = u32x8_set(v00, v01, v02, v03, v04, v05, v06, v07);
        auto hi = u32x8_set(v08, v09, v10, v11, v12, v13, v14, v15);
        return { lo, hi };
    }

    static inline u32x16 u32x16_uload(const void* source)
    {
        auto lo = u32x8_uload(reinterpret_cast<const u32*>(source) + 0);
        auto hi = u32x8_uload(reinterpret_cast<const u32*>(source) + 8);
        return { lo, hi };
    }

    static inline void u32x16_ustore(void* dest, u32x16 a)
    {
        u32x8_ustore(reinterpret_cast<u32*>(dest) + 0, a.data[0]);
        u32x8_ustore(reinterpret_cast<u32*>(dest) + 8, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, unpacklo)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, unpackhi)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, add)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, sub)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, adds)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, subs)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, avg)
    SIMD_COMPOSITE_FUNC2(u32x16, u32x16, mullo)

    SIMD_COMPOSITE_ZEROMASK_FUNC2(u32x16, u32x16, mask32x16, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u32x16, u32x16, mask32x16, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u32x16, u32x16, mask32x16, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u32x16, u32x16, mask32x16, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u32x16, u32x16, mask32x16, adds)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u32x16, u32x16, mask32x16, subs)
    SIMD_COMPOSITE_MASK_FUNC2(u32x16, u32x16, mask32x16, min)
    SIMD_COMPOSITE_MASK_FUNC2(u32x16, u32x16, mask32x16, max)
    SIMD_COMPOSITE_MASK_FUNC2(u32x16, u32x16, mask32x16, add)
    SIMD_COMPOSITE_MASK_FUNC2(u32x16, u32x16, mask32x16, sub)
    SIMD_COMPOSITE_MASK_FUNC2(u32x16, u32x16, mask32x16, adds)
    SIMD_COMPOSITE_MASK_FUNC2(u32x16, u32x16, mask32x16, subs)

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
        auto lo = slli<Count>(a.data[0]);
        auto hi = slli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline u32x16 srli(u32x16 a)
    {
        auto lo = srli<Count>(a.data[0]);
        auto hi = srli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline u32x16 srai(u32x16 a)
    {
        auto lo = srai<Count>(a.data[0]);
        auto hi = srai<Count>(a.data[1]);
        return { lo, hi };
    }

    // shift by scalar

    static inline u32x16 sll(u32x16 a, int count)
    {
        auto lo = sll(a.data[0], count);
        auto hi = sll(a.data[1], count);
        return { lo, hi };
    }

    static inline u32x16 srl(u32x16 a, int count)
    {
        auto lo = srl(a.data[0], count);
        auto hi = srl(a.data[1], count);
        return { lo, hi };
    }

    static inline u32x16 sra(u32x16 a, int count)
    {
        auto lo = sra(a.data[0], count);
        auto hi = sra(a.data[1], count);
        return { lo, hi };
    }

    // shift by vector

    static inline u32x16 sll(u32x16 a, u32x16 count)
    {
        a.data[0] = sll(a.data[0], count.data[0]);
        a.data[1] = sll(a.data[1], count.data[1]);
        return a;
    }

    static inline u32x16 srl(u32x16 a, u32x16 count)
    {
        a.data[0] = srl(a.data[0], count.data[0]);
        a.data[1] = srl(a.data[1], count.data[1]);
        return a;
    }

    static inline u32x16 sra(u32x16 a, u32x16 count)
    {
        a.data[0] = sra(a.data[0], count.data[0]);
        a.data[1] = sra(a.data[1], count.data[1]);
        return a;
    }

    // -----------------------------------------------------------------
    // u64x8
    // -----------------------------------------------------------------

    static inline u64x8 u64x8_zero()
    {
        auto lo = u64x4_zero();
        auto hi = u64x4_zero();
        return { lo, hi };
    }

    static inline u64x8 u64x8_set(u64 s)
    {
        auto lo = u64x4_set(s);
        auto hi = u64x4_set(s);
        return { lo, hi };
    }

    static inline u64x8 u64x8_set(u64 v0, u64 v1, u64 v2, u64 v3, u64 v4, u64 v5, u64 v6, u64 v7)
    {
        auto lo = u64x4_set(v0, v1, v2, v3);
        auto hi = u64x4_set(v4, v5, v6, v7);
        return { lo, hi };
    }

    static inline u64x8 u64x8_uload(const void* source)
    {
        auto lo = u64x4_uload(reinterpret_cast<const u64*>(source) + 0);
        auto hi = u64x4_uload(reinterpret_cast<const u64*>(source) + 4);
        return { lo, hi };
    }

    static inline void u64x8_ustore(void* dest, u64x8 a)
    {
        u64x4_ustore(reinterpret_cast<u64*>(dest) + 0, a.data[0]);
        u64x4_ustore(reinterpret_cast<u64*>(dest) + 4, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, unpacklo)
    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, unpackhi)
    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, add)
    SIMD_COMPOSITE_FUNC2(u64x8, u64x8, sub)
    
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u64x8, u64x8, mask64x8, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u64x8, u64x8, mask64x8, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u64x8, u64x8, mask64x8, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(u64x8, u64x8, mask64x8, sub)
    SIMD_COMPOSITE_MASK_FUNC2(u64x8, u64x8, mask64x8, min)
    SIMD_COMPOSITE_MASK_FUNC2(u64x8, u64x8, mask64x8, max)
    SIMD_COMPOSITE_MASK_FUNC2(u64x8, u64x8, mask64x8, add)
    SIMD_COMPOSITE_MASK_FUNC2(u64x8, u64x8, mask64x8, sub)

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
        auto lo = slli<Count>(a.data[0]);
        auto hi = slli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline u64x8 srli(u64x8 a)
    {
        auto lo = srli<Count>(a.data[0]);
        auto hi = srli<Count>(a.data[1]);
        return { lo, hi };
    }

    // shift by scalar

    static inline u64x8 sll(u64x8 a, int count)
    {
        auto lo = sll(a.data[0], count);
        auto hi = sll(a.data[1], count);
        return { lo, hi };
    }

    static inline u64x8 srl(u64x8 a, int count)
    {
        auto lo = srl(a.data[0], count);
        auto hi = srl(a.data[1], count);
        return { lo, hi };
    }

    // -----------------------------------------------------------------
    // s8x64
    // -----------------------------------------------------------------

    static inline s8x64 s8x64_zero()
    {
        auto lo = s8x32_zero();
        auto hi = s8x32_zero();
        return { lo, hi };
    }

    static inline s8x64 s8x64_set(s8 s)
    {
        auto lo = s8x32_set(s);
        auto hi = s8x32_set(s);
        return { lo, hi };
    }

    static inline s8x64 s8x64_set(
        s8 v00, s8 v01, s8 v02, s8 v03, s8 v04, s8 v05, s8 v06, s8 v07,
        s8 v08, s8 v09, s8 v10, s8 v11, s8 v12, s8 v13, s8 v14, s8 v15,
        s8 v16, s8 v17, s8 v18, s8 v19, s8 v20, s8 v21, s8 v22, s8 v23,
        s8 v24, s8 v25, s8 v26, s8 v27, s8 v28, s8 v29, s8 v30, s8 v31,
        s8 v32, s8 v33, s8 v34, s8 v35, s8 v36, s8 v37, s8 v38, s8 v39,
        s8 v40, s8 v41, s8 v42, s8 v43, s8 v44, s8 v45, s8 v46, s8 v47,
        s8 v48, s8 v49, s8 v50, s8 v51, s8 v52, s8 v53, s8 v54, s8 v55,
        s8 v56, s8 v57, s8 v58, s8 v59, s8 v60, s8 v61, s8 v62, s8 v63)
    {
        auto lo = s8x32_set(v00, v01, v02, v03, v04, v05, v06, v07,
                            v08, v09, v10, v11, v12, v13, v14, v15,
                            v16, v17, v18, v19, v20, v21, v22, v23,
                            v24, v25, v26, v27, v28, v29, v30, v31);
        auto hi = s8x32_set(v32, v33, v34, v35, v36, v37, v38, v39,
                            v40, v41, v42, v43, v44, v45, v46, v47,
                            v48, v49, v50, v51, v52, v53, v54, v55,
                            v56, v57, v58, v59, v60, v61, v62, v63);
        return { lo, hi };
    }

    static inline s8x64 s8x64_uload(const void* source)
    {
        auto lo = s8x32_uload(reinterpret_cast<const s8*>(source) +  0);
        auto hi = s8x32_uload(reinterpret_cast<const s8*>(source) + 32);
        return { lo, hi };
    }

    static inline void s8x64_ustore(void* dest, s8x64 a)
    {
        s8x32_ustore(reinterpret_cast<s8*>(dest) +  0, a.data[0]);
        s8x32_ustore(reinterpret_cast<s8*>(dest) + 32, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, unpacklo)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, unpackhi)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, add)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, sub)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, adds)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, subs)
    SIMD_COMPOSITE_FUNC2(s8x64, s8x64, avg)
    SIMD_COMPOSITE_FUNC1(s8x64, s8x64, abs)
    SIMD_COMPOSITE_FUNC1(s8x64, s8x64, neg)

    SIMD_COMPOSITE_ZEROMASK_FUNC1(s8x64, s8x64, mask8x64, abs)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s8x64, s8x64, mask8x64, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s8x64, s8x64, mask8x64, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s8x64, s8x64, mask8x64, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s8x64, s8x64, mask8x64, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s8x64, s8x64, mask8x64, adds)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s8x64, s8x64, mask8x64, subs)
    SIMD_COMPOSITE_MASK_FUNC1(s8x64, s8x64, mask8x64, abs)
    SIMD_COMPOSITE_MASK_FUNC2(s8x64, s8x64, mask8x64, min)
    SIMD_COMPOSITE_MASK_FUNC2(s8x64, s8x64, mask8x64, max)
    SIMD_COMPOSITE_MASK_FUNC2(s8x64, s8x64, mask8x64, add)
    SIMD_COMPOSITE_MASK_FUNC2(s8x64, s8x64, mask8x64, sub)
    SIMD_COMPOSITE_MASK_FUNC2(s8x64, s8x64, mask8x64, adds)
    SIMD_COMPOSITE_MASK_FUNC2(s8x64, s8x64, mask8x64, subs)

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
        auto lo = s16x16_zero();
        auto hi = s16x16_zero();
        return { lo, hi };
    }

    static inline s16x32 s16x32_set(s16 s)
    {
        auto lo = s16x16_set(s);
        auto hi = s16x16_set(s);
        return { lo, hi };
    }

    static inline s16x32 s16x32_set(
        s16 v00, s16 v01, s16 v02, s16 v03, s16 v04, s16 v05, s16 v06, s16 v07,
        s16 v08, s16 v09, s16 v10, s16 v11, s16 v12, s16 v13, s16 v14, s16 v15,
        s16 v16, s16 v17, s16 v18, s16 v19, s16 v20, s16 v21, s16 v22, s16 v23,
        s16 v24, s16 v25, s16 v26, s16 v27, s16 v28, s16 v29, s16 v30, s16 v31)
    {
        auto lo = s16x16_set(v00, v01, v02, v03, v04, v05, v06, v07,
                             v08, v09, v10, v11, v12, v13, v14, v15);
        auto hi = s16x16_set(v16, v17, v18, v19, v20, v21, v22, v23,
                             v24, v25, v26, v27, v28, v29, v30, v31);
        return { lo, hi };
    }

    static inline s16x32 s16x32_uload(const void* source)
    {
        auto lo = s16x16_uload(reinterpret_cast<const s16*>(source) +  0);
        auto hi = s16x16_uload(reinterpret_cast<const s16*>(source) + 16);
        return { lo, hi };
    }

    static inline void s16x32_ustore(void* dest, s16x32 a)
    {
        s16x16_ustore(reinterpret_cast<s16*>(dest) +  0, a.data[0]);
        s16x16_ustore(reinterpret_cast<s16*>(dest) + 16, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, unpacklo)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, unpackhi)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, add)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, sub)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, mullo)
    SIMD_COMPOSITE_FUNC2(s32x16, s16x32, madd)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, adds)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, subs)
    SIMD_COMPOSITE_FUNC2(s16x32, s16x32, avg)
    SIMD_COMPOSITE_FUNC1(s16x32, s16x32, abs)
    SIMD_COMPOSITE_FUNC1(s16x32, s16x32, neg)

    SIMD_COMPOSITE_ZEROMASK_FUNC1(s16x32, s16x32, mask16x32, abs)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s16x32, s16x32, mask16x32, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s16x32, s16x32, mask16x32, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s16x32, s16x32, mask16x32, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s16x32, s16x32, mask16x32, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s16x32, s16x32, mask16x32, adds)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s16x32, s16x32, mask16x32, subs)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x16, s16x32, mask32x16, madd)
    SIMD_COMPOSITE_MASK_FUNC1(s16x32, s16x32, mask16x32, abs)
    SIMD_COMPOSITE_MASK_FUNC2(s16x32, s16x32, mask16x32, min)
    SIMD_COMPOSITE_MASK_FUNC2(s16x32, s16x32, mask16x32, max)
    SIMD_COMPOSITE_MASK_FUNC2(s16x32, s16x32, mask16x32, add)
    SIMD_COMPOSITE_MASK_FUNC2(s16x32, s16x32, mask16x32, sub)
    SIMD_COMPOSITE_MASK_FUNC2(s16x32, s16x32, mask16x32, adds)
    SIMD_COMPOSITE_MASK_FUNC2(s16x32, s16x32, mask16x32, subs)
    SIMD_COMPOSITE_MASK_FUNC2(s32x16, s16x32, mask32x16, madd)

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
        auto lo = slli<Count>(a.data[0]);
        auto hi = slli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline s16x32 srli(s16x32 a)
    {
        auto lo = srli<Count>(a.data[0]);
        auto hi = srli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline s16x32 srai(s16x32 a)
    {
        auto lo = srai<Count>(a.data[0]);
        auto hi = srai<Count>(a.data[1]);
        return { lo, hi };
    }

    // shift by scalar

    static inline s16x32 sll(s16x32 a, int count)
    {
        auto lo = sll(a.data[0], count);
        auto hi = sll(a.data[1], count);
        return { lo, hi };
    }

    static inline s16x32 srl(s16x32 a, int count)
    {
        auto lo = srl(a.data[0], count);
        auto hi = srl(a.data[1], count);
        return { lo, hi };
    }

    static inline s16x32 sra(s16x32 a, int count)
    {
        auto lo = sra(a.data[0], count);
        auto hi = sra(a.data[1], count);
        return { lo, hi };
    }

    // -----------------------------------------------------------------
    // s32x16
    // -----------------------------------------------------------------

    static inline s32x16 s32x16_zero()
    {
        auto lo = s32x8_zero();
        auto hi = s32x8_zero();
        return { lo, hi };
    }

    static inline s32x16 s32x16_set(s32 s)
    {
        auto lo = s32x8_set(s);
        auto hi = s32x8_set(s);
        return { lo, hi };
    }

    static inline s32x16 s32x16_set(
        s32 v00, s32 v01, s32 v02, s32 v03, s32 v04, s32 v05, s32 v06, s32 v07,
        s32 v08, s32 v09, s32 v10, s32 v11, s32 v12, s32 v13, s32 v14, s32 v15)
    {
        auto lo = s32x8_set(v00, v01, v02, v03, v04, v05, v06, v07);
        auto hi = s32x8_set(v08, v09, v10, v11, v12, v13, v14, v15);
        return { lo, hi };
    }

    static inline s32x16 s32x16_uload(const void* source)
    {
        auto lo = s32x8_uload(reinterpret_cast<const s32*>(source) + 0);
        auto hi = s32x8_uload(reinterpret_cast<const s32*>(source) + 8);
        return { lo, hi };
    }

    static inline void s32x16_ustore(void* dest, s32x16 a)
    {
        s32x8_ustore(reinterpret_cast<s32*>(dest) + 0, a.data[0]);
        s32x8_ustore(reinterpret_cast<s32*>(dest) + 8, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, unpacklo)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, unpackhi)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, add)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, sub)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, adds)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, subs)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, avg)
    SIMD_COMPOSITE_FUNC2(s32x16, s32x16, mullo)
    SIMD_COMPOSITE_FUNC1(s32x16, s32x16, abs)
    SIMD_COMPOSITE_FUNC1(s32x16, s32x16, neg)

    SIMD_COMPOSITE_ZEROMASK_FUNC1(s32x16, s32x16, mask32x16, abs)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x16, s32x16, mask32x16, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x16, s32x16, mask32x16, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x16, s32x16, mask32x16, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x16, s32x16, mask32x16, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x16, s32x16, mask32x16, adds)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s32x16, s32x16, mask32x16, subs)
    SIMD_COMPOSITE_MASK_FUNC1(s32x16, s32x16, mask32x16, abs)
    SIMD_COMPOSITE_MASK_FUNC2(s32x16, s32x16, mask32x16, min)
    SIMD_COMPOSITE_MASK_FUNC2(s32x16, s32x16, mask32x16, max)
    SIMD_COMPOSITE_MASK_FUNC2(s32x16, s32x16, mask32x16, add)
    SIMD_COMPOSITE_MASK_FUNC2(s32x16, s32x16, mask32x16, sub)
    SIMD_COMPOSITE_MASK_FUNC2(s32x16, s32x16, mask32x16, adds)
    SIMD_COMPOSITE_MASK_FUNC2(s32x16, s32x16, mask32x16, subs)

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
        auto lo = slli<Count>(a.data[0]);
        auto hi = slli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline s32x16 srli(s32x16 a)
    {
        auto lo = srli<Count>(a.data[0]);
        auto hi = srli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline s32x16 srai(s32x16 a)
    {
        auto lo = srai<Count>(a.data[0]);
        auto hi = srai<Count>(a.data[1]);
        return { lo, hi };
    }

    // shify by scalar

    static inline s32x16 sll(s32x16 a, int count)
    {
        auto lo = sll(a.data[0], count);
        auto hi = sll(a.data[1], count);
        return { lo, hi };
    }

    static inline s32x16 srl(s32x16 a, int count)
    {
        auto lo = srl(a.data[0], count);
        auto hi = srl(a.data[1], count);
        return { lo, hi };
    }

    static inline s32x16 sra(s32x16 a, int count)
    {
        auto lo = sra(a.data[0], count);
        auto hi = sra(a.data[1], count);
        return { lo, hi };
    }

    // shift by vector

    static inline s32x16 sll(s32x16 a, u32x16 count)
    {
        a.data[0] = sll(a.data[0], count.data[0]);
        a.data[1] = sll(a.data[1], count.data[1]);
        return a;
    }

    static inline s32x16 srl(s32x16 a, u32x16 count)
    {
        a.data[0] = srl(a.data[0], count.data[0]);
        a.data[1] = srl(a.data[1], count.data[1]);
        return a;
    }

    static inline s32x16 sra(s32x16 a, u32x16 count)
    {
        a.data[0] = sra(a.data[0], count.data[0]);
        a.data[1] = sra(a.data[1], count.data[1]);
        return a;
    }

    // -----------------------------------------------------------------
    // s64x8
    // -----------------------------------------------------------------

    static inline s64x8 s64x8_zero()
    {
        auto lo = s64x4_zero();
        auto hi = s64x4_zero();
        return { lo, hi };
    }

    static inline s64x8 s64x8_set(s64 s)
    {
        auto lo = s64x4_set(s);
        auto hi = s64x4_set(s);
        return { lo, hi };
    }

    static inline s64x8 s64x8_set(s64 v0, s64 v1, s64 v2, s64 v3, s64 v4, s64 v5, s64 v6, s64 v7)
    {
        auto lo = s64x4_set(v0, v1, v2, v3);
        auto hi = s64x4_set(v4, v5, v6, v7);
        return { lo, hi };
    }

    static inline s64x8 s64x8_uload(const void* source)
    {
        auto lo = s64x4_uload(reinterpret_cast<const s64*>(source) + 0);
        auto hi = s64x4_uload(reinterpret_cast<const s64*>(source) + 4);
        return { lo, hi };
    }

    static inline void s64x8_ustore(void* dest, s64x8 a)
    {
        s64x4_ustore(reinterpret_cast<s64*>(dest) + 0, a.data[0]);
        s64x4_ustore(reinterpret_cast<s64*>(dest) + 4, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, unpacklo)
    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, unpackhi)
    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, add)
    SIMD_COMPOSITE_FUNC2(s64x8, s64x8, sub)
    SIMD_COMPOSITE_FUNC1(s64x8, s64x8, neg)

    SIMD_COMPOSITE_ZEROMASK_FUNC2(s64x8, s64x8, mask64x8, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s64x8, s64x8, mask64x8, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s64x8, s64x8, mask64x8, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(s64x8, s64x8, mask64x8, sub)
    SIMD_COMPOSITE_MASK_FUNC2(s64x8, s64x8, mask64x8, min)
    SIMD_COMPOSITE_MASK_FUNC2(s64x8, s64x8, mask64x8, max)
    SIMD_COMPOSITE_MASK_FUNC2(s64x8, s64x8, mask64x8, add)
    SIMD_COMPOSITE_MASK_FUNC2(s64x8, s64x8, mask64x8, sub)

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
        auto lo = slli<Count>(a.data[0]);
        auto hi = slli<Count>(a.data[1]);
        return { lo, hi };
    }

    template <int Count>
    static inline s64x8 srli(s64x8 a)
    {
        auto lo = srli<Count>(a.data[0]);
        auto hi = srli<Count>(a.data[1]);
        return { lo, hi };
    }

    // shift by scalar

    static inline s64x8 sll(s64x8 a, int count)
    {
        auto lo = sll(a.data[0], count);
        auto hi = sll(a.data[1], count);
        return { lo, hi };
    }

    static inline s64x8 srl(s64x8 a, int count)
    {
        auto lo = srl(a.data[0], count);
        auto hi = srl(a.data[1], count);
        return { lo, hi };
    }

    // -----------------------------------------------------------------
    // mask8x64
    // -----------------------------------------------------------------

    static inline mask8x64 mask_and(mask8x64 a, mask8x64 b)
    {
        auto lo = mask_and(a.data[0], b.data[0]);
        auto hi = mask_and(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask8x64 mask_or(mask8x64 a, mask8x64 b)
    {
        auto lo = mask_or(a.data[0], b.data[0]);
        auto hi = mask_or(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask8x64 mask_xor(mask8x64 a, mask8x64 b)
    {
        auto lo = mask_xor(a.data[0], b.data[0]);
        auto hi = mask_xor(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask8x64 mask_not(mask8x64 a)
    {
        auto lo = mask_not(a.data[0]);
        auto hi = mask_not(a.data[1]);
        return { lo, hi };
    }

    static inline u64 get_mask(mask8x64 a)
    {
        u64 mask = get_mask(a.data[0]) | (u64(get_mask(a.data[1])) << 32);
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

    static inline mask16x32 mask_and(mask16x32 a, mask16x32 b)
    {
        auto lo = mask_and(a.data[0], b.data[0]);
        auto hi = mask_and(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask16x32 mask_or(mask16x32 a, mask16x32 b)
    {
        auto lo = mask_or(a.data[0], b.data[0]);
        auto hi = mask_or(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask16x32 mask_xor(mask16x32 a, mask16x32 b)
    {
        auto lo = mask_xor(a.data[0], b.data[0]);
        auto hi = mask_xor(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask16x32 mask_not(mask16x32 a)
    {
        auto lo = mask_not(a.data[0]);
        auto hi = mask_not(a.data[1]);
        return { lo, hi };
    }

    static inline u32 get_mask(mask16x32 a)
    {
        u32 mask = get_mask(a.data[0]) | (get_mask(a.data[1]) << 16);
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

    static inline mask32x16 mask_and(mask32x16 a, mask32x16 b)
    {
        auto lo = mask_and(a.data[0], b.data[0]);
        auto hi = mask_and(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask32x16 mask_or(mask32x16 a, mask32x16 b)
    {
        auto lo = mask_or(a.data[0], b.data[0]);
        auto hi = mask_or(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask32x16 mask_xor(mask32x16 a, mask32x16 b)
    {
        auto lo = mask_xor(a.data[0], b.data[0]);
        auto hi = mask_xor(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask32x16 mask_not(mask32x16 a)
    {
        auto lo = mask_not(a.data[0]);
        auto hi = mask_not(a.data[1]);
        return { lo, hi };
    }

    static inline u32 get_mask(mask32x16 a)
    {
        u32 mask = get_mask(a.data[0]) | (get_mask(a.data[1]) << 8);
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

    static inline mask64x8 mask_and(mask64x8 a, mask64x8 b)
    {
        auto lo = mask_and(a.data[0], b.data[0]);
        auto hi = mask_and(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask64x8 mask_or(mask64x8 a, mask64x8 b)
    {
        auto lo = mask_or(a.data[0], b.data[0]);
        auto hi = mask_or(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask64x8 mask_xor(mask64x8 a, mask64x8 b)
    {
        auto lo = mask_xor(a.data[0], b.data[0]);
        auto hi = mask_xor(a.data[1], b.data[1]);
        return { lo, hi };
    }

    static inline mask64x8 mask_not(mask64x8 a)
    {
        auto lo = mask_not(a.data[0]);
        auto hi = mask_not(a.data[1]);
        return { lo, hi };
    }

    static inline u32 get_mask(mask64x8 a)
    {
        u32 mask = get_mask(a.data[0]) | (get_mask(a.data[1]) << 4);
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
#undef SIMD_COMPOSITE_ZEROMASK_FUNC1
#undef SIMD_COMPOSITE_MASK_FUNC1
#undef SIMD_COMPOSITE_ZEROMASK_FUNC2
#undef SIMD_COMPOSITE_MASK_FUNC2
#undef SIMD_COMPOSITE_SELECT

} // namespace mango::simd
