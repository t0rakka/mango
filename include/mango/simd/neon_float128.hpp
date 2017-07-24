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

#ifdef MANGO_COMPILER_CLANG

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    inline float32x4 shuffle(float32x4 a, float32x4 b)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return __builtin_shufflevector(a.data, b.data, x, y, z, w);
    }

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    inline float32x4 shuffle(float32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return __builtin_shufflevector(v.data, v.data, x, y, z, w);
    }

#else

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    inline float32x4 shuffle(float32x4 a, float32x4 b)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        float32x4_t va = a;
        float32x4_t vb = b;
        return (float32x4_t) { va[x], va[y], vb[z], vb[w] };
    }

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    inline float32x4 shuffle(float32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
#if __GNUC__ >= 5
        return (float32x4_t) __builtin_shuffle(v.data, (uint32x4_t) {x, y, z, w});
#else
        float32x4_t temp = v;
        return (float32x4_t) { temp[x], temp[y], temp[z], temp[w] };
#endif
    }

#endif

    //
    // gcc 5 __builtin_shuffle(reg, low.x, low.y, high.z, high.w)
    // legend: generated instruction (-- means generic tbl load)
    // x-axis: low, y-axis: high
    //
    //     xx   xy   xz   xw   yx   yy   yz   yw   zx   zy   zz   zw   wx   wy   wz   ww
    // xx  dup  -    -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // xy  -    -    -    -    -    -    -    -    -    -    -    ext  -    -    -    -
    // xz  -    -    uzp1 -    -    -    -    -    -    -    -    -    -    -    -    -
    // xw  -    -    -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // yx  -    -    -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // yy  zip1 -    -    -    -    dup  -    -    -    -    -    -    -    -    -    -
    // yz  -    -    -    -    -    -    -    -    -    -    -    -    ext  -    -    -
    // yw  -    -    -    -    -    -    -    uzp2 -    -    -    -    -    -    -    -
    // zx  -    -    -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // zy  -    -    -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // zz  trn1 -    -    -    -    -    -    -    -    -    dup  -    -    -    -    -
    // zw  -    mov  -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // wx  -    -    -    -    -    -    ext  -    -    -    -    -    -    -    -    -
    // wy  -    -    -    -    -    -    -    -    -    -    -    -    -    -    -    -
    // wz  -    -    -    -    rev64-    -    -    -    -    -    -    -    -    -    -
    // ww  -    -    -    -    -    trn2 -    -    -    -    zip2 -    -    -    -    dup

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
        const float32x2_t xy = vget_low_f32(v);
        return vdupq_lane_f32(xy, 0);
    }

    template <>
    inline float32x4 shuffle<1, 1, 1, 1>(float32x4 v)
    {
        // .yyyy
        const float32x2_t xy = vget_low_f32(v);
        return vdupq_lane_f32(xy, 1);
    }

    template <>
    inline float32x4 shuffle<2, 2, 2, 2>(float32x4 v)
    {
        // .zzzz
        const float32x2_t zw = vget_high_f32(v);
        return vdupq_lane_f32(zw, 0);
    }

    template <>
    inline float32x4 shuffle<3, 3, 3, 3>(float32x4 v)
    {
        // .wwww
        const float32x2_t zw = vget_high_f32(v);
        return vdupq_lane_f32(zw, 1);
    }

    template <>
    inline float32x4 shuffle<1, 1, 0, 0>(float32x4 v)
    {
        // .yyxx
	    return vcombine_f32(vdup_n_f32(vgetq_lane_f32(v, 1)), vdup_n_f32(vgetq_lane_f32(v, 0)));
    }

    template <>
    inline float32x4 shuffle<2, 2, 0, 0>(float32x4 v)
    {
        // .zzxx
	    return vcombine_f32(vdup_n_f32(vgetq_lane_f32(v, 2)), vdup_n_f32(vgetq_lane_f32(v, 0)));
    }

    template <>
    inline float32x4 shuffle<3, 3, 1, 1>(float32x4 v)
    {
        // .wwyy
	    return vcombine_f32(vdup_n_f32(vgetq_lane_f32(v, 3)), vdup_n_f32(vgetq_lane_f32(v, 1)));
    }

    // indexed access

    template <unsigned int Index>
    static inline float32x4 set_component(float32x4 a, float s)
    {
        static_assert(Index < 4, "Index out of range.");
        return vsetq_lane_f32(s, a, Index);
    }

    template <unsigned int Index>
    static inline float get_component(float32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return vgetq_lane_f32(a, Index);
    }

    static inline float32x4 float32x4_zero()
    {
        return vdupq_n_f32(0.0f);
    }

    static inline float32x4 float32x4_set1(float s)
    {
        return vdupq_n_f32(s);
    }

    static inline float32x4 float32x4_set4(float x, float y, float z, float w)
    {
        float32x4_t temp = { x, y, z, w };
        return temp;
    }

    static inline float32x4 float32x4_uload(const float* source)
    {
        float32x4_t temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline void float32x4_ustore(float* dest, float32x4 a)
    {
        dest[0] = vgetq_lane_f32(a, 0);
        dest[1] = vgetq_lane_f32(a, 1);
        dest[2] = vgetq_lane_f32(a, 2);
        dest[3] = vgetq_lane_f32(a, 3);
    }

    static inline float32x4 movelh(float32x4 a, float32x4 b)
    {
        return vcombine_f32(vget_low_f32(a), vget_low_f32(b));
    }

    static inline float32x4 movehl(float32x4 a, float32x4 b)
    {
        return vcombine_f32(vget_high_f32(b), vget_high_f32(a));
    }

    static inline float32x4 unpackhi(float32x4 a, float32x4 b)
    {
        float32x4x2_t v = vzipq_f32(a, b);
        return v.val[1];
    }

    static inline float32x4 unpacklo(float32x4 a, float32x4 b)
    {
        float32x4x2_t v = vzipq_f32(a, b);
        return v.val[0];
    }

    // bitwise

    static inline float32x4 bitwise_nand(float32x4 a, float32x4 b)
    {
        return vreinterpretq_f32_s32(vbicq_s32(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)));
    }

    static inline float32x4 bitwise_and(float32x4 a, float32x4 b)
    {
        return vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)));
    }

    static inline float32x4 bitwise_or(float32x4 a, float32x4 b)
    {
        return vreinterpretq_f32_s32(vorrq_s32(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)));
    }

    static inline float32x4 bitwise_xor(float32x4 a, float32x4 b)
    {
        return vreinterpretq_f32_s32(veorq_s32(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)));
    }

    static inline float32x4 bitwise_not(float32x4 a)
    {
        return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a), vceqq_f32(a, a)));
    }

    static inline float32x4 min(float32x4 a, float32x4 b)
    {
        return vminq_f32(a, b);
    }

    static inline float32x4 max(float32x4 a, float32x4 b)
    {
        return vmaxq_f32(a, b);
    }

    static inline float32x4 hmin(float32x4 a)
    {
        float32x2_t s = vpmin_f32(vget_low_f32(a), vget_high_f32(a));
        s = vpmin_f32(s, s);
        return vcombine_f32(s, s);
    }

    static inline float32x4 hmax(float32x4 a)
    {
        float32x2_t s = vpmax_f32(vget_low_f32(a), vget_high_f32(a));
        s = vpmax_f32(s, s);
        return vcombine_f32(s, s);
    }

    static inline float32x4 abs(float32x4 a)
    {
        return vabsq_f32(a);
    }

    static inline float32x4 neg(float32x4 a)
    {
        return vnegq_f32(a);
    }

    static inline float32x4 add(float32x4 a, float32x4 b)
    {
        return vaddq_f32(a, b);
    }

    static inline float32x4 sub(float32x4 a, float32x4 b)
    {
        return vsubq_f32(a, b);
    }

    static inline float32x4 mul(float32x4 a, float32x4 b)
    {
        return vmulq_f32(a, b);
    }

