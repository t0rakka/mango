/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/simd/scalar_detail.hpp>

namespace mango::simd
{

    // -----------------------------------------------------------------
    // u32x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    inline u32x2 set_component(u32x2 a, u32 s)
    {
        static_assert(Index < 2, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    inline u32 get_component(u32x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return a[Index];
    }

    inline u32x2 u32x2_zero()
    {
        return detail::scalar_set<u32, 2>(0);
    }

    inline u32x2 u32x2_set(u32 s)
    {
        return detail::scalar_set<u32, 2>(s);
    }

    inline u32x2 u32x2_set(u32 x, u32 y)
    {
        return {{ x, y }};
    }

    inline u32x2 u32x2_uload(const void* source)
    {
        u32x2 temp;
        std::memcpy(&temp, source, sizeof(temp));
        return temp;
    }

    inline void u32x2_ustore(void* dest, u32x2 a)
    {
        std::memcpy(dest, &a, sizeof(a));
    }

    inline u32x2 add(u32x2 a, u32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_add, a, b);
    }

    inline u32x2 sub(u32x2 a, u32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_sub, a, b);
    }

    inline u32x2 bitwise_nand(u32x2 a, u32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_nand, a, b);
    }

    inline u32x2 bitwise_and(u32x2 a, u32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_and, a, b);
    }

    inline u32x2 bitwise_or(u32x2 a, u32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_or, a, b);
    }

    inline u32x2 bitwise_xor(u32x2 a, u32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_xor, a, b);
    }

    inline u32x2 bitwise_not(u32x2 a)
    {
        return detail::scalar_unroll(detail::scalar_not, a);
    }

    inline u32x2 min(u32x2 a, u32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_min, a, b);
    }

    inline u32x2 max(u32x2 a, u32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_max, a, b);
    }

    // -----------------------------------------------------------------
    // s32x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    inline s32x2 set_component(s32x2 a, s32 s)
    {
        static_assert(Index < 2, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    inline s32 get_component(s32x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return a[Index];
    }

    inline s32x2 s32x2_zero()
    {
        return detail::scalar_set<s32, 2>(0);
    }

    inline s32x2 s32x2_set(s32 s)
    {
        return detail::scalar_set<s32, 2>(s);
    }

    inline s32x2 s32x2_set(s32 x, s32 y)
    {
        return {{ x, y }};
    }

    inline s32x2 s32x2_uload(const void* source)
    {
        s32x2 temp;
        std::memcpy(&temp, source, sizeof(temp));
        return temp;
    }

    inline void s32x2_ustore(void* dest, s32x2 a)
    {
        std::memcpy(dest, &a, sizeof(a));
    }

    inline s32x2 add(s32x2 a, s32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_add, a, b);
    }

    inline s32x2 sub(s32x2 a, s32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_sub, a, b);
    }

    inline s32x2 bitwise_nand(s32x2 a, s32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_nand, a, b);
    }

    inline s32x2 bitwise_and(s32x2 a, s32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_and, a, b);
    }

    inline s32x2 bitwise_or(s32x2 a, s32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_or, a, b);
    }

    inline s32x2 bitwise_xor(s32x2 a, s32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_xor, a, b);
    }

    inline s32x2 bitwise_not(s32x2 a)
    {
        return detail::scalar_unroll(detail::scalar_not, a);
    }

    inline s32x2 min(s32x2 a, s32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_min, a, b);
    }

    inline s32x2 max(s32x2 a, s32x2 b)
    {
        return detail::scalar_unroll(detail::scalar_max, a, b);
    }

} // namespace mango::simd
