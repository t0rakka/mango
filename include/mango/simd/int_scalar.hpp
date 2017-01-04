/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_SCALAR_INT
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

    // -----------------------------------------------------------------
    // simd4i
    // -----------------------------------------------------------------

    static inline simd4i simd4i_cast(__simd4f s)
    {
        return reinterpret_cast<__simd4i>(s);
    }

    static inline simd4i simd4i_convert(__simd4f s)
    {
        simd4i v;
        v.x = int(s.x + 0.5f);
        v.y = int(s.y + 0.5f);
        v.z = int(s.z + 0.5f);
        v.w = int(s.w + 0.5f);
        return v;
    }

    static inline simd4i simd4i_truncate(__simd4f s)
    {
        simd4i v;
        v.x = int(s.x);
        v.y = int(s.y);
        v.z = int(s.z);
        v.w = int(s.w);
        return v;
    }

    static inline simd4i simd4i_set_x(__simd4i a, int x)
    {
        simd4i temp = a;
        temp.x = x;
        return temp;
    }

    static inline simd4i simd4i_set_y(__simd4i a, int y)
    {
        simd4i temp = a;
        temp.y = y;
        return temp;
    }

    static inline simd4i simd4i_set_z(__simd4i a, int z)
    {
        simd4i temp = a;
        temp.z = z;
        return temp;
    }

    static inline simd4i simd4i_set_w(__simd4i a, int w)
    {
        simd4i temp = a;
        temp.w = w;
        return temp;
    }

    static inline int simd4i_get_x(__simd4i a)
    {
        return a.x;
    }

    static inline int simd4i_get_y(__simd4i a)
    {
        return a.y;
    }

    static inline int simd4i_get_z(__simd4i a)
    {
        return a.z;
    }

    static inline int simd4i_get_w(__simd4i a)
    {
        return a.w;
    }

    static inline simd4i simd4i_load(const int* source)
    {
        simd4i temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline simd4i simd4i_uload(const int* source)
    {
        return simd4i_load(source);
    }

    static inline void simd4i_store(int* dest, __simd4i a)
    {
        dest[0] = a.x;
        dest[1] = a.y;
        dest[2] = a.z;
        dest[3] = a.w;
    }

    static inline void simd4i_ustore(int* dest, __simd4i a)
    {
        simd4i_store(dest, a);
    }

    static inline simd4i simd4i_zero()
    {
        simd4i temp = { 0, 0, 0, 0 };
        return temp;
    }

    static inline simd4i simd4i_set1(int s)
    {
        simd4i temp = { s, s, s, s };
        return temp;
    }

    static inline simd4i simd4i_set4(int x, int y, int z, int w)
    {
        simd4i temp = { x, y, z, w };
        return temp;
    }

    static inline simd4i simd4i_neg(__simd4i a)
    {
        simd4i v = { -a.x, -a.y, -a.z, -a.w };
        return v;
    }

    static inline simd4i simd4i_add(__simd4i a, __simd4i b)
    {
        simd4i v;
        v.x = a.x + b.x;
        v.y = a.y + b.y;
        v.z = a.z + b.z;
        v.w = a.w + b.w;
        return v;
    }

    static inline simd4i simd4i_sub(__simd4i a, __simd4i b)
    {
        simd4i v;
        v.x = a.x - b.x;
        v.y = a.y - b.y;
        v.z = a.z - b.z;
        v.w = a.w - b.w;
        return v;
    }

    // logical

    static inline simd4i simd4i_and(__simd4i a, __simd4i b)
    {
        simd4i v;
        v.x = a.x & b.x;
        v.y = a.y & b.y;
        v.z = a.z & b.z;
        v.w = a.w & b.w;
        return v;
    }

    static inline simd4i simd4i_nand(__simd4i a, __simd4i b)
    {
        simd4i v;
        v.x = ~a.x & b.x;
        v.y = ~a.y & b.y;
        v.z = ~a.z & b.z;
        v.w = ~a.w & b.w;
        return v;
    }

    static inline simd4i simd4i_or(__simd4i a, __simd4i b)
    {
        simd4i v;
        v.x = a.x | b.x;
        v.y = a.y | b.y;
        v.z = a.z | b.z;
        v.w = a.w | b.w;
        return v;
    }

    static inline simd4i simd4i_xor(__simd4i a, __simd4i b)
    {
        simd4i v;
        v.x = a.x ^ b.x;
        v.y = a.y ^ b.y;
        v.z = a.z ^ b.z;
        v.w = a.w ^ b.w;
        return v;
    }

    // shift

    static inline simd4i simd4i_sll(__simd4i a, int b)
    {
        simd4i v;
        v.x = static_cast<uint32>(a.x) << b;
        v.y = static_cast<uint32>(a.y) << b;
        v.z = static_cast<uint32>(a.z) << b;
        v.w = static_cast<uint32>(a.w) << b;
        return v;
    }

    static inline simd4i simd4i_srl(__simd4i a, int b)
    {
        simd4i v;
        v.x = static_cast<uint32>(a.x) >> b;
        v.y = static_cast<uint32>(a.y) >> b;
        v.z = static_cast<uint32>(a.z) >> b;
        v.w = static_cast<uint32>(a.w) >> b;
        return v;
    }

    static inline simd4i simd4i_sra(__simd4i a, int b)
    {
        simd4i v;
        v.x = a.x >> b;
        v.y = a.y >> b;
        v.z = a.z >> b;
        v.w = a.w >> b;
        return v;
    }

    // compare

    static inline simd4i simd4i_compare_eq(__simd4i a, __simd4i b)
    {
        simd4i v;
        v.x = a.x == b.x ? 0xffffffff : 0;
        v.y = a.y == b.y ? 0xffffffff : 0;
        v.z = a.z == b.z ? 0xffffffff : 0;
        v.w = a.w == b.w ? 0xffffffff : 0;
        return v;
    }

    static inline simd4i simd4i_compare_gt(__simd4i a, __simd4i b)
    {
        simd4i v;
        v.x = a.x > b.x ? 0xffffffff : 0;
        v.y = a.y > b.y ? 0xffffffff : 0;
        v.z = a.z > b.z ? 0xffffffff : 0;
        v.w = a.w > b.w ? 0xffffffff : 0;
        return v;
    }

    static inline simd4i simd4i_select(__simd4i mask, __simd4i a, __simd4i b)
    {
        return simd4i_or(simd4i_and(mask, a), simd4i_nand(mask, b));
    }

    static inline uint32 simd4i_get_mask(__simd4i a)
    {
        const uint32 x = a.x & 0x80000000;
        const uint32 y = a.y & 0x80000000;
        const uint32 z = a.z & 0x80000000;
        const uint32 w = a.w & 0x80000000;
        const uint32 mask = (x >> 31) | (y >> 30) | (z >> 29) | (w >> 28);
        return mask;
    }

    static inline uint32 simd4i_pack(__simd4i s)
    {
        const uint32 x = byteclamp(s.x);
        const uint32 y = byteclamp(s.y);
        const uint32 z = byteclamp(s.z);
        const uint32 w = byteclamp(s.w);
        return x | (y << 8) | (z << 16) | (w << 24);
    }

    static inline simd4i simd4i_unpack(uint32 s)
    {
        simd4i v;
        v.x = (s >> 0) & 0xff;
        v.y = (s >> 8) & 0xff;
        v.z = (s >> 16) & 0xff;
        v.w = (s >> 24);
        return v;
    }
