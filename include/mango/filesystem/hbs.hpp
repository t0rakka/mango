/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/configure.hpp>
#include <mango/core/bits.hpp>

namespace mango::filesystem
{

    // major = high byte, minor = low byte (1.0 -> 0x0100)
    constexpr u32 HBS_VERSION = 0x0100;

    enum : u32
    {
        HBS_MAGIC0 = u32_mask('h', 'b', 's', '0'),
        HBS_MAGIC1 = u32_mask('h', 'b', 's', '1'),
        HBS_MAGIC2 = u32_mask('h', 'b', 's', '2'),
        HBS_MAGIC3 = u32_mask('h', 'b', 's', '3'),
    };

} // namespace mango::filesystem
