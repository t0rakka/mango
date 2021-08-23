/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;

void test_crc()
{

    // ... crc32 ...

    printf("\n");
    printf("CRC32 test vectors: \n");
    printf("\n");

    {
        const char name[] = "123456789";
        const u8* ptr = (const u8*) name;
        
        u32 v0 = mango::crc32(0, ConstMemory(ptr, 9));
        printf("    0x%x : %s\n", v0, v0 == 0xcbf43926 ? "OK" : "FAILED");
    }

    // ... crc32c ...

    // RFC 3720 / iSCSI

    printf("\n");
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
        constexpr u64 MB = 1 << 20;
        constexpr u64 size = 256 * MB;

        Buffer buffer(size);

        for (u64 i = 0; i < size; ++i)
        {
            buffer[i] = i;
        }

        u64 time0 = Time::ms();
        
        u32 v0 = mango::crc32(0, buffer);
        u64 time1 = Time::ms();

        u32 v1 = mango::crc32c(0, buffer);
        u64 time2 = Time::ms();

        u64 x = buffer.size() * 1000; // buffer size in bytes * milliseconds_in_second
        printf("crc32:     0x%x %4d ms (%6d MB/s )\n", v0, u32(time1 - time0), u32(x / ((time1 - time0) * MB)));
        printf("crc32c:    0x%x %4d ms (%6d MB/s )\n", v1, u32(time2 - time1), u32(x / ((time2 - time1) * MB)));
    }

}

int main()
{
    printf("%s", getPlatformInfo().c_str());
    test_crc();
}
