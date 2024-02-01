/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstddef>
#include <vector>
#include <mango/core/configure.hpp>
#include <mango/core/memory.hpp>
#include <mango/core/stream.hpp>

namespace mango
{

    class Buffer : public NonCopyable
    {
    private:
        Memory m_memory;
        size_t m_capacity;

    public:
        explicit Buffer();
        explicit Buffer(size_t bytes);
        explicit Buffer(size_t bytes, u8 value);
        explicit Buffer(const void* source, size_t bytes);
        explicit Buffer(ConstMemory memory);
        explicit Buffer(Stream& stream);
        ~Buffer();

        operator ConstMemory () const;
        operator Memory () const;
        operator u8* () const;
        u8* data() const;

        size_t size() const;
        size_t capacity() const;

        void reset();
        void reset(size_t bytes);
        void reset(size_t bytes, u8 value);
        void resize(size_t bytes);
        void reserve(size_t bytes);
        u8* append(size_t bytes);
        u8* append(size_t bytes, u8 value);
        void append(const void* source, size_t bytes);
        void append(ConstMemory memory);

        template <typename T>
        void append(const std::vector<T>& v)
        {
            append(v.data(), v.size() * sizeof(T));
        }

        [[nodiscard]] Memory acquire();
        static void release(Memory memory);

    private:
        u8* allocate(size_t bytes) const;
    };

    class BufferStream : public Stream
    {
    private:
        Buffer m_buffer;
        u64 m_offset;

    public:
        BufferStream();
        BufferStream(const u8* source, u64 bytes);
        BufferStream(ConstMemory memory);
        ~BufferStream();

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

    using MemoryStream = BufferStream;

} // namespace mango
