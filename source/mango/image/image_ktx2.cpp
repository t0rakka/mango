/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/compress.hpp>
#include <mango/image/image.hpp>
#include <mango/image/compression.hpp>
#include "../../external/basisu/transcoder/basisu_transcoder.h"
#include <map>

// MANGO TODO: more input validation so that fuzzing tests pass :)
// MANGO TODO: supercompression transcoding API to ImageDecoder interface.
/*
    Implementation note: The BASIS_LZ and UASTC supercompression schemes are
    meant as transcoders so that other (supported) block compression-formatted
    data can be extracted from the supercompressed data. We don't yet have
    API for this so we only support the decode-to-surface and can only get
    uncompressed rgba data out.
*/

namespace
{
    using namespace mango;
    using namespace mango::image;
    using namespace mango::vulkan;

    // ------------------------------------------------------------
    // Khronos Basic Data Format Descriptor Block
    // ------------------------------------------------------------

    // https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html#_anchor_id_basicdescriptor_xreflabel_basicdescriptor_khronos_basic_data_format_descriptor_block

    // Vendor ids
    enum : u32
    {
        KHR_DF_VENDORID_KHRONOS = 0U,
    };

    // Descriptor types
    enum : u32
    {
        KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT = 0U,
    };

    enum : u8
    {
        KHR_DF_MODEL_UNSPECIFIED  = 0U, // No interpretation of color channels defined
        KHR_DF_MODEL_RGBSDA       = 1U, // Color primaries (red, green, blue) + alpha, depth and stencil 
        KHR_DF_MODEL_YUVSDA       = 2U, // Color differences (Y', Cb, Cr) + alpha, depth and stencil 
        KHR_DF_MODEL_YIQSDA       = 3U, // Color differences (Y', I, Q) + alpha, depth and stencil         
        KHR_DF_MODEL_LABSDA       = 4U, // Perceptual color (CIE L*a*b*) + alpha, depth and stencil 
        KHR_DF_MODEL_CMYKA        = 5U, // Subtractive colors (cyan, magenta, yellow, black) + alpha 
        KHR_DF_MODEL_XYZW         = 6U, // Non-color coordinate data (X, Y, Z, W) 
        KHR_DF_MODEL_HSVA_ANG     = 7U, // Hue, saturation, value, hue angle on color circle, plus alpha 
        KHR_DF_MODEL_HSLA_ANG     = 8U, // Hue, saturation, lightness, hue angle on color circle, plus alpha 
        KHR_DF_MODEL_HSVA_HEX     = 7U, // Hue, saturation, value, hue on color hexagon, plus alpha 
        KHR_DF_MODEL_HSLA_HEX     = 8U, // Hue, saturation, lightness, hue on color hexagon, plus alpha 
        KHR_DF_MODEL_YCGCOA       = 9U, // Lightweight approximate color difference (luma, orange, green) 

        // Compressed formats start at 128
        KHR_DF_MODEL_DXT1A         = 128U,
        KHR_DF_MODEL_BC1A          = 128U,

        // DXT2/DXT3/BC2, with explicit 4-bit alpha
        KHR_DF_MODEL_DXT2          = 129U,
        KHR_DF_MODEL_DXT3          = 129U,
        KHR_DF_MODEL_BC2           = 129U,

        // DXT4/DXT5/BC3, with interpolated alpha
        KHR_DF_MODEL_DXT4          = 130U,
        KHR_DF_MODEL_DXT5          = 130U,
        KHR_DF_MODEL_BC3           = 130U,

        // BC4 - single channel interpolated 8-bit data
        KHR_DF_MODEL_BC4           = 131U,

        // BC5 - two channel interpolated 8-bit data
        KHR_DF_MODEL_BC5           = 132U,

        // BC6H - DX11 format for 16-bit float channels
        KHR_DF_MODEL_BC6H          = 133U,

        // BC7 - DX11 format
        KHR_DF_MODEL_BC7           = 134U,

        // Mobile compressed formats
        KHR_DF_MODEL_ETC1          = 160U,
        KHR_DF_MODEL_ETC2          = 161U,
        KHR_DF_MODEL_ASTC          = 162U,
        KHR_DF_MODEL_ETC1S         = 163U, // Basis Universal ETC1S Format
        KHR_DF_MODEL_PVRTC         = 164U,
        KHR_DF_MODEL_PVRTC2        = 165U,
        KDF_DF_MODEL_UASTC         = 166U, // Basis Universal UASTC Format
    };

