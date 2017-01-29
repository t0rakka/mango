/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"
#include "common.hpp"

#ifdef MANGO_SIMD_CONVERT_NEON

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
    // float32
    // -----------------------------------------------------------------

    static inline float32x4 float32x4_convert(int32x4 s)
    {
        return vcvtq_f32_s32(s);
    }

    static inline float32x4 float32x4_convert(uint32x4 s)
    {
        return vcvtq_f32_u32(s);
    }

    static inline int32x4 int32x4_convert(float32x4 s)
    {
        const uint32x4_t temp = vandq_u32(vreinterpretq_u32_f32(s), vreinterpretq_u32_f32(vdupq_n_f32(-0.0f)));
        const uint32x4_t half = vreinterpretq_u32_f32(vdupq_n_f32(0.5f));
        return vcvtq_s32_f32(vaddq_f32(s, vreinterpretq_f32_u32(vorrq_u32(temp, half))));
    }

    static inline int32x4 int32x4_truncate(float32x4 s)
    {
        return vcvtq_s32_f32(s);
    }

    // -----------------------------------------------------------------
    // float64
    // -----------------------------------------------------------------

    static inline float64x4 float64x4_convert(int32x4 s)
    {
        float64x4 v;
        v[0] = double(int32x4_get_x(s));
        v[1] = double(int32x4_get_y(s));
        v[2] = double(int32x4_get_z(s));
        v[3] = double(int32x4_get_w(s));
        return v;
    }

    static inline float64x4 float64x4_convert(float32x4 s)
    {
        float64x4 v;
        v[0] = double(float32x4_get_x(s));
        v[1] = double(float32x4_get_y(s));
        v[2] = double(float32x4_get_z(s));
        v[3] = double(float32x4_get_w(s));
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
        v[0] = u32_to_f64(uint32x4_get_x(s));
        v[1] = u32_to_f64(uint32x4_get_y(s));
        v[2] = u32_to_f64(uint32x4_get_z(s));
        v[3] = u32_to_f64(uint32x4_get_w(s));
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
        v[0] = float32x4_get_x(s);
        v[1] = float32x4_get_y(s);
        v[2] = float32x4_get_z(s);
        v[3] = float32x4_get_w(s);
        return v;
    }

#endif // MANGO_ENABLE_FP16

} // namespace simd
} // namespace mango

#endif // MANGO_SIMD_CONVERT_NEON
