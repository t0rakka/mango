/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/configure.hpp>
#include <mango/core/endian.hpp>
#include <mango/core/bits.hpp>

namespace mango::image
{

    constexpr u32 makeBGRA(u32 red, u32 green, u32 blue, u32 alpha) noexcept
    {
        return (alpha << 24) | (red << 16) | (green << 8) | blue;
    }

    constexpr u32 makeRGBA(u32 red, u32 green, u32 blue, u32 alpha) noexcept
    {
        return (alpha << 24) | (blue << 16) | (green << 8) | red;
    }

    union Color
    {
        u8 component[4];
        struct
        {
            u8 r, g, b, a;
        };

        Color()
        {
            ustore32le(this, 0);
        }

        Color(u8 red, u8 green, u8 blue, u8 alpha)
        {
            ustore32le(this, makeRGBA(red, green, blue, alpha));
        }

        Color(u32 value)
        {
            ustore32le(this, value);
        }

        operator u32 () const
        {
            return uload32le(this);
        }

        u8& operator [] (int index)
        {
            return component[index];
        }

        u8 operator [] (int index) const
        {
            return component[index];
        }
    };

    struct Palette
    {
        u32 size { 0 };
        Color color[256];

        Color& operator [] (int index)
        {
            return color[index];
        }

        Color operator [] (int index) const
        {
            return color[index];
        }
    };

} // namespace mango::image
