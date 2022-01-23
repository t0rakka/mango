/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>
#include <mango/image/fourcc.hpp>

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

#if 0

    TextureCompression getCompressionFormat(Format& format, VkFormat vk_format)
    {
        struct FormatDesc
        {
            Format format;
            TextureCompression compression;
        };

        const FormatDesc table[] =
        {
            { FORMAT_NONE, TextureCompression::NONE },

            { Format(8, Format::UNORM, Format::RG, 4, 4, 0, 0), TextureCompression::NONE },

            { Format(16, Format::UNORM, Format::RGBA, 4, 4, 4, 4), TextureCompression::NONE },
            { Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4), TextureCompression::NONE },
            { Format(16, Format::UNORM, Format::RGB, 5, 6, 5, 0), TextureCompression::NONE },
            { Format(16, Format::UNORM, Format::BGR, 5, 6, 5, 0), TextureCompression::NONE },
            { Format(16, Format::UNORM, Format::RGBA, 5, 5, 5, 1), TextureCompression::NONE },
            { Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 1), TextureCompression::NONE },
            { Format(16, Format::UNORM, Format::ARGB, 1, 5, 5, 5), TextureCompression::NONE },

            { Format(8, Format::UNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE },
            { Format(8, Format::SNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE },
            { Format(8, Format::UNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE },
            { Format(8, Format::SNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE },
            { Format(8, Format::UINT, Format::R, 8, 0, 0, 0), TextureCompression::NONE },
            { Format(8, Format::SINT, Format::R, 8, 0, 0, 0), TextureCompression::NONE },
            { Format(8, Format::UNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE }, // srgb

            { Format(16, Format::UNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE },
            { Format(16, Format::SNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE },
            { Format(16, Format::UNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE },
            { Format(16, Format::SNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE },
            { Format(16, Format::UINT, Format::RG, 8, 8, 0, 0), TextureCompression::NONE },
            { Format(16, Format::SINT, Format::RG, 8, 8, 0, 0), TextureCompression::NONE },
            { Format(16, Format::UNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE }, // srgb

            { Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE },
            { Format(24, Format::SNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE },
            { Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE },
            { Format(24, Format::SNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE },
            { Format(24, Format::UINT, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE },
            { Format(24, Format::SINT, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE },
            { Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE }, // srgb

            { Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE },
            { Format(24, Format::SNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE },
            { Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE },
            { Format(24, Format::SNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE },
            { Format(24, Format::UINT, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE },
            { Format(24, Format::SINT, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE },
            { Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE }, // srgb

            { Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::SNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::SNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::UINT, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::SINT, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE }, // srgb

            { Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::SNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::SNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::UINT, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::SINT, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE }, // srgb

            { Format(32, Format::UNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::SNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::UNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::SNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::UINT, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::SINT, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE },
            { Format(32, Format::UNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE }, // srgb

            { Format(32, Format::UNORM, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE },
            { Format(32, Format::SNORM, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE },
            { Format(32, Format::UNORM, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE },
            { Format(32, Format::SNORM, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE },
            { Format(32, Format::UINT, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE },
            { Format(32, Format::SINT, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE },

            { Format(32, Format::UNORM, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE },
            { Format(32, Format::SNORM, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE },
            { Format(32, Format::UNORM, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE },
            { Format(32, Format::SNORM, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE },
            { Format(32, Format::UINT, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE },
            { Format(32, Format::SINT, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE },

            { Format(16, Format::UNORM, Format::R, 16, 0, 0, 0), TextureCompression::NONE },
            { Format(16, Format::SNORM, Format::R, 16, 0, 0, 0), TextureCompression::NONE },
            { Format(16, Format::UNORM, Format::R, 16, 0, 0, 0), TextureCompression::NONE },
            { Format(16, Format::SNORM, Format::R, 16, 0, 0, 0), TextureCompression::NONE },
            { Format(16, Format::UINT, Format::R, 16, 0, 0, 0), TextureCompression::NONE },
            { Format(16, Format::SINT, Format::R, 16, 0, 0, 0), TextureCompression::NONE },
            { Format(16, Format::FLOAT16, Format::R, 16, 0, 0, 0), TextureCompression::NONE },

            { Format(32, Format::UNORM, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },
            { Format(32, Format::SNORM, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },
            { Format(32, Format::UNORM, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },
            { Format(32, Format::SNORM, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },
            { Format(32, Format::UINT, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },
            { Format(32, Format::SINT, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },
            { Format(32, Format::FLOAT16, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },

            { Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },
            { Format(48, Format::SNORM, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },
            { Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },
            { Format(48, Format::SNORM, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },
            { Format(48, Format::UINT, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },
            { Format(48, Format::SINT, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },
            { Format(48, Format::FLOAT16, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },

            { Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },
            { Format(64, Format::SNORM, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },
            { Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },
            { Format(64, Format::SNORM, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },
            { Format(64, Format::UINT, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },
            { Format(64, Format::SINT, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },
            { Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },

            { Format(32, Format::UINT, Format::R, 32, 0, 0, 0), TextureCompression::NONE },
            { Format(32, Format::SINT, Format::R, 32, 0, 0, 0), TextureCompression::NONE },
            { Format(32, Format::FLOAT32, Format::R, 32, 0, 0, 0), TextureCompression::NONE },

            { Format(64, Format::UINT, Format::RG, 32, 32, 0, 0), TextureCompression::NONE },
            { Format(64, Format::SINT, Format::RG, 32, 32, 0, 0), TextureCompression::NONE },
            { Format(64, Format::FLOAT32, Format::RG, 32, 32, 0, 0), TextureCompression::NONE },

            { Format(96, Format::UINT, Format::RGB, 32, 32, 32, 0), TextureCompression::NONE },
            { Format(96, Format::SINT, Format::RGB, 32, 32, 32, 0), TextureCompression::NONE },
            { Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32, 0), TextureCompression::NONE },

            { Format(128, Format::UINT, Format::RGBA, 32, 32, 32, 32), TextureCompression::NONE },
            { Format(128, Format::SINT, Format::RGBA, 32, 32, 32, 32), TextureCompression::NONE },
            { Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32), TextureCompression::NONE },

            { Format(64, Format::UINT, Format::R, 64, 0, 0, 0), TextureCompression::NONE },
            { Format(64, Format::SINT, Format::R, 64, 0, 0, 0), TextureCompression::NONE },
            { Format(64, Format::FLOAT64, Format::R, 64, 0, 0, 0), TextureCompression::NONE },

            { Format(128, Format::UINT, Format::RG, 64, 64, 0, 0), TextureCompression::NONE },
            { Format(128, Format::SINT, Format::RG, 64, 64, 0, 0), TextureCompression::NONE },
            { Format(128, Format::FLOAT64, Format::RG, 64, 64, 0, 0), TextureCompression::NONE },

            { Format(192, Format::UINT, Format::RGB, 64, 64, 64, 0), TextureCompression::NONE },
            { Format(192, Format::SINT, Format::RGB, 64, 64, 64, 0), TextureCompression::NONE },
            { Format(192, Format::FLOAT64, Format::RGB, 64, 64, 64, 0), TextureCompression::NONE },

            { Format(256, Format::UINT, Format::RGBA, 64, 64, 64, 64), TextureCompression::NONE },
            { Format(256, Format::SINT, Format::RGBA, 64, 64, 64, 64), TextureCompression::NONE },
            { Format(256, Format::FLOAT64, Format::RGBA, 64, 64, 64, 64), TextureCompression::NONE },

            { FORMAT_NONE, TextureCompression::R11F_G11F_B10F },
            { FORMAT_NONE, TextureCompression::RGB9_E5 },

            { FORMAT_NONE, TextureCompression::NONE }, // VK_FORMAT_D16_UNORM
            { FORMAT_NONE, TextureCompression::NONE }, // VK_FORMAT_X8_D24_UNORM_PACK32
            { FORMAT_NONE, TextureCompression::NONE }, // VK_FORMAT_D32_SFLOAT
            { FORMAT_NONE, TextureCompression::NONE }, // VK_FORMAT_S8_UINT
            { FORMAT_NONE, TextureCompression::NONE }, // VK_FORMAT_D16_UNORM_S8_UINT
            { FORMAT_NONE, TextureCompression::NONE }, // VK_FORMAT_D24_UNORM_S8_UINT
            { FORMAT_NONE, TextureCompression::NONE }, // VK_FORMAT_D32_SFLOAT_S8_UINT

            { FORMAT_NONE, TextureCompression::BC1_UNORM },
            { FORMAT_NONE, TextureCompression::BC1_UNORM_SRGB },
            { FORMAT_NONE, TextureCompression::BC1_UNORM_ALPHA },
            { FORMAT_NONE, TextureCompression::BC1_UNORM_ALPHA_SRGB },
            { FORMAT_NONE, TextureCompression::BC2_UNORM },
            { FORMAT_NONE, TextureCompression::BC2_UNORM_SRGB },
            { FORMAT_NONE, TextureCompression::BC3_UNORM },
            { FORMAT_NONE, TextureCompression::BC3_UNORM_SRGB },
            { FORMAT_NONE, TextureCompression::BC4_UNORM },
            { FORMAT_NONE, TextureCompression::BC4_SNORM },
            { FORMAT_NONE, TextureCompression::BC5_UNORM },
            { FORMAT_NONE, TextureCompression::BC5_SNORM },
            { FORMAT_NONE, TextureCompression::BC6H_UF16 },
            { FORMAT_NONE, TextureCompression::BC6H_SF16 },
            { FORMAT_NONE, TextureCompression::BC7_UNORM },
            { FORMAT_NONE, TextureCompression::BC7_UNORM_SRGB },
            { FORMAT_NONE, TextureCompression::ETC2_RGB },
            { FORMAT_NONE, TextureCompression::ETC2_SRGB },
            { FORMAT_NONE, TextureCompression::ETC2_RGB_ALPHA1 },
            { FORMAT_NONE, TextureCompression::ETC2_SRGB_ALPHA1 },
            { FORMAT_NONE, TextureCompression::ETC2_RGBA },
            { FORMAT_NONE, TextureCompression::ETC2_SRGB_ALPHA8 },
            { FORMAT_NONE, TextureCompression::EAC_R11 },
            { FORMAT_NONE, TextureCompression::EAC_SIGNED_R11 },
            { FORMAT_NONE, TextureCompression::EAC_RG11 },
            { FORMAT_NONE, TextureCompression::EAC_SIGNED_RG11 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_4x4 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_4x4 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_5x4 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_5x4 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_5x5 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_5x5 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_6x5 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_6x5 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_6x6 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_6x6 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_8x5 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_8x5 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_8x6 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_8x6 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_8x8 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_8x8 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_10x5 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_10x5 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_10x6 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_10x6 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_10x8 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_10x8 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_10x10 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_10x10 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_12x10 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_12x10 },
            { FORMAT_NONE, TextureCompression::ASTC_RGBA_12x12 },
            { FORMAT_NONE, TextureCompression::ASTC_SRGB_ALPHA_12x12 },
        };

        const int maxTableIndex = int((sizeof(table) / sizeof(table[0])) - 1);

        int index = int(vk_format);
        if (index < 0 || index > maxTableIndex)
        {
            // select undefined format
            index = 0;
        }

        format = table[index].format;
        return table[index].compression;
    }

#endif

    /*
        FORMAT_UNDEFINED = 0,
        FORMAT_R4G4_UNORM_PACK8 = 1,
        FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
        FORMAT_B4G4R4A4_UNORM_PACK16 = 3,
        FORMAT_R5G6B5_UNORM_PACK16 = 4,
        FORMAT_B5G6R5_UNORM_PACK16 = 5,
        FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
        FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
        FORMAT_A1R5G5B5_UNORM_PACK16 = 8,
        FORMAT_R8_UNORM = 9,
        FORMAT_R8_SNORM = 10,
        FORMAT_R8_UINT = 13,
        FORMAT_R8_SINT = 14,
        FORMAT_R8_SRGB = 15,
        FORMAT_R8G8_UNORM = 16,
        FORMAT_R8G8_SNORM = 17,
        FORMAT_R8G8_UINT = 20,
        FORMAT_R8G8_SINT = 21,
        FORMAT_R8G8_SRGB = 22,
        FORMAT_R8G8B8_UNORM = 23,
        FORMAT_R8G8B8_SNORM = 24,
        FORMAT_R8G8B8_UINT = 27,
        FORMAT_R8G8B8_SINT = 28,
        FORMAT_R8G8B8_SRGB = 29,
        FORMAT_B8G8R8_UNORM = 30,
        FORMAT_B8G8R8_SNORM = 31,
        FORMAT_B8G8R8_UINT = 34,
        FORMAT_B8G8R8_SINT = 35,
        FORMAT_B8G8R8_SRGB = 36,
        FORMAT_R8G8B8A8_UNORM = 37,
        FORMAT_R8G8B8A8_SNORM = 38,
        FORMAT_R8G8B8A8_UINT = 41,
        FORMAT_R8G8B8A8_SINT = 42,
        FORMAT_R8G8B8A8_SRGB = 43,
        FORMAT_B8G8R8A8_UNORM = 44,
        FORMAT_B8G8R8A8_SNORM = 45,
        FORMAT_B8G8R8A8_UINT = 48,
        FORMAT_B8G8R8A8_SINT = 49,
        FORMAT_B8G8R8A8_SRGB = 50,

        FORMAT_A2R10G10B10_UNORM_PACK32 = 58,
        FORMAT_A2R10G10B10_SNORM_PACK32 = 59,
        FORMAT_A2R10G10B10_UINT_PACK32 = 62,
        FORMAT_A2R10G10B10_SINT_PACK32 = 63,
        FORMAT_A2B10G10R10_UNORM_PACK32 = 64,
        FORMAT_A2B10G10R10_SNORM_PACK32 = 65,
        FORMAT_A2B10G10R10_UINT_PACK32 = 68,
        FORMAT_A2B10G10R10_SINT_PACK32 = 69,

        FORMAT_R16_UNORM = 70,
        FORMAT_R16_SNORM = 71,
        FORMAT_R16_UINT = 74,
        FORMAT_R16_SINT = 75,
        FORMAT_R16_SFLOAT = 76,
        FORMAT_R16G16_UNORM = 77,
        FORMAT_R16G16_SNORM = 78,
        FORMAT_R16G16_UINT = 81,
        FORMAT_R16G16_SINT = 82,
        FORMAT_R16G16_SFLOAT = 83,
        FORMAT_R16G16B16_UNORM = 84,
        FORMAT_R16G16B16_SNORM = 85,
        FORMAT_R16G16B16_UINT = 88,
        FORMAT_R16G16B16_SINT = 89,
        FORMAT_R16G16B16_SFLOAT = 90,
        FORMAT_R16G16B16A16_UNORM = 91,
        FORMAT_R16G16B16A16_SNORM = 92,
        FORMAT_R16G16B16A16_UINT = 95,
        FORMAT_R16G16B16A16_SINT = 96,
        FORMAT_R16G16B16A16_SFLOAT = 97,

        FORMAT_R32_UINT = 98,
        FORMAT_R32_SINT = 99,
        FORMAT_R32_SFLOAT = 100,
        FORMAT_R32G32_UINT = 101,
        FORMAT_R32G32_SINT = 102,
        FORMAT_R32G32_SFLOAT = 103,
        FORMAT_R32G32B32_UINT = 104,
        FORMAT_R32G32B32_SINT = 105,
        FORMAT_R32G32B32_SFLOAT = 106,
        FORMAT_R32G32B32A32_UINT = 107,
        FORMAT_R32G32B32A32_SINT = 108,
        FORMAT_R32G32B32A32_SFLOAT = 109,

        FORMAT_R64_UINT = 110,
        FORMAT_R64_SINT = 111,
        FORMAT_R64_SFLOAT = 112,
        FORMAT_R64G64_UINT = 113,
        FORMAT_R64G64_SINT = 114,
        FORMAT_R64G64_SFLOAT = 115,
        FORMAT_R64G64B64_UINT = 116,
        FORMAT_R64G64B64_SINT = 117,
        FORMAT_R64G64B64_SFLOAT = 118,
        FORMAT_R64G64B64A64_UINT = 119,
        FORMAT_R64G64B64A64_SINT = 120,
        FORMAT_R64G64B64A64_SFLOAT = 121,
        FORMAT_B10G11R11_UFLOAT_PACK32 = 122,
        FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123,

        FORMAT_D16_UNORM = 124,
        FORMAT_X8_D24_UNORM_PACK32 = 125,
        FORMAT_D32_SFLOAT = 126,
        FORMAT_S8_UINT = 127,
        FORMAT_D16_UNORM_S8_UINT = 128,
        FORMAT_D24_UNORM_S8_UINT = 129,
        FORMAT_D32_SFLOAT_S8_UINT = 130,

        FORMAT_BC1_RGB_UNORM_BLOCK = 131,
        FORMAT_BC1_RGB_SRGB_BLOCK = 132,
        FORMAT_BC1_RGBA_UNORM_BLOCK = 133,
        FORMAT_BC1_RGBA_SRGB_BLOCK = 134,
        FORMAT_BC2_UNORM_BLOCK = 135,
        FORMAT_BC2_SRGB_BLOCK = 136,
        FORMAT_BC3_UNORM_BLOCK = 137,
        FORMAT_BC3_SRGB_BLOCK = 138,
        FORMAT_BC4_UNORM_BLOCK = 139,
        FORMAT_BC4_SNORM_BLOCK = 140,
        FORMAT_BC5_UNORM_BLOCK = 141,
        FORMAT_BC5_SNORM_BLOCK = 142,
        FORMAT_BC6H_UFLOAT_BLOCK = 143,
        FORMAT_BC6H_SFLOAT_BLOCK = 144,
        FORMAT_BC7_UNORM_BLOCK = 145,
        FORMAT_BC7_SRGB_BLOCK = 146,
        FORMAT_ETC2_R8G8B8_UNORM_BLOCK = 147,
        FORMAT_ETC2_R8G8B8_SRGB_BLOCK = 148,
        FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK = 149,
        FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK = 150,
        FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK = 151,
        FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK = 152,
        FORMAT_EAC_R11_UNORM_BLOCK = 153,
        FORMAT_EAC_R11_SNORM_BLOCK = 154,
        FORMAT_EAC_R11G11_UNORM_BLOCK = 155,
        FORMAT_EAC_R11G11_SNORM_BLOCK = 156,

        FORMAT_ASTC_4x4_UNORM_BLOCK = 157,
        FORMAT_ASTC_4x4_SRGB_BLOCK = 158,
        FORMAT_ASTC_5x4_UNORM_BLOCK = 159,
        FORMAT_ASTC_5x4_SRGB_BLOCK = 160,
        FORMAT_ASTC_5x5_UNORM_BLOCK = 161,
        FORMAT_ASTC_5x5_SRGB_BLOCK = 162,
        FORMAT_ASTC_6x5_UNORM_BLOCK = 163,
        FORMAT_ASTC_6x5_SRGB_BLOCK = 164,
        FORMAT_ASTC_6x6_UNORM_BLOCK = 165,
        FORMAT_ASTC_6x6_SRGB_BLOCK = 166,
        FORMAT_ASTC_8x5_UNORM_BLOCK = 167,
        FORMAT_ASTC_8x5_SRGB_BLOCK = 168,
        FORMAT_ASTC_8x6_UNORM_BLOCK = 169,
        FORMAT_ASTC_8x6_SRGB_BLOCK = 170,
        FORMAT_ASTC_8x8_UNORM_BLOCK = 171,
        FORMAT_ASTC_8x8_SRGB_BLOCK = 172,
        FORMAT_ASTC_10x5_UNORM_BLOCK = 173,
        FORMAT_ASTC_10x5_SRGB_BLOCK = 174,
        FORMAT_ASTC_10x6_UNORM_BLOCK = 175,
        FORMAT_ASTC_10x6_SRGB_BLOCK = 176,
        FORMAT_ASTC_10x8_UNORM_BLOCK = 177,
        FORMAT_ASTC_10x8_SRGB_BLOCK = 178,
        FORMAT_ASTC_10x10_UNORM_BLOCK = 179,
        FORMAT_ASTC_10x10_SRGB_BLOCK = 180,
        FORMAT_ASTC_12x10_UNORM_BLOCK = 181,
        FORMAT_ASTC_12x10_SRGB_BLOCK = 182,
        FORMAT_ASTC_12x12_UNORM_BLOCK = 183,
        FORMAT_ASTC_12x12_SRGB_BLOCK = 184,

        FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 = 1000156010,
        FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 = 1000156011,
        FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 = 1000156012,
        FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 = 1000156013,
        FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 = 1000156014,
        FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 = 1000156015,
        FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 = 1000156016,
        FORMAT_R12X4_UNORM_PACK16 = 1000156017,
        FORMAT_R12X4G12X4_UNORM_2PACK16 = 1000156018,
        FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16 = 1000156019,
        FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 = 1000156020,
        FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 = 1000156021,
        FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 = 1000156022,
        FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 = 1000156023,
        FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 = 1000156024,
        FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 = 1000156025,
        FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 = 1000156026,
        FORMAT_G16B16G16R16_422_UNORM = 1000156027,
        FORMAT_B16G16R16G16_422_UNORM = 1000156028,
        FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG = 1000054000,
        FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG = 1000054001,
        FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG = 1000054002,
        FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG = 1000054003,
        FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG = 1000054004,
        FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG = 1000054005,
        FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG = 1000054006,
        FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG = 1000054007,

        FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT = 1000066000,
        FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT = 1000066001,
        FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT = 1000066002,
        FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT = 1000066003,
        FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT = 1000066004,
        FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT = 1000066005,
        FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT = 1000066006,
        FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT = 1000066007,
        FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT = 1000066008,
        FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT = 1000066009,
        FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT = 1000066010,
        FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT = 1000066011,
        FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT = 1000066012,
        FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT = 1000066013,

        FORMAT_G8B8G8R8_422_UNORM_KHR        = FORMAT_G8B8G8R8_422_UNORM,
        FORMAT_B8G8R8G8_422_UNORM_KHR        = FORMAT_B8G8R8G8_422_UNORM,
        FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR = FORMAT_G8_B8_R8_3PLANE_420_UNORM,
        FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR  = FORMAT_G8_B8R8_2PLANE_420_UNORM,
        FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR = FORMAT_G8_B8_R8_3PLANE_422_UNORM,
        FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR  = FORMAT_G8_B8R8_2PLANE_422_UNORM,
        FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR = FORMAT_G8_B8_R8_3PLANE_444_UNORM,
        FORMAT_R10X6_UNORM_PACK16_KHR        = FORMAT_R10X6_UNORM_PACK16,
        FORMAT_R10X6G10X6_UNORM_2PACK16_KHR  = FORMAT_R10X6G10X6_UNORM_2PACK16,
        FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR = FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
        FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR = FORMAT_G16_B16_R16_3PLANE_420_UNORM,
        FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR  = FORMAT_G16_B16R16_2PLANE_420_UNORM,
        FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR = FORMAT_G16_B16_R16_3PLANE_422_UNORM,
        FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR  = FORMAT_G16_B16R16_2PLANE_422_UNORM,
        FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR = FORMAT_G16_B16_R16_3PLANE_444_UNORM,
    */

    struct HeaderKTX2
    {
        u32 vkFormat;
        u32 typeSize;
        u32 pixelWidth;
        u32 pixelHeight;
        u32 pixelDepth;
        u32 layerCount;
        u32 faceCount;
        u32 levelCount;
        u32 supercompressionScheme;

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
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ConstMemory m_memory;
        ImageHeader m_header;
        std::vector<LevelKTX2> m_levels;

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

            m_header.width = header.pixelWidth;
            m_header.height = header.pixelHeight;
            m_header.depth = header.pixelDepth;
            m_header.levels = header.levelCount;
            m_header.faces = header.faceCount;
            //m_header.format = ;
            //m_header.compression = ;

            printf("vkFormat: %d\n", header.vkFormat);
            printf("typeSize: %d\n", header.typeSize);
            printf("supercompressionScheme: %d\n", header.supercompressionScheme);

            switch (header.supercompressionScheme)
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

            printf("dfdByteOffset: %d, dfdByteLength: %d\n", dfdByteOffset, dfdByteLength);
            printf("kvdByteOffset: %d, kvdByteLength: %d\n", kvdByteOffset, kvdByteLength);
            printf("sgdByteOffset: %d, sgdByteLength: %d\n", (int)sgdByteOffset, (int)sgdByteLength);

            int levels = std::max(1, m_header.levels);
            printf("levels: %d\n", levels);

            for (int i = 0; i < levels; ++i)
            {
                LevelKTX2 level;

                level.offset = p.read64();
                level.length = p.read64();
                level.uncompressed_length = p.read64();

                m_levels.push_back(level);
                printf("  offset: %10d, length: %10d, uncompressed: %10d\n",  (int)level.offset, (int)level.length, (int)level.uncompressed_length);
            }

            // Data Format Descriptor
            u32 dfdTotalSize = p.read32();
            p += (dfdByteLength - 4);
            MANGO_UNREFERENCED(dfdTotalSize);

            // Key/Value Data
            p += kvdByteLength;

            // Supercompression Global Data
            if (sgdByteLength > 0)
            {
                p = align_pointer<8>(p);
            }

            p += sgdByteLength;
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
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            // TODO
            return ConstMemory();
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(dest);
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            // TODO
            ImageDecodeStatus status;
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

    void registerImageDecoderKTX2()
    {
        registerImageDecoder(createInterface, ".ktx2");
    }

} // namespace mango::image
