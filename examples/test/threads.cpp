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

    for (size_t i = 0; i < count; ++i)
    {
        q.enqueue([]
        {
            Sleep::ms(1);
        });
    }

    return true;
}

int main(int argc, char* argv[])
{
    int count = 1;
    if (argc == 2)
    {
        count = std::atoi(argv[1]);
    }

    using Function = bool (*)(void);

    Function tests [] =
    {
        test0,
        test1,
        test2,
        test3,
        test4,
        test5,
        test6,
        test7,
    };

    for (int i = 0; i < count; ++i)
    {
        int index = 0;
        for (auto func : tests)
        {
            printf("------------------------------------------------------------\n");
            printf(" test%d\n", index++);
            printf("------------------------------------------------------------\n");
            printf("\n");

            u64 time0 = Time::ms();
            bool success = func();
            printf("\n <<<< complete: %d ms \n\n", int(Time::ms() - time0));

            if (!success)
            {
                printf("Failed.\n");
                return 0;
            }
        }
    }
}
