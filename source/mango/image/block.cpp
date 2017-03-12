/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include "../../external/google/etc.hpp"
#include "../../external/google/astc.hpp"
#include "../../external/bc/BC.h"

#define FORMAT_ASTC  MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8)

namespace mango
{

    void decode_block_dxt1           (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride); // BC1
    void decode_block_dxt3           (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride); // BC2
    void decode_block_dxt5           (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride); // BC3
    void decode_block_3dc_x          (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride); // BC4U
    void decode_block_3dc_xy         (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride); // BC5U
    void decode_block_uyvy           (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);
    void decode_block_yuy2           (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);
    void decode_block_grgb8          (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);
    void decode_block_rgbg8          (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);
    void decode_block_rgb9e5         (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);
    void decode_block_r11f_g11f_b10f (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);
    void decode_block_r10f_g11f_b11f (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);
    void decode_block_pvrtc          (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);

    void encode_block_etc1           (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);

} // namespace mango

namespace
{
    using namespace mango;

    const TextureCompressionInfo g_blockTable[] =
    {
        // AMD_compressed_ATC_texture
        { 4, 4,  8, FORMAT_NONE, nullptr, nullptr, TextureCompression::ATC_RGB },
        { 4, 4, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ATC_RGBA_EXPLICIT_ALPHA },
        { 4, 4, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ATC_RGBA_INTERPOLATED_ALPHA },

        // AMD_compressed_3DC_texture
        { 4, 4,  8, MAKE_FORMAT(8, UNORM, R, 8, 0, 0, 0), decode_block_3dc_x, nullptr, TextureCompression::AMD_3DC_X },
        { 4, 4, 16, MAKE_FORMAT(16, UNORM, RG, 8, 8, 0, 0), decode_block_3dc_xy, nullptr, TextureCompression::AMD_3DC_XY },

		// LATC
        { 4, 4,  8, FORMAT_NONE, nullptr, nullptr, TextureCompression::LATC1_LUMINANCE },
        { 4, 4,  8, FORMAT_NONE, nullptr, nullptr, TextureCompression::LATC1_SIGNED_LUMINANCE },
        { 4, 4, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::LATC2_LUMINANCE_ALPHA },
        { 4, 4, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::LATC2_SIGNED_LUMINANCE_ALPHA },

        // DXT
        { 4, 4,  8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_dxt1, encode_block_bc1, TextureCompression::DXT1 },
        { 4, 4,  8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_dxt1, encode_block_bc1, TextureCompression::DXT1_SRGB },
        { 4, 4,  8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_dxt1, encode_block_bc1a, TextureCompression::DXT1_ALPHA1 },
        { 4, 4,  8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_dxt1, encode_block_bc1a, TextureCompression::DXT1_ALPHA1_SRGB },
        { 4, 4, 16, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_dxt3, encode_block_bc2, TextureCompression::DXT3 },
        { 4, 4, 16, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_dxt3, encode_block_bc2, TextureCompression::DXT3_SRGB },
        { 4, 4, 16, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_dxt5, encode_block_bc3, TextureCompression::DXT5 },
        { 4, 4, 16, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_dxt5, encode_block_bc3, TextureCompression::DXT5_SRGB },

#ifdef MANGO_ENABLE_LICENSE_MICROSOFT
        // RGTC
        { 4, 4,  8, MAKE_FORMAT(128, FP32, RGBA, 32, 32, 32, 32), decode_block_bc4u, encode_block_bc4u, TextureCompression::RGTC1_RED },
        { 4, 4,  8, MAKE_FORMAT(128, FP32, RGBA, 32, 32, 32, 32), decode_block_bc4s, encode_block_bc4s, TextureCompression::RGTC1_SIGNED_RED },
        { 4, 4, 16, MAKE_FORMAT(128, FP32, RGBA, 32, 32, 32, 32), decode_block_bc5u, encode_block_bc5u, TextureCompression::RGTC2_RG },
        { 4, 4, 16, MAKE_FORMAT(128, FP32, RGBA, 32, 32, 32, 32), decode_block_bc5s, encode_block_bc5s, TextureCompression::RGTC2_SIGNED_RG },

        // BPTC
        { 4, 4, 16, MAKE_FORMAT(128, FP32, RGBA, 32, 32, 32, 32), decode_block_bc6hu, encode_block_bc6hu, TextureCompression::BPTC_RGB_UNSIGNED_FLOAT },
        { 4, 4, 16, MAKE_FORMAT(128, FP32, RGBA, 32, 32, 32, 32), decode_block_bc6hs, encode_block_bc6hs, TextureCompression::BPTC_RGB_SIGNED_FLOAT },
        { 4, 4, 16, MAKE_FORMAT(128, FP32, RGBA, 32, 32, 32, 32), decode_block_bc7, encode_block_bc7, TextureCompression::BPTC_RGBA_UNORM },
        { 4, 4, 16, MAKE_FORMAT(128, FP32, RGBA, 32, 32, 32, 32), decode_block_bc7, encode_block_bc7, TextureCompression::BPTC_SRGB_ALPHA_UNORM },
#endif

        // IMG_texture_compression_pvrtc
        { 4, 4, 8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_pvrtc, nullptr, TextureCompression::PVRTC_RGB_4BPP },
        { 8, 4, 8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_pvrtc, nullptr, TextureCompression::PVRTC_RGB_2BPP },
        { 4, 4, 8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_pvrtc, nullptr, TextureCompression::PVRTC_RGBA_4BPP },
        { 8, 4, 8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_pvrtc, nullptr, TextureCompression::PVRTC_RGBA_2BPP },

        // IMG_texture_compression_pvrtc2
        { 8, 4, 8, FORMAT_NONE, nullptr, nullptr, TextureCompression::PVRTC2_RGBA_2BPP },
        { 4, 4, 8, FORMAT_NONE, nullptr, nullptr, TextureCompression::PVRTC2_RGBA_4BPP },

        // EXT_pvrtc_sRGB
        { 8, 8, 16, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_pvrtc, nullptr, TextureCompression::PVRTC_SRGB_2BPP },
        { 8, 8, 32, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_pvrtc, nullptr, TextureCompression::PVRTC_SRGB_4BPP },
        { 8, 8, 16, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_pvrtc, nullptr, TextureCompression::PVRTC_SRGB_ALPHA_2BPP },
        { 8, 8, 32, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_pvrtc, nullptr, TextureCompression::PVRTC_SRGB_ALPHA_4BPP },

#ifdef MANGO_ENABLE_LICENSE_APACHE
        // ETC2 / EAC
        { 4, 4,  8, MAKE_FORMAT(16, UNORM, R, 16, 0, 0, 0), decode_block_eac_r11, nullptr, TextureCompression::EAC_R11 },
        { 4, 4,  8, MAKE_FORMAT(16, SNORM, R, 16, 0, 0, 0), decode_block_eac_r11, nullptr, TextureCompression::EAC_SIGNED_R11 },
        { 4, 4, 16, MAKE_FORMAT(32, UNORM, RG, 16, 16, 0, 0), decode_block_eac_rg11, nullptr, TextureCompression::EAC_RG11 },
        { 4, 4, 16, MAKE_FORMAT(32, SNORM, RG, 16, 16, 0, 0), decode_block_eac_rg11, nullptr, TextureCompression::EAC_SIGNED_RG11 },
        { 4, 4,  8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_etc2, nullptr, TextureCompression::ETC2_RGB },
        { 4, 4,  8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_etc2, nullptr, TextureCompression::ETC2_SRGB },
        { 4, 4,  8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_etc2, nullptr, TextureCompression::ETC2_RGB_ALPHA1 },
        { 4, 4,  8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_etc2, nullptr, TextureCompression::ETC2_SRGB_ALPHA1 },
        { 4, 4, 16, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_etc2_eac, nullptr, TextureCompression::ETC2_RGBA },
        { 4, 4, 16, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_etc2_eac, nullptr, TextureCompression::ETC2_SRGB_ALPHA8 },

        // OES_compressed_ETC1_RGB8_texture
        { 4, 4, 8, MAKE_FORMAT(32, UNORM, RGBA, 8, 8, 8, 8), decode_block_etc1, encode_block_etc1, TextureCompression::ETC1_RGB },

        // KHR_texture_compression_astc_ldr
        {  4,  4, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_4x4 },
        {  5,  4, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_5x4 },
        {  5,  5, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_5x5 },
        {  6,  5, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_6x5 },
        {  6,  6, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_6x6 },
        {  8,  5, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_8x5 },
        {  8,  6, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_8x6 },
        {  8,  8, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_8x8 },
        { 10,  5, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_10x5 },
        { 10,  6, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_10x6 },
        { 10,  8, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_10x8 },
        { 10, 10, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_10x10 },
        { 12, 10, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_12x10 },
        { 12, 12, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_RGBA_12x12 },
        {  4,  4, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_4x4 },
        {  5,  4, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_5x4 },
        {  5,  5, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_5x5 },
        {  4,  4, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_6x5 },
        {  6,  6, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_6x6 },
        {  8,  5, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_8x5 },
        {  8,  6, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_8x6 },
        {  8,  8, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_8x8 },
        { 10,  5, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_10x5 },
        { 10,  6, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_10x6 },
        { 10,  8, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_10x8 },
        { 10, 10, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_10x10 },
        { 12, 10, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_12x10 },
        { 12, 12, 16, FORMAT_ASTC, decode_block_astc, nullptr, TextureCompression::ASTC_SRGB_ALPHA_12x12 },

        // KHR_texture_compression_astc_hdr
        { 3, 3, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_RGBA_3x3x3 },
        { 4, 3, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_RGBA_4x3x3 },
        { 4, 4, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_RGBA_4x4x3 },
        { 4, 4, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_RGBA_4x4x4 },
        { 5, 4, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_RGBA_5x4x4 },
        { 5, 5, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_RGBA_5x5x4 },
        { 5, 5, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_RGBA_5x5x5 },
        { 6, 5, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_RGBA_6x5x5 },
        { 6, 6, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_RGBA_6x6x5 },
        { 6, 6, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_RGBA_6x6x6 },
        { 3, 3, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_SRGB_ALPHA_3x3x3 },
        { 4, 4, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_SRGB_ALPHA_4x3x3 },
        { 4, 4, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_SRGB_ALPHA_4x4x3 },
        { 4, 4, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_SRGB_ALPHA_4x4x4 },
        { 5, 4, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_SRGB_ALPHA_5x4x4 },
        { 5, 5, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_SRGB_ALPHA_5x5x4 },
        { 5, 5, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_SRGB_ALPHA_5x5x5 },
        { 6, 5, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_SRGB_ALPHA_6x5x5 },
        { 6, 6, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_SRGB_ALPHA_6x6x5 },
        { 6, 6, 16, FORMAT_NONE, nullptr, nullptr, TextureCompression::ASTC_SRGB_ALPHA_6x6x6 },
#endif

        // Packed Pixel
        { 1, 1, 4, MAKE_FORMAT(128, FP32, RGBA, 32, 32, 32, 32), decode_block_rgb9e5, nullptr, TextureCompression::RGB9_E5 },
        { 1, 1, 4, MAKE_FORMAT(128, FP32, RGBA, 32, 32, 32, 32), decode_block_r11f_g11f_b10f, nullptr, TextureCompression::R11F_G11F_B10F },
        { 1, 1, 4, MAKE_FORMAT(128, FP32, RGBA, 32, 32, 32, 32), decode_block_r10f_g11f_b11f, nullptr, TextureCompression::R10F_G11F_B11F },
        { 2, 1, 4, FORMAT_R8G8B8A8, decode_block_grgb8, nullptr, TextureCompression::G8R8G8B8 },
        { 2, 1, 4, FORMAT_R8G8B8A8, decode_block_rgbg8, nullptr, TextureCompression::R8G8B8G8 },
		{ 2, 1, 4, FORMAT_R8G8B8A8, decode_block_uyvy, nullptr, TextureCompression::UYVY },
		{ 2, 1, 4, FORMAT_R8G8B8A8, decode_block_yuy2, nullptr, TextureCompression::YUY2 },
    };

