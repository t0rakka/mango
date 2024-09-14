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

#ifdef MANGO_COMPILER_CLANG

    template <u32 x, u32 y, u32 z, u32 w>
    inline f32x4 shuffle(f32x4 a, f32x4 b)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return __builtin_shufflevector(a.data, b.data, x, y, z + 4, w + 4);
    }

    template <u32 x, u32 y, u32 z, u32 w>
    inline f32x4 shuffle(f32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return __builtin_shufflevector(v.data, v.data, x, y, z + 4, w + 4);
    }

#else

    template <u32 x, u32 y, u32 z, u32 w>
    inline f32x4 shuffle(f32x4 a, f32x4 b)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
#if 1
        float32x4_t result;
        result = vmovq_n_f32(vgetq_lane_f32(a, x));
        result = vsetq_lane_f32(vgetq_lane_f32(a, y), result, 1);
        result = vsetq_lane_f32(vgetq_lane_f32(b, z), result, 2);
        result = vsetq_lane_f32(vgetq_lane_f32(b, w), result, 3);
        return result;
#else
        // warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
        return (float32x4_t) { a.data[x], a.data[y], b.data[z], b.data[w] };
#endif
    }

    template <u32 x, u32 y, u32 z, u32 w>
    inline f32x4 shuffle(f32x4 v)
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
    inline f32x4 shuffle<0, 1, 2, 3>(f32x4 v)
    {
        // .xyzw
        return v;
    }

    template <>
    inline f32x4 shuffle<0, 0, 0, 0>(f32x4 v)
    {
        // .xxxx
        const float32x2_t xy = vget_low_f32(v);
        return vdupq_lane_f32(xy, 0);
    }

    template <>
    inline f32x4 shuffle<1, 1, 1, 1>(f32x4 v)
    {
        // .yyyy
        const float32x2_t xy = vget_low_f32(v);
        return vdupq_lane_f32(xy, 1);
    }

    template <>
    inline f32x4 shuffle<2, 2, 2, 2>(f32x4 v)
    {
        // .zzzz
        const float32x2_t zw = vget_high_f32(v);
        return vdupq_lane_f32(zw, 0);
    }

    template <>
    inline f32x4 shuffle<3, 3, 3, 3>(f32x4 v)
    {
        // .wwww
        const float32x2_t zw = vget_high_f32(v);
        return vdupq_lane_f32(zw, 1);
    }

    template <>
    inline f32x4 shuffle<1, 1, 0, 0>(f32x4 v)
    {
        // .yyxx
        return vcombine_f32(vdup_n_f32(vgetq_lane_f32(v, 1)), vdup_n_f32(vgetq_lane_f32(v, 0)));
    }

    template <>
    inline f32x4 shuffle<2, 2, 0, 0>(f32x4 v)
    {
        // .zzxx
        return vcombine_f32(vdup_n_f32(vgetq_lane_f32(v, 2)), vdup_n_f32(vgetq_lane_f32(v, 0)));
    }

    template <>
    inline f32x4 shuffle<3, 3, 1, 1>(f32x4 v)
    {
        // .wwyy
        return vcombine_f32(vdup_n_f32(vgetq_lane_f32(v, 3)), vdup_n_f32(vgetq_lane_f32(v, 1)));
    }

    // indexed access

    template <unsigned int Index>
    static inline f32x4 set_component(f32x4 a, f32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return vsetq_lane_f32(s, a, Index);
    }

    template <unsigned int Index>
    static inline f32 get_component(f32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return vgetq_lane_f32(a, Index);
    }

    static inline f32x4 f32x4_zero()
    {
        return vdupq_n_f32(0.0f);
    }

    static inline f32x4 f32x4_set(f32 s)
    {
        return vdupq_n_f32(s);
    }

    static inline f32x4 f32x4_set(f32 x, f32 y, f32 z, f32 w)
    {
        float32x4_t temp = { x, y, z, w };
        return temp;
    }

#if defined(MANGO_COMPILER_GCC)

    static inline f32x4 f32x4_uload(const void* source)
    {
        f32x4 temp;
        std::memcpy(&temp, source, sizeof(temp));
        return temp;
    }

    static inline void f32x4_ustore(void* dest, f32x4 a)
    {
        std::memcpy(dest, &a, sizeof(a));
    }

#else

    static inline f32x4 f32x4_uload(const void* source)
    {
        return vld1q_f32(reinterpret_cast<const f32*>(source));
    }

    static inline void f32x4_ustore(void* dest, f32x4 a)
    {
        vst1q_f32(reinterpret_cast<f32*>(dest), a);
    }

