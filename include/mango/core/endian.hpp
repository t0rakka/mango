/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cassert>
#include <mango/core/configure.hpp>
#include <mango/core/bits.hpp>

namespace mango
{

    // --------------------------------------------------------------
    // unaligned load
    // --------------------------------------------------------------

    static inline
    u16 uload16(const void* p)
    {
        u16 value;
        std::memcpy(&value, p, sizeof(value));
        return value;
    }

    static inline
    u32 uload32(const void* p)
    {
        u32 value;
        std::memcpy(&value, p, sizeof(value));
        return value;
    }

    static inline
    u64 uload64(const void* p)
    {
        u64 value;
        std::memcpy(&value, p, sizeof(value));
        return value;
    }

    static inline
    float16 uload16f(const void* p)
    {
        float16 value;
        std::memcpy(&value, p, sizeof(value));
        return value;
    }

    static inline
    float32 uload32f(const void* p)
    {
        float32 value;
        std::memcpy(&value, p, sizeof(value));
        return value;
    }

    static inline
    float64 uload64f(const void* p)
    {
        float64 value;
        std::memcpy(&value, p, sizeof(value));
        return value;
    }

    // --------------------------------------------------------------
    // unaligned store
    // --------------------------------------------------------------

    static inline
    void ustore16(void* p, u16 value)
    {
        std::memcpy(p, &value, sizeof(value));
    }

    static inline
    void ustore32(void* p, u32 value)
    {
        std::memcpy(p, &value, sizeof(value));
    }

    static inline
    void ustore64(void* p, u64 value)
    {
        std::memcpy(p, &value, sizeof(value));
    }

    static inline
    void ustore16f(void* p, float16 value)
    {
        std::memcpy(p, &value, sizeof(value));
    }

    static inline
    void ustore32f(void* p, float32 value)
    {
        std::memcpy(p, &value, sizeof(value));
    }

    static inline
    void ustore64f(void* p, float64 value)
    {
        std::memcpy(p, &value, sizeof(value));
    }

} // namespace mango

namespace mango::littleEndian
{

    // --------------------------------------------------------------
    // littleEndian load/store
    // --------------------------------------------------------------

#ifdef MANGO_LITTLE_ENDIAN

    inline auto uload16 = mango::uload16;
    inline auto uload32 = mango::uload32;
    inline auto uload64 = mango::uload64;

    inline auto uload16f = mango::uload16f;
    inline auto uload32f = mango::uload32f;
    inline auto uload64f = mango::uload64f;

    inline auto ustore16 = mango::ustore16;
    inline auto ustore32 = mango::ustore32;
    inline auto ustore64 = mango::ustore64;

    inline auto ustore16f = mango::ustore16f;
    inline auto ustore32f = mango::ustore32f;
    inline auto ustore64f = mango::ustore64f;

#else

    inline auto uload16 = [] (const void* p) { return byteswap(mango::uload16(p)); };
    inline auto uload32 = [] (const void* p) { return byteswap(mango::uload32(p)); };
    inline auto uload64 = [] (const void* p) { return byteswap(mango::uload64(p)); };

    inline auto uload16f = [] (const void* p) { return byteswap(mango::uload16f(p)); };
    inline auto uload32f = [] (const void* p) { return byteswap(mango::uload32f(p)); };
    inline auto uload64f = [] (const void* p) { return byteswap(mango::uload64f(p)); };

    inline auto ustore16 = [] (void* p, u16 value) { mango::ustore16(p, byteswap(value)); };
    inline auto ustore32 = [] (void* p, u32 value) { mango::ustore32(p, byteswap(value)); };
    inline auto ustore64 = [] (void* p, u64 value) { mango::ustore64(p, byteswap(value)); };

    inline auto ustore16f = [] (void* p, float16 value) { mango::ustore16f(p, byteswap(value)); };
    inline auto ustore32f = [] (void* p, float32 value) { mango::ustore32f(p, byteswap(value)); };
    inline auto ustore64f = [] (void* p, float64 value) { mango::ustore64f(p, byteswap(value)); };

#endif

} // namespace mango::littleEndian

namespace mango::bigEndian
{

    // --------------------------------------------------------------
    // bigEndian load/store
    // --------------------------------------------------------------

#ifdef MANGO_LITTLE_ENDIAN

    inline auto uload16 = [] (const void* p) { return byteswap(mango::uload16(p)); };
    inline auto uload32 = [] (const void* p) { return byteswap(mango::uload32(p)); };
    inline auto uload64 = [] (const void* p) { return byteswap(mango::uload64(p)); };

    inline auto uload16f = [] (const void* p) { return byteswap(mango::uload16f(p)); };
    inline auto uload32f = [] (const void* p) { return byteswap(mango::uload32f(p)); };
    inline auto uload64f = [] (const void* p) { return byteswap(mango::uload64f(p)); };

    inline auto ustore16 = [] (void* p, u16 value) { mango::ustore16(p, byteswap(value)); };
    inline auto ustore32 = [] (void* p, u32 value) { mango::ustore32(p, byteswap(value)); };
    inline auto ustore64 = [] (void* p, u64 value) { mango::ustore64(p, byteswap(value)); };

