/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float32x2
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y>
    static inline float32x2 shuffle(float32x2 v);

    template <>
    inline float32x2 shuffle<0, 0>(float32x2 v)
    {
        // .xx
        return vdup_lane_f32(v, 0);
    }

    template <>
    inline float32x2 shuffle<0, 1>(float32x2 v)
    {
        // .xy
        return v;
    }

    template <>
    inline float32x2 shuffle<1, 0>(float32x2 v)
    {
        // .yx
        return vrev64_f32(v);
    }

    template <>
    inline float32x2 shuffle<1, 1>(float32x2 v)
    {
        // .yy
        return vdup_lane_f32(v, 1);
    }

    // indexed access

    template <unsigned int Index>
    static inline float32x2 set_component(float32x2 a, float s)
    {
        static_assert(Index < 2, "Index out of range.");
        return vset_lane_f32(s, a, Index);
    }

    template <unsigned int Index>
    static inline float get_component(float32x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return vget_lane_f32(a, Index);
    }

    static inline float32x2 float32x2_zero()
    {
        return vdup_n_f32(0.0f);
    }

    static inline float32x2 float32x2_set1(float s)
    {
        return vdup_n_f32(s);
    }

    static inline float32x2 float32x2_set2(float x, float y)
    {
        float32x2_t temp = { x, y };
        return temp;
    }

    static inline float32x2 float32x2_uload(const float* source)
    {
        float32x2_t temp = { source[0], source[1] };
        return temp;
    }

    static inline void float32x2_ustore(float* dest, float32x2 a)
    {
        dest[0] = vget_lane_f32(a, 0);
        dest[1] = vget_lane_f32(a, 1);
    }

    static inline float32x2 unpackhi(float32x2 a, float32x2 b)
    {
        float32x2x2_t temp = vzip_f32(a, b);
        return temp.val[1];
    }

    static inline float32x2 unpacklo(float32x2 a, float32x2 b)
    {
        float32x2x2_t temp = vzip_f32(a, b);
        return temp.val[0];
    }

    // bitwise

    static inline float32x2 bitwise_nand(float32x2 a, float32x2 b)
    {
        return vreinterpret_f32_s32(vbic_s32(vreinterpret_s32_f32(a), vreinterpret_s32_f32(b)));
    }

    static inline float32x2 bitwise_and(float32x2 a, float32x2 b)
    {
        return vreinterpret_f32_s32(vand_s32(vreinterpret_s32_f32(a), vreinterpret_s32_f32(b)));
    }

    static inline float32x2 bitwise_or(float32x2 a, float32x2 b)
    {
        return vreinterpret_f32_s32(vorr_s32(vreinterpret_s32_f32(a), vreinterpret_s32_f32(b)));
    }

    static inline float32x2 bitwise_xor(float32x2 a, float32x2 b)
    {
        return vreinterpret_f32_s32(veor_s32(vreinterpret_s32_f32(a), vreinterpret_s32_f32(b)));
    }

    static inline float32x2 bitwise_not(float32x2 a)
    {
        return vreinterpret_f32_u32(veor_u32(vreinterpret_u32_f32(a), vceq_f32(a, a)));
    }

    static inline float32x2 min(float32x2 a, float32x2 b)
    {
        return vmin_f32(a, b);
    }

    static inline float32x2 max(float32x2 a, float32x2 b)
    {
        return vmax_f32(a, b);
    }

    static inline float32x2 abs(float32x2 a)
    {
        return vabs_f32(a);
    }

    static inline float32x2 neg(float32x2 a)
    {
        return vneg_f32(a);
    }

    static inline float32x2 add(float32x2 a, float32x2 b)
    {
        return vadd_f32(a, b);
    }

    static inline float32x2 sub(float32x2 a, float32x2 b)
    {
        return vsub_f32(a, b);
    }

    static inline float32x2 mul(float32x2 a, float32x2 b)
    {
        return vmul_f32(a, b);
    }

#ifdef __aarch64__

    static inline float32x2 div(float32x2 a, float32x2 b)
    {
        return vdiv_f32(a, b);
    }

    static inline float32x2 div(float32x2 a, float b)
    {
        float32x2 s = vdup_n_f32(b);
        return vdiv_f32(a, s);
    }

#else

    static inline float32x2 div(float32x2 a, float32x2 b)
    {
        float32x2 n = vrecpe_f32(b);
        n = vmul_f32(vrecps_f32(n, b), n);
        n = vmul_f32(vrecps_f32(n, b), n);
        return vmul_f32(a, n);
    }

    static inline float32x2 div(float32x2 a, float b)
    {
        float32x2 s = vdup_n_f32(b);
        float32x2 n = vrecpe_f32(s);
        n = vmul_f32(vrecps_f32(n, s), n);
        n = vmul_f32(vrecps_f32(n, s), n);
        return vmul_f32(a, n);
    }

