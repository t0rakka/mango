/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"
#include "endian.hpp"
#include "memory.hpp"
#include "object.hpp"
#include "half.hpp"

namespace mango
{

    class Stream : protected NonCopyable
    {
    public:
        enum OpenMode
        {
            READ,
            WRITE
        };

        enum SeekMode
        {
            BEGIN,
            CURRENT,
            END
        };

        Stream() = default;
        virtual ~Stream() = default;

        virtual uint64 size() const = 0;
        virtual uint64 offset() const = 0;
        virtual void seek(uint64 distance, SeekMode mode) = 0;
        virtual void read(void* dest, size_t size) = 0;
        virtual void write(const void* data, size_t size) = 0;

        void write(Memory memory)
        {
            write(memory.address, memory.size);
        }
    };

    namespace detail
    {

        // --------------------------------------------------------------
        // SameEndianStream
        // --------------------------------------------------------------

        class SameEndianStream
        {
        private:
            Stream& s;

        public:
            SameEndianStream(Stream& stream)
                : s(stream)
            {
            }

            uint64 size() const
            {
                return s.size();
            }

            uint64 offset() const
            {
                return s.offset();
            }

            void seek(uint64 distance, Stream::SeekMode mode)
            {
                s.seek(distance, mode);
            }

            void read(void* dest, size_t size)
            {
                s.read(dest, size);
            }

            uint8 read8()
            {
                uint8 value;
                s.read(&value, sizeof(uint8));
                return value;
            }

            uint16 read16()
            {
                uint16 value;
                s.read(&value, sizeof(uint16));
                return value;
            }

            uint32 read32()
            {
                uint32 value;
                s.read(&value, sizeof(uint32));
                return value;
            }

            uint64 read64()
            {
                uint64 value;
                s.read(&value, sizeof(uint64));
                return value;
            }

            float16 read16f()
            {
                Half value;
                value.u = read16();
                return value;
            }

            float read32f()
            {
                Float value;
                value.u = read32();
                return value;
            }

            double read64f()
            {
                Double value;
                value.u = read64();
                return value;
            }

            void write(const void* data, size_t size)
            {
                s.write(data, size);
            }

            void write(Memory memory)
            {
                s.write(memory);
            }

            void write8(uint8 value)
            {
                s.write(&value, sizeof(uint8));

            }

            void write16(uint16 value)
            {
                s.write(&value, sizeof(uint16));
            }

            void write32(uint32 value)
            {
                s.write(&value, sizeof(uint32));
            }

            void write64(uint64 value)
            {
                s.write(&value, sizeof(uint64));
            }

            void write16f(Half value)
            {
                write16(value.u);
            }

            void write32f(Float value)
            {
                write32(value.u);
            }

            void write64f(Double value)
            {
                write64(value.u);
            }
        };

        // --------------------------------------------------------------
        // SwapEndianStream
        // --------------------------------------------------------------

        class SwapEndianStream
        {
        private:
            Stream& s;

        public:
            SwapEndianStream(Stream& stream)
                : s(stream)
            {
            }

            uint64 size() const
            {
                return s.size();
            }

            uint64 offset() const
            {
                return s.offset();
            }

            void seek(uint64 distance, Stream::SeekMode mode)
            {
                s.seek(distance, mode);
            }

            void read(void* dest, size_t size)
            {
                s.read(dest, size);
            }

            uint8 read8()
            {
                uint8 value;
                s.read(&value, sizeof(uint8));
                return value;
            }

            uint16 read16()
            {
                uint16 value;
                s.read(&value, sizeof(uint16));
                value = byteswap(value);
                return value;
            }

            uint32 read32()
            {
                uint32 value;
                s.read(&value, sizeof(uint32));
                value = byteswap(value);
                return value;
            }

            uint64 read64()
            {
                uint64 value;
                s.read(&value, sizeof(uint64));
                value = byteswap(value);
                return value;
            }

            float16 read16f()
            {
                Half value;
                value.u = read16();
                return value;
            }

            float read32f()
            {
                Float value;
                value.u = read32();
                return value;
            }

            double read64f()
            {
                Double value;
                value.u = read64();
                return value;
            }

            void write(const void* data, size_t size)
            {
                s.write(data, size);
            }

            void write(Memory memory)
            {
                s.write(memory);
            }

            void write8(uint8 value)
            {
                s.write(&value, 1);
            }

            void write16(uint16 value)
            {
                value = byteswap(value);
                s.write(&value, 2);
            }

            void write32(uint32 value)
            {
                value = byteswap(value);
                s.write(&value, 4);
            }

            void write64(uint64 value)
            {
                value = byteswap(value);
                s.write(&value, 8);
            }

            void write16f(Half value)
            {
                write16(value.u);
            }

            void write32f(Float value)
            {
                write32(value.u);
            }

            void write64f(Double value)
            {
                write64(value.u);
            }
        };

    } // namespace detail

    // --------------------------------------------------------------
    // EndianStream
    // --------------------------------------------------------------

#ifdef MANGO_LITTLE_ENDIAN

    using LittleEndianStream = detail::SameEndianStream;
    using BigEndianStream = detail::SwapEndianStream;

#else

    using LittleEndianStream = detail::SwapEndianStream;
    using BigEndianStream = detail::SameEndianStream;

#endif

} // namespace mango
