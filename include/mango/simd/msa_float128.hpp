/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline float32x4 shuffle(float32x4 a, float32x4 b)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const v4i32 control = (v4i32) { x, y, z + 4, w + 4 };
        return (v4f32) __msa_vshf_w(control, (v4i32) a, (v4i32) b);
    }

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline float32x4 shuffle(float32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const v4i32 control = (v4i32) { x, y, z, w };
        return (v4f32) __msa_vshf_w(control, (v4i32) v, (v4i32) v);
    }

    template <>
    inline float32x4 shuffle<0, 1, 2, 3>(float32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline float32x4 set_component(float32x4 a, float s)
    {
        static_assert(Index < 4, "Index out of range.");
        int32 temp;
        std::memcpy(&temp, &s, 4);
        return (v4f32) __msa_insert_w((v4i32) a, Index, temp);
    }

    template <int Index>
    static inline float get_component(float32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        uint32 temp = __msa_copy_u_w((v4u32) a, Index);
        float s;
        std::memcpy(&s, &temp, 4);
        return s;
    }

    static inline float32x4 float32x4_zero()
    {
        return (v4f32) { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    static inline float32x4 float32x4_set1(float s)
    {
        return (v4f32) { s, s, s, s };
    }

    static inline float32x4 float32x4_set4(float x, float y, float z, float w)
    {
        return (v4f32) { x, y, z, w };
    }

    static inline float32x4 float32x4_uload(const float* source)
    {
        return reinterpret_cast<const v4f32 *>(source)[0];
    }

    static inline void float32x4_ustore(float* dest, float32x4 a)
    {
        reinterpret_cast<v4f32 *>(dest)[0] = a;
    }

    static inline float32x4 movelh(float32x4 a, float32x4 b)
    {
        return (v4f32) __msa_ilvl_d((v2i64) b, (v2i64) a);
    }

    static inline float32x4 movehl(float32x4 a, float32x4 b)
    {
        return (v4f32) __msa_ilvr_d((v2i64) b, (v2i64) a);
    }

    static inline float32x4 unpackhi(float32x4 a, float32x4 b)
    {
        return (v4f32) __msa_ilvr_w((v4i32) b, (v4i32) a);
    }

    static inline float32x4 unpacklo(float32x4 a, float32x4 b)
    {
        return (v4f32) __msa_ilvl_w((v4i32) b, (v4i32) a);
    }

    // bitwise

    static inline float32x4 bitwise_nand(float32x4 a, float32x4 b)
    {
        return (v4f32) __msa_and_v((v16u8)a, __msa_nor_v((v16u8)b, (v16u8)b));
    }

    static inline float32x4 bitwise_and(float32x4 a, float32x4 b)
    {
        return (v4f32) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline float32x4 bitwise_or(float32x4 a, float32x4 b)
    {
        return (v4f32) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline float32x4 bitwise_xor(float32x4 a, float32x4 b)
    {
        return (v4f32) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline float32x4 bitwise_not(float32x4 a)
    {
        return (v4f32) __msa_nor_v((v16u8) a, (v16u8) a);
    }

    static inline float32x4 min(float32x4 a, float32x4 b)
    {
        return __msa_fmin_w(a, b);
    }

    static inline float32x4 max(float32x4 a, float32x4 b)
    {
        return __msa_fmax_w(a, b);
    }

    static inline float32x4 hmin(float32x4 a)
    {
        auto temp = __msa_fmin_w(a, shuffle<2, 3, 0, 1>(a, a));
        return __msa_fmin_w(temp, shuffle<1, 0, 3, 2>(temp, temp));
    }

    static inline float32x4 hmax(float32x4 a)
    {
        auto temp = __msa_fmax_w(a, shuffle<2, 3, 0, 1>(a, a));
        return __msa_fmax_w(temp, shuffle<1, 0, 3, 2>(temp, temp));
    }

    static inline float32x4 abs(float32x4 a)
    {
        return __msa_fmax_a_w(a, a);
    }

    static inline float32x4 neg(float32x4 a)
    {
        return __msa_fsub_w(float32x4_zero(), a);
    }

    static inline float32x4 sign(float32x4 a)
    {
        v16u8 sign_mask = (v16u8) float32x4_set1(-0.0f);
        v16u8 value_mask = (v16u8) __msa_fcune_w(a, float32x4_zero());
        v16u8 sign_bits = __msa_and_v((v16u8) a, sign_mask);
        v16u8 value_bits = __msa_and_v(value_mask, (v16u8) float32x4_set1(1.0f));
        return (v4f32) __msa_or_v(value_bits, sign_bits);
    }

    static inline float32x4 add(float32x4 a, float32x4 b)
    {
        return __msa_fadd_w(a, b);
    }

    static inline float32x4 sub(float32x4 a, float32x4 b)
    {
        return __msa_fsub_w(a, b);
    }

    static inline float32x4 mul(float32x4 a, float32x4 b)
    {
        return __msa_fmul_w(a, b);
    }

    static inline float32x4 div(float32x4 a, float32x4 b)
    {
        return __msa_fdiv_w(a, b);
    }

    static inline float32x4 div(float32x4 a, float b)
    {
        return __msa_fdiv_w(a, float32x4_set1(b));
    }

    static inline float32x4 hadd(float32x4 a, float32x4 b)
    {
        return add(shuffle<0, 2, 0, 2>(a, b),
                   shuffle<1, 3, 1, 3>(a, b));
    }

    static inline float32x4 madd(float32x4 a, float32x4 b, float32x4 c)
    {
        return __msa_fmadd_w(a, b, c);
    }

    static inline float32x4 msub(float32x4 a, float32x4 b, float32x4 c)
    {
        return __msa_fmsub_w(a, b, c);
    }

    static inline float32x4 fast_rcp(float32x4 a)
    {
        return __msa_frcp_w(a);
    }

    static inline float32x4 fast_rsqrt(float32x4 a)
    {
        return __msa_frsqrt_w(a);
    }

    static inline float32x4 fast_sqrt(float32x4 a)
    {
        return __msa_fsqrt_w(a);
    }

    static inline float32x4 rcp(float32x4 a)
    {
        auto estimate = __msa_frcp_w(a);
        auto temp = __msa_fmul_w(a, estimate);
        return __msa_fmul_w(estimate, __msa_fsub_w(float32x4_set1(2.0f), temp));
    }

    static inline float32x4 rsqrt(float32x4 a)
    {
        float32x4 n = __msa_frsqrt_w(a);
        float32x4 e = __msa_fmul_w(__msa_fmul_w(n, n), a);
        n = __msa_fmul_w(float32x4_set1(0.5f), n);
        e = __msa_fsub_w(float32x4_set1(3.0f), e);
        return __msa_fmul_w(n, e);
    }

    static inline float32x4 sqrt(float32x4 a)
    {
        return __msa_fsqrt_w(a);
    }

    static inline float dot3(float32x4 a, float32x4 b)
    {
        float32x4 s = mul(a, b);
        s = add(shuffle<0, 0, 0, 0>(s),
            add(shuffle<1, 1, 1, 1>(s), shuffle<2, 2, 2, 2>(s)));
        return get_component<0>(float32x4(s));
    }

    static inline float dot4(float32x4 a, float32x4 b)
    {
        float32x4 s = mul(a, b);
        s = add(s, shuffle<2, 3, 0, 1>(s));
        s = add(s, shuffle<1, 0, 3, 2>(s));
        return get_component<0>(s);
    }

    static inline float32x4 cross3(float32x4 a, float32x4 b)
    {
        float32x4 c = sub(mul(a, shuffle<1, 2, 0, 3>(b)),
                          mul(b, shuffle<1, 2, 0, 3>(a)));
        return shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline mask32x4 compare_neq(float32x4 a, float32x4 b)
    {
        return (v4u32) __msa_fcune_w(a, b);
    }

    static inline mask32x4 compare_eq(float32x4 a, float32x4 b)
    {
        return (v4u32) __msa_fceq_w(a, b);
    }

    static inline mask32x4 compare_lt(float32x4 a, float32x4 b)
    {
        return (v4u32) __msa_fclt_w(a, b);
    }

    static inline mask32x4 compare_le(float32x4 a, float32x4 b)
    {
        return (v4u32) __msa_fcle_w(a, b);
    }

    static inline mask32x4 compare_gt(float32x4 a, float32x4 b)
    {
        return (v4u32) __msa_fclt_w(b, a);
    }

    static inline mask32x4 compare_ge(float32x4 a, float32x4 b)
    {
        return (v4u32) __msa_fcle_w(b, a);
    }

    static inline float32x4 select(mask32x4 mask, float32x4 a, float32x4 b)
    {
        return (v4f32) __msa_bsel_v((v16u8) mask, (v16u8) a, (v16u8) b);
    }

    // rounding

    static inline float32x4 round(float32x4 s)
    {
        v4i32 temp = __msa_ftint_s_w(s);
        return __msa_ffint_s_w(temp);
    }

    static inline float32x4 trunc(float32x4 s)
    {
        v4i32 temp = __msa_ftrunc_s_w(s);
        return __msa_ffint_s_w(temp);
    }

    static inline float32x4 floor(float32x4 s)
    {
        float32x4 temp = round(s);
        v16u8 mask = (v16u8) __msa_fclt_w(s, temp);
        v16u8 one = (v16u8) __msa_fill_w(0x3f800000);
        return __msa_fsub_w(temp, (float32x4) __msa_and_v(mask, one));
    }

    static inline float32x4 ceil(float32x4 s)
    {
        float32x4 temp = round(s);
        v16u8 mask = (v16u8) __msa_fclt_w(temp, s);
        v16u8 one = (v16u8) __msa_fill_w(0x3f800000);
        return __msa_fadd_w(temp, (float32x4) __msa_and_v(mask, one));
    }

    static inline float32x4 fract(float32x4 s)
    {
        return sub(s, floor(s));
    }

} // namespace simd
} // namespace mango
