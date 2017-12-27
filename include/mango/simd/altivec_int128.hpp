/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // uint8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint8x16 set_component(uint8x16 a, uint8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return vec_insert(s, a, Index);
    }

    template <unsigned int Index>
    static inline uint8 get_component(uint8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return vec_extract(a, Index);
    }

    static inline uint8x16 uint8x16_zero()
    {
        return vec_splats(uint8(0));
    }

    static inline uint8x16 uint8x16_set1(uint8 s)
    {
        return vec_splats(s);
    }

    static inline uint8x16 unpacklo(uint8x16 a, uint8x16 b)
    {
        return vec_mergeh(a, b);
    }

    static inline uint8x16 unpackhi(uint8x16 a, uint8x16 b)
    {
        return vec_mergel(a, b);
    }

    static inline uint8x16 add(uint8x16 a, uint8x16 b)
    {
        return vec_add(a, b);
    }

    static inline uint8x16 sub(uint8x16 a, uint8x16 b)
    {
        return vec_sub(a, b);
    }

    // saturated

    static inline uint8x16 adds(uint8x16 a, uint8x16 b)
    {
        return vec_adds(a, b);
    }

    static inline uint8x16 subs(uint8x16 a, uint8x16 b)
    {
        return vec_subs(a, b);
    }

    // bitwise

    static inline uint8x16 bitwise_nand(uint8x16 a, uint8x16 b)
    {
        return vec_nand(a, b);
    }

    static inline uint8x16 bitwise_and(uint8x16 a, uint8x16 b)
    {
        return vec_and(a, b);
    }

    static inline uint8x16 bitwise_or(uint8x16 a, uint8x16 b)
    {
        return vec_or(a, b);
    }

    static inline uint8x16 bitwise_xor(uint8x16 a, uint8x16 b)
    {
        return vec_xor(a, b);
    }

    static inline uint8x16 bitwise_not(uint8x16 a)
    {
        return vec_nor(a, a);
    }

    // compare

    static inline umask8x16 compare_eq(uint8x16 a, uint8x16 b)
    {
        return vec_cmpeq(a, b);
    }

    static inline umask8x16 compare_gt(uint8x16 a, uint8x16 b)
    {
        return vec_cmpgt(a, b);
    }

    static inline umask8x16 compare_neq(uint8x16 a, uint8x16 b)
    {
        auto mask = vec_cmpeq(a, b);
        return vec_nor(mask, mask);
    }

    static inline umask8x16 compare_lt(uint8x16 a, uint8x16 b)
    {
        return vec_cmplt(a, b);
    }

    static inline umask8x16 compare_le(uint8x16 a, uint8x16 b)
    {
        return vec_cmple(a, b);
    }

    static inline umask8x16 compare_ge(uint8x16 a, uint8x16 b)
    {
        return vec_cmpge(a, b);
    }

    static inline uint8x16 select(mask8x16 mask, uint8x16 a, uint8x16 b)
    {
        return vec_sel(b, a, mask);
    }

    static inline uint8x16 min(uint8x16 a, uint8x16 b)
    {
        return vec_min(a, b);
    }

    static inline uint8x16 max(uint8x16 a, uint8x16 b)
    {
        return vec_max(a, b);
    }

    // -----------------------------------------------------------------
    // uint16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint16x8 set_component(uint16x8 a, uint16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return vec_insert(s, a, Index);
    }

    template <unsigned int Index>
    static inline uint16 get_component(uint16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return vec_extract(a, Index);
    }

    static inline uint16x8 uint16x8_zero()
    {
        return vec_splats(uint16(0));
    }

    static inline uint16x8 uint16x8_set1(uint16 s)
    {
        return vec_splats(s);
    }

    static inline uint16x8 unpacklo(uint16x8 a, uint16x8 b)
    {
        return vec_mergeh(a, b);
    }

    static inline uint16x8 unpackhi(uint16x8 a, uint16x8 b)
    {
        return vec_mergel(a, b);
    }

    static inline uint16x8 add(uint16x8 a, uint16x8 b)
    {
        return vec_add(a, b);
    }

    static inline uint16x8 sub(uint16x8 a, uint16x8 b)
    {
        return vec_sub(a, b);
    }

    static inline uint16x8 mullo(uint16x8 a, uint16x8 b)
    {
        return vec_mladd(a, b, vec_xor(a, a));
    }

    // saturated

    static inline uint16x8 adds(uint16x8 a, uint16x8 b)
    {
        return vec_adds(a, b);
    }

    static inline uint16x8 subs(uint16x8 a, uint16x8 b)
    {
        return vec_subs(a, b);
    }

    // bitwise

    static inline uint16x8 bitwise_nand(uint16x8 a, uint16x8 b)
    {
        return vec_nand(a, b);
    }

    static inline uint16x8 bitwise_and(uint16x8 a, uint16x8 b)
    {
        return vec_and(a, b);
    }

    static inline uint16x8 bitwise_or(uint16x8 a, uint16x8 b)
    {
        return vec_or(a, b);
    }

    static inline uint16x8 bitwise_xor(uint16x8 a, uint16x8 b)
    {
        return vec_xor(a, b);
    }

    static inline uint16x8 bitwise_not(uint16x8 a)
    {
        return vec_nor(a, a);
    }

    // compare

    static inline mask16x8 compare_neq(uint16x8 a, uint16x8 b)
    {
        auto mask = vec_cmpeq(a, b);
        return vec_nor(mask, mask);
    }

    static inline mask16x8 compare_lt(uint16x8 a, uint16x8 b)
    {
        return vec_cmplt(a, b);
    }

    static inline mask16x8 compare_le(uint16x8 a, uint16x8 b)
    {
        return vec_cmple(a, b);
    }

    static inline mask16x8 compare_ge(uint16x8 a, uint16x8 b)
    {
        return vec_cmpge(a, b);
    }

    static inline mask16x8 compare_eq(uint16x8 a, uint16x8 b)
    {
        return vec_cmpeq(a, b);
    }

    static inline mask16x8 compare_gt(uint16x8 a, uint16x8 b)
    {
        return vec_cmpgt(a, b);
    }

    static inline uint16x8 select(mask16x8 mask, uint16x8 a, uint16x8 b)
    {
        return vec_sel(b, a, mask);
    }

    // shift by constant

    template <int Count>
    static inline uint16x8 slli(uint16x8 a)
    {
        return vec_sl(a, vec_splats(uint16(Count)));
    }

    template <int Count>
    static inline uint16x8 srli(uint16x8 a)
    {
        return vec_sr(a, vec_splats(uint16(Count)));
    }

    template <int Count>
    static inline uint16x8 srai(uint16x8 a)
    {
        return vec_sra(a, vec_splats(uint16(Count)));
    }

    // shift by scalar

    static inline uint16x8 sll(uint16x8 a, int count)
    {
        return vec_sl(a, vec_splats(uint16(count)));
    }

    static inline uint16x8 srl(uint16x8 a, int count)
    {
        return vec_sr(a, vec_splats(uint16(count)));
    }

    static inline uint16x8 sra(uint16x8 a, int count)
    {
        return vec_sra(a, vec_splats(uint16(count)));
    }

    static inline uint16x8 min(uint16x8 a, uint16x8 b)
    {
        return vec_min(a, b);
    }

    static inline uint16x8 max(uint16x8 a, uint16x8 b)
    {
        return vec_max(a, b);
    }
    
    // -----------------------------------------------------------------
    // uint32x4
    // -----------------------------------------------------------------

    // shuffle