    const TextureCompressionInfo* getTextureCompressionInfo(TextureCompression compression)
    {
        for (auto& node : g_blockTable)
        {
            if (node.compression == compression)
                return &node;
        }

        return nullptr;
    }

#define ET(opengl, vulkan, dxgi, compression) \
    { opengl, vulkan, dxgi, TextureCompression::compression, #compression }

    struct CompressInfo
    {
        uint32 format_opengl;
        uint32 format_vulkan;
		uint32 format_dxgi;
        TextureCompression compression;
        const char* name;
    };

    const CompressInfo g_compressionInfoTable[] =
    {
        ET( 0x8C92, 0,    0, ATC_RGB ),
        ET( 0x8C93, 0,    0, ATC_RGBA_EXPLICIT_ALPHA ),
        ET( 0x87EE, 0,    0, ATC_RGBA_INTERPOLATED_ALPHA ),
        ET( 0x87F9, 0,    0, AMD_3DC_X ),
        ET( 0x87FA, 0,    0, AMD_3DC_XY ),
        ET( 0x8C70, 0,    0, LATC1_LUMINANCE ),
        ET( 0x8C71, 0,    0, LATC1_SIGNED_LUMINANCE ),
        ET( 0x8C72, 0,    0, LATC2_LUMINANCE_ALPHA ),
        ET( 0x8C73, 0,    0, LATC2_SIGNED_LUMINANCE_ALPHA ),
		ET( 0x83F0, 131, 71, DXT1 ),
		ET( 0,      132, 72, DXT1_SRGB ),
		ET( 0x83F1, 133,  0, DXT1_ALPHA1 ),
		ET( 0,      133,  0, DXT1_ALPHA1_SRGB ),
		ET( 0x83F2, 135, 74, DXT3 ),
		ET( 0,      136, 75, DXT3_SRGB ),
		ET( 0x83F3, 137, 77, DXT5 ),
        ET( 0,      138, 78, DXT5_SRGB ),
		ET( 0x8DBB, 139, 80, RGTC1_RED ),
        ET( 0x8DBC, 140, 81, RGTC1_SIGNED_RED ),
        ET( 0x8DBD, 141, 83, RGTC2_RG ),
        ET( 0x8DBE, 142, 84, RGTC2_SIGNED_RG ),
        ET( 0x8E8F, 143, 95, BPTC_RGB_UNSIGNED_FLOAT ),
        ET( 0x8E8E, 144, 96, BPTC_RGB_SIGNED_FLOAT ),
        ET( 0x8E8C, 145, 98, BPTC_RGBA_UNORM ),
        ET( 0x8E8D, 146, 99, BPTC_SRGB_ALPHA_UNORM ),
		ET( 0x8C00, 0,    0, PVRTC_RGB_4BPP ),
        ET( 0x8C01, 0,    0, PVRTC_RGB_2BPP ),
        ET( 0x8C02, 0,    0, PVRTC_RGBA_4BPP ),
        ET( 0x8C03, 0,    0, PVRTC_RGBA_2BPP ),
        ET( 0x9137, 0,    0, PVRTC2_RGBA_2BPP ),
        ET( 0x9138, 0,    0, PVRTC2_RGBA_4BPP ),
        ET( 0x8A54, 0,    0, PVRTC_SRGB_2BPP ),
        ET( 0x8A55, 0,    0, PVRTC_SRGB_4BPP ),
        ET( 0x8A56, 0,    0, PVRTC_SRGB_ALPHA_2BPP ),
        ET( 0x8A57, 0,    0, PVRTC_SRGB_ALPHA_4BPP ),
        ET( 0x9270, 153,  0, EAC_R11 ),
        ET( 0x9271, 154,  0, EAC_SIGNED_R11 ),
        ET( 0x9272, 155,  0, EAC_RG11 ),
        ET( 0x9273, 156,  0, EAC_SIGNED_RG11 ),
        ET( 0x9274, 147,  0, ETC2_RGB ),
        ET( 0x9275, 148,  0, ETC2_SRGB ),
        ET( 0x9276, 149,  0, ETC2_RGB_ALPHA1 ),
        ET( 0x9277, 150,  0, ETC2_SRGB_ALPHA1 ),
        ET( 0x9278, 151,  0, ETC2_RGBA ),
        ET( 0x9279, 152,  0, ETC2_SRGB_ALPHA8 ),
        ET( 0x8D64, 0,    0, ETC1_RGB ),
		ET( 0x93B0, 157,  0, ASTC_RGBA_4x4 ),
        ET( 0x93B1, 159,  0, ASTC_RGBA_5x4 ),
        ET( 0x93B2, 161,  0, ASTC_RGBA_5x5 ),
        ET( 0x93B3, 163,  0, ASTC_RGBA_6x5 ),
        ET( 0x93B4, 165,  0, ASTC_RGBA_6x6 ),
        ET( 0x93B5, 167,  0, ASTC_RGBA_8x5 ),
        ET( 0x93B6, 169,  0, ASTC_RGBA_8x6 ),
        ET( 0x93B7, 171,  0, ASTC_RGBA_8x8 ),
        ET( 0x93B8, 173,  0, ASTC_RGBA_10x5 ),
        ET( 0x93B9, 175,  0, ASTC_RGBA_10x6 ),
        ET( 0x93BA, 177,  0, ASTC_RGBA_10x8 ),
        ET( 0x93BB, 179,  0, ASTC_RGBA_10x10 ),
        ET( 0x93BC, 181,  0, ASTC_RGBA_12x10 ),
		ET( 0x93BD, 183,  0, ASTC_RGBA_12x12 ),
        ET( 0x93D0, 158,  0, ASTC_SRGB_ALPHA_4x4 ),
        ET( 0x93D1, 160,  0, ASTC_SRGB_ALPHA_5x4 ),
        ET( 0x93D2, 162,  0, ASTC_SRGB_ALPHA_5x5 ),
        ET( 0x93D3, 164,  0, ASTC_SRGB_ALPHA_6x5 ),
        ET( 0x93D4, 166,  0, ASTC_SRGB_ALPHA_6x6 ),
        ET( 0x93D5, 168,  0, ASTC_SRGB_ALPHA_8x5 ),
        ET( 0x93D6, 170,  0, ASTC_SRGB_ALPHA_8x6 ),
        ET( 0x93D7, 172,  0, ASTC_SRGB_ALPHA_8x8 ),
        ET( 0x93D8, 174,  0, ASTC_SRGB_ALPHA_10x5 ),
        ET( 0x93D9, 176,  0, ASTC_SRGB_ALPHA_10x6 ),
        ET( 0x93DA, 178,  0, ASTC_SRGB_ALPHA_10x8 ),
        ET( 0x93DB, 180,  0, ASTC_SRGB_ALPHA_10x10 ),
        ET( 0x93DC, 182,  0, ASTC_SRGB_ALPHA_12x10 ),
        ET( 0x93DD, 184,  0, ASTC_SRGB_ALPHA_12x12 ),
        ET( 0x93C0, 0,    0, ASTC_RGBA_3x3x3 ),
        ET( 0x93C1, 0,    0, ASTC_RGBA_4x3x3 ),
        ET( 0x93C2, 0,    0, ASTC_RGBA_4x4x3 ),
        ET( 0x93C3, 0,    0, ASTC_RGBA_4x4x4 ),
        ET( 0x93C4, 0,    0, ASTC_RGBA_5x4x4 ),
        ET( 0x93C5, 0,    0, ASTC_RGBA_5x5x4 ),
        ET( 0x93C6, 0,    0, ASTC_RGBA_5x5x5 ),
        ET( 0x93C7, 0,    0, ASTC_RGBA_6x5x5 ),
        ET( 0x93C8, 0,    0, ASTC_RGBA_6x6x5 ),
        ET( 0x93C9, 0,    0, ASTC_RGBA_6x6x6 ),
        ET( 0x93E0, 0,    0, ASTC_SRGB_ALPHA_3x3x3 ),
        ET( 0x93E1, 0,    0, ASTC_SRGB_ALPHA_4x3x3 ),
        ET( 0x93E2, 0,    0, ASTC_SRGB_ALPHA_4x4x3 ),
        ET( 0x93E3, 0,    0, ASTC_SRGB_ALPHA_4x4x4 ),
        ET( 0x93E4, 0,    0, ASTC_SRGB_ALPHA_5x4x4 ),
        ET( 0x93E5, 0,    0, ASTC_SRGB_ALPHA_5x5x4 ),
        ET( 0x93E6, 0,    0, ASTC_SRGB_ALPHA_5x5x5 ),
        ET( 0x93E7, 0,    0, ASTC_SRGB_ALPHA_6x5x5 ),
        ET( 0x93E8, 0,    0, ASTC_SRGB_ALPHA_6x6x5 ),
        ET( 0x93E9, 0,    0, ASTC_SRGB_ALPHA_6x6x6 ),
        ET( 0x8C3D, 0,   67, RGB9_E5 ),
        ET( 0x8C3A, 0,    0, R11F_G11F_B10F ),
        ET( 0,      0,    0, R10F_G11F_B11F ),
        ET( 0,      0,    0, UYVY ),
        ET( 0,      0,    0, YUY2 ),
        ET( 0,      0,   69, G8R8G8B8 ),
        ET( 0,      0,   68, R8G8B8G8 )
	};

    void directBlockDecode(const TextureCompressionInfo& block, const Surface& surface, Memory memory, int xsize, int ysize)
    {
        const int blockImageSize = block.width * surface.format.bytes();
        const int blockImageStride = block.height * surface.stride;

        const bool origin = (block.getCompressionFlags() & TextureCompressionInfo::ORIGIN) != 0;
        const uint8* data = memory.address;

        for (int y = 0; y < ysize; ++y)
        {
            uint8* image = surface.image;
            int stride = surface.stride;

            if (origin)
            {
                image += (ysize - y) * blockImageStride;
                image -= stride;
                stride = -stride;
            }
            else
            {
                image += y * blockImageStride;
            }

            for (int x = 0; x < xsize; ++x)
            {
                block.decode(block, image, data, stride);
                image += blockImageSize;
                data += block.bytes;
            }
        }
    }

    void clipConvertBlockDecode(const TextureCompressionInfo& block, const Surface& surface, Memory memory, int xsize, int ysize)
    {
        Blitter blitter(surface.format, block.format);
        BlitRect rect;

        const bool origin = (block.getCompressionFlags() & TextureCompressionInfo::ORIGIN) != 0;
        const uint8* data = memory.address;

        rect.destStride = origin ? -surface.stride : surface.stride;
        rect.srcStride = block.width * block.format.bytes();

        Buffer temp(block.height * rect.srcStride);
        rect.srcImage = temp;

        const int pixelSize = block.width * surface.format.bytes();

        for (int y = 0; y < surface.height; y += block.height)
        {
            rect.destImage = surface.image + (origin ? surface.height - y - 1 : y) * surface.stride;
            rect.height = std::min(y + block.height, surface.height) - y; // vertical clipping

            for (int x = 0; x < surface.width; x += block.width)
            {
                block.decode(block, temp, data, rect.srcStride);

                rect.width = std::min(x + block.width, surface.width) - x; // horizontal clipping

                // TODO: async conversion
                blitter.convert(rect);

                rect.destImage += pixelSize;
                data += block.bytes;
            }
        }
    }

    void directSurfaceDecode(const TextureCompressionInfo& block, const Surface& surface, Memory memory, int xsize, int ysize)
    {
        TextureCompressionInfo temp = block;
        temp.width = surface.width;
        temp.height = surface.height;
        temp.decode(temp, surface.image, memory.address, surface.stride);
    }

    void clipConvertSurfaceDecode(const TextureCompressionInfo& block, const Surface& surface, Memory memory, int xsize, int ysize)
    {
        TextureCompressionInfo temp = block;
        temp.width = surface.width;
        temp.height = surface.height;

        Bitmap bitmap(surface.width, surface.height, block.format);
        temp.decode(temp, bitmap.image, memory.address, bitmap.stride);
        Surface(surface).blit(0, 0, bitmap);
    }

} // namespace

namespace mango
{

