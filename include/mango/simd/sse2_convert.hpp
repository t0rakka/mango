/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/simd/simd.hpp>

namespace mango::simd::detail
{

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
            : lo(v.data[0])
            , hi(v.data[1])
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
            : lo(v.data[0])
            , hi(v.data[1])
        {
        }

        template <typename T>
        operator composite_vector<T> ()
        {
            return composite_vector<T>(lo, hi);
        }
    };

} // namespace mango::simd::detail

namespace mango::simd
{

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

    // 128 <- 128

#if defined(MANGO_ENABLE_SSE4_1)

    static inline u16x8 extend16x8(u8x16 s)
    {
        return _mm_cvtepu8_epi16(s);
    }

    static inline u32x4 extend32x4(u8x16 s)
    {
        return _mm_cvtepu8_epi32(s);
    }

    static inline u64x2 extend64x2(u8x16 s)
    {
        return _mm_cvtepu8_epi64(s);
    }

    static inline u32x4 extend32x4(u16x8 s)
    {
        return _mm_cvtepu16_epi32(s);
    }

    static inline u64x2 extend64x2(u16x8 s)
    {
        return _mm_cvtepu16_epi64(s);
    }

    static inline u64x2 extend64x2(u32x4 s)
    {
        return _mm_cvtepu32_epi64(s);
    }

#else

    static inline u16x8 extend16x8(u8x16 s)
    {
        const __m128i zero = _mm_setzero_si128();
        return _mm_unpacklo_epi8(s, zero);
    }

    static inline u32x4 extend32x4(u8x16 s)
    {
        const __m128i zero = _mm_setzero_si128();
        return _mm_unpacklo_epi16(_mm_unpacklo_epi8(s, zero), zero);
    }

    static inline u64x2 extend64x2(u8x16 s)
    {
        const __m128i zero = _mm_setzero_si128();
        return _mm_unpacklo_epi32(_mm_unpacklo_epi16(_mm_unpacklo_epi8(s, zero), zero), zero);
    }

    static inline u32x4 extend32x4(u16x8 s)
    {
        const __m128i zero = _mm_setzero_si128();
        return _mm_unpacklo_epi16(s, zero);
    }

    static inline u64x2 extend64x2(u16x8 s)
    {
        const __m128i zero = _mm_setzero_si128();
        return _mm_unpacklo_epi32(_mm_unpacklo_epi16(s, zero), zero);
    }

    static inline u64x2 extend64x2(u32x4 s)
    {
        const __m128i zero = _mm_setzero_si128();
        return _mm_unpacklo_epi32(s, zero);
    }

#endif

    // 256 <- 128

    static inline u16x16 extend16x16(u8x16 s)
    {
        u8x16 high = _mm_unpackhi_epi64(s, s);
        auto lo = extend16x8(s);
        auto hi = extend16x8(high);
        return { lo, hi };
    }

    static inline u32x8 extend32x8(u8x16 s)
    {
        u8x16 high = _mm_unpackhi_epi64(s, s);
        auto lo = extend32x4(s);
        auto hi = extend32x4(high);
        return { lo, hi };
    }

    static inline u64x4 extend64x4(u8x16 s)
    {
        u8x16 high = _mm_unpackhi_epi64(s, s);
        auto lo = extend64x2(s);
        auto hi = extend64x2(high);
        return { lo, hi };
    }

    static inline u32x8 extend32x8(u16x8 s)
    {
        u16x8 high = _mm_unpackhi_epi64(s, s);
        auto lo = extend32x4(s);
        auto hi = extend32x4(high);
        return { lo, hi };
    }

    static inline u64x4 extend64x4(u16x8 s)
    {
        u16x8 high = _mm_unpackhi_epi64(s, s);
        auto lo = extend64x2(s);
        auto hi = extend64x2(high);
        return { lo, hi };
    }

    static inline u64x4 extend64x4(u32x4 s)
    {
        u32x4 high = _mm_unpackhi_epi64(s, s);
        auto lo = extend64x2(s);
        auto hi = extend64x2(high);
        return { lo, hi };
    }

    // -----------------------------------------------------------------
    // sign extend
    // -----------------------------------------------------------------

    // 128 <- 128

#if defined(MANGO_ENABLE_SSE4_1)

