/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

#ifdef MANGO_SIMD_CONVERT_SCALAR

    // -----------------------------------------------------------------
    // reinterpret
    // -----------------------------------------------------------------

    static inline int32x4 int32x4_reinterpret(uint32x4 s)
    {
        return reinterpret_cast<int32x4 &>(s);
    }

    static inline int32x4 int32x4_reinterpret(float32x4 s)
    {
        return reinterpret_cast<int32x4 &>(s);
    }

    static inline uint32x4 uint32x4_reinterpret(int32x4 s)
    {
        return reinterpret_cast<uint32x4 &>(s);
    }

    static inline uint32x4 uint32x4_reinterpret(float32x4 s)
    {
        return reinterpret_cast<uint32x4 &>(s);
    }

    static inline float32x4 float32x4_reinterpret(int32x4 s)
    {
        return reinterpret_cast<float32x4 &>(s);
    }

    static inline float32x4 float32x4_reinterpret(uint32x4 s)
    {
        return reinterpret_cast<float32x4 &>(s);
    }

    // -----------------------------------------------------------------
    // float32
    // -----------------------------------------------------------------

    static inline float32x4 float32x4_convert(int32x4 s)
    {
        float32x4 v;
        v.x = float(s.x);
        v.y = float(s.y);
        v.z = float(s.z);
        v.w = float(s.w);
        return v;
    }

    static inline float32x4 float32x4_convert(uint32x4 s)
    {
        float32x4 v;
        v.x = float(s.x);
        v.y = float(s.y);
        v.z = float(s.z);
        v.w = float(s.w);
        return v;
    }

    static inline int32x4 int32x4_convert(float32x4 s)
    {
        int32x4 v;
        v.x = int(s.x + 0.5f);
        v.y = int(s.y + 0.5f);
        v.z = int(s.z + 0.5f);
        v.w = int(s.w + 0.5f);
        return v;
    }

    static inline int32x4 int32x4_truncate(float32x4 s)
    {
        int32x4 v;
        v.x = int(s.x);
        v.y = int(s.y);
        v.z = int(s.z);
        v.w = int(s.w);
        return v;
    }

    // -----------------------------------------------------------------
    // float64
    // -----------------------------------------------------------------

    static inline float64x4 float64x4_convert(int32x4 s)
    {
        float64x4 v;
        v.x = double(int32x4_get_x(s));
        v.y = double(int32x4_get_y(s));
        v.z = double(int32x4_get_z(s));
        v.w = double(int32x4_get_w(s));
        return v;
    }

    static inline float64x4 float64x4_convert(float32x4 s)
    {
        float64x4 v;
        v.x = double(float32x4_get_x(s));
        v.y = double(float32x4_get_y(s));
        v.z = double(float32x4_get_z(s));
        v.w = double(float32x4_get_w(s));
        return v;
    }

    static inline int32x4 int32x4_convert(float64x4 s)
    {
        int x = int(s.x + 0.5);
        int y = int(s.y + 0.5);
        int z = int(s.z + 0.5);
        int w = int(s.w + 0.5);
        return int32x4_set4(x, y, z, w);
    }

    static inline float32x4 float32x4_convert(float64x4 s)
    {
        float x = float(s.x);
        float y = float(s.y);
        float z = float(s.z);
        float w = float(s.w);
        return float32x4_set4(x, y, z, w);
    }

    static inline float64x4 float64x4_convert(uint32x4 ui)
    {
        float64x4 v;
        v.x = u32_to_f64(uint32x4_get_x(ui));
        v.y = u32_to_f64(uint32x4_get_y(ui));
        v.z = u32_to_f64(uint32x4_get_z(ui));
        v.w = u32_to_f64(uint32x4_get_w(ui));
        return v;
    }

    static inline uint32x4 uint32x4_convert(float64x4 d)
    {
        uint32 x = f64_to_u32(d.x);
        uint32 y = f64_to_u32(d.y);
        uint32 z = f64_to_u32(d.z);
        uint32 w = f64_to_u32(d.w);
        return uint32x4_set4(x, y, z, w);
    }

    static inline int32x4 int32x4_truncate(float64x4 s)
    {
        int x = int(s.x);
        int y = int(s.y);
        int z = int(s.z);
        int w = int(s.w);
        return int32x4_set4(x, y, z, w);
    }

    // -----------------------------------------------------------------
    // float16
    // -----------------------------------------------------------------

    static inline float32x4 float32x4_convert(float16x4 s)
    {
        float32x4 v;
        v.x = s.x;
        v.y = s.y;
        v.z = s.z;
        v.w = s.w;
        return v;
    }

    static inline float16x4 float16x4_convert(float32x4 s)
    {
        float16x4 v;
        v.x = s.x;
        v.y = s.y;
        v.z = s.z;
        v.w = s.w;
        return v;
    }

#endif // MANGO_SIMD_CONVERT_SCALAR
