/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"
#include "endian.hpp"

namespace mango {

    // --------------------------------------------------------------
    // Pointer
    // --------------------------------------------------------------

    class Pointer
    {
    protected:
        uint8* p;

    public:
        explicit Pointer(const Pointer& pointer)
            : p(pointer.p)
        {
        }

        explicit Pointer(uint8* address)
            : p(address)
        {
        }

        Pointer() = default;
        ~Pointer() = default;

        const Pointer& operator = (uint8* address)
        {
            p = address;
            return *this;
        }

        template <typename T>
        T* cast() const
        {
            return reinterpret_cast<T*>(p);
        }

        operator uint8* () const
        {
            return p;
        }

        uint8& operator * () const
        {
            return *p;
        }

        uint8* operator ++ ()
        {
            return ++p;
        }

        uint8* operator ++ (int)
        {
            return p++;
        }

        uint8* operator -- ()
        {
            return --p;
        }

        uint8* operator -- (int)
        {
            return p--;
        }

        uint8* operator += (size_t count)
        {
            p += count;
            return p;
        }

        uint8* operator -= (size_t count)
        {
            p -= count;
            return p;
        }
    };

    static inline ptrdiff_t operator - (const Pointer& a, const Pointer& b)
    {
        return static_cast<const uint8 *>(a) - static_cast<const uint8 *>(b);
    }

    // --------------------------------------------------------------
    // LittleEndianPointer
    // --------------------------------------------------------------

    class LittleEndianPointer : public Pointer
    {
    protected:
        using Pointer::p;

    public:
        LittleEndianPointer(uint8* address)
            : Pointer(address)
        {
        }

        // read methods

        void read(uint8* dest, size_t count)
        {
            std::memcpy(dest, p, count);
            p += count;
        }

        uint8 read8()
        {
            return *p++;
        }

        uint16 read16()
        {
            uint16 value = uload16le(p);
            p += 2;
            return value;
        }

        uint32 read32()
        {
            uint32 value = uload32le(p);
            p += 4;
            return value;
        }

        uint64 read64()
        {
            uint64 value = uload64le(p);
            p += 8;
            return value;
        }

        float16 read16f()
        {
            Half value;
            value.u = uload16le(p);
            p += 2;
            return value;
        }

        float read32f()
        {
            Float value;
            value.u = uload32le(p);
            p += 4;
            return value;
        }

        double read64f()
        {
            Double value;
            value.u = uload64le(p);
            p += 8;
            return value;
        }

        // write methods

        void write(const uint8* source, size_t count)
        {
            std::memcpy(p, source, count);
            p += count;
        }

        void write8(uint8 value)
        {
            *p++ = value;
        }

        void write16(uint16 value)
        {
            ustore16le(p, value);
            p += 2;
        }

        void write32(uint32 value)
        {
            ustore32le(p, value);
            p += 4;
        }

        void write64(uint64 value)
        {
            ustore64le(p, value);
            p += 8;
        }

        void write16f(Half value)
        {
            ustore16le(p, value.u);
            p += 2;
        }

        void write32f(Float value)
        {
            ustore32le(p, value.u);
            p += 4;
        }

        void write64f(Double value)
        {
            ustore64le(p, value.u);
            p += 8;
        }
    };

    // --------------------------------------------------------------
    // BigEndianPointer
    // --------------------------------------------------------------

    class BigEndianPointer : public Pointer
    {
    protected:
        using Pointer::p;

    public:
        BigEndianPointer(uint8* address)
            : Pointer(address)
        {
        }

        // read methods

        void read(uint8* dest, size_t count)
        {
            std::memcpy(dest, p, count);
            p += count;
        }

        uint8 read8()
        {
            return *p++;
        }

        uint16 read16()
        {
            uint16 value = uload16be(p);
            p += 2;
            return value;
        }

        uint32 read32()
        {
            uint32 value = uload32be(p);
            p += 4;
            return value;
        }

        uint64 read64()
        {
            uint64 value = uload64be(p);
            p += 8;
            return value;
        }

        float16 read16f()
        {
            Half value;
            value.u = uload16be(p);
            p += 2;
            return value;
        }

        float read32f()
        {
            Float value;
            value.u = uload32be(p);
            p += 4;
            return value;
        }

        double read64f()
        {
            Double value;
            value.u = uload64be(p);
            p += 8;
            return value;
        }

        // write methods

        void write(const uint8* source, size_t count)
        {
            std::memcpy(p, source, count);
            p += count;
        }

        void write8(uint8 value)
        {
            *p++ = value;
        }

        void write16(uint16 value)
        {
            ustore16be(p, value);
            p += 2;
        }

        void write32(uint32 value)
        {
            ustore32be(p, value);
            p += 4;
        }

        void write64(uint64 value)
        {
            ustore64be(p, value);
            p += 8;
        }

        void write16f(Half value)
        {
            ustore16be(p, value.u);
            p += 2;
        }

        void write32f(Float value)
        {
            ustore32be(p, value.u);
            p += 4;
        }

        void write64f(Double value)
        {
            ustore64be(p, value.u);
            p += 8;
        }
    };

} // namespace mango