    static inline s16x8 extend16x8(s8x16 s)
    {
        return _mm_cvtepi8_epi16(s);
    }

    static inline s32x4 extend32x4(s8x16 s)
    {
        return _mm_cvtepi8_epi32(s);
    }

    static inline s64x2 extend64x2(s8x16 s)
    {
        return _mm_cvtepi8_epi64(s);
    }

    static inline s32x4 extend32x4(s16x8 s)
    {
        return _mm_cvtepi16_epi32(s);
    }

    static inline s64x2 extend64x2(s16x8 s)
    {
        return _mm_cvtepi16_epi64(s);
    }

    static inline s64x2 extend64x2(s32x4 s)
    {
        return _mm_cvtepi32_epi64(s);
    }

#else

    static inline s16x8 extend16x8(s8x16 s)
    {
        const __m128i sign = _mm_cmpgt_epi8(_mm_setzero_si128(), s);
        return _mm_unpacklo_epi8(s, sign);
        //return _mm_srai_epi16(_mm_unpacklo_epi8(s, s), 8);
    }

    static inline s32x4 extend32x4(s8x16 s)
    {
        const __m128i sign = _mm_cmpgt_epi8(_mm_setzero_si128(), s);
        return _mm_unpacklo_epi16(_mm_unpacklo_epi8(s, sign), sign);
    }

    static inline s64x2 extend64x2(s8x16 s)
    {
        const __m128i sign = _mm_cmpgt_epi8(_mm_setzero_si128(), s);
        return _mm_unpacklo_epi32(_mm_unpacklo_epi16(_mm_unpacklo_epi8(s, sign), sign), sign);
    }

    static inline s32x4 extend32x4(s16x8 s)
    {
        const __m128i sign = _mm_cmpgt_epi16(_mm_setzero_si128(), s);
        return _mm_unpacklo_epi16(s, sign);
        //return _mm_srai_epi32(_mm_unpacklo_epi16(s, s), 16);
    }

    static inline s64x2 extend64x2(s16x8 s)
    {
        const __m128i sign = _mm_cmpgt_epi16(_mm_setzero_si128(), s);
        return _mm_unpacklo_epi32(_mm_unpacklo_epi16(s, sign), sign);
    }

    static inline s64x2 extend64x2(s32x4 s)
    {
        const __m128i sign = _mm_cmpgt_epi32(_mm_setzero_si128(), s);
        return _mm_unpacklo_epi32(s, sign);
    }

#endif

    // 256 <- 128

    static inline s16x16 extend16x16(s8x16 s)
    {
        s8x16 high = _mm_unpackhi_epi64(s, s);
        auto lo = extend16x8(s);
        auto hi = extend16x8(high);
        return { lo, hi };
    }

    static inline s32x8 extend32x8(s8x16 s)
    {
        s8x16 high = _mm_unpackhi_epi64(s, s);
        auto lo = extend32x4(s);
        auto hi = extend32x4(high);
        return { lo, hi };
    }

    static inline s64x4 extend64x4(s8x16 s)
    {
        s8x16 high = _mm_unpackhi_epi64(s, s);
        auto lo = extend64x2(s);
        auto hi = extend64x2(high);
        return { lo, hi };
    }

    static inline s32x8 extend32x8(s16x8 s)
    {
        s16x8 high = _mm_unpackhi_epi64(s, s);
        auto lo = extend32x4(s);
        auto hi = extend32x4(high);
        return { lo, hi };
    }

    static inline s64x4 extend64x4(s16x8 s)
    {
        s16x8 high = _mm_unpackhi_epi64(s, s);
        auto lo = extend64x2(s);
        auto hi = extend64x2(high);
        return { lo, hi };
    }

    static inline s64x4 extend64x4(s32x4 s)
    {
        s32x4 high = _mm_unpackhi_epi64(s, s);
        auto lo = extend64x2(s);
        auto hi = extend64x2(high);
        return { lo, hi };
    }

    // -----------------------------------------------------------------
    // narrow
    // -----------------------------------------------------------------

    static inline u8x16 narrow(u16x8 a, u16x8 b)
    {
        return _mm_packus_epi16(a, b);
    }

#if defined(MANGO_ENABLE_SSE4_1)

