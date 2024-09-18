/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/simd/simd.hpp>
#include <mango/simd/common.hpp>

namespace mango::simd::detail
{

    template <int bits>
    struct reinterpret_vector;

    template <>
    struct reinterpret_vector<128>
    {
        v128_t data;

        reinterpret_vector(v128_t data)
            : data(data)
        {
        }

        template <typename ScalarType, int VectorSize, typename VectorType>
        operator hardware_vector<ScalarType, VectorSize, VectorType> ()
        {
            return hardware_vector<ScalarType, VectorSize, VectorType>(data);
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

    static inline u16x8 extend16x8(u8x16 s)
    {
        return wasm_u16x8_extend_low_u8x16(s);
    }

    static inline u32x4 extend32x4(u8x16 s)
    {
        return wasm_u32x4_extend_low_u16x8(s);
    }

    static inline u64x2 extend64x2(u8x16 s)
    {
        const v128_t zero = wasm_v128_xor(s, s);
        return wasm_i8x16_shuffle(s, zero, 0, 16, 16, 16, 16, 16, 16, 16, 1, 16, 16, 16, 16, 16, 16, 16);
    }

    static inline u32x4 extend32x4(u16x8 s)
    {
        const v128_t zero = wasm_v128_xor(s, s);
        return wasm_i16x8_shuffle(s, zero, 0, 8, 1, 8, 2, 8, 3, 8);
    }

    static inline u64x2 extend64x2(u16x8 s)
    {
        const v128_t zero = wasm_v128_xor(s, s);
        return wasm_i16x8_shuffle(s, zero, 0, 8, 8, 8, 1, 8, 8, 8);
    }

    static inline u64x2 extend64x2(u32x4 s)
    {
        return wasm_u64x2_extend_low_u32x4(s);
    }

    // 256 <- 128

    static inline u16x16 extend16x16(u8x16 s)
    {
        u16x16 result;
        result.data[0] = wasm_u16x8_extend_low_u8x16(s);
        result.data[1] = wasm_u16x8_extend_high_u8x16(s);
        return result;
    }

    static inline u32x8 extend32x8(u8x16 s)
    {
        u32x8 result;
        result.data[0] = wasm_u32x4_extend_low_u16x8(s);
        result.data[1] = wasm_u32x4_extend_high_u16x8(s);
        return result;
    }

    static inline u64x4 extend64x4(u8x16 s)
    {
        const v128_t zero = wasm_v128_xor(s, s);
        u64x4 result;
        result.data[0] = wasm_i8x16_shuffle(s, zero, 0, 16, 16, 16, 16, 16, 16, 16, 1, 16, 16, 16, 16, 16, 16, 16);
        result.data[1] = wasm_i8x16_shuffle(s, zero, 2, 16, 16, 16, 16, 16, 16, 16, 3, 16, 16, 16, 16, 16, 16, 16);
        return result;
    }

    static inline u32x8 extend32x8(u16x8 s)
    {
        const v128_t zero = wasm_v128_xor(s, s);
        u32x8 result;
        result.data[0] = wasm_i16x8_shuffle(s, zero, 0, 8, 1, 8, 2, 8, 3, 8);
        result.data[1] = wasm_i16x8_shuffle(s, zero, 4, 8, 5, 8, 6, 8, 7, 8);
        return result;
    }

    static inline u64x4 extend64x4(u16x8 s)
    {
        const v128_t zero = wasm_v128_xor(s, s);
        u64x4 result;
        result.data[0] = wasm_i16x8_shuffle(s, zero, 0, 8, 8, 8, 1, 8, 8, 8);
        result.data[1] = wasm_i16x8_shuffle(s, zero, 2, 8, 8, 8, 3, 8, 8, 8);
        return result;
    }

    static inline u64x4 extend64x4(u32x4 s)
    {
        u64x4 result;
        result.data[0] = wasm_u64x2_extend_low_u32x4(s);
        result.data[1] = wasm_u64x2_extend_high_u32x4(s);
        return result;
    }

    // -----------------------------------------------------------------
    // sign extend
    // -----------------------------------------------------------------

    // 128 <- 128

    static inline s16x8 extend16x8(s8x16 s)
    {
        return wasm_i16x8_extend_low_i8x16(s);
    }

    static inline s32x4 extend32x4(s8x16 s)
    {
        return wasm_i32x4_extend_low_i16x8(s);
    }

    static inline s64x2 extend64x2(s8x16 s)
    {
        const v128_t zero = wasm_v128_xor(s, s);
        const v128_t sign = wasm_i8x16_gt(zero, s);
        return wasm_i8x16_shuffle(s, sign, 0, 16, 16, 16, 16, 16, 16, 16, 1, 16, 16, 16, 16, 16, 16, 16);
    }

    static inline s32x4 extend32x4(s16x8 s)
    {
        const v128_t zero = wasm_v128_xor(s, s);
        const v128_t sign = wasm_i16x8_gt(zero, s);
        return wasm_i16x8_shuffle(s, sign, 0, 8, 1, 8, 2, 8, 3, 8);
    }

    static inline s64x2 extend64x2(s16x8 s)
    {
        const v128_t zero = wasm_v128_xor(s, s);
        const v128_t sign = wasm_i16x8_gt(zero, s);
        return wasm_i16x8_shuffle(s, sign, 0, 8, 8, 8, 1, 8, 8, 8);
    }

    static inline s64x2 extend64x2(s32x4 s)
    {
        return wasm_i64x2_extend_low_i32x4(s);
    }

    // 256 <- 128

    static inline s16x16 extend16x16(s8x16 s)
    {
        s16x16 result;
        result.data[0] = wasm_i16x8_extend_low_i8x16(s);
        result.data[1] = wasm_i16x8_extend_high_i8x16(s);
        return result;
    }

    static inline s32x8 extend32x8(s8x16 s)
    {
        s32x8 result;
        result.data[0] = wasm_i32x4_extend_low_i16x8(s);
        result.data[1] = wasm_i32x4_extend_high_i16x8(s);
        return result;
    }

    static inline s64x4 extend64x4(s8x16 s)
    {
        const v128_t zero = wasm_v128_xor(s, s);
        const v128_t sign = wasm_i8x16_gt(zero, s);
        s64x4 result;
        result.data[0] = wasm_i8x16_shuffle(s, sign, 0, 16, 16, 16, 16, 16, 16, 16, 1, 16, 16, 16, 16, 16, 16, 16);
        result.data[1] = wasm_i8x16_shuffle(s, sign, 2, 16, 16, 16, 16, 16, 16, 16, 3, 16, 16, 16, 16, 16, 16, 16);
        return result;
    }

    static inline s32x8 extend32x8(s16x8 s)
    {
        const v128_t zero = wasm_v128_xor(s, s);
        const v128_t sign = wasm_i16x8_gt(zero, s);
        s32x8 result;
        result.data[0] = wasm_i16x8_shuffle(s, sign, 0, 8, 1, 8, 2, 8, 3, 8);
        result.data[1] = wasm_i16x8_shuffle(s, sign, 4, 8, 5, 8, 6, 8, 7, 8);
        return result;
    }

    static inline s64x4 extend64x4(s16x8 s)
    {
        const v128_t zero = wasm_v128_xor(s, s);
        const v128_t sign = wasm_i16x8_gt(zero, s);
        s64x4 result;
        result.data[0] = wasm_i16x8_shuffle(s, sign, 0, 8, 8, 8, 1, 8, 8, 8);
        result.data[1] = wasm_i16x8_shuffle(s, sign, 2, 8, 8, 8, 3, 8, 8, 8);
        return result;
    }

    static inline s64x4 extend64x4(s32x4 s)
    {
        s64x4 result;
        result.data[0] = wasm_i64x2_extend_low_i32x4(s);
        result.data[1] = wasm_i64x2_extend_high_i32x4(s);
        return result;
    }

    // -----------------------------------------------------------------
    // narrow
    // -----------------------------------------------------------------

    static inline u8x16 narrow(u16x8 a, u16x8 b)
    {
        return wasm_u8x16_narrow_i16x8(a, b);
    }

    static inline u16x8 narrow(u32x4 a, u32x4 b)
    {
        return wasm_u16x8_narrow_i32x4(a, b);
    }

    static inline s8x16 narrow(s16x8 a, s16x8 b)
    {
        return wasm_i8x16_narrow_i16x8(a, b);
    }

    static inline s16x8 narrow(s32x4 a, s32x4 b)
    {
        return wasm_i16x8_narrow_i32x4(a, b);
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
        f32x8 result;
        result.data[0] = a;
        result.data[1] = b;
        return result;
    }

    // 128 bit convert

    template <>
    inline f32x4 convert<f32x4>(u32x4 s)
    {
        return wasm_f32x4_convert_u32x4(s);
    }

    template <>
    inline f32x4 convert<f32x4>(s32x4 s)
    {
        return wasm_f32x4_convert_i32x4(s);
    }

    template <>
    inline u32x4 convert<u32x4>(f32x4 s)
    {
        s = round(s);
        u32 x = u32(get_component<0>(s));
        u32 y = u32(get_component<1>(s));
        u32 z = u32(get_component<2>(s));
        u32 w = u32(get_component<3>(s));
        return u32x4_set(x, y, z, w);
    }

    template <>
    inline s32x4 convert<s32x4>(f32x4 s)
    {
        s = round(s);
        s32 x = s32(get_component<0>(s));
        s32 y = s32(get_component<1>(s));
        s32 z = s32(get_component<2>(s));
        s32 w = s32(get_component<3>(s));
        return s32x4_set(x, y, z, w);
    }

    template <>
    inline s32x4 truncate<s32x4>(f32x4 s)
    {
        return wasm_i32x4_trunc_sat_f32x4(s);
    }

    // 256 bit convert

    template <>
    inline s32x8 convert<s32x8>(f32x8 s)
    {
        s32x8 result;
        result.data[0] = convert<s32x4>(s.data[0]);
        result.data[1] = convert<s32x4>(s.data[1]);
        return result;
    }

    template <>
    inline f32x8 convert<f32x8>(s32x8 s)
    {
        f32x8 result;
        result.data[0] = convert<f32x4>(s.data[0]);
        result.data[1] = convert<f32x4>(s.data[1]);
        return result;
    }

    template <>
    inline u32x8 convert<u32x8>(f32x8 s)
    {
        u32x8 result;
        result.data[0] = convert<u32x4>(s.data[0]);
        result.data[1] = convert<u32x4>(s.data[1]);
        return result;
    }

    template <>
    inline f32x8 convert<f32x8>(u32x8 s)
    {
        f32x8 result;
        result.data[0] = convert<f32x4>(s.data[0]);
        result.data[1] = convert<f32x4>(s.data[1]);
        return result;
    }

    template <>
    inline s32x8 truncate<s32x8>(f32x8 s)
    {
        s32x8 result;
        result.data[0] = truncate<s32x4>(s.data[0]);
        result.data[1] = truncate<s32x4>(s.data[1]);
        return result;
    }

    // 512 bit convert

    template <>
    inline s32x16 convert<s32x16>(f32x16 s)
    {
        s32x16 result;
        result.data[0] = convert<s32x8>(s.data[0]);
        result.data[1] = convert<s32x8>(s.data[1]);
        return result;
    }

    template <>
    inline f32x16 convert<f32x16>(s32x16 s)
    {
        f32x16 result;
        result.data[0] = convert<f32x8>(s.data[0]);
        result.data[1] = convert<f32x8>(s.data[1]);
        return result;
    }

    template <>
    inline u32x16 convert<u32x16>(f32x16 s)
    {
        u32x16 result;
        result.data[0] = convert<u32x8>(s.data[0]);
        result.data[1] = convert<u32x8>(s.data[1]);
        return result;
    }

    template <>
    inline f32x16 convert<f32x16>(u32x16 s)
    {
        f32x16 result;
        result.data[0] = convert<f32x8>(s.data[0]);
        result.data[1] = convert<f32x8>(s.data[1]);
        return result;
    }

    template <>
    inline s32x16 truncate<s32x16>(f32x16 s)
    {
        s32x16 result;
        result.data[0] = truncate<s32x8>(s.data[0]);
        result.data[1] = truncate<s32x8>(s.data[1]);
        return result;
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
        f64x4 result;
        result.data[0] = a;
        result.data[1] = b;
        return result;
    }

    // 256 <- 128

    template <>
    inline f64x4 convert<f64x4>(s32x4 s)
    {
        auto lo = wasm_f64x2_convert_low_i32x4(s);
        auto hi = wasm_f64x2_convert_low_i32x4(wasm_i32x4_shuffle(s, s, 2, 3, 2, 3));
        return { lo, hi };
    }

    template <>
    inline f64x4 convert<f64x4>(u32x4 ui)
    {
        auto lo = wasm_f64x2_convert_low_u32x4(ui);
        auto hi = wasm_f64x2_convert_low_u32x4(wasm_i32x4_shuffle(ui, ui, 2, 3, 2, 3));
        return { lo, hi };
    }

    template <>
    inline f64x4 convert<f64x4>(f32x4 s)
    {
        f64 x = f64(get_component<0>(s));
        f64 y = f64(get_component<1>(s));
        f64 z = f64(get_component<2>(s));
        f64 w = f64(get_component<3>(s));
        return f64x4_set(x, y, z, w);
    }

    // 128 <- 256

    template <>
    inline s32x4 truncate<s32x4>(f64x4 s)
    {
        s = trunc(s);
        s32 x = s32(get_component<0>(s));
        s32 y = s32(get_component<1>(s));
        s32 z = s32(get_component<2>(s));
        s32 w = s32(get_component<3>(s));
        return s32x4_set(x, y, z, w);
    }

    template <>
    inline s32x4 convert<s32x4>(f64x4 s)
    {
        s = round(s);
        s32 x = s32(get_component<0>(s));
        s32 y = s32(get_component<1>(s));
        s32 z = s32(get_component<2>(s));
        s32 w = s32(get_component<3>(s));
        return s32x4_set(x, y, z, w);
    }

    template <>
    inline u32x4 convert<u32x4>(f64x4 s)
    {
        s = round(s);
        u32 x = u32(get_component<0>(s));
        u32 y = u32(get_component<1>(s));
        u32 z = u32(get_component<2>(s));
        u32 w = u32(get_component<3>(s));
        return u32x4_set(x, y, z, w);
    }

    template <>
    inline f32x4 convert<f32x4>(f64x4 s)
    {
        f32 x = f32(get_component<0>(s));
        f32 y = f32(get_component<1>(s));
        f32 z = f32(get_component<2>(s));
        f32 w = f32(get_component<3>(s));
        return f32x4_set(x, y, z, w);
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

    template <>
    inline f32x4 convert<f32x4>(f16x4 s)
    {
        f16 x = u16((s.data >>  0) & 0xffff);
        f16 y = u16((s.data >> 16) & 0xffff);
        f16 z = u16((s.data >> 32) & 0xffff);
        f16 w = u16((s.data >> 48) & 0xffff);
        return f32x4_set(x, y, z, w);
    }

    template <>
    inline f16x4 convert<f16x4>(f32x4 s)
    {
        f16 x = f16(get_component<0>(s));
        f16 y = f16(get_component<1>(s));
        f16 z = f16(get_component<2>(s));
        f16 w = f16(get_component<3>(s));
        return f16x4_set(x, y, z, w);
    }

} // namespace mango::simd