#define VEC_SH4(n, select) \
    (select * 16 + n * 4 + 0), \
    (select * 16 + n * 4 + 1), \
    (select * 16 + n * 4 + 2), \
    (select * 16 + n * 4 + 3)

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline uint32x4 shuffle(uint32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const uint8x16 mask = {{ VEC_SH4(x, 0), VEC_SH4(y, 0), VEC_SH4(z, 0), VEC_SH4(w, 0) }};
        return vec_perm(v, v, mask);
    }

#undef VEC_SH4

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
        return vec_splats(uint32(0));
    }

    static inline uint32x4 uint32x4_set1(uint32 s)
    {
        return vec_splats(s);
    }

    static inline uint32x4 uint32x4_set4(uint32 x, uint32 y, uint32 z, uint32 w)
    {
        return {{ x, y, z, w }};
    }

    static inline uint32x4 uint32x4_uload(const uint32* s)
    {
        return {{ s[0], s[1], s[2], s[3] }};
    }

    static inline void uint32x4_ustore(uint32* dest, uint32x4 a)
    {
        dest[0] = vec_extract(a, 0);
        dest[1] = vec_extract(a, 1);
        dest[2] = vec_extract(a, 2);
        dest[3] = vec_extract(a, 3);
    }

    static inline uint32x4 unpacklo(uint32x4 a, uint32x4 b)
    {
        return vec_mergeh(a, b);
    }

    static inline uint32x4 unpackhi(uint32x4 a, uint32x4 b)
    {
        return vec_mergel(a, b);
    }

    static inline uint32x4 add(uint32x4 a, uint32x4 b)
    {
        return vec_add(a, b);
    }

    static inline uint32x4 sub(uint32x4 a, uint32x4 b)
    {
        return vec_sub(a, b);
    }

