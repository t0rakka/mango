/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifdef MANGO_INCLUDE_SIMD

    // -----------------------------------------------------------------
    // float64x2
    // -----------------------------------------------------------------

    /*
    // TODO

    template <uint32 x, uint32 y>
    static inline float64x2 float64x2_shuffle(float64x2 v)
    {
        static_assert(x < 2 && y < 2, "Index out of range.");
    }

    // set component

    template <int Index>
    static inline float64x2 float64x2_set_component(float64x2 a, double s);

    template <>
    inline float64x2 float64x2_set_component<0>(float64x2 a, double x)
    {
    }

    template <>
    inline float64x2 float64x2_set_component<1>(float64x2 a, double y)
    {
    }

    // get component

    template <int Index>
    static inline double float64x2_get_component(float64x2 a);

    template <>
    inline double float64x2_get_component<0>(float64x2 a)
    {
    }

    template <>
    inline double float64x2_get_component<1>(float64x2 a)
    {
    }

    static inline float64x2 float64x2_zero()
    {
    }

    static inline float64x2 float64x2_set1(double s)
    {
    }

    static inline float64x2 float64x2_set2(double x, double y)
    {
    }

    static inline float64x2 float64x2_uload(const double* source)
    {
    }

    static inline void float64x2_ustore(double* dest, float64x2 a)
    {
    }

    static inline float64x2 float64x2_unpackhi(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_unpacklo(float64x2 a, float64x2 b)
    {
    }

    // logical

    static inline float64x2 float64x2_and(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_nand(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_or(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_xor(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_min(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_max(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_abs(float64x2 a)
    {
    }

    static inline float64x2 float64x2_neg(float64x2 a)
    {
    }

    static inline float64x2 float64x2_add(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_sub(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_mul(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_div(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_div(float64x2 a, double b)
    {
    }

    static inline float64x2 float64x2_madd(float64x2 a, float64x2 b, float64x2 c)
    {
    }

    static inline float64x2 float64x2_msub(float64x2 a, float64x2 b, float64x2 c)
    {
    }

    static inline float64x2 float64x2_fast_reciprocal(float64x2 a)
    {
    }

    static inline float64x2 float64x2_fast_rsqrt(float64x2 a)
    {
    }

    static inline float64x2 float64x2_fast_sqrt(float64x2 a)
    {
    }

    static inline float64x2 float64x2_reciprocal(float64x2 a)
    {
    }

    static inline float64x2 float64x2_rsqrt(float64x2 a)
    {
    }

    static inline float64x2 float64x2_sqrt(float64x2 a)
    {
    }

    static inline float64x2 float64x2_dot2(float64x2 a, float64x2 b)
    {
    }

    // compare

    static inline float64x2 float64x2_compare_neq(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_compare_eq(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_compare_lt(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_compare_le(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_compare_gt(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_compare_ge(float64x2 a, float64x2 b)
    {
    }

    static inline float64x2 float64x2_select(float64x2 mask, float64x2 a, float64x2 b)
    {
    }

    // rounding

    static inline float64x2 float64x2_round(float64x2 s)
    {
    }

    static inline float64x2 float64x2_trunc(float64x2 s)
    {
    }

    static inline float64x2 float64x2_floor(float64x2 s)
    {
    }

    static inline float64x2 float64x2_ceil(float64x2 s)
    {
    }

    static inline float64x2 float64x2_fract(float64x2 s)
    {
    }
    */

#endif // MANGO_INCLUDE_SIMD