#endif

    static inline f32x4 movelh(f32x4 a, f32x4 b)
    {
        return vcombine_f32(vget_low_f32(a), vget_low_f32(b));
    }

    static inline f32x4 movehl(f32x4 a, f32x4 b)
    {
        return vcombine_f32(vget_high_f32(b), vget_high_f32(a));
    }

    static inline f32x4 unpackhi(f32x4 a, f32x4 b)
    {
        float32x4x2_t v = vzipq_f32(a, b);
        return v.val[1];
    }

    static inline f32x4 unpacklo(f32x4 a, f32x4 b)
    {
        float32x4x2_t v = vzipq_f32(a, b);
        return v.val[0];
    }

    // bitwise

    static inline f32x4 bitwise_nand(f32x4 a, f32x4 b)
    {
        return vreinterpretq_f32_s32(vbicq_s32(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)));
    }

    static inline f32x4 bitwise_and(f32x4 a, f32x4 b)
    {
        return vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)));
    }

    static inline f32x4 bitwise_or(f32x4 a, f32x4 b)
    {
        return vreinterpretq_f32_s32(vorrq_s32(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)));
    }

    static inline f32x4 bitwise_xor(f32x4 a, f32x4 b)
    {
        return vreinterpretq_f32_s32(veorq_s32(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)));
    }

    static inline f32x4 bitwise_not(f32x4 a)
    {
        return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a), vceqq_f32(a, a)));
    }

    static inline f32x4 min(f32x4 a, f32x4 b)
    {
        return vminq_f32(a, b);
    }

    static inline f32x4 max(f32x4 a, f32x4 b)
    {
        return vmaxq_f32(a, b);
    }

#ifdef __aarch64__

    static inline f32x4 hmin(f32x4 a)
    {
        a = vpminq_f32(a, a);
        a = vpminq_f32(a, a);
        return a;
    }

    static inline f32x4 hmax(f32x4 a)
    {
        a = vpmaxq_f32(a, a);
        a = vpmaxq_f32(a, a);
        return a;
    }

#else

    static inline f32x4 hmin(f32x4 a)
    {
        float32x2_t s = vpmin_f32(vget_low_f32(a), vget_high_f32(a));
        s = vpmin_f32(s, s);
        return vcombine_f32(s, s);
    }

    static inline f32x4 hmax(f32x4 a)
    {
        float32x2_t s = vpmax_f32(vget_low_f32(a), vget_high_f32(a));
        s = vpmax_f32(s, s);
        return vcombine_f32(s, s);
    }

#endif

    static inline f32x4 abs(f32x4 a)
    {
        return vabsq_f32(a);
    }

    static inline f32x4 neg(f32x4 a)
    {
        return vnegq_f32(a);
    }

    static inline f32x4 sign(f32x4 a)
    {
        auto i = vreinterpretq_u32_f32(a);
        auto value_mask = vmvnq_u32(vceqq_u32(i, vdupq_n_u32(0)));
        auto value_bits = vandq_u32(vdupq_n_u32(0x3f800000), value_mask);
        auto sign_bits = vandq_u32(i, vdupq_n_u32(0x80000000));
        return vreinterpretq_f32_u32(vorrq_u32(value_bits, sign_bits));
    }

    static inline f32x4 add(f32x4 a, f32x4 b)
    {
        return vaddq_f32(a, b);
    }

    static inline f32x4 sub(f32x4 a, f32x4 b)
    {
        return vsubq_f32(a, b);
    }

    static inline f32x4 mul(f32x4 a, f32x4 b)
    {
        return vmulq_f32(a, b);
    }

#ifdef __aarch64__

    static inline f32x4 div(f32x4 a, f32x4 b)
    {
        return vdivq_f32(a, b);
    }

    static inline f32x4 div(f32x4 a, f32 b)
    {
        f32x4 s = vdupq_n_f32(b);
        return vdivq_f32(a, s);
    }

    static inline f32x4 hadd(f32x4 a, f32x4 b)
    {
        return vpaddq_f32(a, b);
    }

    static inline f32x4 hsub(f32x4 a, f32x4 b)
    {
        b = vnegq_f32(b);
        return vpaddq_f32(a, b);
    }

#else

    static inline f32x4 div(f32x4 a, f32x4 b)
    {
        f32x4 n = vrecpeq_f32(b);
        n = vmulq_f32(vrecpsq_f32(n, b), n);
        n = vmulq_f32(vrecpsq_f32(n, b), n);
        return vmulq_f32(a, n);
    }

    static inline f32x4 div(f32x4 a, f32 b)
    {
        f32x4 s = vdupq_n_f32(b);
        f32x4 n = vrecpeq_f32(s);
        n = vmulq_f32(vrecpsq_f32(n, s), n);
        n = vmulq_f32(vrecpsq_f32(n, s), n);
        return vmulq_f32(a, n);
    }

    static inline f32x4 hadd(f32x4 a, f32x4 b)
    {
        return vcombine_f32(vpadd_f32(vget_low_f32(a), vget_high_f32(a)),
                            vpadd_f32(vget_low_f32(b), vget_high_f32(b)));
    }

    static inline f32x4 hsub(f32x4 a, f32x4 b)
    {
        b = vnegq_f32(b);
        return vcombine_f32(vpadd_f32(vget_low_f32(a), vget_high_f32(a)),
                            vpadd_f32(vget_low_f32(b), vget_high_f32(b)));
    }

