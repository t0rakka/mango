/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace mango
{

    // -------------------------------------------------------------------------
    // RingBuffer (single-producer, single-consumer)
    //
    // Fixed-capacity FIFO between exactly one producer thread and one consumer
    // thread. Undefined behavior if multiple threads call push or pop.
    //
    // Capacity must be a power of two (at least 2). Indices wrap with masking;
    // all Capacity slots are usable (monotonic head/tail counters).
    //
    // try_push / try_pop return false when full / empty; they never block.
    // Further variants (e.g. MPSC, MPMC) may be added in this header later.
    //
    // Lock-free SPSC design (memory ordering + cached remote index), after
    // David Álvarez Rosa:
    //   https://david.alvarezrosa.com/posts/optimizing-a-lock-free-ring-buffer/
    // Index caching follows Erik Rigtorp / Lee et al. (see that article).
    // -------------------------------------------------------------------------

    template <typename T, std::size_t Capacity>
    class RingBuffer
    {
        static_assert(Capacity >= 2, "RingBuffer capacity must be at least 2.");
        static_assert((Capacity & (Capacity - 1)) == 0, "RingBuffer capacity must be a power of two.");
        static_assert(std::is_default_constructible_v<T>, "RingBuffer value type must be default constructible.");
        static_assert(std::is_nothrow_destructible_v<T>, "RingBuffer value type must be nothrow destructible.");

        static constexpr std::size_t mask = Capacity - 1;

    public:
        static constexpr std::size_t capacity() noexcept
        {
            return Capacity;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            const std::size_t tail = m_tail.load(std::memory_order_acquire);
            const std::size_t head = m_head.load(std::memory_order_acquire);
            return tail == head;
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            const std::size_t tail = m_tail.load(std::memory_order_acquire);
            const std::size_t head = m_head.load(std::memory_order_acquire);
            return head - tail;
        }

        [[nodiscard]] bool try_push(const T& value) noexcept(std::is_nothrow_copy_assignable_v<T>)
        {
            static_assert(std::is_nothrow_copy_assignable_v<T>,
                "RingBuffer::try_push(const T&) requires nothrow copy assignment.");

            const std::size_t head = m_head.load(std::memory_order_relaxed);
            const std::size_t next_head = head + 1;

            if (next_head - m_tail_cached > Capacity)
            {
                m_tail_cached = m_tail.load(std::memory_order_acquire);
                if (next_head - m_tail_cached > Capacity)
                {
                    return false;
                }
            }

            m_buffer[head & mask] = value;
            m_head.store(next_head, std::memory_order_release);
            return true;
        }

        [[nodiscard]] bool try_push(T&& value) noexcept(std::is_nothrow_move_assignable_v<T>)
        {
            static_assert(std::is_nothrow_move_assignable_v<T>,
                "RingBuffer::try_push(T&&) requires nothrow move assignment.");

            const std::size_t head = m_head.load(std::memory_order_relaxed);
            const std::size_t next_head = head + 1;

            if (next_head - m_tail_cached > Capacity)
            {
                m_tail_cached = m_tail.load(std::memory_order_acquire);
                if (next_head - m_tail_cached > Capacity)
                {
                    return false;
                }
            }

            m_buffer[head & mask] = std::move(value);
            m_head.store(next_head, std::memory_order_release);
            return true;
        }

        template <typename... Args>
        [[nodiscard]] bool try_emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        {
            static_assert(std::is_nothrow_constructible_v<T, Args...>,
                "RingBuffer::try_emplace requires nothrow construction.");

            const std::size_t head = m_head.load(std::memory_order_relaxed);
            const std::size_t next_head = head + 1;

            if (next_head - m_tail_cached > Capacity)
            {
                m_tail_cached = m_tail.load(std::memory_order_acquire);
                if (next_head - m_tail_cached > Capacity)
                {
                    return false;
                }
            }

            T& slot = m_buffer[head & mask];
            slot.~T();
            new (&slot) T(std::forward<Args>(args)...);
            m_head.store(next_head, std::memory_order_release);
            return true;
        }

        [[nodiscard]] bool try_pop(T& value) noexcept(std::is_nothrow_move_assignable_v<T>)
        {
            static_assert(std::is_nothrow_move_assignable_v<T>,
                "RingBuffer::try_pop requires nothrow move assignment.");

            const std::size_t tail = m_tail.load(std::memory_order_relaxed);

            if (tail == m_head_cached)
            {
                m_head_cached = m_head.load(std::memory_order_acquire);
                if (tail == m_head_cached)
                {
                    return false;
                }
            }

            value = std::move(m_buffer[tail & mask]);
            m_tail.store(tail + 1, std::memory_order_release);
            return true;
        }

    private:
        std::array<T, Capacity> m_buffer {};

        // Producer-owned atomic; consumer acquires.
        alignas(64) std::atomic<std::size_t> m_head { 0 };
        // Consumer-side cache of m_head (consumer thread only).
        alignas(64) std::size_t m_head_cached { 0 };

        // Consumer-owned atomic; producer acquires.
        alignas(64) std::atomic<std::size_t> m_tail { 0 };
        // Producer-side cache of m_tail (producer thread only).
        alignas(64) std::size_t m_tail_cached { 0 };
    };

} // namespace mango
