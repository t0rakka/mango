/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/configure.hpp>
#include <mango/core/endian.hpp>
#include <mango/core/memory.hpp>

namespace mango::image
{

#if defined(MANGO_CPU_64BIT)
    using AccumulatorType = u64;
#else
    using AccumulatorType = u32;
#endif

    struct DataRegister
    {
        enum { AccumulatorBits = sizeof(AccumulatorType) * 8 };

        AccumulatorType data = 0;
        int bits = 0;

        const u8* ptr = nullptr;
        const u8* end = nullptr;

        DataRegister(ConstMemory input)
        {
            ptr = input.address;
            end = ptr + input.size;
        }

        template <typename T>
        void append(T chunk, int nbits)
        {
            data |= (AccumulatorType(chunk) << bits);
            bits += nbits;
        }

        bool ensureBits(int nbits)
        {
            while (bits < nbits)
            {
                if (ptr >= end)
                {
                    // out of bytes to read
                    if (bits == 0)
                        return false;
                    bits = nbits; // pad with zeros
                    return true;
                }

#if defined(MANGO_CPU_64BIT)
                if (size_t(end - ptr) >= 8)
                {
                    // read 8 bytes, append only bytes that fit in the accumulator
                    const int bytes = (AccumulatorBits - bits) / 8;
                    u64 chunk = mango::littleEndian::uload64(ptr);
                    append(chunk, bytes * 8);
                    ptr += bytes;
                    return true;
                }
#endif

                // read one byte
                append(*ptr++, 8);
            }

            return true;
        }

        u32 getBits(int nbits) const
        {
            const AccumulatorType mask = (AccumulatorType(1) << nbits) - 1;
            return u32(data & mask);
        }

        void consumeBits(int nbits)
        {
            bits -= nbits;
            data >>= nbits;
        }
    };

    bool ccitt_rle_decompress(Memory output, ConstMemory input, u32 width, u32 height, bool word_aligned);
    bool ccitt_group3_decompress(Memory output, ConstMemory input, u32 width, u32 height, bool is_2d);
    bool ccitt_group4_decompress(Memory output, ConstMemory input, u32 width, u32 height);
    
} // namespace mango::image
