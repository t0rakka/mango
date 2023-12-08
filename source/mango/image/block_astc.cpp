/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/compression.hpp>
#include <mango/image/color.hpp>

#ifdef MANGO_LICENSE_ENABLE_APACHE

#include "../../external/astc/astcenc.h"
#include "../../external/astc/astcenc_internal.h"
#include "../../external/astc/astcenc_diagnostic_trace.h"

// MANGO TODO: stride support

namespace
{
    using namespace mango;
    using namespace mango::image;

    void encode_rows(const TextureCompression& info, u8* output, const u8* input, int width, int height, size_t stride)
    {
        bool isFloat = (info.compression & TextureCompression::FLOAT) != 0;

        const astcenc_profile profile = ASTCENC_PRF_LDR;
        const float quality = ASTCENC_PRE_MEDIUM;

        astcenc_error status;
        astcenc_config config;

        u32 flags = 0;

        status = astcenc_config_init(profile, info.width, info.height, 1, quality, flags, &config);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("[ASTC] Codec config init failed: %s\n", astcenc_get_error_string(status));
            return;
        }

        astcenc_context context;

        status = astcenc_context_alloc(&config, context);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("[ASTC] Codec context alloc failed: %s\n", astcenc_get_error_string(status));
            return;
        }

        astcenc_image image;

        image.dim_x = width;
        image.dim_y = height;
        image.dim_z = 1;
        image.data_type = isFloat ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;

        u8* ptr_image = (u8*)input;
        image.data = reinterpret_cast<void**>(&ptr_image);

        const astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

        status = astcenc_compress_image(context, image, &swizzle, output);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("[ASTC] Codec compress failed: %s\n", astcenc_get_error_string(status));
        }

        astcenc_context_free(context);
    }

    void decode_rows(const TextureCompression& block, u8* output, const u8* input, int width, int height, size_t stride)
    {
        bool isFloat = (block.compression & TextureCompression::FLOAT) != 0;

        const astcenc_profile profile = ASTCENC_PRF_LDR;
        const float quality = ASTCENC_PRE_MEDIUM;

        astcenc_error status;
        astcenc_config config;

        u32 flags = ASTCENC_FLG_DECOMPRESS_ONLY;

        status = astcenc_config_init(profile, block.width, block.height, 1, quality, flags, &config);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("[ASTC] Codec config init failed: %s\n", astcenc_get_error_string(status));
            return;
        }

        astcenc_context context;

        status = astcenc_context_alloc(&config, context);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("[ASTC] Codec context alloc failed: %s\n", astcenc_get_error_string(status));
            return;
        }

        astcenc_image image;

        image.dim_x = width;
        image.dim_y = height;
        image.dim_z = 1;
        image.data_type = isFloat ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;
        image.data = reinterpret_cast<void**>(&output);

        const astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

        u32 xblocks = block.getBlocksX(width);
        u32 yblocks = block.getBlocksY(height);

        image_block blk;
        blk.texel_count = u8(block.width * block.height);

        for (u32 y = 0; y < yblocks; ++y)
        {
            u32 yoffset = y * block.height;
            u32 xoffset = 0;

            for (u32 x = 0; x < xblocks; ++x)
            {
                const physical_compressed_block& pcb = *reinterpret_cast<const physical_compressed_block*>(input);
                symbolic_compressed_block scb;

                physical_to_symbolic(*context.bsd, pcb, scb);
                decompress_symbolic_block(context.config.profile, *context.bsd, xoffset, yoffset, 0, scb, blk);
                store_image_block(image, blk, *context.bsd, xoffset, yoffset, 0, swizzle);

                xoffset += block.width;
                input += 16;
            }
        }

        astcenc_context_free(context);
    }

} // namespace mango

namespace mango::image
{

    void encode_surface_astc(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        TextureCompression block(info.compression);

        // image size
        int width = info.width;
        int height = info.height;

        // Compute the number of ASTC blocks in each dimension
        u32 xblocks = block.getBlocksX(width);
        u32 yblocks = block.getBlocksY(height);

        ConcurrentQueue q;

        int n = 12;

        for (u32 y = 0; y < yblocks; y += n)
        {
            int segment_height = std::min(block.height * n, height);

            q.enqueue([=]
            {
                encode_rows(block, output, input, width, segment_height, stride);
            });

            height -= segment_height;
            input += segment_height * stride;
            output += n * xblocks * 16;
        }
    }

    void decode_surface_astc(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        TextureCompression block(info.compression);

        // image size
        int width = info.width;
        int height = info.height;

        // Compute the number of ASTC blocks in each dimension
        u32 xblocks = block.getBlocksX(width);
        u32 yblocks = block.getBlocksY(height);

        ConcurrentQueue q;

        constexpr int n = 24;

        for (u32 y = 0; y < yblocks; y += n)
        {
            int segment_height = std::min(block.height * n, height);

            q.enqueue([=]
            {
                decode_rows(block, output, input, width, segment_height, stride);
            });

            height -= segment_height;
            output += segment_height * stride;
            input += n * xblocks * 16;
        }
    }

} // namespace mango::image

#endif // MANGO_LICENSE_ENABLE_APACHE
