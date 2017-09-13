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

    template <typename T>
    class TypeCopy {
    protected:
        char data[sizeof(T)];

    public:
        TypeCopy() = default;

        TypeCopy(const T &value) {
            std::memcpy(data, &value, sizeof(T));
        }

        const TypeCopy& operator = (const T &value) {
            std::memcpy(data, &value, sizeof(T));
            return *this;
        }

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

        const TypeSwap& operator = (const T &value) {
            T temp = byteswap(value);
            std::memcpy(data, &temp, sizeof(T));
            return *this;
        }

        operator T () const {
            T temp;
            std::memcpy(&temp, data, sizeof(T));
            return byteswap(temp);
        }
    };

#ifdef MANGO_LITTLE_ENDIAN

    typedef TypeCopy<int16>   int16le;
    typedef TypeCopy<int32>   int32le;
    typedef TypeCopy<int64>   int64le;
    typedef TypeCopy<uint16>  uint16le;
    typedef TypeCopy<uint32>  uint32le;
    typedef TypeCopy<uint64>  uint64le;
    typedef TypeCopy<half>    float16le;
    typedef TypeCopy<float>   float32le;
    typedef TypeCopy<double>  float64le;

    typedef TypeSwap<int16>   int16be;
    typedef TypeSwap<int32>   int32be;
    typedef TypeSwap<int64>   int64be;
    typedef TypeSwap<uint16>  uint16be;
    typedef TypeSwap<uint32>  uint32be;
    typedef TypeSwap<uint64>  uint64be;
    typedef TypeSwap<half>    float16be;
    typedef TypeSwap<float>   float32be;
    typedef TypeSwap<double>  float64be;

#else

    typedef TypeSwap<int16>   int16le;
    typedef TypeSwap<int32>   int32le;
    typedef TypeSwap<int64>   int64le;
    typedef TypeSwap<uint16>  uint16le;
    typedef TypeSwap<uint32>  uint32le;
    typedef TypeSwap<uint64>  uint64le;
    typedef TypeSwap<half>    float16le;
    typedef TypeSwap<float>   float32le;
    typedef TypeSwap<double>  float64le;

    typedef TypeCopy<int16>   int16be;
    typedef TypeCopy<int32>   int32be;
    typedef TypeCopy<int64>   int64be;
    typedef TypeCopy<uint16>  uint16be;
    typedef TypeCopy<uint32>  uint32be;
    typedef TypeCopy<uint64>  uint64be;
    typedef TypeCopy<half>    float16be;
    typedef TypeCopy<float>   float32be;
    typedef TypeCopy<double>  float64be;

#endif
    
} // namespace mango
