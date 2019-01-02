/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <string>
#include "configure.hpp"
#include "memory.hpp"
#include "stream.hpp"

namespace mango
{

    class Buffer : public Stream
    {
    private:
        Memory m_memory;
        size_t m_capacity;
        size_t m_offset;

    public:
        Buffer();
        Buffer(size_t bytes);
        Buffer(const u8* address, size_t bytes);
        Buffer(Memory memory);
        ~Buffer();

        size_t capacity() const;
        void reserve(size_t bytes);
        void resize(size_t bytes);

        // memory
        u8* data() const;
        operator Memory () const;
		operator u8* () const;

        // stream
        u64 size() const;
        u64 offset() const;
        void seek(u64 distance, SeekMode mode);
        void read(void* dest, size_t bytes);
        void write(const void* data, size_t bytes);
    };

} // namespace mango
