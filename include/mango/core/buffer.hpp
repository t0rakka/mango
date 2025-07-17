/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
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

    class MemoryStream : public Stream
    {
    private:
        Buffer m_buffer;
        s64 m_offset;

    public:
        MemoryStream();
        MemoryStream(const u8* source, u64 bytes);
        MemoryStream(ConstMemory memory);
        ~MemoryStream();

        operator ConstMemory () const;
        operator Memory () const;
        operator u8* () const;
        u8* data() const;

        s64 size() const override;
        s64 offset() const override;
        s64 seek(s64 distance, SeekMode mode) override;
        s64 read(void* dest, u64 bytes) override;
        s64 write(const void* source, u64 bytes) override;
    };

    class ConstMemoryStream : public Stream
    {
    protected:
        ConstMemory m_memory;
        s64 m_offset;

    public:
        ConstMemoryStream(ConstMemory memory);
        ~ConstMemoryStream();

        s64 size() const override;
        s64 offset() const override;
        s64 seek(s64 distance, SeekMode mode) override;
        s64 read(void* dest, u64 bytes) override;
        s64 write(const void* data, u64 bytes) override;
    };

} // namespace mango
