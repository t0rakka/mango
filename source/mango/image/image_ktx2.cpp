/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/compress.hpp>
#include <mango/image/image.hpp>
#include <mango/image/fourcc.hpp>
#include <map>

namespace
{
    using namespace mango;
    using namespace mango::image;
    using namespace mango::vulkan;

    // ------------------------------------------------------------
    // KTX2
    // ------------------------------------------------------------

    // KTX2 Format Specification:
    // https://www.khronos.org/registry/KTX/specs/2.0/ktxspec_v2.html
    // https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html

    enum : u32
    {
        SUPERCOMPRESSION_NONE = 0,
        SUPERCOMPRESSION_BASIS_LZ = 1,
        SUPERCOMPRESSION_ZSTANDARD = 2,
        SUPERCOMPRESSION_ZLIB = 3,
    };

    struct VulkanFormatDesc
    {
        Format format;
        TextureCompression compression;
        const char* name;
    };

    static
    VulkanFormatDesc g_vulkan_format_array [] =
    {
        // 0 .. 1
        { Format(), TextureCompression::NONE, "UNDEFINED" },
        { Format(8, Format::UNORM, Format::RG, 4, 4, 0, 0), TextureCompression::NONE, "R4G4_UNORM_PACK8" },

        // 2 .. 8
        { Format(16, Format::UNORM, Format::RGBA, 4, 4, 4, 4), TextureCompression::NONE, "R4G4B4A4_UNORM_PACK16" },
        { Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4), TextureCompression::NONE, "B4G4R4A4_UNORM_PACK16" },
        { Format(16, Format::UNORM, Format::RGB, 5, 6, 5, 0), TextureCompression::NONE, "R5G6B5_UNORM_PACK16" },
        { Format(16, Format::UNORM, Format::BGR, 5, 6, 5, 0), TextureCompression::NONE, "B5G6R5_UNORM_PACK16" },
        { Format(16, Format::UNORM, Format::RGBA, 5, 5, 5, 1), TextureCompression::NONE, "R5G5B5A1_UNORM_PACK16" },
        { Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 1), TextureCompression::NONE, "B5G5R5A1_UNORM_PACK16" },
        { Format(16, Format::UNORM, Format::ARGB, 1, 5, 5, 5), TextureCompression::NONE, "A1R5G5B5_UNORM_PACK16" },

        // 9 .. 15
        { Format(8, Format::UNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE, "R8_UNORM" },
        { Format(8, Format::SNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE, "R8_SNORM" },
        { Format(8, Format::UNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE, "R8_USCALED" },
        { Format(8, Format::SNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE, "R8_SSCALED" },
        { Format(8, Format::UINT, Format::R, 8, 0, 0, 0), TextureCompression::NONE, "R8_UINT" },
        { Format(8, Format::SINT, Format::R, 8, 0, 0, 0), TextureCompression::NONE, "R8_SINT" },
        { Format(8, Format::UNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE,"R8_SRGB" },

        // 16 .. 22
        { Format(16, Format::UNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE, "R8G8_UNORM" },
        { Format(16, Format::SNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE, "R8G8_SNORM" },
        { Format(16, Format::UNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE, "R8G8_USCALED" },
        { Format(16, Format::SNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE, "R8G8_SSCALED" },
        { Format(16, Format::UINT, Format::RG, 8, 8, 0, 0), TextureCompression::NONE, "R8G8_UINT" },
        { Format(16, Format::SINT, Format::RG, 8, 8, 0, 0), TextureCompression::NONE, "R8G8_SINT" },
        { Format(16, Format::UNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE, "R8G8_SRGB" },

        // 23 .. 29
        { Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE, "R8G8B8_UNORM" },
        { Format(24, Format::SNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE, "R8G8B8_SNORM" },
        { Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE, "R8G8B8_USCALED" },
        { Format(24, Format::SNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE, "R8G8B8_SSCALED" },
        { Format(24, Format::UINT, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE, "R8G8B8_UINT" },
        { Format(24, Format::SINT, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE, "R8G8B8_SINT" },
        { Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE, "R8G8B8_SRGB" },

        // 30 .. 36
        { Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE, "B8G8R8_UNORM" },
        { Format(24, Format::SNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE, "B8G8R8_SNORM" },
        { Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE, "B8G8R8_USCALED" },
        { Format(24, Format::SNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE, "B8G8R8_SSCALED" },
        { Format(24, Format::UINT, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE, "B8G8R8_UINT" },
        { Format(24, Format::SINT, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE, "B8G8R8_SINT" },
        { Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE, "B8G8R8_SRGB" },

        // 37 .. 43
        { Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE, "R8G8B8A8_UNORM" },
        { Format(32, Format::SNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE, "R8G8B8A8_SNORM" },
        { Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE, "R8G8B8A8_USCALED" },
        { Format(32, Format::SNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE, "R8G8B8A8_SSCALED" },
        { Format(32, Format::UINT, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE, "R8G8B8A8_UINT" },
        { Format(32, Format::SINT, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE, "R8G8B8A8_SINT" },
        { Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE, "R8G8B8A8_SRGB" },

        // 44 .. 50
        { Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE, "B8G8R8A8_UNORM" },
        { Format(32, Format::SNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE, "B8G8R8A8_SNORM" },
        { Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE, "B8G8R8A8_USCALED" },
        { Format(32, Format::SNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE, "B8G8R8A8_SSCALED" },
        { Format(32, Format::UINT, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE, "B8G8R8A8_UINT" },
        { Format(32, Format::SINT, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE, "B8G8R8A8_SINT" },
        { Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE,"B8G8R8A8_SRGB" },

        // 51 .. 57
        { Format(32, Format::UNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE, "A8B8G8R8_UNORM_PACK32" },
        { Format(32, Format::SNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE, "A8B8G8R8_SNORM_PACK32" },
        { Format(32, Format::UNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE, "A8B8G8R8_USCALED_PACK32" },
        { Format(32, Format::SNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE, "A8B8G8R8_SSCALED_PACK32" },
        { Format(32, Format::UINT, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE, "A8B8G8R8_UINT_PACK32" },
        { Format(32, Format::SINT, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE, "A8B8G8R8_SINT_PACK32" },
        { Format(32, Format::UNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE, "A8B8G8R8_SRGB_PACK32" },

        // 58 .. 63
        { Format(32, Format::UNORM, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE, "A2R10G10B10_UNORM_PACK32" },
        { Format(32, Format::SNORM, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE, "A2R10G10B10_SNORM_PACK32" },
        { Format(32, Format::UNORM, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE, "A2R10G10B10_USCALED_PACK32" },
        { Format(32, Format::SNORM, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE, "A2R10G10B10_SSCALED_PACK32" },
        { Format(32, Format::UINT, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE, "A2R10G10B10_UINT_PACK32" },
        { Format(32, Format::SINT, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE, "A2R10G10B10_SINT_PACK32" },

        // 64 .. 69
        { Format(32, Format::UNORM, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE, "A2B10G10R10_UNORM_PACK32" },
        { Format(32, Format::SNORM, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE, "A2B10G10R10_SNORM_PACK32" },
        { Format(32, Format::UNORM, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE, "A2B10G10R10_USCALED_PACK32" },
        { Format(32, Format::SNORM, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE, "A2B10G10R10_SSCALED_PACK32" },
        { Format(32, Format::UINT, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE, "A2B10G10R10_UINT_PACK32" },
        { Format(32, Format::SINT, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE, "A2B10G10R10_SINT_PACK32" },

        // 70 .. 76
        { Format(16, Format::UNORM, Format::R, 16, 0, 0, 0), TextureCompression::NONE, "R16_UNORM" },
        { Format(16, Format::SNORM, Format::R, 16, 0, 0, 0), TextureCompression::NONE, "R16_SNORM" },
        { Format(16, Format::UNORM, Format::R, 16, 0, 0, 0), TextureCompression::NONE, "R16_USCALED" },
        { Format(16, Format::SNORM, Format::R, 16, 0, 0, 0), TextureCompression::NONE, "R16_SSCALED" },
        { Format(16, Format::UINT, Format::R, 16, 0, 0, 0), TextureCompression::NONE, "R16_UINT" },
        { Format(16, Format::SINT, Format::R, 16, 0, 0, 0), TextureCompression::NONE, "R16_SINT" },
        { Format(16, Format::FLOAT16, Format::R, 16, 0, 0, 0), TextureCompression::NONE, "R16_SFLOAT" },

        // 77 .. 83
        { Format(32, Format::UNORM, Format::RG, 16, 16, 0, 0), TextureCompression::NONE, "R16G16_UNORM" },
        { Format(32, Format::SNORM, Format::RG, 16, 16, 0, 0), TextureCompression::NONE, "R16G16_SNORM" },
        { Format(32, Format::UNORM, Format::RG, 16, 16, 0, 0), TextureCompression::NONE, "R16G16_USCALED" },
        { Format(32, Format::SNORM, Format::RG, 16, 16, 0, 0), TextureCompression::NONE, "R16G16_SSCALED" },
        { Format(32, Format::UINT, Format::RG, 16, 16, 0, 0), TextureCompression::NONE, "R16G16_UINT" },
        { Format(32, Format::SINT, Format::RG, 16, 16, 0, 0), TextureCompression::NONE, "R16G16_SINT" },
        { Format(32, Format::FLOAT16, Format::RG, 16, 16, 0, 0), TextureCompression::NONE, "R16G16_SFLOAT" },

        // 84 .. 90
        { Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE, "R16G16B16_UNORM" },
        { Format(48, Format::SNORM, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE, "R16G16B16_SNORM" },
        { Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE, "R16G16B16_USCALED" },
        { Format(48, Format::SNORM, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE, "R16G16B16_SSCALED" },
        { Format(48, Format::UINT, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE, "R16G16B16_UINT" },
        { Format(48, Format::SINT, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE, "R16G16B16_SINT" },
        { Format(48, Format::FLOAT16, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE, "R16G16B16_SFLOAT" },

        // 91 .. 97
        { Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE, "R16G16B16A16_UNORM" },
        { Format(64, Format::SNORM, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE, "R16G16B16A16_SNORM" },
        { Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE, "R16G16B16A16_USCALED" },
        { Format(64, Format::SNORM, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE, "R16G16B16A16_SSCALED" },
        { Format(64, Format::UINT, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE, "R16G16B16A16_UINT" },
        { Format(64, Format::SINT, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE, "R16G16B16A16_SINT" },
        { Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE, "R16G16B16A16_SFLOAT" },

        // 98 .. 100
        { Format(32, Format::UINT, Format::R, 32, 0, 0, 0), TextureCompression::NONE, "R32_UINT" },
        { Format(32, Format::SINT, Format::R, 32, 0, 0, 0), TextureCompression::NONE, "R32_SINT" },
        { Format(32, Format::FLOAT32, Format::R, 32, 0, 0, 0), TextureCompression::NONE, "R32_SFLOAT" },

        // 101 .. 103
        { Format(64, Format::UINT, Format::RG, 32, 32, 0, 0), TextureCompression::NONE, "R32G32_UINT" },
        { Format(64, Format::SINT, Format::RG, 32, 32, 0, 0), TextureCompression::NONE, "R32G32_SINT" },
        { Format(64, Format::FLOAT32, Format::RG, 32, 32, 0, 0), TextureCompression::NONE, "R32G32_SFLOAT" },

        // 104 .. 106
        { Format(96, Format::UINT, Format::RGB, 32, 32, 32, 0), TextureCompression::NONE, "R32G32B32_UINT" },
        { Format(96, Format::SINT, Format::RGB, 32, 32, 32, 0), TextureCompression::NONE, "R32G32B32_SINT" },
        { Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32, 0), TextureCompression::NONE, "R32G32B32_SFLOAT" },

        // 107 .. 109
        { Format(128, Format::UINT, Format::RGBA, 32, 32, 32, 32), TextureCompression::NONE, "R32G32B32A32_UINT" },
        { Format(128, Format::SINT, Format::RGBA, 32, 32, 32, 32), TextureCompression::NONE, "R32G32B32A32_SINT" },
        { Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32), TextureCompression::NONE, "R32G32B32A32_SFLOAT" },

        // 110 .. 112
        { Format(64, Format::UINT, Format::R, 64, 0, 0, 0), TextureCompression::NONE, "R64_UINT" },
        { Format(64, Format::SINT, Format::R, 64, 0, 0, 0), TextureCompression::NONE, "R64_SINT" },
        { Format(64, Format::FLOAT64, Format::R, 64, 0, 0, 0), TextureCompression::NONE, "R64_SFLOAT" },

        // 113 .. 115
        { Format(128, Format::UINT, Format::RG, 64, 64, 0, 0), TextureCompression::NONE, "R64G64_UINT" },
        { Format(128, Format::SINT, Format::RG, 64, 64, 0, 0), TextureCompression::NONE, "R64G64_SINT" },
        { Format(128, Format::FLOAT64, Format::RG, 64, 64, 0, 0), TextureCompression::NONE, "R64G64_SFLOAT" },

        // 116 .. 118
        { Format(192, Format::UINT, Format::RGB, 64, 64, 64, 0), TextureCompression::NONE, "R64G64B64_UINT" },
        { Format(192, Format::SINT, Format::RGB, 64, 64, 64, 0), TextureCompression::NONE, "R64G64B64_SINT" },
        { Format(192, Format::FLOAT64, Format::RGB, 64, 64, 64, 0), TextureCompression::NONE, "R64G64B64_SFLOAT" },

        // 119 .. 121
        { Format(256, Format::UINT, Format::RGBA, 64, 64, 64, 64), TextureCompression::NONE, "R64G64B64A64_UINT" },
        { Format(256, Format::SINT, Format::RGBA, 64, 64, 64, 64), TextureCompression::NONE, "R64G64B64A64_SINT" },
        { Format(256, Format::FLOAT64, Format::RGBA, 64, 64, 64, 64), TextureCompression::NONE, "R64G64B64A64_SFLOAT" },

        // 122 .. 123
        { Format(), TextureCompression::R11F_G11F_B10F, "B10G11R11_UFLOAT_PACK32" },
        { Format(), TextureCompression::RGB9_E5, "E5B9G9R9_UFLOAT_PACK32" },

         // 124 .. 130
        { Format(), TextureCompression::NONE, "D16_UNORM" },
        { Format(), TextureCompression::NONE, "X8_D24_UNORM_PACK32" },
        { Format(), TextureCompression::NONE, "D32_SFLOAT" },
        { Format(), TextureCompression::NONE, "S8_UINT" },
        { Format(), TextureCompression::NONE, "D16_UNORM_S8_UINT" },
        { Format(), TextureCompression::NONE, "D24_UNORM_S8_UINT" },
        { Format(), TextureCompression::NONE, "D32_SFLOAT_S8_UINT" },

        // 131 .. 146
        { Format(), TextureCompression::BC1_UNORM, "BC1_RGB_UNORM_BLOCK" },
        { Format(), TextureCompression::BC1_UNORM_SRGB, "BC1_RGB_SRGB_BLOCK" },
        { Format(), TextureCompression::BC1_UNORM_ALPHA, "BC1_RGBA_UNORM_BLOCK" },
        { Format(), TextureCompression::BC1_UNORM_ALPHA_SRGB, "BC1_RGBA_SRGB_BLOCK" },
        { Format(), TextureCompression::BC2_UNORM, "BC2_UNORM_BLOCK" },
        { Format(), TextureCompression::BC2_UNORM_SRGB, "BC2_SRGB_BLOCK" },
        { Format(), TextureCompression::BC3_UNORM, "BC3_UNORM_BLOCK" },
        { Format(), TextureCompression::BC3_UNORM_SRGB, "BC3_SRGB_BLOCK" },
        { Format(), TextureCompression::BC4_UNORM, "BC4_UNORM_BLOCK" },
        { Format(), TextureCompression::BC4_SNORM, "BC4_SNORM_BLOCK" },
        { Format(), TextureCompression::BC5_UNORM, "BC5_UNORM_BLOCK" },
        { Format(), TextureCompression::BC5_SNORM, "BC5_SNORM_BLOCK" },
        { Format(), TextureCompression::BC6H_UF16, "BC6H_UFLOAT_BLOCK" },
        { Format(), TextureCompression::BC6H_SF16, "BC6H_SFLOAT_BLOCK" },
        { Format(), TextureCompression::BC7_UNORM, "BC7_UNORM_BLOCK" },
        { Format(), TextureCompression::BC7_UNORM_SRGB, "BC7_SRGB_BLOCK" },

        // 147 .. 152
        { Format(), TextureCompression::ETC2_RGB, "ETC2_R8G8B8_UNORM_BLOCK" },
        { Format(), TextureCompression::ETC2_SRGB, "ETC2_R8G8B8_SRGB_BLOCK" },
        { Format(), TextureCompression::ETC2_RGB_ALPHA1, "ETC2_R8G8B8A1_UNORM_BLOCK" },
        { Format(), TextureCompression::ETC2_SRGB_ALPHA1, "ETC2_R8G8B8A1_SRGB_BLOCK" },
        { Format(), TextureCompression::ETC2_RGBA, "ETC2_R8G8B8A8_UNORM_BLOCK" },
        { Format(), TextureCompression::ETC2_SRGB_ALPHA8, "ETC2_R8G8B8A8_SRGB_BLOCK" },

        // 153 .. 156
        { Format(), TextureCompression::EAC_R11, "EAC_R11_UNORM_BLOCK" },
        { Format(), TextureCompression::EAC_SIGNED_R11, "EAC_R11_SNORM_BLOCK" },
        { Format(), TextureCompression::EAC_RG11, "EAC_R11G11_UNORM_BLOCK" },
        { Format(), TextureCompression::EAC_SIGNED_RG11, "EAC_R11G11_SNORM_BLOCK" },

        // 157 .. 184
        { Format(), TextureCompression::ASTC_RGBA_4x4, "ASTC_4x4_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_4x4, "ASTC_4x4_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_5x4, "ASTC_5x4_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_5x4, "ASTC_5x4_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_5x5, "ASTC_5x5_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_5x5, "ASTC_5x5_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_6x5, "ASTC_6x5_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_6x5, "ASTC_6x5_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_6x6, "ASTC_6x6_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_6x6, "ASTC_6x6_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_8x5, "ASTC_8x5_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_8x5, "ASTC_8x5_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_8x6, "ASTC_8x6_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_8x6, "ASTC_8x6_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_8x8, "ASTC_8x8_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_8x8, "ASTC_8x8_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_10x5, "ASTC_10x5_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_10x5, "ASTC_10x5_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_10x6, "ASTC_10x6_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_10x6, "ASTC_10x6_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_10x8, "ASTC_10x8_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_10x8, "ASTC_10x8_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_10x10, "ASTC_10x10_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_10x10, "ASTC_10x10_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_12x10, "ASTC_12x10_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_12x10, "ASTC_12x10_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_RGBA_12x12, "ASTC_12x12_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_ALPHA_12x12, "ASTC_12x12_SRGB_BLOCK" },
    };

    static
    std::map<u32, VulkanFormatDesc> g_vulkan_format_map =
    {
        { 1000054000, { Format(), TextureCompression::PVRTC1_2BPP_UNORM_BLOCK_IMG, "PVRTC1_2BPP_UNORM_BLOCK_IMG" } },
        { 1000054001, { Format(), TextureCompression::PVRTC1_4BPP_UNORM_BLOCK_IMG, "PVRTC1_4BPP_UNORM_BLOCK_IMG" } },
        { 1000054002, { Format(), TextureCompression::PVRTC2_2BPP_UNORM_BLOCK_IMG, "PVRTC2_2BPP_UNORM_BLOCK_IMG" } },
        { 1000054003, { Format(), TextureCompression::PVRTC2_4BPP_UNORM_BLOCK_IMG, "PVRTC2_4BPP_UNORM_BLOCK_IMG" } },
        { 1000054004, { Format(), TextureCompression::PVRTC1_2BPP_SRGB_BLOCK_IMG, "PVRTC1_2BPP_SRGB_BLOCK_IMG" } },
        { 1000054005, { Format(), TextureCompression::PVRTC1_4BPP_SRGB_BLOCK_IMG, "PVRTC1_4BPP_SRGB_BLOCK_IMG" } },
        { 1000054006, { Format(), TextureCompression::PVRTC2_2BPP_SRGB_BLOCK_IMG, "PVRTC2_2BPP_SRGB_BLOCK_IMG" } },
        { 1000054007, { Format(), TextureCompression::PVRTC2_4BPP_SRGB_BLOCK_IMG, "PVRTC2_4BPP_SRGB_BLOCK_IMG" } },
        { 1000066000, { Format(), TextureCompression::ASTC_RGBA_4x4, "ASTC_4x4_SFLOAT_BLOCK_EXT" } },
        { 1000066001, { Format(), TextureCompression::ASTC_RGBA_5x4, "ASTC_5x4_SFLOAT_BLOCK_EXT" } },
        { 1000066002, { Format(), TextureCompression::ASTC_RGBA_5x5, "ASTC_5x5_SFLOAT_BLOCK_EXT" } },
        { 1000066003, { Format(), TextureCompression::ASTC_RGBA_6x5, "ASTC_6x5_SFLOAT_BLOCK_EXT" } },
        { 1000066004, { Format(), TextureCompression::ASTC_RGBA_6x6, "ASTC_6x6_SFLOAT_BLOCK_EXT" } },
        { 1000066005, { Format(), TextureCompression::ASTC_RGBA_8x5, "ASTC_8x5_SFLOAT_BLOCK_EXT" } },
        { 1000066006, { Format(), TextureCompression::ASTC_RGBA_8x6, "ASTC_8x6_SFLOAT_BLOCK_EXT" } },
        { 1000066007, { Format(), TextureCompression::ASTC_RGBA_8x8, "ASTC_8x8_SFLOAT_BLOCK_EXT" } },
        { 1000066008, { Format(), TextureCompression::ASTC_RGBA_10x5, "ASTC_10x5_SFLOAT_BLOCK_EXT" } },
        { 1000066009, { Format(), TextureCompression::ASTC_RGBA_10x6, "ASTC_10x6_SFLOAT_BLOCK_EXT" } },
        { 1000066010, { Format(), TextureCompression::ASTC_RGBA_10x8, "ASTC_10x8_SFLOAT_BLOCK_EXT" } },
        { 1000066011, { Format(), TextureCompression::ASTC_RGBA_10x10, "ASTC_10x10_SFLOAT_BLOCK_EXT" } },
        { 1000066012, { Format(), TextureCompression::ASTC_RGBA_12x10, "ASTC_12x10_SFLOAT_BLOCK_EXT" } },
        { 1000066013, { Format(), TextureCompression::ASTC_RGBA_12x12, "ASTC_12x12_SFLOAT_BLOCK_EXT" } },
    };

    static
    VulkanFormatDesc getFormatDesc(u32 vkformat)
    {
        VulkanFormatDesc desc;

        const u32 maxIndex = u32((sizeof(g_vulkan_format_array) / sizeof(g_vulkan_format_array[0])));

        if (vkformat < maxIndex)
        {
            desc = g_vulkan_format_array[vkformat];
        }
        else
        {
            // not in the array -> check the map
            auto it = g_vulkan_format_map.find(vkformat);
            if (it != g_vulkan_format_map.end())
            {
                desc = it->second;
            }
            else
            {
                // not found -> select undefined format
                desc = g_vulkan_format_array[0];
            }
        }

        if (desc.compression != TextureCompression::NONE)
        {
            // patch preferred decoding format
            TextureCompressionInfo info(desc.compression);
            desc.format = info.format;
        }

        return desc;
    }

    static
    bool isFormatProhibited(u32 vkformat)
    {
        bool prohibited = false;
        switch (vkformat)
        {
            case FORMAT_A8B8G8R8_UNORM_PACK32:
            case FORMAT_A8B8G8R8_SNORM_PACK32:
            case FORMAT_A8B8G8R8_UINT_PACK32:
            case FORMAT_A8B8G8R8_SINT_PACK32:
            case FORMAT_A8B8G8R8_SRGB_PACK32:
            case FORMAT_R8_USCALED:
            case FORMAT_R8_SSCALED:
            case FORMAT_R8G8_USCALED:
            case FORMAT_R8G8_SSCALED:
            case FORMAT_R8G8B8_USCALED:
            case FORMAT_R8G8B8_SSCALED:
            case FORMAT_B8G8R8_USCALED:
            case FORMAT_B8G8R8_SSCALED:
            case FORMAT_R8G8B8A8_USCALED:
            case FORMAT_R8G8B8A8_SSCALED:
            case FORMAT_B8G8R8A8_USCALED:
            case FORMAT_B8G8R8A8_SSCALED:
            case FORMAT_A8B8G8R8_USCALED_PACK32:
            case FORMAT_A8B8G8R8_SSCALED_PACK32:
            case FORMAT_A2R10G10B10_USCALED_PACK32:
            case FORMAT_A2R10G10B10_SSCALED_PACK32:
            case FORMAT_A2B10G10R10_USCALED_PACK32:
            case FORMAT_A2B10G10R10_SSCALED_PACK32:
            case FORMAT_R16_USCALED:
            case FORMAT_R16_SSCALED:
            case FORMAT_R16G16_USCALED:
            case FORMAT_R16G16_SSCALED:
            case FORMAT_R16G16B16_USCALED:
            case FORMAT_R16G16B16_SSCALED:
            case FORMAT_R16G16B16A16_USCALED:
            case FORMAT_R16G16B16A16_SSCALED:
            case FORMAT_G8B8G8R8_422_UNORM:
            case FORMAT_B8G8R8G8_422_UNORM:
            case FORMAT_G8_B8_R8_3PLANE_420_UNORM:
            case FORMAT_G8_B8R8_2PLANE_420_UNORM:
            case FORMAT_G8_B8_R8_3PLANE_422_UNORM:
            case FORMAT_G8_B8R8_2PLANE_422_UNORM:
            case FORMAT_G8_B8_R8_3PLANE_444_UNORM:
            case FORMAT_R10X6_UNORM_PACK16:
            case FORMAT_R10X6G10X6_UNORM_2PACK16:
            case FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
            case FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
            case FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
            case FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
            case FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
            case FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
            case FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
            case FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
            case FORMAT_R12X4_UNORM_PACK16:
            case FORMAT_R12X4G12X4_UNORM_2PACK16:
            case FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
            case FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
            case FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
            case FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
            case FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
            case FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
            case FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
            case FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
            case FORMAT_G16B16G16R16_422_UNORM:
            case FORMAT_B16G16R16G16_422_UNORM:
            case FORMAT_G16_B16_R16_3PLANE_420_UNORM:
            case FORMAT_G16_B16R16_2PLANE_420_UNORM:
            case FORMAT_G16_B16_R16_3PLANE_422_UNORM:
            case FORMAT_G16_B16R16_2PLANE_422_UNORM:
            case FORMAT_G16_B16_R16_3PLANE_444_UNORM:
                prohibited = true;
                break;
        }
        return prohibited;
    }

    struct HeaderKTX2
    {
        u32 vkFormat = 0;
        u32 typeSize = 0;
        u32 pixelWidth = 0;
        u32 pixelHeight = 0;
        u32 pixelDepth = 0;
        u32 layerCount = 0;
        u32 faceCount = 0;
        u32 levelCount = 0;
        u32 supercompressionScheme = 0;

        bool read(LittleEndianConstPointer& p)
        {
            constexpr u8 identifier[] =
            {
                0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 
                0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
            };

            if (std::memcmp(p, identifier, sizeof(identifier)))
            {
                // incorrect identifier
                return false;
            }

            p += sizeof(identifier);

            vkFormat = p.read32();
            typeSize = p.read32();
            pixelWidth = p.read32();
            pixelHeight = p.read32();
            pixelDepth = p.read32();
            layerCount = p.read32();
            faceCount = p.read32();
            levelCount = p.read32();
            supercompressionScheme = p.read32();

            return true;
        }
    };

    struct LevelKTX2
    {
        u64 offset;
        u64 length;
        u64 uncompressed_length;
        ConstMemory memory;
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ConstMemory m_memory;
        ImageHeader m_header;

        std::vector<LevelKTX2> m_levels;

        u32 m_supercompression = 0;
        Buffer m_buffer;

        bool m_orientation_x = false;
        bool m_orientation_y = false;
        bool m_orientation_z = false;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            LittleEndianConstPointer p = memory.address;

            HeaderKTX2 header;
            if (!header.read(p))
            {
                // TODO: incorrect header
                printf("*** Incorrect identifier.\n");
            }

            if (isFormatProhibited(header.vkFormat))
            {
                // TODO: prohibited format
                printf("*** Prohibited format.\n");
            }

            VulkanFormatDesc desc = getFormatDesc(header.vkFormat);

            m_header.width = header.pixelWidth;
            m_header.height = header.pixelHeight;
            m_header.depth = std::max(1u, header.pixelDepth);
            m_header.levels = header.levelCount;
            m_header.faces = header.faceCount;
            m_header.format = desc.format;
            m_header.compression = desc.compression;

            printf("vkFormat: %d \"%s\"\n", header.vkFormat, desc.name);
            printf("typeSize: %d\n", header.typeSize);
            printf("supercompressionScheme: %d\n", header.supercompressionScheme);

            m_supercompression = header.supercompressionScheme;

            switch (m_supercompression)
            {
                case SUPERCOMPRESSION_NONE:
                    break;
                case SUPERCOMPRESSION_BASIS_LZ:
                    break;
                case SUPERCOMPRESSION_ZSTANDARD:
                    break;
                case SUPERCOMPRESSION_ZLIB:
                    break;
                default:
                    // TODO: unsupported supercompression scheme
                    break;
            }

            // Index
            u32 dfdByteOffset = p.read32();
            u32 dfdByteLength = p.read32();
            u32 kvdByteOffset = p.read32();
            u32 kvdByteLength = p.read32();
            u64 sgdByteOffset = p.read64();
            u64 sgdByteLength = p.read64();

            MANGO_UNREFERENCED(dfdByteOffset);
            MANGO_UNREFERENCED(dfdByteLength);
            MANGO_UNREFERENCED(kvdByteOffset);
            MANGO_UNREFERENCED(kvdByteLength);
            MANGO_UNREFERENCED(sgdByteOffset);
            MANGO_UNREFERENCED(sgdByteLength);

            //printf("dfdByteOffset: %d, dfdByteLength: %d\n", dfdByteOffset, dfdByteLength);
            //printf("kvdByteOffset: %d, kvdByteLength: %d\n", kvdByteOffset, kvdByteLength);
            //printf("sgdByteOffset: %d, sgdByteLength: %d\n", (int)sgdByteOffset, (int)sgdByteLength);

            int levels = std::max(1, m_header.levels);
            printf("levels: %d\n", levels);

            for (int i = 0; i < levels; ++i)
            {
                LevelKTX2 level;

                level.offset = p.read64();
                level.length = p.read64();
                level.uncompressed_length = p.read64();
                level.memory = ConstMemory(m_memory.address + level.offset, level.length);

                m_levels.push_back(level);
                printf("  offset: %d, length: %d, uncompressed: %d\n",  (int)level.offset, (int)level.length, (int)level.uncompressed_length);
            }

            // Data Format Descriptor
            if (dfdByteLength)
            {
                p = memory.address + dfdByteOffset;
            }

            // Key/Value Data
            if (kvdByteLength)
            {
                p = memory.address + kvdByteOffset;
                const u8* end = p + kvdByteLength;

                while (p < end)
                {
                    u32 length = p.read32();
                    u32 padding = (0 - length) & 3;

                    const char* key = p.cast<const char>();
                    const u8* value = p + std::strlen(key) + 1;

                    if (!strcmp(key, "KTXorientation"))
                    {
                        for ( ; *value; value++)
                        {
                            switch (*value)
                            {
                                case 'l':
                                    m_orientation_x = true;
                                    break;
                                case 'u':
                                    m_orientation_y = true;
                                    break;
                                case 'o':
                                    m_orientation_z = true;
                                    break;
                            }
                        }
                    }

                    printf("key: %s\n", key);

                    p += length;
                    p += padding;
                }
            }

            // Supercompression Global Data
            if (sgdByteLength)
            {
                p = memory.address + sgdByteOffset;
            }
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            if (level < 0 || level >= m_levels.size())
            {
                return ConstMemory();
            }

            if (depth < 0 || depth >= m_header.depth)
            {
                return ConstMemory();
            }

            if (face < 0 || face >= m_header.faces)
            {
                return ConstMemory();
            }

            decompress();

            ConstMemory memory = m_levels[level].memory;

            if (depth > 0)
            {
                memory.size /= m_header.depth;
                memory.address += depth * memory.size;
            }
            else if (face > 0)
            {
                memory.size /= m_header.faces;
                memory.address += face * memory.size;
            }

            return memory;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            //MANGO_UNREFERENCED(dest);
            MANGO_UNREFERENCED(options);
            //MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            decompress();

            //TextureCompressionStatus TextureCompressionInfo::decompress(
            //    const Surface& surface, ConstMemory memory) const

            // TODO: level
            // TODO: depth
            // TODO: face
            // TODO: typesize dictates endianness swap on big-endian

            ImageDecodeStatus status;

            const int maxLevel = int(m_levels.size() - 1);
            if (level < 0 || level > maxLevel)
            {
                status.setError("Incorrect level (%d) [%d .. %d]", level, 0, maxLevel);
                return status;
            }

            ConstMemory memory = this->memory(level, depth, face);

            int width = std::max(1, m_header.width >> level);
            int height = std::max(1, m_header.height >> level);
            const Format& format = m_header.format;

            if (m_header.compression != TextureCompression::NONE)
            {
                // TODO: compressed surface -> decode -> blit
                TextureCompressionInfo info(m_header.compression);
                TextureCompressionStatus ts = info.decompress(dest, memory);
                if (!ts)
                {
                    status.setError(ts.info);
                }
            }
            else
            {
                printf("surface: %d x %d (%d bits)\n", width, height, format.bits);
                printf("memory: %d bytes\n", (int)memory.size);

                Surface temp(width, height, format, width * format.bytes(), memory.address);
                if (m_orientation_y)
                {
                    temp.image += temp.stride * (height - 1);
                    temp.stride = 0 - temp.stride;
                }

                dest.blit(0, 0, temp);
            }

            return status;
        }

        void decompress()
        {
            if (m_supercompression)
            {
                if (!m_buffer.size())
                {
                    // compute storage requirements
                    u64 uncompressed_size = 0;

                    for (auto level : m_levels)
                    {
                        uncompressed_size += level.uncompressed_length;
                    }

                    // allocate storage
                    m_buffer.resize(uncompressed_size);

                    u8* data = m_buffer.data();

                    // decompress
                    for (auto& level : m_levels)
                    {
                        CompressionStatus status;
                        u64 bytes = level.uncompressed_length;
                        Memory dest(data, bytes);

                        switch (m_supercompression)
                        {
                            case SUPERCOMPRESSION_BASIS_LZ:
                                status.setError("TODO");
                                break;
                            case SUPERCOMPRESSION_ZSTANDARD:
                                status = zstd::decompress(dest, level.memory);
                                break;
                            case SUPERCOMPRESSION_ZLIB:
                                status = zlib::decompress(dest, level.memory);
                                break;
                        }

                        if (status)
                        {
                            printf("* decompressed: %d bytes\n", int(status.size));
                        }
                        else
                        {
                            printf("* decompress status: %s\n", status.info.c_str());
                        }

                        level.memory = dest;
                        data += level.uncompressed_length;
                    }
                }
            }
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

    void registerImageDecoderKTX2()
    {
        registerImageDecoder(createInterface, ".ktx2");
    }

} // namespace mango::image
