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

    static inline u16 uload16(const u8* p)
    {
        return *reinterpret_cast<const u16 *>(p);
    }

    static inline u32 uload32(const u8* p)
    {
        return *reinterpret_cast<const u32 *>(p);
    }

    static inline u64 uload64(const u8* p)
    {
        return *reinterpret_cast<const u64 *>(p);
    }

    static inline void ustore16(u8* p, u16 value)
    {
        reinterpret_cast<u16 *>(p)[0] = value;
    }

    static inline void ustore32(u8* p, u32 value)
    {
        reinterpret_cast<u32 *>(p)[0] = value;
    }

    static inline void ustore64(u8* p, u64 value)
    {
        std::memcpy(p, &value, sizeof(u64));
    }

#else

    // Platform does not support unaligned load/store

    static inline u16 uload16(const u8* p)
    {
        u16 value;
        std::memcpy(&value, p, sizeof(u16));
        return value;
    }

    static inline u32 uload32(const u8* p)
    {
        u32 value;
        std::memcpy(&value, p, sizeof(u32));
        return value;
    }

    static inline u64 uload64(const u8* p)
    {
        u64 value;
        std::memcpy(&value, p, sizeof(u64));
        return value;
    }

    static inline void ustore16(u8* p, u16 value)
    {
        std::memcpy(p, &value, sizeof(u16));
    }

    static inline void ustore32(u8* p, u32 value)
    {
        std::memcpy(p, &value, sizeof(u32));
    }

    static inline void ustore64(u8* p, u64 value)
    {
        std::memcpy(p, &value, sizeof(u64));
    }

#endif

    // --------------------------------------------------------------
	// endian load/store
    // --------------------------------------------------------------

#ifdef MANGO_LITTLE_ENDIAN

    static inline u16 uload16le(const u8* p)
    {
        return uload16(p);
    }

    static inline u32 uload32le(const u8* p)
    {
        return uload32(p);
    }

    static inline u64 uload64le(const u8* p)
    {
        return uload64(p);
    }

    static inline u16 uload16be(const u8* p)
    {
        return byteswap(uload16(p));
    }

    static inline u32 uload32be(const u8* p)
    {
        return byteswap(uload32(p));
    }

    static inline u64 uload64be(const u8* p)
    {
        return byteswap(uload64(p));
    }

    static inline void ustore16le(u8* p, u16 value)
    {
        ustore16(p, value);
    }

    static inline void ustore32le(u8* p, u32 value)
    {
        ustore32(p, value);
    }

    static inline void ustore64le(u8* p, u64 value)
    {
        ustore64(p, value);
    }

    static inline void ustore16be(u8* p, u16 value)
    {
        ustore16(p, byteswap(value));
    }

    static inline void ustore32be(u8* p, u32 value)
    {
        ustore32(p, byteswap(value));
    }

    static inline void ustore64be(u8* p, u64 value)
    {
        ustore64(p, byteswap(value));
    }

#else

    static inline u16 uload16le(const u8* p)
    {
        return byteswap(uload16(p));
    }

    static inline u32 uload32le(const u8* p)
    {
        return byteswap(uload32(p));
    }

    static inline u64 uload64le(const u8* p)
    {
        return byteswap(uload64(p));
    }

    static inline u16 uload16be(const u8* p)
    {
        return uload16(p);
    }

    static inline u32 uload32be(const u8* p)
    {
        return uload32(p);
    }

    static inline u64 uload64be(const u8* p)
    {
        return uload64(p);
    }

    static inline void ustore16le(u8* p, u16 value)
    {
        ustore16(p, byteswap(value));
    }

    static inline void ustore32le(u8* p, u32 value)
    {
        ustore32(p, byteswap(value));
    }

    static inline void ustore64le(u8* p, u64 value)
    {
        ustore64(p, byteswap(value));
    }

    static inline void ustore16be(u8* p, u16 value)
    {
        ustore16(p, value);
    }

    static inline void ustore32be(u8* p, u32 value)
    {
        ustore32(p, value);
    }

    static inline void ustore64be(u8* p, u64 value)
    {
        ustore64(p, value);
    }

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
