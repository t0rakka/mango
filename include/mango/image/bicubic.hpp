/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/configure.hpp>
#include <mango/image/surface.hpp>

namespace mango::image
{

    // NOTE: Experimental function - name could change until this notice is removed.
    // NOTE: The surfaces MUST be 32 bit UNORM RGBA or BGRA

    void u32_bicubic_blit(const Surface& dest, const Surface& source, float x, float y, float xsize, float ysize);

} // namespace mango::image
