/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <chrono>
#include <mango/core/system.hpp>
#include <mango/core/thread.hpp>
#include "../../external/concurrentqueue/concurrentqueue.h"
#include "../../external/concurrentqueue/readerwriterqueue.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::milliseconds;

// ------------------------------------------------------------
// thread affinity
// ------------------------------------------------------------

#if defined(MANGO_PLATFORM_LINUX) || defined(MANGO_PLATFORM_BSD)

#include <pthread.h>

    template <typename H>
    static void set_thread_affinity(H handle, int processor)
    {
        cpu_set_t cpuset;

        CPU_ZERO(&cpuset);
        CPU_SET(processor, &cpuset);
        pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);
    }

    static void set_current_thread_affinity(int processor)
    {
        set_thread_affinity(pthread_self(), processor);
    }

#elif defined(MANGO_PLATFORM_WINDOWS)

    template <typename H>
    static void set_thread_affinity(H handle, int processor)
    {
        SetThreadAffinityMask(handle, DWORD_PTR(1) << processor);
    }

    static void set_current_thread_affinity(int processor)
    {
        set_thread_affinity(GetCurrentThread(), processor);
    }

#else

    template <typename H>
    static void set_thread_affinity(H handle, int processor)
    {
        MANGO_UNREFERENCED(handle);
        MANGO_UNREFERENCED(processor);
    }

    static void set_current_thread_affinity(int processor)
    {
        MANGO_UNREFERENCED(processor);
    }

#endif

// MinGW usually uses winpthreads as std::thread backend, so we need
// to convert winpthreads' pthread_t to Win32 HANDLE.
#if defined(__WINPTHREADS_VERSION)

#include <pthread.h>

    static auto get_native_handle(std::thread& t)
    {
        // Note: this function is documented as private.
        return pthread_gethandle(t.native_handle());
    }

#else

    static auto get_native_handle(std::thread& t)
    {
        return t.native_handle();
    }

#endif

namespace mango
{

    // ------------------------------------------------------------
    // ThreadPool
    // ------------------------------------------------------------

    struct ThreadPool::TaskQueue
    {
        using Task = ThreadPool::Task;
        moodycamel::ConcurrentQueue<Task> tasks;
    };

    struct ThreadPool::Consumer
    {
        moodycamel::ConsumerToken token;

        Consumer(TaskQueue& queue)
            : token(queue.tasks)
        {
        }
    };

    ThreadPool::ThreadPool(size_t size)
        : m_queue(nullptr)
        , m_threads(size)
    {
        m_queue = new TaskQueue;

        // NOTE: let OS scheduler shuffle tasks as it sees fit
        //       this gives better performance overall UNTIL we have some practical
        //       use for the affinity (eg. dependent tasks using same cache)
        const bool affinity = false; //std::thread::hardware_concurrency() > 1;
        if (affinity)
        {
            set_current_thread_affinity(0);
        }

        for (size_t i = 0; i < size; ++i)
        {
            m_threads[i] = std::thread([this, i]
            {
                thread(i);
            });

#if defined(MANGO_PLATFORM_WINDOWS)
            if (getHardwareConcurrency() > 64)
            {
                // HACK: work around Windows 64 logical processor per ProcessorGroup limitation
                GROUP_AFFINITY group{};
                group.Mask = KAFFINITY(~0);
                group.Group = WORD(i & 1);

                auto handle = get_native_handle(m_threads[i]);
                SetThreadGroupAffinity(handle, &group, nullptr);
            }
#endif

            if (affinity)
            {
                set_thread_affinity(get_native_handle(m_threads[i]), int(i + 1));
            }
        }
    }

    ThreadPool::~ThreadPool()
    {
        m_stop = true;
        m_condition.notify_all();

        for (auto& thread : m_threads)
        {
            thread.join();
        }

        delete m_queue;
    }

    size_t ThreadPool::getHardwareConcurrency()
    {
        return std::max(std::thread::hardware_concurrency(), 1u);
    }

    ThreadPool& ThreadPool::getInstance()
    {
        return getSystemContext().thread_pool;
    }

    int ThreadPool::size() const
    {
        return int(m_threads.size());
    }

