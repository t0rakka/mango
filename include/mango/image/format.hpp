/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/configure.hpp>
#include <mango/core/bits.hpp>
#include <mango/image/color.hpp>

namespace mango::image
{

    class Format
    {
    protected:
        enum : u16
        {
            TYPE_NORM       = 0x0020,
            TYPE_INT        = 0x0040,
            TYPE_FLOAT      = 0x0080,
            TYPE_SIGNED     = 0x0100,

            FLAG_LUMINANCE  = 0x0001,
            FLAG_INDEXED    = 0x0002,
        };

    public:
        enum Component : u32
        {
            RED   = 0,
            GREEN = 1,
            BLUE  = 2,
            ALPHA = 3
        };

        enum Order : u32
        {
            R = u8_mask(0, 1, 2, 3),
            G = u8_mask(1, 0, 2, 3),
            B = u8_mask(2, 1, 0, 3),
            A = u8_mask(3, 1, 2, 0),

            RG = u8_mask(0, 1, 2, 3),
            RB = u8_mask(0, 2, 1, 3),
            RA = u8_mask(0, 3, 2, 1),
            GR = u8_mask(1, 0, 2, 3),
            GB = u8_mask(1, 2, 0, 3),
            GA = u8_mask(1, 3, 0, 2),
            BR = u8_mask(2, 0, 1, 3),
            BG = u8_mask(2, 1, 0, 3),
            BA = u8_mask(2, 3, 0, 1),
            AR = u8_mask(3, 0, 1, 2),
            AG = u8_mask(3, 1, 0, 2),
            AB = u8_mask(3, 2, 0, 1),

            RGB = u8_mask(0, 1, 2, 3),
            RGA = u8_mask(0, 1, 3, 2),
            RBG = u8_mask(0, 2, 1, 3),
            RBA = u8_mask(0, 2, 3, 1),
            RAG = u8_mask(0, 3, 1, 2),
            RAB = u8_mask(0, 3, 2, 1),
            GRB = u8_mask(1, 0, 2, 3),
            GRA = u8_mask(1, 0, 3, 2),
            GBR = u8_mask(1, 2, 0, 3),
            GBA = u8_mask(1, 2, 3, 0),
            GAR = u8_mask(1, 3, 0, 2),
            GAB = u8_mask(1, 3, 2, 0),
            BRG = u8_mask(2, 0, 1, 3),
            BRA = u8_mask(2, 0, 3, 1),
            BGR = u8_mask(2, 1, 0, 3),
            BGA = u8_mask(2, 1, 3, 0),
            BAR = u8_mask(2, 3, 0, 1),
            BAG = u8_mask(2, 3, 1, 0),
            ARG = u8_mask(3, 0, 1, 2),
            ARB = u8_mask(3, 0, 2, 1),
            AGR = u8_mask(3, 1, 0, 2),
            AGB = u8_mask(3, 1, 2, 0),
            ABR = u8_mask(3, 2, 0, 1),
            ABG = u8_mask(3, 2, 1, 0),

            RGBA = u8_mask(0, 1, 2, 3),
            RGAB = u8_mask(0, 1, 3, 2),
            RBGA = u8_mask(0, 2, 1, 3),
            RBAG = u8_mask(0, 2, 3, 1),
            RAGB = u8_mask(0, 3, 1, 2),
            RABG = u8_mask(0, 3, 2, 1),
            GRBA = u8_mask(1, 0, 2, 3),
            GRAB = u8_mask(1, 0, 3, 2),
            GBRA = u8_mask(1, 2, 0, 3),
            GBAR = u8_mask(1, 2, 3, 0),
            GARB = u8_mask(1, 3, 0, 2),
            GABR = u8_mask(1, 3, 2, 0),
            BRGA = u8_mask(2, 0, 1, 3),
            BRAG = u8_mask(2, 0, 3, 1),
            BGRA = u8_mask(2, 1, 0, 3),
            BGAR = u8_mask(2, 1, 3, 0),
            BARG = u8_mask(2, 3, 3, 1),
            BAGR = u8_mask(2, 3, 1, 0),
            ARGB = u8_mask(3, 0, 1, 2),
            ARBG = u8_mask(3, 0, 2, 1),
            AGRB = u8_mask(3, 1, 0, 2),
            AGBR = u8_mask(3, 1, 2, 0),
            ABRG = u8_mask(3, 2, 0, 1),
            ABGR = u8_mask(3, 2, 1, 0)
        };

