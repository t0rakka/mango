/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include "format.hpp"
#include "compression.hpp"

namespace mango
{

    struct ImageHeader
    {
        int     width = 0;   // width
        int     height = 0;  // height
        int     depth = 0;   // depth
        int     levels = 0;  // mipmap levels
        int     faces = 0;   // cubemap faces
        bool    palette = false; // decoder supports palette export
        Format  format;
        TextureCompression compression = TextureCompression::NONE;
    };

} // namespace mango
