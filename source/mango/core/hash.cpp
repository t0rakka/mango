/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/hash.hpp>

#define XXH_STATIC_LINKING_ONLY
#define XXH_INLINE_ALL
#include "../../external/zstd/common/xxhash.h"

namespace mango
{

    u32 xxhash32(u32 seed, ConstMemory memory)
    {
        return XXH32(memory.address, memory.size, seed);
    }

    u64 xxhash64(u64 seed, ConstMemory memory)
    {
        return XXH64(memory.address, memory.size, seed);
    }

    XX3HASH64 xx3hash64(u64 seed, ConstMemory memory)
    {
        return XXH3_64bits_withSeed(memory.address, memory.size, seed);
    }

    XX3HASH128 xx3hash128(u64 seed, ConstMemory memory)
    {
        const XXH128_hash_t hash = XXH128(memory.address, memory.size, seed);
        return {{ hash.low64, hash.high64 }};
    }

} // namespace mango
