/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <atomic>
#include <mango/core/configure.hpp>

namespace mango
{

    /* WARNING!
       Atomic locks are implemented as busy loops which consume significant
       amounts of CPU time if the locks are congested and held for a long
       period of time. The lock duration is non-deterministic to a highest degree.
    */

    // ----------------------------------------------------------------------------
    // SpinLock
    // ----------------------------------------------------------------------------

    class SpinLock
    {
    private:
        std::atomic_flag m_locked = ATOMIC_FLAG_INIT;

    public:
        bool tryLock()
        {
            return m_locked.test_and_set(std::memory_order_acquire);
        }

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

    class SpinLockGuard
    {
    private:
        SpinLock& m_spinlock;
        bool m_locked { false };

    public:
        SpinLockGuard(SpinLock& spinlock)
            : m_spinlock(spinlock)
        {
            lock();
        }

        ~SpinLockGuard()
        {
            unlock();
        }

        void lock()
        {
            if (!m_locked) {
                m_locked = true;
                m_spinlock.lock();
            }
        }

        void unlock()
        {
            if (m_locked) {
                m_locked = false;
                m_spinlock.unlock();
            }
        }
    };

    // ----------------------------------------------------------------------------
    // ReadWriteSpinLock
    // ----------------------------------------------------------------------------

    class ReadWriteSpinLock : protected SpinLock
    {
    private:
        std::atomic<int> m_read_count { 0 };

    public:
        bool tryWriteLock()
        {
            bool status = tryLock();
            if (status) {
                // acquired exclusive access - flush all readers
                while (m_read_count > 0) {
                }
            }

            return status;
        }

        void writeLock()
        {
            // acquire exclusive access
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

        bool tryReadLock()
        {
            bool status = tryLock();
            if (status) {
                // gained temporary exclusivity - add one reader
                m_read_count.fetch_add(1, std::memory_order_acquire);
                unlock();
            }

            return status;
        }

        void readLock()
        {
            // gain temporary exclusivity to add one reader
            lock();
            m_read_count.fetch_add(1, std::memory_order_acquire);
            unlock();
        }

        void readUnlock()
        {
            // reader can be released at any time w/o exclusivity
            m_read_count.fetch_sub(1, std::memory_order_release);
        }
    };

    class WriteSpinLockGuard
    {
    private:
        ReadWriteSpinLock& m_rwlock;
        bool m_locked { false };

    public:
        WriteSpinLockGuard(ReadWriteSpinLock& rwlock)
            : m_rwlock(rwlock)
        {
            lock();
        }

        ~WriteSpinLockGuard()
        {
            unlock();
        }

        void lock()
        {
            if (!m_locked) {
                m_locked = true;
                m_rwlock.writeLock();
            }
        }

        void unlock()
        {
            if (m_locked) {
                m_locked = false;
                m_rwlock.writeUnlock();
            }
        }
    };

    class ReadSpinLockGuard
    {
    private:
        ReadWriteSpinLock& m_rwlock;
        bool m_locked { false };

    public:
        ReadSpinLockGuard(ReadWriteSpinLock& rwlock)
            : m_rwlock(rwlock)
        {
            lock();
        }

        ~ReadSpinLockGuard()
        {
            unlock();
        }

        void lock()
        {
            if (!m_locked) {
                m_locked = true;
                m_rwlock.readLock();
            }
        }

        void unlock()
        {
            if (m_locked) {
                m_locked = false;
                m_rwlock.readUnlock();
            }
        }
    };

} // namespace mango
