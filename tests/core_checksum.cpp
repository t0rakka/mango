/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "core_test.hpp"

#include <cstring>

using namespace mango;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    u8 pattern_byte(size_t index)
    {
        return u8((index * 3 + 1) & 0xff);
    }

    bool test_crc32_lengths_alignment_and_split()
    {
        alignas(64) u8 storage[4200] = {};

        for (size_t i = 0; i < std::size(storage); ++i)
        {
            storage[i] = pattern_byte(i);
        }

        const u8 reference[] = "123456789";
        CHECK(crc32(0, ConstMemory(reference, 9)) == 0xcbf43926);

        const size_t lengths[] = { 0, 1, 7, 8, 63, 64, 65, 4096 };

        for (size_t length : lengths)
        {
            for (size_t i = 0; i < length; ++i)
            {
                storage[i] = pattern_byte(i);
            }

            for (size_t i = 0; i < length; ++i)
            {
                storage[i + 1] = storage[i];
            }

            ConstMemory aligned(storage, length);
            ConstMemory unaligned(storage + 1, length);

            const u32 once = crc32(0, aligned);
            CHECK(crc32(0, unaligned) == once);

            const size_t split_at = length / 2;
            const u32 split = crc32(
                crc32(0, ConstMemory(storage, split_at)),
                ConstMemory(storage + split_at, length - split_at));

            CHECK(split == once);
        }

        return true;
    }

    bool test_crc32c_lengths_alignment_and_split()
    {
        alignas(64) u8 storage[4200] = {};

        for (size_t i = 0; i < std::size(storage); ++i)
        {
            storage[i] = pattern_byte(i + 17);
        }

        const u8 zeros[32] = {};
        const u8 ones[32] =
        {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        };

        CHECK(crc32c(0, ConstMemory(zeros, 32)) == 0x8a9136aa);
        CHECK(crc32c(0, ConstMemory(ones, 32)) == 0x62a8ab43);

        const size_t lengths[] = { 0, 1, 7, 8, 63, 64, 65, 4096 };

        for (size_t length : lengths)
        {
            for (size_t i = 0; i < length; ++i)
            {
                storage[i] = pattern_byte(i + 17);
            }

            for (size_t i = 0; i < length; ++i)
            {
                storage[i + 1] = storage[i];
            }

            ConstMemory aligned(storage, length);
            ConstMemory unaligned(storage + 1, length);

            const u32 once = crc32c(0, aligned);
            CHECK(crc32c(0, unaligned) == once);

            const size_t split_at = length / 2;
            const u32 split = crc32c(
                crc32c(0, ConstMemory(storage, split_at)),
                ConstMemory(storage + split_at, length - split_at));

            CHECK(split == once);
        }

        return true;
    }

    bool test_adler32_lengths_alignment_and_split()
    {
        alignas(64) u8 storage[4200] = {};

        for (size_t i = 0; i < std::size(storage); ++i)
        {
            storage[i] = pattern_byte(i + 31);
        }

        const u8 vec0[] =
        {
            0, 1, 2, 3, 4, 5, 6, 7,
            7, 6, 5, 4, 3, 2, 1, 0,
            0, 1, 2, 3, 4, 5, 6, 7,
            7, 6, 5, 4, 3, 2, 1, 0,
        };

        CHECK(adler32(1, ConstMemory(vec0, 32)) == 0x07580071);

        const size_t lengths[] = { 0, 1, 7, 8, 63, 64, 65, 4096 };

        for (size_t length : lengths)
        {
            for (size_t i = 0; i < length; ++i)
            {
                storage[i] = pattern_byte(i + 31);
            }

            for (size_t i = 0; i < length; ++i)
            {
                storage[i + 1] = storage[i];
            }

            ConstMemory aligned(storage, length);
            ConstMemory unaligned(storage + 1, length);

            const u32 once = adler32(1, aligned);
            CHECK(adler32(1, unaligned) == once);

            const size_t split_at = length / 2;
            const u32 split = adler32(
                adler32(1, ConstMemory(storage, split_at)),
                ConstMemory(storage + split_at, length - split_at));

            CHECK(split == once);
        }

        return true;
    }

    const Case g_cases [] =
    {
        { "crc32 lengths alignment split", test_crc32_lengths_alignment_and_split },
        { "crc32c lengths alignment split", test_crc32c_lengths_alignment_and_split },
        { "adler32 lengths alignment split", test_adler32_lengths_alignment_and_split },
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("core_checksum", g_cases, std::size(g_cases), argc, argv);
}
