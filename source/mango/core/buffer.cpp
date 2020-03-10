/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/buffer.hpp>
#include <mango/core/exception.hpp>

namespace mango {

    // ----------------------------------------------------------------------------
    // Buffer
    // ----------------------------------------------------------------------------

    Buffer::Buffer(Alignment alignment)
        : m_memory()
        , m_capacity(0)
        , m_alignment(alignment)
    {
    }

    Buffer::Buffer(size_t bytes, Alignment alignment)
        : m_memory(allocate(bytes, alignment), bytes)
        , m_capacity(bytes)
        , m_alignment(alignment)
    {
    }

    Buffer::Buffer(size_t bytes, u8 value, Alignment alignment)
        : m_memory(allocate(bytes, alignment), bytes)
        , m_capacity(bytes)
        , m_alignment(alignment)
    {
        std::memset(m_memory.address, value, bytes);
    }

    Buffer::Buffer(const u8* source, size_t bytes, Alignment alignment)
        : m_memory(allocate(bytes, alignment), bytes)
        , m_capacity(bytes)
        , m_alignment(alignment)
    {
        std::memcpy(m_memory.address, source, bytes);
    }

    Buffer::Buffer(ConstMemory memory, Alignment alignment)
        : m_memory(allocate(memory.size, alignment), memory.size)
        , m_capacity(memory.size)
        , m_alignment(alignment)
    {
        std::memcpy(m_memory.address, memory.address, memory.size);
    }

    Buffer::Buffer(Stream& stream, Alignment alignment)
        : m_memory(allocate(stream.size(), alignment), stream.size())
        , m_capacity(m_memory.size)
        , m_alignment(alignment)
    {
        stream.seek(0, Stream::BEGIN);
        stream.read(m_memory.address, m_memory.size);
    }

    Buffer::~Buffer()
    {
        free(m_memory.address);
    }

    Buffer::operator ConstMemory () const
    {
        return m_memory;
    }

    Buffer::operator Memory () const
    {
        return m_memory;
    }

    Buffer::operator u8* () const
    {
        return m_memory.address;
    }

    u8* Buffer::data() const
    {
        return m_memory.address;
    }

    size_t Buffer::size() const
    {
        return m_memory.size;
    }

    size_t Buffer::capacity() const
    {
        return m_capacity;        
    }

    void Buffer::reset()
    {
        free(m_memory.address);
        m_memory = Memory();
        m_capacity = 0;
    }

    void Buffer::resize(size_t bytes)
    {
        reserve(bytes);
        m_memory.size = bytes;
    }

    void Buffer::reserve(size_t bytes)
    {
        if (bytes > m_capacity)
        {
            u8* storage = allocate(bytes, m_alignment);
            if (m_memory.address)
            {
                std::memcpy(storage, m_memory.address, m_memory.size);
                free(m_memory.address);
            }
            m_memory.address = storage;
            m_capacity = bytes;
        }
    }

    void Buffer::append(const void* source, size_t bytes)
    {
        size_t required = m_memory.size + bytes;
        if (required > m_capacity)
        {
            // grow 1.4x the required capacity
            reserve((required * 7) / 5);
        }

        std::memcpy(m_memory.address + m_memory.size, source, bytes);
        m_memory.size += bytes;
    }

    u8* Buffer::allocate(size_t bytes, Alignment alignment) const
    {
        void* ptr = aligned_malloc(bytes, alignment);
        return reinterpret_cast<u8*>(ptr);
    }

    void Buffer::free(u8* ptr) const
    {
        aligned_free(ptr);
    }

    // ----------------------------------------------------------------------------
    // MemoryStream
    // ----------------------------------------------------------------------------

    MemoryStream::MemoryStream()
        : m_buffer()
        , m_offset(0)
    {
    }

    MemoryStream::MemoryStream(const u8* source, size_t bytes)
        : m_buffer(source, bytes)
        , m_offset(bytes)
    {
    }

    MemoryStream::MemoryStream(ConstMemory memory)
        : m_buffer(memory)
        , m_offset(memory.size)
    {
    }

    MemoryStream::~MemoryStream()
    {
    }

    MemoryStream::operator ConstMemory () const
    {
        return m_buffer;
    }

    MemoryStream::operator Memory () const
    {
        return m_buffer;
    }

    MemoryStream::operator u8* () const
    {
        return m_buffer;
    }

    u8* MemoryStream::data() const
    {
        return m_buffer.data();
    }

    u64 MemoryStream::size() const
    {
        return u64(m_buffer.size());
    }

    u64 MemoryStream::offset() const
    {
        return m_offset;
    }

    void MemoryStream::seek(u64 distance, SeekMode mode)
    {
        const u64 size = u64(m_buffer.size());
        switch (mode)
        {
            case BEGIN:
                m_offset = size_t(std::min(size, distance));
                break;

            case CURRENT:
                m_offset = size_t(std::min(size, m_offset + distance));
                break;

            case END:
                m_offset = size_t(distance > size ? 0 : size - distance);
                break;
        }
    }

    void MemoryStream::read(void* dest, size_t bytes)
    {
        const size_t left = m_buffer.size() - m_offset;
        if (left < bytes)
        {
            MANGO_EXCEPTION("[MemoryStream] Reading past end of buffer.");
        }

        std::memcpy(dest, m_buffer.data() + m_offset, bytes);
        m_offset += bytes;
    }

    void MemoryStream::write(const void* source, size_t bytes)
    {
        const size_t left = std::min(bytes, m_buffer.size() - m_offset);
        const size_t right = bytes - left;
        std::memcpy(m_buffer.data() + m_offset, source, left);
        if (right > 0)
        {
            const u8* src = reinterpret_cast<const u8*>(source);
            m_buffer.append(src + left, right);
        }
        m_offset += bytes;
    }

} // namespace mango