    void ThreadPool::thread(size_t threadID)
    {
        std::string name = fmt::format("TP#{:03}", threadID + 1);
        TraceThread th(name);

        Consumer consumer(*m_queue);

        auto time0 = high_resolution_clock::now();

        while (!m_stop.load(std::memory_order_relaxed))
        {
            Task task;
            if (m_queue->tasks.try_dequeue(consumer.token, task))
            {
                process(task);
                time0 = high_resolution_clock::now();
            }
            else
            {
                auto time1 = high_resolution_clock::now();
                auto elapsed = time1 - time0;
                if (elapsed >= milliseconds(60))
                {
                    std::unique_lock<std::mutex> lock(m_queue_mutex);
                    m_condition.wait_for(lock, milliseconds(60));
                }
                else if (elapsed >= microseconds(24))
                {
                    std::this_thread::yield();
                }
                else
                {
                    pause();
                }
            }
        }
    }

    void ThreadPool::enqueue(Queue* queue, std::function<void()>&& func)
    {
        Task task
        {
            .queue = queue,
            .func = std::move(func)
        };

        ++queue->task_counter;

        m_queue->tasks.enqueue(std::move(task));
        m_condition.notify_one();
    }

    void ThreadPool::enqueue_bulk(Queue* queue, const std::vector<std::function<void()>>& functions)
    {
        size_t count = functions.size();
        queue->task_counter += count;

        std::vector<Task> tasks(count);

        for (size_t i = 0; i < count; ++i)
        {
            tasks[i] =
            {
                .queue = queue,
                .func = std::move(functions[i])
            };
        }

        m_queue->tasks.enqueue_bulk(tasks.data(), count);
        m_condition.notify_all();
    }

    void ThreadPool::process(Task& task) const
    {
        Queue* queue = task.queue;

        // check if the task is cancelled
        if (!queue->cancelled)
        {
            if (queue->name.empty())
            {
                task.func();
            }
            else
            {
                Trace trace("Task", queue->name);
                task.func();
            }
        }

        --queue->task_counter;
    }

    bool ThreadPool::dequeue_and_process()
    {
        Task task;
        if (m_queue->tasks.try_dequeue(task))
        {
            process(task);
            return true;
        }

        return false;
    }

    void ThreadPool::wait(Queue* queue)
    {
        while (queue->task_counter > 0)
        {
            dequeue_and_process();
        }
    }

    void ThreadPool::cancel(Queue* queue)
    {
        queue->cancelled = true;
        wait(queue);
        queue->cancelled = false;
    }

    // ------------------------------------------------------------
    // ConcurrentQueue
    // ------------------------------------------------------------

    ConcurrentQueue::ConcurrentQueue()
        : m_pool(ThreadPool::getInstance())
        , m_queue(&m_pool, "")
    {
        // wake up one thread to be available instantly
        m_pool.m_condition.notify_one();
    }

    ConcurrentQueue::ConcurrentQueue(const std::string& name)
        : m_pool(ThreadPool::getInstance())
        , m_queue(&m_pool, name)
    {
        // wake up one thread to be available instantly
        m_pool.m_condition.notify_one();
    }

    ConcurrentQueue::ConcurrentQueue(ThreadPool& pool)
        : m_pool(pool)
        , m_queue(&m_pool, "")
    {
        // wake up one thread to be available instantly
        m_pool.m_condition.notify_one();
    }

    ConcurrentQueue::ConcurrentQueue(ThreadPool& pool, const std::string& name)
        : m_pool(pool)
        , m_queue(&m_pool, name)
    {
        // wake up one thread to be available instantly
        m_pool.m_condition.notify_one();
    }

    ConcurrentQueue::~ConcurrentQueue()
    {
        wait();
    }

    void ConcurrentQueue::steal()
    {
        m_pool.dequeue_and_process();
    }

    void ConcurrentQueue::cancel()
    {
        m_pool.cancel(&m_queue);
    }

    void ConcurrentQueue::wait()
    {
        m_pool.wait(&m_queue);
    }

    // ------------------------------------------------------------
    // SerialQueue
    // ------------------------------------------------------------

    SerialQueue::SerialQueue()
        : m_name("serial.default")
    {
        m_thread = std::thread([this] {
            thread();
        });
    }

    SerialQueue::SerialQueue(const std::string& name)
        : m_name(name)
    {
        m_thread = std::thread([this] {
            thread();
        });
    }

    SerialQueue::~SerialQueue()
    {
        wait();

        std::unique_lock<std::mutex> queue_lock(m_queue_mutex);
        m_stop = true;
        queue_lock.unlock();

        m_task_condition.notify_one();
        m_thread.join();
    }