#endif

    static inline f32x4 madd(f32x4 a, f32x4 b, f32x4 c)
    {
        // a + b * c
        return vmlaq_f32(a, b, c);
    }

    static inline f32x4 msub(f32x4 a, f32x4 b, f32x4 c)
    {
        // b * c - a
        return vnegq_f32(vmlsq_f32(a, b, c));
    }

    static inline f32x4 nmadd(f32x4 a, f32x4 b, f32x4 c)
    {
        // a - b * c
        return vmlsq_f32(a, b, c);
    }

    static inline f32x4 nmsub(f32x4 a, f32x4 b, f32x4 c)
    {
        // -(a + b * c)
        return vnegq_f32(vmlaq_f32(a, b, c));
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
        f32x4 e = vrecpeq_f32(a);
        e = vmulq_f32(vrecpsq_f32(a, e), e);
        return e;
    }

    static inline f32x4 rsqrt(f32x4 a)
    {
        f32x4 e = vrsqrteq_f32(a);
        e = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a, e), e), e);
        return e;
    }

#ifdef __aarch64__

    static inline f32x4 sqrt(f32x4 a)
    {
        return vsqrtq_f32(a);
    }

#else

    static inline f32x4 sqrt(f32x4 a)
    {
        f32x4 e = vrsqrteq_f32(a);
        e = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a, e), e), e);
        return vmulq_f32(a, e);
    }

#endif

#else // MANGO_FAST_MATH

    static inline f32x4 rcp(f32x4 a)
    {
        f32x4 e = vrecpeq_f32(a);
        e = vmulq_f32(vrecpsq_f32(a, e), e);
        e = vmulq_f32(vrecpsq_f32(a, e), e);
        return e;
    }

    static inline f32x4 rsqrt(f32x4 a)
    {
        f32x4 e = vrsqrteq_f32(a);
        e = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a, e), e), e);
        e = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a, e), e), e);
        return e;
    }

#ifdef __aarch64__
    
    static inline f32x4 sqrt(f32x4 a)
    {
        return vsqrtq_f32(a);
    }

#else

    static inline f32x4 sqrt(f32x4 a)
    {
        f32x4 e = vrsqrteq_f32(a);
        e = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a, e), e), e);
        e = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a, e), e), e);
        return vmulq_f32(a, e);
    }

#endif

#endif // MANGO_FAST_MATH

    static inline f32 dot3(f32x4 a, f32x4 b)
    {
        const float32x4_t prod = vmulq_f32(a, b);
        const float32x2_t xy = vget_low_f32(prod);
        const float32x2_t zw = vget_high_f32(prod);
        const float32x2_t s = vadd_f32(vpadd_f32(xy, xy), zw);
        return vget_lane_f32(s, 0);
    }

#ifdef __aarch64__

    static inline f32 dot4(f32x4 a, f32x4 b)
    {
        float32x4_t prod = vmulq_f32(a, b);
        return vaddvq_f32(prod);
    }

#else

    static inline f32 dot4(f32x4 a, f32x4 b)
    {
        const float32x4_t prod = vmulq_f32(a, b);
        float32x2_t s = vpadd_f32(vget_low_f32(prod), vget_high_f32(prod));
        return vget_lane_f32(vpadd_f32(s, s), 0);
    }

