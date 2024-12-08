/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cassert>
#include <mango/core/bits.hpp>
#include <mango/core/memory.hpp>

namespace mango
{

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

#if defined(MANGO_COMPILER_MICROSOFT)

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
    // byteswap()
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

        for (int i = 0; i < count; ++i)
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

        for (int i = 0; i < count; ++i)
        {
            data[i] = byteswap(data[i]);
        }
    }

#endif

#if defined(MANGO_ENABLE_SSSE3)

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

        for (int i = 0; i < count; ++i)
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

        for (int i = 0; i < count; ++i)
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

        for (int i = 0; i < count; ++i)
        {
            data[i] = byteswap(data[i]);
        }
    }

    void byteswap(u64* data, size_t count)
    {
        const uint8x8_t table = { 7, 6, 5, 4, 3, 2, 1, 0 };

        for (int i = 0; i < count; ++i)
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

        for (int i = 0; i < count; ++i)
        {
            data[i] = byteswap(data[i]);
        }
    }

    void byteswap(u64* data, size_t count)
    {
        for (int i = 0; i < count; ++i)
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
