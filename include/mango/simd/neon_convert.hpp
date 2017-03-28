/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"
#include "common.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // reinterpret
    // -----------------------------------------------------------------

    static inline int32x4 int32x4_reinterpret(uint32x4 s)
    {
        return vreinterpretq_s32_u32(s);
    }

    static inline int32x4 int32x4_reinterpret(float32x4 s)
    {
        return vreinterpretq_s32_f32(s);
    }

    static inline uint32x4 uint32x4_reinterpret(int32x4 s)
    {
        return vreinterpretq_u32_s32(s);
    }

    static inline uint32x4 uint32x4_reinterpret(float32x4 s)
    {
        return vreinterpretq_u32_f32(s);
    }

    static inline float32x4 float32x4_reinterpret(int32x4 s)
    {
        return vreinterpretq_f32_s32(s);
    }

    static inline float32x4 float32x4_reinterpret(uint32x4 s)
    {
        return vreinterpretq_f32_u32(s);
    }

    // -----------------------------------------------------------------
    // zero extend
    // -----------------------------------------------------------------

    static inline uint16x8 uint16x8_extend(uint8x16 s)
    {
	    const uint8x8x2_t a = vzip_u8(vget_low_u8(s), vdup_n_u8(0));
        return vreinterpretq_u16_u8(vcombine_u8(a.val[0], a.val[1]));
    }

    static inline uint32x4 uint32x4_extend(uint8x16 s)
    {
	    const uint8x8x2_t a = vzip_u8(vget_low_u8(s), vdup_n_u8(0));
	    const uint16x4x2_t b = vzip_u16(vreinterpret_u16_u8(a.val[0]), vdup_n_u16(0));
	    return vreinterpretq_u32_u16(vcombine_u16(b.val[0], b.val[1]));
    }

    static inline uint32x4 uint32x4_extend(uint16x8 s)
    {
	    const uint16x4x2_t a = vzip_u16(vget_low_u16(s), vdup_n_u16(0));
	    return vreinterpretq_u32_u16(vcombine_u16(a.val[0], a.val[1]));
    }

    // -----------------------------------------------------------------
    // sign extend
    // -----------------------------------------------------------------

    static inline int16x8 int16x8_extend(int8x16 s)
    {
        const int8x8_t low = vget_low_s8(s);
        const int8x8_t sign = vreinterpret_s8_u8(vcgt_s8(vdup_n_s8(0), low));
	    const int8x8x2_t temp = vzip_s8(low, sign);
        return vreinterpretq_s16_s8(vcombine_s8(temp.val[0], temp.val[1]));
    }

    static inline int32x4 int32x4_extend(int8x16 s)
    {
        const int8x8x2_t a = vzip_s8(vget_low_s8(s), vdup_n_s8(0));
        const int16x4x2_t b = vzip_s16(vreinterpret_s16_s8(a.val[0]), vdup_n_s16(0));
        const int32x4_t temp = vreinterpretq_s32_s16(vcombine_s16(b.val[0], b.val[1]));
        const int32x4_t sign = vdupq_n_s32(0x80);
        return vsubq_s32(veorq_s32(temp, sign), sign);
    }

    static inline int32x4 int32x4_extend(int16x8 s)
    {
        const int16x4_t low = vget_low_s16(s);
        const int16x4_t sign = vreinterpret_s16_u16(vcgt_s16(vdup_n_s16(0), low));
	    const int16x4x2_t temp = vzip_s16(low, sign);
	    return vreinterpretq_s32_s16(vcombine_s16(temp.val[0], temp.val[1]));
    }

    // -----------------------------------------------------------------
    // pack
    // -----------------------------------------------------------------

    static inline uint8x16 uint8x16_pack(uint16x8 a, uint16x8 b)
    {
        return vcombine_u8(vqmovn_u16(a), vqmovn_u16(b));
    }

    static inline uint16x8 uint16x8_pack(uint32x4 a, uint32x4 b)
    {
        return vcombine_u16(vqmovn_u32(a), vqmovn_u32(b));
    }

    static inline int8x16 int8x16_pack(int16x8 a, int16x8 b)
    {
        return vcombine_s8(vqmovn_s16(a), vqmovn_s16(b));
    }

    static inline int16x8 int16x8_pack(int32x4 a, int32x4 b)
    {
        return vcombine_s16(vqmovn_s32(a), vqmovn_s32(b));
    }

    // -----------------------------------------------------------------
    // float32
    // -----------------------------------------------------------------

    static inline float32x2 get_low(float32x4 a)
    {
        return vget_low_f32(a);
    }

    static inline float32x2 get_high(float32x4 a)
    {
        return vget_high_f32(a);
    }

    static inline float32x4 combine(float32x2 a, float32x2 b)
    {
        return vcombine_f32(a, b);
    }

    static inline float32x4 get_low(float32x8 a)
    {
        return a.lo;
    }

    static inline float32x4 get_high(float32x8 a)
    {
        return a.hi;
    }

    static inline float32x8 combine(float32x4 a, float32x4 b)
    {
        float32x8 result;
        result.lo = a;
        result.hi = b;
        return result;
    }

    static inline float32x4 float32x4_convert(uint32x4 s)
    {
        return vcvtq_f32_u32(s);
    }

    static inline float32x4 float32x4_convert(int32x4 s)
    {
        return vcvtq_f32_s32(s);
    }

