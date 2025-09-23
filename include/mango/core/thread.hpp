/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
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
#include <mango/core/exception.hpp>
#include <mango/core/memory.hpp>
#include <mango/core/atomic.hpp>

namespace mango
{

    // ----------------------------------------------------------------------------------
    // ThreadPool
    // ----------------------------------------------------------------------------------

    class ThreadPool : private NonCopyable
    {
    private:
        friend class ConcurrentQueue;

        struct Queue
        {
            ThreadPool* pool;
            std::string name;

            alignas(64) std::atomic<int> task_counter { 0 };
            alignas(64) std::atomic<bool> cancelled { false };

            Queue(ThreadPool* pool, const std::string& name)
                : pool(pool)
                , name(name)
            {
            }
        };

        struct Task
        {
            Queue* queue;
            std::function<void()> func;
        };

    public:
        ThreadPool(size_t size);
        ~ThreadPool();

        static size_t getHardwareConcurrency();
        static ThreadPool& getInstance();

        int size() const;

    protected:
        struct Consumer;

        void thread(size_t threadID);

        void enqueue(Queue* queue, std::function<void()>&& func);
        void enqueue_bulk(Queue* queue, const std::vector<std::function<void()>>& functions);
        void process(Task& task) const;
        bool dequeue_and_process();
        void cancel(Queue* queue);
        void wait(Queue* queue);

    private:
        struct TaskQueue;
        alignas(64) TaskQueue* m_queue;

        alignas(64) std::atomic<bool> m_stop { false };
        std::mutex m_queue_mutex;
        std::condition_variable m_condition;
        std::vector<std::thread> m_threads;
    };

    // ----------------------------------------------------------------------------------
    // ConcurrentQueue
    // ----------------------------------------------------------------------------------

    /*
        ConcurrentQueue is API to submit work into the ThreadPool. The tasks have no
        dependency to each other and can be executed in any order. Any number of queues
        can be created from any thread in the program. The ThreadPool is shared between
        queues.

        Usage example:

            // create queue
            ConcurrentQueue q;

            // submit work into the queue
            q.enqueue([]
            {
                // your stuff here..
            });

            // wait until the queue is drained
            q.wait(); // cooperative, blocking (helps pool until all tasks are complete)

    */

    class ConcurrentQueue : private NonCopyable
    {
    protected:
        ThreadPool& m_pool;
        ThreadPool::Queue m_queue;

    public:
        ConcurrentQueue();
        ConcurrentQueue(const std::string& name);
        ConcurrentQueue(ThreadPool& pool);
        ConcurrentQueue(ThreadPool& pool, const std::string& name);
        ~ConcurrentQueue();

        template <class F, class... Args>
        void enqueue(F&& f, Args&&... args)
        {
            m_pool.enqueue(&m_queue, std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }

        void enqueue_bulk(const std::vector<std::function<void()>>& functions)
        {
            m_pool.enqueue_bulk(&m_queue, functions);
        }

        bool isCancelled() const
        {
            return m_queue.cancelled;
        }

        void steal();
        void cancel();
        void wait();
    };

    // ----------------------------------------------------------------------------------
    // SerialQueue
    // ----------------------------------------------------------------------------------

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
            s.enqueue([]
            {
                // your stuff here..
            });

            // wait until the queue is drained
            s.wait(); // non-cooperative, blocking (CPU sleeps until queue is drained)

    */

    class SerialQueue : private NonCopyable
    {
    protected:
        using Task = std::function<void()>;

        std::string m_name;
        std::thread m_thread;

        alignas(64) std::atomic<bool> m_stop { false };
        alignas(64) std::atomic<int> m_task_counter { 0 };

        std::deque<Task> m_task_queue;
        std::mutex m_queue_mutex;
        std::mutex m_wait_mutex;
        std::condition_variable m_task_condition;
        std::condition_variable m_wait_condition;

        void thread();

    public:
        SerialQueue();
        SerialQueue(const std::string& name);
        ~SerialQueue();

        template <class F, class... Args>
        void enqueue(F&& f, Args&&... args)
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_task_queue.emplace_back(f, (args)...);
            ++m_task_counter;
            m_task_condition.notify_one();
        }

        void cancel();
        void wait();
    };

    // ----------------------------------------------------------------------------------
    // TicketQueue
    // ----------------------------------------------------------------------------------

    /*
        TicketQueue is a non-blocking serialization mechanism which allows to schedule work
        from ANY thread with deterministic order. The order is determined by a ticket based
        system; tickets are consumed in the same order they are acquired.

        Usage example:

            ConcurrentQueue q;
            TicketQueue tk;

            for (int i = 0; i < 10; ++i)
            {
                auto ticket = tk.acquire();

                q.enqueue([ticket]
                {
                    // do your heavy calculation here..

                    ticket.consume([]
                    {
                        // this work is serialized and executed in the order the tickets were acquired
                    });
                });
            }

            // wait until the queue is drained
            q.wait();

            // we know all workers have completed -> no more tickets will be consumed
            // now we wait until ticket queue is finished
            tk.wait();

    */

    class TicketQueue : private NonCopyable
    {
    protected:
        struct Task
        {
            std::atomic<int> count { 1 };
            std::function<void()> func;
            std::promise<void> promise;
        };

    public:
        using SharedTask = std::shared_ptr<Task>;

        class Ticket
        {
        protected:
            friend class TicketQueue;

            SharedTask task;

        public:
            Ticket();
            ~Ticket();

            Ticket(const Ticket& ticket);
            const Ticket& operator = (const Ticket& ticket);

            template <class F, class... Args>
            void consume(F&& f, Args&&... args) const
            {
                task->func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
                task->promise.set_value();
            }
        };

        TicketQueue();
        ~TicketQueue();

        Ticket acquire();
        std::vector<Ticket> acquire(size_t count);
        void wait();

    protected:
        std::thread m_thread;

        alignas(64) std::atomic<bool> m_stop { false };
        alignas(64) std::atomic<int> m_ticket_counter { 0 };

        std::mutex m_wait_mutex;
        std::mutex m_consume_mutex;
        std::condition_variable m_wait_condition;
        std::condition_variable m_consume_condition;

        struct TaskQueue;
        alignas(64) TaskQueue* m_queue;

        bool dequeue_and_process();
    };

} // namespace mango
