/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_SIMD_INT
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

    // -----------------------------------------------------------------
    // simd4i
    // -----------------------------------------------------------------

    // conversion

    static inline simd4i simd4i_cast(__simd4f s)
    {
		return (simd4i) s;
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