    enum : u8
    {
        // Unspecified format with nominal channel numbering
        KHR_DF_CHANNEL_UNSPECIFIED_0  = 0U,
        KHR_DF_CHANNEL_UNSPECIFIED_1  = 1U,
        KHR_DF_CHANNEL_UNSPECIFIED_2  = 2U,
        KHR_DF_CHANNEL_UNSPECIFIED_3  = 3U,
        KHR_DF_CHANNEL_UNSPECIFIED_4  = 4U,
        KHR_DF_CHANNEL_UNSPECIFIED_5  = 5U,
        KHR_DF_CHANNEL_UNSPECIFIED_6  = 6U,
        KHR_DF_CHANNEL_UNSPECIFIED_7  = 7U,
        KHR_DF_CHANNEL_UNSPECIFIED_8  = 8U,
        KHR_DF_CHANNEL_UNSPECIFIED_9  = 9U,
        KHR_DF_CHANNEL_UNSPECIFIED_10 = 10U,
        KHR_DF_CHANNEL_UNSPECIFIED_11 = 11U,
        KHR_DF_CHANNEL_UNSPECIFIED_12 = 12U,
        KHR_DF_CHANNEL_UNSPECIFIED_13 = 13U,
        KHR_DF_CHANNEL_UNSPECIFIED_14 = 14U,
        KHR_DF_CHANNEL_UNSPECIFIED_15 = 15U,

        // MODEL_RGBSDA - red, green, blue, stencil, depth, alpha
        KHR_DF_CHANNEL_RGBSDA_RED     =  0U,
        KHR_DF_CHANNEL_RGBSDA_R       =  0U,
        KHR_DF_CHANNEL_RGBSDA_GREEN   =  1U,
        KHR_DF_CHANNEL_RGBSDA_G       =  1U,
        KHR_DF_CHANNEL_RGBSDA_BLUE    =  2U,
        KHR_DF_CHANNEL_RGBSDA_B       =  2U,
        KHR_DF_CHANNEL_RGBSDA_STENCIL = 13U,
        KHR_DF_CHANNEL_RGBSDA_S       = 13U,
        KHR_DF_CHANNEL_RGBSDA_DEPTH   = 14U,
        KHR_DF_CHANNEL_RGBSDA_D       = 14U,
        KHR_DF_CHANNEL_RGBSDA_ALPHA   = 15U,
        KHR_DF_CHANNEL_RGBSDA_A       = 15U,

        // MODEL_YUVSDA - luma, Cb, Cr, stencil, depth, alpha
        KHR_DF_CHANNEL_YUVSDA_Y       =  0U,
        KHR_DF_CHANNEL_YUVSDA_CB      =  1U,
        KHR_DF_CHANNEL_YUVSDA_U       =  1U,
        KHR_DF_CHANNEL_YUVSDA_CR      =  2U,
        KHR_DF_CHANNEL_YUVSDA_V       =  2U,
        KHR_DF_CHANNEL_YUVSDA_STENCIL = 13U,
        KHR_DF_CHANNEL_YUVSDA_S       = 13U,
        KHR_DF_CHANNEL_YUVSDA_DEPTH   = 14U,
        KHR_DF_CHANNEL_YUVSDA_D       = 14U,
        KHR_DF_CHANNEL_YUVSDA_ALPHA   = 15U,
        KHR_DF_CHANNEL_YUVSDA_A       = 15U,

        // MODEL_YIQSDA - luma, in-phase, quadrature, stencil, depth, alpha
        KHR_DF_CHANNEL_YIQSDA_Y       =  0U,
        KHR_DF_CHANNEL_YIQSDA_I       =  1U,
        KHR_DF_CHANNEL_YIQSDA_Q       =  2U,
        KHR_DF_CHANNEL_YIQSDA_STENCIL = 13U,
        KHR_DF_CHANNEL_YIQSDA_S       = 13U,
        KHR_DF_CHANNEL_YIQSDA_DEPTH   = 14U,
        KHR_DF_CHANNEL_YIQSDA_D       = 14U,
        KHR_DF_CHANNEL_YIQSDA_ALPHA   = 15U,
        KHR_DF_CHANNEL_YIQSDA_A       = 15U,

        // MODEL_LABSDA - CIELAB/L*a*b* luma, red-green, blue-yellow, stencil, depth, alpha
        KHR_DF_CHANNEL_LABSDA_L       =  0U,
        KHR_DF_CHANNEL_LABSDA_A       =  1U,
        KHR_DF_CHANNEL_LABSDA_B       =  2U,
        KHR_DF_CHANNEL_LABSDA_STENCIL = 13U,
        KHR_DF_CHANNEL_LABSDA_S       = 13U,
        KHR_DF_CHANNEL_LABSDA_DEPTH   = 14U,
        KHR_DF_CHANNEL_LABSDA_D       = 14U,
        KHR_DF_CHANNEL_LABSDA_ALPHA   = 15U,

        // MODEL_CMYKA - cyan, magenta, yellow, key/blacK, alpha
        KHR_DF_CHANNEL_CMYKSDA_CYAN    =  0U,
        KHR_DF_CHANNEL_CMYKSDA_C       =  0U,
        KHR_DF_CHANNEL_CMYKSDA_MAGENTA =  1U,
        KHR_DF_CHANNEL_CMYKSDA_M       =  1U,
        KHR_DF_CHANNEL_CMYKSDA_YELLOW  =  2U,
        KHR_DF_CHANNEL_CMYKSDA_Y       =  2U,
        KHR_DF_CHANNEL_CMYKSDA_KEY     =  3U,
        KHR_DF_CHANNEL_CMYKSDA_BLACK   =  3U,
        KHR_DF_CHANNEL_CMYKSDA_K       =  3U,
        KHR_DF_CHANNEL_CMYKSDA_ALPHA   = 15U,
        KHR_DF_CHANNEL_CMYKSDA_A       = 15U,

