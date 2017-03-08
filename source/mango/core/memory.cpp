/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
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

    Memory Memory::slice(size_t offset, size_t mem_size) const
    {
        Memory memory(address + offset, size - offset);
        if (mem_size) {
            memory.size = std::min(memory.size, mem_size);
        }
        return memory;
    }

    // -----------------------------------------------------------------------
    // SharedMemory
    // -----------------------------------------------------------------------

    SharedMemory::SharedMemory(size_t size)
    {
        uint8 *address = new uint8[size];
        memory = Memory(address, size);
        ptr = std::shared_ptr<uint8>(address, std::default_delete<uint8[]>());
    }

    SharedMemory::SharedMemory(uint8* address, size_t size)
        : memory(address, size)
        , ptr(address, std::default_delete<uint8[]>())
    {
    }

    // -----------------------------------------------------------------------
    // aligned malloc/free
    // -----------------------------------------------------------------------

#if defined(MANGO_PLATFORM_OSX)

    // NOTE: OSX ABI already aligns memory allocations to 16 bytes

    void* aligned_malloc(size_t size)
    {
        return std::malloc(size);
    }

    void aligned_free(void* aligned)
    {
        std::free(aligned);
    }

#elif defined(MANGO_COMPILER_MICROSOFT)

    void* aligned_malloc(size_t size)
    {
        return _aligned_malloc(size, MANGO_MEMORY_ALIGNMENT);
    }

    void aligned_free(void* aligned)
    {
        _aligned_free(aligned);
    }

#elif defined(MANGO_PLATFORM_UNIX)

    void* aligned_malloc(size_t size)
    {
        return memalign(MANGO_MEMORY_ALIGNMENT, size);
    }

    void aligned_free(void* aligned)
    {
        free(aligned);
    }

#else

    // generic implementation

    void* aligned_malloc(size_t size)
    {
        void* block = std::malloc(size + (MANGO_MEMORY_ALIGNMENT - 1) + sizeof(void*));
        char* aligned = reinterpret_cast<char*>(block) + sizeof(void*);

        if (block) {
            aligned += MANGO_MEMORY_ALIGNMENT - (reinterpret_cast<ptrdiff_t>(aligned) & (MANGO_MEMORY_ALIGNMENT - 1));
            reinterpret_cast<void**>(aligned)[-1] = block;
        }
        else {
            aligned = NULL;
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