        enum Type : u16
        {
            NONE    = 0,
            UNORM   = 1 | TYPE_NORM,
            SNORM   = 2 | TYPE_NORM | TYPE_SIGNED,
            UINT    = 3 | TYPE_INT,
            SINT    = 4 | TYPE_INT   | TYPE_SIGNED,
            FLOAT16 = 5 | TYPE_FLOAT | TYPE_SIGNED,
            FLOAT32 = 6 | TYPE_FLOAT | TYPE_SIGNED,
            FLOAT64 = 7 | TYPE_FLOAT | TYPE_SIGNED,
        };

        u32 bits;
        Type type;
        u16 flags;
        Color size;
        Color offset;

        Format();
        explicit Format(int bits, u32 redMask, u32 greenMask, u32 blueMask, u32 alphaMask);
        explicit Format(int bits, Type type, Color size, Color offset);
        explicit Format(int bits, Type type, Order order, int s0, int s1 = 0, int s2 = 0, int s3 = 0);
        Format(const Format& format) = default;
        ~Format() = default;

        const Format& operator = (const Format& format);

        bool operator == (const Format& format) const;
        bool operator != (const Format& format) const;
        bool operator < (const Format& format) const;

        int bytes() const;
        bool isAlpha() const;
        bool isLuminance() const;
        bool isIndexed() const;
        bool isFloat() const;
        u32 mask(int component) const;
        u32 pack(float red, float green, float blue, float alpha) const;
    };

    struct LuminanceFormat : Format
    {
        explicit LuminanceFormat(int bits, u32 luminanceMask, u32 alphaMask);
        explicit LuminanceFormat(int bits, Type type, u8 luminanceBits, u8 alphaBits);
    };

    struct IndexedFormat : Format
    {
        explicit IndexedFormat(int bits);
    };

    // ----------------------------------------------------------------------------
    // Format macros
    // ----------------------------------------------------------------------------

