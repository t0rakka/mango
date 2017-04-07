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
    // zero extend
    // -----------------------------------------------------------------

    static inline uint16x8 uint16x8_extend(uint8x16 s)
    {
        uint16x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = s[i];
        }
        return v;
    }

    static inline uint32x4 uint32x4_extend(uint8x16 s)
    {
        uint32x4 v;
        for (int i = 0; i < 4; ++i)
        {
            v[i] = s[i];
        }
        return v;
    }

    static inline uint32x4 uint32x4_extend(uint16x8 s)
    {
        uint32x4 v;
        for (int i = 0; i < 4; ++i)
        {
            v[i] = s[i];
        }
        return v;
    }

    // -----------------------------------------------------------------
    // sign extend
    // -----------------------------------------------------------------

    static inline int16x8 int16x8_extend(int8x16 s)
    {
        int16x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = s[i];
        }
        return v;
    }

    static inline int32x4 int32x4_extend(int8x16 s)
    {
        int32x4 v;
        for (int i = 0; i < 4; ++i)
        {
            v[i] = s[i];
        }
        return v;
    }

    static inline int32x4 int32x4_extend(int16x8 s)
    {
        int32x4 v;
        for (int i = 0; i < 4; ++i)
        {
            v[i] = s[i];
        }
        return v;
    }

    // -----------------------------------------------------------------
    // pack
    // -----------------------------------------------------------------

    static inline uint8x16 uint8x16_pack(uint16x8 a, uint16x8 b)
    {
        uint8x16 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i + 0] = uint8(std::min(uint16(0xff), a[i]));
            v[i + 8] = uint8(std::min(uint16(0xff), b[i]));
        }
        return v;
    }

    static inline uint16x8 uint16x8_pack(uint32x4 a, uint32x4 b)
    {
        uint16x8 v;
        for (int i = 0; i < 4; ++i)
        {
            v[i + 0] = std::min(uint32(0xffff), a[i]);
            v[i + 4] = std::min(uint32(0xffff), b[i]);
        }
        return v;
    }

    static inline int8x16 int8x16_pack(int16x8 a, int16x8 b)
    {
        int8x16 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i + 0] = int8(std::min(int16(0x7f), std::max(int16(-0x80), a[i])));
            v[i + 8] = int8(std::min(int16(0x7f), std::max(int16(-0x80), b[i])));
        }
        return v;
    }

    static inline int16x8 int16x8_pack(int32x4 a, int32x4 b)
    {
        int16x8 v;
        for (int i = 0; i < 4; ++i)
        {
            v[i + 0] = std::min(int32(0x7fff), std::max(int32(-0x8000), a[i]));
            v[i + 4] = std::min(int32(0x7fff), std::max(int32(-0x8000), b[i]));
        }
        return v;
    }

    // -----------------------------------------------------------------
    // float32
    // -----------------------------------------------------------------

    static inline float32x2 get_low(float32x4 a)
    {
        float32x2 v;
        v[0] = a[0];
        v[1] = a[1];
        return v;
    }

    static inline float32x2 get_high(float32x4 a)
    {
        float32x2 v;
        v[0] = a[2];
        v[1] = a[3];
        return v;
    }

    static inline float32x4 combine(float32x2 a, float32x2 b)
    {
        float32x4 v;
        v[0] = a[0];
        v[1] = a[1];
        v[2] = b[0];
        v[3] = b[1];
        return v;
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
        float32x8 v;
        v.lo = a;
        v.hi = b;
        return v;
    }

    static inline float32x4 float32x4_convert(uint32x4 s)
    {
        float32x4 v;
        v[0] = float(s[0]);
        v[1] = float(s[1]);
        v[2] = float(s[2]);
        v[3] = float(s[3]);
        return v;
    }

    static inline float32x4 float32x4_convert(int32x4 s)
    {
        float32x4 v;
        v[0] = float(s[0]);
        v[1] = float(s[1]);
        v[2] = float(s[2]);
        v[3] = float(s[3]);
        return v;
    }

    static inline uint32x4 uint32x4_convert(float32x4 s)
    {
        uint32x4 v;
        v[0] = uint32(s[0] + 0.5f);
        v[1] = uint32(s[1] + 0.5f);
        v[2] = uint32(s[2] + 0.5f);
        v[3] = uint32(s[3] + 0.5f);
        return v;
    }

    static inline int32x4 int32x4_convert(float32x4 s)
    {
        int32x4 v;
        v[0] = int32(s[0] + 0.5f);
        v[1] = int32(s[1] + 0.5f);
        v[2] = int32(s[2] + 0.5f);
        v[3] = int32(s[3] + 0.5f);
        return v;
    }

    static inline int32x4 int32x4_truncate(float32x4 s)
    {
        int32x4 v;
        v[0] = int32(s[0]);
        v[1] = int32(s[1]);
        v[2] = int32(s[2]);
        v[3] = int32(s[3]);
        return v;
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
        float32x4 v;
        v[0] = s[0];
        v[1] = s[1];
        v[2] = s[2];
        v[3] = s[3];
        return v;
    }

    static inline float16x4 float16x4_convert(float32x4 s)
    {
        float16x4 v;
        v[0] = s[0];
        v[1] = s[1];
        v[2] = s[2];
        v[3] = s[3];
        return v;
    }

} // namespace simd
} // namespace mango
