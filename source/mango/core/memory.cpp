/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cassert>
#include <mango/core/bits.hpp>
#include <mango/core/memory.hpp>

namespace mango {

    // -----------------------------------------------------------------------
    // Memory
    // -----------------------------------------------------------------------

    Memory::Memory()
        : address(nullptr)
        , size(0)
    {
    }

    Memory::Memory(uint8* address, size_t size)
        : address(address)
        , size(size)
    {
    }

    Memory::operator uint8* () const
    {
        return address;
    }

    Memory::operator char* () const
    {
		return reinterpret_cast<char *>(address);
    }

    Memory Memory::slice(size_t slice_offset, size_t slice_size) const
    {
        Memory memory(address + slice_offset, size - slice_offset);
        if (slice_size) {
            memory.size = std::min(memory.size, slice_size);
        }
        return memory;
    }

    // -----------------------------------------------------------------------
    // SharedMemory
    // -----------------------------------------------------------------------

    SharedMemory::SharedMemory(size_t size)
    {
        uint8 *address = new uint8[size];
        m_memory = Memory(address, size);
        m_ptr = std::shared_ptr<uint8>(address, std::default_delete<uint8[]>());
    }

    SharedMemory::SharedMemory(uint8* address, size_t size)
        : m_memory(address, size)
        , m_ptr(address, std::default_delete<uint8[]>())
    {
    }

    // -----------------------------------------------------------------------
    // aligned malloc/free
    // -----------------------------------------------------------------------

#if defined(MANGO_COMPILER_MICROSOFT)

    void* aligned_malloc(size_t size, size_t alignment)
    {
        assert(u32_is_power_of_two(uint32(alignment)));
        return _aligned_malloc(size, alignment);
    }

    void aligned_free(void* aligned)
    {
        _aligned_free(aligned);
    }

#elif defined(MANGO_PLATFORM_LINUX)

    void* aligned_malloc(size_t size, size_t alignment)
    {
        assert(u32_is_power_of_two(uint32(alignment)));
        return memalign(alignment, size);
    }

    void aligned_free(void* aligned)
    {
        free(aligned);
    }

#else

    // generic implementation

    void* aligned_malloc(size_t size, size_t alignment)
    {
        assert(u32_is_power_of_two(uint32(alignment)));

        const size_t mask = alignment - 1;
        void* block = std::malloc(size + mask + sizeof(void*));
        char* aligned = reinterpret_cast<char*>(block) + sizeof(void*);

        if (block) {
            aligned += alignment - (reinterpret_cast<ptrdiff_t>(aligned) & mask);
            reinterpret_cast<void**>(aligned)[-1] = block;
        }
        else {
            aligned = nullptr;
        }

        return aligned;
    }

    void aligned_free(void* aligned)
    {
        if (aligned) {
            void* block = reinterpret_cast<void**>(aligned)[-1];
            std::free(block);
        }
    }

#endif

} // namespace mango
