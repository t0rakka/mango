/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;

constexpr u64 MB = 1 << 20;

void print(u32 value, u32 reference)
{
    printLine("    {:#010x} : {}", value, value == reference ? "OK" : "FAILED");
}

void print(ConstMemory buffer, const char* name, u64 time)
{
    u64 x = buffer.size * 1000000; // buffer size in bytes * microseconds_in_second
    printLine("{} {:5}.{:1} ms ({:6} MB/s )", name,
        time / 1000,
        ((time + 50) / 100) % 10,
        time ? x / (time * MB) : 0);
}

u64 test_crc32(ConstMemory buffer)
{
    printLine("CRC32 test vectors: ");
    printLine("");

    const u8 name [] = "123456789";

    u32 value0 = mango::crc32(0, ConstMemory(name, 9));
    print(value0, 0xcbf43926);

    u64 time0 = Time::us();
    u32 value1 = mango::crc32(0, buffer);
    u64 time1 = Time::us();

    print(value1, 0x9fb22d1f);

    return time1 - time0;
}

u64 test_crc32c(ConstMemory buffer)
{
    // RFC 3720 / iSCSI

    printLine("");
    printLine("CRC32C test vectors: ");
    printLine("");

    const u8 vec0 [] =
    {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    };

    const u8 vec1 [] =
    {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };

    u32 value0 = mango::crc32c(0, ConstMemory(vec0, 32));
    u32 value1 = mango::crc32c(0, ConstMemory(vec1, 32));
    print(value0, 0x8a9136aa);
    print(value1, 0x62a8ab43);

    u64 time0 = Time::us();
    u32 value2 = mango::crc32c(0, buffer);
    u64 time1 = Time::us();

    print(value2, 0x1fd9c660);

    return time1 - time0;
}

u64 test_adler32(ConstMemory buffer)
{
    printLine("");
    printLine("ADLER32 test vectors: ");
    printLine("");

    const u8 vec0 [] =
    {
        0, 1, 2, 3, 4, 5, 6, 7,
        7, 6, 5, 4, 3, 2, 1, 0,
        0, 1, 2, 3, 4, 5, 6, 7,
        7, 6, 5, 4, 3, 2, 1, 0,
    };

    const u8 vec1 [] =
    {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x80, 0xff, 0x80, 0xff, 0x80, 0xff, 0x80, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };

    u32 value0 = mango::adler32(1, ConstMemory(vec0, 32));
    u32 value1 = mango::adler32(1, ConstMemory(vec1, 32));
    print(value0, 0x07580071);
    print(value1, 0xf4531de5);

    u64 time0 = Time::us();
    u32 value2 = mango::adler32(1, buffer);
    u64 time1 = Time::us();

    print(value2, 0x44fc8efa);

    return time1 - time0;
}

int main()
{
    printLine(getPlatformInfo());

    constexpr u64 size = 256 * MB;
    Buffer buffer(size);

    for (u64 i = 0; i < size; ++i)
    {
        buffer[i] = i;
    }

    u64 time0 = test_crc32(buffer);
    u64 time1 = test_crc32c(buffer);
    u64 time2 = test_adler32(buffer);

    printLine("");
    print(buffer, "crc32:         ", time0);
    print(buffer, "crc32c:        ", time1);
    print(buffer, "adler32:       ", time2);
    printLine("");
}