	namespace opengl
	{
		TextureCompression getTextureCompression(uint32 format)
		{
			for (auto& node : g_compressionInfoTable)
			{
				if (node.format_opengl == format)
					return node.compression;
			}

			return TextureCompression::NONE;
		}

		uint32 getTextureFormat(TextureCompression compression)
		{
			for (auto& node : g_compressionInfoTable)
			{
				if (node.compression == compression)
					return node.format_opengl;
			}

			return 0;
		}
	} // namespace opengl

	namespace vulkan
	{
		TextureCompression getTextureCompression(uint32 format)
		{
			for (auto& node : g_compressionInfoTable)
			{
				if (node.format_vulkan == format)
					return node.compression;
			}

			return TextureCompression::NONE;
		}

		uint32 getTextureFormat(TextureCompression compression)
		{
			for (auto& node : g_compressionInfoTable)
			{
				if (node.compression == compression)
					return node.format_vulkan;
			}

			return 0;
		}
	} // namespace vulkan

	namespace directx
	{
		TextureCompression getTextureCompression(uint32 format)
		{
			for (auto& node : g_compressionInfoTable)
			{
				if (node.format_dxgi == format)
					return node.compression;
			}

			return TextureCompression::NONE;
		}

		uint32 getTextureFormat(TextureCompression compression)
		{
			for (auto& node : g_compressionInfoTable)
			{
				if (node.compression == compression)
					return node.format_dxgi;
			}

			return 0;
		}
	} // namespace directx

