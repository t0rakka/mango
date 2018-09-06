/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/hash.hpp>
#include "../../external/zstd/common/xxhash.h"

namespace mango {

    uint32 xxhash32(Memory memory)
    {
        uint32 seed = 0;
        return XXH32(memory.address, memory.size, seed);
    }

    uint64 xxhash64(Memory memory)
    {
        uint64 seed = 0;
        return XXH64(memory.address, memory.size, seed);
    }

} // namespace mango
