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

} // namespace mango
