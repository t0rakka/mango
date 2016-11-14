/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_INCLUDE_SIMD
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

#include <cmath>
#include <algorithm>

namespace mango
{

    // -----------------------------------------------------------------
    // Cell BE SPU
    // -----------------------------------------------------------------

    struct simd4h
    {
        half x, y, z, w;
    };

    typedef vector float simd4f;
    typedef vector signed int simd4i;

    typedef const simd4h& __simd4h;
    typedef const vector float __simd4f;
    typedef const vector signed int __simd4i;

    // -----------------------------------------------------------------
    // conversions
    // -----------------------------------------------------------------

    static inline simd4f simd4f_cast(__simd4i s)
    {
		return (simd4f) s;
    }

    static inline simd4i simd4i_cast(__simd4f s)
    {
		return (simd4i) s;
    }

    static inline simd4f simd4f_convert(__simd4i s)
    {
        return spu_convtf(s, 0);
    }

    static inline simd4f simd4f_unsigned_convert(__simd4i s)
    {
        return spu_convtf((const vector unsigned int)s, 0);
    }

    static inline simd4i simd4i_convert(__simd4f s)
    {
        return spu_convts(s, 0);
    }

    static inline simd4i simd4i_truncate(__simd4f s)
    {
        const vec_uint4 inrange = spu_cmpabsgt((simd4f)spu_splats(0x4b000000), s);
        const vec_int4 si = spu_convts(s, 0);
        const vec_float4 st = spu_sel(s, spu_convtf(si, 0), inrange);
        return spu_convts(st, 0);
    }

    // -----------------------------------------------------------------
    // simd4i
    // -----------------------------------------------------------------

    // set

    static inline simd4i simd4i_set_x(__simd4i a, int x)
    {
        return spu_insert(x, a, 0);
    }

    static inline simd4i simd4i_set_y(__simd4i a, int y)
    {
        return spu_insert(y, a, 1);
    }

    static inline simd4i simd4i_set_z(__simd4i a, int z)
    {
        return spu_insert(z, a, 2);
    }

    static inline simd4i simd4i_set_w(__simd4i a, int w)
    {
        return spu_insert(w, a, 3);
    }

    // get

    static inline int simd4i_get_x(__simd4i a)
    {
        return spu_extract(a, 0);
    }

    static inline int simd4i_get_y(__simd4i a)
    {
        return spu_extract(a, 1);
    }

    static inline int simd4i_get_z(__simd4i a)
    {
        return spu_extract(a, 2);
    }

    static inline int simd4i_get_w(__simd4i a)
    {
        return spu_extract(a, 3);
    }

    static inline simd4i simd4i_load(const int* source)
    {
        return reinterpret_cast<const simd4i*>(source)[0];
    }

    static inline simd4i simd4i_uload(const int* s)
    {
        simd4i temp = { s[0], s[1], s[2], s[3] };
        return temp;
    }

    static inline void simd4i_store(int* dest, __simd4i a)
    {
        reinterpret_cast<const simd4i*>(dest)[0] = a;
    }

    static inline void simd4i_ustore(int* dest, __simd4i a)
    {
        dest[0] = spu_extract(a, 0);
        dest[1] = spu_extract(a, 1);
        dest[2] = spu_extract(a, 2);
        dest[3] = spu_extract(a, 3);
    }

    static inline simd4i simd4i_zero()
    {
        return spu_splats(0);
    }

    static inline simd4i simd4i_set1(int s)
    {
        return spu_splats(s);
    }

    static inline simd4i simd4i_set4(int x, int y, int z, int w)
    {
		const simd4i temp = { x, y, z, w };
		return temp;
    }

    static inline simd4i simd4i_neg(__simd4i a)
    {
        return spu_sub(spu_splats(0), a);
    }

    static inline simd4i simd4i_add(__simd4i a, __simd4i b)
    {
		return spu_add(a, b);
    }

    static inline simd4i simd4i_sub(__simd4i a, __simd4i b)
    {
		return spu_sub(a, b);
    }

    // logical

    static inline simd4i simd4i_and(__simd4i a, __simd4i b)
    {
		return spu_and(a, b);
    }

    static inline simd4i simd4i_nand(__simd4i a, __simd4i b)
    {
        return spu_nand(a, b);
    }

    static inline simd4i simd4i_or(__simd4i a, __simd4i b)
    {
		return spu_or(a, b);
    }

    static inline simd4i simd4i_xor(__simd4i a, __simd4i b)
    {
		return spu_xor(a, b);
    }

