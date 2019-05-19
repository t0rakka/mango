/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"
#include "endian.hpp"
#include "memory.hpp"

namespace mango {
namespace detail {

    // --------------------------------------------------------------
    // Pointer
    // --------------------------------------------------------------

    template <typename P>
    class Pointer
    {
    protected:
        P* p;

    public:
        explicit Pointer(const Pointer& pointer)
            : p(pointer.p)
        {
        }

        Pointer(void* address)
            : p(reinterpret_cast<P*>(address))
        {
        }

        Pointer() = default;
        ~Pointer() = default;

        const Pointer& operator = (void* address)
        {
            p = reinterpret_cast<P*>(address);
            return *this;
        }

        template <typename T>
        T* cast() const
        {
            return reinterpret_cast<T*>(p);
        }

        operator P* () const
        {
            return p;
        }

        P& operator * () const
        {
            return *p;
        }

        P* operator ++ ()
        {
            return ++p;
        }

        P* operator ++ (int)
        {
            return p++;
        }

        P* operator -- ()
        {
            return --p;
        }

        P* operator -- (int)
        {
            return p--;
        }

        P* operator += (size_t count)
        {
            p += count;
            return p;
        }

        P* operator -= (size_t count)
        {
            p -= count;
            return p;
        }
    };

    // --------------------------------------------------------------
    // SameEndianPointer
    // --------------------------------------------------------------

    template <typename P>
    class SameEndianPointer : public Pointer<P>
    {
    protected:
        using Pointer<P>::p;

    public:
        SameEndianPointer(void* address)
            : Pointer<P>(address)
        {
        }

        // read functions

        void read(u8* dest, size_t count)
        {
            std::memcpy(dest, p, count);
            p += count;
        }

        u8 read8()
        {
            return *p++;
        }

        u16 read16()
        {
            u16 value = uload16(p);
            p += 2;
            return value;
        }

        u32 read32()
        {
            u32 value = uload32(p);
            p += 4;
            return value;
        }

        u64 read64()
        {
            u64 value = uload64(p);
            p += 8;
            return value;
        }

        float16 read16f()
        {
            Half value;
            value.u = uload16(p);
            p += 2;
            return value;
        }

        float read32f()
        {
            Float value;
            value.u = uload32(p);
            p += 4;
            return value;
        }

        double read64f()
        {
            Double value;
            value.u = uload64(p);
            p += 8;
            return value;
        }

        // write functions

        void write(const u8* source, size_t count)
        {
            std::memcpy(p, source, count);
            p += count;
        }

        void write(Memory memory)
        {
            std::memcpy(p, memory.address, memory.size);
            p += memory.size;
        }

        void write8(u8 value)
        {
            *p++ = value;
        }

        void write16(u16 value)
        {
            ustore16(p, value);
            p += 2;
        }

        void write32(u32 value)
        {
            ustore32(p, value);
            p += 4;
        }

        void write64(u64 value)
        {
            ustore64(p, value);
            p += 8;
        }

        void write16f(Half value)
        {
            ustore16(p, value.u);
            p += 2;
        }

        void write32f(Float value)
        {
            ustore32(p, value.u);
            p += 4;
        }

        void write64f(Double value)
        {
            ustore64(p, value.u);
            p += 8;
        }
    };

    // --------------------------------------------------------------
    // SwapEndianPointer
    // --------------------------------------------------------------

    template <typename P>
    class SwapEndianPointer : public Pointer<P>
    {
    protected:
        using Pointer<P>::p;

    public:
        SwapEndianPointer(void* address)
            : Pointer<P>(address)
        {
        }

        // read functions

        void read(u8* dest, size_t count)
        {
            std::memcpy(dest, p, count);
            p += count;
        }

        u8 read8()
        {
            return *p++;
        }

        u16 read16()
        {
            u16 value = uload16swap(p);
            p += 2;
            return value;
        }

        u32 read32()
        {
            u32 value = uload32swap(p);
            p += 4;
            return value;
        }

        u64 read64()
        {
            u64 value = uload64swap(p);
            p += 8;
            return value;
        }

        float16 read16f()
        {
            Half value;
            value.u = uload16swap(p);
            p += 2;
            return value;
        }

        float read32f()
        {
            Float value;
            value.u = uload32swap(p);
            p += 4;
            return value;
        }

        double read64f()
        {
            Double value;
            value.u = uload64swap(p);
            p += 8;
            return value;
        }

        // write functions

        void write(const u8* source, size_t count)
        {
            std::memcpy(p, source, count);
            p += count;
        }

        void write(Memory memory)
        {
            std::memcpy(p, memory.address, memory.size);
            p += memory.size;
        }

        void write8(u8 value)
        {
            *p++ = value;
        }

        void write16(u16 value)
        {
            ustore16swap(p, value);
            p += 2;
        }

        void write32(u32 value)
        {
            ustore32swap(p, value);
            p += 4;
        }

        void write64(u64 value)
        {
            ustore64swap(p, value);
            p += 8;
        }

        void write16f(Half value)
        {
            ustore16swap(p, value.u);
            p += 2;
        }

        void write32f(Float value)
        {
            ustore32swap(p, value.u);
            p += 4;
        }

        void write64f(Double value)
        {
            ustore64swap(p, value.u);
            p += 8;
        }
    };

} // namespace detail

    // --------------------------------------------------------------
    // pointer types
    // --------------------------------------------------------------

    using Pointer = detail::Pointer<u8>;
    using ConstPointer = detail::Pointer<const u8>;

    using SameEndianPointer = detail::SameEndianPointer<u8>;
    using SameEndianConstPointer = detail::SameEndianPointer<const u8>;

    using SwapEndianPointer = detail::SwapEndianPointer<u8>;
    using SwapEndianConstPointer = detail::SwapEndianPointer<const u8>;

#ifdef MANGO_LITTLE_ENDIAN

    using LittleEndianPointer = SameEndianPointer;
    using LittleEndianConstPointer = SameEndianConstPointer;

    using BigEndianPointer = SwapEndianPointer;
    using BigEndianConstPointer = SwapEndianConstPointer;

#else

    using LittleEndianPointer = SwapEndianPointer;
    using LittleEndianConstPointer = SwapEndianConstPointer;

    using BigEndianPointer = SameEndianPointer;
    using BigEndianConstPointer = SameEndianConstPointer;

#endif

} // namespace mango
