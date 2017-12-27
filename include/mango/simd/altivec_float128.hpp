/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float32x4
    // -----------------------------------------------------------------

    // shuffle

#define VEC_SH4(n, select) \
    (select * 16 + n * 4 + 0), \
    (select * 16 + n * 4 + 1), \
    (select * 16 + n * 4 + 2), \
    (select * 16 + n * 4 + 3)

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline float32x4 shuffle(float32x4 a, float32x4 b)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const uint8x16 mask = {{ VEC_SH4(x, 0), VEC_SH4(y, 0), VEC_SH4(z, 1), VEC_SH4(w, 1) }};
        return vec_perm(a, b, mask);
    }

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline float32x4 shuffle(float32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const uint8x16 mask = {{ VEC_SH4(x, 0), VEC_SH4(y, 0), VEC_SH4(z, 0), VEC_SH4(w, 0) }};
        return vec_perm(v, v, mask);
    }

#undef VEC_SH4

    template <>
    inline float32x4 shuffle<0, 1, 2, 3>(float32x4 v)
    {
        // .xyzw
        return v;
    }

    template <>
    inline float32x4 shuffle<0, 0, 0, 0>(float32x4 v)
    {
        // .xxxx
        return vec_splat(v, 0);
    }

    template <>
    inline float32x4 shuffle<1, 1, 1, 1>(float32x4 v)
    {
        // .yyyy
        return vec_splat(v, 1);
    }

    template <>
    inline float32x4 shuffle<2, 2, 2, 2>(float32x4 v)
    {
        // .zzzz
        return vec_splat(v, 2);
    }

    template <>
    inline float32x4 shuffle<3, 3, 3, 3>(float32x4 v)
    {
        // .wwww
        return vec_splat(v, 3);
    }

    // indexed access

    template <unsigned int Index>
    static inline float32x4 set_component(float32x4 a, float s)
    {
        static_assert(Index < 4, "Index out of range.");
        return vec_insert(s, a, Index);
    }

    template <int Index>
    static inline float get_component(float32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return vec_extract(a, Index);
    }

    static inline float32x4 float32x4_zero()
    {
        return vec_splats(0.0f);
    }

    static inline float32x4 float32x4_set1(float s)
    {
        return vec_splats(s);
    }

    static inline float32x4 float32x4_set4(float x, float y, float z, float w)
    {
        return {{ x, y, z, w }};
    }

    static inline float32x4 float32x4_uload(const float* s)
    {
        return {{ s[0], s[1], s[2], s[3] }};
    }

    static inline void float32x4_ustore(float* dest, float32x4 a)
    {
        dest[0] = vec_extract(a, 0);
        dest[1] = vec_extract(a, 1);
        dest[2] = vec_extract(a, 2);
        dest[3] = vec_extract(a, 3);
    }

    static inline float32x4 movelh(float32x4 a, float32x4 b)
    {
        return shuffle<0, 1, 0, 1>(a, b);
    }

    static inline float32x4 movehl(float32x4 a, float32x4 b)
    {
        return shuffle<2, 3, 2, 3>(a, b);
    }

    static inline float32x4 unpacklo(float32x4 a, float32x4 b)
    {
        return vec_mergeh(a, b);
    }

    static inline float32x4 unpackhi(float32x4 a, float32x4 b)
    {
        return vec_mergel(a, b);
    }

    // bitwise

    static inline float32x4 bitwise_nand(float32x4 a, float32x4 b)
    {
        return vec_nand(a, b);
    }

    static inline float32x4 bitwise_and(float32x4 a, float32x4 b)
    {
        return vec_and(a, b);
    }

    static inline float32x4 bitwise_or(float32x4 a, float32x4 b)
    {
        return vec_or(a, b);
    }

    static inline float32x4 bitwise_xor(float32x4 a, float32x4 b)
    {
        return vec_xor(a, b);
    }

    static inline float32x4 bitwise_not(float32x4 a)
    {
        return vec_nor(a, a);
    }

    static inline float32x4 min(float32x4 a, float32x4 b)
    {
        return vec_min(a, b);
    }

    static inline float32x4 max(float32x4 a, float32x4 b)
    {
        return vec_max(a, b);
    }

    static inline float32x4 hmin(float32x4 a)
    {
        auto temp = vec_min(a, shuffle<2, 3, 0, 1>(a, a));
        return vec_min(temp, shuffle<1, 0, 3, 2>(temp, temp));
    }

    static inline float32x4 hmax(float32x4 a)
    {
        auto temp = vec_max(a, shuffle<2, 3, 0, 1>(a, a));
        return vec_max(temp, shuffle<1, 0, 3, 2>(temp, temp));
    }

    static inline float32x4 abs(float32x4 a)
    {
        return vec_abs(a);
    }

    static inline float32x4 neg(float32x4 a)
    {
        return vec_neg(a);
    }

    static inline float32x4 sign(float32x4 a)
    {
        auto sign_mask = vec_splats(-0.0f);
        auto zero_mask = vec_cmpeq(a, vec_splats(0.0f));
        auto value_mask = vec_nor(zero_mask, zero_mask);
        auto sign_bits = vec_and(a, sign_mask);
        auto value_bits = vec_and(value_mask, vec_splats(1.0f));
        return vec_or(value_bits, sign_bits);
    }

    static inline float32x4 add(float32x4 a, float32x4 b)
    {
        return vec_add(a, b);
    }

    static inline float32x4 sub(float32x4 a, float32x4 b)
    {
        return vec_sub(a, b);
    }

    static inline float32x4 mul(float32x4 a, float32x4 b)
    {
        return vec_mul(a, b);
    }

    static inline float32x4 div(float32x4 a, float32x4 b)
    {
        return vec_div(a, b);
    }

    static inline float32x4 div(float32x4 a, float b)
    {
        return vec_div(a, vec_splats(b));
    }

    static inline float32x4 hadd(float32x4 a, float32x4 b)
    {
        return vec_add(shuffle<0, 2, 0, 2>(a, b),
                       shuffle<1, 3, 1, 3>(a, b));
    }

    static inline float32x4 madd(float32x4 a, float32x4 b, float32x4 c)
    {
        return vec_madd(b, c, a);
    }

    static inline float32x4 msub(float32x4 a, float32x4 b, float32x4 c)
    {
        return vec_neg(vec_nmsub(b, c, a));
    }

    static inline float32x4 fast_rcp(float32x4 a)
    {
        return vec_re(a);
    }

    static inline float32x4 fast_rsqrt(float32x4 a)
    {
        return vec_re(vec_sqrt(a));
    }

    static inline float32x4 fast_sqrt(float32x4 a)
    {
        return vec_sqrt(a);
    }

    static inline float32x4 rcp(float32x4 a)
    {
        return vec_re(a);
    }

    static inline float32x4 rsqrt(float32x4 a)
    {
        return vec_re(vec_sqrt(a));
    }

    static inline float32x4 sqrt(float32x4 a)
    {
        return vec_sqrt(a);
    }

    static inline float32x4 dot3(float32x4 a, float32x4 b)
    {
        float32x4 s = vec_mul(a, b);
        return vec_add(shuffle<0, 0, 0, 0>(s),
               vec_add(shuffle<1, 1, 1, 1>(s), shuffle<2, 2, 2, 2>(s)));
    }

    static inline float32x4 dot4(float32x4 a, float32x4 b)
    {
        float32x4 s = vec_mul(a, b);
        s = vec_add(s, shuffle<2, 3, 0, 1>(s));
        s = vec_add(s, shuffle<1, 0, 3, 2>(s));
        return s;
    }

    static inline float32x4 cross3(float32x4 a, float32x4 b)
    {
        float32x4 c = vec_sub(vec_mul(a, shuffle<1, 2, 0, 3>(b)),
                              vec_mul(b, shuffle<1, 2, 0, 3>(a)));
        return shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline mask32x4 compare_neq(float32x4 a, float32x4 b)
    {
        auto mask = vec_cmpeq(a, b);
        return vec_nor(mask, mask);
    }

    static inline mask32x4 compare_eq(float32x4 a, float32x4 b)
    {
        return vec_cmpeq(a, b);
    }

    static inline mask32x4 compare_lt(float32x4 a, float32x4 b)
    {
        return vec_cmplt(a, b);
    }

    static inline mask32x4 compare_le(float32x4 a, float32x4 b)
    {
        return vec_cmple(a, b);
    }

    static inline mask32x4 compare_gt(float32x4 a, float32x4 b)
    {
        return vec_cmpgt(a, b);
    }

    static inline mask32x4 compare_ge(float32x4 a, float32x4 b)
    {
        return vec_cmpge(a, b);
    }

    static inline float32x4 select(mask32x4 mask, float32x4 a, float32x4 b)
    {
        return vec_sel(b, a, mask);
    }

    // rounding

    static inline float32x4 round(float32x4 s)
    {
        return vec_round(s);
    }

    static inline float32x4 trunc(float32x4 s)
    {
        return vec_trunc(s);
    }

    static inline float32x4 floor(float32x4 s)
    {
        return vec_floor(s);
    }

    static inline float32x4 ceil(float32x4 s)
    {
        return vec_ceil(s);
    }

    static inline float32x4 fract(float32x4 s)
    {
        return sub(s, floor(s));
    }

} // namespace simd
} // namespace mango
