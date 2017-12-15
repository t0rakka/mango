/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"
#include "common.hpp"

namespace mango {
namespace simd {
namespace detail {

	template <int bits>
	struct reinterpret_vector;

	template <>
	struct reinterpret_vector<128>
	{
        uint32x4_t data;

        reinterpret_vector() = default;

		reinterpret_vector(int8x16 v)
            : data(vreinterpretq_u32_s8(v))
		{
		}

		reinterpret_vector(int16x8 v)
            : data(vreinterpretq_u32_s16(v))
		{
		}

		reinterpret_vector(int32x4 v)
            : data(vreinterpretq_u32_s32(v))
		{
		}

		reinterpret_vector(int64x2 v)
            : data(vreinterpretq_u32_s64(v))
		{
		}

		reinterpret_vector(uint8x16 v)
            : data(vreinterpretq_u32_u8(v))
		{
		}

		reinterpret_vector(uint16x8 v)
            : data(vreinterpretq_u32_u16(v))
		{
		}

		reinterpret_vector(uint32x4 v)
            : data(v)
		{
		}

		reinterpret_vector(uint64x2 v)
            : data(vreinterpretq_u32_u64(v))
		{
		}

		reinterpret_vector(float32x4 v)
            : data(vreinterpretq_u32_f32(v))
		{
		}

		reinterpret_vector(float64x2 v)
		{
            std::memcpy(&data, &v, 16);
		}

		operator int8x16 ()
		{
			return vreinterpretq_s8_u32(data);
		}

		operator int16x8 ()
		{
			return vreinterpretq_s16_u32(data);
		}

		operator int32x4 ()
		{
			return vreinterpretq_s32_u32(data);
		}

		operator int64x2 ()
		{
			return vreinterpretq_s64_u32(data);
		}

		operator uint8x16 ()
		{
			return vreinterpretq_u8_u32(data);
		}

		operator uint16x8 ()
		{
			return vreinterpretq_u16_u32(data);
		}

		operator uint32x4 ()
		{
			return data;
		}

		operator uint64x2 ()
		{
			return vreinterpretq_u64_u32(data);
		}

		operator float32x4 ()
		{
			return vreinterpretq_f32_u32(data);
		}

		operator float64x2 ()
		{
            float64x2 temp;
            std::memcpy(&temp, &data, 16);
            return temp;
		}
	};

	template <>
	struct reinterpret_vector<256>
	{
        reinterpret_vector<128> lo;
        reinterpret_vector<128> hi;

        reinterpret_vector() = default;

	    template <typename T>
	    reinterpret_vector(composite_vector<T> v)
            : lo(v.lo)
            , hi(v.hi)
	    {
	    }

		reinterpret_vector(float64x4 v)
		{
            std::memcpy(this, &v, 32);
		}

		template <typename T>
		operator composite_vector<T> ()
		{
            return composite_vector<T>(lo, hi);
		}

		operator float64x4 ()
		{
            float64x4 temp;
            std::memcpy(&temp, this, 32);
            return temp;
		}
	};

	template <>
	struct reinterpret_vector<512>
	{
        reinterpret_vector<256> lo;
        reinterpret_vector<256> hi;

        reinterpret_vector() = default;

	    template <typename T>
	    reinterpret_vector(composite_vector<T> v)
            : lo(v.lo)
            , hi(v.hi)
	    {
	    }

		reinterpret_vector(float64x8 v)
		{
            std::memcpy(this, &v, 64);
		}

		template <typename T>
		operator composite_vector<T> ()
		{
            return composite_vector<T>(lo, hi);
		}

		operator float64x8 ()
		{
            float64x8 temp;
            std::memcpy(&temp, this, 64);
            return temp;
		}
	};

} // namespace detail

    // -----------------------------------------------------------------
    // reinterpret
    // -----------------------------------------------------------------

	template <typename D, typename S0, int S1, typename S2>
	inline D reinterpret(hardware_vector<S0, S1, S2> s)
	{
        static_assert(sizeof(hardware_vector<S0, S1, S2>) == sizeof(D), "Vectors must be same size.");
		return D(detail::reinterpret_vector<hardware_vector<S0, S1, S2>::vector_bits>(s));
	}

