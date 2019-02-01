/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "../core/configure.hpp"
#include "../core/bits.hpp"
#include "color.hpp"

namespace mango
{

    constexpr u32 makeOrderMask(int c0, int c1, int c2, int c3) noexcept
    {
        return (c3 << 6) | (c2 << 4) | (c1 << 2) | c0;
    }

    struct Format
    {
        enum Component : u32
        {
            RED   = 0,
            GREEN = 1,
            BLUE  = 2,
            ALPHA = 3
        };

        enum Order : u32
        {
            R = makeOrderMask(0, 1, 2, 3),
            G = makeOrderMask(1, 0, 2, 3),
            B = makeOrderMask(2, 1, 0, 3),
            A = makeOrderMask(3, 1, 2, 0),

            RG = makeOrderMask(0, 1, 2, 3),
            RB = makeOrderMask(0, 2, 1, 3),
            RA = makeOrderMask(0, 3, 2, 1),
            GR = makeOrderMask(1, 0, 2, 3),
            GB = makeOrderMask(1, 2, 0, 3),
            GA = makeOrderMask(1, 3, 0, 2),
            BR = makeOrderMask(2, 0, 1, 3),
            BG = makeOrderMask(2, 1, 0, 3),
            BA = makeOrderMask(2, 3, 0, 1),
            AR = makeOrderMask(3, 0, 1, 2),
            AG = makeOrderMask(3, 1, 0, 2),
            AB = makeOrderMask(3, 2, 0, 1),

            RGB = makeOrderMask(0, 1, 2, 3),
            RGA = makeOrderMask(0, 1, 3, 2),
            RBG = makeOrderMask(0, 2, 1, 3),
            RBA = makeOrderMask(0, 2, 3, 1),
            RAG = makeOrderMask(0, 3, 1, 2),
            RAB = makeOrderMask(0, 3, 2, 1),
            GRB = makeOrderMask(1, 0, 2, 3),
            GRA = makeOrderMask(1, 0, 3, 2),
            GBR = makeOrderMask(1, 2, 0, 3),
            GBA = makeOrderMask(1, 2, 3, 0),
            GAR = makeOrderMask(1, 3, 0, 2),
            GAB = makeOrderMask(1, 3, 2, 0),
            BRG = makeOrderMask(2, 0, 1, 3),
            BRA = makeOrderMask(2, 0, 3, 1),
            BGR = makeOrderMask(2, 1, 0, 3),
            BGA = makeOrderMask(2, 1, 3, 0),
            BAR = makeOrderMask(2, 3, 0, 1),
            BAG = makeOrderMask(2, 3, 1, 0),
            ARG = makeOrderMask(3, 0, 1, 2),
            ARB = makeOrderMask(3, 0, 2, 1),
            AGR = makeOrderMask(3, 1, 0, 2),
            AGB = makeOrderMask(3, 1, 2, 0),
            ABR = makeOrderMask(3, 2, 0, 1),
            ABG = makeOrderMask(3, 2, 1, 0),

            RGBA = makeOrderMask(0, 1, 2, 3),
            RGAB = makeOrderMask(0, 1, 3, 2),
            RBGA = makeOrderMask(0, 2, 1, 3),
            RBAG = makeOrderMask(0, 2, 3, 1),
            RAGB = makeOrderMask(0, 3, 1, 2),
            RABG = makeOrderMask(0, 3, 2, 1),
            GRBA = makeOrderMask(1, 0, 2, 3),
            GRAB = makeOrderMask(1, 0, 3, 2),
            GBRA = makeOrderMask(1, 2, 0, 3),
            GBAR = makeOrderMask(1, 2, 3, 0),
            GARB = makeOrderMask(1, 3, 0, 2),
            GABR = makeOrderMask(1, 3, 2, 0),
            BRGA = makeOrderMask(2, 0, 1, 3),
            BRAG = makeOrderMask(2, 0, 3, 1),
            BGRA = makeOrderMask(2, 1, 0, 3),
            BGAR = makeOrderMask(2, 1, 3, 0),
            BARG = makeOrderMask(2, 3, 3, 1),
            BAGR = makeOrderMask(2, 3, 1, 0),
            ARGB = makeOrderMask(3, 0, 1, 2),
            ARBG = makeOrderMask(3, 0, 2, 1),
            AGRB = makeOrderMask(3, 1, 0, 2),
            AGBR = makeOrderMask(3, 1, 2, 0),
            ABRG = makeOrderMask(3, 2, 0, 1),
            ABGR = makeOrderMask(3, 2, 1, 0)
        };