        // MODEL_XYZW - coordinates x, y, z, w
        KHR_DF_CHANNEL_XYZW_X          = 0U,
        KHR_DF_CHANNEL_XYZW_Y          = 1U,
        KHR_DF_CHANNEL_XYZW_Z          = 2U,
        KHR_DF_CHANNEL_XYZW_W          = 3U,

        // MODEL_HSVA_ANG - value (luma), saturation, hue, alpha, angular projection, conical space
        KHR_DF_CHANNEL_HSVA_ANG_VALUE      = 0U,
        KHR_DF_CHANNEL_HSVA_ANG_V          = 0U,
        KHR_DF_CHANNEL_HSVA_ANG_SATURATION = 1U,
        KHR_DF_CHANNEL_HSVA_ANG_S          = 1U,
        KHR_DF_CHANNEL_HSVA_ANG_HUE        = 2U,
        KHR_DF_CHANNEL_HSVA_ANG_H          = 2U,
        KHR_DF_CHANNEL_HSVA_ANG_ALPHA      = 15U,
        KHR_DF_CHANNEL_HSVA_ANG_A          = 15U,

        // MODEL_HSLA_ANG - lightness (luma), saturation, hue, alpha, angular projection, double conical space
        KHR_DF_CHANNEL_HSLA_ANG_LIGHTNESS  = 0U,
        KHR_DF_CHANNEL_HSLA_ANG_L          = 0U,
        KHR_DF_CHANNEL_HSLA_ANG_SATURATION = 1U,
        KHR_DF_CHANNEL_HSLA_ANG_S          = 1U,
        KHR_DF_CHANNEL_HSLA_ANG_HUE        = 2U,
        KHR_DF_CHANNEL_HSLA_ANG_H          = 2U,
        KHR_DF_CHANNEL_HSLA_ANG_ALPHA      = 15U,
        KHR_DF_CHANNEL_HSLA_ANG_A          = 15U,

        // MODEL_HSVA_HEX - value (luma), saturation, hue, alpha, hexagonal projection, conical space
        KHR_DF_CHANNEL_HSVA_HEX_VALUE      = 0U,
        KHR_DF_CHANNEL_HSVA_HEX_V          = 0U,
        KHR_DF_CHANNEL_HSVA_HEX_SATURATION = 1U,
        KHR_DF_CHANNEL_HSVA_HEX_S          = 1U,
        KHR_DF_CHANNEL_HSVA_HEX_HUE        = 2U,
        KHR_DF_CHANNEL_HSVA_HEX_H          = 2U,
        KHR_DF_CHANNEL_HSVA_HEX_ALPHA      = 15U,
        KHR_DF_CHANNEL_HSVA_HEX_A          = 15U,

        // MODEL_HSLA_HEX - lightness (luma), saturation, hue, alpha, hexagonal projection, double conical space
        KHR_DF_CHANNEL_HSLA_HEX_LIGHTNESS  = 0U,
        KHR_DF_CHANNEL_HSLA_HEX_L          = 0U,
        KHR_DF_CHANNEL_HSLA_HEX_SATURATION = 1U,
        KHR_DF_CHANNEL_HSLA_HEX_S          = 1U,
        KHR_DF_CHANNEL_HSLA_HEX_HUE        = 2U,
        KHR_DF_CHANNEL_HSLA_HEX_H          = 2U,
        KHR_DF_CHANNEL_HSLA_HEX_ALPHA      = 15U,
        KHR_DF_CHANNEL_HSLA_HEX_A          = 15U,

        // MODEL_YCGCOA - luma, green delta, orange delta, alpha
        KHR_DF_CHANNEL_YCGCOA_Y       =  0U,
        KHR_DF_CHANNEL_YCGCOA_CG      =  1U,
        KHR_DF_CHANNEL_YCGCOA_CO      =  2U,
        KHR_DF_CHANNEL_YCGCOA_ALPHA   = 15U,
        KHR_DF_CHANNEL_YCGCOA_A       = 15U,

        // MODEL_DXT1A/MODEL_BC1A
        KHR_DF_CHANNEL_DXT1A_COLOR = 0U,
        KHR_DF_CHANNEL_BC1A_COLOR  = 0U,
        KHR_DF_CHANNEL_DXT1A_ALPHAPRESENT = 1U,
        KHR_DF_CHANNEL_BC1A_ALPHAPRESENT  = 1U,