	template <typename D, typename S0, int S1>
	inline D reinterpret(scalar_vector<S0, S1> s)
	{
        static_assert(sizeof(scalar_vector<S0, S1>) == sizeof(D), "Vectors must be same size.");
		return D(detail::reinterpret_vector<scalar_vector<S0, S1>::vector_bits>(s));
	}

	template <typename D, typename S>
	inline D reinterpret(composite_vector<S> s)
	{
        static_assert(sizeof(composite_vector<S>) == sizeof(D), "Vectors must be same size.");
		return D(detail::reinterpret_vector<composite_vector<S>::vector_bits>(s));
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
	    const uint8x8x2_t a = vzip_u8(vget_low_u8(s), vdup_n_u8(0));
        return vreinterpretq_u16_u8(vcombine_u8(a.val[0], a.val[1]));
    }

    static inline uint32x4 extend32(uint8x16 s)
    {
	    const uint8x8x2_t a = vzip_u8(vget_low_u8(s), vdup_n_u8(0));
	    const uint16x4x2_t b = vzip_u16(vreinterpret_u16_u8(a.val[0]), vdup_n_u16(0));
	    return vreinterpretq_u32_u16(vcombine_u16(b.val[0], b.val[1]));
    }

    static inline uint32x4 extend32(uint16x8 s)
    {
	    const uint16x4x2_t a = vzip_u16(vget_low_u16(s), vdup_n_u16(0));
	    return vreinterpretq_u32_u16(vcombine_u16(a.val[0], a.val[1]));
    }

    // -----------------------------------------------------------------
    // sign extend
    // -----------------------------------------------------------------

    static inline int16x8 extend16(int8x16 s)
    {
        const int8x8_t low = vget_low_s8(s);
        const int8x8_t sign = vreinterpret_s8_u8(vcgt_s8(vdup_n_s8(0), low));
	    const int8x8x2_t temp = vzip_s8(low, sign);
        return vreinterpretq_s16_s8(vcombine_s8(temp.val[0], temp.val[1]));
    }

    static inline int32x4 extend32(int8x16 s)
    {
        const int8x8x2_t a = vzip_s8(vget_low_s8(s), vdup_n_s8(0));
        const int16x4x2_t b = vzip_s16(vreinterpret_s16_s8(a.val[0]), vdup_n_s16(0));
        const int32x4_t temp = vreinterpretq_s32_s16(vcombine_s16(b.val[0], b.val[1]));
        const int32x4_t sign = vdupq_n_s32(0x80);
        return vsubq_s32(veorq_s32(temp, sign), sign);
    }

    static inline int32x4 extend32(int16x8 s)
    {
        const int16x4_t low = vget_low_s16(s);
        const int16x4_t sign = vreinterpret_s16_u16(vcgt_s16(vdup_n_s16(0), low));
	    const int16x4x2_t temp = vzip_s16(low, sign);
	    return vreinterpretq_s32_s16(vcombine_s16(temp.val[0], temp.val[1]));
    }

    // -----------------------------------------------------------------
    // pack
    // -----------------------------------------------------------------

    static inline uint8x16 pack(uint16x8 a, uint16x8 b)
    {
        return vcombine_u8(vqmovn_u16(a), vqmovn_u16(b));
    }

    static inline uint16x8 pack(uint32x4 a, uint32x4 b)
    {
        return vcombine_u16(vqmovn_u32(a), vqmovn_u32(b));
    }

    static inline int8x16 pack(int16x8 a, int16x8 b)
    {
        return vcombine_s8(vqmovn_s16(a), vqmovn_s16(b));
    }

    static inline int16x8 pack(int32x4 a, int32x4 b)
    {
        return vcombine_s16(vqmovn_s32(a), vqmovn_s32(b));
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
        float32x8 result;
        result.lo = a;
        result.hi = b;
        return result;
    }

    // 128 bit convert