#endif

    static inline f32x4 cross3(f32x4 a, f32x4 b)
    {
        f32x4 c = vmulq_f32(a, shuffle<1, 2, 0, 3>(b));
        c = vmlsq_f32(c, b, shuffle<1, 2, 0, 3>(a));
        return shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline mask32x4 compare_neq(f32x4 a, f32x4 b)
    {
        return vmvnq_u32(vceqq_f32(a, b));
    }

    static inline mask32x4 compare_eq(f32x4 a, f32x4 b)
    {
        return vceqq_f32(a, b);
    }

    static inline mask32x4 compare_lt(f32x4 a, f32x4 b)
    {
        return vcltq_f32(a, b);
    }

    static inline mask32x4 compare_le(f32x4 a, f32x4 b)
    {
        return vcleq_f32(a, b);
    }

    static inline mask32x4 compare_gt(f32x4 a, f32x4 b)
    {
        return vcgtq_f32(a, b);
    }

    static inline mask32x4 compare_ge(f32x4 a, f32x4 b)
    {
        return vcgeq_f32(a, b);
    }

    static inline f32x4 select(mask32x4 mask, f32x4 a, f32x4 b)
    {
        return vbslq_f32(mask, a, b);
    }

    // rounding

#if __ARM_ARCH >= 8

    static inline f32x4 round(f32x4 s)
    {
        return vrndaq_f32(s);
    }

    static inline f32x4 trunc(f32x4 s)
    {
        return vrndq_f32(s);
    }

    static inline f32x4 floor(f32x4 s)
    {
        return vrndmq_f32(s);
    }

    static inline f32x4 ceil(f32x4 s)
    {
        return vrndpq_f32(s);
    }

#else

    static inline f32x4 round(f32x4 s)
    {
        float32x4_t magic = vdupq_n_f32(12582912.0f); // 1.5 * (1 << 23)
        float32x4_t result = vsubq_f32(vaddq_f32(s, magic), magic);
        uint32x4_t mask = vcleq_f32(vabsq_f32(s), vreinterpretq_f32_u32(vdupq_n_u32(0x4b000000)));
        return vbslq_f32(mask, result, s);
    }

    static inline f32x4 trunc(f32x4 s)
    {
        int32x4_t truncated = vcvtq_s32_f32(s);
        float32x4_t result = vcvtq_f32_s32(truncated);
        uint32x4_t mask = vcleq_f32(vabsq_f32(s), vreinterpretq_f32_u32(vdupq_n_u32(0x4b000000)));
        return vbslq_f32(mask, result, s);
    }

    static inline f32x4 floor(f32x4 s)
    {
        const f32x4 temp = round(s);
        const uint32x4_t mask = vcltq_f32(s, temp);
        const uint32x4_t one = vdupq_n_u32(0x3f800000);
        return vsubq_f32(temp, vreinterpretq_f32_u32(vandq_u32(mask, one)));
    }

    static inline f32x4 ceil(f32x4 s)
    {
        const f32x4 temp = round(s);
        const uint32x4_t mask = vcgtq_f32(s, temp);
        const uint32x4_t one = vdupq_n_u32(0x3f800000);
        return vaddq_f32(temp, vreinterpretq_f32_u32(vandq_u32(mask, one)));
    }

#endif

    static inline f32x4 fract(f32x4 s)
    {
        return sub(s, floor(s));
    }

    // -----------------------------------------------------------------
    // masked functions
    // -----------------------------------------------------------------

    static inline f32x4 min(f32x4 a, f32x4 b, mask32x4 mask)
    {
        return vreinterpretq_f32_u32(vandq_u32(mask, vreinterpretq_u32_f32(min(a, b))));
    }

    static inline f32x4 max(f32x4 a, f32x4 b, mask32x4 mask)
    {
        return vreinterpretq_f32_u32(vandq_u32(mask, vreinterpretq_u32_f32(max(a, b))));
    }

    static inline f32x4 add(f32x4 a, f32x4 b, mask32x4 mask)
    {
        return vreinterpretq_f32_u32(vandq_u32(mask, vreinterpretq_u32_f32(add(a, b))));
    }

    static inline f32x4 sub(f32x4 a, f32x4 b, mask32x4 mask)
    {
        return vreinterpretq_f32_u32(vandq_u32(mask, vreinterpretq_u32_f32(sub(a, b))));
    }

    static inline f32x4 mul(f32x4 a, f32x4 b, mask32x4 mask)
    {
        return vreinterpretq_f32_u32(vandq_u32(mask, vreinterpretq_u32_f32(mul(a, b))));
    }

    static inline f32x4 div(f32x4 a, f32x4 b, mask32x4 mask)
    {
        return vreinterpretq_f32_u32(vandq_u32(mask, vreinterpretq_u32_f32(div(a, b))));
    }

    // -----------------------------------------------------------------
    // f64x2
    // -----------------------------------------------------------------

#ifdef __aarch64__

#ifdef MANGO_COMPILER_CLANG

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 v)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return __builtin_shufflevector(v.data, v.data, x, y + 2);
    }

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 a, f64x2 b)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return __builtin_shufflevector(a.data, b.data, x, y + 2);
    }

#else

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 v)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        float64x2_t result;
        result = vmovq_n_f64(vgetq_lane_f64(v, x));
        result = vsetq_lane_f64(vgetq_lane_f64(v, y), result, 1);
        return result;
    }

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 a, f64x2 b)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        float64x2_t result;
        result = vmovq_n_f64(vgetq_lane_f64(a, x));
        result = vsetq_lane_f64(vgetq_lane_f64(b, y), result, 1);
        return result;
    }

#endif // MANGO_COMPILER_CLANG

    // indexed access

    template <unsigned int Index>
    static inline f64x2 set_component(f64x2 a, f64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return vsetq_lane_f64(s, a, Index);
    }

    template <unsigned int Index>
    static inline f64 get_component(f64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return vgetq_lane_f64(a, Index);
    }

    static inline f64x2 f64x2_zero()
    {
        return vdupq_n_f64(0.0);
    }

    static inline f64x2 f64x2_set(f64 s)
    {
        return vdupq_n_f64(s);
    }

    static inline f64x2 f64x2_set(f64 x, f64 y)
    {
        float64x2_t temp = { x, y };
        return temp;
    }

