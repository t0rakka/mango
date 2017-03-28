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
        return (int32x4) s;
    }

    static inline int32x4 int32x4_reinterpret(float32x4 s)
    {
		return (int32x4) s;
    }

    static inline uint32x4 uint32x4_reinterpret(int32x4 s)
    {
        return (uint32x4) s;
    }

    static inline uint32x4 uint32x4_reinterpret(float32x4 s)
    {
        return (uint32x4) s;
    }

    static inline float32x4 float32x4_reinterpret(int32x4 s)
    {
		return (float32x4) s;
    }

    static inline float32x4 float32x4_reinterpret(uint32x4 s)
    {
		return (float32x4) s;
    }

    // -----------------------------------------------------------------
    // zero extend
    // -----------------------------------------------------------------

    static inline uint16x8 uint16x8_extend(uint8x16 s)
    {
        // TODO
        return s;
    }

    static inline uint32x4 uint32x4_extend(uint8x16 s)
    {
        // TODO
        return s;
    }

    static inline uint32x4 uint32x4_extend(uint16x8 s)
    {
        // TODO
        return s;
    }

    // -----------------------------------------------------------------
    // sign extend
    // -----------------------------------------------------------------

    static inline int16x8 int16x8_extend(int8x16 s)
    {
        // TODO
        return s;
    }

    static inline int32x4 int32x4_extend(int8x16 s)
    {
        // TODO
        return s;
    }

    static inline int32x4 int32x4_extend(int16x8 s)
    {
        // TODO
        return s;
    }

    // -----------------------------------------------------------------
    // pack
    // -----------------------------------------------------------------

    static inline uint8x16 uint8x16_pack(uint16x8 a, uint16x8 b)
    {
        // TODO
    }

    static inline uint16x8 uint16x8_pack(uint32x4 a, uint32x4 b)
    {
        // TODO
    }

    static inline int8x16 int8x16_pack(int16x8 a, int16x8 b)
    {
        // TODO
    }

    static inline int16x8 int16x8_pack(int32x4 a, int32x4 b)
    {
        // TODO
    }

    // -----------------------------------------------------------------
    // float32
    // -----------------------------------------------------------------

    static inline float32x2 get_low(float32x4 a)
    {
        float32x2 v;
        v[0] = get_x(a);
        v[1] = get_y(a);
        return v;
    }

    static inline float32x2 get_high(float32x4 a)
    {
        float32x2 v;
        v[0] = get_z(a);
        v[1] = get_w(a);
        return v;
    }

    static inline float32x4 combine(float32x2 a, float32x2 b)
    {
        return float32x4_set4(a[0], a[1], b[0], b[1]);
    }

    static inline float32x4 get_low(float32x8 a)
    {
        float x = a[0];
        float y = a[1];
        float z = a[2];
        float w = a[3];
        return float32x4_set4(x, y, z, w);
    }

    static inline float32x4 get_high(float32x8 a)
    {
        float x = a[4];
        float y = a[5];
        float z = a[6];
        float w = a[7];
        return float32x4_set4(x, y, z, w);
    }

    static inline float32x8 combine(float32x4 a, float32x4 b)
    {
        float32x8 result;
        result[0] = float32x4_get_x(a);
        result[1] = float32x4_get_y(a);
        result[2] = float32x4_get_z(a);
        result[3] = float32x4_get_w(a);
        result[4] = float32x4_get_x(b);
        result[5] = float32x4_get_y(b);
        result[6] = float32x4_get_z(b);
        result[7] = float32x4_get_w(b);
        return result;
    }

    static inline float32x4 float32x4_convert(uint32x4 s)
    {
        return spu_convtf(s, 0);
    }

    static inline float32x4 float32x4_convert(int32x4 s)
    {
        return spu_convtf(s, 0);
    }

    static inline uint32x4 uint32x4_convert(float32x4 s)
    {
        return spu_convts(s, 0);
    }

    static inline int32x4 int32x4_convert(float32x4 s)
    {
        return spu_convts(s, 0);
    }

    static inline int32x4 int32x4_truncate(float32x4 s)
    {
        const vec_uint4 inrange = spu_cmpabsgt((float32x4)spu_splats(0x4b000000), s);
        const vec_int4 si = spu_convts(s, 0);
        const vec_float4 st = spu_sel(s, spu_convtf(si, 0), inrange);
        return spu_convts(st, 0);
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

    static inline float64x4 float64x4_convert(uint32x4 ui)
    {
        float64x4 v;
        v[0] = u32_to_f64(get_x(ui));
        v[1] = u32_to_f64(get_y(ui));
        v[2] = u32_to_f64(get_z(ui));
        v[3] = u32_to_f64(get_w(ui));
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
