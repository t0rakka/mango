/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        return vec_insert(s, a.data, Index);
    }

    template <unsigned int Index>
    static inline uint8 get_component(uint8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return vec_extract(a.data, Index);
    }

    static inline uint8x16 uint8x16_zero()
    {
        return vec_splats(uint8(0));
    }

    static inline uint8x16 uint8x16_set1(uint8 s)
    {
        return vec_splats(s);
    }

    static inline uint8x16 uint8x16_set16(
        uint8 s0, uint8 s1, uint8 s2, uint8 s3, uint8 s4, uint8 s5, uint8 s6, uint8 s7,
        uint8 s8, uint8 s9, uint8 s10, uint8 s11, uint8 s12, uint8 s13, uint8 s14, uint8 s15)
    {
        return (uint8x16::vector) { s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15 };
    }

    static inline uint8x16 uint8x16_load_low(const uint8* source)
    {
        auto s0 = source[0];
        auto s1 = source[1];
        auto s2 = source[2];
        auto s3 = source[3];
        auto s4 = source[4];
        auto s5 = source[5];
        auto s6 = source[6];
        auto s7 = source[7];
        return (uint8x16::vector) { s0, s1, s2, s3, s4, s5, s6, s7, 0, 0, 0, 0, 0, 0, 0, 0 };
    }

    static inline void uint8x16_store_low(uint8* dest, uint8x16 a)
    {
        std::memcpy(dest, &a, 8);
    }

    static inline uint8x16 unpacklo(uint8x16 a, uint8x16 b)
    {
        return vec_mergeh(a.data, b.data);
    }

    static inline uint8x16 unpackhi(uint8x16 a, uint8x16 b)
    {
        return vec_mergel(a.data, b.data);
    }

    static inline uint8x16 add(uint8x16 a, uint8x16 b)
    {
        return vec_add(a.data, b.data);
    }

    static inline uint8x16 sub(uint8x16 a, uint8x16 b)
    {
        return vec_sub(a.data, b.data);
    }

    // saturated

    static inline uint8x16 adds(uint8x16 a, uint8x16 b)
    {
        return vec_adds(a.data, b.data);
    }

    static inline uint8x16 subs(uint8x16 a, uint8x16 b)
    {
        return vec_subs(a.data, b.data);
    }

    // bitwise

    static inline uint8x16 bitwise_nand(uint8x16 a, uint8x16 b)
    {
        return vec_nand(a.data, b.data);
    }

    static inline uint8x16 bitwise_and(uint8x16 a, uint8x16 b)
    {
        return vec_and(a.data, b.data);
    }

    static inline uint8x16 bitwise_or(uint8x16 a, uint8x16 b)
    {
        return vec_or(a.data, b.data);
    }

    static inline uint8x16 bitwise_xor(uint8x16 a, uint8x16 b)
    {
        return vec_xor(a.data, b.data);
    }

    static inline uint8x16 bitwise_not(uint8x16 a)
    {
        return vec_nor(a.data, a.data);
    }

    // compare

    static inline mask8x16 compare_eq(uint8x16 a, uint8x16 b)
    {
        return vec_cmpeq(a.data, b.data);
    }

    static inline mask8x16 compare_gt(uint8x16 a, uint8x16 b)
    {
        return vec_cmpgt(a.data, b.data);
    }

    static inline mask8x16 compare_neq(uint8x16 a, uint8x16 b)
    {
        auto mask = vec_cmpeq(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask8x16 compare_lt(uint8x16 a, uint8x16 b)
    {
        return vec_cmplt(a.data, b.data);
    }

    static inline mask8x16 compare_le(uint8x16 a, uint8x16 b)
    {
        auto mask = vec_cmpgt(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask8x16 compare_ge(uint8x16 a, uint8x16 b)
    {
        auto mask = vec_cmplt(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline uint8x16 select(mask8x16 mask, uint8x16 a, uint8x16 b)
    {
        return vec_sel(b.data, a.data, mask.data);
    }

    static inline uint8x16 min(uint8x16 a, uint8x16 b)
    {
        return vec_min(a.data, b.data);
    }

    static inline uint8x16 max(uint8x16 a, uint8x16 b)
    {
        return vec_max(a.data, b.data);
    }

    // -----------------------------------------------------------------
    // uint16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint16x8 set_component(uint16x8 a, uint16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return vec_insert(s, a.data, Index);
    }

    template <unsigned int Index>
    static inline uint16 get_component(uint16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return vec_extract(a.data, Index);
    }

    static inline uint16x8 uint16x8_zero()
    {
        return vec_splats(uint16(0));
    }

    static inline uint16x8 uint16x8_set1(uint16 s)
    {
        return vec_splats(s);
    }

    static inline uint16x8 uint16x8_set8(uint16 s0, uint16 s1, uint16 s2, uint16 s3, uint16 s4, uint16 s5, uint16 s6, uint16 s7)
    {
        return (uint16x8::vector) { s0, s1, s2, s3, s4, s5, s6, s7 };
    }

    static inline uint16x8 uint16x8_load_low(const uint16* source)
    {
        auto s0 = source[0];
        auto s1 = source[1];
        auto s2 = source[2];
        auto s3 = source[3];
        return (uint16x8::vector) { s0, s1, s2, s3, 0, 0, 0, 0 };
    }

    static inline void uint16x8_store_low(uint16* dest, uint16x8 a)
    {
        std::memcpy(dest, &a, 8);
    }

    static inline uint16x8 unpacklo(uint16x8 a, uint16x8 b)
    {
        return vec_mergeh(a.data, b.data);
    }

    static inline uint16x8 unpackhi(uint16x8 a, uint16x8 b)
    {
        return vec_mergel(a.data, b.data);
    }

    static inline uint16x8 add(uint16x8 a, uint16x8 b)
    {
        return vec_add(a.data, b.data);
    }

    static inline uint16x8 sub(uint16x8 a, uint16x8 b)
    {
        return vec_sub(a.data, b.data);
    }

    static inline uint16x8 mullo(uint16x8 a, uint16x8 b)
    {
        return vec_mladd(a.data, b.data, vec_xor(a.data, a.data));
    }

    // saturated

    static inline uint16x8 adds(uint16x8 a, uint16x8 b)
    {
        return vec_adds(a.data, b.data);
    }

    static inline uint16x8 subs(uint16x8 a, uint16x8 b)
    {
        return vec_subs(a.data, b.data);
    }

    // bitwise

    static inline uint16x8 bitwise_nand(uint16x8 a, uint16x8 b)
    {
        return vec_nand(a.data, b.data);
    }

    static inline uint16x8 bitwise_and(uint16x8 a, uint16x8 b)
    {
        return vec_and(a.data, b.data);
    }

    static inline uint16x8 bitwise_or(uint16x8 a, uint16x8 b)
    {
        return vec_or(a.data, b.data);
    }

    static inline uint16x8 bitwise_xor(uint16x8 a, uint16x8 b)
    {
        return vec_xor(a.data, b.data);
    }

    static inline uint16x8 bitwise_not(uint16x8 a)
    {
        return vec_nor(a.data, a.data);
    }

    // compare

    static inline mask16x8 compare_neq(uint16x8 a, uint16x8 b)
    {
        auto mask = vec_cmpeq(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask16x8 compare_lt(uint16x8 a, uint16x8 b)
    {
        return vec_cmplt(a.data, b.data);
    }

    static inline mask16x8 compare_le(uint16x8 a, uint16x8 b)
    {
        auto mask = vec_cmpgt(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask16x8 compare_ge(uint16x8 a, uint16x8 b)
    {
        auto mask = vec_cmplt(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask16x8 compare_eq(uint16x8 a, uint16x8 b)
    {
        return vec_cmpeq(a.data, b.data);
    }

    static inline mask16x8 compare_gt(uint16x8 a, uint16x8 b)
    {
        return vec_cmpgt(a.data, b.data);
    }

    static inline uint16x8 select(mask16x8 mask, uint16x8 a, uint16x8 b)
    {
        return vec_sel(b.data, a.data, mask.data);
    }

    // shift by constant

    template <int Count>
    static inline uint16x8 slli(uint16x8 a)
    {
        return vec_sl(a.data, vec_splats(uint16(Count)));
    }

    template <int Count>
    static inline uint16x8 srli(uint16x8 a)
    {
        return vec_sr(a.data, vec_splats(uint16(Count)));
    }

    template <int Count>
    static inline uint16x8 srai(uint16x8 a)
    {
        return vec_sra(a.data, vec_splats(uint16(Count)));
    }

    // shift by scalar

    static inline uint16x8 sll(uint16x8 a, int count)
    {
        return vec_sl(a.data, vec_splats(uint16(count)));
    }

    static inline uint16x8 srl(uint16x8 a, int count)
    {
        return vec_sr(a.data, vec_splats(uint16(count)));
    }

    static inline uint16x8 sra(uint16x8 a, int count)
    {
        return vec_sra(a.data, vec_splats(uint16(count)));
    }

    static inline uint16x8 min(uint16x8 a, uint16x8 b)
    {
        return vec_min(a.data, b.data);
    }

    static inline uint16x8 max(uint16x8 a, uint16x8 b)
    {
        return vec_max(a.data, b.data);
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
        const uint8x16::vector mask = { VEC_SH4(x, 0), VEC_SH4(y, 0), VEC_SH4(z, 0), VEC_SH4(w, 0) };
        return vec_perm(v.data, v.data, mask);
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
        return vec_insert(s, a.data, Index);
    }

    template <unsigned int Index>
    static inline uint32 get_component(uint32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return vec_extract(a.data, Index);
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
        return (uint32x4::vector) { x, y, z, w };
    }

    static inline uint32x4 uint32x4_uload(const uint32* s)
    {
        return (uint32x4::vector) { s[0], s[1], s[2], s[3] };
    }

    static inline void uint32x4_ustore(uint32* dest, uint32x4 a)
    {
        dest[0] = vec_extract(a.data, 0);
        dest[1] = vec_extract(a.data, 1);
        dest[2] = vec_extract(a.data, 2);
        dest[3] = vec_extract(a.data, 3);
    }

    static inline uint32x4 uint32x4_load_low(const uint32* source)
    {
        auto s0 = source[0];
        auto s1 = source[1];
        return (uint32x4::vector) { s0, s1, 0, 0 };
    }

    static inline void uint32x4_store_low(uint32* dest, uint32x4 a)
    {
        dest[0] = vec_extract(a.data, 0);
        dest[1] = vec_extract(a.data, 1);
    }

    static inline uint32x4 unpacklo(uint32x4 a, uint32x4 b)
    {
        return vec_mergeh(a.data, b.data);
    }

    static inline uint32x4 unpackhi(uint32x4 a, uint32x4 b)
    {
        return vec_mergel(a.data, b.data);
    }

    static inline uint32x4 add(uint32x4 a, uint32x4 b)
    {
        return vec_add(a.data, b.data);
    }

    static inline uint32x4 sub(uint32x4 a, uint32x4 b)
    {
        return vec_sub(a.data, b.data);
    }

    static inline uint32x4 mullo(uint32x4 a, uint32x4 b)
    {
        float32x4 af = vec_ctf(a.data, 0);
        float32x4 bf = vec_ctf(b.data, 0);
        return vec_ctu(vec_mul(af.data, bf.data), 0);
    }

    // saturated

    static inline uint32x4 adds(uint32x4 a, uint32x4 b)
    {
        return vec_adds(a.data, b.data);
    }

    static inline uint32x4 subs(uint32x4 a, uint32x4 b)
    {
        return vec_subs(a.data, b.data);
    }

    // bitwise

    static inline uint32x4 bitwise_nand(uint32x4 a, uint32x4 b)
    {
        return vec_nand(a.data, b.data);
    }

    static inline uint32x4 bitwise_and(uint32x4 a, uint32x4 b)
    {
        return vec_and(a.data, b.data);
    }

    static inline uint32x4 bitwise_or(uint32x4 a, uint32x4 b)
    {
        return vec_or(a.data, b.data);
    }

    static inline uint32x4 bitwise_xor(uint32x4 a, uint32x4 b)
    {
        return vec_xor(a.data, b.data);
    }

    static inline uint32x4 bitwise_not(uint32x4 a)
    {
        return vec_nor(a.data, a.data);
    }

    // compare

    static inline mask32x4 compare_eq(uint32x4 a, uint32x4 b)
    {
        return vec_cmpeq(a.data, b.data);
    }

    static inline mask32x4 compare_gt(uint32x4 a, uint32x4 b)
    {
        return vec_cmpgt(a.data, b.data);
    }

    static inline mask32x4 compare_neq(uint32x4 a, uint32x4 b)
    {
        auto mask = vec_cmpeq(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask32x4 compare_lt(uint32x4 a, uint32x4 b)
    {
        return vec_cmplt(a.data, b.data);
    }

    static inline mask32x4 compare_le(uint32x4 a, uint32x4 b)
    {
        auto mask = vec_cmpgt(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask32x4 compare_ge(uint32x4 a, uint32x4 b)
    {
        auto mask = vec_cmplt(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline uint32x4 select(mask32x4 mask, uint32x4 a, uint32x4 b)
    {
        return vec_sel(b.data, a.data, mask.data);
    }

    // shift by constant

    template <int Count>
    static inline uint32x4 slli(uint32x4 a)
    {
        return vec_sl(a.data, vec_splats(uint32(Count)));
    }

    template <int Count>
    static inline uint32x4 srli(uint32x4 a)
    {
        return vec_sr(a.data, vec_splats(uint32(Count)));
    }

    template <int Count>
    static inline uint32x4 srai(uint32x4 a)
    {
        return vec_sra(a.data, vec_splats(uint32(Count)));
    }

    // shift by scalar

    static inline uint32x4 sll(uint32x4 a, int count)
    {
        return vec_sl(a.data, vec_splats(uint32(count)));
    }

    static inline uint32x4 srl(uint32x4 a, int count)
    {
        return vec_sr(a.data, vec_splats(uint32(count)));
    }

    static inline uint32x4 sra(uint32x4 a, int count)
    {
        return vec_sra(a.data, vec_splats(uint32(count)));
    }

    // shift by vector

    static inline uint32x4 sll(uint32x4 a, uint32x4 count)
    {
        return vec_sl(a.data, count.data);
    }

    static inline uint32x4 srl(uint32x4 a, uint32x4 count)
    {
        return vec_sr(a.data, count.data);
    }

    static inline uint32x4 sra(uint32x4 a, uint32x4 count)
    {
        return vec_sra(a.data, count.data);
    }

    static inline uint32x4 min(uint32x4 a, uint32x4 b)
    {
        return vec_min(a.data, b.data);
    }

    static inline uint32x4 max(uint32x4 a, uint32x4 b)
    {
        return vec_max(a.data, b.data);
    }

    // -----------------------------------------------------------------
    // uint64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint64x2 set_component(uint64x2 a, uint64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return vec_insert(s, a.data, Index);
    }

    template <unsigned int Index>
    static inline uint64 get_component(uint64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return vec_extract(a.data, Index);
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
        return (uint64x2::vector) { x, y };
    }

    static inline uint64x2 unpacklo(uint64x2 a, uint64x2 b)
    {
        return vec_insert(vec_extract(b.data, 0), a.data, 1);
    }

    static inline uint64x2 unpackhi(uint64x2 a, uint64x2 b)
    {
        return vec_insert(vec_extract(a.data, 1), b.data, 0);
    }

    static inline uint64x2 add(uint64x2 a, uint64x2 b)
    {
        return vec_add(a.data, b.data);
    }

    static inline uint64x2 sub(uint64x2 a, uint64x2 b)
    {
        return vec_sub(a.data, b.data);
    }

    static inline uint64x2 bitwise_nand(uint64x2 a, uint64x2 b)
    {
        return vec_nand(a.data, b.data);
    }

    static inline uint64x2 bitwise_and(uint64x2 a, uint64x2 b)
    {
        return vec_and(a.data, b.data);
    }

    static inline uint64x2 bitwise_or(uint64x2 a, uint64x2 b)
    {
        return vec_or(a.data, b.data);
    }

    static inline uint64x2 bitwise_xor(uint64x2 a, uint64x2 b)
    {
        return vec_xor(a.data, b.data);
    }

    static inline uint64x2 bitwise_not(uint64x2 a)
    {
        return vec_nor(a.data, a.data);
    }

    static inline uint64x2 select(mask64x2 mask, uint64x2 a, uint64x2 b)
    {
        return vec_sel(b.data, a.data, mask.data);
    }

    // shift by constant

    template <int Count>
    static inline uint64x2 slli(uint64x2 a)
    {
        a = vec_insert(vec_extract(a.data, 0) << Count, a.data, 0);
        a = vec_insert(vec_extract(a.data, 1) << Count, a.data, 1);
        return a;
    }

    template <int Count>
    static inline uint64x2 srli(uint64x2 a)
    {
        a = vec_insert(vec_extract(a.data, 0) >> Count, a.data, 0);
        a = vec_insert(vec_extract(a.data, 1) >> Count, a.data, 1);
        return a;
    }

    // shift by scalar

    static inline uint64x2 sll(uint64x2 a, int count)
    {
        a = vec_insert(vec_extract(a.data, 0) << count, a.data, 0);
        a = vec_insert(vec_extract(a.data, 1) << count, a.data, 1);
        return a;
    }

    static inline uint64x2 srl(uint64x2 a, int count)
    {
        a = vec_insert(vec_extract(a.data, 0) >> count, a.data, 0);
        a = vec_insert(vec_extract(a.data, 1) >> count, a.data, 1);
        return a;
    }

    // -----------------------------------------------------------------
    // int8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int8x16 set_component(int8x16 a, int8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return vec_insert(s, a.data, Index);
    }

    template <unsigned int Index>
    static inline int8 get_component(int8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return vec_extract(a.data, Index);
    }

    static inline int8x16 int8x16_zero()
    {
        return vec_splats(int8(0));
    }

    static inline int8x16 int8x16_set1(int8 s)
    {
        return vec_splats(s);
    }

    static inline int8x16 int8x16_set16(
        int8 s0, int8 s1, int8 s2, int8 s3, int8 s4, int8 s5, int8 s6, int8 s7,
        int8 s8, int8 s9, int8 s10, int8 s11, int8 s12, int8 s13, int8 s14, int8 s15)
    {
        return (int8x16::vector) { s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15 };
    }

    static inline int8x16 int8x16_load_low(const int8* source)
    {
        auto s0 = source[0];
        auto s1 = source[1];
        auto s2 = source[2];
        auto s3 = source[3];
        auto s4 = source[4];
        auto s5 = source[5];
        auto s6 = source[6];
        auto s7 = source[7];
        return (int8x16::vector) { s0, s1, s2, s3, s4, s5, s6, s7, 0, 0, 0, 0, 0, 0, 0, 0 };
    }

    static inline void int8x16_store_low(int8* dest, int8x16 a)
    {
        std::memcpy(dest, &a, 8);
    }

    static inline int8x16 unpacklo(int8x16 a, int8x16 b)
    {
        return vec_mergeh(a.data, b.data);
    }

    static inline int8x16 unpackhi(int8x16 a, int8x16 b)
    {
        return vec_mergel(a.data, b.data);
    }

    static inline int8x16 add(int8x16 a, int8x16 b)
    {
        return vec_add(a.data, b.data);
    }

    static inline int8x16 sub(int8x16 a, int8x16 b)
    {
        return vec_sub(a.data, b.data);
    }

    // saturated

    static inline int8x16 adds(int8x16 a, int8x16 b)
    {
        return vec_adds(a.data, b.data);
    }

    static inline int8x16 subs(int8x16 a, int8x16 b)
    {
        return vec_subs(a.data, b.data);
    }

    static inline int8x16 abs(int8x16 a)
    {
        return vec_abs(a.data);
    }

    static inline int8x16 neg(int8x16 a)
    {
        return vec_sub(vec_xor(a.data, a.data), a.data);
    }

    // bitwise

    static inline int8x16 bitwise_nand(int8x16 a, int8x16 b)
    {
        return vec_nand(a.data, b.data);
    }

    static inline int8x16 bitwise_and(int8x16 a, int8x16 b)
    {
        return vec_and(a.data, b.data);
    }

    static inline int8x16 bitwise_or(int8x16 a, int8x16 b)
    {
        return vec_or(a.data, b.data);
    }

    static inline int8x16 bitwise_xor(int8x16 a, int8x16 b)
    {
        return vec_xor(a.data, b.data);
    }

    static inline int8x16 bitwise_not(int8x16 a)
    {
        return vec_nor(a.data, a.data);
    }

    // compare

    static inline mask8x16 compare_eq(int8x16 a, int8x16 b)
    {
        return vec_cmpeq(a.data, b.data);
    }

    static inline mask8x16 compare_gt(int8x16 a, int8x16 b)
    {
        return vec_cmpgt(a.data, b.data);
    }

    static inline mask8x16 compare_neq(int8x16 a, int8x16 b)
    {
        auto mask = vec_cmpeq(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask8x16 compare_lt(int8x16 a, int8x16 b)
    {
        return vec_cmplt(a.data, b.data);
    }

    static inline mask8x16 compare_le(int8x16 a, int8x16 b)
    {
        auto mask = vec_cmpgt(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask8x16 compare_ge(int8x16 a, int8x16 b)
    {
        auto mask = vec_cmplt(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline int8x16 select(mask8x16 mask, int8x16 a, int8x16 b)
    {
        return vec_sel(b.data, a.data, mask.data);
    }

    static inline int8x16 min(int8x16 a, int8x16 b)
    {
        return vec_min(a.data, b.data);
    }

    static inline int8x16 max(int8x16 a, int8x16 b)
    {
        return vec_max(a.data, b.data);
    }

    // -----------------------------------------------------------------
    // int16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int16x8 set_component(int16x8 a, int16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return vec_insert(s, a.data, Index);
    }

    template <unsigned int Index>
    static inline int16 get_component(int16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return vec_extract(a.data, Index);
    }

    static inline int16x8 int16x8_zero()
    {
        return vec_splats(int16(0));
    }

    static inline int16x8 int16x8_set1(int16 s)
    {
        return vec_splats(s);
    }

    static inline int16x8 int16x8_set8(int16 s0, int16 s1, int16 s2, int16 s3, int16 s4, int16 s5, int16 s6, int16 s7)
    {
        return (int16x8::vector) { s0, s1, s2, s3, s4, s5, s6, s7 };
    }

    static inline int16x8 int16x8_load_low(const int16* source)
    {
        auto s0 = source[0];
        auto s1 = source[1];
        auto s2 = source[2];
        auto s3 = source[3];
        return (int16x8::vector) { s0, s1, s2, s3, 0, 0, 0, 0 };
    }

    static inline void int16x8_store_low(int16* dest, int16x8 a)
    {
        std::memcpy(dest, &a, 8);
    }

    static inline int16x8 unpacklo(int16x8 a, int16x8 b)
    {
        return vec_mergeh(a.data, b.data);
    }

    static inline int16x8 unpackhi(int16x8 a, int16x8 b)
    {
        return vec_mergel(a.data, b.data);
    }

    static inline int16x8 add(int16x8 a, int16x8 b)
    {
        return vec_add(a.data, b.data);
    }

    static inline int16x8 sub(int16x8 a, int16x8 b)
    {
        return vec_sub(a.data, b.data);
    }

    static inline int16x8 mullo(int16x8 a, int16x8 b)
    {
        return vec_mladd(a.data, b.data, vec_xor(a.data, a.data));
    }

    // saturated

    static inline int16x8 adds(int16x8 a, int16x8 b)
    {
        return vec_adds(a.data, b.data);
    }

    static inline int16x8 subs(int16x8 a, int16x8 b)
    {
        return vec_subs(a.data, b.data);
    }

    static inline int16x8 abs(int16x8 a)
    {
        return vec_abs(a.data);
    }

    static inline int16x8 neg(int16x8 a)
    {
        return vec_sub(vec_xor(a.data, a.data), a.data);
    }

    // bitwise

    static inline int16x8 bitwise_nand(int16x8 a, int16x8 b)
    {
        return vec_nand(a.data, b.data);
    }

    static inline int16x8 bitwise_and(int16x8 a, int16x8 b)
    {
        return vec_and(a.data, b.data);
    }

    static inline int16x8 bitwise_or(int16x8 a, int16x8 b)
    {
        return vec_or(a.data, b.data);
    }

    static inline int16x8 bitwise_xor(int16x8 a, int16x8 b)
    {
        return vec_xor(a.data, b.data);
    }

    static inline int16x8 bitwise_not(int16x8 a)
    {
        return vec_nor(a.data, a.data);
    }

    // compare

    static inline mask16x8 compare_eq(int16x8 a, int16x8 b)
    {
        return vec_cmpeq(a.data, b.data);
    }

    static inline mask16x8 compare_gt(int16x8 a, int16x8 b)
    {
        return vec_cmpgt(a.data, b.data);
    }

    static inline mask16x8 compare_neq(int16x8 a, int16x8 b)
    {
        auto mask = vec_cmpeq(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask16x8 compare_lt(int16x8 a, int16x8 b)
    {
        return vec_cmplt(a.data, b.data);
    }

    static inline mask16x8 compare_le(int16x8 a, int16x8 b)
    {
        auto mask = vec_cmpgt(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask16x8 compare_ge(int16x8 a, int16x8 b)
    {
        auto mask = vec_cmplt(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline int16x8 select(mask16x8 mask, int16x8 a, int16x8 b)
    {
        return vec_sel(b.data, a.data, mask.data);
    }

    // shift by constant

    template <int Count>
    static inline int16x8 slli(int16x8 a)
    {
        return vec_sl(a.data, vec_splats(uint16(Count)));
    }

    template <int Count>
    static inline int16x8 srli(int16x8 a)
    {
        return vec_sr(a.data, vec_splats(uint16(Count)));
    }

    template <int Count>
    static inline int16x8 srai(int16x8 a)
    {
        return vec_sra(a.data, vec_splats(uint16(Count)));
    }

    // shift by scalar

    static inline int16x8 sll(int16x8 a, int count)
    {
        return vec_sl(a.data, vec_splats(uint16(count)));
    }

    static inline int16x8 srl(int16x8 a, int count)
    {
        return vec_sr(a.data, vec_splats(uint16(count)));
    }

    static inline int16x8 sra(int16x8 a, int count)
    {
        return vec_sra(a.data, vec_splats(uint16(count)));
    }

    static inline int16x8 min(int16x8 a, int16x8 b)
    {
        return vec_min(a.data, b.data);
    }

    static inline int16x8 max(int16x8 a, int16x8 b)
    {
        return vec_max(a.data, b.data);
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
        const uint8x16::vector mask = { VEC_SH4(x, 0), VEC_SH4(y, 0), VEC_SH4(z, 0), VEC_SH4(w, 0) };
        return vec_perm(v.data, v.data, mask);
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
        return vec_insert(s, a.data, Index);
    }

    template <unsigned int Index>
    static inline int32 get_component(int32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return vec_extract(a.data, Index);
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
        return (int32x4::vector) { x, y, z, w };
    }

    static inline int32x4 int32x4_uload(const int* s)
    {
        return (int32x4::vector) { s[0], s[1], s[2], s[3] };
    }

    static inline void int32x4_ustore(int* dest, int32x4 a)
    {
        dest[0] = vec_extract(a.data, 0);
        dest[1] = vec_extract(a.data, 1);
        dest[2] = vec_extract(a.data, 2);
        dest[3] = vec_extract(a.data, 3);
    }

    static inline int32x4 int32x4_load_low(const int32* source)
    {
        auto s0 = source[0];
        auto s1 = source[1];
        return (int32x4::vector) { s0, s1, 0, 0 };
    }

    static inline void int32x4_store_low(int32* dest, int32x4 a)
    {
        dest[0] = vec_extract(a.data, 0);
        dest[1] = vec_extract(a.data, 1);
    }

    static inline int32x4 unpacklo(int32x4 a, int32x4 b)
    {
        return vec_mergeh(a.data, b.data);
    }

    static inline int32x4 unpackhi(int32x4 a, int32x4 b)
    {
        return vec_mergel(a.data, b.data);
    }

    static inline int32x4 abs(int32x4 a)
    {
        return vec_abs(a.data);
    }

    static inline int32x4 neg(int32x4 a)
    {
        return vec_sub(vec_xor(a.data, a.data), a.data);
    }

    static inline int32x4 add(int32x4 a, int32x4 b)
    {
        return vec_add(a.data, b.data);
    }

    static inline int32x4 sub(int32x4 a, int32x4 b)
    {
        return vec_sub(a.data, b.data);
    }

    static inline int32x4 mullo(int32x4 a, int32x4 b)
    {
        float32x4 af = vec_ctf(a.data, 0);
        float32x4 bf = vec_ctf(b.data, 0);
        return vec_cts(vec_mul(af.data, bf.data), 0);
    }

    // saturated

    static inline int32x4 adds(int32x4 a, int32x4 b)
    {
        return vec_adds(a.data, b.data);
    }

    static inline int32x4 subs(int32x4 a, int32x4 b)
    {
        return vec_subs(a.data, b.data);
    }

    // bitwise

    static inline int32x4 bitwise_nand(int32x4 a, int32x4 b)
    {
        return vec_nand(a.data, b.data);
    }

    static inline int32x4 bitwise_and(int32x4 a, int32x4 b)
    {
        return vec_and(a.data, b.data);
    }

    static inline int32x4 bitwise_or(int32x4 a, int32x4 b)
    {
        return vec_or(a.data, b.data);
    }

    static inline int32x4 bitwise_xor(int32x4 a, int32x4 b)
    {
        return vec_xor(a.data, b.data);
    }

    static inline int32x4 bitwise_not(int32x4 a)
    {
        return vec_nor(a.data, a.data);
    }

    // compare

    static inline mask32x4 compare_eq(int32x4 a, int32x4 b)
    {
        return vec_cmpeq(a.data, b.data);
    }

    static inline mask32x4 compare_gt(int32x4 a, int32x4 b)
    {
        return vec_cmpgt(a.data, b.data);
    }

    static inline mask32x4 compare_neq(int32x4 a, int32x4 b)
    {
        auto mask = vec_cmpeq(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask32x4 compare_lt(int32x4 a, int32x4 b)
    {
        return vec_cmplt(a.data, b.data);
    }

    static inline mask32x4 compare_le(int32x4 a, int32x4 b)
    {
        auto mask = vec_cmpgt(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline mask32x4 compare_ge(int32x4 a, int32x4 b)
    {
        auto mask = vec_cmplt(a.data, b.data);
        return vec_nor(mask, mask);
    }

    static inline int32x4 select(mask32x4 mask, int32x4 a, int32x4 b)
    {
        return vec_sel(b.data, a.data, mask.data);
    }

    // shift by constant

    template <int Count>
    static inline int32x4 slli(int32x4 a)
    {
        return vec_sl(a.data, vec_splats(uint32(Count)));
    }

    template <int Count>
    static inline int32x4 srli(int32x4 a)
    {
        return vec_sr(a.data, vec_splats(uint32(Count)));
    }

    template <int Count>
    static inline int32x4 srai(int32x4 a)
    {
        return vec_sra(a.data, vec_splats(uint32(Count)));
    }

    // shift by scalar

    static inline int32x4 sll(int32x4 a, int count)
    {
        return vec_sl(a.data, vec_splats(uint32(count)));
    }

    static inline int32x4 srl(int32x4 a, int count)
    {
        return vec_sr(a.data, vec_splats(uint32(count)));
    }

    static inline int32x4 sra(int32x4 a, int count)
    {
        return vec_sra(a.data, vec_splats(uint32(count)));
    }

    // shift by vector

    static inline int32x4 sll(int32x4 a, uint32x4 count)
    {
        return vec_sl(a.data, count.data);
    }

    static inline int32x4 srl(int32x4 a, uint32x4 count)
    {
        return vec_sr(a.data, count.data);
    }

    static inline int32x4 sra(int32x4 a, uint32x4 count)
    {
        return vec_sra(a.data, count.data);
    }

    static inline uint32 pack(int32x4 s)
    {
        int32x4 v = vec_sl(s.data, (uint32x4::vector) { 0, 8, 16, 24 });
        v = vec_or(vec_mergeh(v.data, v.data), vec_mergel(v.data, v.data));
        v = vec_or(vec_mergeh(v.data, v.data), vec_mergel(v.data, v.data));
        return vec_extract(v.data, 0);
    }

    static inline int32x4 min(int32x4 a, int32x4 b)
    {
        return vec_min(a.data, b.data);
    }

    static inline int32x4 max(int32x4 a, int32x4 b)
    {
        return vec_max(a.data, b.data);
    }

    static inline int32x4 unpack(uint32 s)
    {
        uint32x4 v = vec_splats(s);
        v = vec_and(v.data, (uint32x4::vector) { 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 });
        return (int32x4::vector) vec_sr(v.data, (uint32x4::vector) { 0, 8, 16, 24 });
    }

    // -----------------------------------------------------------------
    // int64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int64x2 set_component(int64x2 a, int64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return vec_insert(s, a.data, Index);
    }

    template <unsigned int Index>
    static inline int64 get_component(int64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return vec_extract(a.data, Index);
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
        return (int64x2::vector) { x, y };
    }

    static inline int64x2 unpacklo(int64x2 a, int64x2 b)
    {
        return vec_insert(vec_extract(b.data, 0), a.data, 1);
    }

    static inline int64x2 unpackhi(int64x2 a, int64x2 b)
    {
        return vec_insert(vec_extract(a.data, 1), b.data, 0);
    }

    static inline int64x2 add(int64x2 a, int64x2 b)
    {
        return vec_add(a.data, b.data);
    }

    static inline int64x2 sub(int64x2 a, int64x2 b)
    {
        return vec_sub(a.data, b.data);
    }

    static inline int64x2 bitwise_nand(int64x2 a, int64x2 b)
    {
        return vec_nand(a.data, b.data);
    }

    static inline int64x2 bitwise_and(int64x2 a, int64x2 b)
    {
        return vec_and(a.data, b.data);
    }

    static inline int64x2 bitwise_or(int64x2 a, int64x2 b)
    {
        return vec_or(a.data, b.data);
    }

    static inline int64x2 bitwise_xor(int64x2 a, int64x2 b)
    {
        return vec_xor(a.data, b.data);
    }

    static inline int64x2 bitwise_not(int64x2 a)
    {
        return vec_nor(a.data, a.data);
    }

    static inline int64x2 select(mask64x2 mask, int64x2 a, int64x2 b)
    {
        return vec_sel(b.data, a.data, mask.data);
    }

    // shift by constant

    template <int Count>
    static inline int64x2 slli(int64x2 a)
    {
        a = vec_insert(vec_extract(a.data, 0) << Count, a.data, 0);
        a = vec_insert(vec_extract(a.data, 1) << Count, a.data, 1);
        return a;
    }

    template <int Count>
    static inline int64x2 srli(int64x2 a)
    {
        a = vec_insert(vec_extract(a.data, 0) >> Count, a.data, 0);
        a = vec_insert(vec_extract(a.data, 1) >> Count, a.data, 1);
        return a;
    }

    // shift by scalar

    static inline int64x2 sll(int64x2 a, int count)
    {
        a = vec_insert(vec_extract(a.data, 0) << count, a.data, 0);
        a = vec_insert(vec_extract(a.data, 1) << count, a.data, 1);
        return a;
    }

    static inline int64x2 srl(int64x2 a, int count)
    {
        a = vec_insert(vec_extract(a.data, 0) >> count, a.data, 0);
        a = vec_insert(vec_extract(a.data, 1) >> count, a.data, 1);
        return a;
    }

    // -----------------------------------------------------------------
    // mask8x16
    // -----------------------------------------------------------------

    static inline mask8x16 operator & (mask8x16 a, mask8x16 b)
    {
        return vec_and(a.data, b.data);
    }

    static inline mask8x16 operator | (mask8x16 a, mask8x16 b)
    {
        return vec_or(a.data, b.data);
    }

    static inline mask8x16 operator ^ (mask8x16 a, mask8x16 b)
    {
        return vec_xor(a.data, b.data);
    }

    static inline uint32 get_mask(mask8x16 a)
    {
        const uint32x4::vector zero = uint32x4_zero();
        uint8x16::vector masked = vec_and(a.data, (uint8x16::vector) { 1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128 });
        int32x4::vector sum = (int32x4::vector) vec_sl(vec_sum4s(masked, zero), (uint32x4::vector) { 0, 0, 8, 8 });
        return vec_extract(vec_sums(sum, vec_xor(sum, sum)), 3);
    }

    // -----------------------------------------------------------------
    // mask16x8
    // -----------------------------------------------------------------

    static inline mask16x8 operator & (mask16x8 a, mask16x8 b)
    {
        return vec_and(a.data, b.data);
    }

    static inline mask16x8 operator | (mask16x8 a, mask16x8 b)
    {
        return vec_or(a.data, b.data);
    }

    static inline mask16x8 operator ^ (mask16x8 a, mask16x8 b)
    {
        return vec_xor(a.data, b.data);
    }

    static inline uint32 get_mask(mask16x8 a)
    {
        const int32x4::vector zero = int32x4_zero();
        int16x8::vector masked = (int16x8::vector) vec_and(a.data, (int16x8::vector) { 1, 2, 4, 8, 16, 32, 64, 128 });
        int32x4::vector sum = vec_sum4s(masked, zero);
        return vec_extract(vec_sums(sum, zero), 3);
    }

    // -----------------------------------------------------------------
    // mask32x4
    // -----------------------------------------------------------------

    static inline mask32x4 operator & (mask32x4 a, mask32x4 b)
    {
        return vec_and(a.data, b.data);
    }

    static inline mask32x4 operator | (mask32x4 a, mask32x4 b)
    {
        return vec_or(a.data, b.data);
    }

    static inline mask32x4 operator ^ (mask32x4 a, mask32x4 b)
    {
        return vec_xor(a.data, b.data);
    }

    static inline uint32 get_mask(mask32x4 a)
    {
        const int32x4::vector zero = int32x4_zero();
        int32x4::vector masked = (int32x4::vector) vec_and(a.data, (int32x4::vector) { 1, 2, 4, 8 });
        return vec_extract(vec_sums(masked, zero), 3);
    }

    // -----------------------------------------------------------------
    // mask64x2
    // -----------------------------------------------------------------

    static inline mask64x2 operator & (mask64x2 a, mask64x2 b)
    {
        return (mask64x2::vector) vec_and((uint64x2::vector)a.data, (uint64x2::vector)b.data);
    }

    static inline mask64x2 operator | (mask64x2 a, mask64x2 b)
    {
        return (mask64x2::vector) vec_or((uint64x2::vector)a.data, (uint64x2::vector)b.data);
    }

    static inline mask64x2 operator ^ (mask64x2 a, mask64x2 b)
    {
        return (mask64x2::vector) vec_xor((uint64x2::vector)a.data, (uint64x2::vector)b.data);
    }

    static inline uint32 get_mask(mask64x2 a)
    {
        uint64x2 temp = (uint64x2::vector) a.data;
        uint32 x = uint32(get_component<0>(temp)) & 1;
        uint32 y = uint32(get_component<1>(temp)) & 2;
        return x | y;
    }

} // namespace simd
} // namespace mango
