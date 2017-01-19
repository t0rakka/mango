/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_SIMD_FLOAT
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

    // -----------------------------------------------------------------
    // conversion
    // -----------------------------------------------------------------

    static inline float32x4 float32x4_reinterpret(int32x4 s)
    {
		return (float32x4) s;
    }

    static inline float32x4 float32x4_convert(int32x4 s)
    {
        return spu_convtf(s, 0);
    }

    static inline float32x4 float32x4_convert(uint32x4 s)
    {
        return spu_convtf(s, 0);
    }

    // -----------------------------------------------------------------
    // float32x4
    // -----------------------------------------------------------------

#define SPU_SH4(n, select) \
    (n * 4 + 0) | (select << 4), \
    (n * 4 + 1) | (select << 4), \
    (n * 4 + 2) | (select << 4), \
    (n * 4 + 3) | (select << 4)

    template <int x, int y, int z, int w>
    inline float32x4 float32x4_shuffle(float32x4 v)
    {
		const vector unsigned char mask =
		{
            SPU_SH4(x, 0), SPU_SH4(y, 0), SPU_SH4(z, 0), SPU_SH4(w, 0)
		};
		return spu_shuffle(v, v, mask);
    }

    template <>
    inline float32x4 float32x4_shuffle<0, 1, 2, 3>(float32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed accessor

    template <int Index>
    static inline float32x4 float32x4_set_component(float32x4 a, float s)
    {
        return spu_insert(s, a, Index);
    }

    template <int Index>
    static inline float float32x4_get_component(float32x4 a)
    {
        return spu_extract(a, Index);
    }

    static inline float32x4 float32x4_set_x(float32x4 a, float x)
    {
        return spu_insert(x, a, 0);
    }

    static inline float32x4 float32x4_set_y(float32x4 a, float y)
    {
        return spu_insert(y, a, 1);
    }

    static inline float32x4 float32x4_set_z(float32x4 a, float z)
    {
        return spu_insert(z, a, 2);
    }

    static inline float32x4 float32x4_set_w(float32x4 a, float w)
    {
        return spu_insert(w, a, 3);
    }

    static inline float float32x4_get_x(float32x4 a)
    {
        return spu_extract(a, 0);
    }

    static inline float float32x4_get_y(float32x4 a)
    {
        return spu_extract(a, 1);
    }

    static inline float float32x4_get_z(float32x4 a)
    {
        return spu_extract(a, 2);
    }

    static inline float float32x4_get_w(float32x4 a)
    {
        return spu_extract(a, 3);
    }

    static inline float32x4 float32x4_splat_x(float32x4 a)
    {
        const float x = spu_extract(a, 0);
        return vec_splats(x);
    }
    
    static inline float32x4 float32x4_splat_y(float32x4 a)
    {
        const float y = spu_extract(a, 1);
        return vec_splats(y);
    }
    
    static inline float32x4 float32x4_splat_z(float32x4 a)
    {
        const float z = spu_extract(a, 2);
        return vec_splats(z);
    }
    
    static inline float32x4 float32x4_splat_w(float32x4 a)
    {
        const float w = spu_extract(a, 3);
        return vec_splats(w);
    }

    static inline float32x4 float32x4_zero()
    {
        return spu_splats(0.0f);
    }

    static inline float32x4 float32x4_set1(float s)
    {
        return spu_splats(s);
    }

    static inline float32x4 float32x4_set4(float x, float y, float z, float w)
    {
		const float32x4 temp = { x, y, z, w };
		return temp;
    }

    static inline float32x4 float32x4_load(const float* source)
    {
        return reinterpret_cast<const float32x4*>(source)[0];
    }

    static inline float32x4 float32x4_uload(const float* s)
    {
        float32x4 temp = { s[0], s[1], s[2], s[3] };
        return temp;
    }

    static inline void float32x4_store(float* dest, float32x4 a)
    {
        reinterpret_cast<const float32x4*>(dest)[0] = a;
    }

    static inline void float32x4_ustore(float* dest, float32x4 a)
    {
        dest[0] = spu_extract(a, 0);
        dest[1] = spu_extract(a, 1);
        dest[2] = spu_extract(a, 2);
        dest[3] = spu_extract(a, 3);
    }

    static inline float32x4 float32x4_movelh(float32x4 a, float32x4 b)
    {
        const vector unsigned char mask =
        {
            SPU_SH4(0, 0), SPU_SH4(1, 0), SPU_SH4(0, 1), SPU_SH4(1, 1)
        };
        return spu_shuffle(a, b, mask);
    }

    static inline float32x4 float32x4_movehl(float32x4 a, float32x4 b)
    {
        const vector unsigned char mask =
        {
            SPU_SH4(2, 1), SPU_SH4(3, 1), SPU_SH4(2, 0), SPU_SH4(3, 0)
        };
        return spu_shuffle(a, b, mask);
    }

    static inline float32x4 float32x4_unpackhi(float32x4 a, float32x4 b)
    {
        const vector unsigned char mask =
        {
            SPU_SH4(2, 0), SPU_SH4(2, 1), SPU_SH4(3, 0), SPU_SH4(3, 1)
        };
        return spu_shuffle(a, b, mask);
    }

    static inline float32x4 float32x4_unpacklo(float32x4 a, float32x4 b)
    {
        const vector unsigned char mask =
        {
            SPU_SH4(0, 0), SPU_SH4(0, 1), SPU_SH4(1, 0), SPU_SH4(1, 1)
        };
        return spu_shuffle(a, b, mask);
    }

#undef SPU_SH4

    // logical

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

    static inline float32x4 float32x4_min(float32x4 a, float32x4 b)
    {
        return spu_sel(a, b, spu_cmpgt(a, b));
    }

    static inline float32x4 float32x4_max(float32x4 a, float32x4 b)
    {
        return spu_sel(a, b, spu_cmpgt(b, a));
    }

    static inline float32x4 float32x4_clamp(float32x4 v, float32x4 vmin, float32x4 vmax)
    {
		return float32x4_min(vmax, float32x4_max(vmin, v));
    }

    static inline float32x4 float32x4_abs(float32x4 a)
    {
        return (vec_float4)spu_andc((vec_uint4)a, spu_splats(0x80000000));
    }

    static inline float32x4 float32x4_neg(float32x4 a)
    {
        return (vec_float4)spu_xor((vec_uint4)a, spu_splats(0x80000000));
    }

    static inline float32x4 float32x4_add(float32x4 a, float32x4 b)
    {
		return spu_add(a, b);
    }

    static inline float32x4 float32x4_sub(float32x4 a, float32x4 b)
    {
		return spu_sub(a, b);
    }

    static inline float32x4 float32x4_mul(float32x4 a, float32x4 b)
    {
		return spu_mul(a, b);
    }

    static inline float32x4 float32x4_div(float32x4 a, float32x4 b)
    {
        // Reciprocal estimate and 1 Newton-Raphson iteration.
        // Uses constant of 1.0 + 1 ulp to improve accuracy.
        const vec_float4 onen = (vec_float4)spu_splats(0x3f800001);
        const vec_float4 y0 = spu_re(b);
        const vec_float4 y0a = spu_mul(a, y0);
        return spu_madd(spu_nmsub(b, y0, onen), y0a, y0a);
    }

    static inline float32x4 float32x4_div(float32x4 a, float b)
    {
        return float32x4_div(a, spu_splats(b));
    }

    static inline float32x4 float32x4_madd(float32x4 a, float32x4 b, float32x4 c)
    {
		return spu_madd(b, c, a);
    }

    static inline float32x4 float32x4_msub(float32x4 a, float32x4 b, float32x4 c)
    {
		return spu_sub(a, spu_mul(b, c));
    }

    static inline float32x4 float32x4_fast_reciprocal(float32x4 a)
    {
		return spu_re(a);
    }

    static inline float32x4 float32x4_fast_rsqrt(float32x4 a)
    {
		return spu_rsqrte(a);
    }

    static inline float32x4 float32x4_fast_sqrt(float32x4 a)
    {
		return spu_sqrt(a);
    }

    static inline float32x4 float32x4_reciprocal(float32x4 a)
    {
        const vec_float4 onen = (vec_float4)spu_splats(0x3f800001);
        const vec_float4 y0 = spu_re(a);
        return spu_madd(spu_nmsub(a, y0, onen), y0, y0);
    }

    static inline float32x4 float32x4_rsqrt(float32x4 a)
    {
        const vec_float4 onen = (vec_float4)spu_splats(0x3f800001);
        const vec_float4 y0 = spu_rsqrte(a);
        const vec_float4 y0x = spu_mul(y0, a);
        const vec_float4 y0half = spu_mul(y0, spu_splats(0.5f));
        return spu_madd(spu_nmsub(y0, y0x, onen), y0half, y0);
    }

    static inline float32x4 float32x4_sqrt(float32x4 a)
    {
        const vec_float4 onen = (vec_float4)spu_splats(0x3f800001);
        const vec_float4 y0 = spu_rsqrte(a);
        const vec_float4 y0x = spu_mul(y0, a);
        const vec_float4 y0xhalf = spu_mul(y0x, spu_splats(0.5f));
        return spu_madd(spu_nmsub(y0, y0x, onen), y0xhalf, y0x);
    }

    static inline float32x4 float32x4_dot3(float32x4 a, float32x4 b)
    {
        float32x4 s = spu_mul(a, b);
        return spu_add(float32x4_shuffle<0, 0, 0, 0>(s),
               spu_add(float32x4_shuffle<1, 1, 1, 1>(s), float32x4_shuffle<2, 2, 2, 2>(s)));
    }

    static inline float32x4 float32x4_dot4(float32x4 a, float32x4 b)
    {
        float32x4 s = spu_mul(a, b);
        s = spu_add(s, float32x4_shuffle<2, 3, 0, 1>(s));
        s = spu_add(s, float32x4_shuffle<1, 0, 3, 2>(s));
        return s;
    }

    static inline float32x4 float32x4_cross3(float32x4 a, float32x4 b)
    {
        float32x4 c = spu_sub(spu_mul(a, float32x4_shuffle<1, 2, 0, 3>(b)),
                           spu_mul(b, float32x4_shuffle<1, 2, 0, 3>(a)));
        return float32x4_shuffle<1, 2, 0, 3>(c);
    }

    static inline float32x4 float32x4_compare_neq(float32x4 a, float32x4 b)
    {
        const vec_uint4 mask = spu_cmpeq(a, b);
        return (float32x4) spu_nor(mask, mask);
    }

    static inline float32x4 float32x4_compare_eq(float32x4 a, float32x4 b)
    {
		return (float32x4) spu_cmpeq(a, b);
    }

    static inline float32x4 float32x4_compare_lt(float32x4 a, float32x4 b)
    {
        return (float32x4) spu_cmpgt(b, a);
    }

    static inline float32x4 float32x4_compare_le(float32x4 a, float32x4 b)
    {
        const vec_uint4 mask = spu_cmpgt(a, b);
        return (float32x4) spu_nor(mask, mask);
    }

    static inline float32x4 float32x4_compare_gt(float32x4 a, float32x4 b)
    {
		return (float32x4) spu_cmpgt(a, b);
    }

    static inline float32x4 float32x4_compare_ge(float32x4 a, float32x4 b)
    {
        const vec_uint4 mask = spu_cmpgt(b, a);
        return (float32x4) spu_nor(mask, mask);
    }

    static inline float32x4 float32x4_select(float32x4 mask, float32x4 a, float32x4 b)
    {
        return spu_sel(b, a, (vec_uint4)mask);
    }

    static inline uint32 int32x4_get_mask(int32x4 a)
    {
        // TODO
        return 0;
    }

    static inline float32x4 float32x4_round(float32x4 s)
    {
        // add 0.5 (fixed precision to eliminate rounding issues)
        vec_int4 exp = spu_sub(125, spu_and(spu_rlmask((vec_int4)s, -23), 0xff));
        const vec_uint4 addend = spu_and(spu_rlmask(spu_splats((unsigned int)0x1000000), exp),
                                         spu_cmpgt((vec_uint4)exp, -31));
        const vec_float4 sa = (vec_float4)spu_add((vec_uint4)s, addend);

        // truncate the result
        exp = spu_sub(127, spu_and(spu_rlmask((vec_int4)sa, -23), 0xff));
        const vec_uint4 or_mask = spu_cmpgt(exp, 0);
        const vec_uint4 and_mask = spu_rlmask(spu_splats((unsigned int)0x7fffff), exp);
        const vec_uint4 mask = spu_or(spu_and(and_mask, spu_cmpgt(exp, -31)), or_mask);
        return spu_andc(sa, (vec_float4)mask);
    }

    static inline float32x4 float32x4_trunc(float32x4 s)
    {
        const vec_uint4 inrange = spu_cmpabsgt((float32x4)spu_splats(0x4b000000), s);
        const vec_int4 si = spu_convts(s, 0);
        return spu_sel(s, spu_convtf(si, 0), inrange);
    }

    static inline float32x4 float32x4_floor(float32x4 s)
    {
        // find truncated value and one less.
        const vec_uint4 inrange = spu_cmpabsgt((vec_float4)spu_splats(0x4b000000), s);
        const vec_int4 si = spu_convts(s, 0);
        const vec_int4 si1 = spu_add(si, -1);
        const vec_float4 truncated = spu_sel(s, spu_convtf(si, 0), inrange);
        const vec_float4 truncated1 = spu_sel(s, spu_convtf(si1, 0), inrange);

        // if truncated value is greater than input, subtract one.
        return spu_sel(truncated, truncated1, spu_cmpgt(truncated, s));
    }

    static inline float32x4 float32x4_ceil(float32x4 s)
    {
        // find truncated value and one greater.
        const vec_uint4 inrange = spu_cmpabsgt((vec_float4)spu_splats(0x4b000000), s);
        const vec_int4 si = spu_convts(s, 0);
        const vec_int4 si1 = spu_add(si, 1);
        const vec_float4 truncated = spu_sel(s, spu_convtf(si, 0), inrange);
        const vec_float4 truncated1 = spu_sel(s, spu_convtf(si1, 0), inrange);

        // if truncated value is less than input, add one.
        return spu_sel(truncated, truncated1, spu_cmpgt(s, truncated));
    }

    static inline float32x4 float32x4_fract(float32x4 s)
    {
		return spu_sub(s, float32x4_floor(s));
    }

    // -----------------------------------------------------------------
    // float <-> half conversions
    // -----------------------------------------------------------------

    static inline float32x4 float32x4_convert(float16x4 s)
    {
        float x = s.x;
        float y = s.y;
        float z = s.z;
        float w = s.w;
        return float32x4_set4(x, y, z, w);
    }

    static inline float16x4 float16x4_convert(float32x4 s)
    {
        float16x4 v;
        v.x = float32x4_get_x(s);
        v.y = float32x4_get_y(s);
        v.z = float32x4_get_z(s);
        v.w = float32x4_get_w(s);
        return v;
    }

    // -----------------------------------------------------------------
    // float32x4_matrix
    // -----------------------------------------------------------------

    /* TODO
    static inline void float32x4_matrix_set_scale(float32x4* m, float s)
    {
    }

    static inline void float32x4_matrix_set_scale(float32x4* m, float x, float y, float z)
    {
    }

    static inline void float32x4_matrix_set_translate(float32x4* m, float x, float y, float z)
    {
    }

    static inline void float32x4_matrix_scale(float32x4* m, float s)
    {
    }

    static inline void float32x4_matrix_scale(float32x4* m, float x, float y, float z)
    {
    }

    static inline void float32x4_matrix_translate(float32x4* m, float x, float y, float z)
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
