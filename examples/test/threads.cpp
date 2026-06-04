/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <mango/mango.hpp>

using namespace mango;

static inline
void print(const char* text)
{
    printf("%s", text);
    fflush(stdout);
}

struct Counter
{
    // The counters should have their own cache line to avoid false sharing
    alignas(64)
    std::atomic<int> value = { 0 };
};

bool test0()
{
    // NOTE: This test chokes with CPUs with large number of cores (n > 20)
    //       UNLESS we feed the work in a worker thread ; without the inner
    //       enqueue() call the test takes 10x longer because enqueue is a
    //       bottleneck. This test is in place to monitor the improvements
    //       on the front-end.

    constexpr int N = 10;

    constexpr int size = 32;
    constexpr u64 icount = 7'500'000 / N;

    Counter counter[size];

    if (icount > 0)
    {
        ConcurrentQueue q;

        for (u64 i = 0; i < icount; ++i)
        {
            q.enqueue([&, i]
            {
                // a task that enqueues N tasks
                for (int j = 0; j < N; ++j)
                {
                    const int idx = (i * N + j) % size;
                    q.enqueue([&, idx]
                    {
                        ++counter[idx].value;
                    });
                }
            });
        }
    }

    u64 sum = 0;
    for (int i = 0; i < size; ++i)
    {
        sum += counter[i].value.load();
    }

    return sum == icount * N;
}

bool test1()
{
    ConcurrentQueue q;
    SerialQueue a;
    SerialQueue b;

    std::atomic<int> counter { 0 };

    constexpr u64 icount = 1000;

    for (u64 i = 0; i < icount; ++i)
    {
        q.enqueue([&]
        {
            Sleep::us(100);
            print(".");

            a.enqueue([&]
            {
                ++counter;
                Sleep::us(7);
                print("1");
            });

            b.enqueue([&]
            {
                ++counter;
                Sleep::us(2);
                print("2");
            });

            if (i % 8 == 0)
            {
                a.wait();
                b.wait();
            }
        });
    }

    print("x");

    q.wait(); // producer must be synchronized first
    print("Q");

    a.wait();
    print("A");

    b.wait();
    print("B");

    bool success = counter == icount * 2;

    printf("\n\n");
    printf("counter: %d [%s]\n", counter.load(), success ? "Success" : "FAILED");

    return success;
}

bool test2()
{
    ConcurrentQueue a;
    ConcurrentQueue b;

    u64 time0 = Time::ms();

    std::atomic<int> counter { 0 };

    constexpr u64 icount = 1000;
    constexpr u64 jcount = 1000;

    for (u64 i = 0; i < icount; ++i)
    {
        a.enqueue([&]
        {
            for (u64 j = 0; j < jcount; ++j)
            {
                b.enqueue([&]
                {
                    ++counter;
                });
            }
        });
    }

    u64 time1 = Time::ms();

    printf("enqueue counter: %d\n", counter.load());

    a.wait();
    b.wait();

    u64 time2 = Time::ms();

    bool success = counter == icount * jcount;

    printf("counter: %d [%s]\n", counter.load(), success ? "Success" : "FAILED");
    printf("enqueue: %d ms, execute: %d ms\n", int(time1 - time0), int(time2 - time1));

    return success;
}

bool test3()
{
    SerialQueue q;

    u64 time0 = Time::ms();

    std::atomic<int> counter { 0 };

    constexpr u64 icount = 1000;
    constexpr u64 jcount = 1000;

    for (u64 i = 0; i < icount; ++i)
    {
        //print(".");

        q.enqueue([&]
        {
            for (u64 j = 0; j < jcount; ++j)
            {
                //print("+");

                q.enqueue([&]
                {
                    ++counter;
                });
            }
        });
    }

    //q.cancel();

    u64 time1 = Time::ms();

    printf("enqueue counter: %d\n", counter.load());

    q.wait();

    u64 time2 = Time::ms();

    bool success = counter == icount * jcount;

    printf("counter: %d [%s]\n", counter.load(), success ? "Success" : "FAILED");
    printf("enqueue: %d ms, execute: %d ms\n", int(time1 - time0), int(time2 - time1));

    return success;
}

bool test4()
{
    ConcurrentQueue cq;
    SerialQueue sq;

    std::atomic<int> counter { 0 };
    std::atomic<bool> running { false };

    std::atomic<int> overlaps { 0 };

    std::vector<int> result;

    for (u64 i = 0; i < 10; ++i)
    {
        cq.enqueue([&]
        {
            for (int j = 0; j < 10; ++j)
            {
                sq.enqueue([&]
                {
                    if (running.load())
                    {
                        overlaps ++;
                    }

                    running = true;
                    ++counter;

                    //std::lock_guard<std::mutex> lock(mutex);
                    result.push_back(counter.load() - 1);

                    running = false;
                });
            }
        });
    }

    cq.wait();
    sq.wait();

    bool success = true;
    int errors = 0;

    for (size_t i = 0; i < result.size(); ++i)
    {
        if (i != size_t(result[i]))
            errors ++;
    }
    printf("Sequential errors: %d\n", errors);
    printf("Overlapping tasks: %d\n", overlaps.load());

    if (errors > 0 || overlaps.load() > 0)
    {
        printf("Status: ERROR \n");
        success = false;
    }

    return success;
}