    template <>
    inline float32x4 convert<float32x4>(uint32x4 s)
    {
        return vcvtq_f32_u32(s);
    }

    template <>
    inline float32x4 convert<float32x4>(int32x4 s)
    {
        return vcvtq_f32_s32(s);
    }

#if __ARM_ARCH >= 8 //&& !defined(MANGO_COMPILER_CLANG)

    template <>
    inline uint32x4 convert<uint32x4>(float32x4 s)
    {
        return vcvtnq_u32_f32(s);
    }

    template <>
    inline int32x4 convert<int32x4>(float32x4 s)
    {
        return vcvtnq_s32_f32(s);
    }

#else

    template <>
    inline uint32x4 convert<uint32x4>(float32x4 s)
    {
        return vcvtq_u32_f32(round(s));
    }

    template <>
    inline int32x4 convert<int32x4>(float32x4 s)
    {
        return vcvtq_s32_f32(round(s));
    }

#endif

    template <>
    inline int32x4 truncate<int32x4>(float32x4 s)
    {
        return vcvtq_s32_f32(s);
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
        double x = a.data[0];
        double y = a.data[1];
        double z = b.data[0];
        double w = b.data[1];
        return float64x4_set4(x, y, z, w);
    }

    template <>
    inline float64x4 convert<float64x4>(int32x4 s)
    {
        double x = double(get_x(s));
        double y = double(get_y(s));
        double z = double(get_z(s));
        double w = double(get_w(s));
        return float64x4_set4(x, y, z, w);
    }

    template <>
    inline float64x4 convert<float64x4>(float32x4 s)
    {
        double x = double(get_x(s));
        double y = double(get_y(s));
        double z = double(get_z(s));
        double w = double(get_w(s));
        return float64x4_set4(x, y, z, w);
    }

    template <>
    inline int32x4 convert<int32x4>(float64x4 s)
    {
        int x = int(s.lo.data[0] + 0.5);
        int y = int(s.lo.data[1] + 0.5);
        int z = int(s.hi.data[0] + 0.5);
        int w = int(s.hi.data[1] + 0.5);
        return int32x4_set4(x, y, z, w);
    }

    template <>
    inline float32x4 convert<float32x4>(float64x4 s)
    {
        float x = float(s.lo.data[0]);
        float y = float(s.lo.data[1]);
        float z = float(s.hi.data[0]);
        float w = float(s.hi.data[1]);
        return float32x4_set4(x, y, z, w);
    }

    template <>
    inline float64x4 convert<float64x4>(uint32x4 s)
    {
        double x = u32_to_f64(get_x(s));
        double y = u32_to_f64(get_y(s));
        double z = u32_to_f64(get_z(s));
        double w = u32_to_f64(get_w(s));
        return float64x4_set4(x, y, z, w);
    }

    template <>
    inline uint32x4 convert<uint32x4>(float64x4 d)
    {
        uint32 x = f64_to_u32(d.lo.data[0]);
        uint32 y = f64_to_u32(d.lo.data[1]);
        uint32 z = f64_to_u32(d.hi.data[0]);
        uint32 w = f64_to_u32(d.hi.data[1]);
        return uint32x4_set4(x, y, z, w);
    }

    template <>
    inline int32x4 truncate<int32x4>(float64x4 s)
    {
        int x = int(s.lo.data[0]);
        int y = int(s.lo.data[1]);
        int z = int(s.hi.data[0]);
        int w = int(s.hi.data[1]);
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

#ifdef MANGO_ENABLE_FP16

    template <>
    inline float32x4 convert<float32x4>(float16x4 s)
    {
        return vcvt_f32_f16(s);
    }

    template <>
    inline float16x4 convert<float16x4>(float32x4 s)
    {
        return vcvt_f16_f32(s);
    }

#else

    template <>
    inline float32x4 convert<float32x4>(float16x4 s)
    {
        float x = s[0];
        float y = s[1];
        float z = s[2];
        float w = s[3];
        return float32x4_set4(x, y, z, w);
    }

    template <>
    inline float16x4 convert<float16x4>(float32x4 s)
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
