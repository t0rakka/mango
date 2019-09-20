/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"
#include "memory.hpp"

namespace mango
{

    // -----------------------------------------------------------------------
    // Hash - generic hashing function return type
    // -----------------------------------------------------------------------

    template <typename T, int S>
    struct Hash
    {
        T data[S];

        T operator [] (int index) const
        {
            return data[index];
        }
    };

    // operators

    template <typename T, int S>
    bool operator == (const Hash<T, S>& a, const Hash<T, S>& b)
    {
        return std::memcmp(a.data, b.data, sizeof(Hash<T, S>)) == 0;
    }

    template <typename T, int S>
    bool operator != (const Hash<T, S>& a, const Hash<T, S>& b)
    {
        return std::memcmp(a.data, b.data, sizeof(Hash<T, S>)) != 0;
    }

    template <typename T, int S>
    bool operator < (const Hash<T, S>& a, const Hash<T, S>& b)
    {
        return std::memcmp(a.data, b.data, sizeof(Hash<T, S>)) < 0;
    }

    template <typename T, int S>
    bool operator > (const Hash<T, S>& a, const Hash<T, S>& b)
    {
        return std::memcmp(a.data, b.data, sizeof(Hash<T, S>)) > 0;
    }

    // -----------------------------------------------------------------------
    // hashing functions
    // -----------------------------------------------------------------------

    using MD5 = Hash<u32, 4>;
    using SHA1 = Hash<u32, 5>;
    using SHA2 = Hash<u32, 8>;
    using XX3HASH64 = u64;
    using XX3HASH128 = Hash<u64, 2>;

    MD5 md5(ConstMemory memory);
    SHA1 sha1(ConstMemory memory);
    SHA2 sha2(ConstMemory memory);

    u32 xxhash32(u32 seed, ConstMemory memory);
    u64 xxhash64(u64 seed, ConstMemory memory);

    // WARNING!
    // The experimental SIMD enhanced hashing functions are NON-STABLE; the hash values might
    // change when the xx3-hash is upgraded. This notice will be removed after the API is stable.

    XX3HASH64 xx3hash64(u64 seed, ConstMemory memory);
    XX3HASH128 xx3hash128(u64 seed, ConstMemory memory);

} // namespace mango
