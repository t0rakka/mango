/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cmath>
#include <algorithm>
#include "configure.hpp"
#include "half.hpp"

namespace mango
{

    // ----------------------------------------------------------------------------
    // byteswap
    // ----------------------------------------------------------------------------

#if defined(MANGO_COMPILER_MICROSOFT)

    // Microsoft Visual Studio intrinsics
    #include <intrin.h>

    static inline uint16 byteswap16(uint16 v)
    {
        return _byteswap_ushort(v);
    }

    static inline uint32 byteswap32(uint32 v)
    {
        return _byteswap_ulong(v);
    }

    static inline uint64 byteswap64(uint64 v)
    {
        return _byteswap_uint64(v);
    }

#elif defined(MANGO_COMPILER_GCC) || defined(MANGO_COMPILER_CLANG)

    // GCC / LLVM intrinsics

    static inline uint16 byteswap16(uint16 v)
    {
        return __builtin_bswap32(v << 16);
    }

    static inline uint32 byteswap32(uint32 v)
    {
        return __builtin_bswap32(v);
    }

    static inline uint64 byteswap64(uint64 v)
    {
        return __builtin_bswap64(v);
    }

#else

    // generic implementation

    static inline uint16 byteswap16(uint16 v)
    {
        return static_cast<uint16>((v << 8) | (v >> 8));
    }

    static inline uint32 byteswap32(uint32 v)
    {
        return (v >> 24) | ((v >> 8) & 0x0000ff00) | ((v << 8) & 0x00ff0000) | (v << 24);
    }

    static inline uint64 byteswap64(uint64 v)
    {
        uint32 low = v & 0xffffffff;
        uint32 high = v >> 32;
        low = byteswap32(low);
        high = byteswap32(high);
        return (static_cast<uint64>(low) << 32) | high;
    }

#endif

    template <typename Type>
    static inline Type byteswap(Type v)
    {
        return v;
    }

    template <>
    inline uint16 byteswap<uint16>(uint16 v)
    {
        return byteswap16(v);
    }

    template <>
    inline uint32 byteswap<uint32>(uint32 v)
    {
        return byteswap32(v);
    }

    template <>
    inline uint64 byteswap<uint64>(uint64 v)
    {
        return byteswap64(v);
    }

    template <>
    inline Half byteswap<Half>(Half v)
    {
        v.u = byteswap16(v.u);
        return v;
    }

    template <>
    inline Float byteswap<Float>(Float v)
    {
        v.u = byteswap32(v.u);
        return v;
    }

    template <>
    inline Double byteswap<Double>(Double v)
    {
        v.u = byteswap64(v.u);
        return v;
    }

    // ----------------------------------------------------------------------------
    // uint24
    // ----------------------------------------------------------------------------

    struct uint24
    {
        uint8 data[3];

        uint24()
        {
        }

        uint24(uint32 v)
        {
            *this = v;
        }

#ifdef MANGO_LITTLE_ENDIAN
        operator uint32 () const
        {
            uint32 v = (data[2] << 16) | (data[1] << 8) | data[0];
            return v;
        }

        uint24& operator = (uint32 v)
        {
            data[0] = uint8(v);
            data[1] = uint8(v >> 8);
            data[2] = uint8(v >> 16);
            return *this;
        }
#else
        operator uint32 () const
        {
            uint32 v = (data[0] << 16) | (data[1] << 8) | data[2];
            return v;
        }

        uint24& operator = (uint32 v)
        {
            data[0] = uint8(v >> 16);
            data[1] = uint8(v >> 8);
            data[2] = uint8(v);
            return *this;
        }
#endif
    };

    // ----------------------------------------------------------------------------
    // misc
    // ----------------------------------------------------------------------------

    static inline float ntsc_luminance(float red, float green, float blue)
    {
        return red * 0.299f + green * 0.587f + blue * 0.114f;
    }

    template <typename T>
    static inline T clamp(T value, T min, T max)
    {
        return std::min(max, std::max(min, value));
    }

    static inline uint32 byteclamp(int32 v)
    {
        // clamp value to [0, 255] range
        if (v & 0xffffff00)
        {
            v = (((~v) >> 31) & 0xff);
        }
        return uint32(v);
    }

    static inline int modulo(int value, int range)
    {
        const int remainder = value % range;
        return remainder < 0 ? remainder + range : remainder;
    }

    static inline int round_to_next(int v, int multiple)
    {
        return std::max(1, (v + (multiple - 1)) / multiple);
    }

    static inline float snap(float value, float gridsize)
    {
        if (gridsize)
        {
            value = std::floor(0.5f + value / gridsize) * gridsize;
        }
        return value;
    }

    // TODO: thread-safe API
    static inline uint32 random(uint32 seed = 0, bool reseed = false)
    {
        static uint32 prev = 0x12345678;
        prev = reseed ?	seed : prev;
        prev = prev * 1664525 + 1013904223;
        return prev;
    }

    // ----------------------------------------------------------------------------
    // 32 bits
    // ----------------------------------------------------------------------------

