/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_SIMD_FLOAT
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

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

    // -----------------------------------------------------------------
    // simd4f
    // -----------------------------------------------------------------

    // conversion

    static inline simd4f simd4f_cast(__simd4i s)
    {
		return (simd4f) s;
    }

    static inline simd4f simd4f_convert(__simd4i s)
    {
		return vec_ctf(s, 0);
    }

    static inline simd4f simd4f_unsigned_convert(__simd4i s)
    {
		return vec_ctf((const vector unsigned int)s, 0);
    }

#define VEC_SH4(n, select) \
    (n * 4 + 0) | (select << 4), \
    (n * 4 + 1) | (select << 4), \
    (n * 4 + 2) | (select << 4), \
    (n * 4 + 3) | (select << 4)

    template <int x, int y, int z, int w>
    inline simd4f simd4f_shuffle(__simd4f v)
    {
        const vector unsigned char mask =
        {
            VEC_SH4(x, 0), VEC_SH4(y, 0), VEC_SH4(z, 0), VEC_SH4(w, 0)
        };
        return vec_perm(v, v, mask);
    }

    template <>
    inline simd4f simd4f_shuffle<0, 1, 2, 3>(__simd4f v)
    {
        // .xyzw
        return v;
    }

    template <>
    inline simd4f simd4f_shuffle<0, 0, 0, 0>(__simd4f v)
    {
        // .xxxx
        return vec_splat(v, 0);
    }

    template <>
    inline simd4f simd4f_shuffle<1, 1, 1, 1>(__simd4f v)
    {
        // .yyyy
        return vec_splat(v, 1);
    }

    template <>
    inline simd4f simd4f_shuffle<2, 2, 2, 2>(__simd4f v)
    {
        // .zzzz
        return vec_splat(v, 2);
    }

    template <>
    inline simd4f simd4f_shuffle<3, 3, 3, 3>(__simd4f v)
    {
        // .wwww
        return vec_splat(v, 3);
    }

    // indexed accessor

    template <int Index>
    static inline simd4f simd4f_set_component(__simd4f a, float s)
    {
        return vec_insert(s, a, Index);
    }

    template <int Index>
    static inline float simd4f_get_component(__simd4f a)
    {
        return vec_extract(a, Index);
    }

    static inline simd4f simd4f_set_x(__simd4f a, float x)
    {
        return vec_insert(x, a, 0);
    }

    static inline simd4f simd4f_set_y(__simd4f a, float y)
    {
        return vec_insert(y, a, 1);
    }

    static inline simd4f simd4f_set_z(__simd4f a, float z)
    {
        return vec_insert(z, a, 2);
    }

    static inline simd4f simd4f_set_w(__simd4f a, float w)
    {
        return vec_insert(w, a, 3);
    }

    static inline float simd4f_get_x(__simd4f a)
    {
        return vec_extract(a, 0);
    }

    static inline float simd4f_get_y(__simd4f a)
    {
        return vec_extract(a, 1);
    }

    static inline float simd4f_get_z(__simd4f a)
    {
        return vec_extract(a, 2);
    }

    static inline float simd4f_get_w(__simd4f a)
    {
        return vec_extract(a, 3);

    }

    static inline simd4f simd4f_splat_x(__simd4f a)
    {
        return vec_splat(a, 0);
    }
    
    static inline simd4f simd4f_splat_y(__simd4f a)
    {
        return vec_splat(a, 1);
    }
    
    static inline simd4f simd4f_splat_z(__simd4f a)
    {
        return vec_splat(a, 2);
    }

    static inline simd4f simd4f_splat_w(__simd4f a)
    {
        return vec_splat(a, 3);
    }

    static inline simd4f simd4f_zero()
    {
        return __vec_splatsf4(0.0f);
    }

    static inline simd4f simd4f_set1(float s)
    {
        return __vec_splatsf4(s);
    }

    static inline simd4f simd4f_set4(float x, float y, float z, float w)
    {
        const simd4f temp = { x, y, z, w };
        return temp;
    }

    static inline simd4f simd4f_load(const float* s)
    {
        return reinterpret_cast<const simd4f*>(s)[0];
    }

    static inline simd4f simd4f_uload(const float* s)
    {
        return (simd4f) { s[0], s[1], s[2], s[3] };
    }

    static inline void simd4f_store(float* d, __simd4f a)
    {
        reinterpret_cast<const simd4f*>(d)[0] = a;
    }

    static inline void simd4f_ustore(float* d, __simd4f a)
    {
        const float* s = reinterpret_cast<const float*>(&a);
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
    }

    static inline simd4f simd4f_movelh(__simd4f a, __simd4f b)
    {
        const vector unsigned char mask =
        {
            VEC_SH4(0, 0), VEC_SH4(1, 0), VEC_SH4(0, 1), VEC_SH4(1, 1)
        };
        return vec_perm(a, b, mask);
    }

    static inline simd4f simd4f_movehl(__simd4f a, __simd4f b)
    {
        const vector unsigned char mask =
        {
            VEC_SH4(2, 1), VEC_SH4(3, 1), VEC_SH4(2, 0), VEC_SH4(3, 0)
        };
        return vec_perm(a, b, mask);
    }

    static inline simd4f simd4f_unpackhi(__simd4f a, __simd4f b)
    {
        return vec_mergeh(b, a);
    }

    static inline simd4f simd4f_unpacklo(__simd4f a, __simd4f b)
    {
        return vec_mergel(b, a);
    }