    static inline simd4i simd4i_sll(__simd4i a, int b)
    {
        return spu_sl(a, b);
    }

    static inline simd4i simd4i_srl(__simd4i a, int b)
    {
        return spu_sr(a, b);
    }

    static inline simd4i simd4i_sra(__simd4i a, int b)
    {
        return spu_sra(a, b);
    }

    static inline simd4i simd4i_compare_eq(__simd4i a, __simd4i b)
    {
		return (simd4i) spu_cmpeq(a, b);
    }

    static inline simd4i simd4i_compare_gt(__simd4i a, __simd4i b)
    {
		return (simd4i) spu_cmpgt(a, b);
    }

    static inline simd4i simd4i_select(__simd4i mask, __simd4i a, __simd4i b)
    {
		return spu_sel(b, a, (vec_uint4)mask);
    }

    static inline uint32 simd4i_pack(__simd4i s)
    {
        unsigned int* p = (unsigned int*)&s;
        return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    }

    static inline simd4i simd4i_unpack(uint32 s)
    {
        const int x = (s >> 0) & 0xff;
        const int y = (s >> 8) & 0xff;
        const int z = (s >> 16) & 0xff;
        const int w = (s >> 24);
        return simd4i_set4(x, y, z, w);
    }

    // -----------------------------------------------------------------
    // simd4f
    // -----------------------------------------------------------------

#define SPU_SH4(n, select) \
    (n * 4 + 0) | (select << 4), \
    (n * 4 + 1) | (select << 4), \
    (n * 4 + 2) | (select << 4), \
    (n * 4 + 3) | (select << 4)

    template <int x, int y, int z, int w>
    inline simd4f simd4f_shuffle(__simd4f v)
    {
		const vector unsigned char mask =
		{
            SPU_SH4(x, 0), SPU_SH4(y, 0), SPU_SH4(z, 0), SPU_SH4(w, 0)
		};
		return spu_shuffle(v, v, mask);
    }

    template <>
    inline simd4f simd4f_shuffle<0, 1, 2, 3>(__simd4f v)
    {
        // .xyzw
        return v;
    }

    // indexed accessor

    template <int Index>
    static inline simd4f simd4f_set_component(__simd4f a, float s)
    {
        return spu_insert(s, a, Index);
    }

    template <int Index>
    static inline float simd4f_get_component(__simd4f a)
    {
        return spu_extract(a, Index);
    }

    static inline simd4f simd4f_set_x(__simd4f a, float x)
    {
        return spu_insert(x, a, 0);
    }

    static inline simd4f simd4f_set_y(__simd4f a, float y)
    {
        return spu_insert(y, a, 1);
    }

    static inline simd4f simd4f_set_z(__simd4f a, float z)
    {
        return spu_insert(z, a, 2);
    }

    static inline simd4f simd4f_set_w(__simd4f a, float w)
    {
        return spu_insert(w, a, 3);
    }

    static inline float simd4f_get_x(__simd4f a)
    {
        return spu_extract(a, 0);
    }

    static inline float simd4f_get_y(__simd4f a)
    {
        return spu_extract(a, 1);
    }

    static inline float simd4f_get_z(__simd4f a)
    {
        return spu_extract(a, 2);
    }

    static inline float simd4f_get_w(__simd4f a)
    {
        return spu_extract(a, 3);
    }

    static inline simd4f simd4f_splat_x(__simd4f a)
    {
        const float x = spu_extract(a, 0);
        return vec_splats(x);
    }
    
    static inline simd4f simd4f_splat_y(__simd4f a)
    {
        const float y = spu_extract(a, 1);
        return vec_splats(y);
    }
    
    static inline simd4f simd4f_splat_z(__simd4f a)
    {
        const float z = spu_extract(a, 2);
        return vec_splats(z);
    }
    
    static inline simd4f simd4f_splat_w(__simd4f a)
    {
        const float w = spu_extract(a, 3);
        return vec_splats(w);
    }

    static inline simd4f simd4f_zero()
    {
        return spu_splats(0.0f);
    }

    static inline simd4f simd4f_set1(float s)
    {
        return spu_splats(s);
    }

    static inline simd4f simd4f_set4(float x, float y, float z, float w)
    {
		const simd4f temp = { x, y, z, w };
		return temp;
    }

    static inline simd4f simd4f_load(const float* source)
    {
        return reinterpret_cast<const simd4f*>(source)[0];
    }

    static inline simd4f simd4f_uload(const float* s)
    {
        simd4f temp = { s[0], s[1], s[2], s[3] };
        return temp;
    }

