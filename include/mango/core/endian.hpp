/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cassert>
#include "configure.hpp"
#include "bits.hpp"

namespace mango
{

    // --------------------------------------------------------------
    // unaligned load/store
    // --------------------------------------------------------------

#ifdef MANGO_UNALIGNED_MEMORY

    static inline uint16 uload16(const uint8* p)
    {
        uint16 value = reinterpret_cast<const uint16 *>(p)[0];
        return value;
    }

    static inline uint32 uload32(const uint8* p)
    {
        uint32 value = reinterpret_cast<const uint32 *>(p)[0];
        return value;
    }

    static inline uint64 uload64(const uint8* p)
    {
        uint64 value = reinterpret_cast<const uint64 *>(p)[0];
        return value;
    }

    static inline void ustore16(uint8* p, uint16 value)
    {
        reinterpret_cast<uint16 *>(p)[0] = value;
    }

    static inline void ustore32(uint8* p, uint32 value)
    {
        reinterpret_cast<uint32 *>(p)[0] = value;
    }

#else

    // Platform does not support unaligned load/store

    static inline uint16 uload16(const uint8* p)
    {
        uint16 value;
        std::memcpy(&value, p, sizeof(uint16));
        return value;
    }

    static inline uint32 uload32(const uint8* p)
    {
        uint32 value;
        std::memcpy(&value, p, sizeof(uint32));
        return value;
    }

    static inline uint64 uload64(const uint8* p)
    {
        uint64 value;
        std::memcpy(&value, p, sizeof(uint64));
        return value;
    }

    static inline void ustore16(uint8* p, uint16 value)
    {
        std::memcpy(p, &value, sizeof(uint16));
    }

    static inline void ustore32(uint8* p, uint32 value)
    {
        std::memcpy(p, &value, sizeof(uint32));
    }

#endif

    static inline void ustore64(uint8* p, uint64 value)
    {
        std::memcpy(p, &value, sizeof(uint64));
    }

    // --------------------------------------------------------------
	// endian load/store
    // --------------------------------------------------------------

#ifdef MANGO_LITTLE_ENDIAN

    #define uload16le(p)  mango::uload16(p)
    #define uload32le(p)  mango::uload32(p)
    #define uload64le(p)  mango::uload64(p)
    #define uload16be(p)  mango::byteswap16(mango::uload16(p))
    #define uload32be(p)  mango::byteswap32(mango::uload32(p))
    #define uload64be(p)  mango::byteswap64(mango::uload64(p))

    #define ustore16le(p,v)  mango::ustore16(p, v)
    #define ustore32le(p,v)  mango::ustore32(p, v)
    #define ustore64le(p,v)  mango::ustore64(p, v)
    #define ustore16be(p,v)  mango::ustore16(p, mango::byteswap16(v))
    #define ustore32be(p,v)  mango::ustore32(p, mango::byteswap32(v))
    #define ustore64be(p,v)  mango::ustore64(p, mango::byteswap64(v))

#else

    #define uload16le(p)  mango::byteswap16(mango::uload16(p))
    #define uload32le(p)  mango::byteswap32(mango::uload32(p))
    #define uload64le(p)  mango::byteswap64(mango::uload64(p))
    #define uload16be(p)  mango::uload16(p)
    #define uload32be(p)  mango::uload32(p)
    #define uload64be(p)  mango::uload64(p)

    #define ustore16le(p,v)  mango::ustore16(p, mango::byteswap16(v))
    #define ustore32le(p,v)  mango::ustore32(p, mango::byteswap32(v))
    #define ustore64le(p,v)  mango::ustore64(p, mango::byteswap64(v))
    #define ustore16be(p,v)  mango::ustore16(p, v)
    #define ustore32be(p,v)  mango::ustore32(p, v)
    #define ustore64be(p,v)  mango::ustore64(p, v)

#endif

    // --------------------------------------------------------------
    // endian storage types
    // --------------------------------------------------------------

    // Interface for endian aware storage class.