#undef VEC_SH4

    // logical

    static inline simd4f simd4f_and(__simd4f a, __simd4f b)
    {
        return simd4f_cast(simd4i_and(simd4i_cast(a), simd4i_cast(b)));
    }

    static inline simd4f simd4f_nand(__simd4f a, __simd4f b)
    {
        return simd4f_cast(simd4i_nand(simd4i_cast(a), simd4i_cast(b)));
    }

    static inline simd4f simd4f_or(__simd4f a, __simd4f b)
    {
        return simd4f_cast(simd4i_or(simd4i_cast(a), simd4i_cast(b)));
    }

    static inline simd4f simd4f_xor(__simd4f a, __simd4f b)
    {
        return simd4f_cast(simd4i_xor(simd4i_cast(a), simd4i_cast(b)));
    }

    static inline simd4f simd4f_min(__simd4f a, __simd4f b)
    {
		return vec_min(a, b);
    }

    static inline simd4f simd4f_max(__simd4f a, __simd4f b)
    {
		return vec_max(a, b);
    }

    static inline simd4f simd4f_clamp(__simd4f v, __simd4f vmin, __simd4f vmax)
    {
		return vec_min(vmax, vec_max(vmin, v));
    }

    static inline simd4f simd4f_abs(__simd4f a)
    {
		return vec_abs(a);
    }

    static inline simd4f simd4f_neg(__simd4f a)
    {
        return (vector float) vec_xor((vector unsigned int)a, __vec_splatsu4(0x80000000));
    }

    static inline simd4f simd4f_add(__simd4f a, __simd4f b)
    {
		return vec_add(a, b);
    }

    static inline simd4f simd4f_sub(__simd4f a, __simd4f b)
    {
		return vec_sub(a, b);
    }

    static inline simd4f simd4f_mul(__simd4f a, __simd4f b)
    {
		return vec_mul(a, b);
    }

    static inline simd4f simd4f_div(__simd4f a, __simd4f b)
    {
        const vector float y0 = vec_re(b);
        const vector float a0 = vec_madd(a, y0, __vec_splatsf4(0.0f));
        return vec_madd(vec_nmsub(b, y0, __vec_splatsf4(1.0f)), a0, a0);
    }

    static inline simd4f simd4f_div(__simd4f a, float b)
    {
        return vec_div(a, __vec_splatsf4(b));
    }

    static inline simd4f simd4f_madd(__simd4f a, __simd4f b, __simd4f c)
    {
		return vec_madd(b, c, a);
    }

    static inline simd4f simd4f_msub(__simd4f a, __simd4f b, __simd4f c)
    {
		return vec_sub(a, vec_mul(b, c));
    }

    static inline simd4f simd4f_fast_reciprocal(__simd4f a)
    {
		return vec_re(a);
    }

    static inline simd4f simd4f_fast_rsqrt(__simd4f a)
    {
		return vec_rsqrte(a);
    }

    static inline simd4f simd4f_fast_sqrt(__simd4f a)
    {
		return vec_sqrt(a);
    }

    static inline simd4f simd4f_reciprocal(__simd4f a)
    {
        const vector float one = __vec_splatsf4(1.0f);
        const vector float y0 = vec_re(a);
        return vec_madd(vec_nmsub(a, y0, one), y0, y0);
    }

    static inline simd4f simd4f_rsqrt(__simd4f a)
    {
        const vector float zero = __vec_splatsf4(0.0f);
        const vector float half = __vec_splatsf4(0.5f);
        const vector float one = __vec_splatsf4(1.0f);
        const vector float y0 = vec_rsqrte(a);
        const vector float y0x = vec_madd(y0, a, zero);
        const vector float y0half = vec_madd(y0, half, zero);
        return vec_madd(vec_nmsub( y0, y0x, one), y0half, y0);
    }

    static inline simd4f simd4f_sqrt(__simd4f a)
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

    static inline simd4f simd4f_dot3(__simd4f a, __simd4f b)
    {
        simd4f s = vec_mul(a, b);
        return vec_add(simd4f_shuffle<0, 0, 0, 0>(s),
               vec_add(simd4f_shuffle<1, 1, 1, 1>(s), simd4f_shuffle<2, 2, 2, 2>(s)));
    }

    static inline simd4f simd4f_dot4(__simd4f a, __simd4f b)
    {
        simd4f s = vec_mul(a, b);
        s = vec_add(s, simd4f_shuffle<2, 3, 0, 1>(s));
        s = vec_add(s, simd4f_shuffle<1, 0, 3, 2>(s));
        return s;
    }

    static inline simd4f simd4f_cross3(__simd4f a, __simd4f b)
    {
        simd4f c = vec_sub(vec_mul(a, simd4f_shuffle<1, 2, 0, 3>(b)),
                           vec_mul(b, simd4f_shuffle<1, 2, 0, 3>(a)));
        return simd4f_shuffle<1, 2, 0, 3>(c);
    }

    static inline simd4f simd4f_compare_neq(__simd4f a, __simd4f b)
    {
        const vector unsigned int mask = vec_cmpeq(a, b);
        return (simd4f) vec_nor(mask, mask);
    }

    static inline simd4f simd4f_compare_eq(__simd4f a, __simd4f b)
    {
		return vec_cmpeq(a, b);
    }

    static inline simd4f simd4f_compare_lt(__simd4f a, __simd4f b)
    {
		return vec_cmplt(a, b);
    }

    static inline simd4f simd4f_compare_le(__simd4f a, __simd4f b)
    {
		return vec_cmple(a, b);
    }

    static inline simd4f simd4f_compare_gt(__simd4f a, __simd4f b)
    {
		return vec_cmpgt(a, b);
    }

    static inline simd4f simd4f_compare_ge(__simd4f a, __simd4f b)
    {
		return vec_cmpge(a, b);
    }

    static inline simd4f simd4f_select(__simd4f mask, __simd4f a, __simd4f b)
    {
		return vec_sel(b, a, (vector unsigned int)mask);
    }

    static inline simd4f simd4f_round(__simd4f s)
    {
		return vec_round(s);
    }

    static inline simd4f simd4f_trunc(__simd4f s)
    {
        return vec_trunc(s);
    }

    static inline simd4f simd4f_floor(__simd4f s)
    {
		return vec_floor(s);
    }

    static inline simd4f simd4f_ceil(__simd4f s)
    {
		return vec_ceil(s);
    }

    static inline simd4f simd4f_fract(__simd4f s)
    {
        return simd4f_sub(s, simd4f_floor(s));
    }

    // -----------------------------------------------------------------
    // float <-> half conversions
    // -----------------------------------------------------------------

    static inline simd4f simd4f_convert(__simd4h s)
    {
        float x = s.x;
        float y = s.y;
        float z = s.z;
        float w = s.w;
        return simd4f_set4(x, y, z, w);
    }

    static inline simd4h simd4h_convert(__simd4f s)
    {
        simd4h v;
        v.x = simd4f_get_x(s);
        v.y = simd4f_get_y(s);
        v.z = simd4f_get_z(s);
        v.w = simd4f_get_w(s);
        return v;
    }

    // -----------------------------------------------------------------
    // simd4f_matrix
    // -----------------------------------------------------------------

    /*
    static inline void simd4f_matrix_set_scale(simd4f* result, float s)
    {
    }

    static inline void simd4f_matrix_set_scale(simd4f* result, float x, float y, float z)
    {
    }

    static inline void simd4f_matrix_set_translate(simd4f* result, float x, float y, float z)
    {
    }

    static inline void simd4f_matrix_scale(simd4f* result, float s)
    {
    }

    static inline void simd4f_matrix_scale(simd4f* result, float x, float y, float z)
    {
    }

    static inline void simd4f_matrix_translate(simd4f* result, float x, float y, float z)
    {
    }

    static inline void simd4f_matrix_transpose(simd4f* result, const simd4f* m)
    {
    }

    static inline void simd4f_matrix_inverse(simd4f* result, const simd4f* m)
    {
    }

    static inline void simd4f_matrix_inverse_transpose(simd4f* result, const simd4f* m)
    {
    }

    static inline simd4f simd4f_vector_matrix_multiply(__simd4f v, const simd4f* m)
    {
    }

    static inline void simd4f_matrix_matrix_multiply(simd4f* result, const simd4f* a, const simd4f* b)
    {
    }
    */
