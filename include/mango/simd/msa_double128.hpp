/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        const v4i32 control = (v4i32) { x * 2 + 0, x * 2 + 1, y * 2 + 0, y * 2 + 1 };
        return (v2f64) __msa_vshf_w(control, (v4i32) v, (v4i32) v);
    }

    template <uint32 x, uint32 y>
    static inline float64x2 shuffle(float64x2 a, float64x2 b)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        const v4i32 control = (v4i32) { x * 2 + 0, x * 2 + 1, y * 2 + 4, y * 2 + 5 };
        return (v2f64) __msa_vshf_w(control, (v4i32) a, (v4i32) b);
    }

    // indexed access

    template <int Index>
    static inline float64x2 set_component(float64x2 a, double s)
    {
        static_assert(Index < 2, "Index out of range.");
        int64 temp;
        std::memcpy(&temp, &s, 8);
        return (v2f64) __msa_insert_d((v2i64) a, Index, temp);
    }

    template <int Index>
    static inline double get_component(float64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        uint64 temp = __msa_copy_u_d((v2u64) a, Index);
        double s;
        std::memcpy(&s, &temp, 8);
        return s;
    }

    static inline float64x2 float64x2_zero()
    {
        return (v2f64) { 0.0, 0.0 };
    }

    static inline float64x2 float64x2_set1(double s)
    {
        return (v2f64) { s, s };
    }

    static inline float64x2 float64x2_set2(double x, double y)
    {
        return (v2f64) { x, y };
    }

    static inline float64x2 float64x2_uload(const double* source)
    {
        return reinterpret_cast<const v2f64 *>(source)[0];
    }

    static inline void float64x2_ustore(double* dest, float64x2 a)
    {
        reinterpret_cast<v2f64 *>(dest)[0] = a;
    }

    static inline float64x2 unpackhi(float64x2 a, float64x2 b)
    {
        return (v2f64) __msa_ilvr_d((v2i64) b, (v2i64) a);
    }

    static inline float64x2 unpacklo(float64x2 a, float64x2 b)
    {
        return (v2f64) __msa_ilvl_d((v2i64) b, (v2i64) a);
    }

    // bitwise

    static inline float64x2 bitwise_nand(float64x2 a, float64x2 b)
    {
        return (v2f64) __msa_and_v((v16u8)a, __msa_nor_v((v16u8)b, (v16u8)b));
    }

    static inline float64x2 bitwise_and(float64x2 a, float64x2 b)
    {
        return (v2f64) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline float64x2 bitwise_or(float64x2 a, float64x2 b)
    {
        return (v2f64) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline float64x2 bitwise_xor(float64x2 a, float64x2 b)
    {
        return (v2f64) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline float64x2 bitwise_not(float64x2 a)
    {
        return (v2f64) __msa_nor_v((v16u8) a, (v16u8) a);
    }

    static inline float64x2 min(float64x2 a, float64x2 b)
    {
        return __msa_fmin_d(a, b);
    }

    static inline float64x2 max(float64x2 a, float64x2 b)
    {
        return __msa_fmax_d(a, b);
    }

    static inline float64x2 abs(float64x2 a)
    {
        return __msa_fmax_a_d(a, a);
    }

    static inline float64x2 neg(float64x2 a)
    {
        return __msa_fsub_d(float64x2_zero(), a);
    }

    static inline float64x2 sign(float64x2 a)
    {
        v16u8 sign_mask = (v16u8) float64x2_set1(-0.0);
        v16u8 value_mask = (v16u8) __msa_fcune_d(a, float64x2_zero());
        v16u8 sign_bits = __msa_and_v((v16u8) a, sign_mask);
        v16u8 value_bits = __msa_and_v(value_mask, (v16u8) float64x2_set1(1.0));
        return (v2f64) __msa_or_v(value_bits, sign_bits);
    }

    static inline float64x2 add(float64x2 a, float64x2 b)
    {
        return __msa_fadd_d(a, b);
    }

    static inline float64x2 sub(float64x2 a, float64x2 b)
    {
        return __msa_fsub_d(a, b);
    }

    static inline float64x2 mul(float64x2 a, float64x2 b)
    {
        return __msa_fmul_d(a, b);
    }

    static inline float64x2 div(float64x2 a, float64x2 b)
    {
        return __msa_fdiv_d(a, b);
    }

    static inline float64x2 div(float64x2 a, double b)
    {
        return __msa_fdiv_d(a, float64x2_set1(b));
    }

    static inline float64x2 madd(float64x2 a, float64x2 b, float64x2 c)
    {
        return __msa_fmadd_d(a, b, c);
    }

    static inline float64x2 msub(float64x2 a, float64x2 b, float64x2 c)
    {
        return __msa_fmsub_d(a, b, c);
    }

    static inline float64x2 fast_rcp(float64x2 a)
    {
        return __msa_frcp_d(a);
    }

    static inline float64x2 fast_rsqrt(float64x2 a)
    {
        return __msa_frsqrt_d(a);
    }

    static inline float64x2 fast_sqrt(float64x2 a)
    {
        return __msa_fsqrt_d(a);
    }

    static inline float64x2 rcp(float64x2 a)
    {
        auto estimate = __msa_frcp_d(a);
        auto temp = __msa_fmul_d(a, estimate);
        return __msa_fmul_d(estimate, __msa_fsub_d(float64x2_set1(2.0), temp));
    }

    static inline float64x2 rsqrt(float64x2 a)
    {
        auto n = __msa_frsqrt_d(a);
        auto e = __msa_fmul_d(__msa_fmul_d(n, n), a);
        n = __msa_fmul_d(float64x2_set1(0.5), n);
        e = __msa_fsub_d(float64x2_set1(3.0), e);
        return __msa_fmul_d(n, e);
    }

    static inline float64x2 sqrt(float64x2 a)
    {
        return __msa_fsqrt_d(a);
    }

    static inline double dot2(float64x2 a, float64x2 b)
    {
        float64x2 xy = __msa_fmul_d(a, b);
        float64x2 yx = shuffle<1, 0>(xy);
        float64x2 s = __msa_fadd_d(xy, yx);
        return get_component<0>(s);
    }

    // compare

    static inline mask64x2 compare_neq(float64x2 a, float64x2 b)
    {
        return (v2u64) __msa_fcune_d(a, b);
    }

    static inline mask64x2 compare_eq(float64x2 a, float64x2 b)
    {
        return (v2u64) __msa_fceq_d(a, b);
    }

    static inline mask64x2 compare_lt(float64x2 a, float64x2 b)
    {
        return (v2u64) __msa_fclt_d(a, b);
    }

    static inline mask64x2 compare_le(float64x2 a, float64x2 b)
    {
        return (v2u64) __msa_fcle_d(a, b);
    }

    static inline mask64x2 compare_gt(float64x2 a, float64x2 b)
    {
        return (v2u64) __msa_fclt_d(b, a);
    }

    static inline mask64x2 compare_ge(float64x2 a, float64x2 b)
    {
        return (v2u64) __msa_fcle_d(b, a);
    }

    static inline float64x2 select(mask64x2 mask, float64x2 a, float64x2 b)
    {
        return (v2f64) __msa_bsel_v((v16u8) mask, (v16u8) a, (v16u8) b);
    }

    // rounding

    static inline float64x2 round(float64x2 s)
    {
        v2i64 temp = __msa_ftint_s_d(s);
        return __msa_ffint_s_d(temp);
    }

    static inline float64x2 trunc(float64x2 s)
    {
        v2i64 temp = __msa_ftrunc_s_d(s);
        return __msa_ffint_s_d(temp);
    }

    static inline float64x2 floor(float64x2 s)
    {
        float64x2 temp = round(s);
        v16u8 mask = (v16u8) __msa_fclt_d(s, temp);
        v16u8 one = (v16u8) __msa_fill_d(0x3ff0000000000000);
        return __msa_fsub_d(temp, (float64x2) __msa_and_v(mask, one));
    }

    static inline float64x2 ceil(float64x2 s)
    {
        float64x2 temp = round(s);
        v16u8 mask = (v16u8) __msa_fclt_d(temp, s);
        v16u8 one = (v16u8) __msa_fill_d(0x3ff0000000000000);
        return __msa_fadd_d(temp, (float64x2) __msa_and_v(mask, one));
    }

    static inline float64x2 fract(float64x2 s)
    {
        return sub(s, floor(s));
    }

} // namespace simd
} // namespace mango
