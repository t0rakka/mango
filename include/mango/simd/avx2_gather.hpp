/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/simd/simd.hpp>

namespace mango::simd
{

    // 128 bit gather

    inline u32x4 gather(const u32* address, s32x4 offset)
    {
        return _mm_i32gather_epi32(reinterpret_cast<const int*>(address), offset, 4);
    }

    inline s32x4 gather(const s32* address, s32x4 offset)
    {
        return _mm_i32gather_epi32(reinterpret_cast<const int*>(address), offset, 4);
    }

    inline f32x4 gather(const f32* address, s32x4 offset)
    {
        return _mm_i32gather_ps(reinterpret_cast<const float*>(address), offset, 4);
    }

    inline u64x2 gather(const u64* address, s32x2 offset)
    {
        __m128i offset128 = _mm_loadl_epi64(reinterpret_cast<__m128i const *>(&offset));
        return _mm_i32gather_epi64(reinterpret_cast<const long long*>(address), offset128, 8);
    }

    inline s64x2 gather(const s64* address, s32x2 offset)
    {
        __m128i offset128 = _mm_loadl_epi64(reinterpret_cast<__m128i const *>(&offset));
        return _mm_i32gather_epi64(reinterpret_cast<const long long*>(address), offset128, 8);
    }

    inline f64x2 gather(const f64* address, s32x2 offset)
    {
        __m128i offset128 = _mm_loadl_epi64(reinterpret_cast<__m128i const *>(&offset));
        return _mm_i32gather_pd(reinterpret_cast<const double*>(address), offset128, 8);
    }

    // 256 bit gather

    inline u32x8 gather(const u32* address, s32x8 offset)
    {
        return _mm256_i32gather_epi32(reinterpret_cast<const int*>(address), offset, 4);
    }

    inline s32x8 gather(const s32* address, s32x8 offset)
    {
        return _mm256_i32gather_epi32(reinterpret_cast<const int*>(address), offset, 4);
    }

    inline f32x8 gather(const f32* address, s32x8 offset)
    {
        return _mm256_i32gather_ps(reinterpret_cast<const float*>(address), offset, 4);
    }

    inline u64x4 gather(const u64* address, s32x4 offset)
    {
        return _mm256_i32gather_epi64(reinterpret_cast<const long long*>(address), offset, 8);
    }

    inline s64x4 gather(const s64* address, s32x4 offset)
    {
        return _mm256_i32gather_epi64(reinterpret_cast<const long long*>(address), offset, 8);
    }

    inline f64x4 gather(const f64* address, s32x4 offset)
    {
        return _mm256_i32gather_pd(reinterpret_cast<const double*>(address), offset, 8);
    }

    // 512 bit gather

    inline u32x16 gather(const u32* address, s32x16 offset)
    {
        auto lo = gather(address, offset.data[0]);
        auto hi = gather(address, offset.data[1]);
        return { lo, hi };
    }

    inline s32x16 gather(const s32* address, s32x16 offset)
    {
        auto lo = gather(address, offset.data[0]);
        auto hi = gather(address, offset.data[1]);
        return { lo, hi };
    }

    inline f32x16 gather(const f32* address, s32x16 offset)
    {
        auto lo = gather(address, offset.data[0]);
        auto hi = gather(address, offset.data[1]);
        return { lo, hi };
    }

    inline u64x8 gather(const u64* address, s32x8 offset)
    {
        auto lo = gather(address, get_low(offset));
        auto hi = gather(address, get_high(offset));
        return { lo, hi };
    }

    inline s64x8 gather(const s64* address, s32x8 offset)
    {
        auto lo = gather(address, get_low(offset));
        auto hi = gather(address, get_high(offset));
        return { lo, hi };
    }

    inline f64x8 gather(const f64* address, s32x8 offset)
    {
        auto lo = gather(address, get_low(offset));
        auto hi = gather(address, get_high(offset));
        return { lo, hi };
    }

    // 128 bit masked gather

    inline u32x4 gather(const u32* address, s32x4 offset, u32x4 value, mask32x4 mask)
    {
        return _mm_mask_i32gather_epi32(value, reinterpret_cast<const int*>(address), offset, mask, 4);
    }

