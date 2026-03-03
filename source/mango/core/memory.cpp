/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cassert>
#include <mango/core/bits.hpp>
#include <mango/core/memory.hpp>
#include <mango/math/vector.hpp>

namespace mango
{
    using namespace mango::math;

    // -----------------------------------------------------------------------
    // SharedMemory
    // -----------------------------------------------------------------------

    SharedMemory::SharedMemory(size_t bytes)
    {
        u8* address = new u8[bytes];
        m_memory = Memory(address, bytes);
        m_ptr = std::shared_ptr<u8>(address, std::default_delete<u8[]>());
    }

    SharedMemory::SharedMemory(u8* address, size_t bytes)
        : m_memory(address, bytes)
        , m_ptr(address, std::default_delete<u8[]>())
    {
    }

    // -----------------------------------------------------------------------
    // aligned malloc/free
    // -----------------------------------------------------------------------

#if defined(MANGO_COMPILER_MSVC)

    void* aligned_malloc(size_t bytes, size_t alignment)
    {
        return _aligned_malloc(bytes, alignment);
    }

    void aligned_free(void* aligned)
    {
        _aligned_free(aligned);
    }

#elif defined(MANGO_PLATFORM_LINUX)

    void* aligned_malloc(size_t bytes, size_t alignment)
    {
        return memalign(alignment, bytes);
    }

    void aligned_free(void* aligned)
    {
        free(aligned);
    }

#else

    // generic implementation

    void* aligned_malloc(size_t bytes, size_t alignment)
    {
        const size_t mask = alignment - 1;
        void* block = std::malloc(bytes + mask + sizeof(void*));
        char* aligned = reinterpret_cast<char*>(block) + sizeof(void*);

        if (block)
        {
            aligned += alignment - (reinterpret_cast<ptrdiff_t>(aligned) & mask);
            reinterpret_cast<void**>(aligned)[-1] = block;
        }
        else
        {
            aligned = nullptr;
        }

        return aligned;
    }

    void aligned_free(void* aligned)
    {
        if (aligned)
        {
            void* block = reinterpret_cast<void**>(aligned)[-1];
            std::free(block);
        }
    }

#endif

    // -----------------------------------------------------------------------
    // reverse bits
    // -----------------------------------------------------------------------

    void reverse_bits(u8* output, const u8* input, size_t count)
    {
#if MANGO_SIMD_VECTOR_SIZE >= 128
        while (count >= 16)
        {
            auto value = uint16x8::uload(input);
            value = ((value >> 1) & 0x5555) | ((value << 1) & 0xaaaa);
            value = ((value >> 2) & 0x3333) | ((value << 2) & 0xcccc);
            value = ((value >> 4) & 0x0f0f) | ((value << 4) & 0xf0f0);
            uint16x8::ustore(output, value);

            input += 16;
            output += 16;
            count -= 16;
        }
#endif

        while (count-- > 0)
        {
            *output++ = mango::u8_reverse_bits(*input++);
        }
    }

    void reverse_bits(u16* output, const u16* input, size_t count)
    {
#if MANGO_SIMD_VECTOR_SIZE >= 128
        while (count >= 8)
        {
            auto value = uint16x8::uload(input);
            value = ((value >> 1) & 0x5555) | ((value << 1) & 0xaaaa);
            value = ((value >> 2) & 0x3333) | ((value << 2) & 0xcccc);
            value = ((value >> 4) & 0x0f0f) | ((value << 4) & 0xf0f0);
            value = (value >> 8) | (value << 8);
            uint16x8::ustore(output, value);

            input += 8;
            output += 8;
            count -= 8;
        }
#endif

        while (count-- > 0)
        {
            *output++ = mango::u16_reverse_bits(*input++);
        }
    }

    void reverse_bits(u32* output, const u32* input, size_t count)
    {
#if MANGO_SIMD_VECTOR_SIZE >= 128
        while (count >= 4)
        {
            auto value = uint32x4::uload(input);
            value = ((value >> 1) & 0x55555555) | ((value << 1) & 0xaaaaaaaa);
            value = ((value >> 2) & 0x33333333) | ((value << 2) & 0xcccccccc);
            value = ((value >> 4) & 0x0f0f0f0f) | ((value << 4) & 0xf0f0f0f0);
            value = ((value >> 8) & 0x00ff00ff) | ((value << 8) & 0xff00ff00);
            value = (value >> 16) | (value << 16);
            uint32x4::ustore(output, value);

            input += 4;
            output += 4;
            count -= 4;
        }
#endif

        while (count-- > 0)
        {
            *output++ = mango::u32_reverse_bits(*input++);
        }
    }

    void reverse_bits(u64* output, const u64* input, size_t count)
    {
#if MANGO_SIMD_VECTOR_SIZE >= 128
        while (count >= 2)
        {
            auto value = uint64x2::uload(input);
            value = ((value >>  1) & 0x5555555555555555) | ((value <<  1) & 0xaaaaaaaaaaaaaaaa);
            value = ((value >>  2) & 0x3333333333333333) | ((value <<  2) & 0xcccccccccccccccc);
            value = ((value >>  4) & 0x0f0f0f0f0f0f0f0f) | ((value <<  4) & 0xf0f0f0f0f0f0f0f0);
            value = ((value >>  8) & 0x00ff00ff00ff00ff) | ((value <<  8) & 0xff00ff00ff00ff00);
            value = ((value >> 16) & 0x0000ffff0000ffff) | ((value << 16) & 0xffff0000ffff0000);
            value = ( value >> 32) | ( value << 32);
            uint64x2::ustore(output, value);

            input += 2;
            output += 2;
            count -= 2;
        }
#endif

        while (count-- > 0)
        {
            *output++ = mango::u64_reverse_bits(*input++);
        }
    }

