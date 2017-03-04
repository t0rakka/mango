/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifdef MANGO_INCLUDE_SIMD

#include "common.hpp"

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

#endif // MANGO_INCLUDE_SIMD
