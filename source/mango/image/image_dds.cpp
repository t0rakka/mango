/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/system.hpp>
#include <mango/core/pointer.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

	// ------------------------------------------------------------
    // .dds information
	// ------------------------------------------------------------

    // Format information:
    // http://msdn.microsoft.com/en-us/library/windows/apps/jj651550.aspx
    // http://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx
    //
    // Data Conversion Rules:
    // http://msdn.microsoft.com/en-us/library/windows/desktop/dd607323%28v=vs.85%29.aspx
    //
    // BC6H compression:
    // http://msdn.microsoft.com/en-us/library/windows/desktop/hh308952(v=vs.85).aspx

	// ------------------------------------------------------------
	// FOURCC
	// ------------------------------------------------------------

    enum
    {
        // chunks
        FOURCC_DDS          = u32_mask('D', 'D', 'S', ' '),
        FOURCC_DX10         = u32_mask('D', 'X', '1', '0'),

        // unorm
        FOURCC_R8G8B8       = 20,
        FOURCC_A8R8G8B8     = 21,
        FOURCC_X8R8G8B8     = 22,
        FOURCC_R5G6B5       = 23,
        FOURCC_X1R5G5B5     = 24,
        FOURCC_A1R5G5B5     = 25,
        FOURCC_A4R4G4B4     = 26,
        FOURCC_R3G3B2       = 27,
        FOURCC_A8           = 28,
        FOURCC_A8R3G3B2     = 29,
        FOURCC_X4R4G4B4     = 30,
        FOURCC_A2B10G10R10  = 31,
        FOURCC_A8B8G8R8     = 32,
        FOURCC_X8B8G8R8     = 33,
        FOURCC_G16R16       = 34,
        FOURCC_A2R10G10B10  = 35,
        FOURCC_A16B16G16R16 = 36,
        FOURCC_L8           = 50,
        FOURCC_A8L8         = 51,
        FOURCC_A4L4         = 52,

        // half
        FOURCC_R16F         = 111,
        FOURCC_GR16F        = 112,
        FOURCC_ABGR16F      = 113,

        // float
        FOURCC_R32F         = 114,
        FOURCC_GR32F        = 115,
        FOURCC_ABGR32F      = 116,
    };

	// ------------------------------------------------------------
	// DXGI / DX10
	// ------------------------------------------------------------

    enum : u32
    {
        DXGI_FORMAT_UNKNOWN                     = 0,
        DXGI_FORMAT_R32G32B32A32_TYPELESS       = 1,
        DXGI_FORMAT_R32G32B32A32_FLOAT          = 2,
        DXGI_FORMAT_R32G32B32A32_UINT           = 3,
        DXGI_FORMAT_R32G32B32A32_SINT           = 4,
        DXGI_FORMAT_R32G32B32_TYPELESS          = 5,
        DXGI_FORMAT_R32G32B32_FLOAT             = 6,
        DXGI_FORMAT_R32G32B32_UINT              = 7,
        DXGI_FORMAT_R32G32B32_SINT              = 8,
        DXGI_FORMAT_R16G16B16A16_TYPELESS       = 9,
        DXGI_FORMAT_R16G16B16A16_FLOAT          = 10,
        DXGI_FORMAT_R16G16B16A16_UNORM          = 11,
        DXGI_FORMAT_R16G16B16A16_UINT           = 12,
        DXGI_FORMAT_R16G16B16A16_SNORM          = 13,
        DXGI_FORMAT_R16G16B16A16_SINT           = 14,
        DXGI_FORMAT_R32G32_TYPELESS             = 15,
        DXGI_FORMAT_R32G32_FLOAT                = 16,
        DXGI_FORMAT_R32G32_UINT                 = 17,
        DXGI_FORMAT_R32G32_SINT                 = 18,
        DXGI_FORMAT_R32G8X24_TYPELESS           = 19,
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
        DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
        DXGI_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
        DXGI_FORMAT_R10G10B10A2_TYPELESS        = 23,
        DXGI_FORMAT_R10G10B10A2_UNORM           = 24,
        DXGI_FORMAT_R10G10B10A2_UINT            = 25,
        DXGI_FORMAT_R11G11B10_FLOAT             = 26,
        DXGI_FORMAT_R8G8B8A8_TYPELESS           = 27,
        DXGI_FORMAT_R8G8B8A8_UNORM              = 28,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
        DXGI_FORMAT_R8G8B8A8_UINT               = 30,
        DXGI_FORMAT_R8G8B8A8_SNORM              = 31,
        DXGI_FORMAT_R8G8B8A8_SINT               = 32,
        DXGI_FORMAT_R16G16_TYPELESS             = 33,
        DXGI_FORMAT_R16G16_FLOAT                = 34,
        DXGI_FORMAT_R16G16_UNORM                = 35,
        DXGI_FORMAT_R16G16_UINT                 = 36,
        DXGI_FORMAT_R16G16_SNORM                = 37,
        DXGI_FORMAT_R16G16_SINT                 = 38,
        DXGI_FORMAT_R32_TYPELESS                = 39,
        DXGI_FORMAT_D32_FLOAT                   = 40,
        DXGI_FORMAT_R32_FLOAT                   = 41,
        DXGI_FORMAT_R32_UINT                    = 42,
        DXGI_FORMAT_R32_SINT                    = 43,
        DXGI_FORMAT_R24G8_TYPELESS              = 44,
        DXGI_FORMAT_D24_UNORM_S8_UINT           = 45,
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
        DXGI_FORMAT_X24_TYPELESS_G8_UINT        = 47,
        DXGI_FORMAT_R8G8_TYPELESS               = 48,
        DXGI_FORMAT_R8G8_UNORM                  = 49,
        DXGI_FORMAT_R8G8_UINT                   = 50,
        DXGI_FORMAT_R8G8_SNORM                  = 51,
        DXGI_FORMAT_R8G8_SINT                   = 52,
        DXGI_FORMAT_R16_TYPELESS                = 53,
        DXGI_FORMAT_R16_FLOAT                   = 54,
        DXGI_FORMAT_D16_UNORM                   = 55,
        DXGI_FORMAT_R16_UNORM                   = 56,
        DXGI_FORMAT_R16_UINT                    = 57,
        DXGI_FORMAT_R16_SNORM                   = 58,
        DXGI_FORMAT_R16_SINT                    = 59,
        DXGI_FORMAT_R8_TYPELESS                 = 60,
        DXGI_FORMAT_R8_UNORM                    = 61,
        DXGI_FORMAT_R8_UINT                     = 62,
        DXGI_FORMAT_R8_SNORM                    = 63,
        DXGI_FORMAT_R8_SINT                     = 64,
        DXGI_FORMAT_A8_UNORM                    = 65,
        DXGI_FORMAT_R1_UNORM                    = 66,
        DXGI_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
        DXGI_FORMAT_R8G8_B8G8_UNORM             = 68,
        DXGI_FORMAT_G8R8_G8B8_UNORM             = 69,
        DXGI_FORMAT_BC1_TYPELESS                = 70,
        DXGI_FORMAT_BC1_UNORM                   = 71,
        DXGI_FORMAT_BC1_UNORM_SRGB              = 72,
        DXGI_FORMAT_BC2_TYPELESS                = 73,
        DXGI_FORMAT_BC2_UNORM                   = 74,
        DXGI_FORMAT_BC2_UNORM_SRGB              = 75,
        DXGI_FORMAT_BC3_TYPELESS                = 76,
        DXGI_FORMAT_BC3_UNORM                   = 77,
        DXGI_FORMAT_BC3_UNORM_SRGB              = 78,
        DXGI_FORMAT_BC4_TYPELESS                = 79,
        DXGI_FORMAT_BC4_UNORM                   = 80,
        DXGI_FORMAT_BC4_SNORM                   = 81,
        DXGI_FORMAT_BC5_TYPELESS                = 82,
        DXGI_FORMAT_BC5_UNORM                   = 83,
        DXGI_FORMAT_BC5_SNORM                   = 84,
        DXGI_FORMAT_B5G6R5_UNORM                = 85,
        DXGI_FORMAT_B5G5R5A1_UNORM              = 86,
        DXGI_FORMAT_B8G8R8A8_UNORM              = 87,
        DXGI_FORMAT_B8G8R8X8_UNORM              = 88,
        DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
        DXGI_FORMAT_B8G8R8A8_TYPELESS           = 90,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
        DXGI_FORMAT_B8G8R8X8_TYPELESS           = 92,
        DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
        DXGI_FORMAT_BC6H_TYPELESS               = 94,
        DXGI_FORMAT_BC6H_UF16                   = 95,
        DXGI_FORMAT_BC6H_SF16                   = 96,
        DXGI_FORMAT_BC7_TYPELESS                = 97,
        DXGI_FORMAT_BC7_UNORM                   = 98,
        DXGI_FORMAT_BC7_UNORM_SRGB              = 99,
        DXGI_FORMAT_AYUV                        = 100,
        DXGI_FORMAT_Y410                        = 101,
        DXGI_FORMAT_Y416                        = 102,
        DXGI_FORMAT_NV12                        = 103,
        DXGI_FORMAT_P010                        = 104,
        DXGI_FORMAT_P016                        = 105,
        DXGI_FORMAT_420_OPAQUE                  = 106,
        DXGI_FORMAT_YUY2                        = 107,
        DXGI_FORMAT_Y210                        = 108,
        DXGI_FORMAT_Y216                        = 109,
        DXGI_FORMAT_NV11                        = 110,
        DXGI_FORMAT_AI44                        = 111,
        DXGI_FORMAT_IA44                        = 112,
        DXGI_FORMAT_P8                          = 113,
        DXGI_FORMAT_A8P8                        = 114,
        DXGI_FORMAT_B4G4R4A4_UNORM              = 115,

        DXGI_FORMAT_P208                        = 130,
        DXGI_FORMAT_V208                        = 131,
        DXGI_FORMAT_V408                        = 132,
        DXGI_FORMAT_ASTC_4X4_TYPELESS           = 133,
        DXGI_FORMAT_ASTC_4X4_UNORM              = 134,
        DXGI_FORMAT_ASTC_4X4_UNORM_SRGB         = 135,
        DXGI_FORMAT_ASTC_5X4_TYPELESS           = 137,
        DXGI_FORMAT_ASTC_5X4_UNORM              = 138,
        DXGI_FORMAT_ASTC_5X4_UNORM_SRGB         = 139,
        DXGI_FORMAT_ASTC_5X5_TYPELESS           = 141,
        DXGI_FORMAT_ASTC_5X5_UNORM              = 142,
        DXGI_FORMAT_ASTC_5X5_UNORM_SRGB         = 143,
        DXGI_FORMAT_ASTC_6X5_TYPELESS           = 145,
        DXGI_FORMAT_ASTC_6X5_UNORM              = 146,
        DXGI_FORMAT_ASTC_6X5_UNORM_SRGB         = 147,
        DXGI_FORMAT_ASTC_6X6_TYPELESS           = 149,
        DXGI_FORMAT_ASTC_6X6_UNORM              = 150,
        DXGI_FORMAT_ASTC_6X6_UNORM_SRGB         = 151,
        DXGI_FORMAT_ASTC_8X5_TYPELESS           = 153,
        DXGI_FORMAT_ASTC_8X5_UNORM              = 154,
        DXGI_FORMAT_ASTC_8X5_UNORM_SRGB         = 155,
        DXGI_FORMAT_ASTC_8X6_TYPELESS           = 157,
        DXGI_FORMAT_ASTC_8X6_UNORM              = 158,
        DXGI_FORMAT_ASTC_8X6_UNORM_SRGB         = 159,
        DXGI_FORMAT_ASTC_8X8_TYPELESS           = 161,
        DXGI_FORMAT_ASTC_8X8_UNORM              = 162,
        DXGI_FORMAT_ASTC_8X8_UNORM_SRGB         = 163,
        DXGI_FORMAT_ASTC_10X5_TYPELESS          = 165,
        DXGI_FORMAT_ASTC_10X5_UNORM             = 166,
        DXGI_FORMAT_ASTC_10X5_UNORM_SRGB        = 167,
        DXGI_FORMAT_ASTC_10X6_TYPELESS          = 169,
        DXGI_FORMAT_ASTC_10X6_UNORM             = 170,
        DXGI_FORMAT_ASTC_10X6_UNORM_SRGB        = 171,
        DXGI_FORMAT_ASTC_10X8_TYPELESS          = 173,
        DXGI_FORMAT_ASTC_10X8_UNORM             = 174,
        DXGI_FORMAT_ASTC_10X8_UNORM_SRGB        = 175,
        DXGI_FORMAT_ASTC_10X10_TYPELESS         = 177,
        DXGI_FORMAT_ASTC_10X10_UNORM            = 178,
        DXGI_FORMAT_ASTC_10X10_UNORM_SRGB       = 179,
        DXGI_FORMAT_ASTC_12X10_TYPELESS         = 181,
        DXGI_FORMAT_ASTC_12X10_UNORM            = 182,
        DXGI_FORMAT_ASTC_12X10_UNORM_SRGB       = 183,
        DXGI_FORMAT_ASTC_12X12_TYPELESS         = 185,
        DXGI_FORMAT_ASTC_12X12_UNORM            = 186,
        DXGI_FORMAT_ASTC_12X12_UNORM_SRGB       = 187,
    };

    struct FormatDXGI
    {
        u32 fourcc;
        Format format;
        bool srgb; // NOTE: this is not used anywhere yet
        const char* name;
    };

