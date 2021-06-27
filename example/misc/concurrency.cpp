/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;

namespace
{
    void computeSomethingExpensive(int)
    {
    }

    void computeSomethingExpensive()
    {
    }

    void computeStuff(int)
    {
    }

    void computeOtherStuff(int)
    {
    }

} // namespace

void example1()
{
    // MANGO(tm) ThreadPool implementation never puts work to the pool
    // directly. The work is enqueued using work queues, which keep
    // track of tasks the user has submitted.
    ConcurrentQueue q;

    for (int i = 0; i < 10; ++i)
    {
        // Create tasks for the queue
        q.enqueue([]
        {
            computeSomethingExpensive();
        });
    }

    // The ThreadPool will run the tasks concurrently and they will
    // complete eventually. If we want to block current thread until
    // the queue is drained we can issue this call:
    q.wait();

    // The wait() call is redundant in this example as the queue destructor will
    // have the same effect.
}

void example2()
{
    ConcurrentQueue q;

    for (int i = 0; i < 20; ++i)
    {
        q.enqueue([=]
        {
            computeSomethingExpensive(i);
        });
    }
}

void example3()
{
    // SerialQueue has a special property: each task has automatic
    // dependency to previously issued task. This means that the tasks
    // are executed in well defined order and the results of previous
    // task are always visible to the next task.
    SerialQueue a;
    ConcurrentQueue b;

    for (int i = 0; i < 10; ++i)
    {
        a.enqueue([i]
        {
            computeStuff(i);
        });

        // The tasks in queue b run in complete isolation from queue a.
        // There is no dependency between the two queues. The dependency
        // and serialization is always unique for each queue.

        // Concurrent queues consumes all available pool workers since
        // concurrent tasks don't have any dependencies.

        b.enqueue([i]
        {
            computeOtherStuff(i);
        });
    }
}

void example4()
{
    std::atomic<int> counter { 0 };

    ConcurrentQueue q;

    for (int i = 0; i < 10; ++i)
    {
        ConcurrentQueue x;

        q.enqueue([&]
        {
            // Enqueueuing tasks from tasks works fine and can improve
            // throughput in some situations where single thread cannot keep
            // all of the pool's worker threads busy. This situation is more common
            // when there are a lot of available workers.

            // This is example of really bad workload because it has two serious defects:
            //
            // 1. The work is too trivial compared to the overhead
            // 2. Every worker touches same memory which can saturate highly concurrent system

            x.enqueue([&]
            {
                ++counter; // Oops.. every thread needs to synchronize this memory..
            });
        });
    }
}

void example5()
{
    // Queues can be named and given a priority (LOW, NORMAL, HIGH)

    // The granularity of priorities is not very fine-grained accidentally,
    // the idea is that there is some amount of control but no incentive
    // to go full-retard when scheduling the tasks.

    // The queue names can be useful when debugging and instrumenting
    // the ThreadPool to get more detailed information what is going on.
    ConcurrentQueue queue("important queue", Priority::HIGH);

    // TODO: Enqueue tasks here..
}

/*
    Demonstrate a way to keep reference to shared state which is processed in the
    ThreadPool. The State object is used in the captured lambdas which are executed
    in the pool.
*/
struct State
{
    std::vector<float> data;

    State(size_t size)
        : data(size)
    {
        // initialize the data...
    }

    void process(size_t offset, size_t count)
    {
        assert(offset + count <= data.size());
        // process a range in the data here...
    }
};

void example6()
{
    // Create the shared State in the heap and refer to it using std::shared_ptr
    std::shared_ptr<State> state = std::make_shared<State>(1024);

    ConcurrentQueue q;

    for (size_t i = 0; i < 1024; i += 64)
    {
        // NOTE! State is captued by value so that the reference counting
        // is used to keep track of state life-time. In other words, the last
        // worker thread to complete will "turn off the lights". This way work can be
        // queued in the pool w/o having to synchronize before exiting this scope.
        q.enqueue([state, i]
        {
            state->process(i, 64);
        });
    }

    // q.wait(); <-- won't be required as the std::shared_ptr will keep track of last
    //               shared_ptr to hold reference to the state.

    // ALWAYS KEEP THE LIFE-TIME OF QUEUED OBJECTS IN MIND!!!

    // This techniques allows us to leave the book-keeping to the reference counting
    // in the std::shared_ptr. The downside is, once again, that the object lives in
    // the heap and is dynamically allocated.

    // One nice trick to mention: the State destructor could be used to enqueue
    // more work in the pool. This allows doing processing on the results of the
    // computation w/o requiring a barrier synchronization primitive.
    // In short: have implicit synchronization based on how you feed the work!
}

void example7()
{
    // Can this code deadlock?
    ConcurrentQueue q;
    for (int i = 0; i < 200; ++i)
    {
        q.enqueue([]
        {
            ConcurrentQueue x;
            for (int j = 0; j < 40; ++j)
            {
                x.enqueue([]
                {
                    computeSomethingExpensive();
                });
            }
            x.wait();
        });
    }
    q.wait();

    // At first glance, it seems possible that x.wait() could be running simultaneously
    // as many times as we have available threads in the pool. This would be a problem
    // if all of the workers were sleeping and tasks couldn't get through. We would
    // be deadlocked and our program would hang.

    // It is very easy to paint yourself into the corner like this and generally NEVER
    // sleep or wait for a signal from other tasks in a thread pool.

    // However, our queue wait is implemented as a cooperative-wait so it will help
    // executing tasks in a pool. The unfortunate downside is that this increases latency
    // slightly since the wait might still be currently processing a task not related
    // to the current queue's wait status. The upside is that the pool cannot be
    // deadlocked with wait. Even if task being processed in a wait has a wait in it,
    // that wait will be cooperative as well and we degenerate into recursion but this
    // should be extremely rare, in practise non-existent if the idea to never wait in a
    // task is followed.

    // The takeaway is that if you really, absolutely have to synchronize in a enqueued
    // task then use the queue wait. It's better to have implicit synchronization by
    // NOT issuing any tasks before the data is available (eg. trigger a task when the
    // data is known to be available). Just friendly advice, feel free to ignore and cry.
}

using namespace std::chrono_literals;

void example8()
{
    int counter = 0;
    SpinLock lock;

    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i)
    {
        threads.push_back(std::thread([&counter, &lock]
        {
            for (int i = 0; i < 100; ++i)
            {
                std::this_thread::sleep_for(2us);
                SpinLockGuard guard(lock);
                ++counter;
            }
        }));
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    if (counter != 1000)
    {
        printf("counter: %d\n", counter);
    }
}

int main()
{
    example1();
    example2();
    example3();
    //example4();
    example5();
    example6();
    example7();
    example8();
}
