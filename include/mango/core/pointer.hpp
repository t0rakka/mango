/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"
#include "endian.hpp"

namespace mango
{

    namespace internal
    {

        // --------------------------------------------------------------
        // Pointer
        // --------------------------------------------------------------

        template <typename Type>
        class Pointer
        {
        protected:
            Type* p;

        public:
            Pointer(const Pointer& pointer)
            : p(pointer.p)
            {
            }

            Pointer(Type* address)
            : p(address)
            {
            }

            ~Pointer()
            {
            }

            const Pointer& operator = (Type* address)
            {
                p = address;
                return *this;
            }

            operator Type* () const
            {
                return p;
            }

            Type& operator * () const
            {
                return *p;
            }

            Type* operator ++ ()
            {
                return ++p;
            }

            Type* operator ++ (int)
            {
                return p++;
            }

            Type* operator -- ()
            {
                return --p;
            }

            Type* operator -- (int)
            {
                return p--;
            }

            Type* operator += (size_t count)
            {
                p += count;
                return p;
            }

            Type* operator -= (size_t count)
            {
                p -= count;
                return p;
            }
        };

        template <typename Type>
        ptrdiff_t operator - (const Pointer<Type>& a, const Pointer<Type>& b)
        {
            return static_cast<const Type*>(a) - static_cast<const Type*>(b);
        }

        // --------------------------------------------------------------
        // LittleEndianRead
        // --------------------------------------------------------------

        template <typename Type>
        class LittleEndianRead : public Pointer<Type>
        {
        protected:
            using Pointer<Type>::p;

            LittleEndianRead(Type* address)
            : Pointer<Type>(address)
            {
            }

        public:
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

            half read16f()
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
        };

        // --------------------------------------------------------------
        // LittleEndianWrite
        // --------------------------------------------------------------

        template <typename Type>
        class LittleEndianWrite : public LittleEndianRead<Type>
        {
        protected:
            using Pointer<Type>::p;

            LittleEndianWrite(Type* address)
            : LittleEndianRead<Type>(address)
            {
            }

        public:
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
        // BigEndianRead
        // --------------------------------------------------------------

        template <typename Type>
        class BigEndianRead : public Pointer<Type>
        {
        protected:
            using Pointer<Type>::p;

            BigEndianRead(Type* address)
            : Pointer<Type>(address)
            {
            }

        public:
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

            half read16f()
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
        };

        // --------------------------------------------------------------
        // BigEndianWrite
        // --------------------------------------------------------------

        template <typename Type>
        class BigEndianWrite : public BigEndianRead<Type>
        {
        protected:
            using Pointer<Type>::p;

            BigEndianWrite(Type* address)
            : BigEndianRead<Type>(address)
            {
            }

        public:
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

    } // namespace internal

    // --------------------------------------------------------------
    // LittleEndianConstPointer
    // --------------------------------------------------------------

    class LittleEndianConstPointer : public internal::LittleEndianRead<const uint8>
    {
    public:
        LittleEndianConstPointer()
        : LittleEndianRead(NULL)
        {
        }

        LittleEndianConstPointer(const uint8* address)
        : LittleEndianRead(address)
        {
        }

        ~LittleEndianConstPointer()
        {
        }
    };

    // --------------------------------------------------------------
    // LittleEndianPointer
    // --------------------------------------------------------------

    class LittleEndianPointer : public internal::LittleEndianWrite<uint8>
    {
    public:
        LittleEndianPointer()
        : LittleEndianWrite(NULL)
        {
        }

        LittleEndianPointer(uint8* address)
        : LittleEndianWrite(address)
        {
        }

        ~LittleEndianPointer()
        {
        }
    };

    // --------------------------------------------------------------
    // BigEndianConstPointer
    // --------------------------------------------------------------

    class BigEndianConstPointer : public internal::BigEndianRead<const uint8>
    {
    public:
        BigEndianConstPointer()
        : BigEndianRead(NULL)
        {
        }

        BigEndianConstPointer(const uint8* address)
        : BigEndianRead(address)
        {
        }

        ~BigEndianConstPointer()
        {
        }
    };

    // --------------------------------------------------------------
    // BigEndianPointer
    // --------------------------------------------------------------

    class BigEndianPointer : public internal::BigEndianWrite<uint8>
    {
    public:
        BigEndianPointer()
        : BigEndianWrite(NULL)
        {
        }

        BigEndianPointer(uint8* address)
        : BigEndianWrite(address)
        {
        }

        ~BigEndianPointer()
        {
        }
    };

} // namespace mango
