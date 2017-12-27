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

	template <typename D, typename S0, int S1>
	inline D reinterpret(scalar_vector<S0, S1> s)
	{
        static_assert(sizeof(scalar_vector<S0, S1>) == sizeof(D), "Vectors must be same size.");
        return reinterpret_cast<const D &>(s);
	}

	template <typename D, typename S>
	inline D reinterpret(composite_vector<S> s)
	{
        static_assert(sizeof(composite_vector<S>) == sizeof(D), "Vectors must be same size.");
        return reinterpret_cast<const D &>(s);
	}

    // -----------------------------------------------------------------
    // convert
    // -----------------------------------------------------------------

	template <typename D, typename S>
	inline D convert(S)
	{
		D::undefined_conversion();
	}

	template <typename D, typename S>
	inline D truncate(S)
	{
		D::undefined_conversion();
	}

    // -----------------------------------------------------------------
    // zero extend
    // -----------------------------------------------------------------

    static inline uint16x8 extend16(uint8x16 s)
    {
        uint16x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = s[i];
        }
        return v;
    }

    static inline uint32x4 extend32(uint8x16 s)
    {
        uint32x4 v;
        for (int i = 0; i < 4; ++i)
        {
            v[i] = s[i];
        }
        return v;
    }

    static inline uint32x4 extend32(uint16x8 s)
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

    static inline int16x8 extend16(int8x16 s)
    {
        int16x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = s[i];
        }
        return v;
    }

    static inline int32x4 extend32(int8x16 s)
    {
        int32x4 v;
        for (int i = 0; i < 4; ++i)
        {
            v[i] = s[i];
        }
        return v;
    }

    static inline int32x4 extend32(int16x8 s)
    {
        int32x4 v;
        for (int i = 0; i < 4; ++i)
        {
            v[i] = s[i];
        }
        return v;
    }

    // -----------------------------------------------------------------
    // narrow
    // -----------------------------------------------------------------

    static inline uint8x16 narrow(uint16x8 a, uint16x8 b)
    {
        uint8x16 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i + 0] = uint8(std::min(uint16(0xff), a[i]));
            v[i + 8] = uint8(std::min(uint16(0xff), b[i]));
        }
        return v;
    }

    static inline uint16x8 narrow(uint32x4 a, uint32x4 b)
    {
        uint16x8 v;
        for (int i = 0; i < 4; ++i)
        {
            v[i + 0] = std::min(uint32(0xffff), a[i]);
            v[i + 4] = std::min(uint32(0xffff), b[i]);
        }
        return v;
    }

    static inline int8x16 narrow(int16x8 a, int16x8 b)
    {
        int8x16 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i + 0] = int8(std::min(int16(0x7f), std::max(int16(-0x80), a[i])));
            v[i + 8] = int8(std::min(int16(0x7f), std::max(int16(-0x80), b[i])));
        }
        return v;
    }

    static inline int16x8 narrow(int32x4 a, int32x4 b)
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
    // uint32
    // -----------------------------------------------------------------

    static inline uint32x4 get_low(uint32x8 a)
    {
        return a.lo;
    }

    static inline uint32x4 get_high(uint32x8 a)
    {
        return a.hi;
    }

    static inline uint32x8 set_low(uint32x8 a, uint32x4 low)
    {
        a.lo = low;
        return a;
    }

    static inline uint32x8 set_high(uint32x8 a, uint32x4 high)
    {
        a.hi = high;
        return a;
    }

    static inline uint32x8 combine(uint32x4 a, uint32x4 b)
    {
        uint32x8 v;
        v.lo = a;
        v.hi = b;
        return v;
    }

    // -----------------------------------------------------------------
    // int32
    // -----------------------------------------------------------------

    static inline int32x4 get_low(int32x8 a)
    {
        return a.lo;
    }

    static inline int32x4 get_high(int32x8 a)
    {
        return a.hi;
    }

    static inline int32x8 set_low(int32x8 a, int32x4 low)
    {
        a.lo = low;
        return a;
    }

    static inline int32x8 set_high(int32x8 a, int32x4 high)
    {
        a.hi = high;
        return a;
    }

    static inline int32x8 combine(int32x4 a, int32x4 b)
    {
        int32x8 v;
        v.lo = a;
        v.hi = b;
        return v;
    }

    // -----------------------------------------------------------------
    // float32
    // -----------------------------------------------------------------

    static inline float32x4 get_low(float32x8 a)
    {
        return a.lo;
    }

    static inline float32x4 get_high(float32x8 a)
    {
        return a.hi;
    }

    static inline float32x8 set_low(float32x8 a, float32x4 low)
    {
        a.lo = low;
        return a;
    }

    static inline float32x8 set_high(float32x8 a, float32x4 high)
    {
        a.hi = high;
        return a;
    }

    static inline float32x8 combine(float32x4 a, float32x4 b)
    {
        float32x8 v;
        v.lo = a;
        v.hi = b;
        return v;
    }

    // 128 bit convert

    template <>
    inline float32x4 convert<float32x4>(uint32x4 s)
    {
        float32x4 v;
        v[0] = float(s[0]);
        v[1] = float(s[1]);
        v[2] = float(s[2]);
        v[3] = float(s[3]);
        return v;
    }

    template <>
    inline float32x4 convert<float32x4>(int32x4 s)
    {
        float32x4 v;
        v[0] = float(s[0]);
        v[1] = float(s[1]);
        v[2] = float(s[2]);
        v[3] = float(s[3]);
        return v;
    }

    template <>
    inline uint32x4 convert<uint32x4>(float32x4 s)
    {
        uint32x4 v;
        v[0] = uint32(s[0] + 0.5f);
        v[1] = uint32(s[1] + 0.5f);
        v[2] = uint32(s[2] + 0.5f);
        v[3] = uint32(s[3] + 0.5f);
        return v;
    }

    template <>
    inline int32x4 convert<int32x4>(float32x4 s)
    {
        int32x4 v;
        v[0] = int32(s[0] + 0.5f);
        v[1] = int32(s[1] + 0.5f);
        v[2] = int32(s[2] + 0.5f);
        v[3] = int32(s[3] + 0.5f);
        return v;
    }

    template <>
    inline int32x4 truncate<int32x4>(float32x4 s)
    {
        int32x4 v;
        v[0] = int32(s[0]);
        v[1] = int32(s[1]);
        v[2] = int32(s[2]);
        v[3] = int32(s[3]);
        return v;
    }

    // 256 bit convert

    template <>
    inline int32x8 convert<int32x8>(float32x8 s)
    {
        int32x8 result;
        result.lo = convert<int32x4>(s.lo);
        result.hi = convert<int32x4>(s.hi);
        return result;
    }

    template <>
    inline float32x8 convert<float32x8>(int32x8 s)
    {
        float32x8 result;
        result.lo = convert<float32x4>(s.lo);
        result.hi = convert<float32x4>(s.hi);
        return result;
    }

    template <>
    inline uint32x8 convert<uint32x8>(float32x8 s)
    {
        uint32x8 result;
        result.lo = convert<uint32x4>(s.lo);
        result.hi = convert<uint32x4>(s.hi);
        return result;
    }

    template <>
    inline float32x8 convert<float32x8>(uint32x8 s)
    {
        float32x8 result;
        result.lo = convert<float32x4>(s.lo);
        result.hi = convert<float32x4>(s.hi);
        return result;
    }

    template <>
    inline int32x8 truncate<int32x8>(float32x8 s)
    {
        int32x8 result;
        result.lo = truncate<int32x4>(s.lo);
        result.hi = truncate<int32x4>(s.hi);
        return result;
    }

    // 512 bit convert

    template <>
    inline int32x16 convert<int32x16>(float32x16 s)
    {
        int32x16 result;
        result.lo = convert<int32x8>(s.lo);
        result.hi = convert<int32x8>(s.hi);
        return result;
    }

    template <>
    inline float32x16 convert<float32x16>(int32x16 s)
    {
        float32x16 result;
        result.lo = convert<float32x8>(s.lo);
        result.hi = convert<float32x8>(s.hi);
        return result;
    }

    template <>
    inline uint32x16 convert<uint32x16>(float32x16 s)
    {
        uint32x16 result;
        result.lo = convert<uint32x8>(s.lo);
        result.hi = convert<uint32x8>(s.hi);
        return result;
    }

    template <>
    inline float32x16 convert<float32x16>(uint32x16 s)
    {
        float32x16 result;
        result.lo = convert<float32x8>(s.lo);
        result.hi = convert<float32x8>(s.hi);
        return result;
    }

    template <>
    inline int32x16 truncate<int32x16>(float32x16 s)
    {
        int32x16 result;
        result.lo = truncate<int32x8>(s.lo);
        result.hi = truncate<int32x8>(s.hi);
        return result;
    }

    // -----------------------------------------------------------------
    // float64
    // -----------------------------------------------------------------

    static inline float64x2 get_low(float64x4 a)
    {
        return a.lo;
    }

    static inline float64x2 get_high(float64x4 a)
    {
        return a.hi;
    }

    static inline float64x4 set_low(float64x4 a, float64x2 low)
    {
        a.lo = low;
        return a;
    }

    static inline float64x4 set_high(float64x4 a, float64x2 high)
    {
        a.hi = high;
        return a;
    }

    static inline float64x4 combine(float64x2 a, float64x2 b)
    {
        float64x4 v;
        v.lo = a;
        v.hi = b;
        return v;
    }

    template <>
    inline float64x4 convert<float64x4>(int32x4 s)
    {
        float64x4 v;
        v.lo[0] = double(s[0]);
        v.lo[1] = double(s[1]);
        v.hi[0] = double(s[2]);
        v.hi[1] = double(s[3]);
        return v;
    }

    template <>
    inline float64x4 convert<float64x4>(float32x4 s)
    {
        float64x4 v;
        v.lo[0] = double(s[0]);
        v.lo[1] = double(s[1]);
        v.hi[0] = double(s[2]);
        v.hi[1] = double(s[3]);
        return v;
    }

    template <>
    inline int32x4 convert<int32x4>(float64x4 s)
    {
        int x = int(s.lo[0] + 0.5);
        int y = int(s.lo[1] + 0.5);
        int z = int(s.hi[0] + 0.5);
        int w = int(s.hi[1] + 0.5);
        return int32x4_set4(x, y, z, w);
    }

    template <>
    inline float32x4 convert<float32x4>(float64x4 s)
    {
        float x = float(s.lo[0]);
        float y = float(s.lo[1]);
        float z = float(s.hi[0]);
        float w = float(s.hi[1]);
        return float32x4_set4(x, y, z, w);
    }

    template <>
    inline float64x4 convert<float64x4>(uint32x4 u)
    {
        float64x4 v;
        v.lo[0] = u32_to_f64(u[0]);
        v.lo[1] = u32_to_f64(u[1]);
        v.hi[0] = u32_to_f64(u[2]);
        v.hi[1] = u32_to_f64(u[3]);
        return v;
    }

    template <>
    inline uint32x4 convert<uint32x4>(float64x4 d)
    {
        uint32 x = f64_to_u32(d.lo[0]);
        uint32 y = f64_to_u32(d.lo[1]);
        uint32 z = f64_to_u32(d.hi[0]);
        uint32 w = f64_to_u32(d.hi[1]);
        return uint32x4_set4(x, y, z, w);
    }

    template <>
    inline int32x4 truncate<int32x4>(float64x4 s)
    {
        int x = int(s.lo[0]);
        int y = int(s.lo[1]);
        int z = int(s.hi[0]);
        int w = int(s.hi[1]);
        return int32x4_set4(x, y, z, w);
    }

    template <>
    inline float64x4 convert<float64x4>(int64x4 v)
    {
        double x = double(get_component<0>(v));
        double y = double(get_component<1>(v));
        double z = double(get_component<2>(v));
        double w = double(get_component<3>(v));
        return float64x4_set4(x, y, z, w);
    }

    template <>
    inline int64x4 convert<int64x4>(float64x4 v)
    {
        int64 x = int64(get_component<0>(v));
        int64 y = int64(get_component<1>(v));
        int64 z = int64(get_component<2>(v));
        int64 w = int64(get_component<3>(v));
        return int64x4_set4(x, y, z, w);
    }

    // -----------------------------------------------------------------
    // float16
    // -----------------------------------------------------------------

    template <>
    inline float32x4 convert<float32x4>(float16x4 s)
    {
        float32x4 v;
        v[0] = s[0];
        v[1] = s[1];
        v[2] = s[2];
        v[3] = s[3];
        return v;
    }

    template <>
    inline float16x4 convert<float16x4>(float32x4 s)
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