        // MODEL_DXT2/3/MODEL_BC2
        KHR_DF_CHANNEL_DXT2_COLOR =  0U,
        KHR_DF_CHANNEL_DXT3_COLOR =  0U,
        KHR_DF_CHANNEL_BC2_COLOR  =  0U,
        KHR_DF_CHANNEL_DXT2_ALPHA = 15U,
        KHR_DF_CHANNEL_DXT3_ALPHA = 15U,
        KHR_DF_CHANNEL_BC2_ALPHA  = 15U,

        // MODEL_DXT4/5/MODEL_BC3
        KHR_DF_CHANNEL_DXT4_COLOR =  0U,
        KHR_DF_CHANNEL_DXT5_COLOR =  0U,
        KHR_DF_CHANNEL_BC3_COLOR  =  0U,
        KHR_DF_CHANNEL_DXT4_ALPHA = 15U,
        KHR_DF_CHANNEL_DXT5_ALPHA = 15U,
        KHR_DF_CHANNEL_BC3_ALPHA  = 15U,

        // MODEL_BC4
        KHR_DF_CHANNEL_BC4_DATA   = 0U,

        // MODEL_BC5
        KHR_DF_CHANNEL_BC5_RED    = 0U,
        KHR_DF_CHANNEL_BC5_R      = 0U,
        KHR_DF_CHANNEL_BC5_GREEN  = 1U,
        KHR_DF_CHANNEL_BC5_G      = 1U,

        // MODEL_BC6H
        KHR_DF_CHANNEL_BC6H_COLOR = 0U,

        // MODEL_BC7
        KHR_DF_CHANNEL_BC7_DATA   = 0U,

        // MODEL_ETC1
        KHR_DF_CHANNEL_ETC1_DATA  = 0U,
        KHR_DF_CHANNEL_ETC1_COLOR = 0U,

        // MODEL_ETC2
        KHR_DF_CHANNEL_ETC2_RED   = 0U,
        KHR_DF_CHANNEL_ETC2_R     = 0U,
        KHR_DF_CHANNEL_ETC2_GREEN = 1U,
        KHR_DF_CHANNEL_ETC2_G     = 1U,
        KHR_DF_CHANNEL_ETC2_COLOR = 2U,
        KHR_DF_CHANNEL_ETC2_ALPHA = 15U,
        KHR_DF_CHANNEL_ETC2_A     = 15U,

        // MODEL_ASTC
        KHR_DF_CHANNEL_ASTC_DATA  = 0U,

        // Common channel names shared by multiple formats
        KHR_DF_CHANNEL_COMMON_LUMA    =  0U,
        KHR_DF_CHANNEL_COMMON_L       =  0U,
        KHR_DF_CHANNEL_COMMON_STENCIL = 13U,
        KHR_DF_CHANNEL_COMMON_S       = 13U,
        KHR_DF_CHANNEL_COMMON_DEPTH   = 14U,
        KHR_DF_CHANNEL_COMMON_D       = 14U,
        KHR_DF_CHANNEL_COMMON_ALPHA   = 15U,
        KHR_DF_CHANNEL_COMMON_A       = 15U,

        // Basis Universal ETC1S Format
        KHR_DF_CHANNEL_ETC1S_RGB      = 0,
        KHR_DF_CHANNEL_ETC1S_RRR      = 3,
        KHR_DF_CHANNEL_ETC1S_GGG      = 4,
        KHR_DF_CHANNEL_ETC1S_AAA      = 15,

        // Basis Universal UASTC Format
        KHR_DF_CHANNEL_UASTC_RGB      = 0,
        KHR_DF_CHANNEL_UASTC_RGBA     = 3,
        KHR_DF_CHANNEL_UASTC_RRR      = 4,
        KHR_DF_CHANNEL_UASTC_RRRG     = 5,
        KHR_DF_CHANNEL_UASTC_RG       = 6,
    };

    enum : u8
    {
        KHR_DF_PRIMARIES_UNSPECIFIED = 0U, // No color primaries defined
        KHR_DF_PRIMARIES_BT709       = 1U, // Color primaries of ITU-R BT.709 and sRGB
        KHR_DF_PRIMARIES_SRGB        = 1U, // Synonym for KHR_DF_PRIMARIES_BT709
        KHR_DF_PRIMARIES_BT601_EBU   = 2U, // Color primaries of ITU-R BT.601 (625-line EBU variant)
        KHR_DF_PRIMARIES_BT601_SMPTE = 3U, // Color primaries of ITU-R BT.601 (525-line SMPTE C variant)
        KHR_DF_PRIMARIES_BT2020      = 4U, // Color primaries of ITU-R BT.2020
        KHR_DF_PRIMARIES_CIEXYZ      = 5U, // CIE theoretical color coordinate space
        KHR_DF_PRIMARIES_ACES        = 6U, // Academy Color Encoding System primaries
    };

