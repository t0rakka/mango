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

    using MD5 = Hash<u32, 4>;
    using SHA1 = Hash<u32, 5>;
    using SHA2 = Hash<u32, 8>;

    // -----------------------------------------------------------------------
    // hashing functions
    // -----------------------------------------------------------------------

    MD5 md5(Memory memory);
    SHA1 sha1(Memory memory);
    SHA2 sha2(Memory memory);

    u32 xxhash32(u32 seed, Memory memory);
    u64 xxhash64(u64 seed, Memory memory);

} // namespace mango
