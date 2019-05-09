/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstddef>
#include "configure.hpp"
#include "memory.hpp"
#include "stream.hpp"

namespace mango
{

    class Buffer : public NonCopyable
    {
    private:
        Memory m_memory;
        size_t m_capacity;

    public:
        Buffer();
        Buffer(size_t bytes);
        Buffer(const u8* source, size_t bytes);
        Buffer(Memory memory);
        ~Buffer();

        operator Memory () const;
        operator u8* () const;
        u8* data() const;

        size_t size() const;
        size_t capacity() const;

        void resize(size_t bytes);
        void reserve(size_t bytes);
        void append(const void* source, size_t bytes);
    };

    class MemoryStream : public Stream
    {
    private:
        Buffer m_buffer;
        size_t m_offset;

    public:
        MemoryStream();
        MemoryStream(const u8* source, size_t bytes);
        MemoryStream(Memory memory);
        ~MemoryStream();

        operator Memory () const;
        operator u8* () const;
        u8* data() const;

        u64 size() const;
        u64 offset() const;
        void seek(u64 distance, SeekMode mode);
        void read(void* dest, size_t bytes);
        void write(const void* source, size_t bytes);
    };

} // namespace mango