bool test5()
{
    std::atomic<int> counter { 0 };

    constexpr u64 icount = 1'000'000 / 10;
    constexpr u64 jcount = 10;

    ConcurrentQueue a;
    ConcurrentQueue b;

    for (u64 i = 0; i < icount; ++i)
    {
        a.enqueue([&]
        {
            for (u64 j = 0; j < jcount; ++j)
            {
                b.enqueue([&]
                {
                    ++counter;
                });
            }
        });
    }

    a.wait();
    b.wait();

    bool success = counter == icount * jcount;
    printf("counter: %d [%s]\n", counter.load(), success ? "Success" : "FAILED");
    return success;
}

bool test6()
{
    std::atomic<int> acquire_counter { 0 };
    std::atomic<int> consume_counter { 0 };

    ConcurrentQueue q;
    TicketQueue tk;

    constexpr u64 icount = 1'000'000 / 1;

    for (u64 i = 0; i < icount; ++i)
    {
        auto ticket = tk.acquire();
        ++acquire_counter;

        q.enqueue([ticket, &consume_counter]
        {
            ticket.consume([&consume_counter]
            {
                ++consume_counter;
            });
        });
    }

    q.wait();
    tk.wait();

    int acquired = acquire_counter.load();
    int consumed = consume_counter.load();

    printf("  acquire: %d\n", acquired);
    printf("  consume: %d\n", consumed);

    bool success = (acquired == icount) && (consumed == icount);
    return success;
}

bool test7()
{
    ConcurrentQueue q;

    // This test should take approximately one second
    const size_t count = ThreadPool::getHardwareConcurrency() * 1000;

    u64 time0 = Time::ms();

    for (size_t i = 0; i < count; ++i)
    {
        q.enqueue([]
        {
            Sleep::ms(1);
        });
    }

    q.wait();

    u64 elapsed = Time::ms() - time0;

    printf("  tasks: %zu (concurrency × 1000)\n", count);
    printf("  elapsed: %d ms (expect ~1000 ms)\n", int(elapsed));

    return true;
}

