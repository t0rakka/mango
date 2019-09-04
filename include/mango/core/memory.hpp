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

        Memory(u8* address, size_t bytes)
            : address(address)
            , size(bytes)
        {
        }

        Memory(const u8* address, size_t bytes)
            : address(const_cast<u8*>(address))
            , size(bytes)
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
    // Alignment
    // -----------------------------------------------------------------------

    // NOTE: The alignment has to be a power-of-two and at least sizeof(void*)

    class Alignment
    {
    protected:
        u32 m_alignment;

    public:
        Alignment(); // default alignment
        Alignment(u32 alignment);

        operator u32 () const;
    };

    // -----------------------------------------------------------------------
    // aligned malloc / free
    // -----------------------------------------------------------------------

    void* aligned_malloc(size_t bytes, Alignment alignment = Alignment());
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
        AlignedPointer(size_t size, Alignment alignment = Alignment())
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

    // -----------------------------------------------------------------------
    // aligned (std) memory allocator
    // -----------------------------------------------------------------------

    template <typename T, size_t ALIGNMENT>
    class AlignedAllocator
    {
    public:
        typedef T               value_type;
        typedef T*              pointer;
        typedef T&              reference;
        typedef const T*        const_pointer;
        typedef const T&        const_reference;
        typedef std::size_t     size_type;
        typedef std::ptrdiff_t  difference_type;

        AlignedAllocator()
        {
        }

        AlignedAllocator(const AlignedAllocator& allocator)
        {
            MANGO_UNREFERENCED(allocator);
        }

        template <class U>
        AlignedAllocator(const AlignedAllocator<U, ALIGNMENT>& allocator)
        {
            MANGO_UNREFERENCED(allocator);
        }

        ~AlignedAllocator()
        {
        }

        pointer address(reference x) const
        {
            return &x;
        }

        const_pointer address(const_reference x) const
        {
            return &x;
        }

        pointer allocate(size_type n, std::allocator<void>::const_pointer hint = 0)
        {
            MANGO_UNREFERENCED(hint);
            void* s = aligned_malloc(n * sizeof(T), ALIGNMENT);
            return reinterpret_cast<pointer>(s);
        }

        void deallocate(pointer p, size_type n)
        {
            MANGO_UNREFERENCED(n);
            aligned_free(p);
        }

        void construct(pointer p, const_reference value)
        {
            new(p) T(value);
        }

        void destroy(pointer p)
        {
            p->~T();
        }

        template <class U, class... Args>
        void construct(U* p, Args&&... args)
        {
            new(p) T(args...);
        }

        template <class U>
        void destroy (U* p)
        {
            p->~T();
        }

        size_type max_size() const
        {
            return std::numeric_limits<size_type>::max() / sizeof(T);
        }

        template <typename U>
        struct rebind
        {
            typedef AlignedAllocator<U, ALIGNMENT> other;
        };

        bool operator == (AlignedAllocator const& allocator)
        {
            MANGO_UNREFERENCED(allocator);
            return true;
        }

        bool operator != (AlignedAllocator const& allocator)
        {
            return !operator == (allocator);
        }
    };

} // namespace mango
