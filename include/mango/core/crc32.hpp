/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"

namespace mango
{

    uint32 crc32(uint32 crc, const uint8* data, size_t size);
    uint32 crc32c(uint32 crc, const uint8* data, size_t size);

} // namespace mango