#if defined(_ARCH_PWR8)

    static inline uint32x4 mullo(uint32x4 a, uint32x4 b)
    {
        return vec_vmuluwm(a, b);
    }

#else

    static inline uint32x4 mullo(uint32x4 a, uint32x4 b)
    {
        float32x4 af = vec_ctf(a, 0);
        float32x4 bf = vec_ctf(b, 0);
        return vec_ctu(vec_mul(af, bf), 0);
    }

#endif

    // saturated

    static inline uint32x4 adds(uint32x4 a, uint32x4 b)
    {
        return vec_adds(a, b);
    }

    static inline uint32x4 subs(uint32x4 a, uint32x4 b)
    {
        return vec_subs(a, b);
    }

    // bitwise

    static inline uint32x4 bitwise_nand(uint32x4 a, uint32x4 b)
    {
        return vec_nand(a, b);
    }

    static inline uint32x4 bitwise_and(uint32x4 a, uint32x4 b)
    {
        return vec_amd(a, b);
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
        return vec_nor(a, a);
    }

    // compare

    static inline mask32x4 compare_eq(uint32x4 a, uint32x4 b)
    {
        return vec_cmpeq(a, b);
    }

    static inline mask32x4 compare_gt(uint32x4 a, uint32x4 b)
    {
        return vec_cmpgt(a, b);
    }

    static inline mask32x4 compare_neq(uint32x4 a, uint32x4 b)
    {
        auto mask = vec_cmpeq(a, b);
        return vec_nor(mask, mask);
    }

    static inline mask32x4 compare_lt(uint32x4 a, uint32x4 b)
    {
        return vec_cmplt(a, b);
    }

    static inline mask32x4 compare_le(uint32x4 a, uint32x4 b)
    {
        return vec_cmple(a, b);
    }

    static inline mask32x4 compare_ge(uint32x4 a, uint32x4 b)
    {
        return vec_cmpge(a, b);
    }

    static inline uint32x4 select(mask32x4 mask, uint32x4 a, uint32x4 b)
    {
        return vec_sel(b, a, mask);
    }

    // shift by constant

    template <int Count>
    static inline uint32x4 slli(uint32x4 a)
    {
        return vec_sl(a, vec_splats(uint32(Count)));
    }

    template <int Count>
    static inline uint32x4 srli(uint32x4 a)
    {
        return vec_sr(a, vec_splats(uint32(Count)));
    }

    template <int Count>
    static inline uint32x4 srai(uint32x4 a)
    {
        return vec_sra(a, vec_splats(uint32(Count)));
    }

    // shift by scalar

    static inline uint32x4 sll(uint32x4 a, int count)
    {
        return vec_sl(a, vec_splats(uint32(count)));
    }

    static inline uint32x4 srl(uint32x4 a, int count)
    {
        return vec_sr(a, vec_splats(uint32(count)));
    }

    static inline uint32x4 sra(uint32x4 a, int count)
    {
        return vec_sra(a, vec_splats(uint32(count)));
    }

    // shift by vector

    static inline uint32x4 sll(uint32x4 a, uint32x4 count)
    {
        return vec_sl(a, count);
    }

    static inline uint32x4 srl(uint32x4 a, uint32x4 count)
    {
        return vec_sr(a, count);
    }

    static inline uint32x4 sra(uint32x4 a, uint32x4 count)
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

    // -----------------------------------------------------------------
    // uint64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint64x2 set_component(uint64x2 a, uint64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return vec_insert(s, a, Index);
    }

    template <unsigned int Index>
    static inline uint64 get_component(uint64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return vec_extract(a, Index);
    }

    static inline uint64x2 uint64x2_zero()
    {
        return vec_splats(uint64(0));
    }

    static inline uint64x2 uint64x2_set1(uint64 s)
    {
        return vec_splats(s);
    }

    static inline uint64x2 uint64x2_set2(uint64 x, uint64 y)
    {
        return {{ x, y }};
    }

    static inline uint64x2 unpacklo(uint64x2 a, uint64x2 b)
    {
        return vec_insert(vec_extract(b, 0), a, 1);
    }

    static inline uint64x2 unpackhi(uint64x2 a, uint64x2 b)
    {
        return vec_insert(vec_extract(a, 1), b, 0);
    }

    static inline uint64x2 add(uint64x2 a, uint64x2 b)
    {
        return vec_add(a, b);
    }

    static inline uint64x2 sub(uint64x2 a, uint64x2 b)
    {
        return vec_sub(a, b);
    }

    static inline uint64x2 bitwise_nand(uint64x2 a, uint64x2 b)
    {
        return vec_nand(a, b);
    }

    static inline uint64x2 bitwise_and(uint64x2 a, uint64x2 b)
    {
        return vec_and(a, b);
    }

    static inline uint64x2 bitwise_or(uint64x2 a, uint64x2 b)
    {
        return vec_or(a, b);
    }

    static inline uint64x2 bitwise_xor(uint64x2 a, uint64x2 b)
    {
        return vec_xor(a, b);
    }

    static inline uint64x2 bitwise_not(uint64x2 a)
    {
        return vec_nor(a, a);
    }

    static inline uint64x2 select(mask64x2 mask, uint64x2 a, uint64x2 b)
    {
        return vec_sel(b, a, mask);
    }

    // shift by constant

    template <int Count>
    static inline uint64x2 slli(uint64x2 a)
    {
        vec_insert(vec_extract(a, 0) << Count, a, 0);
        vec_insert(vec_extract(a, 1) << Count, a, 1);
    }

    template <int Count>
    static inline uint64x2 srli(uint64x2 a)
    {
        vec_insert(vec_extract(a, 0) >> Count, a, 0);
        vec_insert(vec_extract(a, 1) >> Count, a, 1);
    }

    // shift by scalar

    static inline uint64x2 sll(uint64x2 a, int count)
    {
        vec_insert(vec_extract(a, 0) << count, a, 0);
        vec_insert(vec_extract(a, 1) << count, a, 1);
    }

    static inline uint64x2 srl(uint64x2 a, int count)
    {
        vec_insert(vec_extract(a, 0) >> count, a, 0);
        vec_insert(vec_extract(a, 1) >> count, a, 1);
    }

    // -----------------------------------------------------------------
    // int8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int8x16 set_component(int8x16 a, int8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return vec_insert(s, a, Index);
    }

    template <unsigned int Index>
    static inline int8 get_component(int8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return vec_extract(a, Index);
    }

    static inline int8x16 int8x16_zero()
    {
        return vec_splats(int8(0));
    }

    static inline int8x16 int8x16_set1(int8 s)
    {
        return vec_splats(s);
    }

    static inline int8x16 unpacklo(int8x16 a, int8x16 b)
    {
        return vec_mergeh(a, b);
    }

    static inline int8x16 unpackhi(int8x16 a, int8x16 b)
    {
        return vec_mergel(a, b);
    }

    static inline int8x16 add(int8x16 a, int8x16 b)
    {
        return vec_add(a, b);
    }

    static inline int8x16 sub(int8x16 a, int8x16 b)
    {
        return vec_sub(a, b);
    }

    // saturated

    static inline int8x16 adds(int8x16 a, int8x16 b)
    {
        return vec_adds(a, b);
    }

    static inline int8x16 subs(int8x16 a, int8x16 b)
    {
        return vec_subs(a, b);
    }

    static inline int8x16 abs(int8x16 a)
    {
        return vec_abs(a);
    }

    static inline int8x16 neg(int8x16 a)
    {
        return vec_neg(a);
    }

    // bitwise

    static inline int8x16 bitwise_nand(int8x16 a, int8x16 b)
    {
        return vec_nand(a, b);
    }

    static inline int8x16 bitwise_and(int8x16 a, int8x16 b)
    {
        return vec_and(a, b);
    }

    static inline int8x16 bitwise_or(int8x16 a, int8x16 b)
    {
        return vec_or(a, b);
    }

    static inline int8x16 bitwise_xor(int8x16 a, int8x16 b)
    {
        return vec_xor(a, b);
    }

    static inline int8x16 bitwise_not(int8x16 a)
    {
        return vec_nor(a, a);
    }

    // compare

    static inline mask8x16 compare_eq(int8x16 a, int8x16 b)
    {
        return vec_cmpeq(a, b);
    }

    static inline mask8x16 compare_gt(int8x16 a, int8x16 b)
    {
        return vec_cmpgt(a, b);
    }

    static inline mask8x16 compare_neq(int8x16 a, int8x16 b)
    {
        auto mask = vec_cmpeq(a, b);
        return vec_nor(mask, mask);
    }

    static inline mask8x16 compare_lt(int8x16 a, int8x16 b)
    {
        return vec_cmplt(a, b);
    }

    static inline mask8x16 compare_le(int8x16 a, int8x16 b)
    {
        return vec_cmple(a, b);
    }

    static inline mask8x16 compare_ge(int8x16 a, int8x16 b)
    {
        return vec_cmpge(a, b);
    }

    static inline int8x16 select(mask8x16 mask, int8x16 a, int8x16 b)
    {
        return vec_sel(b, a, mask);
    }

    static inline int8x16 min(int8x16 a, int8x16 b)
    {
        return vec_min(a, b);
    }

    static inline int8x16 max(int8x16 a, int8x16 b)
    {
        return vec_max(a, b);
    }

    // -----------------------------------------------------------------
    // int16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int16x8 set_component(int16x8 a, int16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return vec_insert(s, a, Index);
    }

    template <unsigned int Index>
    static inline int16 get_component(int16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return vec_extract(a, Index);
    }

    static inline int16x8 int16x8_zero()
    {
        return vec_splats(int16(0));
    }

    static inline int16x8 int16x8_set1(int16 s)
    {
        return vec_splats(s);
    }

    static inline int16x8 unpacklo(int16x8 a, int16x8 b)
    {
        return vec_mergeh(a, b);
    }

    static inline int16x8 unpackhi(int16x8 a, int16x8 b)
    {
        return vec_mergel(a, b);
    }

    static inline int16x8 add(int16x8 a, int16x8 b)
    {
        return vec_add(a, b);
    }

    static inline int16x8 sub(int16x8 a, int16x8 b)
    {
        return vec_sub(a, b);
    }

    static inline int16x8 mullo(int16x8 a, int16x8 b)
    {
        return vec_mladd(a, b, vec_xor(a, a));
    }

    // saturated

    static inline int16x8 adds(int16x8 a, int16x8 b)
    {
        return vec_adds(a, b);
    }

    static inline int16x8 subs(int16x8 a, int16x8 b)
    {
        return vec_subs(a, b);
    }

    static inline int16x8 abs(int16x8 a)
    {
        return vec_abs(a);
    }

    static inline int16x8 neg(int16x8 a)
    {
        return vec_neg(a);
    }

    // bitwise

    static inline int16x8 bitwise_nand(int16x8 a, int16x8 b)
    {
        return vec_nand(a, b);
    }

    static inline int16x8 bitwise_and(int16x8 a, int16x8 b)
    {
        return vec_and(a, b);
    }

    static inline int16x8 bitwise_or(int16x8 a, int16x8 b)
    {
        return vec_or(a, b);
    }

    static inline int16x8 bitwise_xor(int16x8 a, int16x8 b)
    {
        return vec_xor(a, b);
    }

    static inline int16x8 bitwise_not(int16x8 a)
    {
        return vec_nor(a, a);
    }

    // compare

    static inline mask16x8 compare_eq(int16x8 a, int16x8 b)
    {
        return vec_cmpeq(a, b);
    }

    static inline mask16x8 compare_gt(int16x8 a, int16x8 b)
    {
        return vec_cmpgt(a, b);
    }

    static inline mask16x8 compare_neq(int16x8 a, int16x8 b)
    {
        auto mask = vec_cmpeq(a, b);
        return vec_nor(mask, mask);
    }

    static inline mask16x8 compare_lt(int16x8 a, int16x8 b)
    {
        return vec_cmplt(a, b);
    }

    static inline mask16x8 compare_le(int16x8 a, int16x8 b)
    {
        return vec_cmple(a, b);
    }

    static inline mask16x8 compare_ge(int16x8 a, int16x8 b)
    {
        return vec_cmpge(a, b);
    }

    static inline int16x8 select(mask16x8 mask, int16x8 a, int16x8 b)
    {
        return vec_sel(b, a, mask);
    }

    // shift by constant

    template <int Count>
    static inline int16x8 slli(int16x8 a)
    {
        return vec_sl(a, vec_splats(int16(Count)));
    }

    template <int Count>
    static inline int16x8 srli(int16x8 a)
    {
        return vec_sr(a, vec_splats(int16(Count)));
    }

    template <int Count>
    static inline int16x8 srai(int16x8 a)
    {
        return vec_sra(a, vec_splats(int16(Count)));
    }

    // shift by scalar

    static inline int16x8 sll(int16x8 a, int count)
    {
        return vec_sl(a, vec_splats(int16(count)));
    }

    static inline int16x8 srl(int16x8 a, int count)
    {
        return vec_sr(a, vec_splats(int16(count)));
    }

    static inline int16x8 sra(int16x8 a, int count)
    {
        return vec_sra(a, vec_splats(int16(count)));
    }

    static inline int16x8 min(int16x8 a, int16x8 b)
    {
        return vec_min(a, b);
    }

    static inline int16x8 max(int16x8 a, int16x8 b)
    {
        return vec_max(a, b);
    }

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    // shuffle

