/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "../core/configure.hpp"
#include "../core/bits.hpp"

namespace mango
{

    union PackedColor
    {
    private:
        uint32 packed; // packing depends on endianess

    public:
        struct
        {
            uint8 r, g, b, a;
        };

        PackedColor()
        : packed(0)
        {
        }

        PackedColor(uint8 red, uint8 green, uint8 blue, uint8 alpha)
        : r(red), g(green), b(blue), a(alpha)
        {
        }

        PackedColor(uint32 rgba)
        {
#if defined(MANGO_LITTLE_ENDIAN)
            packed = rgba;
#else
            packed = byteswap32(rgba);
#endif
        }

        PackedColor(const uint8* v)
        {
            std::memcpy(*this, v, sizeof(PackedColor));
        }

        operator uint32 () const
        {
#if defined(MANGO_LITTLE_ENDIAN)
            return packed;
#else
            return byteswap32(packed);
#endif
        }

        operator uint8* ()
        {
            return reinterpret_cast<uint8*>(this);
        }

        operator const uint8* () const
        {
            return reinterpret_cast<const uint8*>(this);
        }

        uint8& operator [] (int index)
        {
            uint8* p = *this;
            return p[index];
        }

        uint8 operator [] (int index) const
        {
            const uint8* p = *this;
            return p[index];
        }

        bool operator == (const PackedColor& color)
        {
            return !std::memcmp(this, &color, sizeof(PackedColor));
        }
    };

    #define MAKE_MASK(c0, c1, c2, c3) \
        ((c3 << 6) | (c2 << 4) | (c1 << 2) | c0)

    class Format
    {
    private:
        uint32 m_bits;
        uint32 m_type;
        PackedColor m_size;
        PackedColor m_offset;

        void validate();

    public:
        enum Component
        {
            RED   = 0,
            GREEN = 1,
            BLUE  = 2,
            ALPHA = 3
        };

        enum Order
        {
            R = MAKE_MASK(0, 1, 2, 3),
            G = MAKE_MASK(1, 0, 2, 3),
            B = MAKE_MASK(2, 1, 0, 3),
            A = MAKE_MASK(3, 1, 2, 0),

            RG = MAKE_MASK(0, 1, 2, 3),
            RB = MAKE_MASK(0, 2, 1, 3),
            RA = MAKE_MASK(0, 3, 2, 1),
            GR = MAKE_MASK(1, 0, 2, 3),
            GB = MAKE_MASK(1, 2, 0, 3),
            GA = MAKE_MASK(1, 3, 0, 2),
            BR = MAKE_MASK(2, 0, 1, 3),
            BG = MAKE_MASK(2, 1, 0, 3),
            BA = MAKE_MASK(2, 3, 0, 1),
            AR = MAKE_MASK(3, 0, 1, 2),
            AG = MAKE_MASK(3, 1, 0, 2),
            AB = MAKE_MASK(3, 2, 0, 1),

            RGB = MAKE_MASK(0, 1, 2, 3),
            RGA = MAKE_MASK(0, 1, 3, 2),
            RBG = MAKE_MASK(0, 2, 1, 3),
            RBA = MAKE_MASK(0, 2, 3, 1),
            RAG = MAKE_MASK(0, 3, 1, 2),
            RAB = MAKE_MASK(0, 3, 2, 1),
            GRB = MAKE_MASK(1, 0, 2, 3),
            GRA = MAKE_MASK(1, 0, 3, 2),
            GBR = MAKE_MASK(1, 2, 0, 3),
            GBA = MAKE_MASK(1, 2, 3, 0),
            GAR = MAKE_MASK(1, 3, 0, 2),
            GAB = MAKE_MASK(1, 3, 2, 0),
            BRG = MAKE_MASK(2, 0, 1, 3),
            BRA = MAKE_MASK(2, 0, 3, 1),
            BGR = MAKE_MASK(2, 1, 0, 3),
            BGA = MAKE_MASK(2, 1, 3, 0),
            BAR = MAKE_MASK(2, 3, 0, 1),
            BAG = MAKE_MASK(2, 3, 1, 0),
            ARG = MAKE_MASK(3, 0, 1, 2),
            ARB = MAKE_MASK(3, 0, 2, 1),
            AGR = MAKE_MASK(3, 1, 0, 2),
            AGB = MAKE_MASK(3, 1, 2, 0),
            ABR = MAKE_MASK(3, 2, 0, 1),
            ABG = MAKE_MASK(3, 2, 1, 0),

