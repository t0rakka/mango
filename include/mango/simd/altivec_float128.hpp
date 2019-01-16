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
        const uint8x16::vector mask = { VEC_SH4(x, 0), VEC_SH4(y, 0), VEC_SH4(z, 1), VEC_SH4(w, 1) };
        return vec_perm(a.data, b.data, mask);
    }

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline float32x4 shuffle(float32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const uint8x16::vector mask = { VEC_SH4(x, 0), VEC_SH4(y, 0), VEC_SH4(z, 0), VEC_SH4(w, 0) };
        return vec_perm(v.data, v.data, mask);
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
        return vec_splat(v.data, 0);
    }

    template <>
    inline float32x4 shuffle<1, 1, 1, 1>(float32x4 v)
    {
        // .yyyy
        return vec_splat(v.data, 1);
    }

    template <>
    inline float32x4 shuffle<2, 2, 2, 2>(float32x4 v)
    {
        // .zzzz
        return vec_splat(v.data, 2);
    }

    template <>
    inline float32x4 shuffle<3, 3, 3, 3>(float32x4 v)
    {
        // .wwww
        return vec_splat(v.data, 3);
    }

    // indexed access

    template <unsigned int Index>
    static inline float32x4 set_component(float32x4 a, float s)
    {
        static_assert(Index < 4, "Index out of range.");
        return vec_insert(s, a.data, Index);
    }

    template <int Index>
    static inline float get_component(float32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return vec_extract(a.data, Index);
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
        return (float32x4::vector) { x, y, z, w };
    }

    static inline float32x4 float32x4_uload(const float* s)
    {
        return vec_xl(0, s);
    }

    static inline void float32x4_ustore(float* dest, float32x4 a)
    {
        vec_xst(a.data, 0, dest);
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
        return vec_mergeh(a.data, b.data);
    }

    static inline float32x4 unpackhi(float32x4 a, float32x4 b)
    {
        return vec_mergel(a.data, b.data);
    }

    // bitwise

    static inline float32x4 bitwise_nand(float32x4 a, float32x4 b)
    {
        return vec_nand(a.data, b.data);
    }

    static inline float32x4 bitwise_and(float32x4 a, float32x4 b)
    {
        return vec_and(a.data, b.data);
    }

    static inline float32x4 bitwise_or(float32x4 a, float32x4 b)
    {
        return vec_or(a.data, b.data);
    }

    static inline float32x4 bitwise_xor(float32x4 a, float32x4 b)
    {
        return vec_xor(a.data, b.data);
    }

    static inline float32x4 bitwise_not(float32x4 a)
    {
        return vec_nor(a.data, a.data);
    }

    static inline float32x4 min(float32x4 a, float32x4 b)
    {
        return vec_min(a.data, b.data);
    }

    static inline float32x4 max(float32x4 a, float32x4 b)
    {
        return vec_max(a.data, b.data);
    }

    static inline float32x4 hmin(float32x4 a)
    {
        auto temp = vec_min(a.data, (float32x4::vector) shuffle<2, 3, 0, 1>(a, a));
        return vec_min(temp, (float32x4::vector) shuffle<1, 0, 3, 2>(temp, temp));
    }

    static inline float32x4 hmax(float32x4 a)
    {
        auto temp = vec_max(a.data, (float32x4::vector) shuffle<2, 3, 0, 1>(a, a));
        return vec_max(temp, (float32x4::vector) shuffle<1, 0, 3, 2>(temp, temp));
    }

    static inline float32x4 abs(float32x4 a)
    {
        return vec_abs(a.data);
    }

    static inline float32x4 neg(float32x4 a)
    {
        return vec_sub(vec_xor(a.data, a.data), a.data);
    }

    static inline float32x4 sign(float32x4 a)
    {
        auto sign_mask = vec_splats(-0.0f);
        auto zero_mask = vec_cmpeq(a.data, vec_splats(0.0f));
        auto value_mask = vec_nor(zero_mask, zero_mask);
        auto sign_bits = vec_and(a.data, sign_mask);
        auto value_bits = vec_and(value_mask, vec_splats(1.0f));
        return vec_or(value_bits, sign_bits);
    }

    static inline float32x4 add(float32x4 a, float32x4 b)
    {
        return vec_add(a.data, b.data);
    }

    static inline float32x4 sub(float32x4 a, float32x4 b)
    {
        return vec_sub(a.data, b.data);
    }

    static inline float32x4 mul(float32x4 a, float32x4 b)
    {
        return vec_mul(a.data, b.data);
    }

    static inline float32x4 div(float32x4 a, float32x4 b)
    {
        return vec_div(a.data, b.data);
    }

    static inline float32x4 div(float32x4 a, float b)
    {
        return vec_div(a.data, vec_splats(b));
    }

    static inline float32x4 hadd(float32x4 a, float32x4 b)
    {
        return vec_add((float32x4::vector) shuffle<0, 2, 0, 2>(a, b),
                       (float32x4::vector) shuffle<1, 3, 1, 3>(a, b));
    }

    static inline float32x4 madd(float32x4 a, float32x4 b, float32x4 c)
    {
        return vec_madd(b.data, c.data, a.data);
    }

    static inline float32x4 msub(float32x4 a, float32x4 b, float32x4 c)
    {
        return neg(vec_nmsub(b.data, c.data, a.data));
    }

    static inline float32x4 fast_rcp(float32x4 a)
    {
        return vec_re(a.data);
    }

    static inline float32x4 fast_rsqrt(float32x4 a)
    {
        return vec_rsqrte(a.data);
    }

    static inline float32x4 fast_sqrt(float32x4 a)
    {
        return vec_sqrt(a.data);
    }

    static inline float32x4 rcp(float32x4 a)
    {
        return vec_re(a.data);
    }

    static inline float32x4 rsqrt(float32x4 a)
    {
        return vec_rsqrt(a.data);
    }

    static inline float32x4 sqrt(float32x4 a)
    {
        return vec_sqrt(a.data);
    }

    static inline float dot3(float32x4 a, float32x4 b)
    {
        float32x4 prod = vec_mul(a.data, b.data);
        float32x4 s = vec_add((float32x4::vector) shuffle<0, 0, 0, 0>(prod),
                      vec_add((float32x4::vector) shuffle<1, 1, 1, 1>(prod), 
                              (float32x4::vector) shuffle<2, 2, 2, 2>(prod)));
        return get_component<0>(s);
    }

    static inline float dot4(float32x4 a, float32x4 b)
    {
        float32x4 s = vec_mul(a.data, b.data);
        s = vec_add(s, (float32x4::vector) shuffle<2, 3, 0, 1>(s));
        s = vec_add(s, (float32x4::vector) shuffle<1, 0, 3, 2>(s));
        return get_component<0>(s);
    }

    static inline float32x4 cross3(float32x4 a, float32x4 b)
    {
        float32x4 c = vec_sub(vec_mul(a.data, (float32x4::vector) shuffle<1, 2, 0, 3>(b)),
                              vec_mul(b.data, (float32x4::vector) shuffle<1, 2, 0, 3>(a)));
        return shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline mask32x4 compare_neq(float32x4 a, float32x4 b)
    {
        auto mask = vec_cmpeq(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask32x4 compare_eq(float32x4 a, float32x4 b)
    {
        return vec_cmpeq(a.data, b.data);
    }

    static inline mask32x4 compare_lt(float32x4 a, float32x4 b)
    {
        return vec_cmplt(a.data, b.data);
    }

    static inline mask32x4 compare_le(float32x4 a, float32x4 b)
    {
        return vec_cmple(a.data, b.data);
    }

    static inline mask32x4 compare_gt(float32x4 a, float32x4 b)
    {
        return vec_cmpgt(a.data, b.data);
    }

    static inline mask32x4 compare_ge(float32x4 a, float32x4 b)
    {
        return vec_cmpge(a.data, b.data);
    }

    static inline float32x4 select(mask32x4 mask, float32x4 a, float32x4 b)
    {
        return vec_sel(b.data, a.data, mask.data);
    }

    // rounding

    static inline float32x4 round(float32x4 s)
    {
        return vec_round(s.data);
    }

    static inline float32x4 trunc(float32x4 s)
    {
        return vec_trunc(s.data);
    }

    static inline float32x4 floor(float32x4 s)
    {
        return vec_floor(s.data);
    }

    static inline float32x4 ceil(float32x4 s)
    {
        return vec_ceil(s.data);
    }

    static inline float32x4 fract(float32x4 s)
    {
        return sub(s, floor(s));
    }

} // namespace simd
} // namespace mango
