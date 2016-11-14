/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <memory>
#include <limits>
#include <algorithm>
#include "configure.hpp"
#include "object.hpp"

namespace mango
{

    // -----------------------------------------------------------------------
    // memory block
    // -----------------------------------------------------------------------

   	struct Memory
    {
		Object* object;
        size_t size;
        const uint8* address;

        Memory();
        Memory(const uint8* address, size_t size);
		Memory(const Memory& memory);
        virtual ~Memory();

		const Memory& operator = (const Memory& memory);

		operator const uint8* () const;
		operator const char* () const;

        Memory getBlock(size_t offset, size_t size = 0) const;
    };

	struct ManagedMemory : Memory
	{
		ManagedMemory(size_t size);
        ManagedMemory(const uint8* address, size_t size);

		operator uint8* () const;
		operator char* () const;
	};

    // -----------------------------------------------------------------------
    // aligned malloc/free
    // -----------------------------------------------------------------------

    void* aligned_malloc(size_t size);
    void aligned_free(void* aligned);

    // -----------------------------------------------------------------------
    // aligned memory allocator
    // -----------------------------------------------------------------------

    template <typename T>
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
            MANGO_UNREFERENCED_PARAMETER(allocator);
        }

        template <class U>
        AlignedAllocator(const AlignedAllocator<U>& allocator)
        {
            MANGO_UNREFERENCED_PARAMETER(allocator);
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
            MANGO_UNREFERENCED_PARAMETER(hint);
            void* s = aligned_malloc(n * sizeof(T));
            return reinterpret_cast<pointer>(s);
        }

        void deallocate(pointer p, size_type n)
        {
            MANGO_UNREFERENCED_PARAMETER(n);
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
            typedef AlignedAllocator<U> other;
        };

		bool operator == (AlignedAllocator const& allocator)
        {
            MANGO_UNREFERENCED_PARAMETER(allocator);
            return true;
        }

        bool operator != (AlignedAllocator const& allocator)
        {
            return !operator == (allocator);
        }
    };

} // namespace mango
