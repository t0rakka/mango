/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <memory>
#include <limits>
#include <algorithm>
#include "configure.hpp"
#include "object.hpp"

namespace mango {

    // -----------------------------------------------------------------------
    // memory
    // -----------------------------------------------------------------------

    struct Memory
    {
        u8* address;
        size_t size;

        Memory()
            : address(nullptr)
            , size(0)
        {
        }

        Memory(u8* address, size_t size)
            : address(address)
            , size(size)
        {
        }

        Memory(const u8* address, size_t size)
            : address(const_cast<u8*>(address))
            , size(size)
        {
        }

        operator u8* () const
        {
            return address;
        }

        template <typename T>
        T* cast() const
        {
            return reinterpret_cast<T*>(address);
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

    class SharedMemory
    {
    private:
        Memory m_memory;
        std::shared_ptr<u8> m_ptr;

    public:
        SharedMemory(size_t size);
        SharedMemory(u8* address, size_t size);

        operator Memory () const
        {
            return m_memory;
        }
    };

    class VirtualMemory : private NonCopyable
    {
    protected:
        Memory m_memory;

    public:
        VirtualMemory() = default;
        virtual ~VirtualMemory() {}

        const Memory* operator -> () const
        {
            return &m_memory;
        }

        operator Memory () const
        {
            return m_memory;
        }
    };

    // -----------------------------------------------------------------------
    // aligned malloc / free
    // -----------------------------------------------------------------------

    // NOTE: The alignment has to be a power-of-two and at least sizeof(void*)

    void* aligned_malloc(size_t size, size_t alignment = MANGO_DEFAULT_ALIGNMENT);
    void aligned_free(void* aligned);

    // -----------------------------------------------------------------------
    // AlignedPointer
    // -----------------------------------------------------------------------

    // ONLY store POD types ; even if we have configurable type for the pointer
    // it's only for convenience and WILL NOT call constructor / destructor !!!

    template <typename T>
    class AlignedPointer : public NonCopyable
    {
    private:
        T* m_data;
        size_t m_size;

    public:
        AlignedPointer(size_t size, size_t alignment = MANGO_DEFAULT_ALIGNMENT)
            : m_size(size)
        {
            void* ptr = aligned_malloc(size * sizeof(T), alignment);
            m_data = reinterpret_cast<T*>(ptr);
        }

        ~AlignedPointer()
        {
            aligned_free(m_data);
        }

        operator T* () const
        {
            return m_data;
        }

        T* data() const
        {
            return m_data;
        }

        size_t size() const
        {
            return m_size;
        }
    };

} // namespace mango
