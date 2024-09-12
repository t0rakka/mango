/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/simd/simd.hpp>

namespace mango::simd
{

    // -----------------------------------------------------------------
    // f32x8
    // -----------------------------------------------------------------

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

    static inline f32x8 f32x8_zero()
    {
        f32x8 result;
        result.data[0] = f32x4_zero();
        result.data[1] = f32x4_zero();
        return result;
    }

    static inline f32x8 f32x8_set(f32 s)
    {
        f32x8 result;
        result.data[0] = f32x4_set(s);
        result.data[1] = f32x4_set(s);
        return result;
    }

    static inline f32x8 f32x8_set(f32 s0, f32 s1, f32 s2, f32 s3, f32 s4, f32 s5, f32 s6, f32 s7)
    {
        f32x8 result;
        result.data[0] = f32x4_set(s0, s1, s2, s3);
        result.data[1] = f32x4_set(s4, s5, s6, s7);
        return result;
    }

    static inline f32x8 f32x8_uload(const void* source)
    {
        f32x8 result;
        result.data[0] = f32x4_uload(reinterpret_cast<const f32*>(source) + 0);
        result.data[1] = f32x4_uload(reinterpret_cast<const f32*>(source) + 4);
        return result;
    }

    static inline void f32x8_ustore(void* dest, f32x8 a)
    {
        f32x4_ustore(reinterpret_cast<f32*>(dest) + 0, a.data[0]);
        f32x4_ustore(reinterpret_cast<f32*>(dest) + 4, a.data[1]);
    }

    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, unpackhi)
    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, unpacklo)
    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, bitwise_and)
    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, bitwise_or)
    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(f32x8, f32x8, bitwise_not)
    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, min)
    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, max)
    SIMD_COMPOSITE_FUNC1(f32x8, f32x8, abs)
    SIMD_COMPOSITE_FUNC1(f32x8, f32x8, neg)
    SIMD_COMPOSITE_FUNC1(f32x8, f32x8, sign)
    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, add)
    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, sub)
    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, mul)
    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, div)

    SIMD_COMPOSITE_ZEROMASK_FUNC2(f32x8, f32x8, mask32x8, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f32x8, f32x8, mask32x8, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f32x8, f32x8, mask32x8, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f32x8, f32x8, mask32x8, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f32x8, f32x8, mask32x8, mul)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f32x8, f32x8, mask32x8, div)
    SIMD_COMPOSITE_MASK_FUNC2(f32x8, f32x8, mask32x8, min)
    SIMD_COMPOSITE_MASK_FUNC2(f32x8, f32x8, mask32x8, max)
    SIMD_COMPOSITE_MASK_FUNC2(f32x8, f32x8, mask32x8, add)
    SIMD_COMPOSITE_MASK_FUNC2(f32x8, f32x8, mask32x8, sub)
    SIMD_COMPOSITE_MASK_FUNC2(f32x8, f32x8, mask32x8, mul)
    SIMD_COMPOSITE_MASK_FUNC2(f32x8, f32x8, mask32x8, div)

    static inline f32x8 div(f32x8 a, f32 b)
    {
        f32x8 result;
        result.data[0] = div(a.data[0], b);
        result.data[1] = div(a.data[1], b);
        return result;
    }

    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, hadd)
    SIMD_COMPOSITE_FUNC2(f32x8, f32x8, hsub)
    SIMD_COMPOSITE_FUNC3(f32x8, f32x8, madd)
    SIMD_COMPOSITE_FUNC3(f32x8, f32x8, msub)
    SIMD_COMPOSITE_FUNC3(f32x8, f32x8, nmadd)
    SIMD_COMPOSITE_FUNC3(f32x8, f32x8, nmsub)
    SIMD_COMPOSITE_FUNC3(f32x8, f32x8, lerp)
    SIMD_COMPOSITE_FUNC1(f32x8, f32x8, rcp)
    SIMD_COMPOSITE_FUNC1(f32x8, f32x8, rsqrt)
    SIMD_COMPOSITE_FUNC1(f32x8, f32x8, sqrt)

    // compare

    SIMD_COMPOSITE_FUNC2(mask32x8, f32x8, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask32x8, f32x8, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask32x8, f32x8, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask32x8, f32x8, compare_le)
    SIMD_COMPOSITE_FUNC2(mask32x8, f32x8, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask32x8, f32x8, compare_ge)

    static inline f32x8 select(mask32x8 mask, f32x8 a, f32x8 b)
    {
        f32x8 result;
        result.data[0] = select(mask.data[0], a.data[0], b.data[0]);
        result.data[1] = select(mask.data[1], a.data[1], b.data[1]);
        return result;
    }

    // rounding

    SIMD_COMPOSITE_FUNC1(f32x8, f32x8, round)
    SIMD_COMPOSITE_FUNC1(f32x8, f32x8, trunc)
    SIMD_COMPOSITE_FUNC1(f32x8, f32x8, floor)
    SIMD_COMPOSITE_FUNC1(f32x8, f32x8, ceil)
    SIMD_COMPOSITE_FUNC1(f32x8, f32x8, fract)

