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
		__m128 data;

		reinterpret_vector(__m128 data)
		    : data(data)
		{
		}

		reinterpret_vector(__m128i data)
		    : data(_mm_castsi128_ps(data))
		{
		}

		reinterpret_vector(__m128d data)
		    : data(_mm_castpd_ps(data))
		{
		}

		template <typename ScalarType, int VectorSize, typename VectorType>
		operator hardware_vector<ScalarType, VectorSize, VectorType> ()
		{
			return hardware_vector<ScalarType, VectorSize, VectorType>(data);
		}

		template <typename ScalarType, int VectorSize>
		operator hardware_vector<ScalarType, VectorSize, __m128i> ()
		{
			return hardware_vector<ScalarType, VectorSize, __m128i>(_mm_castps_si128(data));
		}

		template <typename ScalarType, int VectorSize>
		operator hardware_vector<ScalarType, VectorSize, __m128d> ()
		{
			return hardware_vector<ScalarType, VectorSize, __m128d>(_mm_castps_pd(data));
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

#if defined(MANGO_ENABLE_SSE4_1)

    static inline uint16x8 extend16x8(uint8x16 s)
    {
        return _mm_cvtepu8_epi16(s);
    }

    static inline uint32x4 extend32x4(uint8x16 s)
    {
        return _mm_cvtepu8_epi32(s);
    }

    static inline uint32x4 extend32x4(uint16x8 s)
    {
        return _mm_cvtepu16_epi32(s);
    }

#else

    static inline uint16x8 extend16x8(uint8x16 s)
    {
        return _mm_unpacklo_epi8(s, _mm_setzero_si128());
    }

    static inline uint32x4 extend32x4(uint8x16 s)
    {
        const __m128i temp = _mm_unpacklo_epi8(s, _mm_setzero_si128());
        return _mm_unpacklo_epi16(temp, _mm_setzero_si128());
    }

    static inline uint32x4 extend32x4(uint16x8 s)
    {
        return _mm_unpacklo_epi16(s, _mm_setzero_si128());
    }

#endif

    static inline uint32x8 extend32x8(uint16x8 s)
    {
        uint16x8 s_high = _mm_unpackhi_epi64(s, s);
        uint32x8 v;
        v.lo = extend32x4(s);
        v.hi = extend32x4(s_high);
        return v;
    }

    // -----------------------------------------------------------------
    // sign extend
    // -----------------------------------------------------------------

#if defined(MANGO_ENABLE_SSE4_1)

    static inline int16x8 extend16x8(int8x16 s)
    {
        return _mm_cvtepi8_epi16(s);
    }

    static inline int32x4 extend32x4(int8x16 s)
    {
        return _mm_cvtepi8_epi32(s);
    }

    static inline int32x4 extend32x4(int16x8 s)
    {
        return _mm_cvtepi16_epi32(s);
    }

#else

    static inline int16x8 extend16x8(int8x16 s)
    {
        const __m128i sign = _mm_cmpgt_epi8(_mm_setzero_si128(), s);
        return _mm_unpacklo_epi8(s, sign);
    }

    static inline int32x4 extend32x4(int8x16 s)
    {
        const __m128i temp = _mm_unpacklo_epi8(s, _mm_cmpgt_epi8(_mm_setzero_si128(), s));
        return _mm_unpacklo_epi16(temp, _mm_cmpgt_epi16(_mm_setzero_si128(), temp));
    }

    static inline int32x4 extend32x4(int16x8 s)
    {
        const __m128i sign = _mm_cmpgt_epi16(_mm_setzero_si128(), s);
        return _mm_unpacklo_epi16(s, sign);
    }

#endif

    static inline int32x8 extend32x8(int16x8 s)
    {
        int16x8 s_high = _mm_unpackhi_epi64(s, s);
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
        return _mm_packus_epi16(a, b);
    }

    static inline uint16x8 narrow(uint32x4 a, uint32x4 b)
    {
        return simd128_packus_epi32(a, b);
    }

    static inline int8x16 narrow(int16x8 a, int16x8 b)
    {
        return _mm_packs_epi16(a, b);
    }

    static inline int16x8 narrow(int32x4 a, int32x4 b)
    {
        return _mm_packs_epi32(a, b);
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
        // conversion could be done by subtracting 0x80000000 from the value before signed conversion and
        // adding float(0x80000000) to the result after conversion but this would reduce precision on the LSBs.
        const __m128i mask = _mm_set1_epi32(0x0000ffff);
        const __m128i onep39 = _mm_set1_epi32(0x53000000);
        const __m128i x0 = _mm_or_si128(_mm_srli_epi32(s, 16), onep39);
        const __m128i x1 = _mm_and_si128(s, mask);
        const __m128 f1 = _mm_cvtepi32_ps(x1);
        const __m128 f0 = _mm_sub_ps(_mm_castsi128_ps(x0), _mm_castsi128_ps(onep39));
        return _mm_add_ps(f0, f1);
    }

    template <>
    inline float32x4 convert<float32x4>(int32x4 s)
    {
        return _mm_cvtepi32_ps(s);
    }

    template <>
    inline uint32x4 convert<uint32x4>(float32x4 s)
    {
        // conversion could be done by subtracting float(0x80000000) from the value before signed conversion and
        // adding 0x80000000 to the result after conversion but this would reduce precision on the LSBs.
	    __m128 x2 = _mm_castsi128_ps(_mm_set1_epi32(0x4f000000));
	    __m128 x1 = _mm_cmple_ps(x2, s);
  	    __m128i x0 = _mm_cvtps_epi32(_mm_sub_ps(s, _mm_and_ps(x2, x1)));
  	    return _mm_or_si128(x0, _mm_slli_epi32(_mm_castps_si128(x1), 31));
    }

    template <>
    inline int32x4 convert<int32x4>(float32x4 s)
    {
        return _mm_cvtps_epi32(s);
    }

    template <>
    inline int32x4 truncate<int32x4>(float32x4 s)
    {
        return _mm_cvttps_epi32(s);
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
        float64x4 result;
        result.lo = _mm_cvtepi32_pd(s);
        result.hi = _mm_cvtepi32_pd(_mm_shuffle_epi32(s, 0xee));
        return result;
    }

    template <>
    inline float64x4 convert<float64x4>(float32x4 s)
    {
        float64x4 result;
        result.lo = _mm_cvtps_pd(s);
        result.hi = _mm_cvtps_pd(_mm_shuffle_ps(s, s, 0xee));
        return result;
    }

    template <>
    inline int32x4 convert<int32x4>(float64x4 s)
    {
        __m128i xy = _mm_cvtpd_epi32(s.lo);
        __m128i zw = _mm_cvtpd_epi32(s.hi);
        __m128i xzyw = _mm_unpacklo_epi32(xy, zw);
        return _mm_shuffle_epi32(xzyw, 0xd8);
    }

    template <>
    inline float32x4 convert<float32x4>(float64x4 s)
    {
        __m128 xy00 = _mm_cvtpd_ps(s.lo);
        __m128 zw00 = _mm_cvtpd_ps(s.hi);
        return _mm_shuffle_ps(xy00, zw00, 0x44);
    }

    template <>
    inline float64x4 convert<float64x4>(uint32x4 ui)
    {
        const __m128d bias = _mm_set1_pd((1ll << 52) * 1.5);
        const __m128i mask = _mm_set1_epi32(0x43380000);
        __m128i xy = _mm_unpacklo_epi32(ui, mask);
        __m128i zw = _mm_unpackhi_epi32(ui, mask);
        float64x4 result;
        result.lo = _mm_sub_pd(_mm_castsi128_pd(xy), bias);
        result.hi = _mm_sub_pd(_mm_castsi128_pd(zw), bias);
        return result;
    }

    template <>
    inline uint32x4 convert<uint32x4>(float64x4 d)
    {
        const __m128d bias = _mm_set1_pd((1ll << 52) * 1.5);
        __m128 xy = _mm_castpd_ps(_mm_add_pd(d.lo, bias));
        __m128 zw = _mm_castpd_ps(_mm_add_pd(d.hi, bias));
        __m128 u = _mm_shuffle_ps(xy, zw, 0x88);
        return _mm_castps_si128(u);
    }

    template <>
    inline int32x4 truncate<int32x4>(float64x4 s)
    {
        __m128i xy = _mm_cvttpd_epi32(s.lo);
        __m128i zw = _mm_cvttpd_epi32(s.hi);
        __m128i xzyw = _mm_unpacklo_epi32(xy, zw);
        return _mm_shuffle_epi32(xzyw, 0xd8);
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

#ifdef MANGO_ENABLE_F16C

    template <>
    inline float32x4 convert<float32x4>(float16x4 h)
    {
        const __m128i* p = reinterpret_cast<const __m128i *>(&h);
        return _mm_cvtph_ps(_mm_loadl_epi64(p));
    }

    template <>
    inline float16x4 convert<float16x4>(float32x4 f)
    {
        float16x4 h;
        __m128i* p = reinterpret_cast<__m128i *>(&h);
        _mm_storel_epi64(p, _mm_cvtps_ph(f, 0));
        return h;
    }

#else

    template <>
    inline float32x4 convert<float32x4>(float16x4 h)
    {
        const __m128i* p = reinterpret_cast<const __m128i *>(&h);
        const int32x4 u = _mm_unpacklo_epi16(_mm_loadl_epi64(p), _mm_setzero_si128());

        int32x4 no_sign  = bitwise_and(u, int32x4_set1(0x7fff));
        int32x4 sign     = bitwise_and(u, int32x4_set1(0x8000));
        int32x4 exponent = bitwise_and(u, int32x4_set1(0x7c00));
        int32x4 mantissa = bitwise_and(u, int32x4_set1(0x03ff));

        // NaN or Inf
        int32x4 a = bitwise_or(int32x4_set1(0x7f800000), slli(mantissa, 13));

        // Zero or Denormal
        const int32x4 magic = int32x4_set1(0x3f000000);
        int32x4 b;
        b = add(magic, mantissa);
        b = reinterpret<int32x4>(sub(reinterpret<float32x4>(b), reinterpret<float32x4>(magic)));

        // Numeric Value
        int32x4 c = add(int32x4_set1(0x38000000), slli(no_sign, 13));

        // Select a, b, or c based on exponent
        mask32x4 mask;
        int32x4 result;

        mask = compare_eq(exponent, int32x4_zero());
        result = select(mask, b, c);

        mask = compare_eq(exponent, int32x4_set1(0x7c00));
        result = select(mask, a, result);

        // Sign
        result = bitwise_or(result, slli(sign, 16));

        return reinterpret<float32x4>(result);
    }

    template <>
    inline float16x4 convert<float16x4>(float32x4 f)
    {
        const float32x4 magic = float32x4_set1(Float(0, 15, 0).f);
        const int32x4 vinf = int32x4_set1(31 << 23);

        const int32x4 u = reinterpret<int32x4>(f);
        const int32x4 sign = srli(bitwise_and(u, int32x4_set1(0x80000000)), 16);

        const int32x4 vexponent = int32x4_set1(0x7f800000);

        // Inf / NaN
        const mask32x4 s0 = compare_eq(bitwise_and(u, vexponent), vexponent);
        int32x4 mantissa = bitwise_and(u, int32x4_set1(0x007fffff));
        mask32x4 x0 = compare_eq(mantissa, int32x4_zero());
        mantissa = select(x0, int32x4_zero(), srai(mantissa, 13));
        const int32x4 v0 = bitwise_or(int32x4_set1(0x7c00), mantissa);

        int32x4 v1 = bitwise_and(u, int32x4_set1(0x7ffff000));
        v1 = reinterpret<int32x4>(mul(reinterpret<float32x4>(v1), magic));
        v1 = add(v1, int32x4_set1(0x1000));

#if defined(MANGO_ENABLE_SSE4_1)
        v1 = _mm_min_epi32(v1, vinf);
        v1 = srai(v1, 13);

        int32x4 v = select(s0, v0, v1);
        v = bitwise_or(v, sign);
        v = _mm_packus_epi32(v, v);
#else
        v1 = select(compare_gt(v1, vinf), vinf, v1);
        v1 = srai(v1, 13);

        int32x4 v = select(s0, v0, v1);
        v = bitwise_or(v, sign);
        v = _mm_slli_epi32 (v, 16);
        v = _mm_srai_epi32 (v, 16);
        v = _mm_packs_epi32 (v, v);
#endif

        float16x4 h;
        _mm_storel_epi64(reinterpret_cast<__m128i *>(&h), v);
        return h;
    }

#endif // MANGO_ENABLE_F16C

} // namespace simd
} // namespace mango
