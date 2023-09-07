/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cassert>
#include <mango/core/bits.hpp>
#include <mango/core/memory.hpp>

namespace mango
{

    // -----------------------------------------------------------------------
    // SharedMemory
    // -----------------------------------------------------------------------

    SharedMemory::SharedMemory(size_t bytes)
    {
        u8* address = new u8[bytes];
        m_memory = Memory(address, bytes);
        m_ptr = std::shared_ptr<u8>(address, std::default_delete<u8[]>());
    }

    SharedMemory::SharedMemory(u8* address, size_t bytes)
        : m_memory(address, bytes)
        , m_ptr(address, std::default_delete<u8[]>())
    {
    }

    // -----------------------------------------------------------------------
    // aligned malloc/free
    // -----------------------------------------------------------------------

#if defined(MANGO_COMPILER_MICROSOFT)

    void* aligned_malloc(size_t bytes, size_t alignment)
    {
        return _aligned_malloc(bytes, alignment);
    }

    void aligned_free(void* aligned)
    {
        _aligned_free(aligned);
    }

#elif defined(MANGO_PLATFORM_LINUX)

    void* aligned_malloc(size_t bytes, size_t alignment)
    {
        return memalign(alignment, bytes);
    }

    void aligned_free(void* aligned)
    {
        free(aligned);
    }

#else

    // generic implementation

    void* aligned_malloc(size_t bytes, size_t alignment)
    {
        const size_t mask = alignment - 1;
        void* block = std::malloc(bytes + mask + sizeof(void*));
        char* aligned = reinterpret_cast<char*>(block) + sizeof(void*);

        if (block)
        {
            aligned += alignment - (reinterpret_cast<ptrdiff_t>(aligned) & mask);
            reinterpret_cast<void**>(aligned)[-1] = block;
        }
        else
        {
            aligned = nullptr;
        }

        return aligned;
    }

    void aligned_free(void* aligned)
    {
        if (aligned)
        {
            void* block = reinterpret_cast<void**>(aligned)[-1];
            std::free(block);
        }
    }

#endif

} // namespace mango
