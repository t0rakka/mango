/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_SIMD_FLOAT
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

    // -----------------------------------------------------------------
    // float32x4
    // -----------------------------------------------------------------

    // conversion

    static inline float32x4 float32x4_reinterpret(int32x4 s)
    {
        return vreinterpretq_f32_s32(s);
    }

    static inline float32x4 float32x4_convert(int32x4 s)
    {
        return vcvtq_f32_s32(s);
    }

    static inline float32x4 float32x4_unsigned_convert(int32x4 s)
    {
        const uint32x4_t u = vreinterpretq_u32_s32(s);
        return vcvtq_f32_u32(u);
    }

    // shuffle

#ifdef MANGO_COMPILER_CLANG

    template <int x, int y, int z, int w>
    inline float32x4 float32x4_shuffle(float32x4 v)
    {
        return __builtin_shufflevector(v.m, v.m, x, y, z, w);
    }

#else

    template <int x, int y, int z, int w>
    inline float32x4 float32x4_shuffle(float32x4 v)
    {
#if __GNUC__ >= 5
        return (float32x4_t) __builtin_shuffle(v.m, (uint32x4_t) {x, y, z, w});
#else
        return (float32x4_t) { v[x], v[y], v[z], v[w] };
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
    inline float32x4 float32x4_shuffle<0, 1, 2, 3>(float32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed accessor

    template <int Index>
    static inline float32x4 float32x4_set_component(float32x4 a, float s)
    {
        return vsetq_lane_f32(s, a, Index);
    }

    template <int Index>
    static inline float float32x4_get_component(float32x4 a)
    {
        return vgetq_lane_f32(a, Index);
    }

    // set

    static inline float32x4 float32x4_set_x(float32x4 a, float x)
    {
        return vsetq_lane_f32(x, a, 0);
    }

    static inline float32x4 float32x4_set_y(float32x4 a, float y)
    {
        return vsetq_lane_f32(y, a, 1);
    }

    static inline float32x4 float32x4_set_z(float32x4 a, float z)
    {
        return vsetq_lane_f32(z, a, 2);
    }

    static inline float32x4 float32x4_set_w(float32x4 a, float w)
    {
        return vsetq_lane_f32(w, a, 3);
    }

    // get

    static inline float float32x4_get_x(float32x4 a)
    {
        return vgetq_lane_f32(a, 0);
    }

    static inline float float32x4_get_y(float32x4 a)
    {
        return vgetq_lane_f32(a, 1);
    }

    static inline float float32x4_get_z(float32x4 a)
    {
        return vgetq_lane_f32(a, 2);
    }

    static inline float float32x4_get_w(float32x4 a)
    {
        return vgetq_lane_f32(a, 3);
    }

    static inline float32x4 float32x4_splat_x(float32x4 a)
    {
        const float32x2_t xy = vget_low_f32(a);
        return vdupq_lane_f32(xy, 0);
    }
    
    static inline float32x4 float32x4_splat_y(float32x4 a)
    {
        const float32x2_t xy = vget_low_f32(a);
        return vdupq_lane_f32(xy, 1);
    }
    
    static inline float32x4 float32x4_splat_z(float32x4 a)
    {
        const float32x2_t zw = vget_high_f32(a);
        return vdupq_lane_f32(zw, 0);
    }
    
    static inline float32x4 float32x4_splat_w(float32x4 a)
    {
        const float32x2_t zw = vget_high_f32(a);
        return vdupq_lane_f32(zw, 1);
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

    static inline float32x4 float32x4_load(const float* source)
    {
        return vld1q_f32(source);
    }

    static inline float32x4 float32x4_uload(const float* source)
    {
        float32x4_t temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline void float32x4_store(float* dest, float32x4 a)
    {
        vst1q_f32(dest, a);
    }

    static inline void float32x4_ustore(float* dest, float32x4 a)
    {
        dest[0] = vgetq_lane_f32(a, 0);
        dest[1] = vgetq_lane_f32(a, 1);
        dest[2] = vgetq_lane_f32(a, 2);
        dest[3] = vgetq_lane_f32(a, 3);
    }

    static inline float32x4 float32x4_movelh(float32x4 a, float32x4 b)
    {
        return vcombine_f32(vget_low_f32(a), vget_low_f32(b));
    }

    static inline float32x4 float32x4_movehl(float32x4 a, float32x4 b)
    {
        return vcombine_f32(vget_high_f32(b), vget_high_f32(a));
    }

    static inline float32x4 float32x4_unpackhi(float32x4 a, float32x4 b)
    {
        float32x4x2_t v = vzipq_f32(a, b);
        return v.val[1];
    }

    static inline float32x4 float32x4_unpacklo(float32x4 a, float32x4 b)
    {
        float32x4x2_t v = vzipq_f32(a, b);
        return v.val[0];
    }

    // logical

    static inline float32x4 float32x4_and(float32x4 a, float32x4 b)
    {
        return float32x4_reinterpret(int32x4_and(int32x4_reinterpret(a), int32x4_reinterpret(b)));
    }

    static inline float32x4 float32x4_nand(float32x4 a, float32x4 b)
    {
        return float32x4_reinterpret(int32x4_nand(int32x4_reinterpret(a), int32x4_reinterpret(b)));
    }

    static inline float32x4 float32x4_or(float32x4 a, float32x4 b)
    {
        return float32x4_reinterpret(int32x4_or(int32x4_reinterpret(a), int32x4_reinterpret(b)));
    }

    static inline float32x4 float32x4_xor(float32x4 a, float32x4 b)
    {
        return float32x4_reinterpret(int32x4_xor(int32x4_reinterpret(a), int32x4_reinterpret(b)));
    }

    static inline float32x4 float32x4_min(float32x4 a, float32x4 b)
    {
        return vminq_f32(a, b);
    }

    static inline float32x4 float32x4_max(float32x4 a, float32x4 b)
    {
        return vmaxq_f32(a, b);
    }

    static inline float32x4 float32x4_clamp(float32x4 v, float32x4 vmin, float32x4 vmax)
    {
        return vminq_f32(vmax, vmaxq_f32(vmin, v));
    }

    static inline float32x4 float32x4_abs(float32x4 a)
    {
        return vabsq_f32(a);
    }

    static inline float32x4 float32x4_neg(float32x4 a)
    {
        return vnegq_f32(a);
    }

    static inline float32x4 float32x4_add(float32x4 a, float32x4 b)
    {
        return vaddq_f32(a, b);
    }

    static inline float32x4 float32x4_sub(float32x4 a, float32x4 b)
    {
        return vsubq_f32(a, b);
    }

    static inline float32x4 float32x4_mul(float32x4 a, float32x4 b)
    {
        return vmulq_f32(a, b);
    }

#ifdef __aarch64__

    static inline float32x4 float32x4_div(float32x4 a, float32x4 b)
    {
        return vdivq_f32(a, b);
    }

    static inline float32x4 float32x4_div(float32x4 a, float b)
    {
        float32x4 s = vdupq_n_f32(b);
        return vdivq_f32(a, s);
    }

#else

    static inline float32x4 float32x4_div(float32x4 a, float32x4 b)
    {
        float32x4 n = vrecpeq_f32(b);
        n = vmulq_f32(vrecpsq_f32(n, b), n);
        n = vmulq_f32(vrecpsq_f32(n, b), n);
        return vmulq_f32(a, n);
    }

    static inline float32x4 float32x4_div(float32x4 a, float b)
    {
        float32x4 s = vdupq_n_f32(b);
        float32x4 n = vrecpeq_f32(s);
        n = vmulq_f32(vrecpsq_f32(n, s), n);
        n = vmulq_f32(vrecpsq_f32(n, s), n);
        return vmulq_f32(a, n);
    }

#endif

    static inline float32x4 float32x4_madd(float32x4 a, float32x4 b, float32x4 c)
    {
        return vmlaq_f32(a, b, c);
    }

    static inline float32x4 float32x4_msub(float32x4 a, float32x4 b, float32x4 c)
    {
        return vmlsq_f32(a, b, c);
    }

    static inline float32x4 float32x4_fast_reciprocal(float32x4 a)
    {
        float32x4 n = vrecpeq_f32(a);
        n = vmulq_f32(vrecpsq_f32(n, a), n);
        return n;
    }

    static inline float32x4 float32x4_fast_rsqrt(float32x4 a)
    {
        float32x4 n = vrsqrteq_f32(a);
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        return n;
    }

    static inline float32x4 float32x4_fast_sqrt(float32x4 a)
    {
        float32x4 n = vrsqrteq_f32(a);
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        return vmulq_f32(a, n);
    }

    static inline float32x4 float32x4_reciprocal(float32x4 a)
    {
        float32x4 n = vrecpeq_f32(a);
        n = vmulq_f32(vrecpsq_f32(n, a), n);
        n = vmulq_f32(vrecpsq_f32(n, a), n);
        return n;
    }

    static inline float32x4 float32x4_rsqrt(float32x4 a)
    {
        float32x4 n = vrsqrteq_f32(a);
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        return n;
    }

    static inline float32x4 float32x4_sqrt(float32x4 a)
    {
        float32x4 n = vrsqrteq_f32(a);
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        n = vmulq_f32(n, vrsqrtsq_f32(vmulq_f32(n, a), n));
        return vmulq_f32(a, n);
    }

    static inline float32x4 float32x4_dot3(float32x4 a, float32x4 b)
    {
        const float32x4 s = vmulq_f32(a, b);
        const float32x2_t xy = vget_low_f32(s);
        const float32x2_t zw = vget_high_f32(s);
        return vdupq_lane_f32(vadd_f32(vpadd_f32(xy, xy), zw), 0);
    }

    static inline float32x4 float32x4_dot4(float32x4 a, float32x4 b)
    {
        float32x4 m = vmulq_f32(a, b);
        float32x2_t s = vpadd_f32(vget_low_f32(m), vget_high_f32(m));
        s = vpadd_f32(s, s);
        return vdupq_lane_f32(s, 0);
    }

    static inline float32x4 float32x4_cross3(float32x4 a, float32x4 b)
    {
        float32x4 c = vmulq_f32(a, float32x4_shuffle<1, 2, 0, 3>(b));
        c = vmlsq_f32(c, b, float32x4_shuffle<1, 2, 0, 3>(a));
        return float32x4_shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline float32x4 float32x4_compare_neq(float32x4 a, float32x4 b)
    {
        return vreinterpretq_f32_u32(vmvnq_u32(vceqq_f32(a, b)));
    }

    static inline float32x4 float32x4_compare_eq(float32x4 a, float32x4 b)
    {
        return vreinterpretq_f32_u32(vceqq_f32(a, b));
    }

    static inline float32x4 float32x4_compare_lt(float32x4 a, float32x4 b)
    {
        return vreinterpretq_f32_u32(vcltq_f32(a, b));
    }

    static inline float32x4 float32x4_compare_le(float32x4 a, float32x4 b)
    {
        return vreinterpretq_f32_u32(vcleq_f32(a, b));
    }

    static inline float32x4 float32x4_compare_gt(float32x4 a, float32x4 b)
    {
        return vreinterpretq_f32_u32(vcgtq_f32(a, b));
    }

    static inline float32x4 float32x4_compare_ge(float32x4 a, float32x4 b)
    {
        return vreinterpretq_f32_u32(vcgeq_f32(a, b));
    }

    static inline float32x4 float32x4_select(float32x4 mask, float32x4 a, float32x4 b)
    {
        return vbslq_f32(vreinterpretq_u32_f32(mask), a, b);
    }

    // rounding

#if __ARM_ARCH >= 8 && !defined(MANGO_COMPILER_CLANG)

    // Disabled with clang until supported in NDK

    static inline float32x4 float32x4_round(float32x4 s)
    {
        return vrndqa_f32(s);
    }

    static inline float32x4 float32x4_trunc(float32x4 s)
    {
        return vrndq_f32(s);
    }

    static inline float32x4 float32x4_floor(float32x4 s)
    {
        return vrndqm_f32(s);
    }

    static inline float32x4 float32x4_ceil(float32x4 s)
    {
        return vrndqp_f32(s);
    }

#else

    static inline float32x4 float32x4_round(float32x4 s)
    {
        const float32x4_t magic = vdupq_n_f32(12582912.0f); // 1.5 * (1 << 23)
        return vsubq_f32(vaddq_f32(s, magic), magic);
    }

    static inline float32x4 float32x4_trunc(float32x4 s)
    {
        const int32x4_t truncated = vcvtq_s32_f32(s);
        return vcvtq_f32_s32(truncated);
    }

    static inline float32x4 float32x4_floor(float32x4 s)
    {
        const float32x4 temp = float32x4_round(s);
        const uint32x4_t mask = vcltq_f32(s, temp);
        const uint32x4_t one = vdupq_n_u32(0x3f800000);
        return vsubq_f32(temp, vreinterpretq_f32_u32(vandq_u32(mask, one)));
    }

    static inline float32x4 float32x4_ceil(float32x4 s)
    {
        const float32x4 temp = float32x4_round(s);
        const uint32x4_t mask = vcgtq_f32(s, temp);
        const uint32x4_t one = vdupq_n_u32(0x3f800000);
        return vaddq_f32(temp, vreinterpretq_f32_u32(vandq_u32(mask, one)));
    }

#endif

    static inline float32x4 float32x4_fract(float32x4 s)
    {
        return float32x4_sub(s, float32x4_floor(s));
    }

    // -----------------------------------------------------------------
    // float <-> half conversions
    // -----------------------------------------------------------------

#ifdef MANGO_ENABLE_FP16

    static inline float32x4 float32x4_convert(float16x4 s)
    {
        return vcvt_f32_f16(s);
    }

    static inline float16x4 float16x4_convert(float32x4 s)
    {
        return vcvt_f16_f32(s);
    }

#else

    static inline float32x4 float32x4_convert(float16x4 s)
    {
        float x = s.x;
        float y = s.y;
        float z = s.z;
        float w = s.w;
        return float32x4_set4(x, y, z, w);
    }

    static inline float16x4 float16x4_convert(float32x4 s)
    {
        float16x4 v;
        v.x = float32x4_get_x(s);
        v.y = float32x4_get_y(s);
        v.z = float32x4_get_z(s);
        v.w = float32x4_get_w(s);
        return v;
    }

#endif

    // -----------------------------------------------------------------
    // float32x4_matrix
    // -----------------------------------------------------------------

    static inline void float32x4_matrix_set_scale(float32x4* m, float s)
    {
        const float32x4 zero = float32x4_zero();
        const float one = 1.0f;
        m[0] = vsetq_lane_f32(s, zero, 0);
        m[1] = vsetq_lane_f32(s, zero, 1);
        m[2] = vsetq_lane_f32(s, zero, 2);
        m[3] = vsetq_lane_f32(one, zero, 3);
    }

    static inline void float32x4_matrix_set_scale(float32x4* m, float x, float y, float z)
    {
        const float32x4 zero = float32x4_zero();
        const float one = 1.0f;
        m[0] = vsetq_lane_f32(x, zero, 0);
        m[1] = vsetq_lane_f32(y, zero, 1);
        m[2] = vsetq_lane_f32(z, zero, 2);
        m[3] = vsetq_lane_f32(one, zero, 3);
    }

    static inline void float32x4_matrix_set_translate(float32x4* m, float x, float y, float z)
    {
        const float32x4 zero = float32x4_zero();
        const float one = 1.0f;
        m[0] = vsetq_lane_f32(one, zero, 0);
        m[1] = vsetq_lane_f32(one, zero, 1);
        m[2] = vsetq_lane_f32(one, zero, 2);
        m[3] = float32x4_set4(x, y, z, one);
    }

    static inline void float32x4_matrix_scale(float32x4* m, float s)
    {
        const float32x4 v = float32x4_set4(s, s, s, 1.0f);
        m[0] = float32x4_mul(m[0], v);
        m[1] = float32x4_mul(m[1], v);
        m[2] = float32x4_mul(m[2], v);
        m[3] = float32x4_mul(m[3], v);
    }

    static inline void float32x4_matrix_scale(float32x4* m, float x, float y, float z)
    {
        const float32x4 v = float32x4_set4(x, y, z, 1.0f);
        m[0] = float32x4_mul(m[0], v);
        m[1] = float32x4_mul(m[1], v);
        m[2] = float32x4_mul(m[2], v);
        m[3] = float32x4_mul(m[3], v);
    }

    static inline void float32x4_matrix_translate(float32x4* m, float x, float y, float z)
    {
        const float32x4 v = float32x4_set4(x, y, z, 0.0f);
        m[0] = float32x4_madd(m[0], float32x4_shuffle<3, 3, 3, 3>(m[0]), v);
        m[1] = float32x4_madd(m[1], float32x4_shuffle<3, 3, 3, 3>(m[1]), v);
        m[2] = float32x4_madd(m[2], float32x4_shuffle<3, 3, 3, 3>(m[2]), v);
        m[3] = float32x4_madd(m[3], float32x4_shuffle<3, 3, 3, 3>(m[3]), v);
    }

    static inline void float32x4_matrix_transpose(float32x4* result, const float32x4* m)
    {
#if 1
        const float32_t* src = reinterpret_cast<const float32_t*>(m);
        float32_t* dest = reinterpret_cast<float32_t*>(result);
        float32x4x4_t temp = vld4q_f32(src);
        vst1q_f32(dest + 0, temp.val[0]);
        vst1q_f32(dest + 4, temp.val[1]);
        vst1q_f32(dest + 8, temp.val[2]);
        vst1q_f32(dest + 12, temp.val[3]);
#else
        asm volatile (
            "vldmia     %1, { q0-q3 }   \n\t"
            "vtrn.32    q0, q1          \n\t"
            "vtrn.32    q2, q3          \n\t"
            "vswp       d1, d4          \n\t"
            "vswp       d3, d6          \n\t"
            "vstmia     %0, { q0-q3 }   \n\t"
            :
            : "r" (result), "r" (m)
            : "q0", "q1", "q2","q3", "memory"
        );
#endif
    }

    static inline void float32x4_matrix_inverse(float32x4* result, const float32x4* m)
    {
        float32x2_t low0 = vget_low_f32(m[0]);
        float32x2_t low1 = vget_low_f32(m[1]);
        float32x2_t low2 = vget_low_f32(m[2]);
        float32x2_t low3 = vget_low_f32(m[3]);
        float32x2_t high0 = vget_high_f32(m[0]);
        float32x2_t high1 = vget_high_f32(m[1]);
        float32x2_t high2 = vget_high_f32(m[2]);
        float32x2_t high3 = vget_high_f32(m[3]);

        float32x2x2_t n0 = vzip_f32(low0, low1);   // x0, x1, y0, y1
        float32x2x2_t n1 = vzip_f32(high0, high1); // z0, z1, w0, w1
        float32x2x2_t n2 = vzip_f32(low2, low3);   // x2, x3, y2, y3
        float32x2x2_t n3 = vzip_f32(high2, high3); // z2, z3, w2, w3

        float32x4_t row0 = vcombine_f32(n0.val[0], n2.val[0]); // x0, x1, x2, x3
        float32x4_t row1 = vcombine_f32(n2.val[1], n0.val[1]); // y2, y3, y0, y1
        float32x4_t row2 = vcombine_f32(n1.val[0], n3.val[0]); // z0, z1, z2, z3
       	float32x4_t row3 = vcombine_f32(n3.val[1], n1.val[1]); // w2, w3, w0, w1

        float32x4_t temp;
        float32x4_t res0;
        float32x4_t res1;
        float32x4_t res2;
        float32x4_t res3;

        temp = vmulq_f32(row2, row3);
        temp = float32x4_shuffle<1, 0, 3, 2>(temp);
        res0 = vmulq_f32(row1, temp);
        res1 = vmulq_f32(row0, temp);
        temp = float32x4_shuffle<2, 3, 0, 1>(temp);
        res0 = vsubq_f32(vmulq_f32(row1, temp), res0);
        res1 = vsubq_f32(vmulq_f32(row0, temp), res1);
        res1 = float32x4_shuffle<2, 3, 0, 1>(res1);
        temp = vmulq_f32(row1, row2);
        temp = float32x4_shuffle<1, 0, 3, 2>(temp);
        res0 = vmlaq_f32(res0, row3, temp);
        res3 = vmulq_f32(row0, temp);
        temp = float32x4_shuffle<2, 3, 0, 1>(temp);
        res0 = vmlsq_f32(res0, row3, temp);
        res3 = vsubq_f32(vmulq_f32(row0, temp), res3);
        res3 = float32x4_shuffle<2, 3, 0, 1>(res3);
        temp = vmulq_f32(float32x4_shuffle<2, 3, 0, 1>(row1), row3);
        temp = float32x4_shuffle<1, 0, 3, 2>(temp);
        row2 = float32x4_shuffle<2, 3, 0, 1>(row2);
        res0 = vmlaq_f32(res0, row2, temp);
        res2 = vmulq_f32(row0, temp);
        temp = float32x4_shuffle<2, 3, 0, 1>(temp);
        res0 = vmlsq_f32(res0, row2, temp);
        res2 = vsubq_f32(vmulq_f32(row0, temp), res2);
        res2 = float32x4_shuffle<2, 3, 0, 1>(res2);
        temp = vmulq_f32(row0, row1);
        temp = float32x4_shuffle<1, 0, 3, 2>(temp);
        res2 = vmlaq_f32(res2, row3, temp);
        res3 = vsubq_f32(vmulq_f32(row2, temp), res3);
        temp = float32x4_shuffle<2, 3, 0, 1>(temp);
        res2 = vsubq_f32(vmulq_f32(row3, temp), res2);
        res3 = vmlsq_f32(res3, row2, temp);
        temp = vmulq_f32(row0, row3);
        temp = float32x4_shuffle<1, 0, 3,2>(temp);
        res1 = vmlsq_f32(res1, row2, temp);
        res2 = vmlaq_f32(res2, row1, temp);
        temp = float32x4_shuffle<2, 3, 0, 1>(temp);
        res1 = vmlaq_f32(res1, row2, temp);
        res2 = vmlsq_f32(res2, row1, temp);
        temp = vmulq_f32(row0, row2);
        temp = float32x4_shuffle<1, 0, 3, 2>(temp);
        res1 = vmlaq_f32(res1, row3, temp);
        res3 = vmlsq_f32(res3, row1, temp);
        temp = float32x4_shuffle<2, 3, 0, 1>(temp);
        res1 = vmlsq_f32(res1, row3, temp);
        res3 = vmlaq_f32(res3, row1, temp);

        float32x4_t det = vmulq_f32(row0, res0);
        det = vaddq_f32(det, float32x4_shuffle<1, 0, 3, 2>(det));
        det = vaddq_f32(det, float32x4_set1(float32x4_get_z(det)));
        temp = vdupq_n_f32(1.0f / vgetq_lane_f32(det, 0));

        result[0] = vmulq_f32(res0, temp);
        result[1] = vmulq_f32(res1, temp);
        result[2] = vmulq_f32(res2, temp);
        result[3] = vmulq_f32(res3, temp);
    }

    static inline void float32x4_matrix_inverse_transpose(float32x4* result, const float32x4* m)
    {
        float32x4 temp[4];
        float32x4_matrix_inverse(temp, m);
        float32x4_matrix_transpose(result, temp);
    }

    static inline float32x4 float32x4_vector_matrix_multiply(float32x4 v, const float32x4* m)
    {
#if 1
        const float32x4 x = vdupq_n_f32(vgetq_lane_f32(v, 0));
        const float32x4 y = vdupq_n_f32(vgetq_lane_f32(v, 1));
        const float32x4 z = vdupq_n_f32(vgetq_lane_f32(v, 2));
        const float32x4 w = vdupq_n_f32(vgetq_lane_f32(v, 3));
        float32x4 temp = vmulq_f32(x, m[0]);
        temp = vmlaq_f32(temp, y, m[1]);
        temp = vmlaq_f32(temp, z, m[2]);
        temp = vmlaq_f32(temp, w, m[3]);
        return temp;
#else
        float32x4 temp;
        asm volatile (
            "vldmia    %2, { q8-q11 }    \n\t"
            "vmul.f32  %q0, q8, %e1[0]   \n\t"
            "vmla.f32  %q0, q9, %e1[1]   \n\t"
            "vmla.f32  %q0, q10, %f1[0]  \n\t"
            "vmla.f32  %q0, q11, %f1[1]  \n\t"
            : "=w" (temp)
            : "x" (v), "r" (m)
            : "memory", "q8", "q9", "q10", "q11"
        );
        return temp;
#endif
    }

    static inline void float32x4_matrix_matrix_multiply(float32x4* result, const float32x4* a, const float32x4* b)
    {
#if 1
        result[0] = float32x4_vector_matrix_multiply(a[0], b);
        result[1] = float32x4_vector_matrix_multiply(a[1], b);
        result[2] = float32x4_vector_matrix_multiply(a[2], b);
        result[3] = float32x4_vector_matrix_multiply(a[3], b);
#else
        asm volatile (
            "vldmia    %1, { q0-q3 }    \n\t"
            "vldmia    %2, { q8-q11 }   \n\t"
            "vmul.f32  q12, q8, d0[0]   \n\t"
            "vmul.f32  q13, q8, d2[0]   \n\t"
            "vmul.f32  q14, q8, d4[0]   \n\t"
            "vmul.f32  q15, q8, d6[0]   \n\t"
            "vmla.f32  q12, q9, d0[1]   \n\t"
            "vmla.f32  q13, q9, d2[1]   \n\t"
            "vmla.f32  q14, q9, d4[1]   \n\t"
            "vmla.f32  q15, q9, d6[1]   \n\t"
            "vmla.f32  q12, q10, d1[0]  \n\t"
            "vmla.f32  q13, q10, d3[0]  \n\t"
            "vmla.f32  q14, q10, d5[0]  \n\t"
            "vmla.f32  q15, q10, d7[0]  \n\t"
            "vmla.f32  q12, q11, d1[1]  \n\t"
            "vmla.f32  q13, q11, d3[1]  \n\t"
            "vmla.f32  q14, q11, d5[1]  \n\t"
            "vmla.f32  q15, q11, d7[1]  \n\t"
            "vstmia    %0, { q12-q15 }  \n\t"
            :
            : "r" (result), "r" (a), "r" (b)
            : "memory", "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
        );
#endif
    }