    static inline u16x8 narrow(u32x4 a, u32x4 b)
    {
        return _mm_packus_epi32(a, b);
    }

#else

    static inline u16x8 narrow(u32x4 a, u32x4 b)
    {
        a = _mm_slli_epi32(a, 16);
        a = _mm_srai_epi32(a, 16);
        b = _mm_slli_epi32(b, 16);
        b = _mm_srai_epi32(b, 16);
        return _mm_packs_epi32(a, b);
    }

#endif

    static inline s8x16 narrow(s16x8 a, s16x8 b)
    {
        return _mm_packs_epi16(a, b);
    }

    static inline s16x8 narrow(s32x4 a, s32x4 b)
    {
        return _mm_packs_epi32(a, b);
    }

    // -----------------------------------------------------------------
    // u32
    // -----------------------------------------------------------------

    static inline u32x4 get_low(u32x8 a)
    {
        return a.data[0];
    }

    static inline u32x4 get_high(u32x8 a)
    {
        return a.data[1];
    }

    static inline u32x8 set_low(u32x8 a, u32x4 low)
    {
        a.data[0] = low;
        return a;
    }

    static inline u32x8 set_high(u32x8 a, u32x4 high)
    {
        a.data[1] = high;
        return a;
    }

    static inline u32x8 combine(u32x4 a, u32x4 b)
    {
        u32x8 v;
        v.data[0] = a;
        v.data[1] = b;
        return v;
    }

    // -----------------------------------------------------------------
    // s32
    // -----------------------------------------------------------------

    static inline s32x4 get_low(s32x8 a)
    {
        return a.data[0];
    }

    static inline s32x4 get_high(s32x8 a)
    {
        return a.data[1];
    }

    static inline s32x8 set_low(s32x8 a, s32x4 low)
    {
        a.data[0] = low;
        return a;
    }

    static inline s32x8 set_high(s32x8 a, s32x4 high)
    {
        a.data[1] = high;
        return a;
    }

    static inline s32x8 combine(s32x4 a, s32x4 b)
    {
        s32x8 v;
        v.data[0] = a;
        v.data[1] = b;
        return v;
    }

    // -----------------------------------------------------------------
    // f32
    // -----------------------------------------------------------------

    static inline f32x4 get_low(f32x8 a)
    {
        return a.data[0];
    }

    static inline f32x4 get_high(f32x8 a)
    {
        return a.data[1];
    }

    static inline f32x8 set_low(f32x8 a, f32x4 low)
    {
        a.data[0] = low;
        return a;
    }

    static inline f32x8 set_high(f32x8 a, f32x4 high)
    {
        a.data[1] = high;
        return a;
    }

    static inline f32x8 combine(f32x4 a, f32x4 b)
    {
        return { a, b };
    }

    // 128 bit convert

    template <>
    inline f32x4 convert<f32x4>(u32x4 s)
    {
        const __m128 lo = _mm_cvtepi32_ps(_mm_and_si128(s, _mm_set1_epi32(0xffff)));
        const __m128 hi = _mm_cvtepi32_ps(_mm_srli_epi32(s, 16));
        return _mm_add_ps(lo, _mm_mul_ps(hi, _mm_set1_ps(65536.0f)));
    }

    template <>
    inline f32x4 convert<f32x4>(s32x4 s)
    {
        return _mm_cvtepi32_ps(s);
    }

    template <>
    inline u32x4 convert<u32x4>(f32x4 s)
    {
        __m128 x2 = _mm_castsi128_ps(_mm_set1_epi32(0x4f000000));
        __m128 x1 = _mm_cmple_ps(x2, s);
        __m128i x0 = _mm_cvtps_epi32(_mm_sub_ps(s, _mm_and_ps(x2, x1)));
        return _mm_or_si128(x0, _mm_slli_epi32(_mm_castps_si128(x1), 31));
    }

    template <>
    inline s32x4 convert<s32x4>(f32x4 s)
    {
        return _mm_cvtps_epi32(s);
    }

    template <>
    inline s32x4 truncate<s32x4>(f32x4 s)
    {
        return _mm_cvttps_epi32(s);
    }

    // 256 bit convert