    void u8_reverse_bits(Memory output, ConstMemory input)
    {
        reverse_bits(output.address, input.address, input.size);
    }

    void u16_reverse_bits(Memory output, ConstMemory input)
    {
        assert((input.size & 1) == 0);
        size_t count = input.size >> 1;
        reverse_bits(reinterpret_cast<u16*>(output.address),
                     reinterpret_cast<const u16*>(input.address), count);
    }

    void u32_reverse_bits(Memory output, ConstMemory input)
    {
        assert((input.size & 3) == 0);
        size_t count = input.size >> 2;
        reverse_bits(reinterpret_cast<u32*>(output.address),
                     reinterpret_cast<const u32*>(input.address), count);
    }

    void u64_reverse_bits(Memory output, ConstMemory input)
    {
        assert((input.size & 7) == 0);
        size_t count = input.size >> 3;
        reverse_bits(reinterpret_cast<u64*>(output.address),
                     reinterpret_cast<const u64*>(input.address), count);
    }

    // -----------------------------------------------------------------------
    // byteswap
    // -----------------------------------------------------------------------

#if defined(MANGO_ENABLE_SIMD)

    void byteswap(u16* data, size_t count)
    {
        while (count >= 8)
        {
            math::uint16x8 value = math::uint16x8::uload(data);
            math::uint16x8 low   = value >> 8;
            math::uint16x8 high  = value << 8;
            math::uint16x8::ustore(data, high | low);
            data += 8;
            count -= 8;
        }

        for (size_t i = 0; i < count; ++i)
        {
            data[i] = byteswap(data[i]);
        }
    }

#else

    void byteswap(u16* data, size_t count)
    {
        while (count >= 4)
        {
            data[0] = byteswap(data[0]);
            data[1] = byteswap(data[1]);
            data[2] = byteswap(data[2]);
            data[3] = byteswap(data[3]);
            data += 4;
            count -= 4;
        }

        for (size_t i = 0; i < count; ++i)
        {
            data[i] = byteswap(data[i]);
        }
    }

#endif

#if defined(MANGO_ENABLE_SSE4_1)

    void byteswap(u32* data, size_t count)
    {
        const __m128i table = _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);

        while (count >= 4)
        {
            __m128i* ptr = reinterpret_cast<__m128i *>(data);
            __m128i v = _mm_loadu_si128(ptr);
            v = _mm_shuffle_epi8(v, table);
            _mm_storeu_si128(ptr, v);
            data += 4;
            count -= 4;
        }

        for (size_t i = 0; i < count; ++i)
        {
            data[i] = byteswap(data[i]);
        }
    }

    void byteswap(u64* data, size_t count)
    {
        const __m128i table = _mm_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);

        while (count >= 2)
        {
            __m128i* ptr = reinterpret_cast<__m128i *>(data);
            __m128i v = _mm_loadu_si128(ptr);
            v = _mm_shuffle_epi8(v, table);
            _mm_storeu_si128(ptr, v);
            data += 2;
            count -= 2;
        }

        for (size_t i = 0; i < count; ++i)
        {
            data[i] = byteswap(data[i]);
        }
    }

#elif defined(MANGO_ENABLE_NEON)

    void byteswap(u32* data, size_t count)
    {
        const uint8x8_t table = { 3, 2, 1, 0, 7, 6, 5, 4 };

        while (count >= 2)
        {
            u8* ptr = reinterpret_cast<u8*>(data);
            uint8x8_t v = vld1_u8(ptr);
            v = vtbl1_u8(v, table);
            vst1_u8(ptr, v);
            data += 2;
            count -= 2;
        }

        for (size_t i = 0; i < count; ++i)
        {
            data[i] = byteswap(data[i]);
        }
    }

    void byteswap(u64* data, size_t count)
    {
        const uint8x8_t table = { 7, 6, 5, 4, 3, 2, 1, 0 };

        for (size_t i = 0; i < count; ++i)
        {
            u8* ptr = reinterpret_cast<u8*>(data);
            uint8x8_t v = vld1_u8(ptr);
            v = vtbl1_u8(v, table);
            vst1_u8(ptr, v);
            ++data;
        }
    }

#else

    void byteswap(u32* data, size_t count)
    {
        while (count >= 4)
        {
            data[0] = byteswap(data[0]);
            data[1] = byteswap(data[1]);
            data[2] = byteswap(data[2]);
            data[3] = byteswap(data[3]);
            data += 4;
            count -= 4;
        }

        for (size_t i = 0; i < count; ++i)
        {
            data[i] = byteswap(data[i]);
        }
    }

    void byteswap(u64* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            data[i] = byteswap(data[i]);
        }
    }

#endif

    void byteswap(Memory memory, int bits)
    {
        switch (bits)
        {
            case 16:
                byteswap(reinterpret_cast<u16*>(memory.address), memory.size >> 1);
                break;
            case 32:
                byteswap(reinterpret_cast<u32*>(memory.address), memory.size >> 2);
                break;
            case 64:
                byteswap(reinterpret_cast<u64*>(memory.address), memory.size >> 3);
                break;
            default:
                break;
        }
    }

} // namespace mango