            RGBA = MAKE_MASK(0, 1, 2, 3),
            RGAB = MAKE_MASK(0, 1, 3, 2),
            RBGA = MAKE_MASK(0, 2, 1, 3),
            RBAG = MAKE_MASK(0, 2, 3, 1),
            RAGB = MAKE_MASK(0, 3, 1, 2),
            RABG = MAKE_MASK(0, 3, 2, 1),
            GRBA = MAKE_MASK(1, 0, 2, 3),
            GRAB = MAKE_MASK(1, 0, 3, 2),
            GBRA = MAKE_MASK(1, 2, 0, 3),
            GBAR = MAKE_MASK(1, 2, 3, 0),
            GARB = MAKE_MASK(1, 3, 0, 2),
            GABR = MAKE_MASK(1, 3, 2, 0),
            BRGA = MAKE_MASK(2, 0, 1, 3),
            BRAG = MAKE_MASK(2, 0, 3, 1),
            BGRA = MAKE_MASK(2, 1, 0, 3),
            BGAR = MAKE_MASK(2, 1, 3, 0),
            BARG = MAKE_MASK(2, 3, 3, 1),
            BAGR = MAKE_MASK(2, 3, 1, 0),
            ARGB = MAKE_MASK(3, 0, 1, 2),
            ARBG = MAKE_MASK(3, 0, 2, 1),
            AGRB = MAKE_MASK(3, 1, 0, 2),
            AGBR = MAKE_MASK(3, 1, 2, 0),
            ABRG = MAKE_MASK(3, 2, 0, 1),
            ABGR = MAKE_MASK(3, 2, 1, 0)
        };

        enum Type
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

        Format();
        Format(const Format& format);
        explicit Format(int bits, uint32 luminanceMask, uint32 alphaMask);
        explicit Format(int bits, uint32 redMask, uint32 greenMask, uint32 blueMask, uint32 alphaMask);
        explicit Format(int bits, Type type, Order order, int s0, int s1, int s2, int s3);
        explicit Format(int bits, Type type, const PackedColor& size, const PackedColor& offset);
        ~Format();

        const Format& operator = (const Format& format);

        bool operator == (const Format& format) const;
        bool operator != (const Format& format) const;
        bool operator < (const Format& format) const;

        int bits() const;
        int bytes() const;
        int float_bits() const;
        Type type() const;
        bool alpha() const;
        PackedColor size() const;
        PackedColor offset() const;
        int size(int component) const;
        int offset(int component) const;
        uint32 mask(int component) const;
        uint32 pack(float red, float green, float blue, float alpha) const;
    };

    #undef MAKE_MASK

    // ----------------------------------------------------------------------------
    // Format macros
    // ----------------------------------------------------------------------------

    #define MAKE_FORMAT(bits, type, order, s0, s1, s2, s3) \
        Format(bits, Format::type, Format::order, s0, s1, s2, s3)

    // NONE
    #define FORMAT_NONE                 Format()

