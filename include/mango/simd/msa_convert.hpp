/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        uint32x4::vector data;

        reinterpret_vector() = default;

		reinterpret_vector(int8x16 v)
            : data((uint32x4::vector) v.data)
		{
		}

		reinterpret_vector(int16x8 v)
            : data((uint32x4::vector) v.data)
		{
		}

		reinterpret_vector(int32x4 v)
            : data((uint32x4::vector) v.data)
		{
		}

		reinterpret_vector(int64x2 v)
            : data((uint32x4::vector) v.data)
		{
		}

		reinterpret_vector(uint8x16 v)
            : data((uint32x4::vector) v.data)
		{
		}

		reinterpret_vector(uint16x8 v)
            : data((uint32x4::vector) v.data)
		{
		}

		reinterpret_vector(uint32x4 v)
            : data(v.data)
		{
		}

		reinterpret_vector(uint64x2 v)
            : data((uint32x4::vector) v.data)
		{
		}

		reinterpret_vector(float32x4 v)
            : data((uint32x4::vector) v.data)
		{
		}

		reinterpret_vector(float64x2 v)
		{
            std::memcpy(&data, &v, 16);
		}

		operator int8x16 ()
		{
            return (int8x16::vector) data;
		}

		operator int16x8 ()
		{
            return (int16x8::vector) data;
		}

		operator int32x4 ()
		{
            return (int32x4::vector) data;
		}

		operator int64x2 ()
		{
            return (int64x2::vector) data;
		}

		operator uint8x16 ()
		{
            return (uint8x16::vector) data;
		}

		operator uint16x8 ()
		{
            return (uint16x8::vector) data;
		}

		operator uint32x4 ()
		{
			return data;
		}

		operator uint64x2 ()
		{
            return (uint64x2::vector) data;
		}

		operator float32x4 ()
		{
            return (float32x4::vector) data;
		}

		operator float64x2 ()
		{
            return (float64x2::vector) data;
		}
	};

	template <>
	struct reinterpret_vector<256>
	{
        reinterpret_vector<128> lo;
        reinterpret_vector<128> hi;

	    template <typename T>
	    reinterpret_vector(composite_vector<T> v)
            : lo(v.lo)
            , hi(v.hi)
	    {
	    }

		template <typename T>
		operator composite_vector<T> ()
		{
            return composite_vector<T>(lo, hi);
		}
	};

	template <>
	struct reinterpret_vector<512>
	{
        reinterpret_vector<256> lo;
        reinterpret_vector<256> hi;

	    template <typename T>
	    reinterpret_vector(composite_vector<T> v)
            : lo(v.lo)
            , hi(v.hi)
	    {
	    }

		template <typename T>
		operator composite_vector<T> ()
		{
            return composite_vector<T>(lo, hi);
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

    static inline uint16x8 extend16x8(uint8x16 s)
    {
        return (v16u8) __msa_ilvr_b(__msa_fill_b(0), (v16i8)s);
    }

    static inline uint32x4 extend32x4(uint8x16 s)
    {
        v16u8 temp16 = __msa_ilvr_b(__msa_fill_b(0), (v16i8)s);
        return (v4u32) __msa_ilvr_h(__msa_fill_h(0), (v8i16)temp16);
    }

    static inline uint32x4 extend32x4(uint16x8 s)
    {
        return (v4u32) __msa_ilvr_h(__msa_fill_h(0), (v8i16)s);
    }

    static inline uint32x8 extend32x8(uint16x8 s)
    {
        uint16x8 s_high = (v8u16) __msa_ilvl_d((v2i64)s, (v2i64)s);
        uint32x8 v;
        v.lo = extend32x4(s);
        v.hi = extend32x4(s_high);
        return v;
    }

    // -----------------------------------------------------------------
    // sign extend
    // -----------------------------------------------------------------

    static inline int16x8 extend16x8(int8x16 s)
    {
        v16i8 sign = __msa_clt_s_b(s, __msa_fill_b(0));
        return (v16i8) __msa_ilvr_b(sign, (v16i8)s);
    }

    static inline int32x4 extend32x4(int8x16 s)
    {
        v16i8 temp16 = __msa_ilvr_b(__msa_clt_s_b(s, __msa_fill_b(0)), (v16i8)s);
        return (v4i32) __msa_ilvr_h(__msa_clt_s_h(s, __msa_fill_h(0)), (v8i16)temp16);
    }

    static inline int32x4 extend32x4(int16x8 s)
    {
        v8i16 sign = __msa_clt_s_h(s, __msa_fill_h(0));
        return (v4i32) __msa_ilvr_h(sign, (v8i16)s);
    }

    static inline int32x8 extend32x8(int16x8 s)
    {
        int16x8 s_high = (v8i16) __msa_ilvl_d((v2i64)s, (v2i64)s);
        int32x8 v;
        v.lo = extend32x4(s);
        v.hi = extend32x4(s_high);
        return v;
    }

    // -----------------------------------------------------------------
    // narrow
    // -----------------------------------------------------------------

    static inline uint8x16 narrow(uint16x8 a, uint16x8 b)
    {
        const v16i8 control = (v16i8) { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 };
        return (v16u8) __msa_vshf_b(control, (v16i8) a, (v16i8) b);
    }

    static inline uint16x8 narrow(uint32x4 a, uint32x4 b)
    {
        const v8i16 control = (v8i16) { 0, 2, 4, 6, 8, 10, 12, 14 };
        return (v8u16) __msa_vshf_h(control, (v8i16) a, (v8i16) b);
    }

    static inline int8x16 narrow(int16x8 a, int16x8 b)
    {
        const v16i8 control = (v16i8) { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 };
        return (v16i8) __msa_vshf_b(control, (v16i8) a, (v16i8) b);
    }

    static inline int16x8 narrow(int32x4 a, int32x4 b)
    {
        const v8i16 control = (v8i16) { 0, 2, 4, 6, 8, 10, 12, 14 };
        return (v8i16) __msa_vshf_h(control, (v8i16) a, (v8i16) b);
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
        return __msa_ffint_u_w(s);
    }

    template <>
    inline float32x4 convert<float32x4>(int32x4 s)
    {
        return __msa_ffint_s_w(s);
    }

    template <>
    inline uint32x4 convert<uint32x4>(float32x4 s)
    {
        return __msa_ftint_u_w(s);
    }

    template <>
    inline int32x4 convert<int32x4>(float32x4 s)
    {
        return __msa_ftint_s_w(s);
    }

    template <>
    inline int32x4 truncate<int32x4>(float32x4 s)
    {
        return __msa_ftrunc_s_w(s);
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
        float64x4 result;
        result.lo = a;
        result.hi = b;
        return result;
    }

    template <>
    inline float64x4 convert<float64x4>(int32x4 s)
    {
        double x = double(get_component<0>(s));
        double y = double(get_component<1>(s));
        double z = double(get_component<2>(s));
        double w = double(get_component<3>(s));
        return float64x4_set4(x, y, z, w);
    }

    template <>
    inline float64x4 convert<float64x4>(float32x4 s)
    {
        double x = double(get_component<0>(s));
        double y = double(get_component<1>(s));
        double z = double(get_component<2>(s));
        double w = double(get_component<3>(s));
        return float64x4_set4(x, y, z, w);
    }

    template <>
    inline int32x4 convert<int32x4>(float64x4 s)
    {
        int x = int(get_component<0>(s));
        int y = int(get_component<1>(s));
        int z = int(get_component<2>(s));
        int w = int(get_component<3>(s));
        return int32x4_set4(x, y, z, w);
    }

    template <>
    inline float32x4 convert<float32x4>(float64x4 s)
    {
        float x = float(get_component<0>(s));
        float y = float(get_component<1>(s));
        float z = float(get_component<2>(s));
        float w = float(get_component<3>(s));
        return float32x4_set4(x, y, z, w);
    }

    template <>
    inline float64x4 convert<float64x4>(uint32x4 ui)
    {
        double x = double(get_component<0>(ui));
        double y = double(get_component<1>(ui));
        double z = double(get_component<2>(ui));
        double w = double(get_component<3>(ui));
        return float64x4_set4(x, y, z, w);
    }

    template <>
    inline uint32x4 convert<uint32x4>(float64x4 d)
    {
        uint32 x = uint32(get_component<0>(d));
        uint32 y = uint32(get_component<1>(d));
        uint32 z = uint32(get_component<2>(d));
        uint32 w = uint32(get_component<3>(d));
        return uint32x4_set4(x, y, z, w);
    }

    template <>
    inline int32x4 truncate<int32x4>(float64x4 s)
    {
        int64x2 lo = __msa_ftrunc_s_d(s.lo);
        int64x2 hi = __msa_ftrunc_s_d(s.hi);
        int x = int(get_component<0>(lo));
        int y = int(get_component<1>(lo));
        int z = int(get_component<0>(hi));
        int w = int(get_component<1>(hi));
        return int32x4_set4(x, y, z, w);
    }

    // 256 <- 256

    template <>
    inline float64x4 convert<float64x4>(int64x4 v)
    {
        float64x4 temp;
        temp.lo = __msa_ffint_s_d(v.lo);
        temp.hi = __msa_ffint_s_d(v.hi);
        return temp;
    }

    template <>
    inline int64x4 convert<int64x4>(float64x4 v)
    {
        int64x4 temp;
        temp.lo = __msa_ftint_s_d(v.lo);
        temp.hi = __msa_ftint_s_d(v.hi);
        return temp;
    }

    // -----------------------------------------------------------------
    // float16
    // -----------------------------------------------------------------

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

} // namespace simd
} // namespace mango
