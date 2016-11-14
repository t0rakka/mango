/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#define MAKE_FOURCC(c0, c1, c2, c3) \
	((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

#define MAKE_REVERSE_FOURCC(c3, c2, c1, c0) \
	((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

namespace mango
{

    enum
    {
        FOURCC_NONE       = MAKE_FOURCC(0, 0, 0, 0),
        FOURCC_DXT1       = MAKE_FOURCC('D', 'X', 'T', '1'),
        FOURCC_DXT2       = MAKE_FOURCC('D', 'X', 'T', '2'),
        FOURCC_DXT3       = MAKE_FOURCC('D', 'X', 'T', '3'),
        FOURCC_DXT4       = MAKE_FOURCC('D', 'X', 'T', '4'),
        FOURCC_DXT5       = MAKE_FOURCC('D', 'X', 'T', '5'),
        FOURCC_ATI1       = MAKE_FOURCC('A', 'T', 'I', '1'),
        FOURCC_ATI2       = MAKE_FOURCC('A', 'T', 'I', '2'),
        FOURCC_BC4U       = MAKE_FOURCC('B', 'C', '4', 'U'),
        FOURCC_BC4S       = MAKE_FOURCC('B', 'C', '4', 'S'),
        FOURCC_BC5U       = MAKE_FOURCC('B', 'C', '5', 'U'),
        FOURCC_BC5S       = MAKE_FOURCC('B', 'C', '5', 'S'),
        FOURCC_BC6H       = MAKE_FOURCC('B', 'C', '6', 'H'),
        FOURCC_BC7U       = MAKE_FOURCC('B', 'C', '7', 'U'),
        FOURCC_AT1N       = MAKE_FOURCC('A', 'T', '1', 'N'),
        FOURCC_AT2N       = MAKE_FOURCC('A', 'T', '2', 'N'),
        FOURCC_PTC2       = MAKE_FOURCC('P', 'T', 'C', '2'),
        FOURCC_UYVY       = MAKE_FOURCC('U', 'Y', 'V', 'Y'),
        FOURCC_YUY2       = MAKE_FOURCC('Y', 'U', 'Y', '2'),
        FOURCC_G8R8G8B8   = MAKE_FOURCC('G', 'R', 'G', 'B'),
        FOURCC_R8G8B8G8   = MAKE_FOURCC('R', 'G', 'B', 'G'),
    };

} // namespace mango
