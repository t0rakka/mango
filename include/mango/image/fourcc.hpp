/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

namespace mango
{

    constexpr uint32 makeFourCC(char c0, char c1, char c2, char c3)
    {
        return (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
    }

    constexpr uint32 makeReverseFourCC(char c3, char c2, char c1, char c0)
    {
        return (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
    }

    enum : uint32
    {
        FOURCC_NONE       = makeFourCC(0, 0, 0, 0),
        FOURCC_DXT1       = makeFourCC('D', 'X', 'T', '1'),
        FOURCC_DXT2       = makeFourCC('D', 'X', 'T', '2'),
        FOURCC_DXT3       = makeFourCC('D', 'X', 'T', '3'),
        FOURCC_DXT4       = makeFourCC('D', 'X', 'T', '4'),
        FOURCC_DXT5       = makeFourCC('D', 'X', 'T', '5'),
        FOURCC_ATI1       = makeFourCC('A', 'T', 'I', '1'),
        FOURCC_ATI2       = makeFourCC('A', 'T', 'I', '2'),
        FOURCC_BC4U       = makeFourCC('B', 'C', '4', 'U'),
        FOURCC_BC4S       = makeFourCC('B', 'C', '4', 'S'),
        FOURCC_BC5U       = makeFourCC('B', 'C', '5', 'U'),
        FOURCC_BC5S       = makeFourCC('B', 'C', '5', 'S'),
        FOURCC_BC6H       = makeFourCC('B', 'C', '6', 'H'),
        FOURCC_BC7U       = makeFourCC('B', 'C', '7', 'U'),
        FOURCC_AT1N       = makeFourCC('A', 'T', '1', 'N'),
        FOURCC_AT2N       = makeFourCC('A', 'T', '2', 'N'),
        FOURCC_PTC2       = makeFourCC('P', 'T', 'C', '2'),
        FOURCC_UYVY       = makeFourCC('U', 'Y', 'V', 'Y'),
        FOURCC_YUY2       = makeFourCC('Y', 'U', 'Y', '2'),
        FOURCC_G8R8G8B8   = makeFourCC('G', 'R', 'G', 'B'),
        FOURCC_R8G8B8G8   = makeFourCC('R', 'G', 'B', 'G'),
    };

} // namespace mango
