/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/compression.hpp>
#include <mango/image/color.hpp>

#include "../../source/external/astc/astcenc.h"
#include "../../source/external/astc/astcenc_internal_entry.h"
#include "../../source/external/astc/astcenc_diagnostic_trace.h"

namespace
{
    using namespace mango;
    using namespace mango::image;

} // namespace

namespace mango::image
{

    void decode_surface_astc(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        TextureCompression temp(info.compression);

        //bool isFloat = (info.compression & TextureCompression::FLOAT) != 0;

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

        status = astcenc_context_alloc(&config, 1, &context);
        if (status != ASTCENC_SUCCESS)
        {
            debugPrint("ERROR: Codec context alloc failed: %s\n", astcenc_get_error_string(status));
            return;
        }

        astcenc_contexti* ctx = &context->context;

        astcenc_image image;

        // TODO: stride
        image.dim_x = info.width;
        image.dim_y = info.height;
        image.dim_z = 1;
        image.data_type = ASTCENC_TYPE_U8;
        image.data = reinterpret_cast<void**>(&output);

        ConcurrentQueue q;

        for (int y = 0; y < yblocks; ++y)
        {
            q.enqueue([=, &image]
            {
                image_block blk;
                blk.texel_count = u8(block_x * block_y);

                for (int x = 0; x < xblocks; ++x)
                {
                    u32 offset = ((y * xblocks) + x) * 16;
                    const u8* bp = input + offset;

                    const physical_compressed_block& pcb = *reinterpret_cast<const physical_compressed_block*>(bp);
                    symbolic_compressed_block scb;

                    physical_to_symbolic(*ctx->bsd, pcb, scb);

                    decompress_symbolic_block(ctx->config.profile, *ctx->bsd,
                                            x * block_x, y * block_y, 0,
                                            scb, blk);

                    store_image_block(image, blk, *ctx->bsd,
                                    x * block_x, y * block_y, 0, swizzle);
                }
            });
        }

        astcenc_context_free(context);
    }

} // namespace mango::image