        enum Type : u32
        {
            NONE   = 0,
            SRGB   = 1,
            UNORM  = 2,
            SNORM  = 3,
            UINT   = 4,
            SINT   = 5,
            FP16   = 6,
            FP32   = 7,
            FP64   = 8
        };

        u32 bits;
        Type type;
        ColorRGBA size;
        ColorRGBA offset;

        Format()
            : bits(0)
            , type(NONE)
            , size(0)
            , offset(0)
        {
        }

        Format(int bits, Type type, const ColorRGBA& size, const ColorRGBA& offset)
            : bits(bits)
            , type(type)
            , size(size)
            , offset(offset)
        {
        }

        explicit Format(int bits, u32 luminanceMask, u32 alphaMask);
        explicit Format(int bits, u32 redMask, u32 greenMask, u32 blueMask, u32 alphaMask);
        explicit Format(int bits, Type type, Order order, int s0, int s1, int s2, int s3);
        Format(const Format& format) = default;
        ~Format() = default;

        const Format& operator = (const Format& format);

        bool operator == (const Format& format) const;
        bool operator != (const Format& format) const;
        bool operator < (const Format& format) const;

        int bytes() const;
        bool alpha() const;
        bool luminance() const;
        u32 mask(int component) const;
        u32 pack(float red, float green, float blue, float alpha) const;
    };

    // ----------------------------------------------------------------------------
    // Format macros
    // ----------------------------------------------------------------------------

    // NONE
    #define FORMAT_NONE                 Format()

    // UNORM
    #define FORMAT_B8G8R8               Format(24, Format::UNORM, Format::BGR,  8, 8, 8, 0)
    #define FORMAT_R8G8B8               Format(24, Format::UNORM, Format::RGB,  8, 8, 8, 0)
    #define FORMAT_B8G8R8A8             Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8)
    #define FORMAT_B8G8R8X8             Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 0)
    #define FORMAT_R8G8B8A8             Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8)
    #define FORMAT_R8G8B8X8             Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 0)
    #define FORMAT_B5G6R5               Format(16, Format::UNORM, Format::BGR,  5, 6, 5, 0)
    #define FORMAT_B5G5R5X1             Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 0)
    #define FORMAT_B5G5R5A1             Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 1)
    #define FORMAT_B4G4R4A4             Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4)
    #define FORMAT_B4G4R4X4             Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 0)
    #define FORMAT_B2G3R3               Format(8,  Format::UNORM, Format::BGR,  2, 3, 3, 0)
    #define FORMAT_B2G3R3A8             Format(16, Format::UNORM, Format::BGRA, 2, 3, 3, 8)
    #define FORMAT_R10G10B10A2          Format(32, Format::UNORM, Format::RGBA, 10, 10, 10, 2)
    #define FORMAT_B10G10R10A2          Format(32, Format::UNORM, Format::BGRA, 10, 10, 10, 2)
    #define FORMAT_R16G16               Format(32, Format::UNORM, Format::RG,   16, 16, 0, 0)
    #define FORMAT_R16G16B16A16         Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16)
    #define FORMAT_A8                   Format(8,  Format::UNORM, Format::A,    8, 0, 0, 0)
    #define FORMAT_R16                  Format(16, Format::UNORM, Format::R,    16, 0, 0, 0)
    #define FORMAT_RG16                 Format(32, Format::UNORM, Format::RG,   16, 16, 0, 0)
    #define FORMAT_RGB16                Format(48, Format::UNORM, Format::RGB,  16, 16, 16, 0)
    #define FORMAT_RGBA16               Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16)

    // UNORM luminance
    #define FORMAT_L8                   Format(8, 0xff, 0)
    #define FORMAT_L8A8                 Format(16, 0x00ff, 0xff00)
    #define FORMAT_L4A4                 Format(8, 0x0f, 0xf0)
    #define FORMAT_L16                  Format(16, 0xffff, 0)
    #define FORMAT_L16A16               Format(32, Format::UNORM, ColorRGBA(16, 16, 16, 16), ColorRGBA(0, 0, 0, 16))

    // FLOAT / HALF luminance
    #define FORMAT_L16F                 Format(16, Format::FP16, ColorRGBA(16, 16, 16, 0), ColorRGBA(0, 0, 0, 0))
    #define FORMAT_L32F                 Format(32, Format::FP32, ColorRGBA(32, 32, 32, 0), ColorRGBA(0, 0, 0, 0))

    // ----------------------------------------------------------------------------
    // OpenGL packed formats
    // ----------------------------------------------------------------------------

