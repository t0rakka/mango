/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // float64x4
    // -----------------------------------------------------------------

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline float64x4 shuffle(float64x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const int select0 = ((y & 1) << 1) | (x & 1);
        const int select1 = ((w & 1) << 1) | (z & 1);
        const __m128d& v0 = x & 2 ? v.hi : v.lo;
        const __m128d& v1 = y & 2 ? v.hi : v.lo;
        const __m128d& v2 = z & 2 ? v.hi : v.lo;
        const __m128d& v3 = w & 2 ? v.hi : v.lo;
        float64x4 result;
        result.lo = _mm_shuffle_pd(v0, v1, select0);
        result.hi = _mm_shuffle_pd(v2, v3, select1);
        return result;
    }

    template <>
    inline float64x4 shuffle<0, 1, 2, 3>(float64x4 v)
    {
        // .lozw
        return v;
    }

    template <>
    inline float64x4 shuffle<0, 0, 0, 0>(float64x4 v)
    {
        // .xxxx
        const __m128d xx = _mm_shuffle_pd(v.lo, v.lo, 0);
        float64x4 result;
        result.lo = xx;
        result.hi = xx;
        return result;
    }

    template <>
    inline float64x4 shuffle<1, 1, 1, 1>(float64x4 v)
    {
        // .yyyy
        const __m128d yy = _mm_shuffle_pd(v.lo, v.lo, 3);
        float64x4 result;
        result.lo = yy;
        result.hi = yy;
        return result;
    }

    template <>
    inline float64x4 shuffle<2, 2, 2, 2>(float64x4 v)
    {
        // .zzzz
        const __m128d zz = _mm_shuffle_pd(v.hi, v.hi, 0);
        float64x4 result;
        result.lo = zz;
        result.hi = zz;
        return result;
    }

    template <>
    inline float64x4 shuffle<3, 3, 3, 3>(float64x4 v)
    {
        // .wwww
        const __m128d ww = _mm_shuffle_pd(v.hi, v.hi, 3);
        float64x4 result;
        result.lo = ww;
        result.hi = ww;
        return result;
    }

    // set component

    template <int Index>
    static inline float64x4 set_component(float64x4 a, double s);

    template <>
    inline float64x4 set_component<0>(float64x4 a, double x)
    {
        a.lo = _mm_move_sd(a.lo, _mm_set1_pd(x));
        return a;
    }

    template <>
    inline float64x4 set_component<1>(float64x4 a, double y)
    {
        a.lo = _mm_move_sd(_mm_set1_pd(y), a.lo);
        return a;
    }

    template <>
    inline float64x4 set_component<2>(float64x4 a, double z)
    {
        a.hi = _mm_move_sd(a.hi, _mm_set1_pd(z));
        return a;
    }

    template <>
    inline float64x4 set_component<3>(float64x4 a, double w)
    {
        a.hi = _mm_move_sd(_mm_set1_pd(w), a.hi);
        return a;
    }

    // get component

    template <int Index>
    static inline double get_component(float64x4 a);

    template <>
    inline double get_component<0>(float64x4 a)
    {
        return _mm_cvtsd_f64(a.lo);
    }

    template <>
    inline double get_component<1>(float64x4 a)
    {
        const __m128d yy = _mm_unpackhi_pd(a.lo, a.lo);
        return _mm_cvtsd_f64(yy);
    }

    template <>
    inline double get_component<2>(float64x4 a)
    {
        return _mm_cvtsd_f64(a.hi);
    }

    template <>
    inline double get_component<3>(float64x4 a)
    {
        const __m128d ww = _mm_unpackhi_pd(a.hi, a.hi);
        return _mm_cvtsd_f64(ww);
    }

    static inline float64x4 float64x4_zero()
    {
        float64x4 result;
        result.lo =
        result.hi = _mm_setzero_pd();
        return result;
    }

    static inline float64x4 float64x4_set1(double s)
    {
        float64x4 result;
        result.lo =
        result.hi = _mm_set1_pd(s);
        return result;
    }

    static inline float64x4 float64x4_set4(double x, double y, double z, double w)
    {
        float64x4 result;
        result.lo = _mm_setr_pd(x, y);
        result.hi = _mm_setr_pd(z, w);
        return result;
    }

    static inline float64x4 float64x4_uload(const double* source)
    {
        float64x4 result;
        result.lo = _mm_loadu_pd(source + 0);
        result.hi = _mm_loadu_pd(source + 2);
        return result;
    }

    static inline void float64x4_ustore(double* dest, float64x4 a)
    {
        _mm_storeu_pd(dest + 0, a.lo);
        _mm_storeu_pd(dest + 2, a.hi);
    }

    static inline float64x4 movelh(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = a.lo;
        result.hi = b.lo;
        return result;
    }

    static inline float64x4 movehl(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = b.hi;
        result.hi = a.hi;
        return result;
    }

    static inline float64x4 unpackhi(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_unpackhi_pd(a.lo, b.lo);
        result.hi = _mm_unpackhi_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 unpacklo(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_unpacklo_pd(a.lo, b.lo);
        result.hi = _mm_unpacklo_pd(a.hi, b.hi);
        return result;
    }

    // bitwise

    static inline float64x4 bitwise_nand(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_andnot_pd(a.lo, b.lo);
        result.hi = _mm_andnot_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 bitwise_and(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_and_pd(a.lo, b.lo);
        result.hi = _mm_and_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 bitwise_or(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_or_pd(a.lo, b.lo);
        result.hi = _mm_or_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 bitwise_xor(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_xor_pd(a.lo, b.lo);
        result.hi = _mm_xor_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 min(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_min_pd(a.lo, b.lo);
        result.hi = _mm_min_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 max(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_max_pd(a.lo, b.lo);
        result.hi = _mm_max_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 hmin(float64x4 a)
    {
        const __m128d xy = _mm_min_pd(a.lo, _mm_shuffle_pd(a.lo, a.lo, 0x01));
        const __m128d zw = _mm_min_pd(a.lo, _mm_shuffle_pd(a.hi, a.hi, 0x01));
        float64x4 result;
        result.lo = _mm_min_pd(xy, _mm_shuffle_pd(zw, zw, 0x02));
        result.hi = _mm_min_pd(zw, _mm_shuffle_pd(xy, xy, 0x02));
        return result;
    }

    static inline float64x4 hmax(float64x4 a)
    {
        const __m128d xy = _mm_max_pd(a.lo, _mm_shuffle_pd(a.lo, a.lo, 0x01));
        const __m128d zw = _mm_max_pd(a.lo, _mm_shuffle_pd(a.hi, a.hi, 0x01));
        float64x4 result;
        result.lo = _mm_max_pd(xy, _mm_shuffle_pd(zw, zw, 0x02));
        result.hi = _mm_max_pd(zw, _mm_shuffle_pd(xy, xy, 0x02));
        return result;
    }

    static inline float64x4 abs(float64x4 a)
    {
        const __m128d mask = _mm_castsi128_pd(_mm_set1_epi64x(0x7fffffffffffffff));
        float64x4 result;
        result.lo = _mm_and_pd(a.lo, mask);
        result.hi = _mm_and_pd(a.hi, mask);
        return result;
    }

    static inline float64x4 neg(float64x4 a)
    {
        const __m128d mask = _mm_castsi128_pd(_mm_set1_epi64x(0x8000000000000000));
        float64x4 result;
        result.lo = _mm_xor_pd(a.lo, mask);
        result.hi = _mm_xor_pd(a.hi, mask);
        return result;
    }

    static inline float64x4 add(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_add_pd(a.lo, b.lo);
        result.hi = _mm_add_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 sub(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_sub_pd(a.lo, b.lo);
        result.hi = _mm_sub_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 mul(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_mul_pd(a.lo, b.lo);
        result.hi = _mm_mul_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 div(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_div_pd(a.lo, b.lo);
        result.hi = _mm_div_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 div(float64x4 a, double b)
    {
        const __m128d bb = _mm_set1_pd(b);
        float64x4 result;
        result.lo = _mm_div_pd(a.lo, bb);
        result.hi = _mm_div_pd(a.hi, bb);
        return result;
    }

    static inline float64x4 madd(float64x4 a, float64x4 b, float64x4 c)
    {
        float64x4 result;
        result.lo = madd(b.lo, c.lo, a.lo);
        result.hi = madd(b.hi, c.hi, a.hi);
        return result;
    }

    static inline float64x4 msub(float64x4 a, float64x4 b, float64x4 c)
    {
        float64x4 result;
        result.lo = msub(b.lo, c.lo, a.lo);
        result.hi = msub(b.hi, c.hi, a.hi);
        return result;
    }

    static inline float64x4 fast_reciprocal(float64x4 a)
    {
        const __m128d one = _mm_set1_pd(1.0);
        float64x4 result;
        result.lo = _mm_div_pd(one, a.lo);
        result.hi = _mm_div_pd(one, a.hi);
        return result;
    }

    static inline float64x4 fast_rsqrt(float64x4 a)
    {
        const __m128d one = _mm_set1_pd(1.0);
        float64x4 result;
        result.lo = _mm_div_pd(one, _mm_sqrt_pd(a.lo));
        result.hi = _mm_div_pd(one, _mm_sqrt_pd(a.hi));
        return result;
    }

    static inline float64x4 fast_sqrt(float64x4 a)
    {
        float64x4 result;
        result.lo = _mm_sqrt_pd(a.lo);
        result.hi = _mm_sqrt_pd(a.hi);
        return result;
    }

    static inline float64x4 reciprocal(float64x4 a)
    {
        const __m128d one = _mm_set1_pd(1.0);
        float64x4 result;
        result.lo = _mm_div_pd(one, a.lo);
        result.hi = _mm_div_pd(one, a.hi);
        return result;
    }

    static inline float64x4 rsqrt(float64x4 a)
    {
        const __m128d one = _mm_set1_pd(1.0);
        float64x4 result;
        result.lo = _mm_div_pd(one, _mm_sqrt_pd(a.lo));
        result.hi = _mm_div_pd(one, _mm_sqrt_pd(a.hi));
        return result;
    }

    static inline float64x4 sqrt(float64x4 a)
    {
        float64x4 result;
        result.lo = _mm_sqrt_pd(a.lo);
        result.hi = _mm_sqrt_pd(a.hi);
        return result;
    }

    static inline float64x4 dot4(float64x4 a, float64x4 b)
    {
        const __m128d xy = _mm_mul_pd(a.lo, b.lo);
        const __m128d zw = _mm_mul_pd(a.hi, b.hi);
        __m128d s;
        s = _mm_add_pd(xy, zw);
        s = _mm_add_pd(s, _mm_shuffle_pd(xy, xy, 0x01));
        s = _mm_add_pd(s, _mm_shuffle_pd(zw, zw, 0x01));

        float64x4 result;
        result.lo = s;
        result.hi = s;
        return result;
    }

    // compare

    static inline float64x4 compare_neq(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_cmpneq_pd(a.lo, b.lo);
        result.hi = _mm_cmpneq_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 compare_eq(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_cmpeq_pd(a.lo, b.lo);
        result.hi = _mm_cmpeq_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 compare_lt(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_cmplt_pd(a.lo, b.lo);
        result.hi = _mm_cmplt_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 compare_le(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_cmple_pd(a.lo, b.lo);
        result.hi = _mm_cmple_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 compare_gt(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_cmpgt_pd(a.lo, b.lo);
        result.hi = _mm_cmpgt_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 compare_ge(float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = _mm_cmpge_pd(a.lo, b.lo);
        result.hi = _mm_cmpge_pd(a.hi, b.hi);
        return result;
    }

    static inline float64x4 select(float64x4 mask, float64x4 a, float64x4 b)
    {
        float64x4 result;
        result.lo = select(mask.lo, a.lo, b.lo);
        result.hi = select(mask.hi, a.hi, b.hi);
        return result;
    }

    // rounding

    static inline float64x4 round(float64x4 s)
    {
        float64x4 result;
        result.lo = round(s.lo);
        result.hi = round(s.hi);
        return result;
    }

    static inline float64x4 trunc(float64x4 s)
    {
        float64x4 result;
        result.lo = trunc(s.lo);
        result.hi = trunc(s.hi);
        return result;
    }

    static inline float64x4 floor(float64x4 s)
    {
        float64x4 result;
        result.lo = floor(s.lo);
        result.hi = floor(s.hi);
        return result;
    }

    static inline float64x4 ceil(float64x4 s)
    {
        float64x4 result;
        result.lo = ceil(s.lo);
        result.hi = ceil(s.hi);
        return result;
    }

    static inline float64x4 fract(float64x4 s)
    {
        float64x4 result;
        result.lo = fract(s.lo);
        result.hi = fract(s.hi);
        return result;
    }

} // namespace simd
} // namespace mango
