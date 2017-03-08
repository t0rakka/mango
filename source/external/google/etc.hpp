/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2014 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/configure.hpp>
#include <mango/image/compression.hpp>

#ifdef MANGO_ENABLE_LICENSE_APACHE

namespace mango
{

    void decode_block_etc1     (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);
    void decode_block_etc2     (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);
    void decode_block_etc2_eac (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);
    void decode_block_eac_r11  (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);
    void decode_block_eac_rg11 (const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);

} // namespace mango

#endif // MANGO_ENABLE_LICENSE_APACHE