    // Implements swap-on-read and swap-on-write for built-in types
    // with compile-time endianess selection. Strict aliasing is honored
    // by using char array as the underlying storage representation.
    // Compilers will recognize the short memcpy idiom and use native
    // load and store instructions in their place - alignment always works
    // with this approach.

    // The problem this solves is a new one introduced by the memory mapped I/O
    // architecture of MANGO API; client can read/write struct that "just works"

    /* Code example:
    struct Foo
    {
        uint32be a;
        uint32be b;
    };

    void bar(const char *ptr)
    {
        // ptr is pointer to some offset in a memory mapped file
        // convert it to our type Foo. Packing will always be correct as we use char storage (see above)
        const Foo *foo = reinterpret_cast<const Foo *>(ptr);

        // The endian conversion is done when we read from the variables
        uint32 a = foo->a;
        uint32 b = foo->b;
    }

    */

    namespace detail
    {

        template <typename T>
        class TypeCopy {
        protected:
            char data[sizeof(T)];

        public:
            TypeCopy() = default;

            TypeCopy(const T &value) {
                std::memcpy(data, &value, sizeof(T));
            }

            // copy-on-write
            const TypeCopy& operator = (const T &value) {
                std::memcpy(data, &value, sizeof(T));
                return *this;
            }

            // copy-on-read
            operator T () const {
                T temp;
                std::memcpy(&temp, data, sizeof(T));
                return temp;
            }
        };

        template <typename T>
        class TypeSwap {
        protected:
            char data[sizeof(T)];

        public:
            TypeSwap() = default;

            TypeSwap(const T &value) {
                T temp = byteswap(value);
                std::memcpy(data, &temp, sizeof(T));
            }

            // swap-on-write
            const TypeSwap& operator = (const T &value) {
                T temp = byteswap(value);
                std::memcpy(data, &temp, sizeof(T));
                return *this;
            }

            // swap-on-read
            operator T () const {
                T temp;
                std::memcpy(&temp, data, sizeof(T));
                return byteswap(temp);
            }
        };

    } // namespace detail

#ifdef MANGO_LITTLE_ENDIAN

    using int16le = detail::TypeCopy<int16>;
    using int32le = detail::TypeCopy<int32>;
    using int64le = detail::TypeCopy<int64>;
    using uint16le = detail::TypeCopy<uint16>;
    using uint32le = detail::TypeCopy<uint32>;
    using uint64le = detail::TypeCopy<uint64>;
    using float16le = detail::TypeCopy<half>;
    using float32le = detail::TypeCopy<float>;
    using float64le = detail::TypeCopy<double>;

    using int16be = detail::TypeSwap<int16>;
    using int32be = detail::TypeSwap<int32>;
    using int64be = detail::TypeSwap<int64>;
    using uint16be = detail::TypeSwap<uint16>;
    using uint32be = detail::TypeSwap<uint32>;
    using uint64be = detail::TypeSwap<uint64>;
    using float16be = detail::TypeSwap<half>;
    using float32be = detail::TypeSwap<float>;
    using float64be = detail::TypeSwap<double>;

#else

    using int16le = detail::TypeSwap<int16>;
    using int32le = detail::TypeSwap<int32>;
    using int64le = detail::TypeSwap<int64>;
    using uint16le = detail::TypeSwap<uint16>;
    using uint32le = detail::TypeSwap<uint32>;
    using uint64le = detail::TypeSwap<uint64>;
    using float16le = detail::TypeSwap<half>;
    using float32le = detail::TypeSwap<float>;
    using float64le = detail::TypeSwap<double>;

    using int16be = detail::TypeCopy<int16>;
    using int32be = detail::TypeCopy<int32>;
    using int64be = detail::TypeCopy<int64>;
    using uint16be = detail::TypeCopy<uint16>;
    using uint32be = detail::TypeCopy<uint32>;
    using uint64be = detail::TypeCopy<uint64>;
    using float16be = detail::TypeCopy<half>;
    using float32be = detail::TypeCopy<float>;
    using float64be = detail::TypeCopy<double>;

#endif
    
} // namespace mango
