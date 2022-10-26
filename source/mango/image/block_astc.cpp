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

    void encode_rows(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        TextureCompression temp(info.compression);

        bool isFloat = (info.compression & TextureCompression::FLOAT) != 0;

        // Block size
        const u32 block_x = temp.width;
        const u32 block_y = temp.height;

        // Compute the number of ASTC blocks in each dimension
        u32 xblocks = ceil_div(info.width, block_x);
        u32 yblocks = ceil_div(info.height, block_y);

        const astcenc_profile profile = ASTCENC_PRF_LDR;
        const float quality = ASTCENC_PRE_MEDIUM;
        const astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

        astcenc_config config;
        config.block_x = block_x;
        config.block_y = block_y;
        config.block_z = 1;
        config.profile = profile;

        astcenc_error status;
        status = astcenc_config_init(profile, block_x, block_y, 1, quality, 0, &config);
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

        image.dim_x = info.width;
        image.dim_y = info.height;
        image.dim_z = 1;
        image.data_type = isFloat ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;

        u8* ptr_image = (u8*)input;
        image.data = reinterpret_cast<void**>(&ptr_image);

        //printf("  %d x %d\n", image.dim_x, image.dim_y);

        size_t len = xblocks * yblocks * 16;

        status = astcenc_compress_image(context, &image, &swizzle, output, len, 0);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("ERROR: Codec compress failed: %s\n", astcenc_get_error_string(status));
            astcenc_context_free(context);
            return;
        }

        astcenc_context_free(context);
    }

    void decode_rows(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        TextureCompression temp(info.compression);

        bool isFloat = (info.compression & TextureCompression::FLOAT) != 0;

        // Block size
        const u32 block_x = temp.width;
        const u32 block_y = temp.height;

        // Compute the number of ASTC blocks in each dimension
        u32 xblocks = ceil_div(info.width, block_x);
        u32 yblocks = ceil_div(info.height, block_y);

        const astcenc_profile profile = ASTCENC_PRF_LDR;
        const float quality = ASTCENC_PRE_MEDIUM;
        const astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

        astcenc_config config;
        config.block_x = block_x;
        config.block_y = block_y;
        config.profile = profile;

        astcenc_error status;
        status = astcenc_config_init(profile, block_x, block_y, 1, quality, 0, &config);
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

        image.dim_x = info.width;
        image.dim_y = info.height;
        image.dim_z = 1;
        image.data_type = isFloat ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;
        image.data = reinterpret_cast<void**>(&output);

        size_t len = xblocks * yblocks * 16;

        status = astcenc_decompress_image(context, input, len, &image, &swizzle, 0);
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
        TextureCompression temp(info.compression);

        int width = info.width;
        int height = info.height;
        int block_x = temp.width;
        int block_y = temp.height;

        // Compute the number of ASTC blocks in each dimension
        u32 xblocks = ceil_div(width, block_x);
        u32 yblocks = ceil_div(height, block_y);

        ConcurrentQueue q;

        int n = 8;

        for (u32 y = 0; y < yblocks; y += n)
        {
            TextureCompression xx = info;
            xx.height = std::min(block_y * n, height);

            q.enqueue([=]
            {
                encode_rows(xx, output + y * xblocks * 16, input, stride);
            });

            height -= xx.height;
            input += xx.height * stride;
        }
    }

    void decode_surface_astc(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {

        TextureCompression temp(info.compression);

        int width = info.width;
        int height = info.height;
        int block_x = temp.width;
        int block_y = temp.height;

        // Compute the number of ASTC blocks in each dimension
        u32 xblocks = ceil_div(width, block_x);
        u32 yblocks = ceil_div(height, block_y);

        ConcurrentQueue q;

        int n = 8;

        for (u32 y = 0; y < yblocks; y += n)
        {
            TextureCompression xx = info;
            xx.height = std::min(block_y * n, height);

            q.enqueue([=]
            {
                decode_rows(xx, output, input + y * xblocks * 16, stride);
            });

            height -= xx.height;
            output += xx.height * stride;
        }
    }

} // namespace mango::image
