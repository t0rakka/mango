/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"
#include "memory.hpp"

namespace mango
{

    void md5(uint32 hash[4], Memory memory);
    void sha1(uint32 hash[5], Memory memory);
    void sha2(uint32 hash[8], Memory memory);

} // namespace mango