    static inline void simd4f_store(float* dest, __simd4f a)
    {
        reinterpret_cast<const simd4f*>(dest)[0] = a;
    }

    static inline void simd4f_ustore(float* dest, __simd4f a)
    {
        dest[0] = spu_extract(a, 0);
        dest[1] = spu_extract(a, 1);
        dest[2] = spu_extract(a, 2);
        dest[3] = spu_extract(a, 3);
    }

    static inline simd4f simd4f_movelh(__simd4f a, __simd4f b)
    {
        const vector unsigned char mask =
        {
            SPU_SH4(0, 0), SPU_SH4(1, 0), SPU_SH4(0, 1), SPU_SH4(1, 1)
        };
        return spu_shuffle(a, b, mask);
    }

    static inline simd4f simd4f_movehl(__simd4f a, __simd4f b)
    {
        const vector unsigned char mask =
        {
            SPU_SH4(2, 1), SPU_SH4(3, 1), SPU_SH4(2, 0), SPU_SH4(3, 0)
        };
        return spu_shuffle(a, b, mask);
    }

    static inline simd4f simd4f_unpackhi(__simd4f a, __simd4f b)
    {
        const vector unsigned char mask =
        {
            SPU_SH4(2, 0), SPU_SH4(2, 1), SPU_SH4(3, 0), SPU_SH4(3, 1)
        };
        return spu_shuffle(a, b, mask);
    }