    void SerialQueue::thread()
    {
        while (!m_stop.load(std::memory_order_relaxed))
        {
            std::unique_lock<std::mutex> queue_lock(m_queue_mutex);
            if (m_task_counter > 0)
            {
                auto task = std::move(m_task_queue.front());
                m_task_queue.pop_front();
                queue_lock.unlock();

                task();

                std::lock_guard<std::mutex> wait_lock(m_wait_mutex);
                --m_task_counter;
            }
            else
            {
                m_wait_condition.notify_all();
                m_task_condition.wait(queue_lock, [this] { return m_stop || m_task_counter; });
            }
        }
    }

    void SerialQueue::cancel()
    {
        std::lock_guard<std::mutex> queue_lock(m_queue_mutex);
        m_task_counter -= u32(m_task_queue.size());
        m_task_queue.clear();
    }

    void SerialQueue::wait()
    {
        std::unique_lock<std::mutex> wait_lock(m_wait_mutex);
        m_wait_condition.wait(wait_lock, [this] { return !m_task_counter.load(std::memory_order_relaxed); });
    }

    // ------------------------------------------------------------
    // TicketQueue::Ticket
    // ------------------------------------------------------------

    TicketQueue::Ticket::Ticket()
        : task(std::make_shared<Task>())
    {
    }

    TicketQueue::Ticket::~Ticket()
    {
        if (!--task->count)
        {
            if (!task->func)
            {
                task->promise.set_value();
            }
        }
    }

    TicketQueue::Ticket::Ticket(const Ticket& ticket)
    {
        task = ticket.task;
        task->count++;
    }

    const TicketQueue::Ticket& TicketQueue::Ticket::operator = (const Ticket& ticket)
    {
        task = ticket.task;
        task->count++;
        return *this;
    }

    // ------------------------------------------------------------
    // TicketQueue
    // ------------------------------------------------------------

    struct TicketQueue::TaskQueue
    {
        using Task = TicketQueue::SharedTask;
        moodycamel::ReaderWriterQueue<Task> tasks;
    };

    TicketQueue::TicketQueue()
        : m_queue(nullptr)
    {
        m_queue = new TaskQueue();
        m_thread = std::thread([this]
        {
            while (!m_stop.load(std::memory_order_relaxed))
            {
                if (!dequeue_and_process())
                {
                    std::unique_lock<std::mutex> lock(m_consume_mutex);
                    m_consume_condition.wait(lock, [this] { return m_stop || m_queue->tasks.peek(); });
                }
            }
        });
    }

    TicketQueue::~TicketQueue()
    {
        wait();

        std::unique_lock<std::mutex> lock(m_consume_mutex);
        m_stop = true;
        lock.unlock();
        m_consume_condition.notify_one();

        m_thread.join();
        delete m_queue;
    }

    bool TicketQueue::dequeue_and_process()
    {
        bool processed = false;

        SharedTask task;
        if (m_queue->tasks.try_dequeue(task))
        {
            std::future<void> future = task->promise.get_future();
            future.wait();

            if (task->func)
            {
                task->func();
            }

            std::lock_guard<std::mutex> wait_lock(m_wait_mutex);
            if (!--m_ticket_counter)
            {
                m_wait_condition.notify_one();
            }

            processed = true;
        }

        return processed;
    }

    TicketQueue::Ticket TicketQueue::acquire()
    {
        std::lock_guard<std::mutex> wait_lock(m_wait_mutex);
        ++m_ticket_counter;
        Ticket ticket;
        m_queue->tasks.enqueue(ticket.task);
        m_consume_condition.notify_one();
        return ticket;
    }

    std::vector<TicketQueue::Ticket> TicketQueue::acquire(size_t count)
    {
        std::lock_guard<std::mutex> wait_lock(m_wait_mutex);
        m_ticket_counter += count;
        std::vector<TicketQueue::Ticket> tickets(count);

        for (size_t i = 0; i < count; ++i)
        {
            m_queue->tasks.enqueue(tickets[i].task);
        }

        m_consume_condition.notify_one();

        return tickets;
    }

    void TicketQueue::wait()
    {
        std::unique_lock<std::mutex> wait_lock(m_wait_mutex);
        m_wait_condition.wait(wait_lock, [this] { return !m_ticket_counter.load(std::memory_order_relaxed); });
    }

} // namespace mango
