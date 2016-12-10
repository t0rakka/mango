/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"

namespace mango
{

    void sha1(uint32 hash[5], const uint8* data, size_t size);
    void md5(uint32 hash[4], const uint8* data, size_t size);

} // namespace mango
