/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/simd/simd.hpp>

namespace mango::simd
{

#define SIMD_COMPOSITE_FUNC1(R, A, FUNC) \
    static inline R FUNC(A a) \
    { \
        R result; \
        result.data[0] = FUNC(a.data[0]); \
        result.data[1] = FUNC(a.data[1]); \
        return result; \
    }

#define SIMD_COMPOSITE_FUNC2(R, AB, FUNC) \
    static inline R FUNC(AB a, AB b) \
    { \
        R result; \
        result.data[0] = FUNC(a.data[0], b.data[0]); \
        result.data[1] = FUNC(a.data[1], b.data[1]); \
        return result; \
    }

#define SIMD_COMPOSITE_FUNC3(R, ABC, FUNC) \
    static inline R FUNC(ABC a, ABC b, ABC c) \
    { \
        R result; \
        result.data[0] = FUNC(a.data[0], b.data[0], c.data[0]); \
        result.data[1] = FUNC(a.data[1], b.data[1], c.data[1]); \
        return result; \
    }

#define SIMD_COMPOSITE_ZEROMASK_FUNC2(R, AB, MASK, FUNC) \
    static inline R FUNC(AB a, AB b, MASK mask) \
    { \
        R result; \
        result.data[0] = FUNC(a.data[0], b.data[0], mask.data[0]); \
        result.data[1] = FUNC(a.data[1], b.data[1], mask.data[1]); \
        return result; \
    }

#define SIMD_COMPOSITE_MASK_FUNC2(R, AB, MASK, FUNC) \
    static inline R FUNC(AB a, AB b, MASK mask, AB value) \
    { \
        R result; \
        result.data[0] = FUNC(a.data[0], b.data[0], mask.data[0], value.data[0]); \
        result.data[1] = FUNC(a.data[1], b.data[1], mask.data[1], value.data[1]); \
        return result; \
    }

    // -----------------------------------------------------------------
    // f64x8
    // -----------------------------------------------------------------

    static inline f64x8 f64x8_zero()
    {
        f64x8 result;
        result.data[0] = f64x4_zero();
        result.data[1] = f64x4_zero();
        return result;
    }

    static inline f64x8 f64x8_set(f64 s)
    {
        f64x8 result;
        result.data[0] = f64x4_set(s);
        result.data[1] = f64x4_set(s);
        return result;
    }

    static inline f64x8 f64x8_set(f64 s0, f64 s1, f64 s2, f64 s3, f64 s4, f64 s5, f64 s6, f64 s7)
    {
        f64x8 result;
        result.data[0] = f64x4_set(s0, s1, s2, s3);
        result.data[1] = f64x4_set(s4, s5, s6, s7);
        return result;
    }

    static inline f64x8 f64x8_uload(const void* source)
    {
        f64x8 result;
        result.data[0] = f64x4_uload(reinterpret_cast<const f64*>(source) + 0);
        result.data[1] = f64x4_uload(reinterpret_cast<const f64*>(source) + 4);
        return result;
    }

    static inline void f64x8_ustore(void* dest, f64x8 a)
    {
        f64x4_ustore(reinterpret_cast<f64*>(dest) + 0, a.data[0]);
        f64x4_ustore(reinterpret_cast<f64*>(dest) + 4, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, unpackhi)
    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, unpacklo)
    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, bitwise_and)
    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, bitwise_or)
    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(f64x8, f64x8, bitwise_not)
    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, min)
    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, max)
    SIMD_COMPOSITE_FUNC1(f64x8, f64x8, abs)
    SIMD_COMPOSITE_FUNC1(f64x8, f64x8, neg)
    SIMD_COMPOSITE_FUNC1(f64x8, f64x8, sign)
    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, add)
    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, sub)
    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, mul)
    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, div)

    SIMD_COMPOSITE_ZEROMASK_FUNC2(f64x8, f64x8, mask64x8, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f64x8, f64x8, mask64x8, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f64x8, f64x8, mask64x8, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f64x8, f64x8, mask64x8, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f64x8, f64x8, mask64x8, mul)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f64x8, f64x8, mask64x8, div)
    SIMD_COMPOSITE_MASK_FUNC2(f64x8, f64x8, mask64x8, min)
    SIMD_COMPOSITE_MASK_FUNC2(f64x8, f64x8, mask64x8, max)
    SIMD_COMPOSITE_MASK_FUNC2(f64x8, f64x8, mask64x8, add)
    SIMD_COMPOSITE_MASK_FUNC2(f64x8, f64x8, mask64x8, sub)
    SIMD_COMPOSITE_MASK_FUNC2(f64x8, f64x8, mask64x8, mul)
    SIMD_COMPOSITE_MASK_FUNC2(f64x8, f64x8, mask64x8, div)

    static inline f64x8 div(f64x8 a, f64 b)
    {
        f64x8 result;
        result.data[0] = div(a.data[0], b);
        result.data[1] = div(a.data[1], b);
        return result;
    }

    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, hadd)
    SIMD_COMPOSITE_FUNC2(f64x8, f64x8, hsub)
    SIMD_COMPOSITE_FUNC3(f64x8, f64x8, madd)
    SIMD_COMPOSITE_FUNC3(f64x8, f64x8, msub)
    SIMD_COMPOSITE_FUNC3(f64x8, f64x8, nmadd)
    SIMD_COMPOSITE_FUNC3(f64x8, f64x8, nmsub)
    SIMD_COMPOSITE_FUNC3(f64x8, f64x8, lerp)
    SIMD_COMPOSITE_FUNC1(f64x8, f64x8, rcp)
    SIMD_COMPOSITE_FUNC1(f64x8, f64x8, rsqrt)
    SIMD_COMPOSITE_FUNC1(f64x8, f64x8, sqrt)

    // compare

    SIMD_COMPOSITE_FUNC2(mask64x8, f64x8, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask64x8, f64x8, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask64x8, f64x8, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask64x8, f64x8, compare_le)
    SIMD_COMPOSITE_FUNC2(mask64x8, f64x8, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask64x8, f64x8, compare_ge)

    static inline f64x8 select(mask64x8 mask, f64x8 a, f64x8 b)
    {
        f64x8 result;
        result.data[0] = select(mask.data[0], a.data[0], b.data[0]);
        result.data[1] = select(mask.data[1], a.data[1], b.data[1]);
        return result;
    }

    // rounding

    SIMD_COMPOSITE_FUNC1(f64x8, f64x8, round)
    SIMD_COMPOSITE_FUNC1(f64x8, f64x8, trunc)
    SIMD_COMPOSITE_FUNC1(f64x8, f64x8, floor)
    SIMD_COMPOSITE_FUNC1(f64x8, f64x8, ceil)
    SIMD_COMPOSITE_FUNC1(f64x8, f64x8, fract)

#undef SIMD_COMPOSITE_FUNC1
#undef SIMD_COMPOSITE_FUNC2
#undef SIMD_COMPOSITE_FUNC3
#undef SIMD_COMPOSITE_ZEROMASK_FUNC2
#undef SIMD_COMPOSITE_MASK_FUNC2

} // namespace mango::simd
