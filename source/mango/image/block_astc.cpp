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
            debugPrint("ERROR: Codec config init failed: %s\n", astcenc_get_error_string(status));
            return;
        }

        astcenc_context* context = nullptr;

        int threads = 1;

        status = astcenc_context_alloc(&config, threads, &context);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("ERROR: Codec context alloc failed: %s\n", astcenc_get_error_string(status));
            return;
        }

        //astcenc_contexti* ctx = &context->context;

        astcenc_image image;

        // TODO: stride
        // - when supported, rewrite the yflip logic in block.cpp

        image.dim_x = width;
        image.dim_y = height;
        image.dim_z = 1;
        image.data_type = isFloat ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;

        u8* ptr_image = (u8*)input;
        image.data = reinterpret_cast<void**>(&ptr_image);

        //printf("  %d x %d\n", image.dim_x, image.dim_y);
        const astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

        status = astcenc_compress_image(context, &image, &swizzle, output, 0, 0);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("ERROR: Codec compress failed: %s\n", astcenc_get_error_string(status));
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
            debugPrint("ERROR: Codec config init failed: %s\n", astcenc_get_error_string(status));
            return;
        }

        astcenc_context* context = nullptr;

        int threads = 1;

        status = astcenc_context_alloc(&config, threads, &context);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("ERROR: Codec context alloc failed: %s\n", astcenc_get_error_string(status));
            return;
        }

        //astcenc_contexti* ctx = &context->context;

        astcenc_image image;

        // TODO: stride
        // - when supported, rewrite the yflip logic in block.cpp

        image.dim_x = width;
        image.dim_y = height;
        image.dim_z = 1;
        image.data_type = isFloat ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;
        image.data = reinterpret_cast<void**>(&output);

        const astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

        status = astcenc_decompress_image(context, input, 0, &image, &swizzle, 0);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("ERROR: Codec decompress failed: %s\n", astcenc_get_error_string(status));
            astcenc_context_free(context);
            return;
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
        const int block_step = n * xblocks * 16;

        for (u32 y = 0; y < yblocks; y += n)
        {
            int segment_height = std::min(block.height * n, height);

            q.enqueue([=]
            {
                encode_rows(block, output, input, width, segment_height, stride);
            });

            height -= segment_height;
            input += segment_height * stride;
            output += block_step;
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
        const int block_step = n * xblocks * 16;

        for (u32 y = 0; y < yblocks; y += n)
        {
            int segment_height = std::min(block.height * n, height);

            q.enqueue([=]
            {
                decode_rows(block, output, input, width, segment_height, stride);
            });

            height -= segment_height;
            output += segment_height * stride;
            input += block_step;
        }
    }

} // namespace mango::image
