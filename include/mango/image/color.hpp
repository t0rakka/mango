/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "../core/configure.hpp"
#include "../core/endian.hpp"

namespace mango
{

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

        PackedColor(uint32 color)
        {
            ustore32le(component, color);
        }

        operator uint32 () const
        {
            return uload32le(component);
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

    struct BGRA
    {
        uint8 b, g, r, a;

        BGRA() = default;

        BGRA(uint8 r, uint8 g, uint8 b, uint8 a)
            : b(b), g(g), r(r), a(a)
        {
        }

        BGRA(uint32 value)
        {
            ustore32le(&b, value);
        }

        operator uint32 () const
        {
            return uload32le(&b);
        }
    };

    struct RGBA
    {
        uint8 r, g, b, a;

        RGBA() = default;

        RGBA(uint8 r, uint8 g, uint8 b, uint8 a)
            : r(r), g(g), b(b), a(a)
        {
        }

        RGBA(uint32 value)
        {
            ustore32le(&r, value);
        }

        operator uint32 () const
        {
            return uload32le(&r);
        }
    };

    struct Palette
    {
        uint32 size { 0 };
        BGRA color[256];

        BGRA& operator [] (int index)
        {
            return color[index];
        }

        BGRA operator [] (int index) const
        {
            return color[index];
        }
    };

    constexpr uint32 makeBGRA(uint32 r, uint32 g, uint32 b, uint32 a) noexcept
    {
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    constexpr uint32 makeRGBA(uint32 r, uint32 g, uint32 b, uint32 a) noexcept
    {
        return (a << 24) | (b << 16) | (g << 8) | r;
    }

} // namespace mango
