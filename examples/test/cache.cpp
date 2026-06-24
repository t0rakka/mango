/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            printLine("  FAILED: {}", #cond); \
            return false; \
        } \
    } while (0)

bool test_lru_basic()
{
    LRUCache<int, int> cache(3);

    cache.insert(1, 10);
    cache.insert(2, 20);
    cache.insert(3, 30);

    CHECK(cache.size() == 3);
    CHECK(cache.get(1) == 10);
    CHECK(cache.get(2) == 20);
    CHECK(cache.get(3) == 30);

    cache.insert(4, 40);

    CHECK(cache.size() == 3);
    CHECK(!cache.get(1));
    CHECK(cache.get(4) == 40);

    cache.insert(2, 200);
    CHECK(cache.get(2) == 200);

    cache.erase(3);
    CHECK(cache.size() == 2);
    CHECK(!cache.get(3));

    cache.clear();
    CHECK(cache.size() == 0);

    return true;
}

bool test_lru_capacity_guard()
{
    bool rejected = false;

    try
    {
        LRUCache<int, int> cache(0);
        (void)cache;
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }

    CHECK(rejected);

    return true;
}

bool test_arc_basic()
{
    ARCCache<int, int> cache(3);

    cache.insert(1, 10);
    cache.insert(2, 20);
    cache.insert(3, 30);

    CHECK(cache.size() == 3);
    CHECK(cache.get(1) == 10);
    CHECK(cache.get(2) == 20);

    cache.insert(4, 40);

    CHECK(cache.size() == 3);
    CHECK(cache.get(4) == 40);

    cache.insert(2, 200);
    CHECK(cache.get(2) == 200);

    cache.erase(2);
    CHECK(!cache.get(2));

    cache.clear();
    CHECK(cache.size() == 0);

    return true;
}

bool test_twoq_promotion()
{
    TwoQCache<int, int> cache(4);

    cache.insert(1, 10);
    CHECK(cache.get(1) == 10);
    CHECK(cache.size() == 1);

    cache.insert(1, 100);
    CHECK(cache.get(1) == 100);

    cache.insert(2, 20);
    CHECK(cache.get(2) == 20);
    cache.insert(3, 30);
    CHECK(cache.get(3) == 30);
    cache.insert(4, 40);
    CHECK(cache.get(4) == 40);

    CHECK(cache.size() == 4);

    cache.insert(5, 50);
    CHECK(cache.size() == 4);
    CHECK(cache.get(5) == 50);
    CHECK(!cache.get(1));

    return true;
}

bool test_cost_lru_budget()
{
    CostLRUCache<int, int, int> cache(10);

    cache.insert(1, 10, 4);
    cache.insert(2, 20, 4);
    cache.insert(3, 30, 4);

    CHECK(cache.size() == 2);
    CHECK(cache.used() == 8);
    CHECK(!cache.get(1));
    CHECK(cache.get(2) == 20);
    CHECK(cache.get(3) == 30);

    cache.get(2);
    cache.insert(4, 40, 6);

    CHECK(cache.size() == 2);
    CHECK(!cache.get(3));
    CHECK(cache.get(2) == 20);
    CHECK(cache.get(4) == 40);
    CHECK(cache.used() == 10);

    return true;
}

bool test_cost_lru_zero_cost()
{
    CostLRUCache<int, int, int> cache(5);

    cache.insert(1, 10, 0);
    cache.insert(2, 20, 0);

    CHECK(cache.size() == 2);
    CHECK(cache.used() == 2);

    cache.insert(3, 30, 0);
    cache.insert(4, 40, 0);
    cache.insert(5, 50, 0);

    CHECK(cache.size() == 5);
    CHECK(cache.used() == 5);

    return true;
}

bool test_cost_lru_oversized_entry()
{
    CostLRUCache<int, int, int> cache(10);

    cache.insert(1, 100, 100);

    CHECK(cache.size() == 1);
    CHECK(cache.used() == 100);
    CHECK(cache.get(1) == 100);

    cache.insert(2, 20, 5);

    CHECK(cache.size() == 1);
    CHECK(!cache.get(1));
    CHECK(cache.get(2) == 20);
    CHECK(cache.used() == 5);

    return true;
}

bool test_cost_lru_update_cost()
{
    CostLRUCache<int, int, int> cache(10);

    cache.insert(1, 10, 3);
    cache.insert(2, 20, 3);
    cache.insert(1, 11, 8);

    CHECK(cache.size() == 1);
    CHECK(cache.get(1) == 11);
    CHECK(cache.used() == 8);
    CHECK(!cache.get(2));

    return true;
}

int main()
{
    struct Test
    {
        const char* name;
        bool (*func)();
    };

    Test tests [] =
    {
        { "lru_basic",              test_lru_basic },
        { "lru_capacity_guard",     test_lru_capacity_guard },
        { "arc_basic",              test_arc_basic },
        { "twoq_promotion",         test_twoq_promotion },
        { "cost_lru_budget",        test_cost_lru_budget },
        { "cost_lru_zero_cost",     test_cost_lru_zero_cost },
        { "cost_lru_oversized",     test_cost_lru_oversized_entry },
        { "cost_lru_update_cost",   test_cost_lru_update_cost },
    };

    int passed = 0;

    for (const auto& test : tests)
    {
        printLine("------------------------------------------------------------");
        printLine(" {}", test.name);
        printLine("------------------------------------------------------------");

        if (!test.func())
        {
            printLine("");
            printLine("FAILED: {}", test.name);
            return 1;
        }

        printLine("  OK");
        printLine("");
        ++passed;
    }

    printLine("============================================================");
    printLine(" All {} test(s) passed.", passed);
    printLine("============================================================");

    return 0;
}
