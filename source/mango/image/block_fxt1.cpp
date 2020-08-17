/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/endian.hpp>
#include <mango/image/compression.hpp>

// https://www.khronos.org/registry/OpenGL/extensions/3DFX/3DFX_texture_compression_FXT1.txt

namespace
{
    using namespace mango;

} // namespace

namespace mango
{

    void decode_block_fxt1(const TextureCompressionInfo& info, u8* out, const u8* in, int stride)
    {
        MANGO_UNREFERENCED(info);
        MANGO_UNREFERENCED(out);
        MANGO_UNREFERENCED(in);
        MANGO_UNREFERENCED(stride);
        // TODO
    }

} // namespace mango