    enum : u8
    {
        KHR_DF_TRANSFER_UNSPECIFIED = 0U, // No transfer function defined
        KHR_DF_TRANSFER_LINEAR      = 1U, // Linear transfer function (value proportional to intensity)
        KHR_DF_TRANSFER_SRGB        = 2U, // Perceptually-linear transfer function of sRGH (~2.4)
        KHR_DF_TRANSFER_ITU         = 3U, // Perceptually-linear transfer function of ITU specifications (~1/.45)
        KHR_DF_TRANSFER_NTSC        = 4U, // Perceptually-linear gamma function of NTSC (simple 2.2 gamma)
        KHR_DF_TRANSFER_SLOG        = 5U, // Sony S-log used by Sony video cameras
        KHR_DF_TRANSFER_SLOG2       = 6U, // Sony S-log 2 used by Sony video cameras
    };

    enum : u32
    {
        KHR_DF_FLAG_ALPHA_STRAIGHT      = 0U,
        KHR_DF_FLAG_ALPHA_PREMULTIPLIED = 1U
    };

    enum : u8
    {
        KHR_DF_SAMPLE_DATATYPE_LINEAR   = 1U << 4U,
        KHR_DF_SAMPLE_DATATYPE_EXPONENT = 1U << 5U,
        KHR_DF_SAMPLE_DATATYPE_SIGNED   = 1U << 6U,
        KHR_DF_SAMPLE_DATATYPE_FLOAT    = 1U << 7U
    };

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
        u32 compression;
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
        { Format(), TextureCompression::ASTC_UNORM_4x4, "ASTC_4x4_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_4x4, "ASTC_4x4_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_5x4, "ASTC_5x4_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_5x4, "ASTC_5x4_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_5x5, "ASTC_5x5_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_5x5, "ASTC_5x5_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_6x5, "ASTC_6x5_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_6x5, "ASTC_6x5_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_6x6, "ASTC_6x6_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_6x6, "ASTC_6x6_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_8x5, "ASTC_8x5_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_8x5, "ASTC_8x5_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_8x6, "ASTC_8x6_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_8x6, "ASTC_8x6_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_8x8, "ASTC_8x8_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_8x8, "ASTC_8x8_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_10x5, "ASTC_10x5_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_10x5, "ASTC_10x5_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_10x6, "ASTC_10x6_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_10x6, "ASTC_10x6_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_10x8, "ASTC_10x8_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_10x8, "ASTC_10x8_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_10x10, "ASTC_10x10_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_10x10, "ASTC_10x10_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_12x10, "ASTC_12x10_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_12x10, "ASTC_12x10_SRGB_BLOCK" },
        { Format(), TextureCompression::ASTC_UNORM_12x12, "ASTC_12x12_UNORM_BLOCK" },
        { Format(), TextureCompression::ASTC_SRGB_12x12, "ASTC_12x12_SRGB_BLOCK" },
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
        { 1000066000, { Format(), TextureCompression::ASTC_FLOAT_4x4, "ASTC_4x4_SFLOAT_BLOCK" } },
        { 1000066001, { Format(), TextureCompression::ASTC_FLOAT_5x4, "ASTC_5x4_SFLOAT_BLOCK" } },
        { 1000066002, { Format(), TextureCompression::ASTC_FLOAT_5x5, "ASTC_5x5_SFLOAT_BLOCK" } },
        { 1000066003, { Format(), TextureCompression::ASTC_FLOAT_6x5, "ASTC_6x5_SFLOAT_BLOCK" } },
        { 1000066004, { Format(), TextureCompression::ASTC_FLOAT_6x6, "ASTC_6x6_SFLOAT_BLOCK" } },
        { 1000066005, { Format(), TextureCompression::ASTC_FLOAT_8x5, "ASTC_8x5_SFLOAT_BLOCK" } },
        { 1000066006, { Format(), TextureCompression::ASTC_FLOAT_8x6, "ASTC_8x6_SFLOAT_BLOCK" } },
        { 1000066007, { Format(), TextureCompression::ASTC_FLOAT_8x8, "ASTC_8x8_SFLOAT_BLOCK" } },
        { 1000066008, { Format(), TextureCompression::ASTC_FLOAT_10x5, "ASTC_10x5_SFLOAT_BLOCK" } },
        { 1000066009, { Format(), TextureCompression::ASTC_FLOAT_10x6, "ASTC_10x6_SFLOAT_BLOCK" } },
        { 1000066010, { Format(), TextureCompression::ASTC_FLOAT_10x8, "ASTC_10x8_SFLOAT_BLOCK" } },
        { 1000066011, { Format(), TextureCompression::ASTC_FLOAT_10x10, "ASTC_10x10_SFLOAT_BLOCK" } },
        { 1000066012, { Format(), TextureCompression::ASTC_FLOAT_12x10, "ASTC_12x10_SFLOAT_BLOCK" } },
        { 1000066013, { Format(), TextureCompression::ASTC_FLOAT_12x12, "ASTC_12x12_SFLOAT_BLOCK" } },
    };