    template <>
    inline s32x8 convert<s32x8>(f32x8 s)
    {
        auto lo = convert<s32x4>(s.data[0]);
        auto hi = convert<s32x4>(s.data[1]);
        return { lo, hi };
    }

    template <>
    inline f32x8 convert<f32x8>(s32x8 s)
    {
        auto lo = convert<f32x4>(s.data[0]);
        auto hi = convert<f32x4>(s.data[1]);
        return { lo, hi };
    }

    template <>
    inline u32x8 convert<u32x8>(f32x8 s)
    {
        auto lo = convert<u32x4>(s.data[0]);
        auto hi = convert<u32x4>(s.data[1]);
        return { lo, hi };
    }

    template <>
    inline f32x8 convert<f32x8>(u32x8 s)
    {
        auto lo = convert<f32x4>(s.data[0]);
        auto hi = convert<f32x4>(s.data[1]);
        return { lo, hi };
    }

    template <>
    inline s32x8 truncate<s32x8>(f32x8 s)
    {
        auto lo = truncate<s32x4>(s.data[0]);
        auto hi = truncate<s32x4>(s.data[1]);
        return { lo, hi };
    }

    // 512 bit convert

    template <>
    inline s32x16 convert<s32x16>(f32x16 s)
    {
        auto lo = convert<s32x8>(s.data[0]);
        auto hi = convert<s32x8>(s.data[1]);
        return { lo, hi };
    }

    template <>
    inline f32x16 convert<f32x16>(s32x16 s)
    {
        auto lo = convert<f32x8>(s.data[0]);
        auto hi = convert<f32x8>(s.data[1]);
        return { lo, hi };
    }

    template <>
    inline u32x16 convert<u32x16>(f32x16 s)
    {
        auto lo = convert<u32x8>(s.data[0]);
        auto hi = convert<u32x8>(s.data[1]);
        return { lo, hi };
    }

    template <>
    inline f32x16 convert<f32x16>(u32x16 s)
    {
        auto lo = convert<f32x8>(s.data[0]);
        auto hi = convert<f32x8>(s.data[1]);
        return { lo, hi };
    }

    template <>
    inline s32x16 truncate<s32x16>(f32x16 s)
    {
        auto lo = truncate<s32x8>(s.data[0]);
        auto hi = truncate<s32x8>(s.data[1]);
        return { lo, hi };
    }

    // -----------------------------------------------------------------
    // f64
    // -----------------------------------------------------------------

    static inline f64x2 get_low(f64x4 a)
    {
        return a.data[0];
    }

    static inline f64x2 get_high(f64x4 a)
    {
        return a.data[1];
    }

    static inline f64x4 set_low(f64x4 a, f64x2 low)
    {
        a.data[0] = low;
        return a;
    }

    static inline f64x4 set_high(f64x4 a, f64x2 high)
    {
        a.data[1] = high;
        return a;
    }

    static inline f64x4 combine(f64x2 a, f64x2 b)
    {
        return { a, b };
    }

    // 256 <- 128

    template <>
    inline f64x4 convert<f64x4>(s32x4 s)
    {
        auto lo = _mm_cvtepi32_pd(s);
        auto hi = _mm_cvtepi32_pd(_mm_shuffle_epi32(s, 0xee));
        return { lo, hi };
    }

    template <>
    inline f64x4 convert<f64x4>(u32x4 ui)
    {
        const __m128d bias = _mm_set1_pd((1ll << 52) * 1.5);
        const __m128i mask = _mm_set1_epi32(0x43380000);
        __m128i xy = _mm_unpacklo_epi32(ui, mask);
        __m128i zw = _mm_unpackhi_epi32(ui, mask);
        auto lo = _mm_sub_pd(_mm_castsi128_pd(xy), bias);
        auto hi = _mm_sub_pd(_mm_castsi128_pd(zw), bias);
        return { lo, hi };
    }

    template <>
    inline f64x4 convert<f64x4>(f32x4 s)
    {
        auto lo = _mm_cvtps_pd(s);
        auto hi = _mm_cvtps_pd(_mm_shuffle_ps(s, s, 0xee));
        return { lo, hi };
    }

    // 128 <- 256