    // UNORM
    #define FORMAT_B8G8R8               MAKE_FORMAT(24, UNORM, BGR,  8, 8, 8, 0)
    #define FORMAT_R8G8B8               MAKE_FORMAT(24, UNORM, RGB,  8, 8, 8, 0)
    #define FORMAT_B8G8R8A8             MAKE_FORMAT(32, UNORM, BGRA, 8, 8, 8, 8)
    #define FORMAT_B8G8R8X8             MAKE_FORMAT(32, UNORM, BGRA, 8, 8, 8, 0)
    #define FORMAT_R8G8B8A8             MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8)
    #define FORMAT_R8G8B8X8             MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 0)
    #define FORMAT_B5G6R5               MAKE_FORMAT(16, UNORM, BGR,  5, 6, 5, 0)
    #define FORMAT_B5G5R5X1             MAKE_FORMAT(16, UNORM, BGRA, 5, 5, 5, 0)
    #define FORMAT_B5G5R5A1             MAKE_FORMAT(16, UNORM, BGRA, 5, 5, 5, 1)
    #define FORMAT_B4G4R4A4             MAKE_FORMAT(16, UNORM, BGRA, 4, 4, 4, 4)
    #define FORMAT_B4G4R4X4             MAKE_FORMAT(16, UNORM, BGRA, 4, 4, 4, 0)
    #define FORMAT_B2G3R3               MAKE_FORMAT(8,  UNORM, BGR,  2, 3, 3, 0)
    #define FORMAT_B2G3R3A8             MAKE_FORMAT(16, UNORM, BGRA, 2, 3, 3, 8)
    #define FORMAT_R10G10B10A2          MAKE_FORMAT(32, UNORM, RGBA, 10, 10, 10, 2)
    #define FORMAT_B10G10R10A2          MAKE_FORMAT(32, UNORM, BGRA, 10, 10, 10, 2)
    #define FORMAT_R16G16               MAKE_FORMAT(32, UNORM, RG,   16, 16, 0, 0)
    #define FORMAT_R16G16B16A16         MAKE_FORMAT(64, UNORM, RGBA, 16, 16, 16, 16)
    #define FORMAT_A8                   MAKE_FORMAT(8,  UNORM, A,    8, 0, 0, 0)

    #define FORMAT_R16                  MAKE_FORMAT(16, UNORM, R,    16, 0, 0, 0)
    #define FORMAT_RG16                 MAKE_FORMAT(32, UNORM, RG,   16, 16, 0, 0)
    #define FORMAT_RGB16                MAKE_FORMAT(48, UNORM, RGB,  16, 16, 16, 0)
    #define FORMAT_RGBA16               MAKE_FORMAT(64, UNORM, RGBA, 16, 16, 16, 16)

    // UNORM luminance
    #define FORMAT_L8                   Format(8, 0xff, 0)
    #define FORMAT_L8A8                 Format(16, 0x00ff, 0xff00)
    #define FORMAT_L4A4                 Format(8, 0x0f, 0xf0)
    #define FORMAT_L16                  Format(16, 0xffff, 0)
    #define FORMAT_L16A16               Format(32, Format::UNORM, PackedColor(16, 16, 16, 16), PackedColor(0, 0, 0, 16))

    // FLOAT / HALF luminance
    #define FORMAT_L16F                 Format(16, Format::FP16, PackedColor(16, 16, 16, 0), PackedColor(0, 0, 0, 0))
    #define FORMAT_L32F                 Format(32, Format::FP32, PackedColor(32, 32, 32, 0), PackedColor(0, 0, 0, 0))

    // ----------------------------------------------------------------------------
    // OpenGL packed formats
    // ----------------------------------------------------------------------------

    // GL_RGB
    #define FORMAT_RGB_UNSIGNED_SHORT_5_6_5              MAKE_FORMAT(16, UNORM, BGR, 5, 6, 5, 0)
    #define FORMAT_RGB_UNSIGNED_SHORT_5_6_5_REV          MAKE_FORMAT(16, UNORM, RGB, 5, 6, 5, 0)
    #define FORMAT_RGB_UNSIGNED_BYTE_3_3_2               MAKE_FORMAT(8,  UNORM, BGR, 2, 3, 3, 0)
    #define FORMAT_RGB_UNSIGNED_BYTE_2_3_3_REV           MAKE_FORMAT(8,  UNORM, RGB, 3, 3, 2, 0)

