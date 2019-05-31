/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"
#include "memory.hpp"

namespace mango
{

    u32 crc32(u32 crc, Memory memory);
    u32 crc32c(u32 crc, Memory memory);

} // namespace mango