#if defined(MANGO_COMPILER_GCC)

    static inline f64x2 f64x2_uload(const void* source)
    {
        f64x2 temp;
        std::memcpy(&temp, source, sizeof(temp));
        return temp;
    }

    static inline void f64x2_ustore(void* dest, f64x2 a)
    {
        std::memcpy(dest, &a, sizeof(a));
    }

#else

    static inline f64x2 f64x2_uload(const void* source)
    {
        return vld1q_f64(reinterpret_cast<const f64*>(source));
    }

    static inline void f64x2_ustore(void* dest, f64x2 a)
    {
        vst1q_f64(reinterpret_cast<f64*>(dest), a);
    }

#endif // MANGO_COMPILER_GCC

    static inline f64x2 unpackhi(f64x2 a, f64x2 b)
    {
        return vcombine_f64(vget_high_f64(a), vget_high_f64(b));
    }

    static inline f64x2 unpacklo(f64x2 a, f64x2 b)
    {
        return vcombine_f64(vget_low_f64(a), vget_low_f64(b));
    }

    // bitwise

    static inline f64x2 bitwise_nand(f64x2 a, f64x2 b)
    {
        return vreinterpretq_f64_s64(vbicq_s64(vreinterpretq_s64_f64(a), vreinterpretq_s64_f64(b)));
    }

    static inline f64x2 bitwise_and(f64x2 a, f64x2 b)
    {
        return vreinterpretq_f64_s64(vandq_s64(vreinterpretq_s64_f64(a), vreinterpretq_s64_f64(b)));
    }

    static inline f64x2 bitwise_or(f64x2 a, f64x2 b)
    {
        return vreinterpretq_f64_s64(vorrq_s64(vreinterpretq_s64_f64(a), vreinterpretq_s64_f64(b)));
    }

    static inline f64x2 bitwise_xor(f64x2 a, f64x2 b)
    {
        return vreinterpretq_f64_s64(veorq_s64(vreinterpretq_s64_f64(a), vreinterpretq_s64_f64(b)));
    }

    static inline f64x2 bitwise_not(f64x2 a)
    {
        return vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(a), vceqq_f64(a, a)));
    }

    static inline f64x2 min(f64x2 a, f64x2 b)
    {
        return vminq_f64(a, b);
    }

    static inline f64x2 max(f64x2 a, f64x2 b)
    {
        return vmaxq_f64(a, b);
    }

    static inline f64x2 abs(f64x2 a)
    {
        return vabsq_f64(a);
    }

    static inline f64x2 neg(f64x2 a)
    {
        return vnegq_f64(a);
    }

    static inline f64x2 sign(f64x2 a)
    {
        f64 x = vgetq_lane_f64(a, 0);
        f64 y = vgetq_lane_f64(a, 1);
        x = x < 0 ? -1.0 : (x > 0 ? 1.0 : 0.0);
        y = y < 0 ? -1.0 : (y > 0 ? 1.0 : 0.0);
        return f64x2_set(x, y);
    }

    static inline f64x2 add(f64x2 a, f64x2 b)
    {
        return vaddq_f64(a, b);
    }

    static inline f64x2 sub(f64x2 a, f64x2 b)
    {
        return vsubq_f64(a, b);
    }

    static inline f64x2 mul(f64x2 a, f64x2 b)
    {
        return vmulq_f64(a, b);
    }

    static inline f64x2 div(f64x2 a, f64x2 b)
    {
        return vdivq_f64(a, b);
    }

    static inline f64x2 div(f64x2 a, f64 b)
    {
        f64x2 s = vdupq_n_f64(b);
        return vdivq_f64(a, s);
    }

    static inline f64x2 hadd(f64x2 a, f64x2 b)
    {
        return vpaddq_f64(a, b);
    }

    static inline f64x2 hsub(f64x2 a, f64x2 b)
    {
        b = vnegq_f64(b);
        return vpaddq_f64(a, b);
    }

    static inline f64x2 madd(f64x2 a, f64x2 b, f64x2 c)
    {
        // a + b * c
        return vmlaq_f64(a, b, c);
    }

    static inline f64x2 msub(f64x2 a, f64x2 b, f64x2 c)
    {
        // b * c - a
        return vnegq_f64(vmlsq_f64(a, b, c));
    }

    static inline f64x2 nmadd(f64x2 a, f64x2 b, f64x2 c)
    {
        // a - b * c
        return vmlsq_f64(a, b, c);
    }

    static inline f64x2 nmsub(f64x2 a, f64x2 b, f64x2 c)
    {
        // -(a + b * c)
        return vnegq_f64(vmlaq_f64(a, b, c));
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
        f64x2 e = vrecpeq_f64(a);
        e = vmulq_f64(vrecpsq_f64(a, e), e);
        return e;
    }

    static inline f64x2 rsqrt(f64x2 a)
    {
        f64x2 e = vrsqrteq_f64(a);
        e = vmulq_f64(vrsqrtsq_f64(vmulq_f64(a, e), e), e);
        return e;
    }