#define VEC_SH4(n, select) \
    (select * 16 + n * 4 + 0), \
    (select * 16 + n * 4 + 1), \
    (select * 16 + n * 4 + 2), \
    (select * 16 + n * 4 + 3)

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline int32x4 shuffle(int32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const uint8x16 mask = {{ VEC_SH4(x, 0), VEC_SH4(y, 0), VEC_SH4(z, 0), VEC_SH4(w, 0) }};
        return vec_perm(v, v, mask);
    }

#undef VEC_SH4

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
        return vec_splats(int32(0));
    }

    static inline int32x4 int32x4_set1(int s)
    {
        return vec_splats(s);
    }

    static inline int32x4 int32x4_set4(int x, int y, int z, int w)
    {
        return {{ x, y, z, w }};
    }

    static inline int32x4 int32x4_uload(const int* s)
    {
        return {{ s[0], s[1], s[2], s[3] }};
    }

    static inline void int32x4_ustore(int* dest, int32x4 a)
    {
        dest[0] = vec_extract(a, 0);
        dest[1] = vec_extract(a, 1);
        dest[2] = vec_extract(a, 2);
        dest[3] = vec_extract(a, 3);
    }

    static inline int32x4 unpacklo(int32x4 a, int32x4 b)
    {
        return vec_mergeh(a, b);
    }

    static inline int32x4 unpackhi(int32x4 a, int32x4 b)
    {
        return vec_mergel(a, b);
    }

    static inline int32x4 abs(int32x4 a)
    {
        return vec_abs(a);
    }

    static inline int32x4 neg(int32x4 a)
    {
        return vec_neg(a);
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
        float32x4 af = vec_ctf(a, 0);
        float32x4 bf = vec_ctf(b, 0);
        return vec_cts(vec_mul(af, bf), 0);
    }

    // saturated

    static inline int32x4 adds(int32x4 a, int32x4 b)
    {
        return vec_adds(a, b);
    }

    static inline int32x4 subs(int32x4 a, int32x4 b)
    {
        return vec_subs(a, b);
    }

    // bitwise

    static inline int32x4 bitwise_nand(int32x4 a, int32x4 b)
    {
        return vec_nand(a, b);
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
        return vec_nor(a, a);
    }

    // compare

    static inline mask32x4 compare_eq(int32x4 a, int32x4 b)
    {
        return vec_cmpeq(a, b);
    }

    static inline mask32x4 compare_gt(int32x4 a, int32x4 b)
    {
        return vec_cmpgt(a, b);
    }

    static inline mask32x4 compare_neq(int32x4 a, int32x4 b)
    {
        auto mask = vec_cmpeq(a, b);
        return vec_nor(mask, mask);
    }

    static inline mask32x4 compare_lt(int32x4 a, int32x4 b)
    {
        return vec_cmplt(a, b);
    }

    static inline mask32x4 compare_le(int32x4 a, int32x4 b)
    {
        return vec_cmple(a, b);
    }

    static inline mask32x4 compare_ge(int32x4 a, int32x4 b)
    {
        return vec_cmpge(a, b);
    }

    static inline int32x4 select(mask32x4 mask, int32x4 a, int32x4 b)
    {
        return vec_sel(b, a, mask);
    }

    // shift by constant

    template <int Count>
    static inline int32x4 slli(int32x4 a)
    {
        return vec_sl(a, vec_splats(int32(Count)));
    }

    template <int Count>
    static inline int32x4 srli(int32x4 a)
    {
        return vec_sr(a, vec_splats(int32(Count)));
    }

    template <int Count>
    static inline int32x4 srai(int32x4 a)
    {
        return vec_sra(a, vec_splats(int32(Count)));
    }

    // shift by scalar

    static inline int32x4 sll(int32x4 a, int count)
    {
        return vec_sl(a, vec_splats(int32(count)));
    }

    static inline int32x4 srl(int32x4 a, int count)
    {
        return vec_sr(a, vec_splats(int32(count)));
    }

    static inline int32x4 sra(int32x4 a, int count)
    {
        return vec_sra(a, vec_splats(int32(count)));
    }

    // shift by vector

    static inline int32x4 sll(int32x4 a, uint32x4 count)
    {
        return vec_sl(a, count);
    }

    static inline int32x4 srl(int32x4 a, uint32x4 count)
    {
        return vec_sr(a, count);
    }

    static inline int32x4 sra(int32x4 a, uint32x4 count)
    {
        return vec_sra(a, count);
    }

    static inline uint32 pack(int32x4 s)
    {
        int32x4 v = vec_sl(s, {{ 0, 8, 16, 24 }});
        v = vec_or(vec_mergeh(v, v), vec_mergel(v, v));
        v = vec_or(vec_mergeh(v, v), vec_mergel(v, v));
        return vec_extract(v, 0);
    }

    static inline int32x4 min(int32x4 a, int32x4 b)
    {
        return vec_min(a, b);
    }

    static inline int32x4 max(int32x4 a, int32x4 b)
    {
        return vec_max(a, b);
    }

    static inline int32x4 unpack(uint32 s)
    {
        int32x4 v = vec_splats(s);
        v = vec_and(v, {{ 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 }});
        return vec_sr(v, {{ 0, 8, 16, 24 }});
    }

    // -----------------------------------------------------------------
    // int64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int64x2 set_component(int64x2 a, int64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return vec_insert(s, a, Index);
    }

    template <unsigned int Index>
    static inline int64 get_component(int64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return vec_extract(a, Index);
    }

    static inline int64x2 int64x2_zero()
    {
        return vec_splats(int64(0));
    }

    static inline int64x2 int64x2_set1(int64 s)
    {
        return vec_splats(s);
    }

    static inline int64x2 int64x2_set2(int64 x, int64 y)
    {
        return {{ x, y }};
    }

    static inline int64x2 unpacklo(int64x2 a, int64x2 b)
    {
        return vec_insert(vec_extract(b, 0), a, 1);
    }

    static inline int64x2 unpackhi(int64x2 a, int64x2 b)
    {
        return vec_insert(vec_extract(a, 1), b, 0);
    }

    static inline int64x2 add(int64x2 a, int64x2 b)
    {
        return vec_add(a, b);
    }

    static inline int64x2 sub(int64x2 a, int64x2 b)
    {
        return vec_sub(a, b);
    }

    static inline int64x2 bitwise_nand(int64x2 a, int64x2 b)
    {
        return vec_nand(a, b);
    }

    static inline int64x2 bitwise_and(int64x2 a, int64x2 b)
    {
        return vec_and(a, b);
    }

    static inline int64x2 bitwise_or(int64x2 a, int64x2 b)
    {
        return vec_or(a, b);
    }

    static inline int64x2 bitwise_xor(int64x2 a, int64x2 b)
    {
        return vec_xor(a, b);
    }

    static inline int64x2 bitwise_not(int64x2 a)
    {
        return vec_nor(a, a);
    }

    static inline int64x2 select(mask64x2 mask, int64x2 a, int64x2 b)
    {
        return vec_sel(b, a, mask);
    }

    // shift by constant

    template <int Count>
    static inline int64x2 slli(int64x2 a)
    {
        vec_insert(vec_extract(a, 0) << Count, a, 0);
        vec_insert(vec_extract(a, 1) << Count, a, 1);
    }

    template <int Count>
    static inline int64x2 srli(int64x2 a)
    {
        vec_insert(vec_extract(a, 0) >> Count, a, 0);
        vec_insert(vec_extract(a, 1) >> Count, a, 1);
    }

    // shift by scalar

    static inline int64x2 sll(int64x2 a, int count)
    {
        vec_insert(vec_extract(a, 0) << count, a, 0);
        vec_insert(vec_extract(a, 1) << count, a, 1);
    }

    static inline int64x2 srl(int64x2 a, int count)
    {
        vec_insert(vec_extract(a, 0) >> count, a, 0);
        vec_insert(vec_extract(a, 1) >> count, a, 1);
    }

    // -----------------------------------------------------------------
    // mask8x16
    // -----------------------------------------------------------------

    static inline mask8x16 operator & (mask8x16 a, mask8x16 b)
    {
        return vec_and(a, b);
    }

    static inline mask8x16 operator | (mask8x16 a, mask8x16 b)
    {
        return vec_or(a, b);
    }

    static inline mask8x16 operator ^ (mask8x16 a, mask8x16 b)
    {
        return vec_xor(a, b);
    }

    static inline uint32 get_mask(mask8x16 a)
    {
        const uint32x4 zero = uint32x4_zero();
        uint8x16 masked = vec_and(a, {{ 1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128 }});
        int32x4 sum = (int32x4) vec_sl(vec_sum4s(masked, zero), {{ 0, 0, 8, 8 }});
        return vec_extract(vec_sums(sum, vec_xor(sum, sum)), 3);
    }

    // -----------------------------------------------------------------
    // mask16x8
    // -----------------------------------------------------------------

    static inline mask16x8 operator & (mask16x8 a, mask16x8 b)
    {
        return vec_and(a, b);
    }

    static inline mask16x8 operator | (mask16x8 a, mask16x8 b)
    {
        return vec_or(a, b);
    }

    static inline mask16x8 operator ^ (mask16x8 a, mask16x8 b)
    {
        return vec_xor(a, b);
    }

    static inline uint32 get_mask(mask16x8 a)
    {
        const int32x4 zero = uint32x4_zero();
        int16x8 masked = (int16x8) vec_and(a, {{ 1, 2, 4, 8, 16, 32, 64, 128 }});
        int32x4 sum = vec_sum4s(masked, zero);
        return vec_extract(vec_sums(sum, zero), 3);
    }

    // -----------------------------------------------------------------
    // mask32x4
    // -----------------------------------------------------------------

    static inline mask32x4 operator & (mask32x4 a, mask32x4 b)
    {
        return vec_and(a, b);
    }

    static inline mask32x4 operator | (mask32x4 a, mask32x4 b)
    {
        return vec_or(a, b);
    }

    static inline mask32x4 operator ^ (mask32x4 a, mask32x4 b)
    {
        return vec_xor(a, b);
    }

    static inline uint32 get_mask(mask32x4 a)
    {
        const int32x4 zero = uint32x4_zero();
        int32x4 masked = (int32x4) vec_and(a, {{ 1, 2, 4, 8 }});
        return vec_extract(vec_sums(masked, zero), 3);
    }

    // -----------------------------------------------------------------
    // mask64x2
    // -----------------------------------------------------------------

    static inline mask64x2 operator & (mask64x2 a, mask64x2 b)
    {
        return vec_and(a, b);
    }

    static inline mask64x2 operator | (mask64x2 a, mask64x2 b)
    {
        return vec_or(a, b);
    }

    static inline mask64x2 operator ^ (mask64x2 a, mask64x2 b)
    {
        return vec_xor(a, b);
    }

    static inline uint32 get_mask(mask64x2 a)
    {
        uint32 x = uint32(get_component(a, 0)) & 1;
        uint32 y = uint32(get_component(a, 1)) & 2;
        return x | y;
    }

} // namespace simd
} // namespace mango
