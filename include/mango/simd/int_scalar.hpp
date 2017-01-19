/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_SCALAR_INT
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    static inline int32x4 int32x4_reinterpret(float32x4 s)
    {
        return reinterpret_cast<int32x4 &>(s);
    }

    static inline int32x4 int32x4_convert(float32x4 s)
    {
        int32x4 v;
        v.x = int(s.x + 0.5f);
        v.y = int(s.y + 0.5f);
        v.z = int(s.z + 0.5f);
        v.w = int(s.w + 0.5f);
        return v;
    }

    static inline int32x4 int32x4_truncate(float32x4 s)
    {
        int32x4 v;
        v.x = int(s.x);
        v.y = int(s.y);
        v.z = int(s.z);
        v.w = int(s.w);
        return v;
    }

    static inline int32x4 int32x4_set_x(int32x4 a, int x)
    {
        int32x4 temp = a;
        temp.x = x;
        return temp;
    }

    static inline int32x4 int32x4_set_y(int32x4 a, int y)
    {
        int32x4 temp = a;
        temp.y = y;
        return temp;
    }

    static inline int32x4 int32x4_set_z(int32x4 a, int z)
    {
        int32x4 temp = a;
        temp.z = z;
        return temp;
    }

    static inline int32x4 int32x4_set_w(int32x4 a, int w)
    {
        int32x4 temp = a;
        temp.w = w;
        return temp;
    }

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

    static inline int32x4 int32x4_load(const int* source)
    {
        int32x4 temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline int32x4 int32x4_uload(const int* source)
    {
        return int32x4_load(source);
    }

    static inline void int32x4_store(int* dest, int32x4 a)
    {
        dest[0] = a.x;
        dest[1] = a.y;
        dest[2] = a.z;
        dest[3] = a.w;
    }

    static inline void int32x4_ustore(int* dest, int32x4 a)
    {
        int32x4_store(dest, a);
    }

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
        return int32x4_or(int32x4_and(mask, a), int32x4_nand(mask, b));
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
