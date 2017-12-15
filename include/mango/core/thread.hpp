/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

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
            std::atomic<int> reference_count;
            std::atomic<int> task_input_count;
            std::atomic<int> task_complete_count;
            std::atomic<int> stamp_cancel;
            int stamp_barrier;
            std::string name;

            void retain()
            {
                ++reference_count;
            }

            void release()
            {
                if (!--reference_count) {
                    pool->deleteQueue(this);
                }
            }
        };

        struct Task
        {
            Queue* queue;
            int stamp;
            int barrier;
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
        bool dequeue_process();
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

        void barrier();
        void cancel();
        void wait();
    };

    class SerialQueue : private NonCopyable
    {
    protected:
        ThreadPool& m_pool;
        ThreadPool::Queue* m_queue;

    public:
        SerialQueue();
        SerialQueue(const std::string& name, Priority priority = Priority::NORMAL);
        ~SerialQueue();

        template <class F, class... Args>
        void enqueue(F&& f, Args&&... args)
        {
            m_pool.enqueue(m_queue, std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            m_queue->stamp_barrier = m_queue->task_input_count;
        }

        void cancel();
        void wait();
    };

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

#if 0
    // TODO: should probably be deprecated and/or refactored into
    //       SharedPromise<T> class which only assists in capturing promises.

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
            m_future = std::move(shared_promise->get_future());

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
            m_future = std::move(shared_promise->get_future());

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
#endif

} // namespace mango
