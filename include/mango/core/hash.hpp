/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"
#include "memory.hpp"

namespace mango
{

    void sha1(uint32 hash[5], Memory memory);
    void md5(uint32 hash[4], Memory memory);

} // namespace mango