    static
    VulkanFormatDesc getFormatDesc(u32 vkformat)
    {
        VulkanFormatDesc desc;

        const u32 maxIndex = u32(std::size(g_vulkan_format_array));

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
            TextureCompression info(desc.compression);
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
            constexpr u8 identifier [] =
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
    // Basis Universal Helper Classes
    // ------------------------------------------------------------

    struct BasisImageDesc
    {
        u32 imageFlags;
        u32 rgbSliceByteOffset;
        u32 rgbSliceByteLength;
        u32 alphaSliceByteOffset;
        u32 alphaSliceByteLength;
    };

    struct BasisLZ
    {
        u16 endpointCount;
        u16 selectorCount;
        u32 endpointsByteLength;
        u32 selectorsByteLength;
        u32 tablesByteLength;
        u32 extendedByteLength;

        const u8* imageDescData;

        const u8* endpointsData;
        const u8* selectorsData;
        const u8* tablesData;
        const u8* extendedData;

        void read(ConstMemory memory, int imageCount)
        {
            LittleEndianConstPointer p = memory.address;

            endpointCount = p.read16();
            selectorCount = p.read16();
            endpointsByteLength = p.read32();
            selectorsByteLength = p.read32();
            tablesByteLength = p.read32();
            extendedByteLength = p.read32();

            printLine(Print::Info, "");
            printLine(Print::Info, "[BasisLZ]");
            printLine(Print::Info, "  endpointCount: {}", endpointCount);
            printLine(Print::Info, "  selectorCount: {}", selectorCount);
            printLine(Print::Info, "  endpointsByteLength: {}", endpointsByteLength);
            printLine(Print::Info, "  selectorsByteLength: {}", selectorsByteLength);
            printLine(Print::Info, "  tablesByteLength:    {}", tablesByteLength);
            printLine(Print::Info, "  extendedByteLength:  {}", extendedByteLength);

            imageDescData = p;
            p += imageCount * 20;

            endpointsData = p; p += endpointsByteLength;
            selectorsData = p; p += selectorsByteLength;
            tablesData    = p; p += tablesByteLength;
            extendedData  = p;
        }

        BasisImageDesc readImageDesc(int imageIndex) const
        {
            LittleEndianConstPointer p = imageDescData + imageIndex * 20;

            BasisImageDesc desc;

            desc.imageFlags = p.read32();
            desc.rgbSliceByteOffset = p.read32();
            desc.rgbSliceByteLength = p.read32();
            desc.alphaSliceByteOffset = p.read32();
            desc.alphaSliceByteLength = p.read32();

            printLine(Print::Info, "");
            printLine(Print::Info, "[BasisImageDesc]");
            printLine(Print::Info, "  rgb offset: {}, length: {}", desc.rgbSliceByteOffset, desc.rgbSliceByteLength);
            printLine(Print::Info, "  alpha offset: {}, length: {}", desc.alphaSliceByteOffset, desc.alphaSliceByteLength);

            return desc;
        }
    };

    static
    std::mutex g_basis_mutex;

    static
    void initialize_basis()
    {
        g_basis_mutex.lock();

        // This should only be called once but it does have initialization
        // flag internally so we just want to make sure there are no concurrent
        // callers entering the initialization while it may be running.
        basist::basisu_transcoder_init();

        g_basis_mutex.unlock();
    }


    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ConstMemory m_memory;
        ImageHeader m_header;

        std::vector<LevelKTX2> m_levels;
        BasisLZ m_basis;

        bool m_is_etc1s = false;
        bool m_is_uastc = false;

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
                printLine(Print::Info, "[KTX2] Incorrect identifier.");
                return;
            }

            if (isFormatProhibited(header.vkFormat))
            {
                printLine(Print::Info, "[KTX2] Prohibited format.");
                return;
            }

            VulkanFormatDesc desc = getFormatDesc(header.vkFormat);

            m_header.width = header.pixelWidth;
            m_header.height = header.pixelHeight;
            m_header.depth = std::max(1u, header.pixelDepth);
            m_header.levels = header.levelCount;
            m_header.faces = header.faceCount;
            m_header.format = desc.format;
            m_header.compression = desc.compression;
            m_header.linear = (desc.compression & TextureCompression::SRGB) == 0;

            printLine(Print::Info, "");
            printLine(Print::Info, "[HeaderKTX2]");
            printLine(Print::Info, "  vkFormat: {} \"{}\"", header.vkFormat, desc.name);
            printLine(Print::Info, "  typeSize: {}", header.typeSize);
            printLine(Print::Info, "  supercompressionScheme: {}", header.supercompressionScheme);

            m_supercompression = header.supercompressionScheme;

