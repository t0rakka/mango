/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>

using namespace mango;

constexpr u64 MB = 1 << 20;
constexpr int N = 80;
constexpr u64 BUFFER_SIZE = 32 * MB;
constexpr int BAR_WIDTH = 32;

void print(const Buffer& buffer, u64 time0, u64 time1)
{
    u64 x = N * u64(buffer.size()) * 1000000; // buffer size in bytes * microseconds_in_second
    u32 delta = time1 - time0;
    printf("%5d.%1d ms (%6d MB/s )\n",
        u32(delta / 1000),
        u32(((delta + 50) / 100)%10),
        u32(x / (delta * MB)));
}

void progress(int i)
{
    // fraction done
    double fraction = double(i + 1) / N;
    int filled = int(fraction * BAR_WIDTH);

    // build bar
    printf("\r[");
    for (int j = 0; j < BAR_WIDTH; j++)
    {
        putchar(j < filled ? '=' : ' ');
    }
    printf("] %5.1f%%\r", fraction * 100.0);
    fflush(stdout);
}

void test_fips()
{
    // FIPS 197, Appendix B input
    const u8 input[16] =
    {
        0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d, 0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34
    };

    // FIPS 197, Appendix B key
    const u8 key[16] =
    {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x9 , 0xcf, 0x4f, 0x3c
    };

    // FIPS 197, Appendix B output
    const u8 expected[16] =
    {
        0x39, 0x25, 0x84, 0x1D, 0x02, 0xDC, 0x09, 0xFB, 0xDC, 0x11, 0x85, 0x97, 0x19, 0x6A, 0x0B, 0x32
    };

    AES aes(key, 128);

    u8 result[16];
    aes.ecb_block_encrypt(result, input, 16);

    if (!memcmp(result, expected, 16))
    {
        printLine("Status: OK\n");
    }
    else
    {
        printLine("Status: FAILED\n");
    }
}

void test_aes(int bits)
{
    const u8 key[] =
    {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x9 , 0xcf, 0x4f, 0x3c,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };

    AES aes(key, bits);

    constexpr u64 size = BUFFER_SIZE;

    Buffer buffer(size);
    Buffer output(size);
    Buffer temp(size);

    for (u64 i = 0; i < size; ++i)
    {
        buffer[i] = i;
    }

    u64 time0 = Time::us();

    for (int i = 0; i < N; ++i)
    {
        progress(i);
        aes.ecb_encrypt(temp, buffer, size);
    }

    u64 time1 = Time::us();

    printf("\r\033[K"); // clear line
    printf("aes%d encrypt: ", bits);
    print(buffer, time0, time1);


    for (int i = 0; i < N; ++i)
    {
        progress(i);
        aes.ecb_decrypt(output, temp, size);
    }

    u64 time2 = Time::us();

    printf("\r\033[K"); // clear line
    printf("aes%d decrypt: ", bits);
    print(buffer, time1, time2);

    if (!memcmp(output, buffer, size))
    {
        printf("AES%d: PASSED\n\n", bits);
    }
    else
    {
        printf("AES%d: FAILED\n\n", bits);
    }
}

int main()
{
    printLine(getPlatformInfo());
    test_fips();
    test_aes(128);
    test_aes(192);
    test_aes(256);
}
