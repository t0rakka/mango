/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

namespace mango::image
{

    void decode_block_dxt1            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_dxt1a           (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_dxt3            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_dxt5            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_3dc_x           (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_3dc_xy          (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_uyvy            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_yuy2            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_grgb8           (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_rgbg8           (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_rgb9e5          (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_r11f_g11f_b10f  (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_r10f_g11f_b11f  (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_bitplane1       (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_atc             (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_atc_e           (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_atc_i           (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_fxt1_rgb        (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_fxt1_rgba       (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_etc1            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_etc2            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_etc2_eac        (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_eac_r11         (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_eac_rg11        (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_bc4u            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_bc4s            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_bc5u            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_bc5s            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_bc6hu           (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_bc6hs           (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void decode_block_bc7             (const TextureCompression& info, u8* output, const u8* input, size_t stride);

    void encode_block_bc1             (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void encode_block_bc1a            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void encode_block_bc2             (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void encode_block_bc3             (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void encode_block_bc4u            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void encode_block_bc4s            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void encode_block_bc5u            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void encode_block_bc5s            (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void encode_block_bc6hu           (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void encode_block_bc6hs           (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void encode_block_bc7             (const TextureCompression& info, u8* output, const u8* input, size_t stride);
    void encode_block_etc1            (const TextureCompression& info, u8* output, const u8* input, size_t stride);

    void decode_surface_astc          (const TextureCompression& info, const Surface& output, const u8* input);
    void decode_surface_pvrtc         (const TextureCompression& info, const Surface& output, const u8* input);
    void decode_surface_pvrtc2        (const TextureCompression& info, const Surface& output, const u8* input);

    void encode_surface_astc          (const TextureCompression& info, u8* output, const Surface& input);

} // namespace mango::image

namespace
{
    using namespace mango;
    using namespace mango::image;

    const TextureCompression g_blockTable[] =
    {

        // NONE

        TextureCompression(),

        // 3DFX_texture_compression_FXT1

        TextureCompression(
            TextureCompression::FXT1_RGB,
            0,
            opengl::COMPRESSED_RGB_FXT1_3DFX,
            0,
            8, 4, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_fxt1_rgb, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::FXT1_RGBA,
            0,
            opengl::COMPRESSED_RGBA_FXT1_3DFX,
            0,
            8, 4, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_fxt1_rgba, nullptr,
            nullptr, nullptr
        ),

        // AMD_compressed_ATC_texture

        TextureCompression(
            TextureCompression::ATC_RGB,
            0,
            opengl::ATC_RGB_AMD,
            0,
            4, 4, 1, 8, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_atc, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ATC_RGBA_EXPLICIT_ALPHA,
            0,
            opengl::ATC_RGBA_EXPLICIT_ALPHA_AMD,
            0,
            4, 4, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_atc_e, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ATC_RGBA_INTERPOLATED_ALPHA,
            0,
            opengl::ATC_RGBA_INTERPOLATED_ALPHA_AMD,
            0,
            4, 4, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_atc_i, nullptr,
            nullptr, nullptr
        ),

        // AMD_compressed_3DC_texture

        TextureCompression(
            TextureCompression::AMD_3DC_X,
            dxgi::FORMAT_BC4_UNORM,
            opengl::AMD_3DC_X,
            vulkan::FORMAT_BC4_UNORM_BLOCK,
            4, 4, 1, 8, LinearFormat(8, Format::UNORM, Format::R, 8, 0, 0, 0),
            decode_block_3dc_x, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::AMD_3DC_XY,
            dxgi::FORMAT_BC5_UNORM,
            opengl::AMD_3DC_XY,
            vulkan::FORMAT_BC5_UNORM_BLOCK,
            4, 4, 1, 16, LinearFormat(16, Format::UNORM, Format::RG, 8, 8, 0, 0),
            decode_block_3dc_xy, nullptr,
            nullptr, nullptr
        ),

        // LATC

        TextureCompression(
            TextureCompression::LATC1_LUMINANCE,
            0,
            opengl::COMPRESSED_LUMINANCE_LATC1_EXT,
            0,
            4, 4, 1, 8, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::LATC1_SIGNED_LUMINANCE,
            0,
            opengl::COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT,
            0,
            4, 4, 1, 8, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::LATC2_LUMINANCE_ALPHA,
            0,
            opengl::COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,
            0,
            4, 4, 1, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::LATC2_SIGNED_LUMINANCE_ALPHA,
            0,
            opengl::COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT,
            0,
            4, 4, 1, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        // DXT

        TextureCompression(
            TextureCompression::DXT1,
            dxgi::FORMAT_BC1_UNORM,
            opengl::COMPRESSED_RGB_S3TC_DXT1_EXT,
            vulkan::FORMAT_BC1_RGB_UNORM_BLOCK,
            4, 4, 1, 8, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_dxt1, encode_block_bc1,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::DXT1_SRGB,
            dxgi::FORMAT_BC1_UNORM_SRGB,
            opengl::COMPRESSED_SRGB_S3TC_DXT1_EXT,
            vulkan::FORMAT_BC1_RGB_SRGB_BLOCK,
            4, 4, 1, 8, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_dxt1, encode_block_bc1,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::DXT1_ALPHA1,
            0,
            opengl::COMPRESSED_RGBA_S3TC_DXT1_EXT,
            vulkan::FORMAT_BC1_RGBA_UNORM_BLOCK,
            4, 4, 1, 8, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_dxt1a, encode_block_bc1a,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::DXT1_ALPHA1_SRGB,
            0,
            opengl::COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,
            vulkan::FORMAT_BC1_RGBA_SRGB_BLOCK,
            4, 4, 1, 8, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_dxt1a, encode_block_bc1a,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::DXT3,
            dxgi::FORMAT_BC2_UNORM,
            opengl::COMPRESSED_RGBA_S3TC_DXT3_EXT,
            vulkan::FORMAT_BC2_UNORM_BLOCK,
            4, 4, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_dxt3, encode_block_bc2,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::DXT3_SRGB,
            dxgi::FORMAT_BC2_UNORM_SRGB,
            opengl::COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,
            vulkan::FORMAT_BC2_SRGB_BLOCK,
            4, 4, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_dxt3, encode_block_bc2,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::DXT5,
            dxgi::FORMAT_BC3_UNORM,
            opengl::COMPRESSED_RGBA_S3TC_DXT5_EXT,
            vulkan::FORMAT_BC3_UNORM_BLOCK,
            4, 4, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_dxt5, encode_block_bc3,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::DXT5_SRGB,
            dxgi::FORMAT_BC3_UNORM_SRGB,
            opengl::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,
            vulkan::FORMAT_BC3_SRGB_BLOCK,
            4, 4, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_dxt5, encode_block_bc3,
            nullptr, nullptr
        ),

        // RGTC

        TextureCompression(
            TextureCompression::RGTC1_RED,
            dxgi::FORMAT_BC4_UNORM,
            opengl::COMPRESSED_RED_RGTC1,
            vulkan::FORMAT_BC4_UNORM_BLOCK,
            4, 4, 1, 8, LinearFormat(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32),
            decode_block_bc4u, encode_block_bc4u,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::RGTC1_SIGNED_RED,
            dxgi::FORMAT_BC4_SNORM,
            opengl::COMPRESSED_SIGNED_RED_RGTC1,
            vulkan::FORMAT_BC4_SNORM_BLOCK,
            4, 4, 1, 8, LinearFormat(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32),
            decode_block_bc4s, encode_block_bc4s,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::RGTC2_RG,
            dxgi::FORMAT_BC5_UNORM,
            opengl::COMPRESSED_RG_RGTC2,
            vulkan::FORMAT_BC5_UNORM_BLOCK,
            4, 4, 1, 16, LinearFormat(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32),
            decode_block_bc5u, encode_block_bc5u,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::RGTC2_SIGNED_RG,
            dxgi::FORMAT_BC5_SNORM,
            opengl::COMPRESSED_SIGNED_RG_RGTC2,
            vulkan::FORMAT_BC5_SNORM_BLOCK,
            4, 4, 1, 16, LinearFormat(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32),
            decode_block_bc5s, encode_block_bc5s,
            nullptr, nullptr
        ),

        // BPTC

        TextureCompression(
            TextureCompression::BPTC_RGB_UNSIGNED_FLOAT,
            dxgi::FORMAT_BC6H_UF16,
            opengl::COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,
            vulkan::FORMAT_BC6H_UFLOAT_BLOCK,
            4, 4, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            decode_block_bc6hu, encode_block_bc6hu,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::BPTC_RGB_SIGNED_FLOAT,
            dxgi::FORMAT_BC6H_SF16,
            opengl::COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
            vulkan::FORMAT_BC6H_SFLOAT_BLOCK,
            4, 4, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            decode_block_bc6hs, encode_block_bc6hs,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::BPTC_RGBA_UNORM,
            dxgi::FORMAT_BC7_UNORM,
            opengl::COMPRESSED_RGBA_BPTC_UNORM,
            vulkan::FORMAT_BC7_UNORM_BLOCK,
            4, 4, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_bc7, encode_block_bc7,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::BPTC_SRGB_ALPHA_UNORM,
            dxgi::FORMAT_BC7_UNORM_SRGB,
            opengl::COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
            vulkan::FORMAT_BC7_SRGB_BLOCK,
            4, 4, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_bc7, encode_block_bc7,
            nullptr, nullptr
        ),

        // IMG_texture_compression_pvrtc

        TextureCompression(
            TextureCompression::PVRTC_RGB_4BPP,
            0,
            opengl::COMPRESSED_RGB_PVRTC_4BPPV1_IMG,
            0,
            4, 4, 1, 8, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_pvrtc, nullptr
        ),

        TextureCompression(
            TextureCompression::PVRTC_RGB_2BPP,
            0,
            opengl::COMPRESSED_RGB_PVRTC_2BPPV1_IMG,
            0,
            8, 4, 1, 8, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_pvrtc, nullptr
        ),

        TextureCompression(
            TextureCompression::PVRTC_RGBA_4BPP,
            0,
            opengl::COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,
            vulkan::FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG,
            4, 4, 1, 8, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_pvrtc, nullptr
        ),

        TextureCompression(
            TextureCompression::PVRTC_RGBA_2BPP,
            0,
            opengl::COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,
            vulkan::FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG,
            8, 4, 1, 8, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_pvrtc, nullptr
        ),

        // IMG_texture_compression_pvrtc2

        TextureCompression(
            TextureCompression::PVRTC2_RGBA_2BPP,
            0,
            opengl::COMPRESSED_RGBA_PVRTC_2BPPV2_IMG,
            vulkan::FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG,
            8, 4, 1, 8, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_pvrtc2, nullptr
        ),

        TextureCompression(
            TextureCompression::PVRTC2_RGBA_4BPP,
            0,
            opengl::COMPRESSED_RGBA_PVRTC_4BPPV2_IMG,
            vulkan::FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG,
            4, 4, 1, 8, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_pvrtc2, nullptr
        ),

        // VK_IMG_format_pvrtc

        TextureCompression(
            TextureCompression::PVRTC2_2BPP_SRGB_BLOCK_IMG,
            0, // not supported in DirectX
            0, // not supported in OpenGL
            vulkan::FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG,
            8, 4, 1, 8, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_pvrtc2, nullptr
        ),

        TextureCompression(
            TextureCompression::PVRTC2_4BPP_SRGB_BLOCK_IMG,
            0, // not supported in DirectX
            0, // not supported in OpenGL
            vulkan::FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG,
            4, 4, 1, 8, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_pvrtc2, nullptr
        ),

        // EXT_pvrtc_sRGB

        TextureCompression(
            TextureCompression::PVRTC_SRGB_2BPP,
            0,
            opengl::COMPRESSED_SRGB_PVRTC_2BPPV1_EXT,
            0,
            8, 8, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_pvrtc, nullptr
        ),

        TextureCompression(
            TextureCompression::PVRTC_SRGB_4BPP,
            0,
            opengl::COMPRESSED_SRGB_PVRTC_4BPPV1_EXT,
            0,
            8, 8, 1, 32, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_pvrtc, nullptr
        ),

        TextureCompression(
            TextureCompression::PVRTC_SRGB_ALPHA_2BPP,
            0,
            opengl::COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT,
            vulkan::FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG,
            8, 8, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_pvrtc, nullptr
        ),

        TextureCompression(
            TextureCompression::PVRTC_SRGB_ALPHA_4BPP,
            0,
            opengl::COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT,
            vulkan::FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,
            8, 8, 1, 32, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_pvrtc, nullptr
        ),

        // OES_compressed_ETC1_RGB8_texture

        TextureCompression(
            TextureCompression::ETC1_RGB,
            0,
            opengl::ETC1_RGB8_OES,
            0,
            4, 4, 1, 8, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_etc1, encode_block_etc1,
            nullptr, nullptr
        ),

        // ETC2 / EAC

        TextureCompression(
            TextureCompression::EAC_R11,
            0,
            opengl::COMPRESSED_R11_EAC,
            vulkan::FORMAT_EAC_R11_UNORM_BLOCK,
            4, 4, 1, 8, LinearFormat(16, Format::UNORM, Format::R, 16, 0, 0, 0),
            decode_block_eac_r11, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::EAC_SIGNED_R11,
            0,
            opengl::COMPRESSED_SIGNED_R11_EAC,
            vulkan::FORMAT_EAC_R11_SNORM_BLOCK,
            4, 4, 1, 8, LinearFormat(16, Format::SNORM, Format::R, 16, 0, 0, 0),
            decode_block_eac_r11, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::EAC_RG11,
            0,
            opengl::COMPRESSED_RG11_EAC,
            vulkan::FORMAT_EAC_R11G11_UNORM_BLOCK,
            4, 4, 1, 16, LinearFormat(32, Format::UNORM, Format::RG, 16, 16, 0, 0),
            decode_block_eac_rg11, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::EAC_SIGNED_RG11,
            0,
            opengl::COMPRESSED_SIGNED_RG11_EAC,
            vulkan::FORMAT_EAC_R11G11_SNORM_BLOCK,
            4, 4, 1, 16, LinearFormat(32, Format::SNORM, Format::RG, 16, 16, 0, 0),
            decode_block_eac_rg11, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ETC2_RGB,
            0,
            opengl::COMPRESSED_RGB8_ETC2,
            vulkan::FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
            4, 4, 1, 8, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_etc2, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ETC2_SRGB,
            0,
            opengl::COMPRESSED_SRGB8_ETC2,
            vulkan::FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
            4, 4, 1, 8, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_etc2, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ETC2_RGB_ALPHA1,
            0,
            opengl::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
            vulkan::FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
            4, 4, 1, 8, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_etc2, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ETC2_SRGB_ALPHA1,
            0,
            opengl::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
            vulkan::FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
            4, 4, 1, 8, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_etc2, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ETC2_RGBA,
            0,
            opengl::COMPRESSED_RGBA8_ETC2_EAC,
            vulkan::FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
            4, 4, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_etc2_eac, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ETC2_SRGB_ALPHA8,
            0,
            opengl::COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,
            vulkan::FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
            4, 4, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_etc2_eac, nullptr,
            nullptr, nullptr
        ),

        // KHR_texture_compression_astc_ldr

        TextureCompression(
            TextureCompression::ASTC_UNORM_4x4,
            dxgi::FORMAT_ASTC_4X4_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_4x4_KHR,
            vulkan::FORMAT_ASTC_4x4_UNORM_BLOCK,
            4, 4, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_5x4,
            dxgi::FORMAT_ASTC_5X4_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_5x4_KHR,
            vulkan::FORMAT_ASTC_5x4_UNORM_BLOCK,
            5, 4, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_5x5,
            dxgi::FORMAT_ASTC_5X5_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_5x5_KHR,
            vulkan::FORMAT_ASTC_5x5_UNORM_BLOCK,
            5, 5, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_6x5,
            dxgi::FORMAT_ASTC_6X5_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_6x5_KHR,
            vulkan::FORMAT_ASTC_6x5_UNORM_BLOCK,
            6, 5, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_6x6,
            dxgi::FORMAT_ASTC_6X6_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_6x6_KHR,
            vulkan::FORMAT_ASTC_6x6_UNORM_BLOCK,
            6, 6, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_8x5,
            dxgi::FORMAT_ASTC_8X5_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_8x5_KHR,
            vulkan::FORMAT_ASTC_8x5_UNORM_BLOCK,
            8, 5, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_8x6,
            dxgi::FORMAT_ASTC_8X6_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_8x6_KHR,
            vulkan::FORMAT_ASTC_8x6_UNORM_BLOCK,
            8, 6, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_8x8,
            dxgi::FORMAT_ASTC_8X8_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_8x8_KHR,
            vulkan::FORMAT_ASTC_8x8_UNORM_BLOCK,
            8, 8, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_10x5,
            dxgi::FORMAT_ASTC_10X5_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_10x5_KHR,
            vulkan::FORMAT_ASTC_10x5_UNORM_BLOCK,
            10, 5, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_10x6,
            dxgi::FORMAT_ASTC_10X6_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_10x6_KHR,
            vulkan::FORMAT_ASTC_10x6_UNORM_BLOCK,
            10, 6, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_10x8,
            dxgi::FORMAT_ASTC_10X8_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_10x8_KHR,
            vulkan::FORMAT_ASTC_10x8_UNORM_BLOCK,
            10, 8, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_10x10,
            dxgi::FORMAT_ASTC_10X10_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_10x10_KHR,
            vulkan::FORMAT_ASTC_10x10_UNORM_BLOCK,
            10, 10, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_12x10,
            dxgi::FORMAT_ASTC_12X10_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_12x10_KHR,
            vulkan::FORMAT_ASTC_12x10_UNORM_BLOCK,
            12, 10, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_12x12,
            dxgi::FORMAT_ASTC_12X12_UNORM,
            opengl::COMPRESSED_RGBA_ASTC_12x12_KHR,
            vulkan::FORMAT_ASTC_12x12_UNORM_BLOCK,
            12, 12, 1, 16, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_4x4,
            dxgi::FORMAT_ASTC_4X4_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR,
            vulkan::FORMAT_ASTC_4x4_SRGB_BLOCK,
            4, 4, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_5x4,
            dxgi::FORMAT_ASTC_5X4_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR,
            vulkan::FORMAT_ASTC_5x4_SRGB_BLOCK,
            5, 4, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_5x5,
            dxgi::FORMAT_ASTC_5X5_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR,
            vulkan::FORMAT_ASTC_5x5_SRGB_BLOCK,
            5, 5, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_6x5,
            dxgi::FORMAT_ASTC_6X5_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR,
            vulkan::FORMAT_ASTC_6x5_SRGB_BLOCK,
            6, 5, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_6x6,
            dxgi::FORMAT_ASTC_6X6_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR,
            vulkan::FORMAT_ASTC_6x6_SRGB_BLOCK,
            6, 6, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_8x5,
            dxgi::FORMAT_ASTC_8X5_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR,
            vulkan::FORMAT_ASTC_8x5_SRGB_BLOCK,
            8, 5, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_8x6,
            dxgi::FORMAT_ASTC_8X6_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR,
            vulkan::FORMAT_ASTC_8x6_SRGB_BLOCK,
            8, 6, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_8x8,
            dxgi::FORMAT_ASTC_8X8_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR,
            vulkan::FORMAT_ASTC_8x8_SRGB_BLOCK,
            8, 8, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_10x5,
            dxgi::FORMAT_ASTC_10X5_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR,
            vulkan::FORMAT_ASTC_10x5_SRGB_BLOCK,
            10, 5, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_10x6,
            dxgi::FORMAT_ASTC_10X6_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR,
            vulkan::FORMAT_ASTC_10x6_SRGB_BLOCK,
            10, 6, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_10x8,
            dxgi::FORMAT_ASTC_10X8_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR,
            vulkan::FORMAT_ASTC_10x8_SRGB_BLOCK,
            10, 8, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_10x10,
            dxgi::FORMAT_ASTC_10X10_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR,
            vulkan::FORMAT_ASTC_10x10_SRGB_BLOCK,
            10, 10, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_12x10,
            dxgi::FORMAT_ASTC_12X10_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR,
            vulkan::FORMAT_ASTC_12x10_SRGB_BLOCK,
            12, 10, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_12x12,
            dxgi::FORMAT_ASTC_12X12_UNORM_SRGB,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR,
            vulkan::FORMAT_ASTC_12x12_SRGB_BLOCK,
            12, 12, 1, 16, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        // KHR_texture_compression_astc_hdr

        TextureCompression(
            TextureCompression::ASTC_FLOAT_4x4,
            dxgi::FORMAT_ASTC_4X4_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_4x4_KHR,
            vulkan::FORMAT_ASTC_4x4_SFLOAT_BLOCK,
            4, 4, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_5x4,
            dxgi::FORMAT_ASTC_5X4_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_5x4_KHR,
            vulkan::FORMAT_ASTC_5x4_SFLOAT_BLOCK,
            5, 4, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_5x5,
            dxgi::FORMAT_ASTC_5X5_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_5x5_KHR,
            vulkan::FORMAT_ASTC_5x5_SFLOAT_BLOCK,
            5, 5, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_6x5,
            dxgi::FORMAT_ASTC_6X5_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_6x5_KHR,
            vulkan::FORMAT_ASTC_6x5_SFLOAT_BLOCK,
            6, 5, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_6x6,
            dxgi::FORMAT_ASTC_6X6_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_6x6_KHR,
            vulkan::FORMAT_ASTC_6x6_SFLOAT_BLOCK,
            6, 6, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_8x5,
            dxgi::FORMAT_ASTC_8X5_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_8x5_KHR,
            vulkan::FORMAT_ASTC_8x5_SFLOAT_BLOCK,
            8, 5, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_8x6,
            dxgi::FORMAT_ASTC_8X6_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_8x6_KHR,
            vulkan::FORMAT_ASTC_8x6_SFLOAT_BLOCK,
            8, 6, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_8x8,
            dxgi::FORMAT_ASTC_8X8_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_8x8_KHR,
            vulkan::FORMAT_ASTC_8x8_SFLOAT_BLOCK,
            8, 8, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_10x5,
            dxgi::FORMAT_ASTC_10X5_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_10x5_KHR,
            vulkan::FORMAT_ASTC_10x5_SFLOAT_BLOCK,
            10, 5, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_10x6,
            dxgi::FORMAT_ASTC_10X6_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_10x6_KHR,
            vulkan::FORMAT_ASTC_10x6_SFLOAT_BLOCK,
            10, 6, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_10x8,
            dxgi::FORMAT_ASTC_10X8_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_10x8_KHR,
            vulkan::FORMAT_ASTC_10x8_SFLOAT_BLOCK,
            10, 8, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_10x10,
            dxgi::FORMAT_ASTC_10X10_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_10x10_KHR,
            vulkan::FORMAT_ASTC_10x10_SFLOAT_BLOCK,
            10, 10, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_12x10,
            dxgi::FORMAT_ASTC_12X10_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_12x10_KHR,
            vulkan::FORMAT_ASTC_12x10_SFLOAT_BLOCK,
            12, 10, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        TextureCompression(
            TextureCompression::ASTC_FLOAT_12x12,
            dxgi::FORMAT_ASTC_12X12_TYPELESS,
            opengl::COMPRESSED_RGBA_ASTC_12x12_KHR,
            vulkan::FORMAT_ASTC_12x12_SFLOAT_BLOCK,
            12, 12, 1, 16, LinearFormat(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
            nullptr, nullptr,
            decode_surface_astc, encode_surface_astc
        ),

        // OES_texture_compression_astc

        TextureCompression(
            TextureCompression::ASTC_UNORM_3x3x3,
            0,
            opengl::COMPRESSED_RGBA_ASTC_3x3x3_OES,
            0,
            3, 3, 3, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_4x3x3,
            0,
            opengl::COMPRESSED_RGBA_ASTC_4x3x3_OES,
            0,
            4, 3, 3, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_4x4x3,
            0,
            opengl::COMPRESSED_RGBA_ASTC_4x4x3_OES,
            0,
            4, 4, 3, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_4x4x4,
            0,
            opengl::COMPRESSED_RGBA_ASTC_4x4x4_OES,
            0,
            4, 4, 4, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_5x4x4,
            0,
            opengl::COMPRESSED_RGBA_ASTC_5x4x4_OES,
            0,
            5, 4, 4, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_5x5x4,
            0,
            opengl::COMPRESSED_RGBA_ASTC_5x5x4_OES,
            0,
            5, 5, 4, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_5x5x5,
            0,
            opengl::COMPRESSED_RGBA_ASTC_5x5x5_OES,
            0,
            5, 5, 5, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_6x5x5,
            0,
            opengl::COMPRESSED_RGBA_ASTC_6x5x5_OES,
            0,
            6, 5, 5, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_6x6x5,
            0,
            opengl::COMPRESSED_RGBA_ASTC_6x6x5_OES,
            0,
            6, 6, 5, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_UNORM_6x6x6,
            0,
            opengl::COMPRESSED_RGBA_ASTC_6x6x6_OES,
            0,
            6, 6, 6, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_3x3x3,
            0,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES,
            0,
            3, 3, 3, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_4x3x3,
            0,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES,
            0,
            4, 3, 3, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_4x4x3,
            0,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES,
            0,
            4, 4, 3, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_4x4x4,
            0,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES,
            0,
            4, 4, 4, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_5x4x4,
            0,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES,
            0,
            5, 4, 4, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_5x5x4,
            0,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES,
            0,
            5, 5, 4, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_5x5x5,
            0,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES,
            0,
            5, 5, 5, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_6x5x5,
            0,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES,
            0,
            6, 5, 5, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_6x6x5,
            0,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES,
            0,
            6, 6, 5, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::ASTC_SRGB_6x6x6,
            0,
            opengl::COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES,
            0,
            6, 6, 6, 16, Format(),
            nullptr, nullptr,
            nullptr, nullptr
        ),

        // Packed Pixel

        TextureCompression(
            TextureCompression::RGB9_E5,
            dxgi::FORMAT_R9G9B9E5_SHAREDEXP,
            0x8C3D,
            0,
            1, 1, 1, 4, LinearFormat(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32),
            decode_block_rgb9e5, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::R11F_G11F_B10F,
            0,
            0x8C3A,
            0,
            1, 1, 1, 4, LinearFormat(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32),
            decode_block_r11f_g11f_b10f, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::R10F_G11F_B11F,
            0,
            0,
            0,
            1, 1, 1, 4, LinearFormat(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32),
            decode_block_r10f_g11f_b11f, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::BITPLANE1,
            0,
            0,
            0,
            8, 1, 1, 1, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_bitplane1, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::G8R8G8B8,
            dxgi::FORMAT_G8R8_G8B8_UNORM,
            0,
            0,
            2, 1, 1, 4, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_grgb8, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::R8G8B8G8,
            dxgi::FORMAT_R8G8_B8G8_UNORM,
            0,
            0,
            2, 1, 1, 4, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_rgbg8, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::UYVY,
            0,
            0,
            0,
            2, 1, 1, 4, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_uyvy, nullptr,
            nullptr, nullptr
        ),

        TextureCompression(
            TextureCompression::YUY2,
            0,
            0,
            0,
            2, 1, 1, 4, LinearFormat(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
            decode_block_yuy2, nullptr,
            nullptr, nullptr
        ),
    };

    // block decode

    void scanBlockDecode(const TextureCompression& info, u8* image, const u8* data, size_t stride, int xblocks, size_t xstride)
    {
        for (int x = 0; x < xblocks; ++x)
        {
            info.decodeBlock(info, image, data, stride);
            image += xstride;
            data += info.bytes;
        }
    }

    void directBlockDecode(const TextureCompression& info, const Surface& surface, ConstMemory memory, int xblocks, int yblocks)
    {
        const u8* data = memory.address;

        u8* image = surface.image;
        size_t stride = surface.stride;

        size_t xstride = info.width * surface.format.bytes();
        size_t ystride = info.height * surface.stride;

        size_t concurrency = ThreadPool::getHardwareConcurrency();
        bool multithread = concurrency > 1;

        if (multithread)
        {
            ConcurrentQueue queue;

            for (int y = 0; y < yblocks; ++y)
            {
                queue.enqueue([=, &info]
                {
                    scanBlockDecode(info, image, data, stride, xblocks, xstride);
                });

                image += ystride;
                data += info.bytes * xblocks;
            }
        }
        else
        {
            for (int y = 0; y < yblocks; ++y)
            {
                scanBlockDecode(info, image, data, stride, xblocks, xstride);
                image += ystride;
                data += info.bytes * xblocks;
            }
        }
    }

} // namespace

namespace mango::image
{

    // ----------------------------------------------------------------------------
    // TextureCompression
    // ----------------------------------------------------------------------------

    TextureCompression::TextureCompression()
        : compression(TextureCompression::NONE)
        , dxgi(0)
        , opengl(0)
        , vulkan(0)
        , width(1)
        , height(1)
        , depth(1)
        , bytes(0)
        , format()
        , decodeBlock(nullptr)
        , encodeBlock(nullptr)
        , decodeSurface(nullptr)
        , encodeSurface(nullptr)
    {
    }

    TextureCompression::TextureCompression(
        u32 compression, u32 dxgi, u32 opengl, u32 vulkan,
        int width, int height, int depth, int bytes,
        const Format& format,
        DecodeBlock decodeBlock, EncodeBlock encodeBlock,
        DecodeSurface decodeSurface, EncodeSurface encodeSurface)
        : compression(compression)
        , dxgi(dxgi)
        , opengl(opengl)
        , vulkan(vulkan)
        , width(width)
        , height(height)
        , depth(depth)
        , bytes(bytes)
        , format(format)
        , decodeBlock(decodeBlock)
        , encodeBlock(encodeBlock)
        , decodeSurface(decodeSurface)
        , encodeSurface(encodeSurface)
    {
    }

    TextureCompression::TextureCompression(u32 compression)
        : compression(compression)
    {
        const TextureCompression* info = &g_blockTable[0];

        for (const auto& node : g_blockTable)
        {
            // ignore flags when comparing
            if ((node.compression & ~MASK) == (compression & ~MASK))
            {
                info = &node;
                break;
            }
        }

        *this = *info;
        this->compression = compression;
    }

    TextureCompression::TextureCompression(dxgi::TextureFormat format)
    {
        const TextureCompression* info = &g_blockTable[0];

        for (const auto& node : g_blockTable)
        {
            if (node.dxgi == format)
            {
                info = &node;
                break;
            }
        }

        *this = *info;
    }

    TextureCompression::TextureCompression(opengl::TextureFormat format)
    {
        const TextureCompression* info = &g_blockTable[0];

        for (const auto& node : g_blockTable)
        {
            if (node.opengl == format)
            {
                info = &node;
                break;
            }
        }

        *this = *info;
    }

    TextureCompression::TextureCompression(vulkan::TextureFormat format)
    {
        const TextureCompression* info = &g_blockTable[0];

        for (const auto& node : g_blockTable)
        {
            if (node.vulkan == format)
            {
                info = &node;
                break;
            }
        }

        *this = *info;
    }

    TextureCompression::Status TextureCompression::decompress(const Surface& surface, ConstMemory memory) const
    {
        TextureCompression::Status status;

        if (!decodeBlock && !decodeSurface)
        {
            status.setError("No decoder for {:#x}.", compression);
            return status;
        }

        const int xblocks = getBlocksX(surface.width);
        const int yblocks = getBlocksY(surface.height);

        const int compressed_width = xblocks * width;
        const int compressed_height = yblocks * height;

        const bool noclip = surface.width == compressed_width && surface.height == compressed_height;
        const bool noconvert = surface.format == format;
        const bool direct = noclip && noconvert;

        if (direct)
        {
            if (decodeSurface)
            {
                decodeSurface(*this, surface, memory.address);
            }
            else
            {
                directBlockDecode(*this, surface, memory, xblocks, yblocks);
            }
        }
        else
        {
            Bitmap temp(compressed_width, compressed_height, format);

            if (decodeSurface)
            {
                decodeSurface(*this, temp, memory.address);
            }
            else
            {
                directBlockDecode(*this, temp, memory, xblocks, yblocks);
            }

            surface.blit(0, 0, temp);
        }

        status.direct = direct;

        return status;
    }

    TextureCompression::Status TextureCompression::compress(Memory memory, const Surface& surface) const
    {
        TextureCompression::Status status;

        if (!encodeBlock && !encodeSurface)
        {
            status.setError("No encoder for {:#x}.", compression);
            return status;
        }

        const int xblocks = getBlocksX(surface.width);
        const int yblocks = getBlocksY(surface.height);

        const int compressed_width = xblocks * width;
        const int compressed_height = yblocks * height;

        if (encodeSurface)
        {
            TemporaryBitmap temp(surface, compressed_width, compressed_height, format);
            encodeSurface(*this, memory.address, temp);
        }
        else
        {
            ConcurrentQueue queue;

            u8* address = memory.address;

            for (int y = 0; y < yblocks; ++y)
            {
                queue.enqueue([=, this]
                {
                    Bitmap temp(compressed_width, height, format);

                    int source_width = std::min(surface.width, compressed_width);
                    int source_height = std::min(height, surface.height - y * height);

                    Surface source(surface, 0, y * height, source_width, source_height);
                    temp.blit(0, 0, source);

                    u8* data = address + y * xblocks * bytes;
                    u8* image = temp.image;
                    size_t step = width * format.bytes();

                    for (int x = 0; x < xblocks; ++x)
                    {
                        encodeBlock(*this, data, image, temp.stride);
                        data += bytes;
                        image += step;
                    }
                });
            }
        }

        return status;
    }

    bool TextureCompression::isLinear() const
    {
        return (compression & SRGB) == 0;
    }

    int TextureCompression::getBlocksX(int c_width) const
    {
        return div_ceil(c_width, width);
    }

    int TextureCompression::getBlocksY(int c_height) const
    {
        return div_ceil(c_height, height);
    }

    int TextureCompression::getBlockCount(int c_width, int c_height) const
    {
        int xblocks = getBlocksX(c_width);
        int yblocks = getBlocksY(c_height);
        return xblocks * yblocks;
    }

    u64 TextureCompression::getBlockBytes(int c_width, int c_height) const
    {
        int xblocks = getBlocksX(c_width);
        int yblocks = getBlocksY(c_height);
        return xblocks * yblocks * bytes;
    }

} // namespace mango::image
