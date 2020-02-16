/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
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

    // ----------------------------------------------------------------------------------
    // ThreadPool
    // ----------------------------------------------------------------------------------

    class ThreadPool : private NonCopyable
    {
    private:
        friend struct TaskQueue;
        friend class ConcurrentQueue;

        struct Queue
        {
            ThreadPool* pool;
            int priority;
            std::atomic<int> task_input_count { 0 };
            std::atomic<int> task_complete_count { 0 };
            std::atomic<bool> cancelled { false };
            std::string name;

            Queue(ThreadPool* pool, int priority, const std::string& name)
                : pool(pool)
                , priority(priority)
                , name(name)
            {
            }

            bool empty() const
            {
                return task_input_count.load() == task_complete_count.load();
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

        static ThreadPool& getInstance();
        static int getInstanceSize();

        int size() const;

        void enqueue(std::function<void()>&& func)
        {
            enqueue(&m_static_queue, std::move(func));
        }

    protected:
        void thread(size_t threadID);

        void enqueue(Queue* queue, std::function<void()>&& func);
        bool dequeue_and_process();
        void cancel(Queue* queue);
        void wait(Queue* queue);

    private:
        alignas(64) struct TaskQueue* m_queues;

        std::atomic<bool> m_stop { false };
        std::atomic<int> m_sleep_count { 0 };
        std::mutex m_queue_mutex;
        std::condition_variable m_condition;

        Queue m_static_queue;
        std::vector<std::thread> m_threads;
    };

    enum class Priority
    {
        HIGH = 0,
        NORMAL = 1,
        LOW = 2
    };

    // ----------------------------------------------------------------------------------
    // ConcurrentQueue
    // ----------------------------------------------------------------------------------

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
        q.enqueue([]
        {
            // TODO: do your stuff here..
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
        ConcurrentQueue(const std::string& name, Priority priority = Priority::NORMAL);
        ~ConcurrentQueue();

        template <class F, class... Args>
        void enqueue(F&& f, Args&&... args)
        {
            m_pool.enqueue(&m_queue, std::bind(std::forward<F>(f), std::forward<Args>(args)...));
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
            // TODO: do your stuff here..
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
        std::atomic<bool> m_stop { false };
        std::atomic<int> m_task_counter { 0 };

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
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_task_queue.emplace_back(f, (args)...);
            ++m_task_counter;
            m_task_condition.notify_one();
        }

        void cancel();
        void wait();
    };

    // ----------------------------------------------------------------------------------
    // Task
    // ----------------------------------------------------------------------------------

    /*
        Task is a simple queue-less convenience object to enqueue work into the ThreadPool.
        Synchronization must be done manually.
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

    // ----------------------------------------------------------------------------------
    // FutureTask
    // ----------------------------------------------------------------------------------

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
        using Function = std::function<T()>;

        Promise m_promise;
        Future m_future;

    public:
        template <class F, class... Args>
        FutureTask(F&& f, Args&&... args)
            : m_promise()
            , m_future(m_promise.get_future())
        {
            Function func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            auto container = std::bind([this] (Function func) {
                T value = func();
                m_promise.set_value(value);
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
        using Function = std::function<void()>;

        Promise m_promise;
        Future m_future;

    public:
        template <class F, class... Args>
        FutureTask(F&& f, Args&&... args)
            : m_promise()
            , m_future(m_promise.get_future())
        {
            Function func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            auto container = std::bind([this] (Function func) {
                func();
                m_promise.set_value();
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

        // we know all workes have completed -> no more tickets will be consumed
        // now we wait until ticket queue is finished
        tk.wait();

    */

    class TicketQueue : private NonCopyable
    {
    protected:
        friend struct TaskQueue2;

        struct Task
        {
            std::atomic<int> count { 1 };
            std::atomic<bool> ready { false };
            std::function<void()> func;
            std::promise<void> promise;
        };

        using SharedTask = std::shared_ptr<Task>;

    public:
        class Ticket
        {
        protected:
            friend class TicketQueue;

            SharedTask task;

        public:
            Ticket()
                : task(std::make_shared<Task>())
            {
            }

            Ticket(const Ticket& ticket)
            {
                task = ticket.task;
                task->count++;
            }

            const Ticket& operator = (const Ticket& ticket)
            {
                task = ticket.task;
                task->count++;
                return *this;
            }

            ~Ticket()
            {
                if (!--task->count)
                {
                    if (!task->ready)
                    {
                        task->promise.set_value();
                    }
                }
            }

            template <class F, class... Args>
            void consume(F&& f, Args&&... args) const
            {
                task->func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
                task->ready = true;
                task->promise.set_value();
            }
        };

        TicketQueue();
        ~TicketQueue();

        Ticket acquire();
        void wait();

    protected:
        std::atomic<bool> m_stop { false };
        std::thread m_thread;

        std::atomic<int> m_ticket_counter { 0 };

        std::mutex m_wait_mutex;
        std::mutex m_consume_mutex;
        std::condition_variable m_wait_condition;
        std::condition_variable m_consume_condition;

        alignas(64) struct TaskQueue2* m_queue;

        bool dequeue_and_process();
    };

} // namespace mango
