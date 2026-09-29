/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <atomic>
#include <thread>
#include <mango/core/configure.hpp>

#if defined(MANGO_CPU_INTEL) && defined(MANGO_COMPILER_MSVC)
    #include <intrin.h>
#endif

#if defined(MANGO_CPU_ARM) && defined(MANGO_COMPILER_MSVC)
    #if defined(_M_ARM64)
        #include <arm64intr.h>
    #elif defined(_M_ARM)
        #include <armintr.h>
    #endif
#endif

namespace mango
{

    // ----------------------------------------------------------------------------
    // pause()
    // ----------------------------------------------------------------------------
    //
    // Architecture-specific spin-wait hint for contended atomics (see SpinLock).
    // Falls back to std::this_thread::yield() only when no suitable hint exists.

    inline
    void pause()
    {
#if defined(MANGO_CPU_INTEL)

        #if defined(MANGO_COMPILER_MSVC)
            _mm_pause();
        #elif defined(__has_builtin)
            #if __has_builtin(__builtin_ia32_pause)
                __builtin_ia32_pause();
            #else
                __asm__ __volatile__("pause" ::: "memory");
            #endif
        #else
            __builtin_ia32_pause();
        #endif

#elif defined(MANGO_CPU_ARM)

        #if defined(MANGO_COMPILER_MSVC)
            __yield();
        #elif defined(__has_builtin)
            #if __has_builtin(__builtin_arm_yield)
                __builtin_arm_yield();
            #elif defined(__aarch64__) || (defined(__ARM_ARCH) && __ARM_ARCH >= 7)
                __asm__ __volatile__("yield" ::: "memory");
            #else
                __asm__ __volatile__("nop" ::: "memory");
            #endif
        #elif defined(__aarch64__) || (defined(__ARM_ARCH) && __ARM_ARCH >= 7)
            __asm__ __volatile__("yield" ::: "memory");
        #else
            __asm__ __volatile__("nop" ::: "memory");
        #endif

#elif defined(MANGO_CPU_PPC)

        #if defined(__has_builtin) && __has_builtin(__builtin_ppc_yield)
            __builtin_ppc_yield();
        #else
            // POWER thread-yield hint (Linux kernel cpu_relax); safe on older cores as a lightweight barrier.
            __asm__ __volatile__("or 1,1,1" ::: "memory");
        #endif

#elif defined(MANGO_CPU_MIPS)

        #if ((defined(__mips_isa_rev) && __mips_isa_rev >= 2) || \
             (defined(_MIPS_ISA_REV) && _MIPS_ISA_REV >= 2))
            __asm__ __volatile__("pause" ::: "memory");
        #else
            __asm__ __volatile__("nop" ::: "memory");
        #endif

#elif defined(MANGO_CPU_RISCV)

        #if defined(__has_builtin) && __has_builtin(__builtin_riscv_pause)
            __builtin_riscv_pause();
        #else
            __asm__ __volatile__("nop" ::: "memory");
        #endif

#elif defined(MANGO_CPU_SPARC)

        __asm__ __volatile__("membar #LoadLoad | #LoadStore" ::: "memory");

#elif defined(MANGO_CPU_ALPHA)

        __asm__ __volatile__("" ::: "memory");

#elif defined(MANGO_CPU_M68K)

        __asm__ __volatile__("nop" ::: "memory");

#else

        std::this_thread::yield();

#endif
    }

    // ----------------------------------------------------------------------------
    // SpinLock
    // ----------------------------------------------------------------------------

    /* WARNING!
       Atomic locks are implemented as busy loops which potentially consume
       significant amounts of CPU time.
    */

    // Contended spin-lock strategy (acquire/release ordering, test-and-test-and-set,
    // exponential backoff) after David Álvarez Rosa:
    // https://david.alvarezrosa.com/posts/optimizing-a-spin-lock/

    class SpinLock
    {
    private:
        std::atomic<bool> m_locked = { false };

    public:
        bool tryLock()
        {
            return !m_locked.load(std::memory_order_relaxed) &&
                   !m_locked.exchange(true, std::memory_order_acquire);
        }

        void lock()
        {
            int backoff = 1;
            while (m_locked.exchange(true, std::memory_order_acquire))
            {
                do
                {
                    for (int i = 0; i < backoff; ++i)
                    {
                        pause();
                    }
                    backoff = backoff < 64 ? backoff << 1 : 64;
                }
                while (m_locked.load(std::memory_order_relaxed));
            }
        }

        void unlock()
        {
            m_locked.store(false, std::memory_order_release);
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
            if (!m_locked)
            {
                m_locked = true;
                m_spinlock.lock();
            }
        }

        void unlock()
        {
            if (m_locked)
            {
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
            if (status)
            {
                // acquired exclusive access - flush all readers
                while (m_read_count > 0)
                {
                }
            }

            return status;
        }

        void writeLock()
        {
            // acquire exclusive access
            lock();

            // flush all readers
            while (m_read_count > 0)
            {
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
            if (status)
            {
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
            if (!m_locked)
            {
                m_locked = true;
                m_rwlock.writeLock();
            }
        }

        void unlock()
        {
            if (m_locked)
            {
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
            if (!m_locked)
            {
                m_locked = true;
                m_rwlock.readLock();
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_locked = false;
                m_rwlock.readUnlock();
            }
        }
    };

} // namespace mango
