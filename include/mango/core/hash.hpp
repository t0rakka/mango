/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"
#include "memory.hpp"

namespace mango
{

    void md5(u32 hash[4], Memory memory);
    void sha1(u32 hash[5], Memory memory);
    void sha2(u32 hash[8], Memory memory);

    u32 xxhash32(Memory memory);
    u64 xxhash64(Memory memory);

} // namespace mango