    /*

    // UNORM
    #define FORMAT_B8G8R8               Format(24, mango::Format::UNORM, mango::Format::BGR,  8, 8, 8, 0)
    #define FORMAT_R8G8B8               Format(24, mango::Format::UNORM, mango::Format::RGB,  8, 8, 8, 0)
    #define FORMAT_B8G8R8A8             Format(32, mango::Format::UNORM, mango::Format::BGRA, 8, 8, 8, 8)
    #define FORMAT_B8G8R8X8             Format(32, mango::Format::UNORM, mango::Format::BGRA, 8, 8, 8, 0)
    #define FORMAT_R8G8B8A8             Format(32, mango::Format::UNORM, mango::Format::RGBA, 8, 8, 8, 8)
    #define FORMAT_R8G8B8X8             Format(32, mango::Format::UNORM, mango::Format::RGBA, 8, 8, 8, 0)
    #define FORMAT_B5G6R5               Format(16, mango::Format::UNORM, mango::Format::BGR,  5, 6, 5, 0)
    #define FORMAT_B5G5R5X1             Format(16, mango::Format::UNORM, mango::Format::BGRA, 5, 5, 5, 0)
    #define FORMAT_B5G5R5A1             Format(16, mango::Format::UNORM, mango::Format::BGRA, 5, 5, 5, 1)
    #define FORMAT_B4G4R4A4             Format(16, mango::Format::UNORM, mango::Format::BGRA, 4, 4, 4, 4)
    #define FORMAT_B4G4R4X4             Format(16, mango::Format::UNORM, mango::Format::BGRA, 4, 4, 4, 0)
    #define FORMAT_B2G3R3               Format(8,  mango::Format::UNORM, mango::Format::BGR,  2, 3, 3, 0)
    #define FORMAT_B2G3R3A8             Format(16, mango::Format::UNORM, mango::Format::BGRA, 2, 3, 3, 8)
    #define FORMAT_R10G10B10A2          Format(32, mango::Format::UNORM, mango::Format::RGBA, 10, 10, 10, 2)
    #define FORMAT_B10G10R10A2          Format(32, mango::Format::UNORM, mango::Format::BGRA, 10, 10, 10, 2)
    #define FORMAT_R16G16               Format(32, mango::Format::UNORM, mango::Format::RG,   16, 16, 0, 0)
    #define FORMAT_R16G16B16A16         Format(64, mango::Format::UNORM, mango::Format::RGBA, 16, 16, 16, 16)
    #define FORMAT_A8                   Format(8,  mango::Format::UNORM, mango::Format::A,    8, 0, 0, 0)
    #define FORMAT_R16                  Format(16, mango::Format::UNORM, mango::Format::R,    16, 0, 0, 0)
    #define FORMAT_RG16                 Format(32, mango::Format::UNORM, mango::Format::RG,   16, 16, 0, 0)
    #define FORMAT_RGB16                Format(48, mango::Format::UNORM, mango::Format::RGB,  16, 16, 16, 0)
    #define FORMAT_RGBA16               Format(64, mango::Format::UNORM, mango::Format::RGBA, 16, 16, 16, 16)

    // UNORM luminance
    #define FORMAT_L8                   LuminanceFormat( 8, mango::Format::UNORM,  8,  0)
    #define FORMAT_L8A8                 LuminanceFormat(16, mango::Format::UNORM,  8,  8)
    #define FORMAT_L4A4                 LuminanceFormat( 8, mango::Format::UNORM,  4,  4)
    #define FORMAT_L16                  LuminanceFormat(16, mango::Format::UNORM, 16,  0)
    #define FORMAT_L16A16               LuminanceFormat(32, mango::Format::UNORM, 16, 16)

    // FLOAT / HALF luminance
    #define FORMAT_L16F                 LuminanceFormat(16, mango::Format::FLOAT16, 16, 0)
    #define FORMAT_L32F                 LuminanceFormat(32, mango::Format::FLOAT32, 32, 0)

    // ----------------------------------------------------------------------------
    // OpenGL packed formats
    // ----------------------------------------------------------------------------

    // GL_RGB
    #define FORMAT_RGB_UNSIGNED_SHORT_5_6_5              Format(16, mango::Format::UNORM, mango::Format::BGR, 5, 6, 5, 0)
    #define FORMAT_RGB_UNSIGNED_SHORT_5_6_5_REV          Format(16, mango::Format::UNORM, mango::Format::RGB, 5, 6, 5, 0)
    #define FORMAT_RGB_UNSIGNED_BYTE_3_3_2               Format(8,  mango::Format::UNORM, mango::Format::BGR, 2, 3, 3, 0)
    #define FORMAT_RGB_UNSIGNED_BYTE_2_3_3_REV           Format(8,  mango::Format::UNORM, mango::Format::RGB, 3, 3, 2, 0)

    // GL_BGRA
    #define FORMAT_BGRA_UNSIGNED_INT_8_8_8_8             Format(32, mango::Format::UNORM, mango::Format::ARGB, 8, 8, 8, 8)
    #define FORMAT_BGRA_UNSIGNED_INT_8_8_8_8_REV         Format(32, mango::Format::UNORM, mango::Format::BGRA, 8, 8, 8, 8)
    #define FORMAT_BGRA_UNSIGNED_INT_10_10_10_2          Format(32, mango::Format::UNORM, mango::Format::ARGB, 2, 10, 10, 10)
    #define FORMAT_BGRA_UNSIGNED_INT_2_10_10_10_REV      Format(32, mango::Format::UNORM, mango::Format::BGRA, 10, 10, 10, 2)
    #define FORMAT_BGRA_UNSIGNED_SHORT_4_4_4_4           Format(16, mango::Format::UNORM, mango::Format::ARGB, 4, 4, 4, 4)
    #define FORMAT_BGRA_UNSIGNED_SHORT_4_4_4_4_REV       Format(16, mango::Format::UNORM, mango::Format::BGRA, 4, 4, 4, 4)
    #define FORMAT_BGRA_UNSIGNED_SHORT_5_5_5_1           Format(16, mango::Format::UNORM, mango::Format::ARGB, 1, 5, 5, 5)
    #define FORMAT_BGRA_UNSIGNED_SHORT_1_5_5_5_REV       Format(16, mango::Format::UNORM, mango::Format::BGRA, 5, 5, 5, 1)

    // GL_RGBA
    #define FORMAT_RGBA_UNSIGNED_INT_8_8_8_8             Format(32, mango::Format::UNORM, mango::Format::ABGR, 8, 8, 8, 8)
    #define FORMAT_RGBA_UNSIGNED_INT_8_8_8_8_REV         Format(32, mango::Format::UNORM, mango::Format::RGBA, 8, 8, 8, 8)
    #define FORMAT_RGBA_UNSIGNED_INT_10_10_10_2          Format(32, mango::Format::UNORM, mango::Format::ABGR, 2, 10, 10, 10)
    #define FORMAT_RGBA_UNSIGNED_INT_2_10_10_10_REV      Format(32, mango::Format::UNORM, mango::Format::RGBA, 10, 10, 10, 2)
    #define FORMAT_RGBA_UNSIGNED_SHORT_4_4_4_4           Format(16, mango::Format::UNORM, mango::Format::ABGR, 4, 4, 4, 4)
    #define FORMAT_RGBA_UNSIGNED_SHORT_4_4_4_4_REV       Format(16, mango::Format::UNORM, mango::Format::RGBA, 4, 4, 4, 4)
    #define FORMAT_RGBA_UNSIGNED_SHORT_5_5_5_1           Format(16, mango::Format::UNORM, mango::Format::ABGR, 1, 5, 5, 5)
    #define FORMAT_RGBA_UNSIGNED_SHORT_1_5_5_5_REV       Format(16, mango::Format::UNORM, mango::Format::RGBA, 5, 5, 5, 1)

    // GL_ABGR_EXT
    #define FORMAT_ABGR_EXT_UNSIGNED_INT_8_8_8_8         Format(32, mango::Format::UNORM, mango::Format::RGBA, 8, 8, 8, 8)
    #define FORMAT_ABGR_EXT_UNSIGNED_INT_8_8_8_8_REV     Format(32, mango::Format::UNORM, mango::Format::ABGR, 8, 8, 8, 8)
    #define FORMAT_ABGR_EXT_UNSIGNED_INT_10_10_10_2      Format(32, mango::Format::UNORM, mango::Format::RGBA, 2, 10, 10, 10)
    #define FORMAT_ABGR_EXT_UNSIGNED_INT_2_10_10_10_REV  Format(32, mango::Format::UNORM, mango::Format::ABGR, 10, 10, 10, 2)
    #define FORMAT_ABGR_EXT_UNSIGNED_SHORT_4_4_4_4       Format(16, mango::Format::UNORM, mango::Format::RGBA, 4, 4, 4, 4)
    #define FORMAT_ABGR_EXT_UNSIGNED_SHORT_4_4_4_4_REV   Format(16, mango::Format::UNORM, mango::Format::ABGR, 4, 4, 4,4)
    #define FORMAT_ABGR_EXT_UNSIGNED_SHORT_5_5_5_1       Format(16, mango::Format::UNORM, mango::Format::RGBA, 1, 5, 5, 5)
    #define FORMAT_ABGR_EXT_UNSIGNED_SHORT_1_5_5_5_REV   Format(16, mango::Format::UNORM, mango::Format::ABGR, 5, 5, 5, 1)

    // float
    #define FORMAT_RGBA16F                               Format(64,  mango::Format::FLOAT16, mango::Format::RGBA, 16, 16, 16, 16)
    #define FORMAT_RGBA32F                               Format(128, mango::Format::FLOAT32, mango::Format::RGBA, 32, 32, 32, 32)
    #define FORMAT_RGBA64F                               Format(256, mango::Format::FLOAT64, mango::Format::RGBA, 64, 64, 64, 64)

    */

} // namespace mango::image