            switch (m_supercompression)
            {
                case SUPERCOMPRESSION_NONE:
                    break;
                case SUPERCOMPRESSION_BASIS_LZ:
                    m_is_etc1s = true;
                    m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    m_header.linear = false;
                    m_header.supercompression = SUPERCOMPRESS_BASISU_ETC1S;
                    break;
                case SUPERCOMPRESSION_ZSTANDARD:
                    break;
                case SUPERCOMPRESSION_ZLIB:
                    break;
                default:
                    // Unsupported / Incorrect compression scheme
                    m_header = ImageHeader();
                    return;
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

            printLine(Print::Info, "  dfdByteOffset: {}, dfdByteLength: {}", dfdByteOffset, dfdByteLength);
            printLine(Print::Info, "  kvdByteOffset: {}, kvdByteLength: {}", kvdByteOffset, kvdByteLength);
            printLine(Print::Info, "  sgdByteOffset: {}, sgdByteLength: {}", sgdByteOffset, sgdByteLength);

            int levels = std::max(1, m_header.levels);
            printLine(Print::Info, "");
            printLine(Print::Info, "[levels: {}]", levels);

            for (int i = 0; i < levels; ++i)
            {
                LevelKTX2 level;

                level.offset = p.read64();
                level.length = p.read64();
                level.uncompressed_length = p.read64();
                level.memory = ConstMemory(m_memory.address + level.offset, level.length);

                m_levels.push_back(level);
                printLine(Print::Info, "  offset: {}, length: {}, uncompressed: {}", level.offset, level.length, level.uncompressed_length);
            }

            // Data Format Descriptor
            if (dfdByteLength)
            {
                p = memory.address + dfdByteOffset;
                const u8* end = p + dfdByteLength;

                p += 4; // skip totalSize

                while (p < end)
                {
                    u32 v0 = p.read32();
                    u32 v1 = p.read32();

                    u32 vendor_id = v0 & 0x1ffff;
                    u32 descriptor_type = v0 >> 17;
                    u32 version_number = v1 & 0xffff;
                    u32 descriptor_block_size = v1 >> 16;

                    printLine(Print::Info, "");
                    printLine(Print::Info, "[DataFormatDescriptor]");
                    printLine(Print::Info, "  vendor: {}, version: {}", vendor_id, version_number);
                    printLine(Print::Info, "  type: {}, size: {}", descriptor_type, descriptor_block_size);

                    u8 colorModel           = p[0];
                    u8 colorPrimaries       = p[1];
                    u8 transferFunction     = p[2];
                    u8 flags                = p[3];
                    u8 texelBlockDimension0 = p[4];
                    u8 texelBlockDimension1 = p[5];
                    u8 texelBlockDimension2 = p[6];
                    u8 texelBlockDimension3 = p[7];
                    p += 8;

                    switch (colorModel)
                    {
                        case KHR_DF_MODEL_ETC1S:
                            // NOTE: This should already have been detected as supercompression
                            break;
                        case KDF_DF_MODEL_UASTC:
                            m_is_uastc = true;
                            m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                            m_header.linear = false;
                            m_header.supercompression = SUPERCOMPRESS_BASISU_UASTC;
                            break;
                    }

                    texelBlockDimension0 += !!texelBlockDimension0;
                    texelBlockDimension1 += !!texelBlockDimension1;
                    texelBlockDimension2 += !!texelBlockDimension2;
                    texelBlockDimension3 += !!texelBlockDimension3;

                    printLine(Print::Info, "  colorModel: {}", colorModel);
                    printLine(Print::Info, "  colorPrimaries: {}", colorPrimaries);
                    printLine(Print::Info, "  transferFunction: {}", transferFunction);
                    printLine(Print::Info, "  flags: {}", flags);
                    printLine(Print::Info, "  dimensions: {} {} {} {}",
                        texelBlockDimension0, texelBlockDimension1,
                        texelBlockDimension2, texelBlockDimension3);

                    u8 bytesPlane0 = p[0];
                    u8 bytesPlane1 = p[1];
                    u8 bytesPlane2 = p[2];
                    u8 bytesPlane3 = p[3];
                    u8 bytesPlane4 = p[4];
                    u8 bytesPlane5 = p[5];
                    u8 bytesPlane6 = p[6];
                    u8 bytesPlane7 = p[7];
                    p += 8;

                    printLine(Print::Info, "  planes: {} {} {} {} {} {} {} {}",
                        bytesPlane0, bytesPlane1, bytesPlane2, bytesPlane3,
                        bytesPlane4, bytesPlane5, bytesPlane6, bytesPlane7);

                    u32 sample_count = (descriptor_block_size - 24) / 16;

                    for (u32 i = 0; i < sample_count; ++i)
                    {
                        p += 16; // skip
                    }
                }
            }

            m_header.format.setLinear(m_header.linear);

            // Key / Value Data
            if (kvdByteLength)
            {
                p = memory.address + kvdByteOffset;
                const u8* end = p + kvdByteLength;

                printLine(Print::Info, "");
                printLine(Print::Info, "[Key/Value Data]");

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
                    else if (!strcmp(key, "KTXswizzle"))
                    {
                        // MANGO TODO: this modifies m_header.format
                    }

                    printLine(Print::Info, "  {}", key);

                    p += length;
                    p += padding;
                }
            }

            if (m_orientation_y)
            {
                // MANGO TODO: compressed format origin is at bottom
            }

