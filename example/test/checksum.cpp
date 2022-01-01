/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;

constexpr u64 MB = 1 << 20;

void print(u32 value, u32 reference)
{
    printf("    0x%.8x : %s\n", value, value == reference ? "OK" : "FAILED");
}

void print(const Buffer& buffer, const char* name, u64 time)
{
    u64 x = buffer.size() * 1000000; // buffer size in bytes * microseconds_in_second
    printf("%s %5d.%1d ms (%6d MB/s )\n", name, u32(time / 1000), u32(((time + 50) / 100) % 10), u32(x / (time * MB)));
}

void test_crc()
{

    constexpr u64 buffer_size = 256 * MB;

    Buffer buffer(buffer_size);

    for (u64 i = 0; i < buffer_size; ++i)
    {
        buffer[i] = i;
    }

    u64 time0 = 0;
    u64 time1 = 0;
    u64 time2 = 0;

    u32 value0 = 0;
    u32 value1 = 0;
    u32 value2 = 0;

    // --------------------------------------------------------------
    // crc32
    // --------------------------------------------------------------

    printf("CRC32 test vectors: \n");
    printf("\n");

    {
        const char name [] = "123456789";
        const u8* ptr = (const u8*) name;
        
        u32 v0 = mango::crc32(0, ConstMemory(ptr, 9));
        print(v0, 0xcbf43926);
    }

    {
        u64 time = Time::us();
        value0 = mango::crc32(0, buffer);
        time0 = Time::us() - time;
    }

    print(value0, 0x9fb22d1f);

    // --------------------------------------------------------------
    // crc32c
    // --------------------------------------------------------------

    // RFC 3720 / iSCSI

    printf("\n");
    printf("CRC32C test vectors: \n");
    printf("\n");

    {
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

        u32 v0 = mango::crc32c(0, ConstMemory(vec0, 32));
        u32 v1 = mango::crc32c(0, ConstMemory(vec1, 32));
        print(v0, 0x8a9136aa);
        print(v1, 0x62a8ab43);
    }

    {
        u64 time = Time::us();
        value1 = mango::crc32c(0, buffer);
        time1 = Time::us() - time;
    }

    print(value1, 0x1fd9c660);

    // --------------------------------------------------------------
    // adler32
    // --------------------------------------------------------------

    printf("\n");
    printf("ADLER32 test vectors: \n");
    printf("\n");

    {
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
            0x80, 0xff, 0x80, 0xff, 0x80, 0xff, 0x80, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        };

        u32 v0 = mango::adler32(1, ConstMemory(vec0, 32));
        u32 v1 = mango::adler32(1, ConstMemory(vec1, 32));
        print(v0, 0x00200001);
        print(v1, 0xf4531de5);
    }

    {
        u64 time = Time::us();
        value2 = mango::adler32(1, buffer);
        time2 = Time::us() - time;
    }

    // --------------------------------------------------------------
    // results
    // --------------------------------------------------------------

    printf("\n");
    print(buffer, "crc32:   ", time0);
    print(buffer, "crc32c:  ", time1);
    print(buffer, "adler32: ", time2);

    printf("\n");

}

int main()
{
    printf("%s\n", getPlatformInfo().c_str());
    test_crc();
}
