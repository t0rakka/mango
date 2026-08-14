/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "core_test.hpp"

using namespace mango;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    bool test_memory_slice_basic()
    {
        u8 storage[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
        ConstMemory memory(storage, sizeof(storage));

        ConstMemory head = memory.slice(0, 3);
        CHECK(head.address == storage);
        CHECK(head.size == 3);

        ConstMemory tail = memory.slice(5);
        CHECK(tail.address == storage + 5);
        CHECK(tail.size == 3);

        ConstMemory middle = memory.slice(2, 2);
        CHECK(middle.address == storage + 2);
        CHECK(middle.size == 2);
        CHECK(middle.address[0] == 2);
        CHECK(middle.address[1] == 3);

        return true;
    }

    bool test_memory_slice_edge_cases()
    {
        u8 storage[] = { 10, 20, 30 };
        ConstMemory memory(storage, sizeof(storage));

        ConstMemory empty_offset = memory.slice(4);
        CHECK(empty_offset.address == nullptr);
        CHECK(empty_offset.size == 0);

        ConstMemory clamped = memory.slice(1, 99);
        CHECK(clamped.address == storage + 1);
        CHECK(clamped.size == 2);

        ConstMemory zero_length = memory.slice(1, 0);
        CHECK(zero_length.address == storage + 1);
        CHECK(zero_length.size == 2);

        ConstMemory null_memory;
        ConstMemory from_null = null_memory.slice(0, 4);
        CHECK(from_null.address == nullptr);
        CHECK(from_null.size == 0);

        return true;
    }

    const Case g_cases [] =
    {
        { "slice_basic",      test_memory_slice_basic },
        { "slice_edge_cases", test_memory_slice_edge_cases },
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("core_slice", g_cases, sizeof(g_cases) / sizeof(g_cases[0]), argc, argv);
}
