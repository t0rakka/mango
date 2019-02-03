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
    static inline uint8x16 set_component(uint8x16 a, u8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return (v16u8) __msa_insert_b((v16i8) a, Index, s);
    }

    template <unsigned int Index>
    static inline u8 get_component(uint8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return __msa_copy_u_b(a, Index);
    }

    static inline uint8x16 uint8x16_zero()
    {
        return (v16u8) __msa_fill_b(0);
    }

    static inline uint8x16 uint8x16_set1(u8 s)
    {
        return (v16u8) __msa_fill_b(s);
    }

    static inline uint8x16 uint8x16_set16(
        u8 s0, u8 s1, u8 s2, u8 s3, u8 s4, u8 s5, u8 s6, u8 s7,
        u8 s8, u8 s9, u8 s10, u8 s11, u8 s12, u8 s13, u8 s14, u8 s15)
    {
        return (v16u8) { s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15 };
    }

    static inline uint8x16 uint8x16_load_low(const uint8* source)
    {
        return (v2u64) { uload64(source), 0 };
    }

    static inline void uint8x16_store_low(u8* dest, uint8x16 a)
    {
        std::memcpy(dest, &a, 8);
    }

    static inline uint8x16 unpacklo(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_ilvr_b((v16i8)b, (v16i8)a);
    }

    static inline uint8x16 unpackhi(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_ilvl_b((v16i8)b, (v16i8)a);
    }

    static inline uint8x16 add(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_addv_b((v16i8)a, (v16i8)b);
    }

    static inline uint8x16 sub(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_subv_b((v16i8)a, (v16i8)b);
    }

    // saturated

    static inline uint8x16 adds(uint8x16 a, uint8x16 b)
    {
        return __msa_adds_u_b(a, b);
    }

    static inline uint8x16 subs(uint8x16 a, uint8x16 b)
    {
        return __msa_subs_u_b(a, b);
    }

    // bitwise

    static inline uint8x16 bitwise_nand(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_and_v((v16u8)a, __msa_nor_v((v16u8)b, (v16u8)b));
    }

    static inline uint8x16 bitwise_and(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline uint8x16 bitwise_or(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline uint8x16 bitwise_xor(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline uint8x16 bitwise_not(uint8x16 a)
    {
        return (v16u8) __msa_nor_v((v16u8) a, (v16u8) a);
    }

    // compare

    static inline mask8x16 compare_eq(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_ceq_b((v16i8) a, (v16i8) b);
    }

    static inline mask8x16 compare_gt(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_clt_u_b(b, a);
    }

    static inline mask8x16 compare_neq(uint8x16 a, uint8x16 b)
    {
        auto mask = (v16u8) __msa_ceq_b((v16i8) a, (v16i8) b);
        return (v16u8) __msa_nor_v(mask, mask);
    }

    static inline mask8x16 compare_lt(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_clt_u_b(a, b);
    }

    static inline mask8x16 compare_le(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_cle_u_b(a, b);
    }

    static inline mask8x16 compare_ge(uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_cle_u_b(b, a);
    }

    static inline uint8x16 select(mask8x16 mask, uint8x16 a, uint8x16 b)
    {
        return (v16u8) __msa_bsel_v((v16u8) mask, (v16u8) a, (v16u8) b);
    }

    static inline uint8x16 min(uint8x16 a, uint8x16 b)
    {
        return __msa_min_u_b(a, b);
    }

    static inline uint8x16 max(uint8x16 a, uint8x16 b)
    {
        return __msa_max_u_b(a, b);
    }

    // -----------------------------------------------------------------
    // uint16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint16x8 set_component(uint16x8 a, u16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return (v8u16) __msa_insert_h((v8i16) a, Index, s);
    }

    template <unsigned int Index>
    static inline u16 get_component(uint16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return __msa_copy_u_h(a, Index);
    }

    static inline uint16x8 uint16x8_zero()
    {
        return (v8u16) __msa_fill_h(0);
    }

    static inline uint16x8 uint16x8_set1(u16 s)
    {
        return (v8u16) __msa_fill_h(s);
    }

    static inline uint16x8 uint16x8_set8(u16 s0, u16 s1, u16 s2, u16 s3, u16 s4, u16 s5, u16 s6, u16 s7)
    {
        return (v8u16) { s0, s1, s2, s3, s4, s5, s6, s7 };
    }

    static inline uint16x8 uint16x8_load_low(const u16* source)
    {
        return (v8u16) { source[0], source[1], source[2], source[3], 0, 0, 0, 0 };
    }

    static inline void uint16x8_store_low(u16* dest, uint16x8 a)
    {
        std::memcpy(dest, &a, 8);
    }

    static inline uint16x8 unpacklo(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_ilvr_h((v8i16)b, (v8i16)a);
    }

    static inline uint16x8 unpackhi(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_ilvl_h((v8i16)b, (v8i16)a);
    }

    static inline uint16x8 add(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_addv_h((v8i16)a, (v8i16)b);
    }

    static inline uint16x8 sub(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_subv_h((v8i16)a, (v8i16)b);
    }

    static inline uint16x8 mullo(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_mulv_h((v8i16) a, (v8i16) b);
    }

    // saturated

    static inline uint16x8 adds(uint16x8 a, uint16x8 b)
    {
        return __msa_adds_u_h(a, b);
    }

    static inline uint16x8 subs(uint16x8 a, uint16x8 b)
    {
        return __msa_subs_u_h(a, b);
    }

    // bitwise

    static inline uint16x8 bitwise_nand(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_and_v((v16u8)a, __msa_nor_v((v16u8)b, (v16u8)b));
    }

    static inline uint16x8 bitwise_and(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline uint16x8 bitwise_or(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline uint16x8 bitwise_xor(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline uint16x8 bitwise_not(uint16x8 a)
    {
        return (v8u16) __msa_nor_v((v16u8) a, (v16u8) a);
    }

    // compare

    static inline mask16x8 compare_neq(uint16x8 a, uint16x8 b)
    {
        auto mask = (v8u16) __msa_ceq_h((v8i16) a, (v8i16) b);
        return (v8u16) __msa_nor_v(mask, mask);
    }

    static inline mask16x8 compare_lt(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_clt_u_h(a, b);
    }

    static inline mask16x8 compare_le(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_cle_u_h(a, b);
    }

    static inline mask16x8 compare_ge(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_cle_u_h(b, a);
    }

    static inline mask16x8 compare_eq(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_ceq_h((v8i16) a, (v8i16) b);
    }

    static inline mask16x8 compare_gt(uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_clt_u_h(b, a);
    }

    static inline uint16x8 select(mask16x8 mask, uint16x8 a, uint16x8 b)
    {
        return (v8u16) __msa_bsel_v((v16u8) mask, (v16u8) a, (v16u8) b);
    }

    // shift by constant

    template <int Count>
    static inline uint16x8 slli(uint16x8 a)
    {
        return (v8u16) __msa_slli_h((v8i16)a, Count);
    }

    template <int Count>
    static inline uint16x8 srli(uint16x8 a)
    {
        return (v8u16) __msa_srli_h((v8i16)a, Count);
    }

    template <int Count>
    static inline uint16x8 srai(uint16x8 a)
    {
        return (v8u16) __msa_srai_h((v8i16)a, Count);
    }

    // shift by scalar

    static inline uint16x8 sll(uint16x8 a, int count)
    {
        return (v8u16) __msa_sll_h((v8u16) a, (v8i16) __msa_fill_h(count));
    }

    static inline uint16x8 srl(uint16x8 a, int count)
    {
        return (v8u16) __msa_srl_h((v8u16) a, (v8i16) __msa_fill_h(count));
    }

    static inline uint16x8 sra(uint16x8 a, int count)
    {
        return (v8u16) __msa_sra_h((v8u16) a, (v8i16) __msa_fill_h(count));
    }

    static inline uint16x8 min(uint16x8 a, uint16x8 b)
    {
        return __msa_min_u_h(a, b);
    }

    static inline uint16x8 max(uint16x8 a, uint16x8 b)
    {
        return __msa_max_u_h(a, b);
    }

    // -----------------------------------------------------------------
    // uint32x4
    // -----------------------------------------------------------------

    // shuffle

    template <u32 x, u32 y, u32 z, u32 w>
    static inline uint32x4 shuffle(uint32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const v4i32 control = (v4i32) { x, y, z, w };
        return (v4u32) __msa_vshf_w(control, (v4i32) v, (v4i32) v);
    }

    template <>
    inline uint32x4 shuffle<0, 1, 2, 3>(uint32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline uint32x4 set_component(uint32x4 a, u32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return (v4u32) __msa_insert_w((v4i32) a, Index, s);
    }

    template <unsigned int Index>
    static inline u32 get_component(uint32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return __msa_copy_u_w(a, Index);
    }

    static inline uint32x4 uint32x4_zero()
    {
        return (v4u32) __msa_fill_w(0);
    }

    static inline uint32x4 uint32x4_set1(u32 s)
    {
        return (v4u32) __msa_fill_w(s);
    }

    static inline uint32x4 uint32x4_set4(u32 x, u32 y, u32 z, u32 w)
    {
        return (v4u32) { x, y, z, w };
    }

    static inline uint32x4 uint32x4_uload(const u32* source)
    {
        return reinterpret_cast<const v4u32 *>(source)[0];
    }

    static inline void uint32x4_ustore(u32* dest, uint32x4 a)
    {
        reinterpret_cast<v4u32 *>(dest)[0] = a;
    }

    static inline uint32x4 uint32x4_load_low(const u32* source)
    {
        return (v4u32) { source[0], source[1], 0, 0 };
    }

    static inline void uint32x4_store_low(u32* dest, uint32x4 a)
    {
        std::memcpy(dest, &a, 8);
    }

    static inline uint32x4 unpacklo(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_ilvr_w((v4i32)b, (v4i32)a);
    }

    static inline uint32x4 unpackhi(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_ilvl_w((v4i32)b, (v4i32)a);
    }

    static inline uint32x4 add(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_addv_w((v4i32)a, (v4i32)b);
    }

    static inline uint32x4 sub(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_subv_w((v4i32)a, (v4i32)b);
    }

    static inline uint32x4 mullo(uint32x4 a, uint32x4 b)
    {
        return (uint32x4) __msa_mulv_w((int32x4) a, (int32x4) b);
    }

    // saturated

    static inline uint32x4 adds(uint32x4 a, uint32x4 b)
    {
        return __msa_adds_u_w(a, b);
    }

    static inline uint32x4 subs(uint32x4 a, uint32x4 b)
    {
        return __msa_subs_u_w(a, b);
    }

    // bitwise

    static inline uint32x4 bitwise_nand(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_and_v((v16u8)a, __msa_nor_v((v16u8)b, (v16u8)b));
    }

    static inline uint32x4 bitwise_and(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline uint32x4 bitwise_or(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline uint32x4 bitwise_xor(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline uint32x4 bitwise_not(uint32x4 a)
    {
        return (v4u32) __msa_nor_v((v16u8) a, (v16u8) a);
    }

    // compare

    static inline mask32x4 compare_eq(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_ceq_w((v4i32) a, (v4i32) b);
    }

    static inline mask32x4 compare_gt(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_clt_u_w(b, a);
    }

    static inline mask32x4 compare_neq(uint32x4 a, uint32x4 b)
    {
        auto mask = (v4u32) __msa_ceq_w((v4i32) a, (v4i32) b);
        return (v4u32) __msa_nor_v(mask, mask);
    }

    static inline mask32x4 compare_lt(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_clt_u_w(a, b);
    }

    static inline mask32x4 compare_le(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_cle_u_w(a, b);
    }

    static inline mask32x4 compare_ge(uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_cle_u_w(b, a);
    }

    static inline uint32x4 select(mask32x4 mask, uint32x4 a, uint32x4 b)
    {
        return (v4u32) __msa_bsel_v((v16u8) mask, (v16u8) a, (v16u8) b);
    }

    // shift by constant

    template <int Count>
    static inline uint32x4 slli(uint32x4 a)
    {
        return (v4u32) __msa_slli_w((v4i32)a, Count);
    }

    template <int Count>
    static inline uint32x4 srli(uint32x4 a)
    {
        return (v4u32) __msa_srli_w((v4i32)a, Count);
    }

    template <int Count>
    static inline uint32x4 srai(uint32x4 a)
    {
        return (v4u32) __msa_srai_w((v4i32)a, Count);
    }

    // shift by scalar

    static inline uint32x4 sll(uint32x4 a, int count)
    {
        return (v4u32) __msa_sll_w((v4u32) a, (v4i32) __msa_fill_w(count));
    }

    static inline uint32x4 srl(uint32x4 a, int count)
    {
        return (v4u32) __msa_srl_w((v4u32) a, (v4i32) __msa_fill_w(count));
    }

    static inline uint32x4 sra(uint32x4 a, int count)
    {
        return (v4u32) __msa_sra_w((v4u32) a, (v4i32) __msa_fill_w(count));
    }

    // shift by vector

    static inline uint32x4 sll(uint32x4 a, uint32x4 count)
    {
        return (v4u32) __msa_sll_w((v4i32)a, (v4i32)count);
    }

    static inline uint32x4 srl(uint32x4 a, uint32x4 count)
    {
        return (v4u32) __msa_srl_w((v4i32)a, (v4i32)count);
    }

    static inline uint32x4 sra(uint32x4 a, uint32x4 count)
    {
        return (v4u32) __msa_sra_w((v4i32)a, (v4i32)count);
    }

    static inline uint32x4 min(uint32x4 a, uint32x4 b)
    {
        return __msa_min_u_w(a, b);
    }

    static inline uint32x4 max(uint32x4 a, uint32x4 b)
    {
        return __msa_max_u_w(a, b);
    }

    // -----------------------------------------------------------------
    // uint64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint64x2 set_component(uint64x2 a, u64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return (v2u64) __msa_insert_d((v2i64) a, Index, s);
    }

    template <unsigned int Index>
    static inline u64 get_component(uint64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return __msa_copy_u_d(a, Index);
    }

    static inline uint64x2 uint64x2_zero()
    {
        return (v2u64) __msa_fill_d(0);
    }

    static inline uint64x2 uint64x2_set1(u64 s)
    {
        return (v2u64) __msa_fill_d(s);
    }

    static inline uint64x2 uint64x2_set2(u64 x, u64 y)
    {
        return (v2u64) { x, y };
    }

    static inline uint64x2 unpacklo(uint64x2 a, uint64x2 b)
    {
        return (v2u64) __msa_ilvr_d((v2i64)b, (v2i64)a);
    }

    static inline uint64x2 unpackhi(uint64x2 a, uint64x2 b)
    {
        return (v2u64) __msa_ilvl_d((v2i64)b, (v2i64)a);
    }

    static inline uint64x2 add(uint64x2 a, uint64x2 b)
    {
        return (v2u64) __msa_addv_d((v2i64)a, (v2i64)b);
    }

    static inline uint64x2 sub(uint64x2 a, uint64x2 b)
    {
        return (v2u64) __msa_subv_d((v2i64)a, (v2i64)b);
    }

    static inline uint64x2 bitwise_nand(uint64x2 a, uint64x2 b)
    {
        return (v2u64) __msa_and_v((v16u8)a, __msa_nor_v((v16u8)b, (v16u8)b));
    }

    static inline uint64x2 bitwise_and(uint64x2 a, uint64x2 b)
    {
        return (v2u64) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline uint64x2 bitwise_or(uint64x2 a, uint64x2 b)
    {
        return (v2u64) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline uint64x2 bitwise_xor(uint64x2 a, uint64x2 b)
    {
        return (v2u64) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline uint64x2 bitwise_not(uint64x2 a)
    {
        return (v2u64) __msa_nor_v((v16u8) a, (v16u8) a);
    }

    static inline uint64x2 select(mask64x2 mask, uint64x2 a, uint64x2 b)
    {
        return (v2u64) __msa_bsel_v((v16u8) mask, (v16u8) a, (v16u8) b);
    }

    // shift by constant

    template <int Count>
    static inline uint64x2 slli(uint64x2 a)
    {
        return (v2u64) __msa_slli_d((v2i64)a, Count);
    }

    template <int Count>
    static inline uint64x2 srli(uint64x2 a)
    {
        return (v2u64) __msa_srli_d((v2i64)a, Count);
    }

    // shift by scalar

    static inline uint64x2 sll(uint64x2 a, int count)
    {
        return (v2u64) __msa_sll_d((v2u64) a, (v2i64) __msa_fill_d(count));
    }

    static inline uint64x2 srl(uint64x2 a, int count)
    {
        return (v2u64) __msa_srl_d((v2u64) a, (v2i64) __msa_fill_d(count));
    }

    // -----------------------------------------------------------------
    // int8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int8x16 set_component(int8x16 a, s8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        return __msa_insert_b(a, Index, s);
    }

    template <unsigned int Index>
    static inline s8 get_component(int8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return __msa_copy_s_b(a, Index);
    }

    static inline int8x16 int8x16_zero()
    {
        return __msa_fill_b(0);
    }

    static inline int8x16 int8x16_set1(s8 s)
    {
        return __msa_fill_b(s);
    }

    static inline int8x16 int8x16_set16(
        s8 v0, s8 v1, s8 v2, s8 v3, s8 v4, s8 v5, s8 v6, s8 v7,
        s8 v8, s8 v9, s8 v10, s8 v11, s8 v12, s8 v13, s8 v14, s8 v15)
    {
        return (v16i8) { v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15 };
    }

    static inline int8x16 int8x16_load_low(const s8* source)
    {
        return (v2i64) { uload64(source), 0 };
    }

    static inline void int8x16_store_low(s8* dest, int8x16 a)
    {
        std::memcpy(dest, &a, 8);
    }

    static inline int8x16 unpacklo(int8x16 a, int8x16 b)
    {
        return __msa_ilvr_b(b, a);
    }

    static inline int8x16 unpackhi(int8x16 a, int8x16 b)
    {
        return __msa_ilvl_b(b, a);
    }

    static inline int8x16 add(int8x16 a, int8x16 b)
    {
        return __msa_addv_b(a, b);
    }

    static inline int8x16 sub(int8x16 a, int8x16 b)
    {
        return __msa_subv_b(a, b);
    }

    // saturated

    static inline int8x16 adds(int8x16 a, int8x16 b)
    {
        return __msa_adds_s_b(a, b);
    }

    static inline int8x16 subs(int8x16 a, int8x16 b)
    {
        return __msa_subs_s_b(a, b);
    }

    static inline int8x16 abs(int8x16 a)
    {
        return __msa_add_a_b(a, (v16i8) __msa_xor_v((v16u8) a, (v16u8) a));
    }

    static inline int8x16 neg(int8x16 a)
    {
        const v16i8 zero = (v16i8) __msa_xor_v((v16u8) a, (v16u8) a);
        return __msa_subs_s_b(zero, a);
    }

    // bitwise

    static inline int8x16 bitwise_nand(int8x16 a, int8x16 b)
    {
        return (v16i8) __msa_and_v((v16u8)a, __msa_nor_v((v16u8)b, (v16u8)b));
    }

    static inline int8x16 bitwise_and(int8x16 a, int8x16 b)
    {
        return (v16i8) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline int8x16 bitwise_or(int8x16 a, int8x16 b)
    {
        return (v16i8) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline int8x16 bitwise_xor(int8x16 a, int8x16 b)
    {
        return (v16i8) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline int8x16 bitwise_not(int8x16 a)
    {
        return (v16i8) __msa_nor_v((v16u8) a, (v16u8) a);
    }

    // compare

    static inline mask8x16 compare_eq(int8x16 a, int8x16 b)
    {
        return (v16u8) __msa_ceq_b(a, b);
    }

    static inline mask8x16 compare_gt(int8x16 a, int8x16 b)
    {
        return (v16u8) __msa_clt_s_b(b, a);
    }

    static inline mask8x16 compare_neq(int8x16 a, int8x16 b)
    {
        auto mask = (v16u8) __msa_ceq_b(a, b);
        return (v16u8) __msa_nor_v(mask, mask);
    }

    static inline mask8x16 compare_lt(int8x16 a, int8x16 b)
    {
        return (v16u8) __msa_clt_s_b(a, b);
    }

    static inline mask8x16 compare_le(int8x16 a, int8x16 b)
    {
        return (v16u8) __msa_cle_s_b(a, b);
    }

    static inline mask8x16 compare_ge(int8x16 a, int8x16 b)
    {
        return (v16u8) __msa_cle_s_b(b, a);
    }

    static inline int8x16 select(mask8x16 mask, int8x16 a, int8x16 b)
    {
        return (v16i8) __msa_bsel_v((v16u8) mask, (v16u8) a, (v16u8) b);
    }

    static inline int8x16 min(int8x16 a, int8x16 b)
    {
        return __msa_min_s_b(a, b);
    }

    static inline int8x16 max(int8x16 a, int8x16 b)
    {
        return __msa_max_s_b(a, b);
    }

    // -----------------------------------------------------------------
    // int16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int16x8 set_component(int16x8 a, s16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        return __msa_insert_h(a, Index, s);
    }

    template <unsigned int Index>
    static inline s16 get_component(int16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return __msa_copy_s_h(a, Index);
    }

    static inline int16x8 int16x8_zero()
    {
        return __msa_fill_h(0);
    }

    static inline int16x8 int16x8_set1(s16 s)
    {
        return __msa_fill_h(s);
    }

    static inline int16x8 int16x8_set8(s16 s0, s16 s1, s16 s2, s16 s3, s16 s4, s16 s5, s16 s6, s16 s7)
    {
        return (v8i16) { s0, s1, s2, s3, s4, s5, s6, s7 };
    }

    static inline int16x8 int16x8_load_low(const s16* source)
    {
        return (v8i16) { source[0], source[1], source[2], source[3], 0, 0, 0, 0 };
    }

    static inline void int16x8_store_low(s16* dest, int16x8 a)
    {
        std::memcpy(dest, &a, 8);
    }

    static inline int16x8 unpacklo(int16x8 a, int16x8 b)
    {
        return __msa_ilvr_h(b, a);
    }

    static inline int16x8 unpackhi(int16x8 a, int16x8 b)
    {
        return __msa_ilvl_h(b, a);
    }

    static inline int16x8 add(int16x8 a, int16x8 b)
    {
        return __msa_addv_h(a, b);
    }

    static inline int16x8 sub(int16x8 a, int16x8 b)
    {
        return __msa_subv_h(a, b);
    }

    static inline int16x8 mullo(int16x8 a, int16x8 b)
    {
        return __msa_mulv_h(a, b);
    }

    // saturated

    static inline int16x8 adds(int16x8 a, int16x8 b)
    {
        return __msa_adds_s_h(a, b);
    }

    static inline int16x8 subs(int16x8 a, int16x8 b)
    {
        return __msa_subs_s_h(a, b);
    }

    static inline int16x8 abs(int16x8 a)
    {
        return __msa_add_a_h(a, (v8i16) __msa_xor_v((v16u8) a, (v16u8) a));
    }

    static inline int16x8 neg(int16x8 a)
    {
        const v8i16 zero = (v8i16) __msa_xor_v((v16u8) a, (v16u8) a);
        return __msa_subs_s_h(zero, a);
    }

    // bitwise

    static inline int16x8 bitwise_nand(int16x8 a, int16x8 b)
    {
        return (v8i16) __msa_and_v((v16u8)a, __msa_nor_v((v16u8)b, (v16u8)b));
    }

    static inline int16x8 bitwise_and(int16x8 a, int16x8 b)
    {
        return (v8i16) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline int16x8 bitwise_or(int16x8 a, int16x8 b)
    {
        return (v8i16) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline int16x8 bitwise_xor(int16x8 a, int16x8 b)
    {
        return (v8i16) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline int16x8 bitwise_not(int16x8 a)
    {
        return (v8i16) __msa_nor_v((v16u8) a, (v16u8) a);
    }

    // compare

    static inline mask16x8 compare_eq(int16x8 a, int16x8 b)
    {
        return (v8u16) __msa_ceq_h(a, b);
    }

    static inline mask16x8 compare_gt(int16x8 a, int16x8 b)
    {
        return (v8u16) __msa_clt_s_h(b, a);
    }

    static inline mask16x8 compare_neq(int16x8 a, int16x8 b)
    {
        auto mask = (v8u16) __msa_ceq_h(a, b);
        return (v8u16) __msa_nor_v(mask, mask);
    }

    static inline mask16x8 compare_lt(int16x8 a, int16x8 b)
    {
        return (v8u16) __msa_clt_s_h(a, b);
    }

    static inline mask16x8 compare_le(int16x8 a, int16x8 b)
    {
        return (v8u16) __msa_cle_s_h(a, b);
    }

    static inline mask16x8 compare_ge(int16x8 a, int16x8 b)
    {
        return (v8u16) __msa_cle_s_h(b, a);
    }

    static inline int16x8 select(mask16x8 mask, int16x8 a, int16x8 b)
    {
        return (v8i16) __msa_bsel_v((v16u8) mask, (v16u8) a, (v16u8) b);
    }

    // shift by constant

    template <int Count>
    static inline int16x8 slli(int16x8 a)
    {
        return __msa_slli_h(a, Count);
    }

    template <int Count>
    static inline int16x8 srli(int16x8 a)
    {
        return __msa_srli_h(a, Count);
    }

    template <int Count>
    static inline int16x8 srai(int16x8 a)
    {
        return __msa_srai_h(a, Count);
    }

    // shift by scalar

    static inline int16x8 sll(int16x8 a, int count)
    {
        return __msa_sll_h(a, __msa_fill_h(count));
    }

    static inline int16x8 srl(int16x8 a, int count)
    {
        return __msa_srl_h(a, __msa_fill_h(count));
    }

    static inline int16x8 sra(int16x8 a, int count)
    {
        return __msa_sra_h(a, __msa_fill_h(count));
    }

    static inline int16x8 min(int16x8 a, int16x8 b)
    {
        return __msa_min_s_h(a, b);
    }

    static inline int16x8 max(int16x8 a, int16x8 b)
    {
        return __msa_max_s_h(a, b);
    }

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    // shuffle

    template <u32 x, u32 y, u32 z, u32 w>
    static inline int32x4 shuffle(int32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        const v4i32 control = (v4i32) { x, y, z, w };
        return __msa_vshf_w(control, v, v);
    }

    template <>
    inline int32x4 shuffle<0, 1, 2, 3>(int32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline int32x4 set_component(int32x4 a, s32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        return __msa_insert_w(a, Index, s);
    }

    template <unsigned int Index>
    static inline s32 get_component(int32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return __msa_copy_s_w(a, Index);
    }

    static inline int32x4 int32x4_zero()
    {
        return __msa_fill_w(0);
    }

    static inline int32x4 int32x4_set1(s32 s)
    {
        return __msa_fill_w(s);
    }

    static inline int32x4 int32x4_set4(s32 x, s32 y, s32 z, s32 w)
    {
        return (v4i32) { x, y, z, w };
    }

    static inline int32x4 int32x4_uload(const s32* source)
    {
        return reinterpret_cast<const v4i32 *>(source)[0];
    }

    static inline void int32x4_ustore(s32* dest, int32x4 a)
    {
        reinterpret_cast<v4i32 *>(dest)[0] = a;
    }

    static inline int32x4 int32x4_load_low(const s32* source)
    {
        return (v4i32) { source[0], source[1], 0, 0 };
    }

    static inline void int32x4_store_low(s32* dest, int32x4 a)
    {
        std::memcpy(dest, &a, 8);
    }

    static inline int32x4 unpacklo(int32x4 a, int32x4 b)
    {
        return __msa_ilvr_w(b, a);
    }

    static inline int32x4 unpackhi(int32x4 a, int32x4 b)
    {
        return __msa_ilvl_w(b, a);
    }

    static inline int32x4 abs(int32x4 a)
    {
        return __msa_add_a_w(a, (v4i32) __msa_xor_v((v16u8) a, (v16u8) a));
    }

    static inline int32x4 neg(int32x4 a)
    {
        const v4i32 zero = (v4i32) __msa_xor_v((v16u8) a, (v16u8) a);
        return __msa_subs_s_w(zero, a);
    }

    static inline int32x4 add(int32x4 a, int32x4 b)
    {
        return __msa_addv_w(a, b);
    }

    static inline int32x4 sub(int32x4 a, int32x4 b)
    {
        return __msa_subv_w(a, b);
    }

    static inline int32x4 mullo(int32x4 a, int32x4 b)
    {
        return __msa_mulv_w(a, b);
    }

    // saturated

    static inline int32x4 adds(int32x4 a, int32x4 b)
    {
        return __msa_adds_s_w(a, b);
    }

    static inline int32x4 subs(int32x4 a, int32x4 b)
    {
        return __msa_subs_s_w(a, b);
    }

    // bitwise

    static inline int32x4 bitwise_nand(int32x4 a, int32x4 b)
    {
        return (v4i32) __msa_and_v((v16u8)a, __msa_nor_v((v16u8)b, (v16u8)b));
    }

    static inline int32x4 bitwise_and(int32x4 a, int32x4 b)
    {
        return (v4i32) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline int32x4 bitwise_or(int32x4 a, int32x4 b)
    {
        return (v4i32) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline int32x4 bitwise_xor(int32x4 a, int32x4 b)
    {
        return (v4i32) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline int32x4 bitwise_not(int32x4 a)
    {
        return (v4i32) __msa_nor_v((v16u8) a, (v16u8) a);
    }

    // compare

    static inline mask32x4 compare_eq(int32x4 a, int32x4 b)
    {
        return (v4u32) __msa_ceq_w(a, b);
    }

    static inline mask32x4 compare_gt(int32x4 a, int32x4 b)
    {
        return (v4u32) __msa_clt_s_w(b, a);
    }

    static inline mask32x4 compare_neq(int32x4 a, int32x4 b)
    {
        auto mask = (v4u32) __msa_ceq_w(a, b);
        return (v4u32) __msa_nor_v(mask, mask);
    }

    static inline mask32x4 compare_lt(int32x4 a, int32x4 b)
    {
        return (v4u32) __msa_clt_s_w(a, b);
    }

    static inline mask32x4 compare_le(int32x4 a, int32x4 b)
    {
        return (v4u32) __msa_cle_s_w(a, b);
    }

    static inline mask32x4 compare_ge(int32x4 a, int32x4 b)
    {
        return (v4u32) __msa_cle_s_w(b, a);
    }

    static inline int32x4 select(mask32x4 mask, int32x4 a, int32x4 b)
    {
        return (v4i32) __msa_bsel_v((v16u8) mask, (v16u8) a, (v16u8) b);
    }

    // shift by constant

    template <int Count>
    static inline int32x4 slli(int32x4 a)
    {
        return __msa_slli_w(a, Count);
    }

    template <int Count>
    static inline int32x4 srli(int32x4 a)
    {
        return __msa_srli_w(a, Count);
    }

    template <int Count>
    static inline int32x4 srai(int32x4 a)
    {
        return __msa_srai_w(a, Count);
    }

    // shift by scalar

    static inline int32x4 sll(int32x4 a, int count)
    {
        return __msa_sll_w(a, __msa_fill_w(count));
    }

    static inline int32x4 srl(int32x4 a, int count)
    {
        return __msa_srl_w(a, __msa_fill_w(count));
    }

    static inline int32x4 sra(int32x4 a, int count)
    {
        return __msa_sra_w(a, __msa_fill_w(count));
    }

    // shift by vector

    static inline int32x4 sll(int32x4 a, uint32x4 count)
    {
        return __msa_sll_w(a, (v4i32)count);
    }

    static inline int32x4 srl(int32x4 a, uint32x4 count)
    {
        return __msa_srl_w(a, (v4i32)count);
}

    static inline int32x4 sra(int32x4 a, uint32x4 count)
    {
        return __msa_sra_w(a, (v4i32)count);
    }

    static inline u32 pack(int32x4 s)
    {
        u32 x = __msa_copy_s_w(s, 0);
        u32 y = __msa_copy_s_w(s, 1);
        u32 z = __msa_copy_s_w(s, 2);
        u32 w = __msa_copy_s_w(s, 3);
        return (w << 24) | (z << 16) | (y << 8) | x;
    }

    static inline int32x4 min(int32x4 a, int32x4 b)
    {
        return __msa_min_s_w(a, b);
    }

    static inline int32x4 max(int32x4 a, int32x4 b)
    {
        return __msa_max_s_w(a, b);
    }

    static inline int32x4 unpack(u32 s)
    {
        s32 x = (s >> 0) & 0xff;
        s32 y = (s >> 8) & 0xff;
        s32 z = (s >> 16) & 0xff;
        s32 w = (s >> 24) & 0xff;
        return int32x4_set4(x, y, z, w);
    }

    // -----------------------------------------------------------------
    // int64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int64x2 set_component(int64x2 a, s64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        return __msa_insert_d(a, Index, s);
    }

    template <unsigned int Index>
    static inline s64 get_component(int64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return __msa_copy_s_d(a, Index);
    }

    static inline int64x2 int64x2_zero()
    {
        return __msa_fill_d(0);
    }

    static inline int64x2 int64x2_set1(s64 s)
    {
        return __msa_fill_d(s);
    }

    static inline int64x2 int64x2_set2(s64 x, s64 y)
    {
        return (v2i64) { x, y };
    }

    static inline int64x2 unpacklo(int64x2 a, int64x2 b)
    {
        return __msa_ilvr_d(b, a);
    }

    static inline int64x2 unpackhi(int64x2 a, int64x2 b)
    {
        return __msa_ilvl_d(b, a);
    }

    static inline int64x2 add(int64x2 a, int64x2 b)
    {
        return __msa_addv_d(a, b);
    }

    static inline int64x2 sub(int64x2 a, int64x2 b)
    {
        return __msa_subv_d(a, b);
    }

    static inline int64x2 bitwise_nand(int64x2 a, int64x2 b)
    {
        return (v2i64) __msa_and_v((v16u8)a, __msa_nor_v((v16u8)b, (v16u8)b));
    }

    static inline int64x2 bitwise_and(int64x2 a, int64x2 b)
    {
        return (v2i64) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline int64x2 bitwise_or(int64x2 a, int64x2 b)
    {
        return (v2i64) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline int64x2 bitwise_xor(int64x2 a, int64x2 b)
    {
        return (v2i64) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline int64x2 bitwise_not(int64x2 a)
    {
        return (v2i64) __msa_nor_v((v16u8) a, (v16u8) a);
    }

    static inline int64x2 select(mask64x2 mask, int64x2 a, int64x2 b)
    {
        return (v2i64) __msa_bsel_v((v16u8) mask, (v16u8) a, (v16u8) b);
    }

    // shift by constant

    template <int Count>
    static inline int64x2 slli(int64x2 a)
    {
        return __msa_slli_d(a, Count);
    }

    template <int Count>
    static inline int64x2 srli(int64x2 a)
    {
        return __msa_srli_d(a, Count);
    }

    // shift by scalar

    static inline int64x2 sll(int64x2 a, int count)
    {
        return __msa_sll_d(a, __msa_fill_d(count));
    }

    static inline int64x2 srl(int64x2 a, int count)
    {
        return __msa_srl_d(a, __msa_fill_d(count));
    }

    // -----------------------------------------------------------------
    // mask8x16
    // -----------------------------------------------------------------

    static inline mask8x16 operator & (mask8x16 a, mask8x16 b)
    {
        return (v16u8) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline mask8x16 operator | (mask8x16 a, mask8x16 b)
    {
        return (v16u8) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline mask8x16 operator ^ (mask8x16 a, mask8x16 b)
    {
        return (v16u8) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline u32 get_mask(mask8x16 a)
    {
        u32 x = __msa_copy_u_w(a, 0) & 0x01800180;
        u32 y = __msa_copy_u_w(a, 1) & 0x01800180;
        u32 z = __msa_copy_u_w(a, 2) & 0x01800180;
        u32 w = __msa_copy_u_w(a, 3) & 0x01800180;
        x = (x >> 21) | (x >> 7);
        y = (y >> 17) | (y >> 3);
        z = (z >> 13) | (z << 1);
        w = (w >>  9) | (w << 5);
        return w | z | y | x;
    }

    static inline bool none_of(mask8x16 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask8x16 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask8x16 a)
    {
        return get_mask(a) == 0xffff;
    }

    // -----------------------------------------------------------------
    // mask16x8
    // -----------------------------------------------------------------

    static inline mask16x8 operator & (mask16x8 a, mask16x8 b)
    {
        return (v8u16) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline mask16x8 operator | (mask16x8 a, mask16x8 b)
    {
        return (v8u16) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline mask16x8 operator ^ (mask16x8 a, mask16x8 b)
    {
        return (v8u16) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline u32 get_mask(mask16x8 a)
    {
        u32 x = __msa_copy_u_w(a, 0) & 0x00018000;
        u32 y = __msa_copy_u_w(a, 1) & 0x00018000;
        u32 z = __msa_copy_u_w(a, 2) & 0x00018000;
        u32 w = __msa_copy_u_w(a, 3) & 0x00018000;
        return (w >> 9) | (z >> 11) | (y >> 13) | (x >> 15);
    }

    static inline bool none_of(mask16x8 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask16x8 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask16x8 a)
    {
        return get_mask(a) == 0xff;
    }

    // -----------------------------------------------------------------
    // mask32x4
    // -----------------------------------------------------------------

    static inline mask32x4 operator & (mask32x4 a, mask32x4 b)
    {
        return (v4u32) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline mask32x4 operator | (mask32x4 a, mask32x4 b)
    {
        return (v4u32) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline mask32x4 operator ^ (mask32x4 a, mask32x4 b)
    {
        return (v4u32) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline u32 get_mask(mask32x4 a)
    {
        u32 x = __msa_copy_u_w(a, 0) & 1;
        u32 y = __msa_copy_u_w(a, 1) & 2;
        u32 z = __msa_copy_u_w(a, 2) & 4;
        u32 w = __msa_copy_u_w(a, 3) & 8;
        return w | z | y | x;
    }

    static inline bool none_of(mask32x4 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask32x4 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask32x4 a)
    {
        return get_mask(a) == 0xf;
    }

    // -----------------------------------------------------------------
    // mask64x2
    // -----------------------------------------------------------------

    static inline mask64x2 operator & (mask64x2 a, mask64x2 b)
    {
        return (v2u64) __msa_and_v((v16u8) a, (v16u8) b);
    }

    static inline mask64x2 operator | (mask64x2 a, mask64x2 b)
    {
        return (v2u64) __msa_or_v((v16u8) a, (v16u8) b);
    }

    static inline mask64x2 operator ^ (mask64x2 a, mask64x2 b)
    {
        return (v2u64) __msa_xor_v((v16u8) a, (v16u8) b);
    }

    static inline u32 get_mask(mask64x2 a)
    {
        u32 x = __msa_copy_u_d(a, 0) & 1;
        u32 y = __msa_copy_u_d(a, 1) & 2;
        return y | x;
    }

    static inline bool none_of(mask64x2 a)
    {
        return get_mask(a) == 0;
    }

    static inline bool any_of(mask64x2 a)
    {
        return get_mask(a) != 0;
    }

    static inline bool all_of(mask64x2 a)
    {
        return get_mask(a) == 0x3;
    }

} // namespace simd
} // namespace mango