    static inline simd4f simd4f_unpacklo(__simd4f a, __simd4f b)
    {
        const vector unsigned char mask =
        {
            SPU_SH4(0, 0), SPU_SH4(0, 1), SPU_SH4(1, 0), SPU_SH4(1, 1)
        };
        return spu_shuffle(a, b, mask);
    }

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
        return spu_sel(a, b, spu_cmpgt(a, b));
    }

    static inline simd4f simd4f_max(__simd4f a, __simd4f b)
    {
        return spu_sel(a, b, spu_cmpgt(b, a));
    }

    static inline simd4f simd4f_clamp(__simd4f v, __simd4f vmin, __simd4f vmax)
    {
		return simd4f_min(vmax, simd4f_max(vmin, v));
    }

    static inline simd4f simd4f_abs(__simd4f a)
    {
        return (vec_float4)spu_andc((vec_uint4)a, spu_splats(0x80000000));
    }

    static inline simd4f simd4f_neg(__simd4f a)
    {
        return (vec_float4)spu_xor((vec_uint4)a, spu_splats(0x80000000));
    }

    static inline simd4f simd4f_add(__simd4f a, __simd4f b)
    {
		return spu_add(a, b);
    }

    static inline simd4f simd4f_sub(__simd4f a, __simd4f b)
    {
		return spu_sub(a, b);
    }

    static inline simd4f simd4f_mul(__simd4f a, __simd4f b)
    {
		return spu_mul(a, b);
    }

    static inline simd4f simd4f_div(__simd4f a, __simd4f b)
    {
        // Reciprocal estimate and 1 Newton-Raphson iteration.
        // Uses constant of 1.0 + 1 ulp to improve accuracy.
        const vec_float4 onen = (vec_float4)spu_splats(0x3f800001);
        const vec_float4 y0 = spu_re(b);
        const vec_float4 y0a = spu_mul(a, y0);
        return spu_madd(spu_nmsub(b, y0, onen), y0a, y0a);
    }

    static inline simd4f simd4f_div(__simd4f a, float b)
    {
        return simd4f_div(a, spu_splats(b));
    }

    static inline simd4f simd4f_madd(__simd4f a, __simd4f b, __simd4f c)
    {
		return spu_madd(b, c, a);
    }

    static inline simd4f simd4f_msub(__simd4f a, __simd4f b, __simd4f c)
    {
		return spu_sub(a, spu_mul(b, c));
    }

    static inline simd4f simd4f_fast_reciprocal(__simd4f a)
    {
		return spu_re(a);
    }

    static inline simd4f simd4f_fast_rsqrt(__simd4f a)
    {
		return spu_rsqrte(a);
    }

    static inline simd4f simd4f_fast_sqrt(__simd4f a)
    {
		return spu_sqrt(a);
    }

    static inline simd4f simd4f_reciprocal(__simd4f a)
    {
        const vec_float4 onen = (vec_float4)spu_splats(0x3f800001);
        const vec_float4 y0 = spu_re(a);
        return spu_madd(spu_nmsub(a, y0, onen), y0, y0);
    }

    static inline simd4f simd4f_rsqrt(__simd4f a)
    {
        const vec_float4 onen = (vec_float4)spu_splats(0x3f800001);
        const vec_float4 y0 = spu_rsqrte(a);
        const vec_float4 y0x = spu_mul(y0, a);
        const vec_float4 y0half = spu_mul(y0, spu_splats(0.5f));
        return spu_madd(spu_nmsub(y0, y0x, onen), y0half, y0);
    }

    static inline simd4f simd4f_sqrt(__simd4f a)
    {
        const vec_float4 onen = (vec_float4)spu_splats(0x3f800001);
        const vec_float4 y0 = spu_rsqrte(a);
        const vec_float4 y0x = spu_mul(y0, a);
        const vec_float4 y0xhalf = spu_mul(y0x, spu_splats(0.5f));
        return spu_madd(spu_nmsub(y0, y0x, onen), y0xhalf, y0x);
    }

    static inline simd4f simd4f_dot3(__simd4f a, __simd4f b)
    {
        simd4f s = spu_mul(a, b);
        return spu_add(simd4f_shuffle<0, 0, 0, 0>(s),
               spu_add(simd4f_shuffle<1, 1, 1, 1>(s), simd4f_shuffle<2, 2, 2, 2>(s)));
    }

    static inline simd4f simd4f_dot4(__simd4f a, __simd4f b)
    {
        simd4f s = spu_mul(a, b);
        s = spu_add(s, simd4f_shuffle<2, 3, 0, 1>(s));
        s = spu_add(s, simd4f_shuffle<1, 0, 3, 2>(s));
        return s;
    }

    static inline simd4f simd4f_cross3(__simd4f a, __simd4f b)
    {
        simd4f c = spu_sub(spu_mul(a, simd4f_shuffle<1, 2, 0, 3>(b)),
                           spu_mul(b, simd4f_shuffle<1, 2, 0, 3>(a)));
        return simd4f_shuffle<1, 2, 0, 3>(c);
    }

    static inline simd4f simd4f_compare_neq(__simd4f a, __simd4f b)
    {
        const vec_uint4 mask = spu_cmpeq(a, b);
        return (simd4f) spu_nor(mask, mask);
    }

    static inline simd4f simd4f_compare_eq(__simd4f a, __simd4f b)
    {
		return (simd4f) spu_cmpeq(a, b);
    }

    static inline simd4f simd4f_compare_lt(__simd4f a, __simd4f b)
    {
        return (simd4f) spu_cmpgt(b, a);
    }

    static inline simd4f simd4f_compare_le(__simd4f a, __simd4f b)
    {
        const vec_uint4 mask = spu_cmpgt(a, b);
        return (simd4f) spu_nor(mask, mask);
    }

    static inline simd4f simd4f_compare_gt(__simd4f a, __simd4f b)
    {
		return (simd4f) spu_cmpgt(a, b);
    }

    static inline simd4f simd4f_compare_ge(__simd4f a, __simd4f b)
    {
        const vec_uint4 mask = spu_cmpgt(b, a);
        return (simd4f) spu_nor(mask, mask);
    }

    static inline simd4f simd4f_select(__simd4f mask, __simd4f a, __simd4f b)
    {
        return spu_sel(b, a, (vec_uint4)mask);
    }

    static inline uint32 simd4i_get_mask(__simd4i a)
    {
        // TODO
        return 0;
    }

    static inline simd4f simd4f_round(__simd4f s)
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

    static inline simd4f simd4f_trunc(__simd4f s)
    {
        const vec_uint4 inrange = spu_cmpabsgt((simd4f)spu_splats(0x4b000000), s);
        const vec_int4 si = spu_convts(s, 0);
        return spu_sel(s, spu_convtf(si, 0), inrange);
    }

    static inline simd4f simd4f_floor(__simd4f s)
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

    static inline simd4f simd4f_ceil(__simd4f s)
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

    static inline simd4f simd4f_fract(__simd4f s)
    {
		return spu_sub(s, simd4f_floor(s));
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

    /* TODO
    static inline void simd4f_matrix_set_scale(simd4f* m, float s)
    {
    }

    static inline void simd4f_matrix_set_scale(simd4f* m, float x, float y, float z)
    {
    }

    static inline void simd4f_matrix_set_translate(simd4f* m, float x, float y, float z)
    {
    }

    static inline void simd4f_matrix_scale(simd4f* m, float s)
    {
    }

    static inline void simd4f_matrix_scale(simd4f* m, float x, float y, float z)
    {
    }

    static inline void simd4f_matrix_translate(simd4f* m, float x, float y, float z)
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

#undef SPU_SH4

} // namespace mango
