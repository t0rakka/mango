/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

#ifdef MANGO_SIMD_INT_ALTIVEC

    // -----------------------------------------------------------------
    // helpers
    // -----------------------------------------------------------------

    static inline vector signed int __vec_splatsi4(const signed int x)
    {
        return (vector signed int) {x, x, x, x};
    }

    // -----------------------------------------------------------------
    // conversion
    // -----------------------------------------------------------------

    static inline uint32x4 uint32x4_reinterpret(int32x4 s)
    {
		return (uint32x4) s;
    }

    static inline int32x4 int32x4_reinterpret(uint32x4 s)
    {
		return (int32x4) s;
    }

    static inline int32x4 int32x4_reinterpret(float32x4 s)
    {
		return (int32x4) s;
    }

    static inline int32x4 int32x4_convert(float32x4 s)
    {
        return vec_cts(s, 0);
    }

    static inline int32x4 int32x4_truncate(float32x4 s)
    {
        return int32x4_convert(vec_trunc(s));
    }

    // -----------------------------------------------------------------
    // uint32x4
    // -----------------------------------------------------------------

    // set

    static inline uint32x4 uint32x4_set1(uint32 s)
    {
        return __vec_splatsi4(s);
    }

    static inline uint32x4 uint32x4_set4(uint32 x, uint32 y, uint32 z, uint32 w)
    {
        const uint32x4 temp = { x, y, z, w };
        return temp;
    }

    static inline uint32x4 uint32x4_set_x(uint32x4 a, int x)
    {
        return vec_insert(x, a, 0);
    }

    static inline uint32x4 uint32x4_set_y(uint32x4 a, int y)
    {
        return vec_insert(y, a, 1);
    }

    static inline uint32x4 uint32x4_set_z(uint32x4 a, int z)
    {
        return vec_insert(z, a, 2);
    }

    static inline uint32x4 uint32x4_set_w(uint32x4 a, int w)
    {
        return vec_insert(w, a, 3);
    }

    // get

    static inline int uint32x4_get_x(uint32x4 a)
    {
        return vec_extract(a, 0);
    }

    static inline int uint32x4_get_y(uint32x4 a)
    {
        return vec_extract(a, 1);
    }

    static inline int uint32x4_get_z(uint32x4 a)
    {
        return vec_extract(a, 2);
    }

    static inline int uint32x4_get_w(uint32x4 a)
    {
        return vec_extract(a, 3);
    }

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    // set

    static inline int32x4 int32x4_zero()
    {
        return __vec_splatsi4(0);
    }

    static inline int32x4 int32x4_set1(int s)
    {
        return __vec_splatsi4(s);
    }

    static inline int32x4 int32x4_set4(int x, int y, int z, int w)
    {
        const int32x4 temp = { x, y, z, w };
        return temp;
    }

    static inline int32x4 int32x4_set_x(int32x4 a, int x)
    {
        return vec_insert(x, a, 0);
    }

    static inline int32x4 int32x4_set_y(int32x4 a, int y)
    {
        return vec_insert(y, a, 1);
    }

    static inline int32x4 int32x4_set_z(int32x4 a, int z)
    {
        return vec_insert(z, a, 2);
    }

    static inline int32x4 int32x4_set_w(int32x4 a, int w)
    {
        return vec_insert(w, a, 3);
    }

    // get

    static inline int int32x4_get_x(int32x4 a)
    {
        return vec_extract(a, 0);
    }

    static inline int int32x4_get_y(int32x4 a)
    {
        return vec_extract(a, 1);
    }

    static inline int int32x4_get_z(int32x4 a)
    {
        return vec_extract(a, 2);
    }

    static inline int int32x4_get_w(int32x4 a)
    {
        return vec_extract(a, 3);
    }

    static inline int32x4 int32x4_load(const int* s)
    {
        return reinterpret_cast<const int32x4*>(s)[0];
    }

    static inline int32x4 int32x4_uload(const int* s)
    {
        return (int32x4) { s[0], s[1], s[2], s[3] };
    }

    static inline void int32x4_store(int* d, int32x4 a)
    {
        reinterpret_cast<const int32x4*>(d)[0] = a;
    }

    static inline void int32x4_ustore(int* d, int32x4 a)
    {
        const int* s = reinterpret_cast<const float*>(&a);
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
    }

    static inline int32x4 int32x4_neg(int32x4 a)
    {
        vector signed int zero = __vec_splatsi4(0);
        return vec_sub(zero, x);
    }

    static inline int32x4 int32x4_add(int32x4 a, int32x4 b)
    {
        return vec_add(a, b);
    }

    static inline int32x4 int32x4_sub(int32x4 a, int32x4 b)
    {
        return vec_sub(a, b);
    }

    // logical

    static inline int32x4 int32x4_and(int32x4 a, int32x4 b)
    {
        return vec_and(a, b);
    }

    static inline int32x4 int32x4_nand(int32x4 a, int32x4 b)
    {
        return vec_neg(vec_and(a, b));
    }

    static inline int32x4 int32x4_or(int32x4 a, int32x4 b)
    {
        return vec_or(a, b);
    }

    static inline int32x4 int32x4_xor(int32x4 a, int32x4 b)
    {
        return vec_xor(a, b);
    }

    static inline int32x4 int32x4_sll(int32x4 a, int b)
    {
        return vec_sl(a, b);
    }

    static inline int32x4 int32x4_srl(int32x4 a, int b)
    {
        return vec_sr(a, b);
    }

    static inline int32x4 int32x4_sra(int32x4 a, int b)
    {
        return vec_sra(a, b);
    }

    static inline int32x4 int32x4_compare_eq(int32x4 a, int32x4 b)
    {
        return vec_cmpeq(a, b);
    }

    static inline int32x4 int32x4_compare_gt(int32x4 a, int32x4 b)
    {
        return vec_cmpgt(a, b);
    }

    static inline int32x4 int32x4_select(int32x4 mask, int32x4 a, int32x4 b)
    {
        return vec_sel(b, a, (vector unsigned int)mask);
    }

    static inline uint32 int32x4_get_mask(int32x4 a)
    {
        // TODO
        return 0;
    }

    static inline uint32 int32x4_pack(int32x4 s)
    {
        unsigned int* p = (unsigned int*)&s;
        return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    }

    static inline int32x4 int32x4_unpack(uint32 s)
    {
        const int x = (s >> 0) & 0xff;
        const int y = (s >> 8) & 0xff;
        const int z = (s >> 16) & 0xff;
        const int w = (s >> 24);
        return int32x4_set4(x, y, z, w);
    }

#endif // MANGO_SIMD_INT_ALTIVEC