    // GL_BGRA
    #define FORMAT_BGRA_UNSIGNED_INT_8_8_8_8             MAKE_FORMAT(32, UNORM, ARGB, 8, 8, 8, 8)
    #define FORMAT_BGRA_UNSIGNED_INT_8_8_8_8_REV         MAKE_FORMAT(32, UNORM, BGRA, 8, 8, 8, 8)
    #define FORMAT_BGRA_UNSIGNED_INT_10_10_10_2          MAKE_FORMAT(32, UNORM, ARGB, 2, 10, 10, 10)
    #define FORMAT_BGRA_UNSIGNED_INT_2_10_10_10_REV      MAKE_FORMAT(32, UNORM, BGRA, 10, 10, 10, 2)
    #define FORMAT_BGRA_UNSIGNED_SHORT_4_4_4_4           MAKE_FORMAT(16, UNORM, ARGB, 4, 4, 4, 4)
    #define FORMAT_BGRA_UNSIGNED_SHORT_4_4_4_4_REV       MAKE_FORMAT(16, UNORM, BGRA, 4, 4, 4, 4)
    #define FORMAT_BGRA_UNSIGNED_SHORT_5_5_5_1           MAKE_FORMAT(16, UNORM, ARGB, 1, 5, 5, 5)
    #define FORMAT_BGRA_UNSIGNED_SHORT_1_5_5_5_REV       MAKE_FORMAT(16, UNORM, BGRA, 5, 5, 5, 1)

    // GL_RGBA
    #define FORMAT_RGBA_UNSIGNED_INT_8_8_8_8             MAKE_FORMAT(32, UNORM, ABGR, 8, 8, 8, 8)
    #define FORMAT_RGBA_UNSIGNED_INT_8_8_8_8_REV         MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8)
    #define FORMAT_RGBA_UNSIGNED_INT_10_10_10_2          MAKE_FORMAT(32, UNORM, ABGR, 2, 10, 10, 10)
    #define FORMAT_RGBA_UNSIGNED_INT_2_10_10_10_REV      MAKE_FORMAT(32, UNORM, RGBA, 10, 10, 10, 2)
    #define FORMAT_RGBA_UNSIGNED_SHORT_4_4_4_4           MAKE_FORMAT(16, UNORM, ABGR, 4, 4, 4, 4)
    #define FORMAT_RGBA_UNSIGNED_SHORT_4_4_4_4_REV       MAKE_FORMAT(16, UNORM, RGBA, 4, 4, 4, 4)
    #define FORMAT_RGBA_UNSIGNED_SHORT_5_5_5_1           MAKE_FORMAT(16, UNORM, ABGR, 1, 5, 5, 5)
    #define FORMAT_RGBA_UNSIGNED_SHORT_1_5_5_5_REV       MAKE_FORMAT(16, UNORM, RGBA, 5, 5, 5, 1)

    // GL_ABGR_EXT
    #define FORMAT_ABGR_EXT_UNSIGNED_INT_8_8_8_8         MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8)
    #define FORMAT_ABGR_EXT_UNSIGNED_INT_8_8_8_8_REV     MAKE_FORMAT(32, UNORM, ABGR, 8, 8, 8, 8)
    #define FORMAT_ABGR_EXT_UNSIGNED_INT_10_10_10_2      MAKE_FORMAT(32, UNORM, RGBA, 2, 10, 10, 10)
    #define FORMAT_ABGR_EXT_UNSIGNED_INT_2_10_10_10_REV  MAKE_FORMAT(32, UNORM, ABGR, 10, 10, 10, 2)
    #define FORMAT_ABGR_EXT_UNSIGNED_SHORT_4_4_4_4       MAKE_FORMAT(16, UNORM, RGBA, 4, 4, 4, 4)
    #define FORMAT_ABGR_EXT_UNSIGNED_SHORT_4_4_4_4_REV   MAKE_FORMAT(16, UNORM, ABGR, 4, 4, 4,4)
    #define FORMAT_ABGR_EXT_UNSIGNED_SHORT_5_5_5_1       MAKE_FORMAT(16, UNORM, RGBA, 1, 5, 5, 5)
    #define FORMAT_ABGR_EXT_UNSIGNED_SHORT_1_5_5_5_REV   MAKE_FORMAT(16, UNORM, ABGR, 5, 5, 5, 1)

    // FLOAT / HALF
    #define FORMAT_RGBA16F                               MAKE_FORMAT(64,  FP16, RGBA, 16, 16, 16, 16)
    #define FORMAT_RGBA32F                               MAKE_FORMAT(128, FP32, RGBA, 32, 32, 32, 32)

} // namespace mango
