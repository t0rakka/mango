/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifdef MANGO_INCLUDE_SIMD

    // -----------------------------------------------------------------
    // helpers
    // -----------------------------------------------------------------

    static inline vector float __vec_splatsf4(const float x)
    {
        return (vector float) {x, x, x, x};
    }

    static inline vector unsigned int __vec_splatsu4(const unsigned int x)
    {
        return (vector unsigned int) {x, x, x, x};
    }

#define VEC_SH4(n, select) \
    (n * 4 + 0) | (select << 4), \
    (n * 4 + 1) | (select << 4), \
    (n * 4 + 2) | (select << 4), \
    (n * 4 + 3) | (select << 4)

    // -----------------------------------------------------------------
    // float32x4
    // -----------------------------------------------------------------

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    inline float32x4 shuffle(float32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const vector unsigned char mask =
        {
            VEC_SH4(x, 0), VEC_SH4(y, 0), VEC_SH4(z, 0), VEC_SH4(w, 0)
        };
        return vec_perm(v, v, mask);
    }

    template <>
    inline float32x4 shuffle<0, 1, 2, 3>(float32x4 v)
    {
        // .xyzw
        return v;
    }

    template <>
    inline float32x4 shuffle<0, 0, 0, 0>(float32x4 v)
    {
        // .xxxx
        return vec_splat(v, 0);
    }

    template <>
    inline float32x4 shuffle<1, 1, 1, 1>(float32x4 v)
    {
        // .yyyy
        return vec_splat(v, 1);
    }

    template <>
    inline float32x4 shuffle<2, 2, 2, 2>(float32x4 v)
    {
        // .zzzz
        return vec_splat(v, 2);
    }

    template <>
    inline float32x4 shuffle<3, 3, 3, 3>(float32x4 v)
    {
        // .wwww
        return vec_splat(v, 3);
    }

    // indexed accessor

    template <int Index>
    static inline float32x4 set_component(float32x4 a, float s)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        return vec_insert(s, a, Index);
    }

    template <int Index>
    static inline float get_component(float32x4 a)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        return vec_extract(a, Index);
    }

    static inline float32x4 float32x4_zero()
    {
        return __vec_splatsf4(0.0f);
    }

    static inline float32x4 float32x4_set1(float s)
    {
        return __vec_splatsf4(s);
    }

    static inline float32x4 float32x4_set4(float x, float y, float z, float w)
    {
        const float32x4 temp = { x, y, z, w };
        return temp;
    }

    static inline float32x4 float32x4_uload(const float* s)
    {
        return (float32x4) { s[0], s[1], s[2], s[3] };
    }

    static inline void float32x4_ustore(float* d, float32x4 a)
    {
        const float* s = reinterpret_cast<const float*>(&a);
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
    }

    static inline float32x4 movelh(float32x4 a, float32x4 b)
    {
        const vector unsigned char mask =
        {
            VEC_SH4(0, 0), VEC_SH4(1, 0), VEC_SH4(0, 1), VEC_SH4(1, 1)
        };
        return vec_perm(a, b, mask);
    }

    static inline float32x4 movehl(float32x4 a, float32x4 b)
    {
        const vector unsigned char mask =
        {
            VEC_SH4(2, 1), VEC_SH4(3, 1), VEC_SH4(2, 0), VEC_SH4(3, 0)
        };
        return vec_perm(a, b, mask);
    }

    static inline float32x4 unpackhi(float32x4 a, float32x4 b)
    {
        return vec_mergeh(b, a);
    }

    static inline float32x4 unpacklo(float32x4 a, float32x4 b)
    {
        return vec_mergel(b, a);
    }

    // bitwise

    static inline float32x4 float32x4_and(float32x4 a, float32x4 b)
    {
        return float32x4_reinterpret(int32x4_and(int32x4_reinterpret(a), int32x4_reinterpret(b)));
    }

    static inline float32x4 float32x4_nand(float32x4 a, float32x4 b)
    {
        return float32x4_reinterpret(int32x4_nand(int32x4_reinterpret(a), int32x4_reinterpret(b)));
    }

    static inline float32x4 float32x4_or(float32x4 a, float32x4 b)
    {
        return float32x4_reinterpret(int32x4_or(int32x4_reinterpret(a), int32x4_reinterpret(b)));
    }

    static inline float32x4 float32x4_xor(float32x4 a, float32x4 b)
    {
        return float32x4_reinterpret(int32x4_xor(int32x4_reinterpret(a), int32x4_reinterpret(b)));
    }

    static inline float32x4 min(float32x4 a, float32x4 b)
    {
		return vec_min(a, b);
    }

    static inline float32x4 max(float32x4 a, float32x4 b)
    {
		return vec_max(a, b);
    }

    static inline float32x4 hmin(float32x4 a)
    {
        return a; // TODO
    }

    static inline float32x4 hmax(float32x4 a)
    {
        return a; // TODO
    }

    static inline float32x4 abs(float32x4 a)
    {
		return vec_abs(a);
    }

    static inline float32x4 neg(float32x4 a)
    {
        return (vector float) vec_xor((vector unsigned int)a, __vec_splatsu4(0x80000000));
    }

    static inline float32x4 add(float32x4 a, float32x4 b)
    {
		return vec_add(a, b);
    }

    static inline float32x4 sub(float32x4 a, float32x4 b)
    {
		return vec_sub(a, b);
    }

    static inline float32x4 mul(float32x4 a, float32x4 b)
    {
		return vec_mul(a, b);
    }

    static inline float32x4 div(float32x4 a, float32x4 b)
    {
        const vector float y0 = vec_re(b);
        const vector float a0 = vec_madd(a, y0, __vec_splatsf4(0.0f));
        return vec_madd(vec_nmsub(b, y0, __vec_splatsf4(1.0f)), a0, a0);
    }

    static inline float32x4 div(float32x4 a, float b)
    {
        return vec_div(a, __vec_splatsf4(b));
    }

    static inline float32x4 madd(float32x4 a, float32x4 b, float32x4 c)
    {
		return vec_madd(b, c, a);
    }

    static inline float32x4 msub(float32x4 a, float32x4 b, float32x4 c)
    {
		return vec_sub(a, vec_mul(b, c));
    }

    static inline float32x4 fast_reciprocal(float32x4 a)
    {
		return vec_re(a);
    }

    static inline float32x4 fast_rsqrt(float32x4 a)
    {
		return vec_rsqrte(a);
    }

    static inline float32x4 fast_sqrt(float32x4 a)
    {
		return vec_sqrt(a);
    }

    static inline float32x4 reciprocal(float32x4 a)
    {
        const vector float one = __vec_splatsf4(1.0f);
        const vector float y0 = vec_re(a);
        return vec_madd(vec_nmsub(a, y0, one), y0, y0);
    }

    static inline float32x4 rsqrt(float32x4 a)
    {
        const vector float zero = __vec_splatsf4(0.0f);
        const vector float half = __vec_splatsf4(0.5f);
        const vector float one = __vec_splatsf4(1.0f);
        const vector float y0 = vec_rsqrte(a);
        const vector float y0x = vec_madd(y0, a, zero);
        const vector float y0half = vec_madd(y0, half, zero);
        return vec_madd(vec_nmsub( y0, y0x, one), y0half, y0);
    }

    static inline float32x4 sqrt(float32x4 a)
    {
        const vector float zero = __vec_splatsf4(0.0f);
        const vector float half = __vec_splatsf4(0.5f);
        const vector float one = __vec_splatsf4(1.0f);
        const vector float y0 = vec_rsqrte(a);
        const vector unsigned int cmp_zero = (vector unsigned int)vec_cmpeq(a, zero);
        const vector float y0x = vec_madd(y0, x, zero);
        const vector float y0xhalf = vec_madd(y0x, half, zero);
        return vec_sel(vec_madd(vec_nmsub(y0, y0x, one), y0xhalf, y0x), zero, cmp_zero);
    }

    static inline float32x4 dot3(float32x4 a, float32x4 b)
    {
        float32x4 s = vec_mul(a, b);
        return vec_add(shuffle<0, 0, 0, 0>(s),
               vec_add(shuffle<1, 1, 1, 1>(s), shuffle<2, 2, 2, 2>(s)));
    }

    static inline float32x4 dot4(float32x4 a, float32x4 b)
    {
        float32x4 s = vec_mul(a, b);
        s = vec_add(s, shuffle<2, 3, 0, 1>(s));
        s = vec_add(s, shuffle<1, 0, 3, 2>(s));
        return s;
    }

    static inline float32x4 cross3(float32x4 a, float32x4 b)
    {
        float32x4 c = vec_sub(vec_mul(a, shuffle<1, 2, 0, 3>(b)),
                              vec_mul(b, shuffle<1, 2, 0, 3>(a)));
        return shuffle<1, 2, 0, 3>(c);
    }

    // compare

    static inline float32x4 compare_neq(float32x4 a, float32x4 b)
    {
        const vector unsigned int mask = vec_cmpeq(a, b);
        return (float32x4) vec_nor(mask, mask);
    }

    static inline float32x4 compare_eq(float32x4 a, float32x4 b)
    {
		return vec_cmpeq(a, b);
    }

    static inline float32x4 compare_lt(float32x4 a, float32x4 b)
    {
		return vec_cmplt(a, b);
    }

    static inline float32x4 compare_le(float32x4 a, float32x4 b)
    {
		return vec_cmple(a, b);
    }

    static inline float32x4 compare_gt(float32x4 a, float32x4 b)
    {
		return vec_cmpgt(a, b);
    }

    static inline float32x4 compare_ge(float32x4 a, float32x4 b)
    {
		return vec_cmpge(a, b);
    }

    static inline float32x4 select(float32x4 mask, float32x4 a, float32x4 b)
    {
		return vec_sel(b, a, (vector unsigned int)mask);
    }

    // rounding

    static inline float32x4 round(float32x4 s)
    {
		return vec_round(s);
    }

    static inline float32x4 trunc(float32x4 s)
    {
        return vec_trunc(s);
    }

    static inline float32x4 floor(float32x4 s)
    {
		return vec_floor(s);
    }

    static inline float32x4 ceil(float32x4 s)
    {
		return vec_ceil(s);
    }

    static inline float32x4 float32x4_fract(float32x4 s)
    {
        return sub(s, floor(s));
    }

    // -----------------------------------------------------------------
    // float32x4_matrix
    // -----------------------------------------------------------------

    /*
    static inline void float32x4_matrix_set_scale(float32x4* result, float s)
    {
    }

    static inline void float32x4_matrix_set_scale(float32x4* result, float x, float y, float z)
    {
    }

    static inline void float32x4_matrix_set_translate(float32x4* result, float x, float y, float z)
    {
    }

    static inline void float32x4_matrix_scale(float32x4* result, float s)
    {
    }

    static inline void float32x4_matrix_scale(float32x4* result, float x, float y, float z)
    {
    }

    static inline void float32x4_matrix_translate(float32x4* result, float x, float y, float z)
    {
    }

    static inline void float32x4_matrix_transpose(float32x4* result, const float32x4* m)
    {
    }

    static inline void float32x4_matrix_inverse(float32x4* result, const float32x4* m)
    {
    }

    static inline void float32x4_matrix_inverse_transpose(float32x4* result, const float32x4* m)
    {
    }

    static inline float32x4 float32x4_vector_matrix_multiply(float32x4 v, const float32x4* m)
    {
    }

    static inline void float32x4_matrix_matrix_multiply(float32x4* result, const float32x4* a, const float32x4* b)
    {
    }
    */

#undef VEC_SH4

#endif // MANGO_INCLUDE_SIMD