    inline auto ustore16f = [] (void* p, float16 value) { mango::ustore16f(p, byteswap(value)); };
    inline auto ustore32f = [] (void* p, float32 value) { mango::ustore32f(p, byteswap(value)); };
    inline auto ustore64f = [] (void* p, float64 value) { mango::ustore64f(p, byteswap(value)); };

#else

    inline auto uload16 = mango::uload16;
    inline auto uload32 = mango::uload32;
    inline auto uload64 = mango::uload64;

    inline auto uload16f = mango::uload16f;
    inline auto uload32f = mango::uload32f;
    inline auto uload64f = mango::uload64f;

    inline auto ustore16 = mango::ustore16;
    inline auto ustore32 = mango::ustore32;
    inline auto ustore64 = mango::ustore64;

    inline auto ustore16f = mango::ustore16f;
    inline auto ustore32f = mango::ustore32f;
    inline auto ustore64f = mango::ustore64f;

#endif

} // namespace mango::bigEndian

namespace mango::detail
{

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
        bigEndian::u32 a;
        bigEndian::u32 b;
    };

    void bar(const char *ptr)
    {
        // ptr is pointer to some offset in a memory mapped file
        // convert it to our type Foo. Packing will always be correct as we use char storage (see above)
        const Foo *foo = reinterpret_cast<const Foo *>(ptr);

        // The endian conversion is done when we read from the variables
        u32 a = foo->a;
        u32 b = foo->b;
    }

    */

    template <typename T>
    class TypeCopy
    {
    protected:
        char data[sizeof(T)];

    public:
        TypeCopy() = default;

        TypeCopy(const T &value)
        {
            std::memcpy(data, &value, sizeof(T));
        }

        // copy-on-write
        const TypeCopy& operator = (const T &value)
        {
            std::memcpy(data, &value, sizeof(T));
            return *this;
        }

        // copy-on-read
        operator T () const
        {
            T temp;
            std::memcpy(&temp, data, sizeof(T));
            return temp;
        }
    };

    template <typename T>
    class TypeSwap
    {
    protected:
        char data[sizeof(T)];

    public:
        TypeSwap() = default;

        TypeSwap(const T &value)
        {
            T temp = byteswap(value);
            std::memcpy(data, &temp, sizeof(T));
        }

        // swap-on-write
        const TypeSwap& operator = (const T &value)
        {
            T temp = byteswap(value);
            std::memcpy(data, &temp, sizeof(T));
            return *this;
        }

        // swap-on-read
        operator T () const
        {
            T temp;
            std::memcpy(&temp, data, sizeof(T));
            return byteswap(temp);
        }
    };

} // namespace mango::detail

namespace mango::littleEndian
{

    // --------------------------------------------------------------
    // littleEndian storage types
    // --------------------------------------------------------------

#ifdef MANGO_LITTLE_ENDIAN

    using s16 = detail::TypeCopy<mango::s16>;
    using s32 = detail::TypeCopy<mango::s32>;
    using s64 = detail::TypeCopy<mango::s64>;
    using u16 = detail::TypeCopy<mango::u16>;
    using u32 = detail::TypeCopy<mango::u32>;
    using u64 = detail::TypeCopy<mango::u64>;

    using float16 = detail::TypeCopy<mango::float16>;
    using float32 = detail::TypeCopy<mango::float32>;
    using float64 = detail::TypeCopy<mango::float64>;

#else

    using s16 = detail::TypeSwap<mango::s16>;
    using s32 = detail::TypeSwap<mango::s32>;
    using s64 = detail::TypeSwap<mango::s64>;
    using u16 = detail::TypeSwap<mango::u16>;
    using u32 = detail::TypeSwap<mango::u32>;
    using u64 = detail::TypeSwap<mango::u64>;

    using float16 = detail::TypeSwap<mango::float16>;
    using float32 = detail::TypeSwap<mango::float32>;
    using float64 = detail::TypeSwap<mango::float64>;

#endif

} // namespace mango::littleEndian

namespace mango::bigEndian
{

    // --------------------------------------------------------------
    // bigEndian storage types
    // --------------------------------------------------------------

#ifdef MANGO_LITTLE_ENDIAN

    using s16 = detail::TypeSwap<mango::s16>;
    using s32 = detail::TypeSwap<mango::s32>;
    using s64 = detail::TypeSwap<mango::s64>;
    using u16 = detail::TypeSwap<mango::u16>;
    using u32 = detail::TypeSwap<mango::u32>;
    using u64 = detail::TypeSwap<mango::u64>;

    using float16 = detail::TypeSwap<mango::float16>;
    using float32 = detail::TypeSwap<mango::float32>;
    using float64 = detail::TypeSwap<mango::float64>;

#else

    using s16 = detail::TypeCopy<mango::s16>;
    using s32 = detail::TypeCopy<mango::s32>;
    using s64 = detail::TypeCopy<mango::s64>;
    using u16 = detail::TypeCopy<mango::u16>;
    using u32 = detail::TypeCopy<mango::u32>;
    using u64 = detail::TypeCopy<mango::u64>;

    using float16 = detail::TypeCopy<mango::float16>;
    using float32 = detail::TypeCopy<mango::float32>;
    using float64 = detail::TypeCopy<mango::float64>;

#endif

} // namespace mango::bigEndian
