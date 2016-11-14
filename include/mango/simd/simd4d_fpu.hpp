/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef MANGO_INCLUDE_SIMD
#error "THIS HEADER MUST NEVER BE INCLUDED MANUALLY."
#endif

#include <cmath>
#include <algorithm>
#include "../core/bits.hpp"

namespace mango
{

    // -----------------------------------------------------------------
    // simd4d
    // -----------------------------------------------------------------

    struct simd4d
    {
        double x, y, z, w;
    };

    typedef const simd4d& __simd4d;

    // -----------------------------------------------------------------
    // conversion
    // -----------------------------------------------------------------

    static inline simd4d simd4d_convert(__simd4i s)
    {
        simd4d v;
        v.x = double(simd4i_get_x(s));
        v.y = double(simd4i_get_y(s));
        v.z = double(simd4i_get_z(s));
        v.w = double(simd4i_get_w(s));
        return v;
    }

    static inline simd4i simd4i_convert(__simd4d s)
    {
        int x = int(s.x + 0.5);
        int y = int(s.y + 0.5);
        int z = int(s.z + 0.5);
        int w = int(s.w + 0.5);
        return simd4i_set4(x, y, z, w);
    }

    static inline simd4d simd4d_unsigned_convert(__simd4i i)
    {
        simd4d v;
        v.x = u32_to_f64(simd4i_get_x(i));
        v.y = u32_to_f64(simd4i_get_y(i));
        v.z = u32_to_f64(simd4i_get_z(i));
        v.w = u32_to_f64(simd4i_get_w(i));
        return v;
    }

    static inline simd4i simd4i_unsigned_convert(__simd4d d)
    {
        uint32 x = f64_to_u32(d.x);
        uint32 y = f64_to_u32(d.y);
        uint32 z = f64_to_u32(d.z);
        uint32 w = f64_to_u32(d.w);
        return simd4i_set4(x, y, z, w);
    }

    static inline simd4i simd4i_truncate(__simd4d s)
    {
        int x = int(s.x);
        int y = int(s.y);
        int z = int(s.z);
        int w = int(s.w);
        return simd4i_set4(x, y, z, w);
    }

    // -----------------------------------------------------------------
    // simd4d
    // -----------------------------------------------------------------

    template <int x, int y, int z, int w>
    inline simd4d simd4d_shuffle(__simd4d v)
    {
        // .generic
        const double* s = reinterpret_cast<const double*>(&v);
        simd4d n = { s[x], s[y], s[z], s[w] };
        return n;
    }

    template <>
    inline simd4d simd4d_shuffle<0, 1, 2, 3>(__simd4d v)
    {
        // .xyzw
        return v;
    }

    // set component

    template <int Index>
    static inline simd4d simd4d_set_component(simd4d a, double s);

    template <>
    inline simd4d simd4d_set_component<0>(simd4d a, double x)
    {
        a.x = x;
        return a;
    }

    template <>
    inline simd4d simd4d_set_component<1>(simd4d a, double y)
    {
        a.y = y;
        return a;
    }

    template <>
    inline simd4d simd4d_set_component<2>(simd4d a, double z)
    {
        a.z = z;
        return a;
    }

    template <>
    inline simd4d simd4d_set_component<3>(simd4d a, double w)
    {
        a.w = w;
        return a;
    }

    // get component

    template <int Index>
    static inline double simd4d_get_component(__simd4d a);

    template <>
    inline double simd4d_get_component<0>(__simd4d a)
    {
        return a.x;
    }

    template <>
    inline double simd4d_get_component<1>(__simd4d a)
    {
        return a.y;
    }

    template <>
    inline double simd4d_get_component<2>(__simd4d a)
    {
        return a.z;
    }

    template <>
    inline double simd4d_get_component<3>(__simd4d a)
    {
        return a.w;
    }

    static inline simd4d simd4d_set_x(simd4d a, double x)
    {
        a.x = x;
        return a;
    }

    static inline simd4d simd4d_set_y(simd4d a, double y)
    {
        a.y = y;
        return a;
    }

