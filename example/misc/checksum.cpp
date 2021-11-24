/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;

constexpr u64 MB = 1 << 20;

void print(const Buffer& buffer, const char* name, u64 time0, u64 time1, u32 value, u32 reference)
{
    u64 x = buffer.size() * 1000000; // buffer size in bytes * microseconds_in_second
    u32 delta = time1 - time0;
    printf("%s 0x%x %5d.%1d ms (%6d MB/s ) : %s\n", name, value,
        u32(delta/1000), u32(((delta+50)/100)%10), u32(x / (delta * MB)),
        value == reference ? "OK" : "FAILED");
}

void test_crc()
{

    // ... crc32 ...

    printf("CRC32 test vectors: \n");
    printf("\n");

    {
        const char name[] = "123456789";
        const u8* ptr = (const u8*) name;
        
        u32 v0 = mango::crc32(0, ConstMemory(ptr, 9));
        printf("    0x%x : %s\n", v0, v0 == 0xcbf43926 ? "OK" : "FAILED");
    }

    printf("\n");

    // ... crc32c ...

    // RFC 3720 / iSCSI

    printf("CRC32C test vectors: \n");
    printf("\n");

    {
        const u32 sum0 = 0x8a9136aa;
        const u8 vec0[] =
        {
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
        };

        const u32 sum1 = 0x62a8ab43;
        const u8 vec1[] =
        {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        };

        u32 v0 = mango::crc32c(0, ConstMemory(vec0, 32));
        u32 v1 = mango::crc32c(0, ConstMemory(vec1, 32));
        printf("    0x%x : %s\n", v0, v0 == sum0 ? "OK" : "FAILED");
        printf("    0x%x : %s\n", v1, v1 == sum1 ? "OK" : "FAILED");
        printf("\n");
    }

    // performance

    {
        constexpr u64 size = 256 * MB;

        Buffer buffer(size);

        for (u64 i = 0; i < size; ++i)
        {
            buffer[i] = i;
        }

        u64 time0 = Time::us();
        
        u32 v0 = mango::crc32(0, buffer);
        u64 time1 = Time::us();

        u32 v1 = mango::crc32c(0, buffer);
        u64 time2 = Time::us();

        print(buffer, "crc32:  ", time0, time1, v0, 0x9fb22d1f);
        print(buffer, "crc32c: ", time1, time2, v1, 0x1fd9c660);
    }

}

int main()
{
    printf("%s\n", getPlatformInfo().c_str());
    test_crc();
}
