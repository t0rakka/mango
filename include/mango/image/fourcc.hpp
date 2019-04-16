/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "../core/endian.hpp"

namespace mango
{

    enum : u32
    {
        FOURCC_NONE       = u32_mask(0, 0, 0, 0),
        FOURCC_DXT1       = u32_mask('D', 'X', 'T', '1'),
        FOURCC_DXT2       = u32_mask('D', 'X', 'T', '2'),
        FOURCC_DXT3       = u32_mask('D', 'X', 'T', '3'),
        FOURCC_DXT4       = u32_mask('D', 'X', 'T', '4'),
        FOURCC_DXT5       = u32_mask('D', 'X', 'T', '5'),
        FOURCC_ATI1       = u32_mask('A', 'T', 'I', '1'),
        FOURCC_ATI2       = u32_mask('A', 'T', 'I', '2'),
        FOURCC_BC4U       = u32_mask('B', 'C', '4', 'U'),
        FOURCC_BC4S       = u32_mask('B', 'C', '4', 'S'),
        FOURCC_BC5U       = u32_mask('B', 'C', '5', 'U'),
        FOURCC_BC5S       = u32_mask('B', 'C', '5', 'S'),
        FOURCC_BC6H       = u32_mask('B', 'C', '6', 'H'),
        FOURCC_BC7U       = u32_mask('B', 'C', '7', 'U'),
        FOURCC_AT1N       = u32_mask('A', 'T', '1', 'N'),
        FOURCC_AT2N       = u32_mask('A', 'T', '2', 'N'),
        FOURCC_PTC2       = u32_mask('P', 'T', 'C', '2'),
        FOURCC_UYVY       = u32_mask('U', 'Y', 'V', 'Y'),
        FOURCC_YUY2       = u32_mask('Y', 'U', 'Y', '2'),
        FOURCC_G8R8G8B8   = u32_mask('G', 'R', 'G', 'B'),
        FOURCC_R8G8B8G8   = u32_mask('R', 'G', 'B', 'G'),
    };

} // namespace mango