#undef SIMD_COMPOSITE_FUNC1
#undef SIMD_COMPOSITE_FUNC2
#undef SIMD_COMPOSITE_FUNC3
#undef SIMD_COMPOSITE_ZEROMASK_FUNC2
#undef SIMD_COMPOSITE_MASK_FUNC2

    // -----------------------------------------------------------------
    // f64x4
    // -----------------------------------------------------------------

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

    // shuffle

    template <u32 x, u32 y, u32 z, u32 w>
    static inline f64x4 shuffle(f64x4 v)
    {
        const f64x2 v0 = x & 2 ? v.data[1] : v.data[0];
        const f64x2 v1 = y & 2 ? v.data[1] : v.data[0];
        const f64x2 v2 = z & 2 ? v.data[1] : v.data[0];
        const f64x2 v3 = w & 2 ? v.data[1] : v.data[0];

        f64x4 result;
        result.data[0] = shuffle<x & 1, y & 1>(v0, v1);
        result.data[1] = shuffle<z & 1, w & 1>(v2, v3);
        return result;
    }

    template <>
    inline f64x4 shuffle<0, 1, 2, 3>(f64x4 v)
    {
        // .xyzw
        return v;
    }

    template <>
    inline f64x4 shuffle<0, 0, 0, 0>(f64x4 v)
    {
        // .xxxx
        const f64x2 xx = shuffle<0, 0>(v.data[0]);
        f64x4 result;
        result.data[0] = xx;
        result.data[1] = xx;
        return result;
    }

    template <>
    inline f64x4 shuffle<1, 1, 1, 1>(f64x4 v)
    {
        // .yyyy
        const f64x2 yy = shuffle<1, 1>(v.data[0]);
        f64x4 result;
        result.data[0] = yy;
        result.data[1] = yy;
        return result;
    }

    template <>
    inline f64x4 shuffle<2, 2, 2, 2>(f64x4 v)
    {
        // .zzzz
        const f64x2 zz = shuffle<0, 0>(v.data[1]);
        f64x4 result;
        result.data[0] = zz;
        result.data[1] = zz;
        return result;
    }

    template <>
    inline f64x4 shuffle<3, 3, 3, 3>(f64x4 v)
    {
        // .wwww
        const f64x2 ww = shuffle<1, 1>(v.data[1]);
        f64x4 result;
        result.data[0] = ww;
        result.data[1] = ww;
        return result;
    }

    // set component

    template <unsigned int Index>
    static inline f64x4 set_component(f64x4 a, f64 s)
    {
        static_assert(Index < 4, "Index out of range.");
        switch (Index)
        {
            case 0: a.data[0] = set_component<0>(a.data[0], s); break;
            case 1: a.data[0] = set_component<1>(a.data[0], s); break;
            case 2: a.data[1] = set_component<0>(a.data[1], s); break;
            case 3: a.data[1] = set_component<1>(a.data[1], s); break;
        }
        return a;
    }

    // get component

    template <unsigned int Index>
    static inline f64 get_component(f64x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        f64 s = 0.0;
        switch (Index)
        {
            case 0: s = get_component<0>(a.data[0]); break;
            case 1: s = get_component<1>(a.data[0]); break;
            case 2: s = get_component<0>(a.data[1]); break;
            case 3: s = get_component<1>(a.data[1]); break;
        }
        return s;
    }

    static inline f64x4 f64x4_zero()
    {
        f64x4 result;
        result.data[0] = f64x2_zero();
        result.data[1] = f64x2_zero();
        return result;
    }

    static inline f64x4 f64x4_set(f64 s)
    {
        f64x4 result;
        result.data[0] = f64x2_set(s);
        result.data[1] = f64x2_set(s);
        return result;
    }

    static inline f64x4 f64x4_set(f64 x, f64 y, f64 z, f64 w)
    {
        f64x4 result;
        result.data[0] = f64x2_set(x, y);
        result.data[1] = f64x2_set(z, w);
        return result;
    }

    static inline f64x4 f64x4_uload(const void* source)
    {
        f64x4 result;
        result.data[0] = f64x2_uload(reinterpret_cast<const f64*>(source) + 0);
        result.data[1] = f64x2_uload(reinterpret_cast<const f64*>(source) + 2);
        return result;
    }

    static inline void f64x4_ustore(void* dest, f64x4 a)
    {
        f64x2_ustore(reinterpret_cast<f64*>(dest) + 0, a.data[0]);
        f64x2_ustore(reinterpret_cast<f64*>(dest) + 2, a.data[1]);
    }

    static inline f64x4 movelh(f64x4 a, f64x4 b)
    {
        f64x4 result;
        result.data[0] = a.data[0];
        result.data[1] = b.data[0];
        return result;
    }

    static inline f64x4 movehl(f64x4 a, f64x4 b)
    {
        f64x4 result;
        result.data[0] = b.data[1];
        result.data[1] = a.data[1];
        return result;
    }

    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, unpackhi)
    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, unpacklo)
    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, bitwise_nand)
    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, bitwise_and)
    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, bitwise_or)
    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, bitwise_xor)
    SIMD_COMPOSITE_FUNC1(f64x4, f64x4, bitwise_not)
    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, min)
    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, max)

    static inline f64x4 hmin(f64x4 a)
    {
        const f64x2 xy = min(a.data[0], shuffle<1, 0>(a.data[0]));
        const f64x2 zw = min(a.data[1], shuffle<1, 0>(a.data[1]));
        const f64x2 s = min(xy, zw);
        f64x4 result;
        result.data[0] = s;
        result.data[1] = s;
        return result;
    }

    static inline f64x4 hmax(f64x4 a)
    {
        const f64x2 xy = max(a.data[0], shuffle<1, 0>(a.data[0]));
        const f64x2 zw = max(a.data[1], shuffle<1, 0>(a.data[1]));
        const f64x2 s = max(xy, zw);
        f64x4 result;
        result.data[0] = s;
        result.data[1] = s;
        return result;
    }

    SIMD_COMPOSITE_FUNC1(f64x4, f64x4, abs)
    SIMD_COMPOSITE_FUNC1(f64x4, f64x4, neg)
    SIMD_COMPOSITE_FUNC1(f64x4, f64x4, sign)
    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, add)
    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, sub)
    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, mul)
    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, div)

    SIMD_COMPOSITE_ZEROMASK_FUNC2(f64x4, f64x4, mask64x4, min)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f64x4, f64x4, mask64x4, max)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f64x4, f64x4, mask64x4, add)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f64x4, f64x4, mask64x4, sub)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f64x4, f64x4, mask64x4, mul)
    SIMD_COMPOSITE_ZEROMASK_FUNC2(f64x4, f64x4, mask64x4, div)
    SIMD_COMPOSITE_MASK_FUNC2(f64x4, f64x4, mask64x4, min)
    SIMD_COMPOSITE_MASK_FUNC2(f64x4, f64x4, mask64x4, max)
    SIMD_COMPOSITE_MASK_FUNC2(f64x4, f64x4, mask64x4, add)
    SIMD_COMPOSITE_MASK_FUNC2(f64x4, f64x4, mask64x4, sub)
    SIMD_COMPOSITE_MASK_FUNC2(f64x4, f64x4, mask64x4, mul)
    SIMD_COMPOSITE_MASK_FUNC2(f64x4, f64x4, mask64x4, div)

    static inline f64x4 div(f64x4 a, f64 b)
    {
        f64x4 result;
        result.data[0] = div(a.data[0], b);
        result.data[1] = div(a.data[1], b);
        return result;
    }

    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, hadd)
    SIMD_COMPOSITE_FUNC2(f64x4, f64x4, hsub)
    SIMD_COMPOSITE_FUNC3(f64x4, f64x4, madd)
    SIMD_COMPOSITE_FUNC3(f64x4, f64x4, msub)
    SIMD_COMPOSITE_FUNC3(f64x4, f64x4, nmadd)
    SIMD_COMPOSITE_FUNC3(f64x4, f64x4, nmsub)
    SIMD_COMPOSITE_FUNC3(f64x4, f64x4, lerp)
    SIMD_COMPOSITE_FUNC1(f64x4, f64x4, rcp)
    SIMD_COMPOSITE_FUNC1(f64x4, f64x4, rsqrt)
    SIMD_COMPOSITE_FUNC1(f64x4, f64x4, sqrt)

    static inline f64 dot4(f64x4 a, f64x4 b)
    {
        f64 low = dot2(a.data[0], b.data[0]);
        f64 high = dot2(a.data[1], b.data[1]);
        return low + high;
    }

    // compare

    SIMD_COMPOSITE_FUNC2(mask64x4, f64x4, compare_neq)
    SIMD_COMPOSITE_FUNC2(mask64x4, f64x4, compare_eq)
    SIMD_COMPOSITE_FUNC2(mask64x4, f64x4, compare_lt)
    SIMD_COMPOSITE_FUNC2(mask64x4, f64x4, compare_le)
    SIMD_COMPOSITE_FUNC2(mask64x4, f64x4, compare_gt)
    SIMD_COMPOSITE_FUNC2(mask64x4, f64x4, compare_ge)

    static inline f64x4 select(mask64x4 mask, f64x4 a, f64x4 b)
    {
        f64x4 result;
        result.data[0] = select(mask.data[0], a.data[0], b.data[0]);
        result.data[1] = select(mask.data[1], a.data[1], b.data[1]);
        return result;
    }

    // rounding

    SIMD_COMPOSITE_FUNC1(f64x4, f64x4, round)
    SIMD_COMPOSITE_FUNC1(f64x4, f64x4, trunc)
    SIMD_COMPOSITE_FUNC1(f64x4, f64x4, floor)
    SIMD_COMPOSITE_FUNC1(f64x4, f64x4, ceil)
    SIMD_COMPOSITE_FUNC1(f64x4, f64x4, fract)

#undef SIMD_COMPOSITE_FUNC1
#undef SIMD_COMPOSITE_FUNC2
#undef SIMD_COMPOSITE_FUNC3
#undef SIMD_COMPOSITE_ZEROMASK_FUNC2
#undef SIMD_COMPOSITE_MASK_FUNC2

} // namespace mango::simd