    template <>
    inline s32x4 truncate<s32x4>(f64x4 s)
    {
        __m128i xy = _mm_cvttpd_epi32(s.data[0]);
        __m128i zw = _mm_cvttpd_epi32(s.data[1]);
        __m128i xzyw = _mm_unpacklo_epi32(xy, zw);
        return _mm_shuffle_epi32(xzyw, 0xd8);
    }

    template <>
    inline s32x4 convert<s32x4>(f64x4 s)
    {
        __m128i xy = _mm_cvtpd_epi32(s.data[0]);
        __m128i zw = _mm_cvtpd_epi32(s.data[1]);
        __m128i xzyw = _mm_unpacklo_epi32(xy, zw);
        return _mm_shuffle_epi32(xzyw, 0xd8);
    }

    template <>
    inline u32x4 convert<u32x4>(f64x4 d)
    {
        const __m128d bias = _mm_set1_pd((1ll << 52) * 1.5);
        __m128 xy = _mm_castpd_ps(_mm_add_pd(d.data[0], bias));
        __m128 zw = _mm_castpd_ps(_mm_add_pd(d.data[1], bias));
        __m128 u = _mm_shuffle_ps(xy, zw, 0x88);
        return _mm_castps_si128(u);
    }

    template <>
    inline f32x4 convert<f32x4>(f64x4 s)
    {
        __m128 xy00 = _mm_cvtpd_ps(s.data[0]);
        __m128 zw00 = _mm_cvtpd_ps(s.data[1]);
        return _mm_shuffle_ps(xy00, zw00, 0x44);
    }

    template <>
    inline f32x4 convert<f32x4>(s64x4 s)
    {
        f32 x = f32(get_component<0>(s));
        f32 y = f32(get_component<1>(s));
        f32 z = f32(get_component<2>(s));
        f32 w = f32(get_component<3>(s));
        return f32x4_set(x, y, z, w);
    }

    template <>
    inline f32x4 convert<f32x4>(u64x4 s)
    {
        f32 x = f32(get_component<0>(s));
        f32 y = f32(get_component<1>(s));
        f32 z = f32(get_component<2>(s));
        f32 w = f32(get_component<3>(s));
        return f32x4_set(x, y, z, w);
    }

    // 128 <- 128

#if 1

    template <>
    inline s64x2 convert<s64x2>(f64x2 v)
    {
        s64 x = s64(get_component<0>(v) + 0.5);
        s64 y = s64(get_component<1>(v) + 0.5);
        return s64x2_set(x, y);
    }

    template <>
    inline u64x2 convert<u64x2>(f64x2 v)
    {
        u64 x = u64(get_component<0>(v) + 0.5);
        u64 y = u64(get_component<1>(v) + 0.5);
        return u64x2_set(x, y);
    }

    template <>
    inline s64x2 truncate<s64x2>(f64x2 v)
    {
        v = trunc(v);
        s64 x = s64(get_component<0>(v));
        s64 y = s64(get_component<1>(v));
        return s64x2_set(x, y);
    }

    template <>
    inline u64x2 truncate<u64x2>(f64x2 v)
    {
        v = trunc(v);
        u64 x = u64(get_component<0>(v));
        u64 y = u64(get_component<1>(v));
        return u64x2_set(x, y);
    }

    template <>
    inline f64x2 convert<f64x2>(s64x2 v)
    {
        f64 x = f64(get_component<0>(v));
        f64 y = f64(get_component<1>(v));
        return f64x2_set(x, y);
    }

    template <>
    inline f64x2 convert<f64x2>(u64x2 v)
    {
        f64 x = f64(get_component<0>(v));
        f64 y = f64(get_component<1>(v));
        return f64x2_set(x, y);
    }

#else

    template <>
    inline s64x2 convert<s64x2>(f64x2 v)
    {
        // valid range: [-2^51, 2^51]
        v = _mm_add_pd(v, _mm_set1_pd(0x0018000000000000));
        return _mm_sub_epi64(
            _mm_castpd_si128(v),
            _mm_castpd_si128(_mm_set1_pd(0x0018000000000000)));
    }

    template <>
    inline u64x2 convert<u64x2>(f64x2 v)
    {
        // valid range: [0, 2^52)
        v = _mm_add_pd(v, _mm_set1_pd(0x0010000000000000));
        return _mm_xor_si128(
            _mm_castpd_si128(v),
            _mm_castpd_si128(_mm_set1_pd(0x0010000000000000)));
    }