bool test8()
{
    // Nested ConcurrentQueue — pinned local drain on the pool worker

    std::atomic<int> counter { 0 };

    ConcurrentQueue q;
    q.enqueue([&]
    {
        ConcurrentQueue nested;

        for (int i = 0; i < 8; ++i)
        {
            nested.enqueue([&]
            {
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        }

        nested.wait();
    });
    q.wait();

    const bool success = counter.load() == 8;

    printf("  nested tasks completed: %d / 8 [%s]\n", counter.load(), success ? "Success" : "FAILED");

    return success;
}

bool test9()
{
    // Nested queue.wait() from pool workers — must not deadlock (hdecompress failure mode)

    const size_t concurrency = ThreadPool::getHardwareConcurrency();
    const size_t tasks = std::max(size_t(4), concurrency * 2);
    const size_t subtasks = 8;

    ConcurrentQueue q;
    std::atomic<size_t> counter { 0 };

    u64 time0 = Time::ms();

    for (size_t i = 0; i < tasks; ++i)
    {
        q.enqueue([&]
        {
            ConcurrentQueue nested;

            for (size_t j = 0; j < subtasks; ++j)
            {
                nested.enqueue([&]
                {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
            }

            nested.wait();
        });
    }

    q.wait();

    u64 elapsed = std::max(u64(1), Time::ms() - time0);

    const size_t expected = tasks * subtasks;
    const size_t got = counter.load();
    const bool success = got == expected;

    printf("  workers: %zu × %zu nested wait() calls\n", tasks, subtasks);
    printf("  counter: %zu / %zu [%s]\n", got, expected, success ? "Success" : "FAILED");
    printf("  elapsed: %d ms\n", int(elapsed));

    return success;
}

bool test10()
{
    // crc32c throughput: parallel on main vs nested queue drain on pool workers

    constexpr size_t MB = 1024 * 1024;
    constexpr size_t size = 64 * MB;

    Buffer buffer(size);
    std::memset(buffer.data(), 0xa5, size);

    ConstMemory memory = buffer;

    u64 main_time0 = Time::ms();
    const u32 crc_main = crc32c(0, memory);
    u64 main_ms = std::max(u64(1), Time::ms() - main_time0);

    std::atomic<u32> crc_worker { 0 };
    u64 worker_ms { 0 };

    ConcurrentQueue q;
    q.enqueue([&]
    {
        u64 time0 = Time::ms();
        crc_worker = crc32c(0, memory);
        worker_ms = std::max(u64(1), Time::ms() - time0);
    });
    q.wait();

    const bool success = crc_main == crc_worker.load();
    const double mb = size / double(MB);

    printf("  buffer: %.0f MB\n", mb);
    printf("  main:   crc=%08x  %.0f MB/s  (%d ms)  [parallel]\n",
        crc_main, mb * 1000.0 / main_ms, int(main_ms));
    printf("  worker: crc=%08x  %.0f MB/s  (%d ms)  [nested queue drain]\n",
        crc_worker.load(), mb * 1000.0 / worker_ms, int(worker_ms));
    printf("  crc match: %s\n", success ? "yes" : "NO");

    return success;
}

bool test11()
{
    // hdecompress-like: bounded in-flight + crc32c inside pool workers

    const size_t concurrency = ThreadPool::getHardwareConcurrency();
    const size_t max_in_flight = std::max(size_t(2), concurrency * 2);
    const size_t block_count = std::max(size_t(64), concurrency * 8);
    constexpr size_t block_size = 512 * 1024;

    Buffer block(block_size);
    std::memset(block.data(), 0x5a, block_size);
    ConstMemory memory = block;

    ConcurrentQueue q;
    std::atomic<size_t> schedule_pos { 0 };
    std::atomic<size_t> blocks_in_flight { 0 };
    std::atomic<size_t> blocks_done { 0 };
    std::atomic<u32> crc_accum { 0 };

    auto pump = [&]()
    {
        for (;;)
        {
            size_t in_flight = blocks_in_flight.load(std::memory_order_relaxed);
            if (in_flight >= max_in_flight)
            {
                break;
            }

            size_t pos = schedule_pos.load(std::memory_order_relaxed);
            if (pos >= block_count)
            {
                break;
            }

            if (!blocks_in_flight.compare_exchange_weak(in_flight, in_flight + 1,
                    std::memory_order_acq_rel, std::memory_order_relaxed))
            {
                continue;
            }

            if (!schedule_pos.compare_exchange_weak(pos, pos + 1,
                    std::memory_order_acq_rel, std::memory_order_relaxed))
            {
                blocks_in_flight.fetch_sub(1, std::memory_order_relaxed);
                continue;
            }

            q.enqueue([&]
            {
                u32 crc = crc32c(0, memory);
                crc_accum.fetch_xor(crc, std::memory_order_relaxed);
                blocks_done.fetch_add(1, std::memory_order_relaxed);
                blocks_in_flight.fetch_sub(1, std::memory_order_relaxed);
            });
        }
    };

    u64 time0 = Time::ms();

    pump();

    while (blocks_done.load(std::memory_order_relaxed) < block_count)
    {
        pump();
        q.steal();
    }

    q.wait();

    u64 elapsed = std::max(u64(1), Time::ms() - time0);
    const size_t done = blocks_done.load();
    const bool success = done == block_count;
    const double mb = double(block_count * block_size) / (1024.0 * 1024.0);

    printf("  blocks: %zu × %.0f KB, in-flight cap: %zu\n",
        block_count, block_size / 1024.0, max_in_flight);
    printf("  done: %zu / %zu [%s]\n", done, block_count, success ? "Success" : "FAILED");
    printf("  throughput: %.0f MB/s  (%d ms)\n", mb * 1000.0 / elapsed, int(elapsed));
    printf("  crc xor: %08x\n", crc_accum.load());

    return success;
}

int main(int argc, char* argv[])
{
    int count = 1;
    if (argc == 2)
    {
        count = std::atoi(argv[1]);
    }

    using Function = bool (*)(void);

    struct Test
    {
        const char* name;
        const char* description;
        Function func;
    };

    Test tests [] =
    {
        { "test0",  "enqueue fan-out (front-end throughput)",           test0 },
        { "test1",  "ConcurrentQueue + SerialQueue interleave",        test1 },
        { "test2",  "nested ConcurrentQueue enqueue timing",          test2 },
        { "test3",  "nested SerialQueue enqueue timing",              test3 },
        { "test4",  "SerialQueue ordering (no overlap)",              test4 },
        { "test5",  "nested ConcurrentQueue counter stress",           test5 },
        { "test6",  "TicketQueue ordering",                             test6 },
        { "test7",  "sleep task soak (~1 s)",                           test7 },
        { "test8",  "nested ConcurrentQueue local drain",             test8 },
        { "test9",  "nested wait() from workers (deadlock check)",    test9 },
        { "test10", "crc32c main (parallel) vs worker (nested CQ)", test10 },
        { "test11", "bounded in-flight + worker crc (hdecompress-like)", test11 },
    };

    int passed = 0;

    for (int i = 0; i < count; ++i)
    {
        for (const auto& test : tests)
        {
            printf("------------------------------------------------------------\n");
            printf(" %s\n", test.name);
            printf(" %s\n", test.description);
            printf("------------------------------------------------------------\n");
            printf("\n");

            u64 time0 = Time::ms();
            bool success = test.func();
            printf("\n <<<< complete: %d ms \n\n", int(Time::ms() - time0));

            if (!success)
            {
                printf("Failed.\n");
                return 1;
            }

            ++passed;
        }
    }

    printf("============================================================\n");
    printf(" All %d test(s) passed.\n", passed);
    printf("============================================================\n");

    return 0;
}