#else

    static inline f64x2 rcp(f64x2 a)
    {
        f64x2 e = vrecpeq_f64(a);
        e = vmulq_f64(vrecpsq_f64(a, e), e);
        e = vmulq_f64(vrecpsq_f64(a, e), e);
        return e;
    }

    static inline f64x2 rsqrt(f64x2 a)
    {
        f64x2 e = vrsqrteq_f64(a);
        e = vmulq_f64(vrsqrtsq_f64(vmulq_f64(a, e), e), e);
        e = vmulq_f64(vrsqrtsq_f64(vmulq_f64(a, e), e), e);
        return e;
    }

#endif // MANGO_FAST_MATH

    static inline f64x2 sqrt(f64x2 a)
    {
        return vsqrtq_f64(a);
    }

    static inline f64 dot2(f64x2 a, f64x2 b)
    {
        const float64x2_t prod = vmulq_f64(a, b);
        return vaddvq_f64(prod);
    }

    // compare

    static inline mask64x2 compare_neq(f64x2 a, f64x2 b)
    {
        return veorq_u64(vceqq_f64(a, b), vceqq_f64(a, a));
    }

    static inline mask64x2 compare_eq(f64x2 a, f64x2 b)
    {
        return vceqq_f64(a, b);
    }

    static inline mask64x2 compare_lt(f64x2 a, f64x2 b)
    {
        return vcltq_f64(a, b);
    }

    static inline mask64x2 compare_le(f64x2 a, f64x2 b)
    {
        return vcleq_f64(a, b);
    }

    static inline mask64x2 compare_gt(f64x2 a, f64x2 b)
    {
        return vcgtq_f64(a, b);
    }

    static inline mask64x2 compare_ge(f64x2 a, f64x2 b)
    {
        return vcgeq_f64(a, b);
    }

    static inline f64x2 select(mask64x2 mask, f64x2 a, f64x2 b)
    {
        return vbslq_f64(mask, a, b);
    }

    // rounding

    static inline f64x2 round(f64x2 s)
    {
        return vrndaq_f64(s);
    }

    static inline f64x2 trunc(f64x2 s)
    {
        return vrndq_f64(s);
    }

    static inline f64x2 floor(f64x2 s)
    {
        return vrndmq_f64(s);
    }

    static inline f64x2 ceil(f64x2 s)
    {
        return vrndpq_f64(s);
    }

    static inline f64x2 fract(f64x2 s)
    {
        return sub(s, floor(s));
    }

    // -----------------------------------------------------------------
    // masked functions
    // -----------------------------------------------------------------

    static inline f64x2 min(f64x2 a, f64x2 b, mask64x2 mask)
    {
        return vreinterpretq_f64_u64(vandq_u64(mask, vreinterpretq_u64_f64(min(a, b))));
    }

    static inline f64x2 max(f64x2 a, f64x2 b, mask64x2 mask)
    {
        return vreinterpretq_f64_u64(vandq_u64(mask, vreinterpretq_u64_f64(max(a, b))));
    }

    static inline f64x2 add(f64x2 a, f64x2 b, mask64x2 mask)
    {
        return vreinterpretq_f64_u64(vandq_u64(mask, vreinterpretq_u64_f64(add(a, b))));
    }

    static inline f64x2 sub(f64x2 a, f64x2 b, mask64x2 mask)
    {
        return vreinterpretq_f64_u64(vandq_u64(mask, vreinterpretq_u64_f64(sub(a, b))));
    }

    static inline f64x2 mul(f64x2 a, f64x2 b, mask64x2 mask)
    {
        return vreinterpretq_f64_u64(vandq_u64(mask, vreinterpretq_u64_f64(mul(a, b))));
    }

    static inline f64x2 div(f64x2 a, f64x2 b, mask64x2 mask)
    {
        return vreinterpretq_f64_u64(vandq_u64(mask, vreinterpretq_u64_f64(div(a, b))));
    }

