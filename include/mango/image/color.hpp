/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "../core/configure.hpp"
#include "../core/endian.hpp"

namespace mango
{

    constexpr uint32 makeBGRA(int r, int g, int b, int a) noexcept
    {
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    constexpr uint32 makeRGBA(int r, int g, int b, int a) noexcept
    {
        return (a << 24) | (b << 16) | (g << 8) | r;
    }

    struct PackedColor
    {
        uint8 component[4];

        constexpr PackedColor()
        : component { 0, 0, 0, 0 }
        {
        }

        constexpr PackedColor(uint8 r, uint8 g, uint8 b, uint8 a)
        : component { r, g, b, a }
        {
        }

        constexpr PackedColor(const PackedColor& color)
        : component { color.component[0], color.component[1], color.component[2], color.component[3] }
        {
        }

        PackedColor(uint32 color)
        {
            ustore32le(component, color);
        }

        PackedColor(const uint8* v)
        {
            std::memcpy(component, v, sizeof(PackedColor));
        }

        operator uint32 () const
        {
            return uload32le(component);
        }

        operator uint8* ()
        {
            return component;
        }

        operator const uint8* () const
        {
            return component;
        }

        uint8& operator [] (int index)
        {
            return component[index];
        }

        uint8 operator [] (int index) const
        {
            return component[index];
        }

        bool operator == (const PackedColor& color)
        {
            return !std::memcmp(component, color.component, sizeof(PackedColor));
        }
    };

    struct Palette
    {
        uint32 size { 0 };
        PackedColor color[256];
    };

} // namespace mango
