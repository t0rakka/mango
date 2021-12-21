/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/adler32.hpp>

#include "../../external/zlib/zlib.h"

namespace mango
{

    u32 adler32(u32 adler, ConstMemory memory)
    {
        return ::adler32(adler, memory.address, memory.size);
    }

    u32 adler32_combine(u32 adler0, u32 adler1, size_t length1)
    {
        return ::adler32_combine(adler0, adler1, length1);
    }

} // namespace mango