    template <>
    inline s64x2 truncate<s64x2>(f64x2 v)
    {
        v = trunc(v);
        return convert<s64x2>(v);
    }

    template <>
    inline u64x2 truncate<u64x2>(f64x2 v)
    {
        v = trunc(v);
        return convert<u64x2>(v);
    }

    template <>
    inline f64x2 convert<f64x2>(s64x2 v)
    {
        // valid range: [-2^51, 2^51]
        v = _mm_add_epi64(v, _mm_castpd_si128(_mm_set1_pd(0x0018000000000000)));
        return _mm_sub_pd(_mm_castsi128_pd(v), _mm_set1_pd(0x0018000000000000));
    }

    template <>
    inline f64x2 convert<f64x2>(u64x2 v)
    {
        // valid range: [0, 2^52)
        v = _mm_or_si128(v, _mm_castpd_si128(_mm_set1_pd(0x0010000000000000)));
        return _mm_sub_pd(_mm_castsi128_pd(v), _mm_set1_pd(0x0010000000000000));
    }

#endif

    // 256 <- 256

    template <>
    inline s64x4 convert<s64x4>(f64x4 v)
    {
        auto lo = convert<s64x2>(v.data[0]);
        auto hi = convert<s64x2>(v.data[1]);
        return { lo, hi };
    }

    template <>
    inline u64x4 convert<u64x4>(f64x4 v)
    {
        auto lo = convert<u64x2>(v.data[0]);
        auto hi = convert<u64x2>(v.data[1]);
        return { lo, hi };
    }

    template <>
    inline s64x4 truncate<s64x4>(f64x4 v)
    {
        auto lo = truncate<s64x2>(v.data[0]);
        auto hi = truncate<s64x2>(v.data[1]);
        return { lo, hi };
    }

    template <>
    inline u64x4 truncate<u64x4>(f64x4 v)
    {
        auto lo = truncate<u64x2>(v.data[0]);
        auto hi = truncate<u64x2>(v.data[1]);
        return { lo, hi };
    }

    template <>
    inline f64x4 convert<f64x4>(s64x4 v)
    {
        auto lo = convert<f64x2>(v.data[0]);
        auto hi = convert<f64x2>(v.data[1]);
        return { lo, hi };
    }

    template <>
    inline f64x4 convert<f64x4>(u64x4 v)
    {
        auto lo = convert<f64x2>(v.data[0]);
        auto hi = convert<f64x2>(v.data[1]);
        return { lo, hi };
    }

    // 512 <- 512

    template <>
    inline s64x8 convert<s64x8>(f64x8 v)
    {
        auto lo = convert<s64x4>(v.data[0]);
        auto hi = convert<s64x4>(v.data[1]);
        return { lo, hi };
    }

    template <>
    inline u64x8 convert<u64x8>(f64x8 v)
    {
        auto lo = convert<u64x4>(v.data[0]);
        auto hi = convert<u64x4>(v.data[1]);
        return { lo, hi };
    }

    template <>
    inline s64x8 truncate<s64x8>(f64x8 v)
    {
        auto lo = truncate<s64x4>(v.data[0]);
        auto hi = truncate<s64x4>(v.data[1]);
        return { lo, hi };
    }

    template <>
    inline u64x8 truncate<u64x8>(f64x8 v)
    {
        auto lo = truncate<u64x4>(v.data[0]);
        auto hi = truncate<u64x4>(v.data[1]);
        return { lo, hi };
    }

    template <>
    inline f64x8 convert<f64x8>(s64x8 v)
    {
        auto lo = convert<f64x4>(v.data[0]);
        auto hi = convert<f64x4>(v.data[1]);
        return { lo, hi };
    }

    template <>
    inline f64x8 convert<f64x8>(u64x8 v)
    {
        auto lo = convert<f64x4>(v.data[0]);
        auto hi = convert<f64x4>(v.data[1]);
        return { lo, hi };
    }

    // -----------------------------------------------------------------
    // f16
    // -----------------------------------------------------------------

#ifdef __F16C__

