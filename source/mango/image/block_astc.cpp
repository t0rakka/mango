/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/compression.hpp>
#include <mango/image/color.hpp>

#include "../../external/astc/astcenc.h"
#include "../../external/astc/astcenc_internal_entry.h"
#include "../../external/astc/astcenc_diagnostic_trace.h"

// TODO: clean up the code
// TODO: stride support
// TODO: optimize; the native threading is still faster

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

        status = astcenc_config_init(profile, info.width, info.height, 1, quality, 0, &config);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("[ASTC] Codec config init failed: %s\n", astcenc_get_error_string(status));
            return;
        }

        astcenc_context* context = nullptr;

        int threads = 1;

        status = astcenc_context_alloc(&config, threads, &context);
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

        status = astcenc_compress_image(context, &image, &swizzle, output, 0, 0);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("[ASTC] Codec compress failed: %s\n", astcenc_get_error_string(status));
            astcenc_context_free(context);
            return;
        }

        astcenc_context_free(context);
    }

    void decode_rows(const TextureCompression& info, u8* output, const u8* input, int width, int height, size_t stride)
    {
        bool isFloat = (info.compression & TextureCompression::FLOAT) != 0;

        const astcenc_profile profile = ASTCENC_PRF_LDR;
        const float quality = ASTCENC_PRE_MEDIUM;

        astcenc_error status;
        astcenc_config config;

        status = astcenc_config_init(profile, info.width, info.height, 1, quality, 0, &config);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("[ASTC] Codec config init failed: %s\n", astcenc_get_error_string(status));
            return;
        }

        astcenc_context* context = nullptr;

        int threads = 1;

        status = astcenc_context_alloc(&config, threads, &context);
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

        astcenc_contexti* ctx = &context->context;

        unsigned int block_x = ctx->config.block_x;
        unsigned int block_y = ctx->config.block_y;
        unsigned int block_z = ctx->config.block_z;

        unsigned int xblocks = (image.dim_x + block_x - 1) / block_x;
        unsigned int yblocks = (image.dim_y + block_y - 1) / block_y;
        unsigned int zblocks = (image.dim_z + block_z - 1) / block_z;

        image_block blk;
        blk.texel_count = static_cast<uint8_t>(block_x * block_y * block_z);

        astcenc_decompress_reset(context);

        // Only the first thread actually runs the initializer
        context->manage_decompress.init(zblocks * yblocks * xblocks);

        constexpr int z = 0;

        for (u32 y = 0; y < yblocks; ++y)
        {
            for (u32 x = 0; x < xblocks; ++x)
            {
                const physical_compressed_block& pcb = *reinterpret_cast<const physical_compressed_block*>(input);
                symbolic_compressed_block scb;

                physical_to_symbolic(*ctx->bsd, pcb, scb);

                decompress_symbolic_block(ctx->config.profile, *ctx->bsd,
                                          x * block_x, y * block_y, z * block_z,
                                          scb, blk);

                store_image_block(image, blk, *ctx->bsd,
                                  x * block_x, y * block_y, z * block_z, swizzle);

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

        int n = 8;

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

        constexpr int n = 16;

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
