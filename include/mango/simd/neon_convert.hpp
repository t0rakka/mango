/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
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

		reinterpret_vector(s8x16 v)
            : data(vreinterpretq_u32_s8(v))
		{
		}

		reinterpret_vector(s16x8 v)
            : data(vreinterpretq_u32_s16(v))
		{
		}

		reinterpret_vector(s32x4 v)
            : data(vreinterpretq_u32_s32(v))
		{
		}

		reinterpret_vector(s64x2 v)
            : data(vreinterpretq_u32_s64(v))
		{
		}

		reinterpret_vector(u8x16 v)
            : data(vreinterpretq_u32_u8(v))
		{
		}

		reinterpret_vector(u16x8 v)
            : data(vreinterpretq_u32_u16(v))
		{
		}

		reinterpret_vector(u32x4 v)
            : data(v)
		{
		}

		reinterpret_vector(u64x2 v)
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

		operator s8x16 ()
		{
			return vreinterpretq_s8_u32(data);
		}

		operator s16x8 ()
		{
			return vreinterpretq_s16_u32(data);
		}

		operator s32x4 ()
		{
			return vreinterpretq_s32_u32(data);
		}

		operator s64x2 ()
		{
			return vreinterpretq_s64_u32(data);
		}

		operator u8x16 ()
		{
			return vreinterpretq_u8_u32(data);
		}

		operator u16x8 ()
		{
			return vreinterpretq_u16_u32(data);
		}

		operator u32x4 ()
		{
			return data;
		}

		operator u64x2 ()
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

    static inline u16x8 extend16x8(u8x16 s)
    {
        return vmovl_u8(vget_low_u8(s));
    }

    static inline u32x4 extend32x4(u8x16 s)
    {
        auto temp = vmovl_u8(vget_low_u8(s));
        return vmovl_u16(vget_low_u16(temp));
    }

    static inline u32x4 extend32x4(u16x8 s)
    {
        return vmovl_u16(vget_low_u16(s));
    }

    static inline u32x8 extend32x8(u16x8 s)
    {
        u32x8 v;
        v.lo = vmovl_u16(vget_low_u16(s));
        v.hi = vmovl_u16(vget_high_u16(s));
        return v;
    }

    // -----------------------------------------------------------------
    // sign extend
    // -----------------------------------------------------------------

    static inline s16x8 extend16x8(s8x16 s)
    {
        return vmovl_s8(vget_low_s8(s));
    }

    static inline s32x4 extend32x4(s8x16 s)
    {
        auto temp = vmovl_s8(vget_low_s8(s));
        return vmovl_s16(vget_low_s16(temp));
    }

    static inline s32x4 extend32x4(s16x8 s)
    {
        return vmovl_s16(vget_low_s16(s));
    }

    static inline s32x8 extend32x8(s16x8 s)
    {
        s32x8 v;
        v.lo = vmovl_s16(vget_low_s16(s));
        v.hi = vmovl_s16(vget_high_s16(s));
        return v;
    }

    // -----------------------------------------------------------------
    // narrow
    // -----------------------------------------------------------------

    static inline u8x16 narrow(u16x8 a, u16x8 b)
    {
        return vcombine_u8(vqmovn_u16(a), vqmovn_u16(b));
    }

    static inline u16x8 narrow(u32x4 a, u32x4 b)
    {
        return vcombine_u16(vqmovn_u32(a), vqmovn_u32(b));
    }

    static inline s8x16 narrow(s16x8 a, s16x8 b)
    {
        return vcombine_s8(vqmovn_s16(a), vqmovn_s16(b));
    }

    static inline s16x8 narrow(s32x4 a, s32x4 b)
    {
        return vcombine_s16(vqmovn_s32(a), vqmovn_s32(b));
    }

    // -----------------------------------------------------------------
    // u32
    // -----------------------------------------------------------------

    static inline u32x4 get_low(u32x8 a)
    {
        return a.lo;
    }

    static inline u32x4 get_high(u32x8 a)
    {
        return a.hi;
    }

    static inline u32x8 set_low(u32x8 a, u32x4 low)
    {
        a.lo = low;
        return a;
    }

    static inline u32x8 set_high(u32x8 a, u32x4 high)
    {
        a.hi = high;
        return a;
    }

    static inline u32x8 combine(u32x4 a, u32x4 b)
    {
        u32x8 v;
        v.lo = a;
        v.hi = b;
        return v;
    }

    // -----------------------------------------------------------------
    // s32
    // -----------------------------------------------------------------

    static inline s32x4 get_low(s32x8 a)
    {
        return a.lo;
    }

    static inline s32x4 get_high(s32x8 a)
    {
        return a.hi;
    }

    static inline s32x8 set_low(s32x8 a, s32x4 low)
    {
        a.lo = low;
        return a;
    }

    static inline s32x8 set_high(s32x8 a, s32x4 high)
    {
        a.hi = high;
        return a;
    }

    static inline s32x8 combine(s32x4 a, s32x4 b)
    {
        s32x8 v;
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
    inline float32x4 convert<float32x4>(u32x4 s)
    {
        return vcvtq_f32_u32(s);
    }

    template <>
    inline float32x4 convert<float32x4>(s32x4 s)
    {
        return vcvtq_f32_s32(s);
    }

#if __ARM_ARCH >= 8 //&& !defined(MANGO_COMPILER_CLANG)

    template <>
    inline u32x4 convert<u32x4>(float32x4 s)
    {
        return vcvtnq_u32_f32(s);
    }

    template <>
    inline s32x4 convert<s32x4>(float32x4 s)
    {
        return vcvtnq_s32_f32(s);
    }

#else

    template <>
    inline u32x4 convert<u32x4>(float32x4 s)
    {
        return vcvtq_u32_f32(round(s));
    }

    template <>
    inline s32x4 convert<s32x4>(float32x4 s)
    {
        return vcvtq_s32_f32(round(s));
    }

#endif

    template <>
    inline s32x4 truncate<s32x4>(float32x4 s)
    {
        return vcvtq_s32_f32(s);
    }

    // 256 bit convert

    template <>
    inline s32x8 convert<s32x8>(float32x8 s)
    {
        s32x8 result;
        result.lo = convert<s32x4>(s.lo);
        result.hi = convert<s32x4>(s.hi);
        return result;
    }

    template <>
    inline float32x8 convert<float32x8>(s32x8 s)
    {
        float32x8 result;
        result.lo = convert<float32x4>(s.lo);
        result.hi = convert<float32x4>(s.hi);
        return result;
    }

    template <>
    inline u32x8 convert<u32x8>(float32x8 s)
    {
        u32x8 result;
        result.lo = convert<u32x4>(s.lo);
        result.hi = convert<u32x4>(s.hi);
        return result;
    }

    template <>
    inline float32x8 convert<float32x8>(u32x8 s)
    {
        float32x8 result;
        result.lo = convert<float32x4>(s.lo);
        result.hi = convert<float32x4>(s.hi);
        return result;
    }

    template <>
    inline s32x8 truncate<s32x8>(float32x8 s)
    {
        s32x8 result;
        result.lo = truncate<s32x4>(s.lo);
        result.hi = truncate<s32x4>(s.hi);
        return result;
    }

    // 512 bit convert

    template <>
    inline s32x16 convert<s32x16>(float32x16 s)
    {
        s32x16 result;
        result.lo = convert<s32x8>(s.lo);
        result.hi = convert<s32x8>(s.hi);
        return result;
    }

    template <>
    inline float32x16 convert<float32x16>(s32x16 s)
    {
        float32x16 result;
        result.lo = convert<float32x8>(s.lo);
        result.hi = convert<float32x8>(s.hi);
        return result;
    }

    template <>
    inline u32x16 convert<u32x16>(float32x16 s)
    {
        u32x16 result;
        result.lo = convert<u32x8>(s.lo);
        result.hi = convert<u32x8>(s.hi);
        return result;
    }

    template <>
    inline float32x16 convert<float32x16>(u32x16 s)
    {
        float32x16 result;
        result.lo = convert<float32x8>(s.lo);
        result.hi = convert<float32x8>(s.hi);
        return result;
    }

    template <>
    inline s32x16 truncate<s32x16>(float32x16 s)
    {
        s32x16 result;
        result.lo = truncate<s32x8>(s.lo);
        result.hi = truncate<s32x8>(s.hi);
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
    inline float64x4 convert<float64x4>(s32x4 s)
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
    inline s32x4 convert<s32x4>(float64x4 s)
    {
        s32 x = s32(s.lo.data[0] + 0.5);
        s32 y = s32(s.lo.data[1] + 0.5);
        s32 z = s32(s.hi.data[0] + 0.5);
        s32 w = s32(s.hi.data[1] + 0.5);
        return s32x4_set4(x, y, z, w);
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
    inline float64x4 convert<float64x4>(u32x4 s)
    {
        double x = u32_to_f64(get_x(s));
        double y = u32_to_f64(get_y(s));
        double z = u32_to_f64(get_z(s));
        double w = u32_to_f64(get_w(s));
        return float64x4_set4(x, y, z, w);
    }

    template <>
    inline u32x4 convert<u32x4>(float64x4 d)
    {
        u32 x = f64_to_u32(d.lo.data[0]);
        u32 y = f64_to_u32(d.lo.data[1]);
        u32 z = f64_to_u32(d.hi.data[0]);
        u32 w = f64_to_u32(d.hi.data[1]);
        return u32x4_set4(x, y, z, w);
    }

    template <>
    inline s32x4 truncate<s32x4>(float64x4 s)
    {
        s32 x = s32(s.lo.data[0]);
        s32 y = s32(s.lo.data[1]);
        s32 z = s32(s.hi.data[0]);
        s32 w = s32(s.hi.data[1]);
        return s32x4_set4(x, y, z, w);
    }

    template <>
    inline float64x4 convert<float64x4>(s64x4 v)
    {
        double x = double(get_component<0>(v));
        double y = double(get_component<1>(v));
        double z = double(get_component<2>(v));
        double w = double(get_component<3>(v));
        return float64x4_set4(x, y, z, w);
    }

    template <>
    inline s64x4 convert<s64x4>(float64x4 v)
    {
        s64 x = s64(get_component<0>(v));
        s64 y = s64(get_component<1>(v));
        s64 z = s64(get_component<2>(v));
        s64 w = s64(get_component<3>(v));
        return s64x4_set4(x, y, z, w);
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
