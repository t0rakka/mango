/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/ring_buffer.hpp>
#include <mango/core/print.hpp>
#include <atomic>
#include <thread>
#include <vector>

using namespace mango;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            printLine("  FAILED: {}", #cond); \
            return false; \
        } \
    } while (0)

bool test_basic_fifo()
{
    RingBuffer<int, 8> queue;

    CHECK(queue.capacity() == 8);
    CHECK(queue.empty());
    CHECK(queue.size() == 0);

    int value = -1;
    CHECK(!queue.try_pop(value));
    CHECK(value == -1);

    CHECK(queue.try_push(10));
    CHECK(queue.try_push(20));
    CHECK(!queue.empty());
    CHECK(queue.size() == 2);

    CHECK(queue.try_pop(value) && value == 10);
    CHECK(queue.try_pop(value) && value == 20);
    CHECK(queue.empty());
    CHECK(queue.size() == 0);

    return true;
}

bool test_full_and_empty()
{
    RingBuffer<int, 4> queue;

    CHECK(queue.try_push(1));
    CHECK(queue.try_push(2));
    CHECK(queue.try_push(3));
    CHECK(queue.try_push(4));
    CHECK(queue.size() == 4);
    CHECK(!queue.try_push(5));

    int v = 0;
    CHECK(queue.try_pop(v) && v == 1);
    CHECK(queue.try_push(5));
    CHECK(!queue.try_push(6));

    CHECK(queue.try_pop(v) && v == 2);
    CHECK(queue.try_pop(v) && v == 3);
    CHECK(queue.try_pop(v) && v == 4);
    CHECK(queue.try_pop(v) && v == 5);
    CHECK(queue.empty());
    CHECK(!queue.try_pop(v));

    return true;
}

bool test_wrap_single_thread()
{
    RingBuffer<int, 4> queue;
    constexpr int iterations = 10000;

    for (int i = 0; i < iterations; ++i)
    {
        CHECK(queue.try_push(i));
        int v = -1;
        CHECK(queue.try_pop(v));
        CHECK(v == i);
    }

    CHECK(queue.empty());

    return true;
}

bool test_try_emplace()
{
    RingBuffer<std::pair<int, int>, 4> queue;

    CHECK(queue.try_emplace(3, 4));
    std::pair<int, int> out { 0, 0 };
    CHECK(queue.try_pop(out));
    CHECK(out.first == 3);
    CHECK(out.second == 4);

    return true;
}

bool test_move_push()
{
    RingBuffer<std::string, 4> queue;

    std::string a = "hello";
    CHECK(queue.try_push(std::move(a)));
    CHECK(a.empty());

    std::string b;
    CHECK(queue.try_pop(b));
    CHECK(b == "hello");

    return true;
}

bool test_spsc_stress()
{
    constexpr std::size_t capacity = 256;
    constexpr int message_count = 500000;

    RingBuffer<int, capacity> queue;

    std::atomic<bool> producer_failed { false };
    std::atomic<bool> consumer_failed { false };

    std::thread producer([&]
    {
        for (int i = 0; i < message_count; ++i)
        {
            while (!queue.try_push(i))
            {
                // spin; SPSC backpressure
            }
        }
    });

    std::thread consumer([&]
    {
        int expected = 0;
        while (expected < message_count)
        {
            int value = -1;
            if (!queue.try_pop(value))
            {
                continue;
            }

            if (value != expected)
            {
                printLine("  order error: expected {}, got {}", expected, value);
                consumer_failed = true;
                return;
            }

            ++expected;
        }
    });

    producer.join();
    consumer.join();

    CHECK(!producer_failed);
    CHECK(!consumer_failed);
    CHECK(queue.empty());

    return true;
}

bool test_spsc_burst()
{
    RingBuffer<int, 16> queue;

    std::atomic<bool> failed { false };

    std::thread producer([&]
    {
        for (int round = 0; round < 2000; ++round)
        {
            const int burst = 12;
            for (int i = 0; i < burst; ++i)
            {
                while (!queue.try_push(round * 100 + i))
                {
                }
            }
        }
    });

    std::thread consumer([&]
    {
        int expected_round = 0;
        int expected_in_round = 0;

        while (expected_round < 2000)
        {
            int value = -1;
            if (!queue.try_pop(value))
            {
                continue;
            }

            const int round = value / 100;
            const int index = value % 100;

            if (round != expected_round || index != expected_in_round)
            {
                printLine("  burst error: got {}/{}, expected {}/{}",
                    round, index, expected_round, expected_in_round);
                failed = true;
                return;
            }

            ++expected_in_round;
            if (expected_in_round == 12)
            {
                expected_in_round = 0;
                ++expected_round;
            }
        }
    });

    producer.join();
    consumer.join();

    CHECK(!failed);
    CHECK(queue.empty());

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
        { "basic_fifo        ", test_basic_fifo },
        { "full_and_empty    ", test_full_and_empty },
        { "wrap_single_thread", test_wrap_single_thread },
        { "try_emplace       ", test_try_emplace },
        { "move_push         ", test_move_push },
        { "spsc_stress       ", test_spsc_stress },
        { "spsc_burst        ", test_spsc_burst },
    };

    int passed = 0;

    for (const auto& test : tests)
    {
        printLine("------------------------------------------------------------");
        printLine(" {}", test.name);
        printLine("------------------------------------------------------------");

        if (!test.func())
        {
            printLine("Failed: {}", test.name);
            return 1;
        }

        ++passed;
        printLine(" OK");
    }

    printLine("------------------------------------------------------------");
    printLine(" All {} test(s) passed.", passed);
    printLine("------------------------------------------------------------");

    return 0;
}