    static inline uint32 u32_expand_lsb(uint32 v)
    {
		// NOTE: 0 expands to 0xffffffff
        return v ^ (v - 1);
    }

    static inline uint32 u32_expand_msb(uint32 v)
    {
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        return v;
    }

    static inline uint32 u32_lsb(uint32 v)
    {
        return v & (0 - v);
    }

    static inline uint32 u32_msb(uint32 v)
    {
        v = u32_expand_msb(v);
        return (v + 1) >> 1;
    }

    static inline int u32_index_of_bit(uint32 v)
    {
        static const uint8 table[] =
        {
            0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
            31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
        };
        return table[(v * 0x077cb531) >> 27];
    }

    static inline int u32_index_of_expanded_bit(uint32 v)
    {
        static const uint8 table[] =
        {
			0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
			8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
        };
        return table[(v * 0x07c4acdd) >> 27];
    }

    static inline int u32_index_of_lsb(uint32 v)
    {
        v = u32_lsb(v);
        return u32_index_of_bit(v);
    }

    static inline int u32_index_of_msb(uint32 v)
    {
        int base = 0;
        uint32 n;
        n = v & 0xffff0000; if (n) { base |= 16; v = n; }
        n = v & 0xff00ff00; if (n) { base |= 8;  v = n; }
        n = v & 0xf0f0f0f0; if (n) { base |= 4;  v = n; }
        n = v & 0xcccccccc; if (n) { base |= 2;  v = n; }
        n = v & 0xaaaaaaaa; if (n) { base |= 1; }
        return base;
    }

    static inline int u32_index_of_msb_in_mask(uint32 v)
    {
        v = v & ~(v >> 1);
        return u32_index_of_bit(v);
    }

    static inline uint32 u32_reverse_bits(uint32 v)
    {
        // reverse bits
        v = ((v >> 1) & 0x55555555) | ((v << 1) & 0xaaaaaaaa);
        v = ((v >> 2) & 0x33333333) | ((v << 2) & 0xcccccccc);
        v = ((v >> 4) & 0x0f0f0f0f) | ((v << 4) & 0xf0f0f0f0);
        v = ((v >> 8) & 0x00ff00ff) | ((v << 8) & 0xff00ff00);
        v = (v >> 16) | (v << 16);
        return v;
    }

#if defined(MANGO_ENABLE_POPCNT)

    static inline int u32_count_bits(uint32 v)
    {
		return _mm_popcnt_u32(v);
    }

#else

    static inline int u32_count_bits(uint32 v)
    {
        v -= (v >> 1) & 0x55555555;
        v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
        v = (v + (v >> 4)) & 0x0f0f0f0f;
        return int((v * 0x01010101) >> 24);
    }

#endif

    static inline bool u32_is_power_of_two(uint32 value)
    {
        // value:     xxxxx100000
        // value - 1: xxxxx011111
        // Subtracting one from the value borrows from the first set bit
        // that is encountered. The bitwise-and will yield 0 ONLY if the
        // the other bits in the value (marked with x) are not set.
        // The expression will evaluate to zero ONLY when a single bit is set,
        // which means the value is a power of two.
        return (value & (value - 1)) == 0;
    }

    static inline uint32 u32_floor_power_of_two(uint32 v)
    {
        v = u32_expand_msb(v);
        return (v + 1) >> 1;
    }

    static inline uint32 u32_ceil_power_of_two(uint32 v)
    {
        v = u32_expand_msb(v - 1);
        return v + 1;
    }

    static inline int u32_log2(uint32 v)
    {
        v = u32_expand_msb(v);
        return u32_index_of_expanded_bit(v);
    }

#ifdef MANGO_ENABLE_BMI

    static inline uint32 u32_extract_bits(uint32 src, int start, int len)
    {
        return _bextr_u32(src, start, len);
    }

#else

    static inline uint32 u32_extract_bits(uint32 src, int start, int len)
    {
        return (src >> start) & ((1 << len) - 1);
    }

#endif

#ifdef MANGO_ENABLE_BMI2

    static inline uint32 u32_interleave_bits(uint32 v)
    {
        return _pdep_u32(v, 0x55555555);
    }

    static inline uint32 u32_encode_morton(uint32 x, uint32 y)
    {
        return _pdep_u32(x, 0x55555555) | _pdep_u32(y, 0xaaaaaaaa);
    }

#else

    static inline uint32 u32_interleave_bits(uint32 v)
    {
        v = (v | (v << 8)) & 0x00ff00ff;
        v = (v | (v << 4)) & 0x0f0f0f0f;
        v = (v | (v << 2)) & 0x33333333;
        v = (v | (v << 1)) & 0x55555555;
        return v;
    }

    static inline uint32 u32_encode_morton(uint32 x, uint32 y)
    {
        x = u32_interleave_bits(x);
        y = u32_interleave_bits(y);
        return x | (y << 1);
    }

#endif

    static inline bool u32_has_zero_byte(uint32 v)
    {
        const uint32 mask = 0x7f7f7f7f;
        v = ((v & mask) + mask) | v;
        return ~(v | mask) != 0;
    }