#if __ARM_ARCH >= 8 && !defined(MANGO_COMPILER_CLANG)

    // ARMv8 supports rouding float-to-int conversion ...
    // ... but clang doesn't

    static inline uint32x4 uint32x4_convert(float32x4 s)
    {
        return vcvtnq_u32_f32(s);
    }

    static inline int32x4 int32x4_convert(float32x4 s)
    {
        return vcvtnq_s32_f32(s);
    }

#else

    static inline uint32x4 uint32x4_convert(float32x4 s)
    {
        return vcvtq_u32_f32(round(s));
    }

    static inline int32x4 int32x4_convert(float32x4 s)
    {
        return vcvtq_s32_f32(round(s));
    }

#endif

    static inline int32x4 int32x4_truncate(float32x4 s)
    {
        return vcvtq_s32_f32(s);
    }

    // -----------------------------------------------------------------
    // float64
    // -----------------------------------------------------------------

    static inline float64x2 get_low(float64x4 a)
    {
        float64x2 v;
        v[0] = a[0];
        v[1] = a[1];
        return v;
    }

    static inline float64x2 get_high(float64x4 a)
    {
        float64x2 v;
        v[0] = a[2];
        v[1] = a[3];
        return v;
    }

    static inline float64x4 combine(float64x2 a, float64x2 b)
    {
        float64x4 v;
        v[0] = a[0];
        v[1] = a[1];
        v[2] = b[0];
        v[3] = b[1];
        return v;
    }

    static inline float64x4 float64x4_convert(int32x4 s)
    {
        float64x4 v;
        v[0] = double(get_x(s));
        v[1] = double(get_y(s));
        v[2] = double(get_z(s));
        v[3] = double(get_w(s));
        return v;
    }

    static inline float64x4 float64x4_convert(float32x4 s)
    {
        float64x4 v;
        v[0] = double(get_x(s));
        v[1] = double(get_y(s));
        v[2] = double(get_z(s));
        v[3] = double(get_w(s));
        return v;
    }

    static inline int32x4 int32x4_convert(float64x4 s)
    {
        int x = int(s[0] + 0.5);
        int y = int(s[1] + 0.5);
        int z = int(s[2] + 0.5);
        int w = int(s[3] + 0.5);
        return int32x4_set4(x, y, z, w);
    }

    static inline float32x4 float32x4_convert(float64x4 s)
    {
        float x = float(s[0]);
        float y = float(s[1]);
        float z = float(s[2]);
        float w = float(s[3]);
        return float32x4_set4(x, y, z, w);
    }

    static inline float64x4 float64x4_convert(uint32x4 s)
    {
        float64x4 v;
        v[0] = u32_to_f64(get_x(s));
        v[1] = u32_to_f64(get_y(s));
        v[2] = u32_to_f64(get_z(s));
        v[3] = u32_to_f64(get_w(s));
        return v;
    }

    static inline uint32x4 uint32x4_convert(float64x4 d)
    {
        uint32 x = f64_to_u32(d[0]);
        uint32 y = f64_to_u32(d[1]);
        uint32 z = f64_to_u32(d[2]);
        uint32 w = f64_to_u32(d[3]);
        return uint32x4_set4(x, y, z, w);
    }

    static inline int32x4 int32x4_truncate(float64x4 s)
    {
        int x = int(s[0]);
        int y = int(s[1]);
        int z = int(s[2]);
        int w = int(s[3]);
        return int32x4_set4(x, y, z, w);
    }

    // -----------------------------------------------------------------
    // float16
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
        float x = s[0];
        float y = s[1];
        float z = s[2];
        float w = s[3];
        return float32x4_set4(x, y, z, w);
    }

    static inline float16x4 float16x4_convert(float32x4 s)
    {
        float16x4 v;
        v[0] = get_x(s);
        v[1] = get_y(s);
        v[2] = get_z(s);
        v[3] = get_w(s);
        return v;
    }

#endif // MANGO_ENABLE_FP16

} // namespace simd
} // namespace mango
