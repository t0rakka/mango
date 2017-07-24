/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // helpers
    // -----------------------------------------------------------------

    static inline vector unsigned int __vec_splatsui4(const unsigned int x)
    {
        return (vector unsigned int) {x, x, x, x};
    }

    static inline vector signed int __vec_splatsi4(const signed int x)
    {
        return (vector signed int) {x, x, x, x};
    }

    // -----------------------------------------------------------------
    // uint8x16
    // -----------------------------------------------------------------

    // TODO

    // -----------------------------------------------------------------
    // uint16x8
    // -----------------------------------------------------------------

    // TODO

    // -----------------------------------------------------------------
    // uint32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline uint32x4 shuffle(uint32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const vector unsigned char mask =
        {
            VEC_SH4(x, 0), VEC_SH4(y, 0), VEC_SH4(z, 0), VEC_SH4(w, 0)
        };
        return vec_perm(v, v, mask);
    }

    template <>
    inline uint32x4 shuffle<0, 1, 2, 3>(uint32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline uint32x4 set_component(uint32x4 a, uint32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return vec_insert(s, a, Index);
    }

    template <unsigned int Index>
    static inline uint32 get_component(uint32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return vec_extract(a, Index);
    }

    static inline uint32x4 uint32x4_zero()
    {
        return __vec_splatsui4(0);
    }

    static inline uint32x4 uint32x4_set1(uint32 s)
    {
        return __vec_splatsui4(s);
    }

    static inline uint32x4 uint32x4_set4(uint32 x, uint32 y, uint32 z, uint32 w)
    {
        const uint32x4 temp = { x, y, z, w };
        return temp;
    }

    static inline uint32x4 uint32x4_uload(const uin32* s)
    {
        return (uint32x4) { s[0], s[1], s[2], s[3] };
    }

    static inline void uint32x4_ustore(uint32* d, uint32x4 a)
    {
        const uint32* s = reinterpret_cast<const uint32 *>(&a);
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
    }

    static inline uint32x4 unpacklo(uint32x4 a, uint32x4 b)
    {
        // TODO
    }

    static inline uint32x4 unpackhi(uint32x4 a, uint32x4 b)
    {
        // TODO
    }

    static inline uint32x4 add(uint32x4 a, uint32x4 b)
    {
        return vec_add(a, b);
    }

    static inline uint32x4 sub(uint32x4 a, uint32x4 b)
    {
        return vec_sub(a, b);
    }

    static inline uint32x4 mullo(uint32x4 a, uint32x4 b)
    {
        // TODO
    }

    // saturated

    static inline uint32x4 adds(uint32x4 a, uint32x4 b)
    {
        // TODO
    }

    static inline uint32x4 subs(uint32x4 a, uint32x4 b)
    {
        // TODO
    }

    // bitwise

    static inline uint32x4 bitwise_nand(uint32x4 a, uint32x4 b)
    {
        return vec_neg(vec_and(a, b));
    }

    static inline uint32x4 bitwise_and(uint32x4 a, uint32x4 b)
    {
        return vec_and(a, b);
    }

    static inline uint32x4 bitwise_or(uint32x4 a, uint32x4 b)
    {
        return vec_or(a, b);
    }

    static inline uint32x4 bitwise_xor(uint32x4 a, uint32x4 b)
    {
        return vec_xor(a, b);
    }

    static inline uint32x4 bitwise_not(uint32x4 a)
    {
        return vec_xor(a, (uint32x4) vec_cmpeq(a, a));
    }

    // compare

    static inline uint32x4::mask compare_eq(uint32x4 a, uint32x4 b)
    {
        return vec_cmpeq(a, b);
    }

    static inline uint32x4::mask compare_gt(uint32x4 a, uint32x4 b)
    {
        return vec_cmpgt(a, b);
    }

    static inline uint32x4::mask compare_neq(uint32x4 a, uint32x4 b)
    {
        // TODO
    }

    static inline uint32x4::mask compare_lt(uint32x4 a, uint32x4 b)
    {
        // TODO
    }

    static inline uint32x4::mask compare_le(uint32x4 a, uint32x4 b)
    {
        // TODO
    }

    static inline uint32x4::mask compare_ge(uint32x4 a, uint32x4 b)
    {
        // TODO
    }

    static inline uint32x4 select(uint32x4::mask mask, uint32x4 a, uint32x4 b)
    {
        return vec_sel(b, a, mask);
    }

    // shift

    template <int Count>
    static inline uint32x4 slli(uint32x4 a)
    {
        return vec_sl(a, Count);
    }

    template <int Count>
    static inline uint32x4 srli(uint32x4 a)
    {
        return vec_sr(a, Count);
    }

    template <int Count>
    static inline uint32x4 srai(uint32x4 a)
    {
        return vec_sra(a, Count);
    }

    static inline uint32x4 sll(uint32x4 a, int count)
    {
        return vec_sl(a, count);
    }

    static inline uint32x4 srl(uint32x4 a, int count)
    {
        return vec_sr(a, count);
    }

    static inline uint32x4 sra(uint32x4 a, int count)
    {
        return vec_sra(a, count);
    }

    static inline uint32x4 min(uint32x4 a, uint32x4 b)
    {
        return vec_min(a, b);
    }

    static inline uint32x4 max(uint32x4 a, uint32x4 b)
    {
        return vec_max(a, b);
    }

    static inline uint32 get_mask(uint32x4 a)
    {
        // TODO
        return 0;
    }

    // -----------------------------------------------------------------
    // uint64x2
    // -----------------------------------------------------------------

    // TODO

    // -----------------------------------------------------------------
    // int8x16
    // -----------------------------------------------------------------

    // TODO

    // -----------------------------------------------------------------
    // int16x8
    // -----------------------------------------------------------------

    // TODO

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline int32x4 shuffle(int32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const vector unsigned char mask =
        {
            VEC_SH4(x, 0), VEC_SH4(y, 0), VEC_SH4(z, 0), VEC_SH4(w, 0)
        };
        return vec_perm(v, v, mask);
    }

    template <>
    inline int32x4 shuffle<0, 1, 2, 3>(int32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline int32x4 set_component(int32x4 a, int32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return vec_insert(s, a, Index);
    }

    template <unsigned int Index>
    static inline int32 get_component(int32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return vec_extract(a, Index);
    }

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

    static inline int32x4 int32x4_uload(const int* s)
    {
        return (int32x4) { s[0], s[1], s[2], s[3] };
    }

    static inline void int32x4_ustore(int* d, int32x4 a)
    {
        const int* s = reinterpret_cast<const float*>(&a);
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
    }

    static inline int32x4 unpacklo(int32x4 a, int32x4 b)
    {
        // TODO
    }

    static inline int32x4 unpackhi(int32x4 a, int32x4 b)
    {
        // TODO
    }

    static inline int32x4 abs(int32x4 a)
    {
        return vec_abs(a);
    }

    static inline int32x4 neg(int32x4 a)
    {
        return vec_sub(vec_xor(a, a), a);
    }

    static inline int32x4 add(int32x4 a, int32x4 b)
    {
        return vec_add(a, b);
    }

    static inline int32x4 sub(int32x4 a, int32x4 b)
    {
        return vec_sub(a, b);
    }

    static inline int32x4 mullo(int32x4 a, int32x4 b)
    {
        // TODO
    }

    // saturated

    static inline int32x4 adds(int32x4 a, int32x4 b)
    {
        return a; // TODO
    }

    static inline int32x4 subs(int32x4 a, int32x4 b)
    {
        return a; // TODO
    }

    // bitwise

    static inline int32x4 bitwise_nand(int32x4 a, int32x4 b)
    {
        return vec_neg(vec_and(a, b));
    }

    static inline int32x4 bitwise_and(int32x4 a, int32x4 b)
    {
        return vec_and(a, b);
    }

    static inline int32x4 bitwise_or(int32x4 a, int32x4 b)
    {
        return vec_or(a, b);
    }

    static inline int32x4 bitwise_xor(int32x4 a, int32x4 b)
    {
        return vec_xor(a, b);
    }

    static inline int32x4 bitwise_not(int32x4 a)
    {
        return vec_xor(a, (int32x4) vec_cmpeq(a, a));
    }

    // compare

    static inline int32x4::mask compare_eq(int32x4 a, int32x4 b)
    {
        return vec_cmpeq(a, b);
    }

    static inline int32x4::mask compare_gt(int32x4 a, int32x4 b)
    {
        return vec_cmpgt(a, b);
    }

    static inline int32x4::mask compare_neq(int32x4 a, int32x4 b)
    {
        // TODO
    }

    static inline int32x4::mask compare_lt(int32x4 a, int32x4 b)
    {
        // TODO
    }

    static inline int32x4::mask compare_le(int32x4 a, int32x4 b)
    {
        // TODO
    }

    static inline int32x4::mask compare_ge(int32x4 a, int32x4 b)
    {
        // TODO
    }

    static inline int32x4 select(int32x4::mask mask, int32x4 a, int32x4 b)
    {
        return vec_sel(b, a, (vector unsigned int)mask);
    }

    // shift

    template <int Count>
    static inline int32x4 slli(int32x4 a)
    {
        return vec_sl(a, Count);
    }

    template <int Count>
    static inline int32x4 srli(int32x4 a)
    {
        return vec_sr(a, Count);
    }

    template <int Count>
    static inline int32x4 srai(int32x4 a)
    {
        return vec_sra(a, Count);
    }

    static inline int32x4 sll(int32x4 a, int count)
    {
        return vec_sl(a, count);
    }

    static inline int32x4 srl(int32x4 a, int count)
    {
        return vec_sr(a, count);
    }

    static inline int32x4 sra(int32x4 a, int count)
    {
        return vec_sra(a, count);
    }

    static inline int32x4 min(int32x4 a, int32x4 b)
    {
        return vec_min(a, b);
    }

    static inline int32x4 max(int32x4 a, int32x4 b)
    {
        return vec_max(a, b);
    }

    static inline uint32 get_mask(int32x4 a)
    {
        // TODO
        return 0;
    }

    static inline uint32 pack(int32x4 s)
    {
        unsigned int* p = (unsigned int*)&s;
        return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    }

    static inline int32x4 unpack(uint32 s)
    {
        const int x = (s >> 0) & 0xff;
        const int y = (s >> 8) & 0xff;
        const int z = (s >> 16) & 0xff;
        const int w = (s >> 24);
        return int32x4_set4(x, y, z, w);
    }

    // -----------------------------------------------------------------
    // int64x2
    // -----------------------------------------------------------------

    // TODO

} // namespace simd
} // namespace mango
