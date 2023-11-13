/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <memory>
#include <limits>
#include <algorithm>
#include <mango/core/configure.hpp>

#if MANGO_CPP_VERSION >= 20
    #include <span>
#endif

namespace mango::detail
{

    template <typename T>
    struct Memory
    {
        T* address;
        size_t size;

        Memory()
            : address(nullptr)
            , size(0)
        {
        }

        Memory(T* address, size_t bytes)
            : address(address)
            , size(bytes)
        {
        }

#if MANGO_CPP_VERSION >= 20

        Memory(std::span<T> s)
            : address(s.data())
            , size(s.size())
        {
        }

        const Memory& operator = (std::span<T> s)
        {
            address = s.data();
            size = s.size();
            return *this;
        }

        operator std::span<T> () const
        {
            return std::span<T>(address, address + size);
        }

#endif

        T* end() const
        {
            return address + size;
        }

        operator T* () const
        {
            return address;
        }

        template <typename S>
        operator Memory<S> () const
        {
            return Memory<S>(address, size);
        }

        template <typename S>
        S* cast() const
        {
            return reinterpret_cast<S*>(address);
        }

        Memory slice(size_t slice_offset, size_t slice_size = 0) const
        {
            Memory memory(address + slice_offset, size - slice_offset);
            if (slice_size)
            {
                memory.size = std::min(memory.size, slice_size);
            }
            return memory;
        }
    };

} // namespace mango::detail

namespace mango
{

    // -----------------------------------------------------------------------
    // NonCopyable
    // -----------------------------------------------------------------------

    class NonCopyable
    {
    protected:
        NonCopyable() = default;

    private:
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator = (const NonCopyable&) = delete;
    };

    // -----------------------------------------------------------------------
    // memory
    // -----------------------------------------------------------------------

    using Memory = detail::Memory<u8>;
    using ConstMemory = detail::Memory<const u8>;

    class SharedMemory
    {
    private:
        Memory m_memory;
        std::shared_ptr<u8> m_ptr;

    public:
        SharedMemory(size_t bytes);
        SharedMemory(u8* address, size_t bytes);

        operator Memory () const
        {
            return m_memory;
        }
    };

    class VirtualMemory : private NonCopyable
    {
    protected:
        ConstMemory m_memory;

    public:
        VirtualMemory() = default;
        virtual ~VirtualMemory() {}

        const ConstMemory* operator -> () const
        {
            return &m_memory;
        }

        operator ConstMemory () const
        {
            return m_memory;
        }
    };

    // -----------------------------------------------------------------------
    // aligned malloc / free
    // -----------------------------------------------------------------------

    // IMPORTANT: Don't give in to temptation to use std::aligned_alloc -
    //            IT DOESN'T WORK ON ALL PLATFORMS! Blood has been sacrificed for this notice!

    void* aligned_malloc(size_t bytes, size_t alignment = 64);
    void aligned_free(void* aligned);

    template <typename T>
    T* aligned_malloc(size_t count, size_t alignment = 64)
    {
        void* ptr = aligned_malloc(count * sizeof(T), alignment);
        return reinterpret_cast<T*>(ptr);
    }

    // -----------------------------------------------------------------------
    // AlignedStorage
    // -----------------------------------------------------------------------

    template <typename T>
    class AlignedStorage : public NonCopyable
    {
    private:
        void* m_data;
        size_t m_size;

    public:
        AlignedStorage()
            : m_data(nullptr)
            , m_size(0)
        {
        }

        AlignedStorage(size_t size, size_t alignment = 64)
        {
            m_data = aligned_malloc(size * sizeof(T), alignment);
            m_size = size;
        }

        ~AlignedStorage()
        {
            aligned_free(m_data);
        }

        void resize(size_t size, size_t alignment = 64)
        {
            if (size != m_size)
            {
                aligned_free(m_data);
                m_data = size ? aligned_malloc(size * sizeof(T), alignment) : nullptr;
                m_size = size;
            }
        }

        operator T* () const
        {
            return reinterpret_cast<T*>(m_data);
        }

        T* data() const
        {
            return reinterpret_cast<T*>(m_data);
        }

        size_t size() const
        {
            return m_size;
        }

        T& operator [] (int index)
        {
            return data()[index];
        }

        const T& operator [] (int index) const
        {
            return data()[index];
        }

        T* begin()
        {
            return data();
        }

        T* end()
        {
            return data() + m_size;
        }

        const T* begin() const
        {
            return data();
        }

        const T* end() const
        {
            return data() + m_size;
        }
    };

} // namespace mango
