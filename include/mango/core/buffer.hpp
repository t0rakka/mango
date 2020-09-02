/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        u64 m_capacity;
        Alignment m_alignment;

    public:
        explicit Buffer(Alignment alignment = Alignment());
        explicit Buffer(u64 bytes, Alignment alignment = Alignment());
        explicit Buffer(u64 bytes, u8 value, Alignment alignment = Alignment());
        explicit Buffer(const u8* source, u64 bytes, Alignment alignment = Alignment());
        explicit Buffer(ConstMemory memory, Alignment alignment = Alignment());
        explicit Buffer(Stream& stream, Alignment alignment = Alignment());
        ~Buffer();

        operator ConstMemory () const;
        operator Memory () const;
        operator u8* () const;
        u8* data() const;

        u64 size() const;
        u64 capacity() const;

        void reset();
        void resize(u64 bytes);
        void reserve(u64 bytes);
        void append(const void* source, u64 bytes);

    private:
        u8* allocate(u64 bytes, Alignment alignment) const;
        void free(u8* ptr) const;
    };

    class MemoryStream : public Stream
    {
    private:
        Buffer m_buffer;
        u64 m_offset;

    public:
        MemoryStream();
        MemoryStream(const u8* source, u64 bytes);
        MemoryStream(ConstMemory memory);
        ~MemoryStream();

        operator ConstMemory () const;
        operator Memory () const;
        operator u8* () const;
        u8* data() const;

        u64 size() const override;
        u64 offset() const override;
        void seek(s64 distance, SeekMode mode) override;
        void read(void* dest, u64 bytes) override;
        void write(const void* source, u64 bytes) override;

        void write(ConstMemory memory)
        {
            Stream::write(memory);
        }
    };

} // namespace mango
