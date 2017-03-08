/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/buffer.hpp>
#include <mango/core/exception.hpp>

#define ID "Buffer: "

namespace mango {

    Buffer::Buffer()
        : m_offset(0)
    {
    }

    Buffer::Buffer(size_t _size)
        : m_buffer(_size)
        , m_offset(0)
    {
    }

    Buffer::Buffer(const uint8* address, size_t _size)
        : m_buffer(address, address + _size)
        , m_offset(0)
    {
    }

    Buffer::Buffer(const Memory& memory)
        : m_buffer(memory.address, memory.address + memory.size)
        , m_offset(0)
    {
    }

    Buffer::~Buffer()
    {
    }

    void Buffer::resize(size_t size)
    {
        m_buffer.resize(size);
        m_offset = size;
    }

    Buffer::operator Memory () const
    {
        uint8 *address = const_cast<uint8 *>(m_buffer.data());
        return Memory(address, m_buffer.size());
    }

	Buffer::operator uint8* ()
	{
		return &m_buffer[0];
	}

    uint64 Buffer::size() const
    {
        return m_buffer.size();
    }

    uint64 Buffer::offset() const
    {
        return m_offset;
    }

    void Buffer::seek(uint64 distance, SeekMode mode)
    {
        switch (mode)
        {
            case BEGIN:
                m_offset = static_cast<size_t>(distance);
                break;

            case CURRENT:
                m_offset += static_cast<size_t>(distance);
                break;

            case END:
                m_offset = static_cast<size_t>(m_buffer.size() - distance);
                break;
        }
    }

    void Buffer::read(void* dest, size_t size)
    {
        const size_t left = m_buffer.size() - m_offset;
        if (left < size) {
            size = left;
            MANGO_EXCEPTION(ID"Reading past end of buffer.");
        }
        std::memcpy(dest, &m_buffer[m_offset], size);
        m_offset += size;
    }

    void Buffer::write(const void* data, size_t size)
    {
        const char* source = reinterpret_cast<const char *>(data);
        const size_t left = m_buffer.size() - m_offset;
        const size_t right = size - left;

        if (left > 0) {
            // write into existing array
            size = std::min(size, left);
            std::memcpy(&m_buffer[m_offset], source, size);
            source += size;
            m_offset += size;
        }

        if (right > 0) {
            // append at end of existing array
            m_buffer.insert(m_buffer.end(), source, source + right);
            m_offset += right;
        }
    }

} // namespace mango
