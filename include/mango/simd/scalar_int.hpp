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

    // set

    static inline uint32x4 uint32x4_zero()
    {
        uint32x4 temp = { 0, 0, 0, 0 };
        return temp;
    }

    static inline uint32x4 uint32x4_set1(uint32 s)
    {
        uint32x4 temp = { s, s, s, s };
        return temp;
    }

    static inline uint32x4 uint32x4_set4(uint32 x, uint32 y, uint32 z, uint32 w)
    {
        uint32x4 temp = { x, y, z, w };
        return temp;
    }

    static inline uint32x4 uint32x4_set_x(uint32x4 a, int x)
    {
        a.x = x;
        return a;
    }

    static inline uint32x4 uint32x4_set_y(uint32x4 a, int y)
    {
        a.y = y;
        return a;
    }

    static inline uint32x4 uint32x4_set_z(uint32x4 a, int z)
    {
        a.z = z;
        return a;
    }

    static inline uint32x4 uint32x4_set_w(uint32x4 a, int w)
    {
        a.w = w;
        return a;
    }

    // get

    static inline int uint32x4_get_x(uint32x4 a)
    {
        return a.x;
    }

    static inline int uint32x4_get_y(uint32x4 a)
    {
        return a.y;
    }

    static inline int uint32x4_get_z(uint32x4 a)
    {
        return a.z;
    }

    static inline int uint32x4_get_w(uint32x4 a)
    {
        return a.w;
    }

    static inline uint32x4 uint32x4_uload(const uint32* source)
    {
        uint32x4 temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline void uint32x4_ustore(uint32* dest, uint32x4 a)
    {
        dest[0] = a.x;
        dest[1] = a.y;
        dest[2] = a.z;
        dest[3] = a.w;
    }

    // logical

    static inline uint32x4 uint32x4_and(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = a.x & b.x;
        v.y = a.y & b.y;
        v.z = a.z & b.z;
        v.w = a.w & b.w;
        return v;
    }

    static inline uint32x4 uint32x4_nand(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = ~a.x & b.x;
        v.y = ~a.y & b.y;
        v.z = ~a.z & b.z;
        v.w = ~a.w & b.w;
        return v;
    }

    static inline uint32x4 uint32x4_or(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = a.x | b.x;
        v.y = a.y | b.y;
        v.z = a.z | b.z;
        v.w = a.w | b.w;
        return v;
    }

    static inline uint32x4 uint32x4_xor(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = a.x ^ b.x;
        v.y = a.y ^ b.y;
        v.z = a.z ^ b.z;
        v.w = a.w ^ b.w;
        return v;
    }

    static inline uint32x4 uint32x4_compare_eq(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = a.x == b.x ? 0xffffffff : 0;
        v.y = a.y == b.y ? 0xffffffff : 0;
        v.z = a.z == b.z ? 0xffffffff : 0;
        v.w = a.w == b.w ? 0xffffffff : 0;
        return v;
    }

    static inline uint32x4 uint32x4_compare_gt(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = a.x > b.x ? 0xffffffff : 0;
        v.y = a.y > b.y ? 0xffffffff : 0;
        v.z = a.z > b.z ? 0xffffffff : 0;
        v.w = a.w > b.w ? 0xffffffff : 0;
        return v;
    }

    static inline uint32x4 uint32x4_select(uint32x4 mask, uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = (mask.x & a.x) | (~mask.x & b.x);
        v.y = (mask.y & a.y) | (~mask.y & b.y);
        v.z = (mask.z & a.z) | (~mask.z & b.z);
        v.w = (mask.w & a.w) | (~mask.w & b.w);
        return v;
    }

    static inline uint32x4 uint32x4_min(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = std::min(a.x, b.x);
        v.y = std::min(a.y, b.y);
        v.z = std::min(a.z, b.z);
        v.w = std::min(a.w, b.w);
        return v;
    }

    static inline uint32x4 uint32x4_max(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = std::max(a.x, b.x);
        v.y = std::max(a.y, b.y);
        v.z = std::max(a.z, b.z);
        v.w = std::max(a.w, b.w);
        return v;
    }

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    // set

    static inline int32x4 int32x4_zero()
    {
        int32x4 temp = { 0, 0, 0, 0 };
        return temp;
    }

    static inline int32x4 int32x4_set1(int s)
    {
        int32x4 temp = { s, s, s, s };
        return temp;
    }

    static inline int32x4 int32x4_set4(int x, int y, int z, int w)
    {
        int32x4 temp = { x, y, z, w };
        return temp;
    }

    static inline int32x4 int32x4_set_x(int32x4 a, int x)
    {
        a.x = x;
        return a;
    }

    static inline int32x4 int32x4_set_y(int32x4 a, int y)
    {
        a.y = y;
        return a;
    }

    static inline int32x4 int32x4_set_z(int32x4 a, int z)
    {
        a.z = z;
        return a;
    }

    static inline int32x4 int32x4_set_w(int32x4 a, int w)
    {
        a.w = w;
        return a;
    }

    // get

    static inline int int32x4_get_x(int32x4 a)
    {
        return a.x;
    }

    static inline int int32x4_get_y(int32x4 a)
    {
        return a.y;
    }

    static inline int int32x4_get_z(int32x4 a)
    {
        return a.z;
    }

    static inline int int32x4_get_w(int32x4 a)
    {
        return a.w;
    }

    static inline int32x4 int32x4_uload(const int* source)
    {
        int32x4 temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline void int32x4_ustore(int* dest, int32x4 a)
    {
        dest[0] = a.x;
        dest[1] = a.y;
        dest[2] = a.z;
        dest[3] = a.w;
    }

    static inline int32x4 int32x4_neg(int32x4 a)
    {
        int32x4 v = { -a.x, -a.y, -a.z, -a.w };
        return v;
    }

    static inline int32x4 int32x4_add(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x + b.x;
        v.y = a.y + b.y;
        v.z = a.z + b.z;
        v.w = a.w + b.w;
        return v;
    }

    static inline int32x4 int32x4_sub(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x - b.x;
        v.y = a.y - b.y;
        v.z = a.z - b.z;
        v.w = a.w - b.w;
        return v;
    }

    // logical

    static inline int32x4 int32x4_and(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x & b.x;
        v.y = a.y & b.y;
        v.z = a.z & b.z;
        v.w = a.w & b.w;
        return v;
    }

    static inline int32x4 int32x4_nand(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = ~a.x & b.x;
        v.y = ~a.y & b.y;
        v.z = ~a.z & b.z;
        v.w = ~a.w & b.w;
        return v;
    }

    static inline int32x4 int32x4_or(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x | b.x;
        v.y = a.y | b.y;
        v.z = a.z | b.z;
        v.w = a.w | b.w;
        return v;
    }

    static inline int32x4 int32x4_xor(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x ^ b.x;
        v.y = a.y ^ b.y;
        v.z = a.z ^ b.z;
        v.w = a.w ^ b.w;
        return v;
    }

    // shift

    static inline int32x4 int32x4_sll(int32x4 a, int b)
    {
        int32x4 v;
        v.x = static_cast<uint32>(a.x) << b;
        v.y = static_cast<uint32>(a.y) << b;
        v.z = static_cast<uint32>(a.z) << b;
        v.w = static_cast<uint32>(a.w) << b;
        return v;
    }

    static inline int32x4 int32x4_srl(int32x4 a, int b)
    {
        int32x4 v;
        v.x = static_cast<uint32>(a.x) >> b;
        v.y = static_cast<uint32>(a.y) >> b;
        v.z = static_cast<uint32>(a.z) >> b;
        v.w = static_cast<uint32>(a.w) >> b;
        return v;
    }

    static inline int32x4 int32x4_sra(int32x4 a, int b)
    {
        int32x4 v;
        v.x = a.x >> b;
        v.y = a.y >> b;
        v.z = a.z >> b;
        v.w = a.w >> b;
        return v;
    }

    // compare

    static inline int32x4 int32x4_compare_eq(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x == b.x ? 0xffffffff : 0;
        v.y = a.y == b.y ? 0xffffffff : 0;
        v.z = a.z == b.z ? 0xffffffff : 0;
        v.w = a.w == b.w ? 0xffffffff : 0;
        return v;
    }

    static inline int32x4 int32x4_compare_gt(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x > b.x ? 0xffffffff : 0;
        v.y = a.y > b.y ? 0xffffffff : 0;
        v.z = a.z > b.z ? 0xffffffff : 0;
        v.w = a.w > b.w ? 0xffffffff : 0;
        return v;
    }

    static inline int32x4 int32x4_select(int32x4 mask, int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = (mask.x & a.x) | (~mask.x & b.x);
        v.y = (mask.y & a.y) | (~mask.y & b.y);
        v.z = (mask.z & a.z) | (~mask.z & b.z);
        v.w = (mask.w & a.w) | (~mask.w & b.w);
        return v;
    }

    static inline uint32 int32x4_get_mask(int32x4 a)
    {
        const uint32 x = a.x & 0x80000000;
        const uint32 y = a.y & 0x80000000;
        const uint32 z = a.z & 0x80000000;
        const uint32 w = a.w & 0x80000000;
        const uint32 mask = (x >> 31) | (y >> 30) | (z >> 29) | (w >> 28);
        return mask;
    }

    static inline int32x4 int32x4_min(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = std::min(a.x, b.x);
        v.y = std::min(a.y, b.y);
        v.z = std::min(a.z, b.z);
        v.w = std::min(a.w, b.w);
        return v;
    }

    static inline int32x4 int32x4_max(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = std::max(a.x, b.x);
        v.y = std::max(a.y, b.y);
        v.z = std::max(a.z, b.z);
        v.w = std::max(a.w, b.w);
        return v;
    }

    static inline uint32 int32x4_pack(int32x4 s)
    {
        const uint32 x = byteclamp(s.x);
        const uint32 y = byteclamp(s.y);
        const uint32 z = byteclamp(s.z);
        const uint32 w = byteclamp(s.w);
        return x | (y << 8) | (z << 16) | (w << 24);
    }

    static inline int32x4 int32x4_unpack(uint32 s)
    {
        int32x4 v;
        v.x = (s >> 0) & 0xff;
        v.y = (s >> 8) & 0xff;
        v.z = (s >> 16) & 0xff;
        v.w = (s >> 24);
        return v;
    }

} // namespace simd
} // namespace mango

#endif // MANGO_SIMD_INT_SCALAR