    inline s32x4 gather(const s32* address, s32x4 offset, s32x4 value, mask32x4 mask)
    {
        return _mm_mask_i32gather_epi32(value, reinterpret_cast<const int*>(address), offset, mask, 4);
    }

    inline f32x4 gather(const f32* address, s32x4 offset, f32x4 value, mask32x4 mask)
    {
        return _mm_mask_i32gather_ps(value, reinterpret_cast<const float*>(address), offset, _mm_castsi128_ps(mask), 4);
    }

    inline u64x2 gather(const u64* address, s32x2 offset, u64x2 value, mask64x2 mask)
    {
        __m128i offset128 = _mm_loadl_epi64(reinterpret_cast<__m128i const *>(&offset));
        return _mm_mask_i32gather_epi64(value, reinterpret_cast<const long long*>(address), offset128, mask, 8);
    }

    inline s64x2 gather(const s64* address, s32x2 offset, s64x2 value, mask64x2 mask)
    {
        __m128i offset128 = _mm_loadl_epi64(reinterpret_cast<__m128i const *>(&offset));
        return _mm_mask_i32gather_epi64(value, reinterpret_cast<const long long*>(address), offset128, mask, 8);
    }

    inline f64x2 gather(const f64* address, s32x2 offset, f64x2 value, mask64x2 mask)
    {
        __m128i offset128 = _mm_loadl_epi64(reinterpret_cast<__m128i const *>(&offset));
        return _mm_mask_i32gather_pd(value, reinterpret_cast<const double*>(address), offset128, _mm_castsi128_pd(mask), 8);
    }

    // 256 bit masked gather

    inline u32x8 gather(const u32* address, s32x8 offset, u32x8 value, mask32x8 mask)
    {
        return _mm256_mask_i32gather_epi32(value, reinterpret_cast<const int*>(address), offset, mask, 4);
    }

    inline s32x8 gather(const s32* address, s32x8 offset, s32x8 value, mask32x8 mask)
    {
        return _mm256_mask_i32gather_epi32(value, reinterpret_cast<const int*>(address), offset, mask, 4);
    }

    inline f32x8 gather(const f32* address, s32x8 offset, f32x8 value, mask32x8 mask)
    {
        return _mm256_mask_i32gather_ps(value, reinterpret_cast<const float*>(address), offset, _mm256_castsi256_ps(mask), 4);
    }

    inline u64x4 gather(const u64* address, s32x4 offset, u64x4 value, mask64x4 mask)
    {
        return _mm256_mask_i32gather_epi64(value, reinterpret_cast<const long long*>(address), offset, mask, 8);
    }

    inline s64x4 gather(const s64* address, s32x4 offset, s64x4 value, mask64x4 mask)
    {
        return _mm256_mask_i32gather_epi64(value, reinterpret_cast<const long long*>(address), offset, mask, 8);
    }

    inline f64x4 gather(const f64* address, s32x4 offset, f64x4 value, mask64x4 mask)
    {
        return _mm256_mask_i32gather_pd(value, reinterpret_cast<const double*>(address), offset, _mm256_castsi256_pd(mask), 8);
    }

    // 512 bit masked gather

    inline u32x16 gather(const u32* address, s32x16 offset, u32x16 value, mask32x16 mask)
    {
        return select(mask, gather(address, offset), value);
    }

    inline s32x16 gather(const s32* address, s32x16 offset, s32x16 value, mask32x16 mask)
    {
        return select(mask, gather(address, offset), value);
    }

    inline f32x16 gather(const f32* address, s32x16 offset, f32x16 value, mask32x16 mask)
    {
        return select(mask, gather(address, offset), value);
    }

    inline u64x8 gather(const u64* address, s32x8 offset, u64x8 value, mask64x8 mask)
    {
        return select(mask, gather(address, offset), value);
    }

    inline s64x8 gather(const s64* address, s32x8 offset, s64x8 value, mask64x8 mask)
    {
        return select(mask, gather(address, offset), value);
    }

    inline f64x8 gather(const f64* address, s32x8 offset, f64x8 value, mask64x8 mask)
    {
        return select(mask, gather(address, offset), value);
    }

} // namespace mango::simd
