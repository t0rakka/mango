/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/compression.hpp>
#include <mango/image/surface.hpp>
#include <mango/image/color.hpp>

#ifdef MANGO_LICENSE_ENABLE_APACHE

#include "../../external/astc/astcenc.h"
#include "../../external/astc/astcenc_internal.h"
#include "../../external/astc/astcenc_diagnostic_trace.h"

namespace mango::image
{

    void encode_surface_astc(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(stride);

        // NOTE: implement stride and the temporary copy isn't needed anymore
        Surface source(info.width, info.height, info.format, stride, input);
        Bitmap temp(source, info.format);

        TextureCompression block(info.compression);

        // image size
        int width = info.width;
        int height = info.height;

        // Compute the number of ASTC blocks in each dimension
        u32 xblocks = block.getBlocksX(width);
        u32 yblocks = block.getBlocksY(height);

        bool isFloat = (info.compression & TextureCompression::FLOAT) != 0;
        bool isSRGB = (info.compression & TextureCompression::SRGB) != 0;

        const astcenc_profile profiles [] =
        {
            ASTCENC_PRF_LDR,
            ASTCENC_PRF_LDR_SRGB,
            ASTCENC_PRF_HDR,
            ASTCENC_PRF_HDR // Technically there is no "HDR + sRGB" but here we are
        };

        astcenc_profile profile = profiles[isFloat * 2 + isSRGB];
        float quality = ASTCENC_PRE_MEDIUM;

        astcenc_error status;
        astcenc_config config;

        u32 flags = 0;

        status = astcenc_config_init(profile, block.width, block.height, 1, quality, flags, &config);
        if (status != ASTCENC_SUCCESS)
        {
            printLine(Print::Error, "[ASTC] astcenc_config_init: {}", astcenc_get_error_string(status));
            return;
        }

        astcenc_context* context;
        u32 thread_count = 1;

        status = astcenc_context_alloc(&config, thread_count, &context);
        if (status != ASTCENC_SUCCESS)
        {
            printLine(Print::Error, "[ASTC] astcenc_context_alloc: {}", astcenc_get_error_string(status));
            return;
        }

        astcenc_image image;

        image.dim_x = width;
        image.dim_y = height;
        image.dim_z = 1;
        image.data_type = isFloat ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;

        u8* ptr_image = temp.image;
        image.data = reinterpret_cast<void**>(&ptr_image);

        const astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

        size_t output_bytes = xblocks * yblocks * 16;

        u32 thread_index = 0;

        status = astcenc_compress_image(context, &image, &swizzle, output, output_bytes, thread_index);
        if (status != ASTCENC_SUCCESS)
        {
            printLine(Print::Error, "[ASTC] astcenc_compress_image: {}", astcenc_get_error_string(status));
        }

        astcenc_context_free(context);
    }

    void decode_surface_astc(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(stride);

        // NOTE: implement stride and the temporary copy isn't needed anymore
        Bitmap temp(info.width, info.height, info.format);

        TextureCompression block(info.compression);

        // image size
        int width = info.width;
        int height = info.height;

        // Compute the number of ASTC blocks in each dimension
        u32 xblocks = block.getBlocksX(width);
        u32 yblocks = block.getBlocksY(height);

        bool isFloat = (block.compression & TextureCompression::FLOAT) != 0;
        bool isSRGB = (block.compression & TextureCompression::SRGB) != 0;

        const astcenc_profile profiles [] =
        {
            ASTCENC_PRF_LDR,
            ASTCENC_PRF_LDR_SRGB,
            ASTCENC_PRF_HDR,
            ASTCENC_PRF_HDR // Technically there is no "HDR + sRGB" but here we are
        };

        astcenc_profile profile = profiles[isFloat * 2 + isSRGB];
        float quality = ASTCENC_PRE_MEDIUM;

        astcenc_error status;
        astcenc_config config;

        u32 flags = ASTCENC_FLG_DECOMPRESS_ONLY;

        status = astcenc_config_init(profile, block.width, block.height, 1, quality, flags, &config);
        if (status != ASTCENC_SUCCESS)
        {
            printLine(Print::Error, "[ASTC] astcenc_config_init: {}", astcenc_get_error_string(status));
            return;
        }

        astcenc_context* context;
        u32 thread_count = 1;

        status = astcenc_context_alloc(&config, thread_count, &context);
        if (status != ASTCENC_SUCCESS)
        {
            printLine(Print::Error, "[ASTC] astcenc_context_alloc: {}", astcenc_get_error_string(status));
            return;
        }

        astcenc_image image;

        image.dim_x = width;
        image.dim_y = height;
        image.dim_z = 1;
        image.data_type = isFloat ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;

        u8* ptr_image = temp.image;
        image.data = reinterpret_cast<void**>(&ptr_image);

        const astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

        size_t input_bytes = xblocks * yblocks * 16;

        u32 thread_index = 0;

        status = astcenc_decompress_image(context, input, input_bytes, &image, &swizzle, thread_index);
        if (status != ASTCENC_SUCCESS)
        {
            printLine(Print::Error, "[ASTC] astcenc_decompress_image: {}", astcenc_get_error_string(status));
        }

        // NOTE: implement stride and the temporary copy isn't needed anymore
        Surface target(info.width, info.height, info.format, stride, output);
        target.blit(0, 0, temp);

        astcenc_context_free(context);
    }

} // namespace mango::image

#endif // MANGO_LICENSE_ENABLE_APACHE
