/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float64x2
    // -----------------------------------------------------------------

    template <uint32 x, uint32 y>
    static inline float64x2 shuffle(float64x2 v)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return vec_permi(v, v, x * 2 + y);
    }

    template <uint32 x, uint32 y>
    static inline float64x2 shuffle(float64x2 a, float64x2 b)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return vec_permi(a, b, x * 2 + y);
    }

    template <>
    inline float64x2 shuffle<0, 1>(float64x2 v)
    {
        // .xy
        return v;
    }

    template <>
    inline float64x2 shuffle<0, 0>(float64x2 v)
    {
        // .xx
        return vec_splat(v, 0);
    }

    template <>
    inline float64x2 shuffle<1, 1>(float64x2 v)
    {
        // .yy
        return vec_splat(v, 1);
    }
    
    // indexed access

    template <unsigned int Index>
    static inline float64x2 set_component(float64x2 a, double s)
    {
        static_assert(Index < 2, "Index out of range.");
        return vec_insert(s, a, Index);
    }

    template <int Index>
    static inline double get_component(float64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return vec_extract(a, Index);
    }

    static inline float64x2 float64x2_zero()
    {
        return vec_splats(0.0);
    }

    static inline float64x2 float64x2_set1(double s)
    {
        return vec_splats(s);
    }

    static inline float64x2 float64x2_set2(double x, double y)
    {
        return {{ x, y }};
    }

    static inline float64x2 float64x2_uload(const double* s)
    {
        return {{ s[0], s[1] }};
    }

    static inline void float64x2_ustore(double* dest, float64x2 a)
    {
        dest[0] = vec_extract(a, 0);
        dest[1] = vec_extract(a, 1);
    }

    static inline float64x2 unpackhi(float64x2 a, float64x2 b)
    {
        return vec_mergeh(a, b);
    }

    static inline float64x2 unpacklo(float64x2 a, float64x2 b)
    {
        return vec_mergel(a, b);
    }

    // bitwise

    static inline float64x2 bitwise_nand(float64x2 a, float64x2 b)
    {
        return vec_nand(a, b);
    }

    static inline float64x2 bitwise_and(float64x2 a, float64x2 b)
    {
        return vec_and(a, b);
    }

    static inline float64x2 bitwise_or(float64x2 a, float64x2 b)
    {
        return vec_or(a, b);
    }

    static inline float64x2 bitwise_xor(float64x2 a, float64x2 b)
    {
        return vec_xor(a, b);
    }

    static inline float64x2 bitwise_not(float64x2 a)
    {
        return vec_nor(a, a);
    }

    static inline float64x2 min(float64x2 a, float64x2 b)
    {
        return vec_min(a, b);
    }

    static inline float64x2 max(float64x2 a, float64x2 b)
    {
        return vec_max(a, b);
    }

    static inline float64x2 abs(float64x2 a)
    {
        return vec_abs(a);
    }

    static inline float64x2 neg(float64x2 a)
    {
        return vec_neg(a);
    }

    static inline float64x2 sign(float64x2 a)
    {
        auto sign_mask = vec_splats(-0.0);
        auto zero_mask = vec_cmpeq(a, vec_splats(0.0));
        auto value_mask = vec_nor(zero_mask, zero_mask);
        auto sign_bits = vec_and(a, sign_mask);
        auto value_bits = vec_and(value_mask, vec_splats(1.0));
        return vec_or(value_bits, sign_bits);
    }

    static inline float64x2 add(float64x2 a, float64x2 b)
    {
        return vec_add(a, b);
    }

    static inline float64x2 sub(float64x2 a, float64x2 b)
    {
        return vec_sub(a, b);
    }

    static inline float64x2 mul(float64x2 a, float64x2 b)
    {
        return vec_mul(a, b);
    }

    static inline float64x2 div(float64x2 a, float64x2 b)
    {
        return vec_div(a, b);
    }

    static inline float64x2 div(float64x2 a, double b)
    {
        return vec_div(a, vec_splats(b));
    }

    static inline float64x2 madd(float64x2 a, float64x2 b, float64x2 c)
    {
        return vec_madd(b, c, a);
    }

    static inline float64x2 msub(float64x2 a, float64x2 b, float64x2 c)
    {
        return vec_neg(vec_nmsub(b, c, a));
    }

    static inline float64x2 fast_rcp(float64x2 a)
    {
        return vec_re(a);
    }

    static inline float64x2 fast_rsqrt(float64x2 a)
    {
        return vec_re(vec_sqrt(a));
    }

    static inline float64x2 fast_sqrt(float64x2 a)
    {
        return vec_sqrt(a);
    }

    static inline float64x2 rcp(float64x2 a)
    {
        return vec_re(a);
    }

    static inline float64x2 rsqrt(float64x2 a)
    {
        return vec_re(vec_sqrt(a));
    }

    static inline float64x2 sqrt(float64x2 a)
    {
        return vec_sqrt(a);
    }

    static inline float64x2 dot2(float64x2 a, float64x2 b)
    {
        auto s = vec_mul(a, b);
        return vec_add(s, vec_permi(s, s, 2));
    }

    // compare

    static inline mask64x2 compare_neq(float64x2 a, float64x2 b)
    {
        auto mask = vec_cmpeq(a, b);
        return vec_nor(mask, mask);
    }

    static inline mask64x2 compare_eq(float64x2 a, float64x2 b)
    {
        return vec_cmpeq(a, b);
    }

    static inline mask64x2 compare_lt(float64x2 a, float64x2 b)
    {
        return vec_cmplt(a, b);
    }

    static inline mask64x2 compare_le(float64x2 a, float64x2 b)
    {
        return vec_cmple(a, b);
    }

    static inline mask64x2 compare_gt(float64x2 a, float64x2 b)
    {
        return vec_cmpgt(a, b);
    }

    static inline mask64x2 compare_ge(float64x2 a, float64x2 b)
    {
        return vec_cmpge(a, b);
    }

    static inline float64x2 select(mask64x2 mask, float64x2 a, float64x2 b)
    {
        return vec_sel(b, a, mask);
    }

    // rounding

    static inline float64x2 round(float64x2 s)
    {
        return vec_round(s);
    }

    static inline float64x2 trunc(float64x2 s)
    {
        return vec_trunc(s);
    }

    static inline float64x2 floor(float64x2 s)
    {
        return vec_floor(s);
    }

    static inline float64x2 ceil(float64x2 s)
    {
        return vec_ceil(s);
    }

    static inline float64x2 fract(float64x2 s)
    {
        return sub(s, floor(s));
    }

} // namespace simd
} // namespace mango