    static inline simd4d simd4d_set_z(simd4d a, double z)
    {
        a.z = z;
        return a;
    }

    static inline simd4d simd4d_set_w(simd4d a, double w)
    {
        a.w = w;
        return a;
    }

    static inline double simd4d_get_x(__simd4d a)
    {
        return a.x;
    }

    static inline double simd4d_get_y(__simd4d a)
    {
        return a.y;
    }

    static inline double simd4d_get_z(__simd4d a)
    {
        return a.z;
    }

    static inline double simd4d_get_w(__simd4d a)
    {
        return a.w;
    }

    static inline simd4d simd4d_zero()
    {
        simd4d temp = { 0.0, 0.0, 0.0, 0.0 };
        return temp;
    }

    static inline simd4d simd4d_set1(double s)
    {
        simd4d temp = { s, s, s, s };
        return temp;
    }

    static inline simd4d simd4d_set4(double x, double y, double z, double w)
    {
        simd4d temp = { x, y, z, w };
        return temp;
    }

    static inline simd4d simd4d_load(const double* source)
    {
        simd4d temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline simd4d simd4d_uload(const double* source)
    {
        return simd4d_load(source);
    }

    static inline void simd4d_store(double* dest, __simd4d a)
    {
        dest[0] = a.x;
        dest[1] = a.y;
        dest[2] = a.z;
        dest[3] = a.w;
    }

    static inline void simd4d_ustore(double* dest, __simd4d a)
    {
        simd4d_store(dest, a);
    }

    static inline simd4d simd4d_unpackhi(__simd4d a, __simd4d b)
    {
        simd4d v = { a.y, b.y, a.w, b.w };
        return v;
    }

    static inline simd4d simd4d_unpacklo(__simd4d a, __simd4d b)
    {
        simd4d v = { a.x, b.x, a.z, b.z };
        return v;
    }

    // logical

    static inline simd4d simd4d_and(__simd4d a, __simd4d b)
    {
        const Double x(Double(a.x).u & Double(b.x).u);
        const Double y(Double(a.y).u & Double(b.y).u);
        const Double z(Double(a.z).u & Double(b.z).u);
        const Double w(Double(a.w).u & Double(b.w).u);
        simd4d v;
        v.x = x;
        v.y = y;
        v.z = z;
        v.w = w;
        return v;
    }

    static inline simd4d simd4d_nand(__simd4d a, __simd4d b)
    {
        const Double x(~Double(a.x).u & Double(b.x).u);
        const Double y(~Double(a.y).u & Double(b.y).u);
        const Double z(~Double(a.z).u & Double(b.z).u);
        const Double w(~Double(a.w).u & Double(b.w).u);
        simd4d v;
        v.x = x;
        v.y = y;
        v.z = z;
        v.w = w;
        return v;
    }

    static inline simd4d simd4d_or(__simd4d a, __simd4d b)
    {
        const Double x(Double(a.x).u | Double(b.x).u);
        const Double y(Double(a.y).u | Double(b.y).u);
        const Double z(Double(a.z).u | Double(b.z).u);
        const Double w(Double(a.w).u | Double(b.w).u);
        simd4d v;
        v.x = x;
        v.y = y;
        v.z = z;
        v.w = w;
        return v;
    }

    static inline simd4d simd4d_xor(__simd4d a, __simd4d b)
    {
        const Double x(Double(a.x).u ^ Double(b.x).u);
        const Double y(Double(a.y).u ^ Double(b.y).u);
        const Double z(Double(a.z).u ^ Double(b.z).u);
        const Double w(Double(a.w).u ^ Double(b.w).u);
        simd4d v;
        v.x = x;
        v.y = y;
        v.z = z;
        v.w = w;
        return v;
    }

    static inline simd4d simd4d_min(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = std::min(a.x, b.x);
        v.y = std::min(a.y, b.y);
        v.z = std::min(a.z, b.z);
        v.w = std::min(a.w, b.w);
        return v;
    }

    static inline simd4d simd4d_max(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = std::max(a.x, b.x);
        v.y = std::max(a.y, b.y);
        v.z = std::max(a.z, b.z);
        v.w = std::max(a.w, b.w);
        return v;
    }

    static inline simd4d simd4d_clamp(__simd4d a, __simd4d vmin, __simd4d vmax)
    {
        simd4d v;
        v.x = std::min(vmax.x, std::max(vmin.x, a.x));
        v.y = std::min(vmax.y, std::max(vmin.y, a.y));
        v.z = std::min(vmax.z, std::max(vmin.z, a.z));
        v.w = std::min(vmax.w, std::max(vmin.w, a.w));
        return v;
    }

    static inline simd4d simd4d_abs(__simd4d a)
    {
        simd4d v;
        v.x = std::abs(a.x);
        v.y = std::abs(a.y);
        v.z = std::abs(a.z);
        v.w = std::abs(a.w);
        return v;
    }

    static inline simd4d simd4d_neg(__simd4d a)
    {
        simd4d v = { -a.x, -a.y, -a.z, -a.w };
        return v;
    }

    static inline simd4d simd4d_add(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = a.x + b.x;
        v.y = a.y + b.y;
        v.z = a.z + b.z;
        v.w = a.w + b.w;
        return v;
    }

    static inline simd4d simd4d_sub(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = a.x - b.x;
        v.y = a.y - b.y;
        v.z = a.z - b.z;
        v.w = a.w - b.w;
        return v;
    }

    static inline simd4d simd4d_mul(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = a.x * b.x;
        v.y = a.y * b.y;
        v.z = a.z * b.z;
        v.w = a.w * b.w;
        return v;
    }

    static inline simd4d simd4d_div(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = a.x / b.x;
        v.y = a.y / b.y;
        v.z = a.z / b.z;
        v.w = a.w / b.w;
        return v;
    }

    static inline simd4d simd4d_div(__simd4d a, double b)
    {
        simd4d v;
        v.x = a.x / b;
        v.y = a.y / b;
        v.z = a.z / b;
        v.w = a.w / b;
        return v;
    }

    static inline simd4d simd4d_madd(__simd4d a, __simd4d b, __simd4d c)
    {
        simd4d v;
        v.x = a.x + b.x * c.x;
        v.y = a.y + b.y * c.y;
        v.z = a.z + b.z * c.z;
        v.w = a.w + b.w * c.w;
        return v;
    }

    static inline simd4d simd4d_msub(__simd4d a, __simd4d b, __simd4d c)
    {
        simd4d v;
        v.x = a.x - b.x * c.x;
        v.y = a.y - b.y * c.y;
        v.z = a.z - b.z * c.z;
        v.w = a.w - b.w * c.w;
        return v;
    }

    static inline simd4d simd4d_fast_reciprocal(__simd4d a)
    {
        simd4d v;
        v.x = 1.0 / a.x;
        v.y = 1.0 / a.y;
        v.z = 1.0 / a.z;
        v.w = 1.0 / a.w;
        return v;
    }

    static inline simd4d simd4d_fast_rsqrt(__simd4d a)
    {
        simd4d v;
        v.x = 1.0 / std::sqrt(a.x);
        v.y = 1.0 / std::sqrt(a.y);
        v.z = 1.0 / std::sqrt(a.z);
        v.w = 1.0 / std::sqrt(a.w);
        return v;
    }

    static inline simd4d simd4d_fast_sqrt(__simd4d a)
    {
        simd4d v;
        v.x = std::sqrt(a.x);
        v.y = std::sqrt(a.y);
        v.z = std::sqrt(a.z);
        v.w = std::sqrt(a.w);
        return v;
    }

    static inline simd4d simd4d_reciprocal(__simd4d a)
    {
        return simd4d_fast_reciprocal(a);
    }

    static inline simd4d simd4d_rsqrt(__simd4d a)
    {
        return simd4d_fast_rsqrt(a);
    }

    static inline simd4d simd4d_sqrt(__simd4d a)
    {
        return simd4d_fast_sqrt(a);
    }

    static inline simd4d simd4d_dot4(__simd4d a, __simd4d b)
    {
        const double s = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        return simd4d_set1(s);
    }

    // compare

    static inline simd4d simd4d_compare_neq(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = Double(uint64(a.x != b.x ? 0xffffffffffffffff : 0));
        v.y = Double(uint64(a.y != b.y ? 0xffffffffffffffff : 0));
        v.z = Double(uint64(a.z != b.z ? 0xffffffffffffffff : 0));
        v.w = Double(uint64(a.w != b.w ? 0xffffffffffffffff : 0));
        return v;
    }

    static inline simd4d simd4d_compare_eq(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = Double(uint64(a.x == b.x ? 0xffffffffffffffff : 0));
        v.y = Double(uint64(a.y == b.y ? 0xffffffffffffffff : 0));
        v.z = Double(uint64(a.z == b.z ? 0xffffffffffffffff : 0));
        v.w = Double(uint64(a.w == b.w ? 0xffffffffffffffff : 0));
        return v;
    }

    static inline simd4d simd4d_compare_lt(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = Double(uint64(a.x < b.x ? 0xffffffffffffffff : 0));
        v.y = Double(uint64(a.y < b.y ? 0xffffffffffffffff : 0));
        v.z = Double(uint64(a.z < b.z ? 0xffffffffffffffff : 0));
        v.w = Double(uint64(a.w < b.w ? 0xffffffffffffffff : 0));
        return v;
    }

    static inline simd4d simd4d_compare_le(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = Double(uint64(a.x <= b.x ? 0xffffffffffffffff : 0));
        v.y = Double(uint64(a.y <= b.y ? 0xffffffffffffffff : 0));
        v.z = Double(uint64(a.z <= b.z ? 0xffffffffffffffff : 0));
        v.w = Double(uint64(a.w <= b.w ? 0xffffffffffffffff : 0));
        return v;
    }

    static inline simd4d simd4d_compare_gt(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = Double(uint64(a.x > b.x ? 0xffffffffffffffff : 0));
        v.y = Double(uint64(a.y > b.y ? 0xffffffffffffffff : 0));
        v.z = Double(uint64(a.z > b.z ? 0xffffffffffffffff : 0));
        v.w = Double(uint64(a.w > b.w ? 0xffffffffffffffff : 0));
        return v;
    }

    static inline simd4d simd4d_compare_ge(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = Double(uint64(a.x >= b.x ? 0xffffffffffffffff : 0));
        v.y = Double(uint64(a.y >= b.y ? 0xffffffffffffffff : 0));
        v.z = Double(uint64(a.z >= b.z ? 0xffffffffffffffff : 0));
        v.w = Double(uint64(a.w >= b.w ? 0xffffffffffffffff : 0));
        return v;
    }

    static inline simd4d simd4d_select(__simd4d mask, __simd4d a, __simd4d b)
    {
        return simd4d_or(simd4d_and(mask, a), simd4d_nand(mask, b));
    }

    // rounding

    static inline simd4d simd4d_round(__simd4d s)
    {
        simd4d v;
        v.x = std::round(s.x);
        v.y = std::round(s.y);
        v.z = std::round(s.z);
        v.w = std::round(s.w);
        return v;
    }

    static inline simd4d simd4d_trunc(__simd4d s)
    {
        simd4d v;
        v.x = std::trunc(s.x);
        v.y = std::trunc(s.y);
        v.z = std::trunc(s.z);
        v.w = std::trunc(s.w);
        return v;
    }

    static inline simd4d simd4d_floor(__simd4d s)
    {
        simd4d v;
        v.x = std::floor(s.x);
        v.y = std::floor(s.y);
        v.z = std::floor(s.z);
        v.w = std::floor(s.w);
        return v;
    }

    static inline simd4d simd4d_ceil(__simd4d s)
    {
        simd4d v;
        v.x = std::ceil(s.x);
        v.y = std::ceil(s.y);
        v.z = std::ceil(s.z);
        v.w = std::ceil(s.w);
        return v;
    }

    static inline simd4d simd4d_fract(__simd4d s)
    {
        return simd4d_sub(s, simd4d_floor(s));
    }

} // namespace mango
