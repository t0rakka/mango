/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include "format.hpp"
#include "compression.hpp"

namespace mango
{

    struct ImageHeader
    {
        int     width = 0;
        int     height = 0;
        int     depth = 0;
        int     levels = 0;
        int     faces = 0;
        Format  format;
		TextureCompression compression = TextureCompression::NONE;
    };

} // namespace mango
