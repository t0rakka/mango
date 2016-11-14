/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        std::vector<uint8> m_buffer;
        size_t m_offset;

    public:
        Buffer();
        Buffer(size_t size);
        Buffer(const uint8* address, size_t size);
        Buffer(const Memory& memory);
        ~Buffer();

        void resize(size_t size);

        // memory
        operator Memory () const;
		operator const uint8* () const;
		operator uint8* ();

        // stream
        uint64 size() const;
        uint64 offset() const;
        void seek(uint64 distance, SeekMode mode);
        void read(void* dest, size_t size);
        void write(const void* data, size_t size);
    };

} // namespace mango
