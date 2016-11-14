/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2014 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#ifndef MANGO_INCLUDE_ASTC
#define MANGO_INCLUDE_ASTC

#include <mango/core/configure.hpp>
#include <mango/image/compression.hpp>

#ifdef MANGO_ENABLE_LICENSE_APACHE

namespace mango
{

    void decode_block_astc(const TextureCompressionInfo& info, uint8* output, const uint8* input, int stride);

} // namespace mango

#endif // MANGO_ENABLE_LICENSE_APACHE
#endif