    // ----------------------------------------------------------------------------
    // 64 bits
    // ----------------------------------------------------------------------------

    static inline uint64 u64_expand_lsb(uint64 v)
    {
		// NOTE: 0 expands to 0xffffffffffffffff
        return v ^ (v - 1);
    }

    static inline uint64 u64_expand_msb(uint64 v)
    {
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        return v;
    }

    static inline uint64 u64_lsb(uint64 v)
    {
        return v & (0 - v);
    }

    static inline uint64 u64_msb(uint64 v)
    {
        v = u64_expand_msb(v);
        return (v + 1) >> 1;
    }

    static inline int u64_index_of_bit(uint64 v)
    {
        static const uint8 table[] =
        {
            0,  1,  2, 53,  3,  7, 54, 27,
            4, 38, 41,  8, 34, 55, 48, 28,
            62,  5, 39, 46, 44, 42, 22,  9,
            24, 35, 59, 56, 49, 18, 29, 11,
            63, 52,  6, 26, 37, 40, 33, 47,
            61, 45, 43, 21, 23, 58, 17, 10,
            51, 25, 36, 32, 60, 20, 57, 16,
            50, 31, 19, 15, 30, 14, 13, 12,
        };
        return table[(v * 0x022fdd63cc95386du) >> 58];
    }

    static inline int u64_index_of_expanded_bit(uint64 v)
    {
        static const uint8 table[] =
        {
            0, 47, 1, 56, 48, 27, 2, 60,
            57, 49, 41, 37, 28, 16, 3, 61,
            54, 58, 35, 52, 50, 42, 21, 44,
            38, 32, 29, 23, 17, 11, 4, 62,
            46, 55, 26, 59, 40, 36, 15, 53,
            34, 51, 20, 43, 31, 22, 10, 45,
            25, 39, 14, 33, 19, 30, 9, 24,
            13, 18, 8, 12, 7, 6, 5, 63
        };
        return table[(v * 0x03f79d71b4cb0a89u) >> 58];
    }

    static inline int u64_index_of_lsb(uint64 v)
    {
        v = u64_lsb(v);
        return u64_index_of_bit(v);
    }

    static inline int u64_index_of_msb(uint64 v)
    {
        v = u64_expand_msb(v);
        return u64_index_of_expanded_bit(v);
    }

#if defined(MANGO_ENABLE_POPCNT)

    static inline int u64_count_bits(uint64 v)
    {
    #if defined(MANGO_CPU_64BIT)
        return static_cast<int>(_mm_popcnt_u64(v));
    #else
        // popcnt_u64 is invalid instruction in 32 bit legacy mode
        uint32 low = v & 0xffffffff;
        uint32 high = v >> 32;
        return _mm_popcnt_u32(low) + _mm_popcnt_u32(high);
    #endif
    }

#else

    static inline int u64_count_bits(uint64 v)
    {
        const uint64 c = 0x3333333333333333u;
        v -= (v >> 1) & 0x5555555555555555u;
        v = (v & c) + ((v >> 2) & c);
        v = (v + (v >> 4)) & 0x0f0f0f0f0f0f0f0fu;
        return int((v * 0x0101010101010101u) >> 56);
    }

#endif

#ifdef MANGO_ENABLE_BMI

    static inline uint64 u64_extract_bits(uint64 src, int start, int len)
    {
        return _bextr_u64(src, start, len);
    }

#else

    static inline uint64 u64_extract_bits(uint64 src, int start, int len)
    {
        return (src >> start) & ((1 << len) - 1);
    }

#endif

#ifdef MANGO_ENABLE_BMI2

    static inline uint64 u64_interleave_bits(uint64 v)
    {
        return _pdep_u64(x, 0x55555555);
    }

    static inline uint64 u64_encode_morton(uint64 x, uint64 y)
    {
        return _pdep_u64(x, 0x55555555) | _pdep_u64(y, 0xaaaaaaaa);
    }

#else

    static inline uint64 u64_interleave_bits(uint64 v)
    {
        v = (v | (v << 16)) & 0x0000ffff0000ffff;
        v = (v | (v <<  8)) & 0x00ff00ff00ff00ff;
        v = (v | (v <<  4)) & 0x0f0f0f0f0f0f0f0f;
        v = (v | (v <<  2)) & 0x3333333333333333;
        v = (v | (v <<  1)) & 0x5555555555555555;
        return v;
    }

    static inline uint64 u64_encode_morton(uint64 x, uint64 y)
    {
        x = u64_interleave_bits(x);
        y = u64_interleave_bits(y);
        return x | (y << 1);
    }

#endif

    static inline bool u64_has_zero_byte(uint64 v)
    {
        const uint64 mask = 0x7f7f7f7f7f7f7f7f;
        v = ((v & mask) + mask) | v;
        return ~(v | mask) != 0;
    }

    static inline bool u64_is_power_of_two(uint64 value)
    {
        return (value & (value - 1)) == 0;
    }

} // namespace mango