    // GL_RGB
    #define FORMAT_RGB_UNSIGNED_SHORT_5_6_5              Format(16, Format::UNORM, Format::BGR, 5, 6, 5, 0)
    #define FORMAT_RGB_UNSIGNED_SHORT_5_6_5_REV          Format(16, Format::UNORM, Format::RGB, 5, 6, 5, 0)
    #define FORMAT_RGB_UNSIGNED_BYTE_3_3_2               Format(8,  Format::UNORM, Format::BGR, 2, 3, 3, 0)
    #define FORMAT_RGB_UNSIGNED_BYTE_2_3_3_REV           Format(8,  Format::UNORM, Format::RGB, 3, 3, 2, 0)

    // GL_BGRA
    #define FORMAT_BGRA_UNSIGNED_INT_8_8_8_8             Format(32, Format::UNORM, Format::ARGB, 8, 8, 8, 8)
    #define FORMAT_BGRA_UNSIGNED_INT_8_8_8_8_REV         Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8)
    #define FORMAT_BGRA_UNSIGNED_INT_10_10_10_2          Format(32, Format::UNORM, Format::ARGB, 2, 10, 10, 10)
    #define FORMAT_BGRA_UNSIGNED_INT_2_10_10_10_REV      Format(32, Format::UNORM, Format::BGRA, 10, 10, 10, 2)
    #define FORMAT_BGRA_UNSIGNED_SHORT_4_4_4_4           Format(16, Format::UNORM, Format::ARGB, 4, 4, 4, 4)
    #define FORMAT_BGRA_UNSIGNED_SHORT_4_4_4_4_REV       Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4)
    #define FORMAT_BGRA_UNSIGNED_SHORT_5_5_5_1           Format(16, Format::UNORM, Format::ARGB, 1, 5, 5, 5)
    #define FORMAT_BGRA_UNSIGNED_SHORT_1_5_5_5_REV       Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 1)

    // GL_RGBA
    #define FORMAT_RGBA_UNSIGNED_INT_8_8_8_8             Format(32, Format::UNORM, Format::ABGR, 8, 8, 8, 8)
    #define FORMAT_RGBA_UNSIGNED_INT_8_8_8_8_REV         Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8)
    #define FORMAT_RGBA_UNSIGNED_INT_10_10_10_2          Format(32, Format::UNORM, Format::ABGR, 2, 10, 10, 10)
    #define FORMAT_RGBA_UNSIGNED_INT_2_10_10_10_REV      Format(32, Format::UNORM, Format::RGBA, 10, 10, 10, 2)
    #define FORMAT_RGBA_UNSIGNED_SHORT_4_4_4_4           Format(16, Format::UNORM, Format::ABGR, 4, 4, 4, 4)
    #define FORMAT_RGBA_UNSIGNED_SHORT_4_4_4_4_REV       Format(16, Format::UNORM, Format::RGBA, 4, 4, 4, 4)
    #define FORMAT_RGBA_UNSIGNED_SHORT_5_5_5_1           Format(16, Format::UNORM, Format::ABGR, 1, 5, 5, 5)
    #define FORMAT_RGBA_UNSIGNED_SHORT_1_5_5_5_REV       Format(16, Format::UNORM, Format::RGBA, 5, 5, 5, 1)

    // GL_ABGR_EXT
    #define FORMAT_ABGR_EXT_UNSIGNED_INT_8_8_8_8         Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8)
    #define FORMAT_ABGR_EXT_UNSIGNED_INT_8_8_8_8_REV     Format(32, Format::UNORM, Format::ABGR, 8, 8, 8, 8)
    #define FORMAT_ABGR_EXT_UNSIGNED_INT_10_10_10_2      Format(32, Format::UNORM, Format::RGBA, 2, 10, 10, 10)
    #define FORMAT_ABGR_EXT_UNSIGNED_INT_2_10_10_10_REV  Format(32, Format::UNORM, Format::ABGR, 10, 10, 10, 2)
    #define FORMAT_ABGR_EXT_UNSIGNED_SHORT_4_4_4_4       Format(16, Format::UNORM, Format::RGBA, 4, 4, 4, 4)
    #define FORMAT_ABGR_EXT_UNSIGNED_SHORT_4_4_4_4_REV   Format(16, Format::UNORM, Format::ABGR, 4, 4, 4,4)
    #define FORMAT_ABGR_EXT_UNSIGNED_SHORT_5_5_5_1       Format(16, Format::UNORM, Format::RGBA, 1, 5, 5, 5)
    #define FORMAT_ABGR_EXT_UNSIGNED_SHORT_1_5_5_5_REV   Format(16, Format::UNORM, Format::ABGR, 5, 5, 5, 1)

    // FLOAT / HALF
    #define FORMAT_RGBA16F                               Format(64,  Format::FP16, Format::RGBA, 16, 16, 16, 16)
    #define FORMAT_RGBA32F                               Format(128, Format::FP32, Format::RGBA, 32, 32, 32, 32)

} // namespace mango
