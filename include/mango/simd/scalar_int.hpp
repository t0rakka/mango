/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

#ifdef MANGO_SIMD_INT_SCALAR

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // uint32x4
    // -----------------------------------------------------------------

    // shuffle

    template <int x, int y, int z, int w>
    static inline uint32x4 uint32x4_shuffle(uint32x4 v)
    {
        // .generic
        return uint32x4(v[x], v[y], v[z], v[w]);
    }

    template <>
    inline uint32x4 uint32x4_shuffle<0, 1, 2, 3>(uint32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <int Index>
    static inline uint32x4 uint32x4_set_component(uint32x4 a, uint32 s)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <int Index>
    static inline uint32 uint32x4_get_component(uint32x4 a)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        return a[Index];
    }

    static inline uint32x4 uint32x4_zero()
    {
        return uint32x4(0, 0, 0, 0);
    }

    static inline uint32x4 uint32x4_set1(uint32 s)
    {
        return uint32x4(s, s, s, s);
    }

    static inline uint32x4 uint32x4_set4(uint32 x, uint32 y, uint32 z, uint32 w)
    {
        return uint32x4(x, y, z, w);
    }

    static inline uint32x4 uint32x4_uload(const uint32* source)
    {
        return uint32x4(source[0], source[1], source[2], source[3]);
    }

    static inline void uint32x4_ustore(uint32* dest, uint32x4 a)
    {
        dest[0] = a[0];
        dest[1] = a[1];
        dest[2] = a[2];
        dest[3] = a[3];
    }

    static inline uint32x4 uint32x4_add(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v[0] = a[0] + b[0];
        v[1] = a[1] + b[1];
        v[2] = a[2] + b[2];
        v[3] = a[3] + b[3];
        return v;
    }

    static inline uint32x4 uint32x4_sub(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v[0] = a[0] - b[0];
        v[1] = a[1] - b[1];
        v[2] = a[2] - b[2];
        v[3] = a[3] - b[3];
        return v;
    }

    // logical

    static inline uint32x4 uint32x4_and(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v[0] = a[0] & b[0];
        v[1] = a[1] & b[1];
        v[2] = a[2] & b[2];
        v[3] = a[3] & b[3];
        return v;
    }

    static inline uint32x4 uint32x4_nand(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v[0] = ~a[0] & b[0];
        v[1] = ~a[1] & b[1];
        v[2] = ~a[2] & b[2];
        v[3] = ~a[3] & b[3];
        return v;
    }

    static inline uint32x4 uint32x4_or(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v[0] = a[0] | b[0];
        v[1] = a[1] | b[1];
        v[2] = a[2] | b[2];
        v[3] = a[3] | b[3];
        return v;
    }

    static inline uint32x4 uint32x4_xor(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v[0] = a[0] ^ b[0];
        v[1] = a[1] ^ b[1];
        v[2] = a[2] ^ b[2];
        v[3] = a[3] ^ b[3];
        return v;
    }

    // shift

    template <int Count> 
    static inline uint32x4 uint32x4_sll(uint32x4 a)
    {
        uint32x4 v;
        v[0] = a[0] << Count;
        v[1] = a[1] << Count;
        v[2] = a[2] << Count;
        v[3] = a[3] << Count;
        return v;
    }

    template <int Count> 
    static inline uint32x4 uint32x4_srl(uint32x4 a)
    {
        uint32x4 v;
        v[0] = a[0] >> Count;
        v[1] = a[1] >> Count;
        v[2] = a[2] >> Count;
        v[3] = a[3] >> Count;
        return v;
    }

    template <int Count> 
    static inline uint32x4 uint32x4_sra(uint32x4 a)
    {
        uint32x4 v;
        v[0] = static_cast<int32>(a[0]) >> Count;
        v[1] = static_cast<int32>(a[1]) >> Count;
        v[2] = static_cast<int32>(a[2]) >> Count;
        v[3] = static_cast<int32>(a[3]) >> Count;
        return v;
    }

    // compare

    static inline uint32x4 uint32x4_compare_eq(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v[0] = a[0] == b[0] ? 0xffffffff : 0;
        v[1] = a[1] == b[1] ? 0xffffffff : 0;
        v[2] = a[2] == b[2] ? 0xffffffff : 0;
        v[3] = a[3] == b[3] ? 0xffffffff : 0;
        return v;
    }

    static inline uint32x4 uint32x4_compare_gt(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v[0] = a[0] > b[0] ? 0xffffffff : 0;
        v[1] = a[1] > b[1] ? 0xffffffff : 0;
        v[2] = a[2] > b[2] ? 0xffffffff : 0;
        v[3] = a[3] > b[3] ? 0xffffffff : 0;
        return v;
    }

    static inline uint32x4 uint32x4_select(uint32x4 mask, uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v[0] = (mask[0] & a[0]) | (~mask[0] & b[0]);
        v[1] = (mask[1] & a[1]) | (~mask[1] & b[1]);
        v[2] = (mask[2] & a[2]) | (~mask[2] & b[2]);
        v[3] = (mask[3] & a[3]) | (~mask[3] & b[3]);
        return v;
    }

    static inline uint32x4 uint32x4_min(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v[0] = std::min(a[0], b[0]);
        v[1] = std::min(a[1], b[1]);
        v[2] = std::min(a[2], b[2]);
        v[3] = std::min(a[3], b[3]);
        return v;
    }

    static inline uint32x4 uint32x4_max(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v[0] = std::max(a[0], b[0]);
        v[1] = std::max(a[1], b[1]);
        v[2] = std::max(a[2], b[2]);
        v[3] = std::max(a[3], b[3]);
        return v;
    }

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    // shuffle

    template <int x, int y, int z, int w>
    static inline int32x4 int32x4_shuffle(int32x4 v)
    {
        // .generic
        return int32x4(v[x], v[y], v[z], v[w]);
    }

    template <>
    inline int32x4 int32x4_shuffle<0, 1, 2, 3>(int32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <int Index>
    static inline int32x4 int32x4_set_component(int32x4 a, int32 s)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <int Index>
    static inline int32 int32x4_get_component(int32x4 a)
    {
        static_assert(Index >= 0 && Index < 4, "Index out of range.");
        return a[Index];
    }

    static inline int32x4 int32x4_zero()
    {
        return int32x4(0, 0, 0, 0);
    }

    static inline int32x4 int32x4_set1(int s)
    {
        return int32x4(s, s, s, s);
    }

    static inline int32x4 int32x4_set4(int x, int y, int z, int w)
    {
        return int32x4(x, y, z, w);
    }

    static inline int32x4 int32x4_uload(const int* source)
    {
        return int32x4(source[0], source[1], source[2], source[3]);
    }

    static inline void int32x4_ustore(int* dest, int32x4 a)
    {
        dest[0] = a[0];
        dest[1] = a[1];
        dest[2] = a[2];
        dest[3] = a[3];
    }

    static inline int32x4 int32x4_abs(int32x4 a)
    {
        int32x4 v;
        v[0] = std::abs(a[0]);
        v[1] = std::abs(a[1]);
        v[2] = std::abs(a[2]);
        v[3] = std::abs(a[3]);
        return v;
    }

    static inline int32x4 int32x4_neg(int32x4 a)
    {
        return int32x4(-a[0], -a[1], -a[2], -a[3]);
    }

    static inline int32x4 int32x4_add(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v[0] = a[0] + b[0];
        v[1] = a[1] + b[1];
        v[2] = a[2] + b[2];
        v[3] = a[3] + b[3];
        return v;
    }

    static inline int32x4 int32x4_sub(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v[0] = a[0] - b[0];
        v[1] = a[1] - b[1];
        v[2] = a[2] - b[2];
        v[3] = a[3] - b[3];
        return v;
    }

    // logical

    static inline int32x4 int32x4_and(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v[0] = a[0] & b[0];
        v[1] = a[1] & b[1];
        v[2] = a[2] & b[2];
        v[3] = a[3] & b[3];
        return v;
    }

    static inline int32x4 int32x4_nand(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v[0] = ~a[0] & b[0];
        v[1] = ~a[1] & b[1];
        v[2] = ~a[2] & b[2];
        v[3] = ~a[3] & b[3];
        return v;
    }

    static inline int32x4 int32x4_or(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v[0] = a[0] | b[0];
        v[1] = a[1] | b[1];
        v[2] = a[2] | b[2];
        v[3] = a[3] | b[3];
        return v;
    }

    static inline int32x4 int32x4_xor(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v[0] = a[0] ^ b[0];
        v[1] = a[1] ^ b[1];
        v[2] = a[2] ^ b[2];
        v[3] = a[3] ^ b[3];
        return v;
    }

    // shift

    template <int Count> 
    static inline int32x4 int32x4_sll(int32x4 a)
    {
        int32x4 v;
        v[0] = static_cast<uint32>(a[0]) << Count;
        v[1] = static_cast<uint32>(a[1]) << Count;
        v[2] = static_cast<uint32>(a[2]) << Count;
        v[3] = static_cast<uint32>(a[3]) << Count;
        return v;
    }

    template <int Count> 
    static inline int32x4 int32x4_srl(int32x4 a)
    {
        int32x4 v;
        v[0] = static_cast<uint32>(a[0]) >> Count;
        v[1] = static_cast<uint32>(a[1]) >> Count;
        v[2] = static_cast<uint32>(a[2]) >> Count;
        v[3] = static_cast<uint32>(a[3]) >> Count;
        return v;
    }

    template <int Count> 
    static inline int32x4 int32x4_sra(int32x4 a)
    {
        int32x4 v;
        v[0] = a[0] >> Count;
        v[1] = a[1] >> Count;
        v[2] = a[2] >> Count;
        v[3] = a[3] >> Count;
        return v;
    }

    // compare

    static inline int32x4 int32x4_compare_eq(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v[0] = a[0] == b[0] ? 0xffffffff : 0;
        v[1] = a[1] == b[1] ? 0xffffffff : 0;
        v[2] = a[2] == b[2] ? 0xffffffff : 0;
        v[3] = a[3] == b[3] ? 0xffffffff : 0;
        return v;
    }

    static inline int32x4 int32x4_compare_gt(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v[0] = a[0] > b[0] ? 0xffffffff : 0;
        v[1] = a[1] > b[1] ? 0xffffffff : 0;
        v[2] = a[2] > b[2] ? 0xffffffff : 0;
        v[3] = a[3] > b[3] ? 0xffffffff : 0;
        return v;
    }

    static inline int32x4 int32x4_select(int32x4 mask, int32x4 a, int32x4 b)
    {
        int32x4 v;
        v[0] = (mask[0] & a[0]) | (~mask[0] & b[0]);
        v[1] = (mask[1] & a[1]) | (~mask[1] & b[1]);
        v[2] = (mask[2] & a[2]) | (~mask[2] & b[2]);
        v[3] = (mask[3] & a[3]) | (~mask[3] & b[3]);
        return v;
    }

    static inline uint32 int32x4_get_mask(int32x4 a)
    {
        const uint32 x = a[0] & 0x80000000;
        const uint32 y = a[1] & 0x80000000;
        const uint32 z = a[2] & 0x80000000;
        const uint32 w = a[3] & 0x80000000;
        const uint32 mask = (x >> 31) | (y >> 30) | (z >> 29) | (w >> 28);
        return mask;
    }

    static inline int32x4 int32x4_min(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v[0] = std::min(a[0], b[0]);
        v[1] = std::min(a[1], b[1]);
        v[2] = std::min(a[2], b[2]);
        v[3] = std::min(a[3], b[3]);
        return v;
    }

    static inline int32x4 int32x4_max(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v[0] = std::max(a[0], b[0]);
        v[1] = std::max(a[1], b[1]);
        v[2] = std::max(a[2], b[2]);
        v[3] = std::max(a[3], b[3]);
        return v;
    }

    static inline uint32 int32x4_pack(int32x4 s)
    {
        const uint32 x = byteclamp(s[0]);
        const uint32 y = byteclamp(s[1]);
        const uint32 z = byteclamp(s[2]);
        const uint32 w = byteclamp(s[3]);
        return x | (y << 8) | (z << 16) | (w << 24);
    }

    static inline int32x4 int32x4_unpack(uint32 s)
    {
        int32x4 v;
        v[0] = (s >> 0) & 0xff;
        v[1] = (s >> 8) & 0xff;
        v[2] = (s >> 16) & 0xff;
        v[3] = (s >> 24);
        return v;
    }

} // namespace simd
} // namespace mango

#endif // MANGO_SIMD_INT_SCALAR
