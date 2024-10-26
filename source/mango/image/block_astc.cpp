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

namespace mango::image
{

    void encode_surface_astc(const TextureCompression& info, u8* output, const Surface& input)
    {
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

        status = astcenc_config_init(profile, info.width, info.height, 1, quality, flags, &config);
        if (status != ASTCENC_SUCCESS)
        {
            printLine(Print::Error, "[ASTC] astcenc_config_init: {}", astcenc_get_error_string(status));
            return;
        }

        astcenc_context* context;
        u32 thread_count = u32(ThreadPool::getHardwareConcurrency());

        status = astcenc_context_alloc(&config, thread_count, &context);
        if (status != ASTCENC_SUCCESS)
        {
            printLine(Print::Error, "[ASTC] astcenc_context_alloc: {}", astcenc_get_error_string(status));
            return;
        }

        // image size
        int width = input.width;
        int height = input.height;

        // NOTE: implement stride and the temporary copy isn't needed anymore
        Bitmap temp(input, info.format);

        astcenc_image image;

        image.dim_x = width;
        image.dim_y = height;
        image.dim_z = 1;
        image.data_type = isFloat ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;
        image.data = reinterpret_cast<void**>(&temp.image);

        const astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

        size_t output_bytes = info.getBlockBytes(width, height);

        if (thread_count > 1)
        {
            ConcurrentQueue q;

            for (u32 thread_index = 0; thread_index < thread_count; ++thread_index)
            {
                q.enqueue([&, thread_index]
                {
                    auto status = astcenc_compress_image(context, &image, &swizzle, output, output_bytes, thread_index);
                    MANGO_UNREFERENCED(status);
                });
            }
        }
        else
        {
            u32 thread_index = 0;
            auto status = astcenc_compress_image(context, &image, &swizzle, output, output_bytes, thread_index);
            if (status != ASTCENC_SUCCESS)
            {
                printLine(Print::Error, "[ASTC] astcenc_compress_image: {}", astcenc_get_error_string(status));
            }
        }

        astcenc_context_free(context);
    }

    void decode_surface_astc(const TextureCompression& info, const Surface& output, const u8* input)
    {
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

        u32 flags = ASTCENC_FLG_DECOMPRESS_ONLY;

        status = astcenc_config_init(profile, info.width, info.height, 1, quality, flags, &config);
        if (status != ASTCENC_SUCCESS)
        {
            printLine(Print::Error, "[ASTC] astcenc_config_init: {}", astcenc_get_error_string(status));
            return;
        }

        astcenc_context* context;
        u32 thread_count = u32(ThreadPool::getHardwareConcurrency());

        status = astcenc_context_alloc(&config, thread_count, &context);
        if (status != ASTCENC_SUCCESS)
        {
            printLine(Print::Error, "[ASTC] astcenc_context_alloc: {}", astcenc_get_error_string(status));
            return;
        }

        // image size
        int width = output.width;
        int height = output.height;

        // NOTE: implement stride and the temporary copy isn't needed anymore
        Bitmap temp(width, height, info.format);

        astcenc_image image;

        image.dim_x = width;
        image.dim_y = height;
        image.dim_z = 1;
        image.data_type = isFloat ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;
        image.data = reinterpret_cast<void**>(&temp.image);

        const astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

        size_t input_bytes = info.getBlockBytes(width, height);

        if (thread_count > 1)
        {
            ConcurrentQueue q;

            for (u32 thread_index = 0; thread_index < thread_count; ++thread_index)
            {
                q.enqueue([&, thread_index]
                {
                    auto status = astcenc_decompress_image(context, input, input_bytes, &image, &swizzle, thread_index);
                    MANGO_UNREFERENCED(status);
                });
            }
        }
        else
        {
            u32 thread_index = 0;
            status = astcenc_decompress_image(context, input, input_bytes, &image, &swizzle, thread_index);
            if (status != ASTCENC_SUCCESS)
            {
                printLine(Print::Error, "[ASTC] astcenc_decompress_image: {}", astcenc_get_error_string(status));
            }
        }

        // NOTE: implement stride and the temporary copy isn't needed anymore
        output.blit(0, 0, temp);

        astcenc_context_free(context);
    }

} // namespace mango::image

#endif // MANGO_LICENSE_ENABLE_APACHE
