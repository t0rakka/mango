/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/configure.hpp>
#include <mango/core/endian.hpp>
#include <mango/core/memory.hpp>
#include <mango/core/half.hpp>
#include <mango/core/exception.hpp>

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

        virtual u64 size() const = 0;
        virtual u64 offset() const = 0;
        virtual void seek(s64 distance, SeekMode mode) = 0;
        virtual void read(void* dest, u64 size) = 0;
        virtual void write(const void* data, u64 size) = 0;

        void write(ConstMemory memory)
        {
            write(memory.address, memory.size);
        }
    };

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

        operator Stream& ()
        {
            return s;
        }

        u64 size() const
        {
            return s.size();
        }

        u64 offset() const
        {
            return s.offset();
        }

        void seek(s64 distance, Stream::SeekMode mode)
        {
            s.seek(distance, mode);
        }

        // read functions

        void read(void* dest, u64 size)
        {
            s.read(dest, size);
        }

        u8 read8()
        {
            u8 value;
            s.read(&value, sizeof(u8));
            return value;
        }

        u16 read16()
        {
            u16 value;
            s.read(&value, sizeof(u16));
            return value;
        }

        u32 read32()
        {
            u32 value;
            s.read(&value, sizeof(u32));
            return value;
        }

        u64 read64()
        {
            u64 value;
            s.read(&value, sizeof(u64));
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

        // write functions

        void write(const void* data, u64 size)
        {
            s.write(data, size);
        }

        void write(ConstMemory memory)
        {
            s.write(memory);
        }

        template <typename T>
        void write8(T value)
        {
            u8 temp = u8(value);
            s.write(&temp, sizeof(temp));
        }

        template <typename T>
        void write16(T value)
        {
            u16 temp = u16(value);
            s.write(&temp, sizeof(temp));
        }

        template <typename T>
        void write32(T value)
        {
            u32 temp = u32(value);
            s.write(&temp, sizeof(temp));
        }

        template <typename T>
        void write64(T value)
        {
            u64 temp = u64(value);
            s.write(&temp, sizeof(temp));
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

        operator Stream& ()
        {
            return s;
        }

        u64 size() const
        {
            return s.size();
        }

        u64 offset() const
        {
            return s.offset();
        }

        void seek(s64 distance, Stream::SeekMode mode)
        {
            s.seek(distance, mode);
        }

        // read functions

        void read(void* dest, u64 size)
        {
            s.read(dest, size);
        }

        u8 read8()
        {
            u8 value;
            s.read(&value, sizeof(u8));
            return value;
        }

        u16 read16()
        {
            u16 value;
            s.read(&value, sizeof(u16));
            value = byteswap(value);
            return value;
        }

        u32 read32()
        {
            u32 value;
            s.read(&value, sizeof(u32));
            value = byteswap(value);
            return value;
        }

        u64 read64()
        {
            u64 value;
            s.read(&value, sizeof(u64));
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

        // write functions

        void write(const void* data, u64 size)
        {
            s.write(data, size);
        }

        void write(ConstMemory memory)
        {
            s.write(memory);
        }

        template <typename T>
        void write8(T value)
        {
            u8 temp = u8(value);
            s.write(&temp, sizeof(temp));
        }

        template <typename T>
        void write16(T value)
        {
            u16 temp = u16(value);
            temp = byteswap(temp);
            s.write(&temp, sizeof(temp));
        }

        template <typename T>
        void write32(T value)
        {
            u32 temp = u32(value);
            temp = byteswap(temp);
            s.write(&temp, sizeof(temp));
        }

        template <typename T>
        void write64(T value)
        {
            u64 temp = u64(value);
            temp = byteswap(temp);
            s.write(&temp, sizeof(temp));
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
    // Little / BigEndianStream
    // --------------------------------------------------------------

#ifdef MANGO_LITTLE_ENDIAN

    using LittleEndianStream = SameEndianStream;
    using BigEndianStream = SwapEndianStream;

#else

    using LittleEndianStream = SwapEndianStream;
    using BigEndianStream = SameEndianStream;

#endif

} // namespace mango