#ifdef __aarch64__

    static inline float32x4 div(float32x4 a, float32x4 b)
    {
        return vdivq_f32(a, b);
    }

    static inline float32x4 div(float32x4 a, float b)
    {
        float32x4 s = vdupq_n_f32(b);
        return vdivq_f32(a, s);
    }

#else

    static inline float32x4 div(float32x4 a, float32x4 b)
    {
        float32x4 n = vrecpeq_f32(b);
        n = vmulq_f32(vrecpsq_f32(n, b), n);
        n = vmulq_f32(vrecpsq_f32(n, b), n);
        return vmulq_f32(a, n);
    }

    static inline float32x4 div(float32x4 a, float b)
    {
        float32x4 s = vdupq_n_f32(b);
        float32x4 n = vrecpeq_f32(s);
        n = vmulq_f32(vrecpsq_f32(n, s), n);
        n = vmulq_f32(vrecpsq_f32(n, s), n);
        return vmulq_f32(a, n);
    }

#endif

    static inline float32x4 hadd(float32x4 a, float32x4 b)
    {
        return vcombine_f32(vpadd_f32(vget_low_f32(a), vget_high_f32(a)), 
	                        vpadd_f32(vget_low_f32(b), vget_high_f32(b)));
    }

    static inline float32x4 madd(float32x4 a, float32x4 b, float32x4 c)
    {
        return vmlaq_f32(a, b, c);
    }

    static inline float32x4 msub(float32x4 a, float32x4 b, float32x4 c)
    {
        return vmlsq_f32(a, b, c);
    }

    static inline float32x4 fast_reciprocal(float32x4 a)
    {
        float32x4 n = vrecpeq_f32(a);
        n = vmulq_f32(vrecpsq_f32(n, a), n);
        return n;
    }

    static inline float32x4 fast_rsqrt(float32x4 a)
    {
        float32x4 n = vrsqrteq_f32(a);
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        return n;
    }

    static inline float32x4 fast_sqrt(float32x4 a)
    {
        float32x4 n = vrsqrteq_f32(a);
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        return vmulq_f32(a, n);
    }

    static inline float32x4 reciprocal(float32x4 a)
    {
        float32x4 n = vrecpeq_f32(a);
        n = vmulq_f32(vrecpsq_f32(n, a), n);
        n = vmulq_f32(vrecpsq_f32(n, a), n);
        return n;
    }

    static inline float32x4 rsqrt(float32x4 a)
    {
        float32x4 n = vrsqrteq_f32(a);
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        return n;
    }

    static inline float32x4 sqrt(float32x4 a)
    {
        float32x4 n = vrsqrteq_f32(a);
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        return vmulq_f32(a, n);
    }

    static inline float32x4 dot3(float32x4 a, float32x4 b)
    {
        const float32x4 s = vmulq_f32(a, b);
        const float32x2_t xy = vget_low_f32(s);
        const float32x2_t zw = vget_high_f32(s);
        return vdupq_lane_f32(vadd_f32(vpadd_f32(xy, xy), zw), 0);
    }

    static inline float32x4 dot4(float32x4 a, float32x4 b)
    {
        float32x4 m = vmulq_f32(a, b);
        float32x2_t s = vpadd_f32(vget_low_f32(m), vget_high_f32(m));
        s = vpadd_f32(s, s);
        return vdupq_lane_f32(s, 0);
    }

    static inline float32x4 cross3(float32x4 a, float32x4 b)
    {
        float32x4 c = vmulq_f32(a, shuffle<1, 2, 0, 3>(b));
        c = vmlsq_f32(c, b, shuffle<1, 2, 0, 3>(a));
        return shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline float32x4::mask compare_neq(float32x4 a, float32x4 b)
    {
        return vmvnq_u32(vceqq_f32(a, b));
    }

    static inline float32x4::mask compare_eq(float32x4 a, float32x4 b)
    {
        return vceqq_f32(a, b);
    }

    static inline float32x4::mask compare_lt(float32x4 a, float32x4 b)
    {
        return vcltq_f32(a, b);
    }

    static inline float32x4::mask compare_le(float32x4 a, float32x4 b)
    {
        return vcleq_f32(a, b);
    }

    static inline float32x4::mask compare_gt(float32x4 a, float32x4 b)
    {
        return vcgtq_f32(a, b);
    }

    static inline float32x4::mask compare_ge(float32x4 a, float32x4 b)
    {
        return vcgeq_f32(a, b);
    }

    static inline float32x4 select(float32x4::mask mask, float32x4 a, float32x4 b)
    {
        return vbslq_f32(mask, a, b);
    }

    static inline uint32 get_mask(float32x4::mask a)
    {
        const uint32x4_t mask = { 1, 2, 4, 8 };
        const uint32x4_t masked = vandq_u32(a, mask);
        const uint32x2_t high = vget_high_u32(masked);
        const uint32x2_t low = vget_low_u32(masked);
        const uint32x2_t d0 = vorr_u32(high, low);
        const uint32x2_t d1 = vpadd_u32(d0, d0);
        return vget_lane_u32(d1, 0);
    }

    // rounding

#if __ARM_ARCH >= 8 && !defined(MANGO_COMPILER_CLANG)

    // Disabled with clang until supported in NDK (tested last with r14b)

    static inline float32x4 round(float32x4 s)
    {
        return vrndqa_f32(s);
    }

    static inline float32x4 trunc(float32x4 s)
    {
        return vrndq_f32(s);
    }

    static inline float32x4 floor(float32x4 s)
    {
        return vrndqm_f32(s);
    }

    static inline float32x4 ceil(float32x4 s)
    {
        return vrndqp_f32(s);
    }

#else

    static inline float32x4 round(float32x4 s)
    {
        float32x4_t magic = vdupq_n_f32(12582912.0f); // 1.5 * (1 << 23)
        float32x4_t result = vsubq_f32(vaddq_f32(s, magic), magic);
        uint32x4_t mask = vcleq_f32(vabsq_f32(s), vreinterpretq_f32_u32(vdupq_n_u32(0x4b000000)));
        return vbslq_f32(mask, result, s);
    }

    static inline float32x4 trunc(float32x4 s)
    {
        int32x4_t truncated = vcvtq_s32_f32(s);
        float32x4_t result = vcvtq_f32_s32(truncated);
        uint32x4_t mask = vcleq_f32(vabsq_f32(s), vreinterpretq_f32_u32(vdupq_n_u32(0x4b000000)));
        return vbslq_f32(mask, result, s);
    }

    static inline float32x4 floor(float32x4 s)
    {
        const float32x4 temp = round(s);
        const uint32x4_t mask = vcltq_f32(s, temp);
        const uint32x4_t one = vdupq_n_u32(0x3f800000);
        return vsubq_f32(temp, vreinterpretq_f32_u32(vandq_u32(mask, one)));
    }

    static inline float32x4 ceil(float32x4 s)
    {
        const float32x4 temp = round(s);
        const uint32x4_t mask = vcgtq_f32(s, temp);
        const uint32x4_t one = vdupq_n_u32(0x3f800000);
        return vaddq_f32(temp, vreinterpretq_f32_u32(vandq_u32(mask, one)));
    }

#endif

    static inline float32x4 fract(float32x4 s)
    {
        return sub(s, floor(s));
    }

} // namespace simd
} // namespace mango