    template <>
    inline f32x4 convert<f32x4>(f16x4 h)
    {
        return _mm_cvtph_ps(_mm_set1_epi64x(h.data));
    }

    template <>
    inline f16x4 convert<f16x4>(f32x4 f)
    {
        __m128i temp = _mm_cvtps_ph(f, _MM_FROUND_TO_NEAREST_INT);
#if defined(MANGO_ENABLE_SSE4_1) && defined(MANGO_CPU_64BIT)
        return _mm_extract_epi64(temp, 0);
#else
        f16x4 h;
        _mm_storel_epi64(reinterpret_cast<__m128i *>(&h), temp);
        return h;
#endif
    }

#else

    template <>
    inline f32x4 convert<f32x4>(f16x4 h)
    {
        const s32x4 u = _mm_unpacklo_epi16(_mm_set1_epi64x(h.data), _mm_setzero_si128());

        s32x4 no_sign  = bitwise_and(u, s32x4_set(0x7fff));
        s32x4 sign     = bitwise_and(u, s32x4_set(0x8000));
        s32x4 exponent = bitwise_and(u, s32x4_set(0x7c00));
        s32x4 mantissa = bitwise_and(u, s32x4_set(0x03ff));

        // NaN or Inf
        s32x4 a = bitwise_or(s32x4_set(0x7f800000), slli<13>(mantissa));

        // Zero or Denormal
        const s32x4 magic = s32x4_set(0x3f000000);
        s32x4 b;
        b = add(magic, mantissa);
        b = reinterpret<s32x4>(sub(reinterpret<f32x4>(b), reinterpret<f32x4>(magic)));

        // Numeric Value
        s32x4 c = add(s32x4_set(0x38000000), slli<13>(no_sign));

        // Select a, b, or c based on exponent
        mask32x4 mask;
        s32x4 result;

        mask = compare_eq(exponent, s32x4_zero());
        result = select(mask, b, c);

        mask = compare_eq(exponent, s32x4_set(0x7c00));
        result = select(mask, a, result);

        // Sign
        result = bitwise_or(result, slli<16>(sign));

        return reinterpret<f32x4>(result);
    }

    template <>
    inline f16x4 convert<f16x4>(f32x4 f)
    {
        const f32x4 magic = f32x4_set(Float(0, 15, 0).f);
        const s32x4 vinf = s32x4_set(31 << 23);

        const s32x4 u = reinterpret<s32x4>(f);
        const s32x4 sign = srli<16>(bitwise_and(u, s32x4_set(0x80000000)));

        const s32x4 vexponent = s32x4_set(0x7f800000);

        // Inf / NaN
        const mask32x4 s0 = compare_eq(bitwise_and(u, vexponent), vexponent);
        s32x4 mantissa = bitwise_and(u, s32x4_set(0x007fffff));
        mask32x4 x0 = compare_eq(mantissa, s32x4_zero());
        mantissa = select(x0, s32x4_zero(), srai<13>(mantissa));
        const s32x4 v0 = bitwise_or(s32x4_set(0x7c00), mantissa);

        s32x4 v1 = bitwise_and(u, s32x4_set(0x7ffff000));
        v1 = reinterpret<s32x4>(mul(reinterpret<f32x4>(v1), magic));
        v1 = add(v1, s32x4_set(0x1000));

#if defined(MANGO_ENABLE_SSE4_1) && defined(MANGO_CPU_64BIT)
        v1 = _mm_min_epi32(v1, vinf);
        v1 = srai<13>(v1);

        s32x4 v = select(s0, v0, v1);
        v = bitwise_or(v, sign);
        v = _mm_packus_epi32(v, v);

        return _mm_extract_epi64(v, 0);
#else
        v1 = select(compare_gt(v1, vinf), vinf, v1);
        v1 = srai<13>(v1);

        s32x4 v = select(s0, v0, v1);
        v = bitwise_or(v, sign);
        v = _mm_slli_epi32 (v, 16);
        v = _mm_srai_epi32 (v, 16);
        v = _mm_packs_epi32 (v, v);

        f16x4 h;
        _mm_storel_epi64(reinterpret_cast<__m128i *>(&h), v);
        return h;
#endif // defined(MANGO_ENABLE_SSE4_1)
    }

#endif // __F16C__

} // namespace mango::simd
