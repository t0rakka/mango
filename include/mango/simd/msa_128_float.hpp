/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/simd/simd.hpp>

namespace mango::simd
{

    // -----------------------------------------------------------------
    // f32x4
    // -----------------------------------------------------------------

    // shuffle

    template <u32 x, u32 y, u32 z, u32 w>
    static inline f32x4 shuffle(f32x4 a, f32x4 b)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const v4i32 control = (v4i32) { x, y, z + 4, w + 4 };
        return (v4f32) __msa_vshf_w(control, (v4i32) a, (v4i32) b);
    }

    template <u32 x, u32 y, u32 z, u32 w>
    static inline f32x4 shuffle(f32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const v4i32 control = (v4i32) { x, y, z, w };
        return (v4f32) __msa_vshf_w(control, (v4i32) v, (v4i32) v);
    }

    template <>
    inline f32x4 shuffle<0, 1, 2, 3>(f32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline f32x4 set_component(f32x4 a, f32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        s32 temp;
        std::memcpy(&temp, &s, 4);
        return (v4f32) __msa_insert_w((v4i32) a, Index, temp);
    }

    template <unsigned int Index>
    static inline f32 get_component(f32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        u32 temp = __msa_copy_u_w((v4u32) a, Index);
        f32 s;
        std::memcpy(&s, &temp, 4);
        return s;
    }

    static inline f32x4 f32x4_zero()
    {
        return (v4f32) { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    static inline f32x4 f32x4_set(f32 s)
    {
        return (v4f32) { s, s, s, s };
    }

    static inline f32x4 f32x4_set(f32 x, f32 y, f32 z, f32 w)
    {
        return (v4f32) { x, y, z, w };
    }

    static inline f32x4 f32x4_uload(const void* source)
    {
        //return v4f32(__msa_ld_w(source, 0));
        return reinterpret_cast<const v4f32 *>(source)[0];
    }

    static inline void f32x4_ustore(void* dest, f32x4 a)
    {
        //__msa_st_w(v4i32(a), dest, 0);
        reinterpret_cast<v4f32 *>(dest)[0] = a;
    }

    static inline f32x4 movelh(f32x4 a, f32x4 b)
    {
        return (v4f32) __msa_ilvl_d((v2i64) b, (v2i64) a);
    }

    static inline f32x4 movehl(f32x4 a, f32x4 b)
    {
        return (v4f32) __msa_ilvr_d((v2i64) b, (v2i64) a);
    }

    static inline f32x4 unpackhi(f32x4 a, f32x4 b)
    {
        return (v4f32) __msa_ilvr_w((v4i32) b, (v4i32) a);
    }

    static inline f32x4 unpacklo(f32x4 a, f32x4 b)
    {
        return (v4f32) __msa_ilvl_w((v4i32) b, (v4i32) a);
    }

    // bitwise

    static inline f32x4 bitwise_nand(f32x4 a, f32x4 b)
    {
        return (v4f32) __msa_and_v((v16u8)a, __msa_nor_v((v16u8)b, (v16u8)b));
    }

    static inline f32x4 bitwise_and(f32x4 a, f32x4 b)
    {
        return (v4f32) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline f32x4 bitwise_or(f32x4 a, f32x4 b)
    {
        return (v4f32) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline f32x4 bitwise_xor(f32x4 a, f32x4 b)
    {
        return (v4f32) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline f32x4 bitwise_not(f32x4 a)
    {
        return (v4f32) __msa_nor_v((v16u8) a, (v16u8) a);
    }

    static inline f32x4 min(f32x4 a, f32x4 b)
    {
        return __msa_fmin_w(a, b);
    }

    static inline f32x4 max(f32x4 a, f32x4 b)
    {
        return __msa_fmax_w(a, b);
    }

    static inline f32x4 hmin(f32x4 a)
    {
        auto temp = __msa_fmin_w(a, shuffle<2, 3, 0, 1>(a, a));
        return __msa_fmin_w(temp, shuffle<1, 0, 3, 2>(temp, temp));
    }

    static inline f32x4 hmax(f32x4 a)
    {
        auto temp = __msa_fmax_w(a, shuffle<2, 3, 0, 1>(a, a));
        return __msa_fmax_w(temp, shuffle<1, 0, 3, 2>(temp, temp));
    }

    static inline f32x4 abs(f32x4 a)
    {
        return __msa_fmax_a_w(a, a);
    }

    static inline f32x4 neg(f32x4 a)
    {
        return __msa_fsub_w(f32x4_zero(), a);
    }

    static inline f32x4 sign(f32x4 a)
    {
        v16u8 sign_mask = (v16u8) f32x4_set(-0.0f);
        v16u8 sign_bits = __msa_and_v((v16u8) a, sign_mask);
        v16u8 value_mask = (v16u8) __msa_fcune_w(a, f32x4_zero());
        v16u8 value_bits = __msa_and_v(value_mask, (v16u8) f32x4_set(1.0f));
        return (v4f32) __msa_or_v(value_bits, sign_bits);
    }

    static inline f32x4 add(f32x4 a, f32x4 b)
    {
        return __msa_fadd_w(a, b);
    }

    static inline f32x4 sub(f32x4 a, f32x4 b)
    {
        return __msa_fsub_w(a, b);
    }

    static inline f32x4 mul(f32x4 a, f32x4 b)
    {
        return __msa_fmul_w(a, b);
    }

    static inline f32x4 div(f32x4 a, f32x4 b)
    {
        return __msa_fdiv_w(a, b);
    }

    static inline f32x4 div(f32x4 a, f32 b)
    {
        return __msa_fdiv_w(a, f32x4_set(b));
    }

    static inline f32x4 hadd(f32x4 a, f32x4 b)
    {
        return add(shuffle<0, 2, 0, 2>(a, b),
                   shuffle<1, 3, 1, 3>(a, b));
    }

    static inline f32x4 hsub(f32x4 a, f32x4 b)
    {
        return sub(shuffle<0, 2, 0, 2>(a, b),
                   shuffle<1, 3, 1, 3>(a, b));
    }

    static inline f32x4 madd(f32x4 a, f32x4 b, f32x4 c)
    {
        // a + b * c
        return __msa_fmadd_w(a, b, c);
    }

    static inline f32x4 msub(f32x4 a, f32x4 b, f32x4 c)
    {
        // b * c - a
        return __msa_fsub_w(f32x4_zero(), __msa_fmsub_w(a, b, c));
    }

    static inline f32x4 nmadd(f32x4 a, f32x4 b, f32x4 c)
    {
        // a - b * c
        return __msa_fmsub_w(a, b, c);
    }

    static inline f32x4 nmsub(f32x4 a, f32x4 b, f32x4 c)
    {
        // -(a + b * c)
        return __msa_fsub_w(f32x4_zero(), __msa_fmadd_w(a, b, c));
    }

    static inline f32x4 lerp(f32x4 a, f32x4 b, f32x4 s)
    {
        // a * (1.0 - s) + b * s
        // (a - a * s) + (b * s)
        return madd(nmadd(a, a, s), b, s);
    }

#if defined(MANGO_FAST_MATH)

    static inline f32x4 rcp(f32x4 a)
    {
        return __msa_frcp_w(a);
    }

    static inline f32x4 rsqrt(f32x4 a)
    {
        return __msa_frsqrt_w(a);
    }

#else

    static inline f32x4 rcp(f32x4 a)
    {
        auto estimate = __msa_frcp_w(a);
        auto temp = __msa_fmul_w(a, estimate);
        return __msa_fmul_w(estimate, __msa_fsub_w(f32x4_set(2.0f), temp));
    }

    static inline f32x4 rsqrt(f32x4 a)
    {
        f32x4 n = __msa_frsqrt_w(a);
        f32x4 e = __msa_fmul_w(__msa_fmul_w(n, n), a);
        n = __msa_fmul_w(f32x4_set(0.5f), n);
        e = __msa_fsub_w(f32x4_set(3.0f), e);
        return __msa_fmul_w(n, e);
    }

#endif

    static inline f32x4 sqrt(f32x4 a)
    {
        return __msa_fsqrt_w(a);
    }

    static inline f32 dot3(f32x4 a, f32x4 b)
    {
        f32x4 s = mul(a, b);
        s = add(shuffle<0, 0, 0, 0>(s),
            add(shuffle<1, 1, 1, 1>(s), shuffle<2, 2, 2, 2>(s)));
        return get_component<0>(f32x4(s));
    }

    static inline f32 dot4(f32x4 a, f32x4 b)
    {
        f32x4 s = mul(a, b);
        s = add(s, shuffle<2, 3, 0, 1>(s));
        s = add(s, shuffle<1, 0, 3, 2>(s));
        return get_component<0>(s);
    }

    static inline f32x4 cross3(f32x4 a, f32x4 b)
    {
        f32x4 c = sub(mul(a, shuffle<1, 2, 0, 3>(b)),
                          mul(b, shuffle<1, 2, 0, 3>(a)));
        return shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline mask32x4 compare_neq(f32x4 a, f32x4 b)
    {
        return (v4u32) __msa_fcune_w(a, b);
    }

    static inline mask32x4 compare_eq(f32x4 a, f32x4 b)
    {
        return (v4u32) __msa_fceq_w(a, b);
    }

    static inline mask32x4 compare_lt(f32x4 a, f32x4 b)
    {
        return (v4u32) __msa_fclt_w(a, b);
    }

    static inline mask32x4 compare_le(f32x4 a, f32x4 b)
    {
        return (v4u32) __msa_fcle_w(a, b);
    }

    static inline mask32x4 compare_gt(f32x4 a, f32x4 b)
    {
        return (v4u32) __msa_fclt_w(b, a);
    }

    static inline mask32x4 compare_ge(f32x4 a, f32x4 b)
    {
        return (v4u32) __msa_fcle_w(b, a);
    }

    static inline f32x4 select(mask32x4 mask, f32x4 a, f32x4 b)
    {
        return (v4f32) __msa_bsel_v((v16u8) mask, (v16u8) a, (v16u8) b);
    }

    // rounding

    static inline f32x4 round(f32x4 s)
    {
        v4i32 temp = __msa_ftint_s_w(s);
        return __msa_ffint_s_w(temp);
    }

    static inline f32x4 trunc(f32x4 s)
    {
        v4i32 temp = __msa_ftrunc_s_w(s);
        return __msa_ffint_s_w(temp);
    }

    static inline f32x4 floor(f32x4 s)
    {
        f32x4 temp = round(s);
        v16u8 mask = (v16u8) __msa_fclt_w(s, temp);
        v16u8 one = (v16u8) __msa_fill_w(0x3f800000);
        return __msa_fsub_w(temp, (f32x4) __msa_and_v(mask, one));
    }

    static inline f32x4 ceil(f32x4 s)
    {
        f32x4 temp = round(s);
        v16u8 mask = (v16u8) __msa_fclt_w(temp, s);
        v16u8 one = (v16u8) __msa_fill_w(0x3f800000);
        return __msa_fadd_w(temp, (f32x4) __msa_and_v(mask, one));
    }

    static inline f32x4 fract(f32x4 s)
    {
        return sub(s, floor(s));
    }

    // -----------------------------------------------------------------
    // f64x2
    // -----------------------------------------------------------------

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 v)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        const v4i32 control = (v4i32) { x * 2 + 0, x * 2 + 1, y * 2 + 0, y * 2 + 1 };
        return (v2f64) __msa_vshf_w(control, (v4i32) v, (v4i32) v);
    }

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 a, f64x2 b)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        const v4i32 control = (v4i32) { x * 2 + 0, x * 2 + 1, y * 2 + 4, y * 2 + 5 };
        return (v2f64) __msa_vshf_w(control, (v4i32) a, (v4i32) b);
    }

    // indexed access

    template <unsigned int Index>
    static inline f64x2 set_component(f64x2 a, f64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        s64 temp;
        std::memcpy(&temp, &s, 8);
        return (v2f64) __msa_insert_d((v2i64) a, Index, temp);
    }

    template <unsigned int Index>
    static inline f64 get_component(f64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        u64 temp = __msa_copy_u_d((v2u64) a, Index);
        f64 s;
        std::memcpy(&s, &temp, 8);
        return s;
    }

    static inline f64x2 f64x2_zero()
    {
        return (v2f64) { 0.0, 0.0 };
    }

    static inline f64x2 f64x2_set(f64 s)
    {
        return (v2f64) { s, s };
    }

    static inline f64x2 f64x2_set(f64 x, f64 y)
    {
        return (v2f64) { x, y };
    }

    static inline f64x2 f64x2_uload(const void* source)
    {
        //return v2f64(__msa_ld_d(source, 0));
        return reinterpret_cast<const v2f64 *>(source)[0];
    }

    static inline void f64x2_ustore(void* dest, f64x2 a)
    {
        //__msa_st_d(v2i64(a), dest, 0);
        reinterpret_cast<v2f64 *>(dest)[0] = a;
    }

    static inline f64x2 unpackhi(f64x2 a, f64x2 b)
    {
        return (v2f64) __msa_ilvr_d((v2i64) b, (v2i64) a);
    }

    static inline f64x2 unpacklo(f64x2 a, f64x2 b)
    {
        return (v2f64) __msa_ilvl_d((v2i64) b, (v2i64) a);
    }

    // bitwise

    static inline f64x2 bitwise_nand(f64x2 a, f64x2 b)
    {
        return (v2f64) __msa_and_v((v16u8)a, __msa_nor_v((v16u8)b, (v16u8)b));
    }

    static inline f64x2 bitwise_and(f64x2 a, f64x2 b)
    {
        return (v2f64) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline f64x2 bitwise_or(f64x2 a, f64x2 b)
    {
        return (v2f64) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline f64x2 bitwise_xor(f64x2 a, f64x2 b)
    {
        return (v2f64) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline f64x2 bitwise_not(f64x2 a)
    {
        return (v2f64) __msa_nor_v((v16u8) a, (v16u8) a);
    }

    static inline f64x2 min(f64x2 a, f64x2 b)
    {
        return __msa_fmin_d(a, b);
    }

    static inline f64x2 max(f64x2 a, f64x2 b)
    {
        return __msa_fmax_d(a, b);
    }

    static inline f64x2 abs(f64x2 a)
    {
        return __msa_fmax_a_d(a, a);
    }

    static inline f64x2 neg(f64x2 a)
    {
        return __msa_fsub_d(f64x2_zero(), a);
    }

    static inline f64x2 sign(f64x2 a)
    {
        v16u8 sign_mask = (v16u8) f64x2_set(-0.0);
        v16u8 value_mask = (v16u8) __msa_fcune_d(a, f64x2_zero());
        v16u8 sign_bits = __msa_and_v((v16u8) a, sign_mask);
        v16u8 value_bits = __msa_and_v(value_mask, (v16u8) f64x2_set(1.0));
        return (v2f64) __msa_or_v(value_bits, sign_bits);
    }

    static inline f64x2 add(f64x2 a, f64x2 b)
    {
        return __msa_fadd_d(a, b);
    }

    static inline f64x2 sub(f64x2 a, f64x2 b)
    {
        return __msa_fsub_d(a, b);
    }

    static inline f64x2 mul(f64x2 a, f64x2 b)
    {
        return __msa_fmul_d(a, b);
    }

    static inline f64x2 div(f64x2 a, f64x2 b)
    {
        return __msa_fdiv_d(a, b);
    }

    static inline f64x2 div(f64x2 a, f64 b)
    {
        return __msa_fdiv_d(a, f64x2_set(b));
    }

    static inline f64x2 hadd(f64x2 a, f64x2 b)
    {
        return add(unpacklo(a, b), unpackhi(a, b));
    }

    static inline f64x2 hsub(f64x2 a, f64x2 b)
    {
        return sub(unpacklo(a, b), unpackhi(a, b));
    }

    static inline f64x2 madd(f64x2 a, f64x2 b, f64x2 c)
    {
        // a + b * c
        return __msa_fmadd_d(a, b, c);
    }

    static inline f64x2 msub(f64x2 a, f64x2 b, f64x2 c)
    {
        // b * c - a
        return neg(__msa_fmsub_d(a, b, c));
    }

    static inline f64x2 nmadd(f64x2 a, f64x2 b, f64x2 c)
    {
        // a - b * c
        return __msa_fmsub_d(a, b, c);
    }

    static inline f64x2 nmsub(f64x2 a, f64x2 b, f64x2 c)
    {
        // -(a + b * c)
        return neg(__msa_fmadd_w(a, b, c));
    }

    static inline f64x2 lerp(f64x2 a, f64x2 b, f64x2 s)
    {
        // a * (1.0 - s) + b * s
        // (a - a * s) + (b * s)
        return madd(nmadd(a, a, s), b, s);
    }

#if defined(MANGO_FAST_MATH)

    static inline f64x2 rcp(f64x2 a)
    {
        return __msa_frcp_d(a);
    }

    static inline f64x2 rsqrt(f64x2 a)
    {
        return __msa_frsqrt_d(a);
    }

#else

    static inline f64x2 rcp(f64x2 a)
    {
        auto estimate = __msa_frcp_d(a);
        auto temp = __msa_fmul_d(a, estimate);
        return __msa_fmul_d(estimate, __msa_fsub_d(f64x2_set(2.0), temp));
    }

    static inline f64x2 rsqrt(f64x2 a)
    {
        auto n = __msa_frsqrt_d(a);
        auto e = __msa_fmul_d(__msa_fmul_d(n, n), a);
        n = __msa_fmul_d(f64x2_set(0.5), n);
        e = __msa_fsub_d(f64x2_set(3.0), e);
        return __msa_fmul_d(n, e);
    }

#endif

    static inline f64x2 sqrt(f64x2 a)
    {
        return __msa_fsqrt_d(a);
    }

    static inline f64 dot2(f64x2 a, f64x2 b)
    {
        f64x2 xy = __msa_fmul_d(a, b);
        f64x2 yx = shuffle<1, 0>(xy);
        f64x2 s = __msa_fadd_d(xy, yx);
        return get_component<0>(s);
    }

    // compare

    static inline mask64x2 compare_neq(f64x2 a, f64x2 b)
    {
        return (v2u64) __msa_fcune_d(a, b);
    }

    static inline mask64x2 compare_eq(f64x2 a, f64x2 b)
    {
        return (v2u64) __msa_fceq_d(a, b);
    }

    static inline mask64x2 compare_lt(f64x2 a, f64x2 b)
    {
        return (v2u64) __msa_fclt_d(a, b);
    }

    static inline mask64x2 compare_le(f64x2 a, f64x2 b)
    {
        return (v2u64) __msa_fcle_d(a, b);
    }

    static inline mask64x2 compare_gt(f64x2 a, f64x2 b)
    {
        return (v2u64) __msa_fclt_d(b, a);
    }

    static inline mask64x2 compare_ge(f64x2 a, f64x2 b)
    {
        return (v2u64) __msa_fcle_d(b, a);
    }

    static inline f64x2 select(mask64x2 mask, f64x2 a, f64x2 b)
    {
        return (v2f64) __msa_bsel_v((v16u8) mask, (v16u8) a, (v16u8) b);
    }

    // rounding

    static inline f64x2 round(f64x2 s)
    {
        v2i64 temp = __msa_ftint_s_d(s);
        return __msa_ffint_s_d(temp);
    }

    static inline f64x2 trunc(f64x2 s)
    {
        v2i64 temp = __msa_ftrunc_s_d(s);
        return __msa_ffint_s_d(temp);
    }

    static inline f64x2 floor(f64x2 s)
    {
        f64x2 temp = round(s);
        v16u8 mask = (v16u8) __msa_fclt_d(s, temp);
        v16u8 one = (v16u8) __msa_fill_d(0x3ff0000000000000);
        return __msa_fsub_d(temp, (f64x2) __msa_and_v(mask, one));
    }

    static inline f64x2 ceil(f64x2 s)
    {
        f64x2 temp = round(s);
        v16u8 mask = (v16u8) __msa_fclt_d(temp, s);
        v16u8 one = (v16u8) __msa_fill_d(0x3ff0000000000000);
        return __msa_fadd_d(temp, (f64x2) __msa_and_v(mask, one));
    }

    static inline f64x2 fract(f64x2 s)
    {
        return sub(s, floor(s));
    }

    // -----------------------------------------------------------------
    // masked functions
    // -----------------------------------------------------------------

#define SIMD_ZEROMASK_FLOAT128
#define SIMD_ZEROMASK_DOUBLE128
#define SIMD_MASK_FLOAT128
#define SIMD_MASK_DOUBLE128
#include <mango/simd/common_mask.hpp>
#undef SIMD_ZEROMASK_FLOAT128
#undef SIMD_ZEROMASK_DOUBLE128
#undef SIMD_MASK_FLOAT128
#undef SIMD_MASK_DOUBLE128

} // namespace mango::simd
