/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/configure.hpp>
#include <mango/core/endian.hpp>
#include <mango/core/memory.hpp>
#include <mango/core/bits.hpp>
#include <mango/math/srgb.hpp>

namespace mango::image
{

    class Surface;

    // ------------------------------------------------------------------

    static constexpr
    u32 makeBGRA(u32 red, u32 green, u32 blue, u32 alpha) noexcept
    {
        return (alpha << 24) | (red << 16) | (green << 8) | blue;
    }

    static constexpr
    u32 makeRGBA(u32 red, u32 green, u32 blue, u32 alpha) noexcept
    {
        return (alpha << 24) | (blue << 16) | (green << 8) | red;
    }

    // ------------------------------------------------------------------
    // Color
    // ------------------------------------------------------------------

    union Color
    {
        u8 component[4] {};
        struct
        {
            u8 r, g, b, a;
        };

        Color()
        {
            littleEndian::ustore32(this, 0);
        }

        Color(u8 red, u8 green, u8 blue, u8 alpha)
        {
            littleEndian::ustore32(this, makeRGBA(red, green, blue, alpha));
        }

        Color(u32 value)
        {
            littleEndian::ustore32(this, value);
        }

        operator u32 () const
        {
            return littleEndian::uload32(this);
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

    // ------------------------------------------------------------------
    // Palette
    // ------------------------------------------------------------------

    struct Palette
    {
        u32 size;
        Color color[256];

        Palette()
            : size(0)
        {
        }

        Palette(u32 size)
            : size(size)
        {
        }

        Color& operator [] (int index)
        {
            return color[index];
        }

        Color operator [] (int index) const
        {
            return color[index];
        }
    };

    // ------------------------------------------------------------------
    // sRGB
    // ------------------------------------------------------------------

    struct sRGB
    {
        // NOTE: The alpha component is always linear
        Color color;

        sRGB() = default;

        sRGB(Color srgb)
            : color(srgb)
        {
        }

        sRGB(math::float32x4 linear)
        {
            *this = linear;
        }

        sRGB& operator = (math::float32x4 linear)
        {
            math::float32x4 srgb = math::linear_to_srgb(linear) * 255.0f;
            srgb.w = linear.w * 255.0f; // pass-through linear alpha
            color = srgb.pack();
            return *this;
        }

        sRGB& operator = (Color srgb)
        {
            color = srgb;
            return *this;
        }

        operator u32 () const
        {
            return color;
        }

        operator math::float32x4 () const
        {
            math::float32x4 srgb = math::float32x4::unpack(color) / 255.0f;
            math::float32x4 linear = math::srgb_to_linear(srgb);
            linear.w = float(color >> 24) / 255.0f; // pass-through linear alpha
            return linear;
        }
    };

    // ------------------------------------------------------------------
    // ColorManager
    // ------------------------------------------------------------------

    class ColorProfile : public NonCopyable
    {
    protected:
        void* m_profile;

    public:
        ColorProfile(void* profile);
        ~ColorProfile();

        operator void* () const;
    };

    class ColorManager : public NonCopyable
    {
    private:
        void* m_context;

    public:
        ColorManager();
        ~ColorManager();

        ColorProfile create(ConstMemory icc);
        ColorProfile createSRGB();

        void transform(const Surface& target, const ColorProfile& output, const ColorProfile& input);
    };

} // namespace mango::image