	// ----------------------------------------------------------------------------
    // TextureCompressionInfo
    // ----------------------------------------------------------------------------

    TextureCompressionInfo::TextureCompressionInfo()
    : width(1), height(1), bytes(0), format(FORMAT_NONE), decode(nullptr), encode(nullptr), compression(TextureCompression::NONE)
    {
    }

    TextureCompressionInfo::TextureCompressionInfo(TextureCompression textureCompression)
    {
        const TextureCompressionInfo* info = getTextureCompressionInfo(textureCompression);
        *this = info ? *info : TextureCompressionInfo();

    }

    TextureCompressionInfo::TextureCompressionInfo(int width, int height, int bytes, const Format& format,
                                                   DecodeFunc decode, EncodeFunc encode, TextureCompression compression)
    : width(width), height(height), bytes(bytes), format(format), decode(decode), encode(encode), compression(compression)
    {
    }

    void TextureCompressionInfo::decompress(const Surface& surface, Memory memory) const
    {
        if (!decode)
            return;

        const int xsize = round_to_next(surface.width, width);
        const int ysize = round_to_next(surface.height, height);

        const bool noclip = surface.width == (xsize * width) &&
                            surface.height == (ysize * height);
        const bool noconvert = surface.format == format;
        const bool direct = noclip && noconvert;

        if (getCompressionFlags() & TextureCompressionInfo::SURFACE)
        {
            if (direct)
            {
                directSurfaceDecode(*this, surface, memory, xsize, ysize);
            }
            else
            {
                clipConvertSurfaceDecode(*this, surface, memory, xsize, ysize);
            }
        }
        else
        {
            if (direct)
            {
                directBlockDecode(*this, surface, memory, xsize, ysize);
            }
            else
            {
                clipConvertBlockDecode(*this, surface, memory, xsize, ysize);
            }
        }
    }

    void TextureCompressionInfo::compress(Memory memory, const Surface& surface) const
    {
        if (!encode)
            return;

        ConcurrentQueue queue;

        uint8* address = memory.address;

        const int xblocks = round_to_next(surface.width, width);
        const int yblocks = round_to_next(surface.height, height);

        for (int y = 0; y < yblocks; ++y)
        {
            queue.enqueue([this, y, xblocks, &surface, address]
            {
                Bitmap temp(width, height, format);
                uint8* data = address + y * xblocks * bytes;

                for (int x = 0; x < xblocks; ++x)
                {
                    Surface source(surface, x * width, y * height, width, height);
                    temp.blit(0, 0, source);

                    uint8* image = temp.address<uint8>();
                    encode(*this, data, image, temp.stride);
                    data += bytes;
                }
            });
        }

        queue.wait();
    }

} // namespace mango