#else // __aarch64__

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 v)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return {{ v.data[x], v.data[y] }};
    }

    template <u32 x, u32 y>
    static inline f64x2 shuffle(f64x2 a, f64x2 b)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
        return {{ a.data[x], b.data[y] }};
    }

    // indexed access

    template <unsigned int Index>
    static inline f64x2 set_component(f64x2 a, f64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        a.data[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline f64 get_component(f64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return a.data[Index];
    }

    static inline f64x2 f64x2_zero()
    {
        return {{ 0.0, 0.0 }};
    }

    static inline f64x2 f64x2_set(f64 s)
    {
        return {{ s, s }};
    }

    static inline f64x2 f64x2_set(f64 x, f64 y)
    {
        return {{ x, y }};
    }

    static inline f64x2 f64x2_uload(const void* source)
    {
        f64x2 temp;
        std::memcpy(&temp, source, sizeof(temp));
        return temp;
    }

    static inline void f64x2_ustore(void* dest, f64x2 a)
    {
        std::memcpy(dest, &a, sizeof(a));
    }

    static inline f64x2 unpackhi(f64x2 a, f64x2 b)
    {
        return f64x2_set(a.data[1], b.data[1]);
    }

    static inline f64x2 unpacklo(f64x2 a, f64x2 b)
    {
        return f64x2_set(a.data[0], b.data[0]);
    }

    // bitwise

    static inline f64x2 bitwise_nand(f64x2 a, f64x2 b)
    {
        const Double x(~Double(a.data[0]).u & Double(b.data[0]).u);
        const Double y(~Double(a.data[1]).u & Double(b.data[1]).u);
        return f64x2_set(x, y);
    }

    static inline f64x2 bitwise_and(f64x2 a, f64x2 b)
    {
        const Double x(Double(a.data[0]).u & Double(b.data[0]).u);
        const Double y(Double(a.data[1]).u & Double(b.data[1]).u);
        return f64x2_set(x, y);
    }

    static inline f64x2 bitwise_or(f64x2 a, f64x2 b)
    {
        const Double x(Double(a.data[0]).u | Double(b.data[0]).u);
        const Double y(Double(a.data[1]).u | Double(b.data[1]).u);
        return f64x2_set(x, y);
    }

    static inline f64x2 bitwise_xor(f64x2 a, f64x2 b)
    {
        const Double x(Double(a.data[0]).u ^ Double(b.data[0]).u);
        const Double y(Double(a.data[1]).u ^ Double(b.data[1]).u);
        return f64x2_set(x, y);
    }

    static inline f64x2 bitwise_not(f64x2 a)
    {
        const Double x(~Double(a.data[0]).u);
        const Double y(~Double(a.data[1]).u);
        return f64x2_set(x, y);
    }

    static inline f64x2 min(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = std::min(a.data[0], b.data[0]);
        v.data[1] = std::min(a.data[1], b.data[1]);
        return v;
    }

    static inline f64x2 max(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = std::max(a.data[0], b.data[0]);
        v.data[1] = std::max(a.data[1], b.data[1]);
        return v;
    }

    static inline f64x2 abs(f64x2 a)
    {
        f64x2 v;
        v.data[0] = std::abs(a.data[0]);
        v.data[1] = std::abs(a.data[1]);
        return v;
    }

    static inline f64x2 neg(f64x2 a)
    {
        return f64x2_set(-a.data[0], -a.data[1]);
    }

    static inline f64x2 sign(f64x2 a)
    {
        f64x2 v;
        v.data[0] = a.data[0] < 0 ? -1.0f : (a.data[0] > 0 ? 1.0f : 0.0f);
        v.data[1] = a.data[1] < 0 ? -1.0f : (a.data[1] > 0 ? 1.0f : 0.0f);
        return v;
    }

    static inline f64x2 add(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = a.data[0] + b.data[0];
        v.data[1] = a.data[1] + b.data[1];
        return v;
    }

    static inline f64x2 sub(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = a.data[0] - b.data[0];
        v.data[1] = a.data[1] - b.data[1];
        return v;
    }

    static inline f64x2 mul(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = a.data[0] * b.data[0];
        v.data[1] = a.data[1] * b.data[1];
        return v;
    }

    static inline f64x2 div(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = a.data[0] / b.data[0];
        v.data[1] = a.data[1] / b.data[1];
        return v;
    }

    static inline f64x2 div(f64x2 a, f64 b)
    {
        f64x2 v;
        v.data[0] = a.data[0] / b;
        v.data[1] = a.data[1] / b;
        return v;
    }

    static inline f64x2 hadd(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = a.data[0] + a.data[1];
        v.data[1] = b.data[0] + b.data[1];
        return v;
    }

    static inline f64x2 hsub(f64x2 a, f64x2 b)
    {
        f64x2 v;
        v.data[0] = a.data[0] - a.data[1];
        v.data[1] = b.data[0] - b.data[1];
        return v;
    }

    static inline f64x2 madd(f64x2 a, f64x2 b, f64x2 c)
    {
        // a + b * c
        f64x2 v;
        v.data[0] = a.data[0] + b.data[0] * c.data[0];
        v.data[1] = a.data[1] + b.data[1] * c.data[1];
        return v;
    }

    static inline f64x2 msub(f64x2 a, f64x2 b, f64x2 c)
    {
        // b * c - a
        f64x2 v;
        v.data[0] = b.data[0] * c.data[0] - a.data[0];
        v.data[1] = b.data[1] * c.data[1] - a.data[1];
        return v;
    }

    static inline f64x2 nmadd(f64x2 a, f64x2 b, f64x2 c)
    {
        // a - b * c
        f64x2 v;
        v.data[0] = a.data[0] - b.data[0] * c.data[0];
        v.data[1] = a.data[1] - b.data[1] * c.data[1];
        return v;
    }

    static inline f64x2 nmsub(f64x2 a, f64x2 b, f64x2 c)
    {
        // -(a + b * c)
        f64x2 v;
        v.data[0] = -(a.data[0] + b.data[0] * c.data[0]);
        v.data[1] = -(a.data[1] + b.data[1] * c.data[1]);
        return v;
    }

    static inline f64x2 lerp(f64x2 a, f64x2 b, f64x2 s)
    {
        // a * (1.0 - s) + b * s
        // (a - a * s) + (b * s)
        return madd(nmadd(a, a, s), b, s);
    }

    static inline f64x2 rcp(f64x2 a)
    {
        f64x2 v;
        v.data[0] = 1.0 / a.data[0];
        v.data[1] = 1.0 / a.data[1];
        return v;
    }

    static inline f64x2 rsqrt(f64x2 a)
    {
        f64x2 v;
        v.data[0] = 1.0 / std::sqrt(a.data[0]);
        v.data[1] = 1.0 / std::sqrt(a.data[1]);
        return v;
    }

    static inline f64x2 sqrt(f64x2 a)
    {
        f64x2 v;
        v.data[0] = std::sqrt(a.data[0]);
        v.data[1] = std::sqrt(a.data[1]);
        return v;
    }

    static inline f64 dot2(f64x2 a, f64x2 b)
    {
        return a.data[0] * b.data[0] + a.data[1] * b.data[1];
    }

    // compare

    static inline mask64x2 compare_neq(f64x2 a, f64x2 b)
    {
        u64 x = a.data[0] != b.data[0] ? ~0 : 0;
        u64 y = a.data[1] != b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_eq(f64x2 a, f64x2 b)
    {
        u64 x = a.data[0] == b.data[0] ? ~0 : 0;
        u64 y = a.data[1] == b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_lt(f64x2 a, f64x2 b)
    {
        u64 x = a.data[0] < b.data[0] ? ~0 : 0;
        u64 y = a.data[1] < b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_le(f64x2 a, f64x2 b)
    {
        u64 x = a.data[0] <= b.data[0] ? ~0 : 0;
        u64 y = a.data[1] <= b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_gt(f64x2 a, f64x2 b)
    {
        u64 x = a.data[0] > b.data[0] ? ~0 : 0;
        u64 y = a.data[1] > b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline mask64x2 compare_ge(f64x2 a, f64x2 b)
    {
        u64 x = a.data[0] >= b.data[0] ? ~0 : 0;
        u64 y = a.data[1] >= b.data[1] ? ~0 : 0;
        uint64x2_t mask = { x, y };
        return mask;
    }

    static inline f64x2 select(mask64x2 mask, f64x2 a, f64x2 b)
    {
        f64x2 m;
        m.data[0] = vgetq_lane_u64(mask, 0);
        m.data[1] = vgetq_lane_u64(mask, 1);
        return bitwise_or(bitwise_and(m, a), bitwise_nand(m, b));
    }

    // rounding

    static inline f64x2 round(f64x2 s)
    {
        f64x2 v;
        v.data[0] = std::round(s.data[0]);
        v.data[1] = std::round(s.data[1]);
        return v;
    }

    static inline f64x2 trunc(f64x2 s)
    {
        f64x2 v;
        v.data[0] = std::trunc(s.data[0]);
        v.data[1] = std::trunc(s.data[1]);
        return v;
    }

    static inline f64x2 floor(f64x2 s)
    {
        f64x2 v;
        v.data[0] = std::floor(s.data[0]);
        v.data[1] = std::floor(s.data[1]);
        return v;
    }

    static inline f64x2 ceil(f64x2 s)
    {
        f64x2 v;
        v.data[0] = std::ceil(s.data[0]);
        v.data[1] = std::ceil(s.data[1]);
        return v;
    }

    static inline f64x2 fract(f64x2 s)
    {
        f64x2 v;
        v.data[0] = s.data[0] - std::floor(s.data[0]);
        v.data[1] = s.data[1] - std::floor(s.data[1]);
        return v;
    }

#endif // __aarch64__

    // -----------------------------------------------------------------
    // masked functions
    // -----------------------------------------------------------------

#ifdef __aarch64__
    #define SIMD_MASK_FLOAT128
    #define SIMD_MASK_DOUBLE128
    #include <mango/simd/common_mask.hpp>
    #undef SIMD_MASK_FLOAT128
    #undef SIMD_MASK_DOUBLE128
#else
    #define SIMD_ZEROMASK_DOUBLE128
    #define SIMD_MASK_FLOAT128
    #define SIMD_MASK_DOUBLE128
    #include <mango/simd/common_mask.hpp>
    #undef SIMD_ZEROMASK_DOUBLE128
    #undef SIMD_MASK_FLOAT128
    #undef SIMD_MASK_DOUBLE128
#endif // __aarch64__

} // namespace mango::simd
