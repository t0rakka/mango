/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;

void test_aes()
{

    // FIPS 197, Appendix B input
    const uint8_t input[16] =
    {
        0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d, 0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34
    };

    // FIPS 197, Appendix B key
    const uint8_t key[16] =
    {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x9 , 0xcf, 0x4f, 0x3c
    };

    // FIPS 197, Appendix B output
    const uint8_t expected[16] =
    {
        0x39, 0x25, 0x84, 0x1D, 0x02, 0xDC, 0x09, 0xFB, 0xDC, 0x11, 0x85, 0x97, 0x19, 0x6A, 0x0B, 0x32
    };

    AES aes(key, 128);

    u8 result[16];
    aes.ecb_block_encrypt(result, input, 16);

    if (!memcmp(result, expected, 16))
    {
        printf("Status: OK\n");
    }
    else
    {
        printf("Status: FAILED\n");
    }

    // performance

    {
        constexpr u64 MB = 1 << 20;
        constexpr u64 size = 256 * MB;

        Buffer buffer(size);
        Buffer output(size);
        Buffer temp(size);

        for (u64 i = 0; i < size; ++i)
        {
            buffer[i] = i;
        }

        u64 time0 = Time::ms();

        aes.ecb_encrypt(temp, buffer, size);

        u64 time1 = Time::ms();

        aes.ecb_decrypt(output, temp, size);

        u64 time2 = Time::ms();

        u64 delta1 = std::max(u64(1), time1 - time0);
        u64 delta2 = std::max(u64(1), time2 - time1);

        u64 x = buffer.size() * 1000;
        printf("aes encrypt: %4d ms (%6d MB/s)\n", u32(delta1), u32(x / (delta1 * MB)));
        printf("aes decrypt: %4d ms (%6d MB/s)\n", u32(delta2), u32(x / (delta2 * MB)));

        if (!memcmp(output, buffer, size))
        {
            printf("Buffer: PASSED\n");
        }
        else
        {
            printf("Buffer: FAILED\n");
        }

    }
}

int main()
{
    test_aes();
}