            // Supercompression Global Data
            if (sgdByteLength)
            {
                if (m_supercompression == SUPERCOMPRESSION_BASIS_LZ)
                {
                    int imageCount = std::max(1u, header.levelCount) * header.faceCount;
                    m_basis.read(ConstMemory(memory.address + sgdByteOffset, sgdByteLength), imageCount);
                }
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
            if (level < 0 || level >= int(m_levels.size()))
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

            if (m_orientation_z)
            {
                depth = m_header.depth - (depth + 1);
            }

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
            MANGO_UNREFERENCED(options);

            decompress();

            // MANGO TODO: typesize dictates endianness swap on big-endian

            ImageDecodeStatus status;

            const int maxLevel = int(m_levels.size() - 1);
            if (level < 0 || level > maxLevel)
            {
                status.setError("Incorrect level ({}) [{} .. {}]", level, 0, maxLevel);
                return status;
            }

            int width = std::max(1, m_header.width >> level);
            int height = std::max(1, m_header.height >> level);
            const Format& format = m_header.format;

            if (m_is_etc1s)
            {
#ifdef MANGO_LICENSE_ENABLE_APACHE
                ConstMemory memory = this->memory(level, depth, 0);

                Bitmap temp(width, height, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
                //printLine(Print::Info, "memory: {} bytes", memory.size);

                initialize_basis();
                basist::basisu_lowlevel_etc1s_transcoder transcoder;

                transcoder.decode_palettes(
                    m_basis.endpointCount, m_basis.endpointsData, m_basis.endpointsByteLength,
                    m_basis.selectorCount, m_basis.selectorsData, m_basis.selectorsByteLength);
                transcoder.decode_tables(m_basis.tablesData, m_basis.tablesByteLength);

                int xblocks = std::max(1, width / 4);
                int yblocks = std::max(1, height / 4);

                const int imageIndex = level * m_header.faces + face;
                BasisImageDesc desc = m_basis.readImageDesc(imageIndex);

                bool x = transcoder.transcode_image(basist::transcoder_texture_format::cTFRGBA32,
                    temp.image, width * height,
                    memory.address, u32(memory.size),
                    xblocks, yblocks, width, height,
                    level,
                    desc.rgbSliceByteOffset, desc.rgbSliceByteLength,
                    desc.alphaSliceByteOffset, desc.alphaSliceByteLength);

                MANGO_UNREFERENCED(x);

                dest.blit(0, 0, temp);
#endif
            }
            else if (m_is_uastc)
            {
#ifdef MANGO_LICENSE_ENABLE_APACHE
                ConstMemory memory = this->memory(level, depth, face);

                Bitmap temp(width, height, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

                initialize_basis();
                basist::basisu_lowlevel_uastc_transcoder transcoder;

                int xblocks = std::max(1, width / 4);
                int yblocks = std::max(1, height / 4);

                u32 slice_offset = 0;
                u32 slice_length = u32(memory.size);

                bool x = transcoder.transcode_image(basist::transcoder_texture_format::cTFRGBA32,
                    temp.image, width * height,
                    memory.address, u32(memory.size),
                    xblocks, yblocks, width, height, level,
                    slice_offset, slice_length);

                MANGO_UNREFERENCED(x);

                dest.blit(0, 0, temp);
#endif
            }
            else
            {
                ConstMemory memory = this->memory(level, depth, face);

                if (m_header.compression != TextureCompression::NONE)
                {
                    TextureCompression info(m_header.compression);
                    TextureCompression::Status ts = info.decompress(dest, memory);
                    if (!ts)
                    {
                        status.setError(ts.info);
                    }
                }
                else
                {
                    printLine(Print::Info, "surface: {} x {} ({} bits)", width, height, format.bits);
                    printLine(Print::Info, "memory: {} bytes", memory.size);

                    // The image data is uncompressed in the file
                    Surface temp(width, height, format, width * format.bytes(), memory.address);

                    // Mirror the image when required
                    if (m_orientation_y)
                    {
                        temp.image += temp.stride * (height - 1);
                        temp.stride = 0 - temp.stride;
                    }

                    dest.blit(0, 0, temp);
                }
            }

            return status;
        }

        void decompress()
        {
            if (m_supercompression > SUPERCOMPRESSION_BASIS_LZ)
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
                            case SUPERCOMPRESSION_ZSTANDARD:
                                status = zstd::decompress(dest, level.memory);
                                break;
#ifdef MANGO_LICENSE_ENABLE_ZLIB
                            case SUPERCOMPRESSION_ZLIB:
                                status = zlib::decompress(dest, level.memory);
                                break;
#endif
                        }

                        if (status)
                        {
                            printLine(Print::Info, "* decompressed: {} bytes", status.size);
                        }
                        else
                        {
                            printLine(Print::Info, "* decompress status: {}", status.info);
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

    void registerImageCodecKTX2()
    {
        registerImageDecoder(createInterface, ".ktx2");
    }

} // namespace mango::image
