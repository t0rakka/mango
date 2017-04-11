/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <atomic>
#include "configure.hpp"

namespace mango
{

    /* WARNING!
       Atomic locks are implemented as busy loops which consume significant
       amounts of CPU time if the locks are congested and held for a long
       period of time.
    */

    // ----------------------------------------------------------------------------
    // SpinMutex / SpinLock
    // ----------------------------------------------------------------------------

    class SpinMutex
    {
    private:
        std::atomic_flag m_locked = ATOMIC_FLAG_INIT;

    public:
        void lock()
        {
            while (m_locked.test_and_set(std::memory_order_acquire)) {
            }
        }

        void unlock()
        {
            m_locked.clear(std::memory_order_release);
        }
    };

    class SpinLock
    {
    private:
        SpinMutex& m_mutex;
        bool m_locked { false };

    public:
        SpinLock(SpinMutex& mutex)
            : m_mutex(mutex)
        {
            lock();
        }

        ~SpinLock()
        {
            unlock();
        }

        void lock()
        {
            if (!m_locked) {
                m_locked = true;
                m_mutex.lock();
            }
        }

        void unlock()
        {
            if (m_locked) {
                m_locked = false;
                m_mutex.unlock();
            }
        }
    };

    // ----------------------------------------------------------------------------
    // ReadWriteMutex / WriteLock / ReadLock
    // ----------------------------------------------------------------------------

    class ReadWriteMutex : protected SpinMutex
    {
    private:
        std::atomic<int> m_read_count { 0 };

    public:
        void writeLock()
        {
            // acquire exclusive access to the mutex
            lock();

            // flush all readers
            while (m_read_count > 0) {
            }
        }

        void writeUnlock()
        {
            // release exclusivity
            unlock();
        }

        void readLock()
        {
            // gain temporary exclusivity to add one reader
            lock();
            ++m_read_count;
            unlock();
        }

        void readUnlock()
        {
            // reader can be released at any time w/o exclusivity
            --m_read_count;
        }
    };

    class WriteLock
    {
    private:
        ReadWriteMutex& m_mutex;
        bool m_locked { false };

    public:
        WriteLock(ReadWriteMutex& mutex)
            : m_mutex(mutex)
        {
            lock();
        }

        ~WriteLock()
        {
            unlock();
        }

        void lock()
        {
            if (!m_locked) {
                m_locked = true;
                m_mutex.writeLock();
            }
        }

        void unlock()
        {
            if (m_locked) {
                m_locked = false;
                m_mutex.writeUnlock();
            }
        }
    };

    class ReadLock
    {
    private:
        ReadWriteMutex& m_mutex;
        bool m_locked { false };

    public:
        ReadLock(ReadWriteMutex& mutex)
            : m_mutex(mutex)
        {
            lock();
        }

        ~ReadLock()
        {
            unlock();
        }

        void lock()
        {
            if (!m_locked) {
                m_locked = true;
                m_mutex.readLock();
            }
        }

        void unlock()
        {
            if (m_locked) {
                m_locked = false;
                m_mutex.readUnlock();
            }
        }
    };

} // namespace mango