#endif

    static inline float32x2 madd(float32x2 a, float32x2 b, float32x2 c)
    {
        return vmla_f32(a, b, c);
    }

    static inline float32x2 msub(float32x2 a, float32x2 b, float32x2 c)
    {
        return vmls_f32(a, b, c);
    }

    static inline float32x2 fast_reciprocal(float32x2 a)
    {
        float32x2_t n = vrecpe_f32(a);
        n = vmul_f32(vrecps_f32(n, a), n);
        return n;
    }

    static inline float32x2 fast_rsqrt(float32x2 a)
    {
        float32x2_t n = vrsqrte_f32(a);
        n = vmul_f32(n, vrsqrts_f32(vmul_f32(n, a), n));
        return n;
    }

    static inline float32x2 fast_sqrt(float32x2 a)
    {
        float32x2_t n = vrsqrte_f32(a);
        n = vmul_f32(n, vrsqrts_f32(vmul_f32(n, a), n));
        return vmul_f32(a, n);
    }

    static inline float32x2 reciprocal(float32x2 a)
    {
        float32x2_t n = vrecpe_f32(a);
        n = vmul_f32(vrecps_f32(n, a), n);
        n = vmul_f32(vrecps_f32(n, a), n);
        return n;
    }

    static inline float32x2 rsqrt(float32x2 a)
    {
        float32x2_t n = vrsqrte_f32(a);
        n = vmul_f32(n, vrsqrts_f32(vmul_f32(n, a), n));
        n = vmul_f32(n, vrsqrts_f32(vmul_f32(n, a), n));
        n = vmul_f32(n, vrsqrts_f32(vmul_f32(n, a), n));
        return n;
    }

    static inline float32x2 sqrt(float32x2 a)
    {
        float32x2_t n = vrsqrte_f32(a);
        n = vmul_f32(n, vrsqrts_f32(vmul_f32(n, a), n));
        n = vmul_f32(n, vrsqrts_f32(vmul_f32(n, a), n));
        n = vmul_f32(n, vrsqrts_f32(vmul_f32(n, a), n));
        return vmul_f32(a, n);
    }

    static inline float32x2 dot2(float32x2 a, float32x2 b)
    {
        const float32x2_t xy = vmul_f32(a, b);
        return vpadd_f32(xy, xy);
    }

    // compare

    static inline float32x2::mask compare_neq(float32x2 a, float32x2 b)
    {
        return vmvn_u32(vceq_f32(a, b));
    }

    static inline float32x2::mask compare_eq(float32x2 a, float32x2 b)
    {
        return vceq_f32(a, b);
    }

    static inline float32x2::mask compare_lt(float32x2 a, float32x2 b)
    {
        return vclt_f32(a, b);
    }

    static inline float32x2::mask compare_le(float32x2 a, float32x2 b)
    {
        return vcle_f32(a, b);
    }

    static inline float32x2::mask compare_gt(float32x2 a, float32x2 b)
    {
        return vcgt_f32(a, b);
    }

    static inline float32x2::mask compare_ge(float32x2 a, float32x2 b)
    {
        return vcge_f32(a, b);
    }

    static inline float32x2 select(float32x2::mask mask, float32x2 a, float32x2 b)
    {
        return vbsl_f32(mask, a, b);
    }

    // rounding

#if __ARM_ARCH >= 8 && !defined(MANGO_COMPILER_CLANG)

    // Disabled with clang until supported in NDK (tested last with r14b)

    static inline float32x2 round(float32x2 s)
    {
        return vrnda_f32(s);
    }

    static inline float32x2 trunc(float32x2 s)
    {
        return vrnd_f32(s);
    }

    static inline float32x2 floor(float32x2 s)
    {
        return vrndm_f32(s);
    }

    static inline float32x2 ceil(float32x2 s)
    {
        return vrndp_f32(s);
    }

#else

    static inline float32x2 round(float32x2 s)
    {
        float32x2_t magic = vdup_n_f32(12582912.0f); // 1.5 * (1 << 23)
        float32x2_t result = vsub_f32(vadd_f32(s, magic), magic);
        uint32x2_t mask = vcle_f32(vabs_f32(s), vreinterpret_f32_u32(vdup_n_u32(0x4b000000)));
        return vbsl_f32(mask, result, s);
    }

    static inline float32x2 trunc(float32x2 s)
    {
        int32x2_t truncated = vcvt_s32_f32(s);
        float32x2_t result = vcvt_f32_s32(truncated);
        uint32x2_t mask = vcle_f32(vabs_f32(s), vreinterpret_f32_u32(vdup_n_u32(0x4b000000)));
        return vbsl_f32(mask, result, s);
    }

    static inline float32x2 floor(float32x2 s)
    {
        const float32x2_t temp = round(s);
        const uint32x2_t mask = vclt_f32(s, temp);
        const uint32x2_t one = vdup_n_u32(0x3f800000);
        return vsub_f32(temp, vreinterpret_f32_u32(vand_u32(mask, one)));
    }

    static inline float32x2 ceil(float32x2 s)
    {
        const float32x2_t temp = round(s);
        const uint32x2_t mask = vcgt_f32(s, temp);
        const uint32x2_t one = vdup_n_u32(0x3f800000);
        return vadd_f32(temp, vreinterpret_f32_u32(vand_u32(mask, one)));
    }

#endif

    static inline float32x2 fract(float32x2 s)
    {
        return vsub_f32(s, floor(s));
    }

} // namespace simd
} // namespace mango