#define MAKE_FORMAT(bits, type, order, s0, s1, s2, s3) \
    Format(bits, Format::type, Format::order, s0, s1, s2, s3)

    const FormatDXGI g_dxgi_table[] =
    {
        { 0, Format(), false, "UNKNOWN" },
        { 0, MAKE_FORMAT(128, NONE, RGBA, 32, 32, 32, 32), false, "R32G32B32A32_TYPELESS" },
        { 0, MAKE_FORMAT(128, FLOAT32, RGBA, 32, 32, 32, 32), false, "R32G32B32A32_FLOAT" },
        { 0, MAKE_FORMAT(128, UINT, RGBA, 32, 32, 32, 32), false, "R32G32B32A32_UINT" },
        { 0, MAKE_FORMAT(128, SINT, RGBA, 32, 32, 32, 32), false, "R32G32B32A32_SINT" },
        { 0, MAKE_FORMAT(96, NONE, RGB, 32, 32, 32, 0), false, "R32G32B32_TYPELESS" },
        { 0, MAKE_FORMAT(96, FLOAT32, RGB, 32, 32, 32, 0), false, "R32G32B32_FLOAT" },
        { 0, MAKE_FORMAT(96, UINT, RGB, 32, 32, 32, 0), false, "R32G32B32_UINT" },
        { 0, MAKE_FORMAT(96, SINT, RGB, 32, 32, 32, 0), false, "R32G32B32_SINT" },
        { 0, MAKE_FORMAT(64, NONE, RGBA, 16, 16, 16, 16), false, "R16G16B16A16_TYPELESS" },
        { 0, MAKE_FORMAT(64, FLOAT16, RGBA, 16, 16, 16, 16), false, "R16G16B16A16_FLOAT" },
        { 0, MAKE_FORMAT(64, UNORM, RGBA, 16, 16, 16, 16), false, "R16G16B16A16_UNORM" },
        { 0, MAKE_FORMAT(64, UINT, RGBA, 16, 16, 16, 16), false, "R16G16B16A16_UINT" },
        { 0, MAKE_FORMAT(64, SNORM, RGBA, 16, 16, 16, 16), false, "R16G16B16A16_SNORM" },
        { 0, MAKE_FORMAT(64, SINT, RGBA, 16, 16, 16, 16), false, "R16G16B16A16_SINT" },
        { 0, MAKE_FORMAT(64, NONE, RG, 32, 32, 0, 0), false, "R32G32_TYPELESS" },
        { 0, MAKE_FORMAT(64, FLOAT32, RG, 32, 32, 0, 0), false, "R32G32_FLOAT" },
        { 0, MAKE_FORMAT(64, UINT, RG, 32, 32, 0, 0), false, "R32G32_UINT" },
        { 0, MAKE_FORMAT(64, SINT, RG, 32, 32, 0, 0), false, "R32G32_SINT" },
        { 0, Format(), false, "R32G8X24_TYPELESS" }, // not supported
        { 0, Format(), false, "D32_FLOAT_S8X24_UINT" }, // not supported
        { 0, Format(), false, "R32_FLOAT_X8X24_TYPELESS" }, // not supported
        { 0, Format(), false, "X32_TYPELESS_G8X24_UINT" }, // not supported
        { 0, MAKE_FORMAT(32, NONE, RGBA, 10, 10, 10, 2), false, "R10G10B10A2_TYPELESS" },
        { 0, MAKE_FORMAT(32, UNORM, RGBA, 10, 10, 10, 2), false, "R10G10B10A2_UNORM" },
        { 0, MAKE_FORMAT(32, UINT, RGBA, 10, 10, 10, 2), false, "R10G10B10A2_UINT" },
        { 0, Format(), false, "R11G11B10_FLOAT" }, // not supported
        { 0, MAKE_FORMAT(32, NONE, RGBA, 8, 8, 8, 8), false, "R8G8B8A8_TYPELESS" },
        { 0, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), false, "R8G8B8A8_UNORM" },
        { 0, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), true, "R8G8B8A8_UNORM_SRGB" },
        { 0, MAKE_FORMAT(32, UINT, RGBA, 8, 8, 8, 8), false, "R8G8B8A8_UINT" },
        { 0, MAKE_FORMAT(32, SNORM, RGBA, 8, 8, 8, 8), false, "R8G8B8A8_SNORM" },
        { 0, MAKE_FORMAT(32, SINT, RGBA, 8, 8, 8, 8), false, "R8G8B8A8_SINT" },
        { 0, MAKE_FORMAT(32, NONE, RG, 16, 16, 0, 0), false, "R16G16_TYPELESS" },
        { 0, MAKE_FORMAT(32, FLOAT16, RG, 16, 16, 0, 0), false, "R16G16_FLOAT" },
        { 0, MAKE_FORMAT(32, UNORM, RG, 16, 16, 0, 0), false, "R16G16_UNORM" },
        { 0, MAKE_FORMAT(32, UINT, RG, 16, 16, 0, 0), false, "R16G16_UINT" },
        { 0, MAKE_FORMAT(32, SNORM, RG, 16, 16, 0, 0), false, "R16G16_SNORM" },
        { 0, MAKE_FORMAT(32, SINT, RG, 16, 16, 0, 0), false, "R16G16_SINT" },
        { 0, MAKE_FORMAT(32, NONE, R, 32, 0, 0, 0), false, "R32_TYPELESS" },
        { 0, Format(), false, "D32_FLOAT" }, // not supported
        { 0, MAKE_FORMAT(32, FLOAT32, R, 32, 0, 0, 0), false, "R32_FLOAT" },
        { 0, MAKE_FORMAT(32, UINT, R, 32, 0, 0, 0), false, "R32_UINT" },
        { 0, MAKE_FORMAT(32, SINT, R, 32, 0, 0, 0), false, "R32_SINT" },
        { 0, MAKE_FORMAT(32, NONE, RG, 24, 8, 0, 0), false, "R24G8_TYPELESS" },
        { 0, Format(), false, "D24_UNORM_S8_UINT" }, // not supported
        { 0, Format(), false, "R24_UNORM_X8_TYPELESS" }, // not supported
        { 0, Format(), false, "X24_TYPELESS_G8_UINT" }, // not supported
        { 0, MAKE_FORMAT(16, NONE, RG, 8, 8, 0, 0), false, "R8G8_TYPELESS" },
        { 0, MAKE_FORMAT(16, UNORM, RG, 8, 8, 0, 0), false, "R8G8_UNORM" },
        { 0, MAKE_FORMAT(16, UINT, RG, 8, 8, 0, 0), false, "R8G8_UINT" },
        { 0, MAKE_FORMAT(16, SNORM, RG, 8, 8, 0, 0), false, "R8G8_SNORM" },
        { 0, MAKE_FORMAT(16, SINT, RG, 8, 8, 0, 0), false, "R8G8_SINT" },
        { 0, MAKE_FORMAT(16, NONE, R, 16, 0, 0, 0), false, "R16_TYPELESS" },
        { 0, MAKE_FORMAT(16, FLOAT16, R, 16, 0, 0, 0), false, "R16_FLOAT" },
        { 0, Format(), false, "D16_UNORM" }, // not supported
        { 0, MAKE_FORMAT(16, UNORM, R, 16, 0, 0, 0), false, "R16_UNORM" },
        { 0, MAKE_FORMAT(16, UINT, R, 16, 0, 0, 0), false, "R16_UINT" },
        { 0, MAKE_FORMAT(16, SNORM, R, 16, 0, 0, 0), false, "R16_SNORM" },
        { 0, MAKE_FORMAT(16, SINT, R, 16, 0, 0, 0), false, "R16_SINT" },
        { 0, MAKE_FORMAT(8, NONE, R, 8, 0, 0, 0), false, "R8_TYPELESS" },
        { 0, MAKE_FORMAT(8, UNORM, R, 8, 0, 0, 0), false, "R8_UNORM" },
        { 0, MAKE_FORMAT(8, UINT, R, 8, 0, 0, 0), false, "R8_UINT" },
        { 0, MAKE_FORMAT(8, SNORM, R, 8, 0, 0, 0), false, "R8_SNORM" },
        { 0, MAKE_FORMAT(8, SINT, R, 8, 0, 0, 0), false, "R8_SINT" },
        { 0, MAKE_FORMAT(8, UNORM, A, 8, 0, 0, 0), false, "A8_UNORM" },
        { 0, Format(), false, "R1_UNORM" }, // not supported
        { 0, Format(), false, "R9G9B9E5_SHAREDEXP" }, // not supported
        { 0, Format(), false, "R8G8_B8G8_UNORM" },  // not supported: could be handled by FOURCC
        { 0, Format(), false, "G8R8_G8B8_UNORM" },  // not supported: could be handled by FOURCC
        { FOURCC_DXT1, Format(), false, "BC1_TYPELESS" },
        { FOURCC_DXT1, Format(), false, "BC1_UNORM" },
        { FOURCC_DXT1, Format(), true, "BC1_UNORM_SRGB" },
        { FOURCC_DXT3, Format(), false, "BC2_TYPELESS" },
        { FOURCC_DXT3, Format(), false, "BC2_UNORM" },
        { FOURCC_DXT3, Format(), true, "BC2_UNORM_SRGB" },
        { FOURCC_DXT5, Format(), false, "BC3_TYPELESS" },
        { FOURCC_DXT5, Format(), false, "BC3_UNORM" },
        { FOURCC_DXT5, Format(), true, "BC3_UNORM_SRGB" },
        { FOURCC_BC4U, Format(), false, "BC4_TYPELESS" },
        { FOURCC_BC4U, Format(), false, "BC4_UNORM" },
        { FOURCC_BC4S, Format(), false, "BC4_SNORM" },
        { FOURCC_BC5U, Format(), false, "BC5_TYPELESS" },
        { FOURCC_BC5U, Format(), false, "BC5_UNORM" },
        { FOURCC_BC5S, Format(), false, "BC5_SNORM" },
        { 0, MAKE_FORMAT(16, UNORM, BGR, 5, 6, 5, 0), false, "B5G6R5_UNORM" },
        { 0, MAKE_FORMAT(16, UNORM, BGRA, 5, 5, 5, 1), false, "B5G5R5A1_UNORM" },
        { 0, MAKE_FORMAT(32, UNORM, BGRA, 8, 8, 8, 8), false, "B8G8R8A8_UNORM" },
        { 0, MAKE_FORMAT(32, UNORM, BGRA, 8, 8, 8, 0), false, "B8G8R8X8_UNORM" },
        { 0, Format(), false, "R10G10B10_XR_BIAS_A2_UNORM" }, // not supported
        { 0, MAKE_FORMAT(32, NONE, BGRA, 8, 8, 8, 8), false, "B8G8R8A8_TYPELESS" },
        { 0, MAKE_FORMAT(32, UNORM, BGRA, 8, 8, 8, 8), true, "B8G8R8A8_UNORM_SRGB" },
        { 0, MAKE_FORMAT(32, NONE, BGRA, 8, 8, 8, 0), false, "B8G8R8X8_TYPELESS" },
        { 0, MAKE_FORMAT(32, UNORM, BGRA, 8, 8, 8, 0), true, "B8G8R8X8_UNORM_SRGB" },
        { 0, Format(), false, "BC6H_TYPELESS" }, // not supported
        { 0, Format(), false, "BC6H_UF16" }, // not supported
        { 0, Format(), false, "BC6H_SF16" }, // not supported
        { 0, Format(), false, "BC7_TYPELESS" }, // not supported
        { 0, Format(), false, "BC7_UNORM" }, // not supported
        { 0, Format(), true, "BC7_UNORM_SRGB" }, // not supported
        { 0, Format(), false, "AYUV" }, // not supported
        { 0, Format(), false, "Y410" }, // not supported
        { 0, Format(), false, "Y416" }, // not supported
        { 0, Format(), false, "NV12" }, // not supported
        { 0, Format(), false, "P010" }, // not supported
        { 0, Format(), false, "P016" }, // not supported
        { 0, Format(), false, "420_OPAQUE" }, // not supported
        { FOURCC_YUY2, Format(), false, "YUY2" },
        { 0, Format(), false, "Y210" }, // not supported
        { 0, Format(), false, "Y216" }, // not supported
        { 0, Format(), false, "NV11" }, // not supported
        { 0, Format(), false, "AI44" }, // not supported
        { 0, Format(), false, "IA44" }, // not supported
        { 0, Format(), false, "P8" }, // not supported
        { 0, Format(), false, "A8P8" }, // not supported
        { 0, MAKE_FORMAT(16, UNORM, BGRA, 4, 4, 4, 4), false, "B4G4R4A4_UNORM" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "UNKNOWN" },
        { 0, Format(), false, "P208" }, // not supported
        { 0, Format(), false, "V208" }, // not supported
        { 0, Format(), false, "V408" }, // not supported
        { FOURCC_ASTC4x4, Format(), false, "ASTC_4X4_TYPELESS" },
        { FOURCC_ASTC4x4, Format(), false, "ASTC_4X4_UNORM" },
        { FOURCC_ASTC4x4, Format(), true,  "ASTC_4X4_UNORM_SRGB" },
        { 0, Format(), false, "ASTC_5X4_TYPELESS" },
        { 0, Format(), false, "ASTC_5X4_UNORM" },
        { 0, Format(), true,  "ASTC_5X4_UNORM_SRGB" },
        { FOURCC_ASTC5x5, Format(), false, "ASTC_5X5_TYPELESS" },
        { FOURCC_ASTC5x5, Format(), false, "ASTC_5X5_UNORM" },
        { FOURCC_ASTC5x5, Format(), true,  "ASTC_5X5_UNORM_SRGB" },
        { 0, Format(), false, "ASTC_6X5_TYPELESS" },
        { 0, Format(), false, "ASTC_6X5_UNORM" },
        { 0, Format(), true,  "ASTC_6X5_UNORM_SRGB" },
        { FOURCC_ASTC6x6, Format(), false, "ASTC_6X6_TYPELESS" },
        { FOURCC_ASTC6x6, Format(), false, "ASTC_6X6_UNORM" },
        { FOURCC_ASTC6x6, Format(), true,  "ASTC_6X6_UNORM_SRGB" },
        { FOURCC_ASTC8x5, Format(), false, "ASTC_8X5_TYPELESS" },
        { FOURCC_ASTC8x5, Format(), false, "ASTC_8X5_UNORM" },
        { FOURCC_ASTC8x5, Format(), true,  "ASTC_8X5_UNORM_SRGB" },
        { FOURCC_ASTC8x6, Format(), false, "ASTC_8X6_TYPELESS" },
        { FOURCC_ASTC8x6, Format(), false, "ASTC_8X6_UNORM" },
        { FOURCC_ASTC8x6, Format(), true,  "ASTC_8X6_UNORM_SRGB" },
        { 0, Format(), false, "ASTC_8X8_TYPELESS" },
        { 0, Format(), false, "ASTC_8X8_UNORM" },
        { 0, Format(), true,  "ASTC_8X8_UNORM_SRGB" },
        { FOURCC_ASTC10x5, Format(), false, "ASTC_10X5_TYPELESS" },
        { FOURCC_ASTC10x5, Format(), false, "ASTC_10X5_UNORM" },
        { FOURCC_ASTC10x5, Format(), true,  "ASTC_10X5_UNORM_SRGB" },
        { 0, Format(), false, "ASTC_10X6_TYPELESS" },
        { 0, Format(), false, "ASTC_10X6_UNORM" },
        { 0, Format(), true,  "ASTC_10X6_UNORM_SRGB" },
        { 0, Format(), false, "ASTC_10X8_TYPELESS" },
        { 0, Format(), false, "ASTC_10X8_UNORM" },
        { 0, Format(), true,  "ASTC_10X8_UNORM_SRGB" },
        { 0, Format(), false, "ASTC_10X10_TYPELESS" },
        { 0, Format(), false, "ASTC_10X10_UNORM" },
        { 0, Format(), true,  "ASTC_10X10_UNORM_SRGB" },
        { 0, Format(), false, "ASTC_12X10_TYPELESS" },
        { 0, Format(), false, "ASTC_12X10_UNORM" },
        { 0, Format(), true,  "ASTC_12X10_UNORM_SRGB" },
        { 0, Format(), false, "ASTC_12X12_TYPELESS" },
        { 0, Format(), false, "ASTC_12X12_UNORM" },
        { 0, Format(), true,  "ASTC_12X12_UNORM_SRGB" },
    };

    const int g_dxgi_table_size = sizeof(g_dxgi_table) / sizeof(g_dxgi_table[0]);

    struct HeaderDX10
    {
        u32 dxgiFormat;
        u32 resourceDimension;
        u32 miscFlag;
        u32 arraySize;
        u32 reserved;

        const u8* read(LittleEndianConstPointer p)
        {
            dxgiFormat = p.read32();
            resourceDimension = p.read32();
            miscFlag = p.read32();
            arraySize = p.read32();
            reserved = p.read32();
            return p;
        }
    };

    TextureCompression fourcc_to_compression(u32 fourcc)
    {
        TextureCompression compression = TextureCompression::NONE;

        switch (fourcc)
        {
            case FOURCC_DXT1:
            case FOURCC_DXT2:
                compression = TextureCompression::DXT1;
                break;
            case FOURCC_DXT3:
            case FOURCC_DXT4:
                compression = TextureCompression::DXT3;
                break;
            case FOURCC_DXT5:
                compression = TextureCompression::DXT5;
                break;
			case FOURCC_ATI1:
			case FOURCC_AT1N:
            case FOURCC_3DC1:
            case FOURCC_BC4U:
				compression = TextureCompression::RGTC1_RED;
                break;
            case FOURCC_BC4S:
                compression = TextureCompression::RGTC1_SIGNED_RED;
                break;
			case FOURCC_ATI2:
			case FOURCC_AT2N:
            case FOURCC_3DC2:
            case FOURCC_BC5U:
				compression = TextureCompression::RGTC2_RG;
                break;
            case FOURCC_BC5S:
                compression = TextureCompression::RGTC2_SIGNED_RG;
                break;
            case FOURCC_BC6H:
                compression = TextureCompression::BPTC_RGB_UNSIGNED_FLOAT;
                break;
            case FOURCC_BC7U:
                compression = TextureCompression::BPTC_RGBA_UNORM;
                break;
            case FOURCC_PTC1:
                compression = TextureCompression::PVRTC_RGB_2BPP;
                break;
            case FOURCC_PTC2:
                compression = TextureCompression::PVRTC_RGBA_2BPP;
                break;
            case FOURCC_PTC3:
                compression = TextureCompression::PVRTC_RGB_4BPP;
                break;
            case FOURCC_PTC4:
                compression = TextureCompression::PVRTC_RGBA_4BPP;
                break;
            case FOURCC_UYVY:
                compression = TextureCompression::UYVY;
                break;
            case FOURCC_YUY2:
                compression = TextureCompression::YUY2;
                break;
            case FOURCC_G8R8G8B8:
                compression = TextureCompression::G8R8G8B8;
                break;
            case FOURCC_R8G8B8G8:
                compression = TextureCompression::R8G8B8G8;
                break;
        }

        return compression;
    }

	// ------------------------------------------------------------
	// DDS
	// ------------------------------------------------------------

    enum
    {
        DDSD_CAPS           = 0x00000001,
        DDSD_HEIGHT         = 0x00000002,
        DDSD_WIDTH          = 0x00000004,
        DDSD_PITCH          = 0x00000008,
        DDSD_PIXELFORMAT    = 0x00001000,
        DDSD_MIPMAPCOUNT    = 0x00020000,
        DDSD_LINEARSIZE     = 0x00080000,
        DDSD_DEPTH          = 0x00800000
    };

    enum
    {
        DDPF_ALPHAPIXELS    = 0x00000001,
        DDPF_ALPHA          = 0x00000002,
        DDPF_FOURCC         = 0x00000004,
        DDPF_PALETTE        = 0x00000020,
        DDPF_RGB            = 0x00000040,
        DDPF_YUV            = 0x00000200,
        DDPF_LUMINANCE      = 0x00020000
    };

    enum
    {
        DDSCAPS_COMPLEX     = 0x00000008,
        DDSCAPS_TEXTURE     = 0x00001000,
        DDSCAPS_MIPMAP      = 0x00400000
    };

    enum
    {
        DDSCAPS2_CUBEMAP            = 0x00000200,
        DDSCAPS2_CUBEMAP_POSITIVEX  = 0x00000400,
        DDSCAPS2_CUBEMAP_NEGATIVEX  = 0x00000800,
        DDSCAPS2_CUBEMAP_POSITIVEY  = 0x00001000,
        DDSCAPS2_CUBEMAP_NEGATIVEY  = 0x00002000,
        DDSCAPS2_CUBEMAP_POSITIVEZ  = 0x00004000,
        DDSCAPS2_CUBEMAP_NEGATIVEZ  = 0x00008000,
        DDSCAPS2_CUBEMAP_ALLFACES   = 0x0000fc00,
        DDSCAPS2_VOLUME             = 0x00200000
    };

    struct FormatDDS
    {
        u32 size;
        u32 flags;
        u32 fourCC;
        u32 rgbBitCount;
        u32 rBitMask;
        u32 gBitMask;
        u32 bBitMask;
        u32 aBitMask;

        Format format;
        TextureCompression compression;

        const char* error = nullptr;

        void processFourCC()
        {
            char temp[4];
            ustore32(temp, fourCC);
            debugPrint(".dds fourcc: %c%c%c%c\n", temp[0], temp[1], temp[2], temp[3]);

            bool preserve_fourcc = false;

            switch (fourCC)
            {
                case FOURCC_DX10:
					format = Format();
					compression = TextureCompression::NONE;
                    preserve_fourcc = true;
					break;

                case FOURCC_R8G8B8:
                    format = Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0);
                    break;
                case FOURCC_A8R8G8B8:
                    format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
                    break;
                case FOURCC_X8R8G8B8:
                    format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 0);
                    break;
                case FOURCC_R5G6B5:
                    format = Format(16, Format::UNORM, Format::BGR, 5, 6, 5, 0);
                    break;
                case FOURCC_X1R5G5B5:
                    format = Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 0);
                    break;
                case FOURCC_A1R5G5B5:
                    format = Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 1);
                    break;
                case FOURCC_A4R4G4B4:
                    format = Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4);
                    break;
                case FOURCC_R3G3B2:
                    format = Format(8, Format::UNORM, Format::BGR, 2, 3, 3, 0);
                    break;
                case FOURCC_A8:
                    format = Format(8, Format::UNORM, Format::A, 0, 0, 0, 8);
                    break;
                case FOURCC_A8R3G3B2:
                    format = Format(16, Format::UNORM, Format::BGRA, 2, 3, 3, 8);
                    break;
                case FOURCC_X4R4G4B4:
                    format = Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 0);
                    break;
                case FOURCC_A2B10G10R10:
                    format = Format(32, Format::UNORM, Format::RGBA, 10, 10, 10, 2);
                    break;
                case FOURCC_A8B8G8R8:
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    break;
                case FOURCC_X8B8G8R8:
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 0);
                    break;
                case FOURCC_G16R16:
                    format = Format(32, Format::UNORM, Format::RG, 16, 16, 0, 0);
                    break;
                case FOURCC_A2R10G10B10:
                    format = Format(32, Format::UNORM, Format::BGRA, 10, 10, 10, 2);
                    break;
                case FOURCC_A16B16G16R16:
                    format = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
                    break;
                case FOURCC_L8:
                    format = LuminanceFormat(8, Format::UNORM, 8, 0);
                    break;
                case FOURCC_A8L8:
                    format = LuminanceFormat(16, Format::UNORM, 8, 8);
                    break;
                case FOURCC_A4L4:
                    format = LuminanceFormat(8, Format::UNORM, 4, 4);
                    break;

                case FOURCC_R16F:
                    format = MAKE_FORMAT(16, FLOAT16, R, 16, 0, 0, 0);
                    compression = TextureCompression::NONE;
                    break;
                case FOURCC_GR16F:
                    format = MAKE_FORMAT(32, FLOAT16, RG, 16, 16, 0, 0);
                    compression = TextureCompression::NONE;
                    break;
                case FOURCC_ABGR16F:
                    format = MAKE_FORMAT(64, FLOAT16, RGBA, 16, 16, 16, 16);
                    compression = TextureCompression::NONE;
                    break;

                case FOURCC_R32F:
                    format = MAKE_FORMAT(32, FLOAT32, R, 32, 0, 0, 0);
                    compression = TextureCompression::NONE;
                    break;
                case FOURCC_GR32F:
                    format = MAKE_FORMAT(64, FLOAT32, RG, 32, 32, 0, 0);
                    compression = TextureCompression::NONE;
                    break;
                case FOURCC_ABGR32F:
                    format = MAKE_FORMAT(128, FLOAT32, RGBA, 32, 32, 32, 32);
                    compression = TextureCompression::NONE;
                    break;

                case FOURCC_PTC2:
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    compression = TextureCompression::PVRTC_RGB_2BPP;
                    break;

                case FOURCC_DXT1:
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
					if (flags & DDPF_ALPHAPIXELS)
					{
	                    compression = TextureCompression::DXT1_ALPHA1;
					}
					else
					{
                        compression = TextureCompression::DXT1;
					}
                    break;

                case FOURCC_DXT2:
                case FOURCC_DXT3:
                case FOURCC_DXT4:
                case FOURCC_DXT5:
                case FOURCC_BC4U:
                case FOURCC_BC4S:
                case FOURCC_ATI1:
                case FOURCC_AT1N:
                case FOURCC_BC5U:
                case FOURCC_BC5S:
                case FOURCC_ATI2:
                case FOURCC_AT2N:
                case FOURCC_UYVY:
                case FOURCC_YUY2:
                case FOURCC_G8R8G8B8:
                case FOURCC_R8G8B8G8:
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    compression = fourcc_to_compression(fourCC);
                    break;

                default:
                    preserve_fourcc = true;
                    error = "Unsupported fourcc.";
                    break;
            }

            if (!preserve_fourcc)
            {
                // mark fourcc as consumed (format has been evaluated with it)
                fourCC = 0;
            }
        }

        const u8* read(LittleEndianConstPointer p)
        {
            size = p.read32();
            if (size != 32)
            {
                error = "Incorrect format size.";
                return nullptr;
            }

            flags = p.read32();
            fourCC = p.read32();
            rgbBitCount = p.read32();
            rBitMask = p.read32();
            gBitMask = p.read32();
            bBitMask = p.read32();
            aBitMask = p.read32();

            debugPrint(".dds format: [bits: %d, red: %d, green: %d, blue: %d, alpha: %d]\n",
                rgbBitCount,
                u32_count_bits(rBitMask),
                u32_count_bits(gBitMask),
                u32_count_bits(bBitMask),
                u32_count_bits(aBitMask));

            if (flags & DDPF_FOURCC)
            {
                processFourCC();
            }
            else
            {
                const u32 alphaMask = flags & DDPF_ALPHAPIXELS ? aBitMask : 0;

                compression = TextureCompression::NONE;

                if (flags & DDPF_RGB)
                {
                    format = Format(rgbBitCount, rBitMask, gBitMask, bBitMask, alphaMask);
                }
                else if (flags & DDPF_LUMINANCE)
                {
                    format = LuminanceFormat(rgbBitCount, rBitMask, alphaMask);
                }
                else if (flags & DDPF_ALPHA)
                {
                    format = LuminanceFormat(rgbBitCount, 0, aBitMask);
                }
                else if (flags & DDPF_YUV)
                {
                    error = "Unsupported mode (YUV).";
                }
                else if (flags & DDPF_PALETTE)
                {
                    error = "Unsupported mode (P8).";
                }
                else
                {
                    error = "Unknown mode.";
                }
            }

            return p;
        }
    };

	struct HeaderDDS
	{
        u32 size;
        u32 flags;
        u32 height;
        u32 width;
        u32 pitchOrLinearSize;
        u32 depth;
        u32 mipMapCount;
        FormatDDS pixelFormat;
        u32 caps;
        u32 caps2;
        u32 caps3;
        u32 caps4;

        ImageHeader header;
        TextureCompressionInfo info;

        const u8* data;

        void read(LittleEndianConstPointer p)
        {
            u32 magic = p.read32();
            if (magic != FOURCC_DDS)
            {
                header.setError("[ImageDecoder.DDS] Incorrect header.");
                return;
            }

            size = p.read32();
            if (size != 124)
            {
                header.setError("[ImageDecoder.DDS] Incorrect header size.");
                return;
            }

            flags = p.read32();
            height = p.read32();
            width = p.read32();
            pitchOrLinearSize = p.read32();
            depth = p.read32();
            mipMapCount = p.read32();
            p += 44;

            p = pixelFormat.read(p);
            if (pixelFormat.error)
            {
                header.setError("[ImageDecoder.DDS] %s", pixelFormat.error);
                return;
            }

            caps = p.read32();
            caps2 = p.read32();
            caps3 = p.read32();
            caps4 = p.read32();
            p += 4;

            debugPrint(".dds image: [%d x %d]\n", width, height);
            debugPrint("     depth: %d, mips: %d\n", depth, mipMapCount);

            if (pixelFormat.flags & DDPF_FOURCC)
            {
                if (pixelFormat.fourCC == FOURCC_DX10)
                {
                    HeaderDX10 header10;
                    p = header10.read(p);
                    processDX10(header10);
                }
            }

            // The header parsing is complete; compute compressed block format
            info = pixelFormat.compression;

            data = p;

            header.width   = width;
            header.height  = height;
            header.depth   = 0; // TODO: support volume images
            header.levels  = getMipmapCount();
            header.faces   = getFaceCount();
			header.palette = false;
            header.format  = pixelFormat.format;
            header.compression = pixelFormat.compression;
        }

        void processDX10(const HeaderDX10& header10)
        {
            debugPrint("DXGI format: %d\n", header10.dxgiFormat);

            if (header10.dxgiFormat >= u32(g_dxgi_table_size))
            {
                header.setError("[ImageDecoder.DDS] DXGI index out of range.");
                return;
            }
            else if (!header10.dxgiFormat)
            {
                // Unknown DXGI format
				return;
            }

            if (header10.arraySize > 1)
            {
                // TODO
                header.setError("[ImageDecoder.DDS] Arrays are not supported.");
                return;
            }

            // handle special cases
            switch (header10.dxgiFormat)
            {
                case DXGI_FORMAT_BC6H_TYPELESS:
                case DXGI_FORMAT_BC6H_UF16:
                    pixelFormat.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    pixelFormat.compression = TextureCompression::BPTC_RGB_UNSIGNED_FLOAT;
                    pixelFormat.fourCC = 0;
                    return;

                case DXGI_FORMAT_BC6H_SF16:
                    pixelFormat.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    pixelFormat.compression = TextureCompression::BPTC_RGB_SIGNED_FLOAT;
                    pixelFormat.fourCC = 0;
                    return;

                case DXGI_FORMAT_BC7_TYPELESS:
                case DXGI_FORMAT_BC7_UNORM:
                    pixelFormat.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    pixelFormat.compression = TextureCompression::BPTC_RGBA_UNORM;
                    pixelFormat.fourCC = 0;
                    return;

                case DXGI_FORMAT_BC7_UNORM_SRGB:
                    pixelFormat.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    pixelFormat.compression = TextureCompression::BPTC_SRGB_ALPHA_UNORM;
                    pixelFormat.fourCC = 0;
                    return;
            }

            const FormatDXGI& dxgi = g_dxgi_table[header10.dxgiFormat];

            if (dxgi.fourcc)
            {
                pixelFormat.fourCC = dxgi.fourcc;
                pixelFormat.processFourCC();
            }
            else
            {
                if (!dxgi.format.bits)
                {
                    // TODO: which format (id = header.dxgiFormat)
                    header.setError("[ImageDecoder.DDS] DXGI format not supported.");
                    return;
                }

                pixelFormat.fourCC = 0;
                pixelFormat.compression = TextureCompression::NONE;

                switch (dxgi.format.type)
                {
                    case Format::FLOAT16:
                    case Format::FLOAT32:
                    case Format::UNORM:
                        pixelFormat.format = dxgi.format;
                        break;

                    case Format::NONE:
                    case Format::UINT:
                    case Format::SINT:
                    case Format::SNORM:
					case Format::FLOAT64:
						// TODO: these WILL be supported in custom "DXGI Blitter"
                        header.setError("[ImageDecoder.DDS] DXGI format type not supported.");
                        return;
                }
            }
        }

        int getMipmapCount() const
        {
            int value = 1;

            if (flags & DDSD_MIPMAPCOUNT)
            {
                value = mipMapCount;
            }
            else if (caps & DDSCAPS_MIPMAP)
            {
                value = u32_log2(std::max(width, height)) + 1;
            }

            return value;
        }

        int getFaceCount() const
        {
            int value = 1;

            if (caps2 & DDSCAPS2_CUBEMAP)
            {
                u32 mask = (caps2 & DDSCAPS2_CUBEMAP_ALLFACES) >> 10;
                value = u32_count_bits(mask);
            }

            return value;
        }

        int getLevelSize(int xsize, int ysize) const
        {
            int pitch = 0;

            if (info.compression != TextureCompression::NONE)
            {
                const int xblocks = ceil_div(xsize, info.width);
                const int yblocks = ceil_div(ysize, info.height);
                pitch = xblocks * info.bytes;
                ysize = yblocks;
            }
            else
            {
                const int bytesPerPixel = ceil_div(pixelFormat.format.bits, 8);
                pitch = xsize * bytesPerPixel;
            }

            // TODO: should the pitch be aligned to 32 bits?
            return ysize * pitch;
        }

        ConstMemory getMemory(int level, int depth, int face) const
        {
            MANGO_UNREFERENCED(depth); // TODO: support depth parameter for volume textures

            const int maxFace = getFaceCount();
            const int maxLevel = getMipmapCount();

            const u8* image = data;
            ConstMemory selected;

            for (int iFace = 0; iFace < maxFace; ++iFace)
            {
                for (int iLevel = 0; iLevel < maxLevel; ++iLevel)
                {
                    const int xsize = std::max(1, int(width) >> iLevel);
                    const int ysize = std::max(1, int(height) >> iLevel);
                    const int bytes = getLevelSize(xsize, ysize);

                    if (iFace == face && iLevel == level)
                    {
                        // Store selected address
                        selected = ConstMemory(image, bytes);
                    }

                    image += bytes;
                }
            }

            return selected;
        }
	};

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        HeaderDDS m_header;

        Interface(ConstMemory memory)
        {
            LittleEndianConstPointer p = memory.address;
            m_header.read(p);
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header.header;
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            return m_header.getMemory(level, depth, face);
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);

            ImageDecodeStatus status;

			const ImageHeader& header = m_header.header;
            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            ConstMemory imageMemory = m_header.getMemory(level, depth, face);
            TextureCompression compression = header.compression;

            if (m_header.pixelFormat.fourCC)
            {
                TextureCompressionInfo info = fourcc_to_compression(m_header.pixelFormat.fourCC);
                TextureCompressionStatus cs = info.decompress(dest, imageMemory);

                status.info = cs.info;
                status.success = cs.success;
                status.direct = cs.direct;
            }
            else if (compression != TextureCompression::NONE)
            {
                TextureCompressionInfo info = compression;
                TextureCompressionStatus cs = info.decompress(dest, imageMemory);

                status.info = cs.info;
                status.success = cs.success;
                status.direct = cs.direct;
            }
            else
            {
                Format format = header.format;
                int width = std::max(1, header.width >> level);
                int height = std::max(1, header.height >> level);
                size_t stride = width * format.bytes();

                Surface source(width, height, format, stride, imageMemory.address);
                dest.blit(0, 0, source);
            }

            return status;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageDecoderDDS()
    {
        registerImageDecoder(createInterface, ".dds");
    }

} // namespace mango::image
