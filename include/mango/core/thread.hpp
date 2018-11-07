/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <queue>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <future>
#include "exception.hpp"
#include "object.hpp"
#include "atomic.hpp"

namespace mango
{

    // TODO: use lock-free MPMC queue for free objects and only lock
    //       when running out of objects im the queue
    template <typename T>
    class ObjectCache
    {
    protected:
        int m_block_size;
        std::vector<T*> m_blocks;
        T** m_stack { nullptr };
        int m_stack_capacity { 0 };
        int m_stack_size { 0 };
        SpinLock m_lock;

    public:
        ObjectCache(int block_size)
            : m_block_size(block_size)
        {
        }

        ~ObjectCache()
        {
            for (auto block : m_blocks) {
                delete[] block;
            }

            delete[] m_stack;
        }

        T* acquire()
        {
            SpinLockGuard guard(m_lock);
            if (!m_stack_size)
            {
                T* block = new T[m_block_size];
                m_blocks.push_back(block);

                m_stack_capacity += m_block_size;

                delete[] m_stack;
                m_stack = new T*[m_stack_capacity];

                for (int i = 0; i < m_block_size; ++i)
                    m_stack[i] = block + i;

                m_stack_size = m_block_size;
            }

            T* object = m_stack[--m_stack_size];
            return object;
        }

        void discard(T* object)
        {
            SpinLockGuard guard(m_lock);
            m_stack[m_stack_size++] = object;
        }
    };

    struct TaskQueue;

    class ThreadPool : private NonCopyable
    {
    private:
        friend struct TaskQueue;
        friend class ConcurrentQueue;
        friend class SerialQueue;

        struct Queue
        {
            ThreadPool* pool;
            int priority;
            std::atomic<int> task_input_count;
            std::atomic<int> task_complete_count;
            std::atomic<int> stamp_cancel;
            std::string name;
        };

        struct Task
        {
            Queue* queue;
            int stamp;
            std::function<void()> func;
        };

    public:
        ThreadPool(size_t size);
        ~ThreadPool();

        static ThreadPool& getInstance();
        static int getInstanceSize();

        int size() const;

        void enqueue(std::function<void()>&& func)
        {
            enqueue(m_static_queue, std::move(func));
        }

    protected:
        void thread(size_t threadID);

        Queue* createQueue(const std::string& name, int priority);
        void deleteQueue(Queue* queue);

        void enqueue(Queue* queue, std::function<void()>&& func);
        bool dequeue_and_process();
        void cancel(Queue* queue);
        void wait(Queue* queue);

    private:
        alignas(64) ObjectCache<Queue> m_queue_cache;
        alignas(64) TaskQueue* m_queues;

        std::atomic<bool> m_stop { false };
        std::atomic<int> m_sleep_count { 0 };
        std::mutex m_queue_mutex;
        std::condition_variable m_condition;

        Queue* m_static_queue;
        std::vector<std::thread> m_threads;
    };

    enum class Priority
    {
        HIGH = 0,
        NORMAL = 1,
        LOW = 2
    };

    /*
        ConcurrentQueue is API to submit work into the ThreadPool. The tasks have no
        dependency to each other and can be executed in any order. Any number of queues
        can be created from any thread in the program. The ThreadPool is shared between
        queues. The queues can be configuted to different priorities to control which tasks
        are more time critical.

        Usage example:

        // create queue
        ConcurrentQueue q;

        // submit work into the queue
        q.enqueue([] {
            // TODO: do your stuff here..
        });

        // wait until the queue is drained
        q.wait();

    */

    class ConcurrentQueue : private NonCopyable
    {
    protected:
        ThreadPool& m_pool;
        ThreadPool::Queue* m_queue;

    public:
        ConcurrentQueue();
        ConcurrentQueue(const std::string& name, Priority priority = Priority::NORMAL);
        ~ConcurrentQueue();

        template <class F, class... Args>
        void enqueue(F&& f, Args&&... args)
        {
            m_pool.enqueue(m_queue, std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }

        void cancel();
        void wait();
    };

    /*
        SerialQueue is API to serialize tasks to be executed after previous task
        in the queue has completed. The tasks are NOT executed in the ThreadPool; each
        queue has it's own execution thread.

        SerialQueue and ConcurrentQueue can be freely mixed can can enqueue work to other
        queues from their tasks.

        Usage example:

        // create queue
        SerialQueue s;

        // submit work into the queue
        s.enqueue([] {
            // TODO: do your stuff here..
        });

        // wait until the queue is drained
        s.wait();

    */

    class SerialQueue : private NonCopyable
    {
    protected:
        using Task = std::function<void()>;

        std::thread m_thread;
        std::atomic<bool> m_stop { false };

        std::atomic<bool> m_executing { false };
        std::queue<Task> m_task_queue;
        std::mutex m_queue_mutex;
        std::condition_variable m_condition;

        void thread();

    public:
        SerialQueue();
        SerialQueue(const std::string& name);
        ~SerialQueue();

        template <class F, class... Args>
        void enqueue(F&& f, Args&&... args)
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_task_queue.emplace(f, (args)...);
            lock.unlock();
            m_condition.notify_one();
        }

        void cancel();
        void wait();
    };

    /*
        Task is a simple queue-less convenience object to enqueue work into the ThreadPool.
        The synchronization has to be done "manually", for example, with atomic flag.
    */

    class Task
    {
    public:
        template <class F, class... Args>
        Task(F&& f, Args&&... args)
        {
            ThreadPool& pool = ThreadPool::getInstance();
            pool.enqueue(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }
    };

    /*
        FutureTask is an asynchronous API to submit tasks into the ThreadPool.
        The get() member function will block the current thread until the result is available
        and does not consume any significant amount of CPU; the thread will yield/sleep
        while waiting for the result.

        Usage example:

        // enqueue a simple task into the ThreadPool
        FutureTask<int> task([] () -> int {
            return 7;
        });

        // this will block until the task has been completed
        int x = task.get();

    */

    template <typename T>
    class FutureTask
    {
    private:
        using Future = std::future<T>;
        using Promise = std::promise<T>;
        using SharedPromise = std::shared_ptr<Promise>;
        using Function = std::function<T()>;

        Future m_future;

    public:
        template <class F, class... Args>
        FutureTask(F&& f, Args&&... args)
        {
            SharedPromise shared_promise = std::make_shared<Promise>();
            m_future = shared_promise->get_future();

            Function func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            auto container = std::bind([shared_promise] (Function func) {
                T value = func();
                shared_promise->set_value(value);
            }, func);

            ThreadPool& pool = ThreadPool::getInstance();
            pool.enqueue(container);
        }

        T get()
        {
            return m_future.get();
        }

        void wait()
        {
            m_future.wait();
        }
    };

    template <>
    class FutureTask<void>
    {
    private:
        using Future = std::future<void>;
        using Promise = std::promise<void>;
        using SharedPromise = std::shared_ptr<Promise>;
        using Function = std::function<void()>;

        Future m_future;

    public:
        template <class F, class... Args>
        FutureTask(F&& f, Args&&... args)
        {
            SharedPromise shared_promise = std::make_shared<Promise>();
            m_future = shared_promise->get_future();

            Function func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            auto container = std::bind([shared_promise] (Function func) {
                func();
                shared_promise->set_value();
            }, func);

            ThreadPool& pool = ThreadPool::getInstance();
            pool.enqueue(container);
        }

        void get()
        {
            m_future.get();
        }

        void wait()
        {
            m_future.wait();
        }
    };

} // namespace mango
