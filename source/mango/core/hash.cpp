/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/hash.hpp>
#include "../../external/zstd/common/xxhash.h"

namespace mango {

    u32 xxhash32(Memory memory)
    {
        u32 seed = 0;
        return XXH32(memory.address, memory.size, seed);
    }

    u64 xxhash64(Memory memory)
    {
        u64 seed = 0;
        return XXH64(memory.address, memory.size, seed);
    }

} // namespace mango
