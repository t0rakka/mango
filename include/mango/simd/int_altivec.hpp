/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_SIMD_INT
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

    // -----------------------------------------------------------------
    // helpers
    // -----------------------------------------------------------------

    static inline vector signed int __vec_splatsi4(const signed int x)
    {
        return (vector signed int) {x, x, x, x};
    }

    // -----------------------------------------------------------------
    // simd4i
    // -----------------------------------------------------------------

    // conversions

    static inline simd4i simd4i_cast(__simd4f s)
    {
		return (simd4i) s;
    }

    static inline simd4i simd4i_convert(__simd4f s)
    {
        return vec_cts(s, 0);
    }

    static inline simd4i simd4i_truncate(__simd4f s)
    {
        return simd4i_convert(vec_trunc(s));
    }

    // set

    static inline simd4i simd4i_set_x(__simd4i a, int x)
    {
        return vec_insert(x, a, 0);
    }

    static inline simd4i simd4i_set_y(__simd4i a, int y)
    {
        return vec_insert(y, a, 1);
    }

    static inline simd4i simd4i_set_z(__simd4i a, int z)
    {
        return vec_insert(z, a, 2);
    }

    static inline simd4i simd4i_set_w(__simd4i a, int w)
    {
        return vec_insert(w, a, 3);
    }

    // get

    static inline int simd4i_get_x(__simd4i a)
    {
        return vec_extract(a, 0);
    }

    static inline int simd4i_get_y(__simd4i a)
    {
        return vec_extract(a, 1);
    }

    static inline int simd4i_get_z(__simd4i a)
    {
        return vec_extract(a, 2);
    }

    static inline int simd4i_get_w(__simd4i a)
    {
        return vec_extract(a, 3);
    }

    static inline simd4i simd4i_load(const int* s)
    {
        return reinterpret_cast<const simd4i*>(s)[0];
    }

    static inline simd4i simd4i_uload(const int* s)
    {
        return (simd4i) { s[0], s[1], s[2], s[3] };
    }

    static inline void simd4i_store(int* d, __simd4i a)
    {
        reinterpret_cast<const simd4i*>(d)[0] = a;
    }

    static inline void simd4i_ustore(int* d, __simd4i a)
    {
        const int* s = reinterpret_cast<const float*>(&a);
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
    }

    static inline simd4i simd4i_zero()
    {
        return __vec_splatsi4(0);
    }

    static inline simd4i simd4i_set1(int s)
    {
        return __vec_splatsi4(s);
    }

    static inline simd4i simd4i_set4(int x, int y, int z, int w)
    {
        const simd4i temp = { x, y, z, w };
        return temp;
    }

    static inline simd4i simd4i_neg(__simd4i a)
    {
        vector signed int zero = __vec_splatsi4(0);
        return vec_sub(zero, x);
    }

    static inline simd4i simd4i_add(__simd4i a, __simd4i b)
    {
        return vec_add(a, b);
    }

    static inline simd4i simd4i_sub(__simd4i a, __simd4i b)
    {
        return vec_sub(a, b);
    }

    // logical

    static inline simd4i simd4i_and(__simd4i a, __simd4i b)
    {
        return vec_and(a, b);
    }

    static inline simd4i simd4i_nand(__simd4i a, __simd4i b)
    {
        return vec_neg(vec_and(a, b));
    }

    static inline simd4i simd4i_or(__simd4i a, __simd4i b)
    {
        return vec_or(a, b);
    }

    static inline simd4i simd4i_xor(__simd4i a, __simd4i b)
    {
        return vec_xor(a, b);
    }

    static inline simd4i simd4i_sll(__simd4i a, int b)
    {
        return vec_sl(a, b);
    }

    static inline simd4i simd4i_srl(__simd4i a, int b)
    {
        return vec_sr(a, b);
    }

    static inline simd4i simd4i_sra(__simd4i a, int b)
    {
        return vec_sra(a, b);
    }

    static inline simd4i simd4i_compare_eq(__simd4i a, __simd4i b)
    {
        return vec_cmpeq(a, b);
    }

    static inline simd4i simd4i_compare_gt(__simd4i a, __simd4i b)
    {
        return vec_cmpgt(a, b);
    }

    static inline simd4i simd4i_select(__simd4i mask, __simd4i a, __simd4i b)
    {
        return vec_sel(b, a, (vector unsigned int)mask);
    }

    static inline uint32 simd4i_get_mask(__simd4i a)
    {
        // TODO
        return 0;
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
