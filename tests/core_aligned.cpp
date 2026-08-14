/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstdint>
#include <cstring>

#include "core_test.hpp"

using namespace mango;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    bool is_aligned(const void* ptr, size_t alignment)
    {
        const auto address = reinterpret_cast<uintptr_t>(ptr);
        return (address & (alignment - 1)) == 0;
    }

    bool test_aligned_malloc_basic()
    {
        void* ptr16 = aligned_malloc(128, 16);
        void* ptr64 = aligned_malloc(256, 64);

        CHECK(ptr16 != nullptr);
        CHECK(ptr64 != nullptr);
        CHECK(is_aligned(ptr16, 16));
        CHECK(is_aligned(ptr64, 64));

        std::memset(ptr16, 0x5a, 128);
        std::memset(ptr64, 0xa5, 256);

        aligned_free(nullptr);
        aligned_free(ptr16);
        aligned_free(ptr64);

        return true;
    }

    bool test_buffer_uses_aligned_storage()
    {
        Buffer buffer(1024, 0);

        CHECK(buffer.data() != nullptr);
        CHECK(is_aligned(buffer.data(), 64));

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            buffer.data()[i] = u8(i & 0xff);
        }

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            CHECK(buffer.data()[i] == u8(i & 0xff));
        }

        return true;
    }

    const Case g_cases [] =
    {
        { "aligned_malloc_basic",   test_aligned_malloc_basic },
        { "buffer_aligned_storage", test_buffer_uses_aligned_storage },
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("core_aligned", g_cases, sizeof(g_cases) / sizeof(g_cases[0]), argc, argv);
}
