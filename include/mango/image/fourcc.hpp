/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "../core/endian.hpp"

namespace mango
{

    enum : u32
    {
        FOURCC_NONE       = make32le(0, 0, 0, 0),
        FOURCC_DXT1       = make32le('D', 'X', 'T', '1'),
        FOURCC_DXT2       = make32le('D', 'X', 'T', '2'),
        FOURCC_DXT3       = make32le('D', 'X', 'T', '3'),
        FOURCC_DXT4       = make32le('D', 'X', 'T', '4'),
        FOURCC_DXT5       = make32le('D', 'X', 'T', '5'),
        FOURCC_ATI1       = make32le('A', 'T', 'I', '1'),
        FOURCC_ATI2       = make32le('A', 'T', 'I', '2'),
        FOURCC_BC4U       = make32le('B', 'C', '4', 'U'),
        FOURCC_BC4S       = make32le('B', 'C', '4', 'S'),
        FOURCC_BC5U       = make32le('B', 'C', '5', 'U'),
        FOURCC_BC5S       = make32le('B', 'C', '5', 'S'),
        FOURCC_BC6H       = make32le('B', 'C', '6', 'H'),
        FOURCC_BC7U       = make32le('B', 'C', '7', 'U'),
        FOURCC_AT1N       = make32le('A', 'T', '1', 'N'),
        FOURCC_AT2N       = make32le('A', 'T', '2', 'N'),
        FOURCC_PTC2       = make32le('P', 'T', 'C', '2'),
        FOURCC_UYVY       = make32le('U', 'Y', 'V', 'Y'),
        FOURCC_YUY2       = make32le('Y', 'U', 'Y', '2'),
        FOURCC_G8R8G8B8   = make32le('G', 'R', 'G', 'B'),
        FOURCC_R8G8B8G8   = make32le('R', 'G', 'B', 'G'),
    };

} // namespace mango
