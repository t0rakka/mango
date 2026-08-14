/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "core_test.hpp"

using namespace mango;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    bool test_uload_ustore_unaligned()
    {
        alignas(8) u8 storage[32] = {};

        storage[1] = 0x12;
        storage[2] = 0x34;
        CHECK(uload16_reverse(storage + 1) == 0x1234);

        storage[3] = 0x56;
        storage[4] = 0x78;
        storage[5] = 0x9a;
        CHECK(uload24(storage + 3) == 0x9a7856);

        storage[6] = 0x11;
        storage[7] = 0x22;
        storage[8] = 0x33;
        storage[9] = 0x44;
        CHECK(uload32_reverse(storage + 6) == 0x11223344);

        storage[10] = 0xaa;
        storage[11] = 0xbb;
        storage[12] = 0xcc;
        storage[13] = 0xdd;
        storage[14] = 0xee;
        storage[15] = 0xff;
        storage[16] = 0x00;
        storage[17] = 0x11;
        CHECK(uload64_reverse(storage + 10) == 0xaabbccddeeff0011ULL);

        ustore16(storage + 18, 0xbeef);
        CHECK(storage[18] == 0xef);
        CHECK(storage[19] == 0xbe);

        ustore24(storage + 20, 0x102030);
        CHECK(storage[20] == 0x30);
        CHECK(storage[21] == 0x20);
        CHECK(storage[22] == 0x10);

        ustore32(storage + 23, 0xdeadbeef);
        CHECK(uload32(storage + 23) == 0xdeadbeef);

        return true;
    }

    bool test_big_endian_const_pointer()
    {
        const u8 bytes[] =
        {
            0x00,
            0x12,
            0x34, 0x56,
            0x78, 0x9a, 0xbc,
            0xde, 0xf0, 0x11, 0x22,
            0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa,
        };

        BigEndianConstPointer p(bytes + 1);

        CHECK(p.read8() == 0x12);
        CHECK(p.read16() == 0x3456);
        CHECK(p.read24() == 0xbc9a78); // uload24 is always little-endian on the wire
        CHECK(p.read32() == 0xdef01122);
        CHECK(p.read64() == 0x33445566778899aaULL);

        return true;
    }

    bool test_endian_stream_roundtrip()
    {
        MemoryStream stream;

        {
            BigEndianStream out(stream);
            out.write8(0x01);
            out.write16(0x0203);
            out.write24(0x040506);
            out.write32(0x0708090a);
            out.write64(0x0b0c0d0e0f101112ULL);
        }

        stream.seek(0, Stream::SeekMode::Begin);

        BigEndianStream in(stream);
        CHECK(in.read8() == 0x01);
        CHECK(in.read16() == 0x0203);
        CHECK(in.read24() == 0x040506);
        CHECK(in.read32() == 0x0708090a);
        CHECK(in.read64() == 0x0b0c0d0e0f101112ULL);

        return true;
    }

    bool test_byteswap_memory()
    {
        alignas(16) u16 data16[] = { 0x1234, 0xabcd, 0x0001, 0xff00, 0x00ff };
        const u16 expect16[] = { 0x3412, 0xcdab, 0x0100, 0x00ff, 0xff00 };

        byteswap(data16, 5);
        for (int i = 0; i < 5; ++i)
        {
            CHECK(data16[i] == expect16[i]);
        }

        alignas(16) u32 data32[] = { 0x12345678, 0x9abcdef0, 0x00000001, 0x80000000 };
        const u32 expect32[] = { 0x78563412, 0xf0debc9a, 0x01000000, 0x00000080 };

        byteswap(data32, 4);
        for (int i = 0; i < 4; ++i)
        {
            CHECK(data32[i] == expect32[i]);
        }

        u8 raw[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
        byteswap(Memory(raw, sizeof(raw)), 16);
        CHECK(raw[0] == 0x22);
        CHECK(raw[1] == 0x11);
        CHECK(raw[6] == 0x88);
        CHECK(raw[7] == 0x77);

        return true;
    }

    u8 naive_u8_reverse_bits(u8 value)
    {
        u8 result = 0;
        for (int i = 0; i < 8; ++i)
        {
            result = u8((result << 1) | (value & 1));
            value = u8(value >> 1);
        }
        return result;
    }

    u16 naive_u16_reverse_bits(u16 value)
    {
        u16 result = 0;
        for (int i = 0; i < 16; ++i)
        {
            result = u16((result << 1) | (value & 1));
            value = u16(value >> 1);
        }
        return result;
    }

    u32 naive_u32_reverse_bits(u32 value)
    {
        u32 result = 0;
        for (int i = 0; i < 32; ++i)
        {
            result = (result << 1) | (value & 1);
            value >>= 1;
        }
        return result;
    }

    u64 naive_u64_reverse_bits(u64 value)
    {
        u64 result = 0;
        for (int i = 0; i < 64; ++i)
        {
            result = (result << 1) | (value & 1);
            value >>= 1;
        }
        return result;
    }

    bool test_reverse_bits_u8()
    {
        alignas(64) u8 input[33];
        alignas(64) u8 output[33];

        for (size_t i = 0; i < sizeof(input); ++i)
        {
            input[i] = u8((i * 37 + 11) & 0xff);
        }

        for (size_t count : { size_t(0), size_t(1), size_t(7), size_t(8), size_t(15), size_t(16), size_t(17), size_t(33) })
        {
            reverse_bits(output, input, count);

            for (size_t i = 0; i < count; ++i)
            {
                CHECK(output[i] == naive_u8_reverse_bits(input[i]));
            }
        }

        u8 memory_input[17];
        u8 memory_output[17];
        for (size_t i = 0; i < sizeof(memory_input); ++i)
        {
            memory_input[i] = u8(i ^ 0xa5);
        }

        u8_reverse_bits(Memory(memory_output, sizeof(memory_output)), ConstMemory(memory_input, sizeof(memory_input)));
        for (size_t i = 0; i < sizeof(memory_input); ++i)
        {
            CHECK(memory_output[i] == naive_u8_reverse_bits(memory_input[i]));
        }

        return true;
    }

    bool test_reverse_bits_u16_u32_u64()
    {
        const u16 input16[] = { 0x1234, 0xabcd, 0x0001, 0xff00, 0x00ff, 0x8421, 0x1357, 0x2468, 0x0000 };
        u16 output16[std::size(input16)] = {};

        reverse_bits(output16, input16, std::size(input16));
        for (size_t i = 0; i < std::size(input16); ++i)
        {
            CHECK(output16[i] == naive_u16_reverse_bits(input16[i]));
        }

        const u32 input32[] = { 0x12345678, 0x9abcdef0, 0x00000001, 0x80000000, 0xdeadbeef, 0x00000000 };
        u32 output32[std::size(input32)] = {};

        reverse_bits(output32, input32, std::size(input32));
        for (size_t i = 0; i < std::size(input32); ++i)
        {
            CHECK(output32[i] == naive_u32_reverse_bits(input32[i]));
        }

        const u64 input64[] = { 0x0123456789abcdefULL, 0xfedcba9876543210ULL, 0x0000000000000001ULL, 0x8000000000000000ULL };
        u64 output64[std::size(input64)] = {};

        reverse_bits(output64, input64, std::size(input64));
        for (size_t i = 0; i < std::size(input64); ++i)
        {
            CHECK(output64[i] == naive_u64_reverse_bits(input64[i]));
        }

        return true;
    }

    const Case g_cases [] =
    {
        { "uload_ustore_unaligned",   test_uload_ustore_unaligned },
        { "big_endian_const_pointer", test_big_endian_const_pointer },
        { "endian_stream_roundtrip",  test_endian_stream_roundtrip },
        { "byteswap_memory",          test_byteswap_memory },
        { "reverse_bits_u8",          test_reverse_bits_u8 },
        { "reverse_bits_wide",        test_reverse_bits_u16_u32_u64 },
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("core_endian", g_cases, sizeof(g_cases) / sizeof(g_cases[0]), argc, argv);
}
