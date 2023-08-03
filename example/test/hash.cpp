/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;

constexpr u64 MB = 1 << 20;

template <typename T>
void print(T hash)
{
    for (size_t i = 0; i < sizeof(hash.data) / sizeof(hash.data[0]); ++i)
    {
        printf("%.8x ", hash.data[i]);
    }
}

template <typename T>
void check(T hash, T correct)
{
    const size_t size = sizeof(hash.data) / sizeof(hash.data[0]);

    bool success = true;

    for (size_t i = 0; i < size; ++i)
    {
        if (hash.data[i] != byteswap(correct.data[i]))
        {
            success = false;
            break;
        }
    }

    printf("  ");
    print(hash);
    if (!success)
        printf(" : FAILED");
    printf("\n");

    if (!success)
    {
        printf("  ");
        for (size_t i = 0; i < size; ++i)
        {
            printf("%.8x ", byteswap(correct.data[i]));
        }
        printf(" < Reference\n");
    }
}

void validate(Buffer& message5)
{
    const u8 message0 [] = "abc";
    const u8 message1 [] = "";
    const u8 message2 [] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    const u8 message3 [] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";

    ConstMemory memory0(message0, sizeof(message0) - 1);
    ConstMemory memory1(message1, sizeof(message1) - 1);
    ConstMemory memory2(message2, sizeof(message2) - 1);
    ConstMemory memory3(message3, sizeof(message3) - 1);
    Buffer message4(1000000, 0x61);

    printf("SHA1 test vectors:\n");
    printf("\n");

    check(mango::sha1(memory0), { 0xa9993e36, 0x4706816a, 0xba3e2571, 0x7850c26c, 0x9cd0d89d });
    check(mango::sha1(memory1), { 0xda39a3ee, 0x5e6b4b0d, 0x3255bfef, 0x95601890, 0xafd80709 });
    check(mango::sha1(memory2), { 0x84983e44, 0x1c3bd26e, 0xbaae4aa1, 0xf95129e5, 0xe54670f1 });
    check(mango::sha1(memory3), { 0xa49b2446, 0xa02c645b, 0xf419f995, 0xb6709125, 0x3a04a259 });
    check(mango::sha1(message4), { 0x34aa973c, 0xd4c4daa4, 0xf61eeb2b, 0xdbad2731, 0x6534016f });
    check(mango::sha1(message5), { 0x37a6b201, 0x48116c58, 0x4c875f2a, 0xb963248a, 0x630d6aad });

    printf("\n");
    printf("SHA2 test vectors:\n");
    printf("\n");

    check(mango::sha2(ConstMemory(message0, sizeof(message0) - 1)), { 0xba7816bf, 0x8f01cfea, 0x414140de, 0x5dae2223, 0xb00361a3, 0x96177a9c, 0xb410ff61, 0xf20015ad });
    check(mango::sha2(ConstMemory(message1, sizeof(message1) - 1)), { 0xe3b0c442, 0x98fc1c14, 0x9afbf4c8, 0x996fb924, 0x27ae41e4, 0x649b934c, 0xa495991b, 0x7852b855 });
    check(mango::sha2(ConstMemory(message2, sizeof(message2) - 1)), { 0x248d6a61, 0xd20638b8, 0xe5c02693, 0x0c3e6039, 0xa33ce459, 0x64ff2167, 0xf6ecedd4, 0x19db06c1 });
    check(mango::sha2(ConstMemory(message3, sizeof(message3) - 1)), { 0xcf5b16a7, 0x78af8380, 0x036ce59e, 0x7b049237, 0x0b249b11, 0xe8f07a51, 0xafac4503, 0x7afee9d1 });
    check(mango::sha2(message4), { 0xcdc76e5c, 0x9914fb92, 0x81a1c7e2, 0x84d73e67, 0xf1809a48, 0xa497200e, 0x046d39cc, 0xc7112cd0 });
    check(mango::sha2(message5), { 0x486cc817, 0xb95d853d, 0x3c357ff2, 0x83b204c0, 0x144bd255, 0xe73fe2de, 0xb1389493, 0xb257e3c0 });

    printf("\n");
}

void print(const Buffer& buffer, const char* name, u64 time0, u64 time1, u32 value)
{
    u64 x = buffer.size() * 1000000; // buffer size in bytes * microseconds_in_second
    u32 delta = time1 - time0;
    printf("%s 0x%.8x %5d.%1d ms (%6d MB/s )\n", name, value, u32(delta/1000), u32(((delta+50)/100)%10), u32(x / (delta * MB)));
}

void test_md5(const Buffer& buffer)
{
    u64 time0 = Time::us();
    
    MD5 v = mango::md5(buffer);
    u64 time1 = Time::us();

    print(buffer, "md5:        ", time0, time1, v[0]);
}

void test_sha1(const Buffer& buffer)
{
    u64 time0 = Time::us();

    SHA1 v = mango::sha1(buffer);
    u64 time1 = Time::us();

    print(buffer, "sha1:       ", time0, time1, v[0]);
}

void test_sha2(const Buffer& buffer)
{
    u64 time0 = Time::us();
    
    SHA2 v = mango::sha2(buffer);
    u64 time1 = Time::us();

    print(buffer, "sha2:       ", time0, time1, v[0]);
}

void test_xxhash32(const Buffer& buffer)
{
    u64 time0 = Time::us();

    u64 seed = 0;
    u32 v = mango::xxhash32(seed, buffer);
    u64 time1 = Time::us();

    print(buffer, "xxhash32:   ", time0, time1, v);
}

void test_xxhash64(const Buffer& buffer)
{
    u64 time0 = Time::us();

    u64 seed = 0;
    u64 v = mango::xxhash32(seed, buffer);
    u64 time1 = Time::us();

    print(buffer, "xxhash64:   ", time0, time1, u32(v));
}

void test_xx3hash64(const Buffer& buffer)
{
    u64 time0 = Time::us();

    u64 seed = 0;
    u64 v = mango::xx3hash64(seed, buffer);
    u64 time1 = Time::us();

    print(buffer, "xx3hash64:  ", time0, time1, u32(v));
}

void test_xx3hash128(const Buffer& buffer)
{
    u64 time0 = Time::us();

    u64 seed = 0;
    mango::XX3H128 v = mango::xx3hash128(seed, buffer);
    u64 time1 = Time::us();

    print(buffer, "xx3hash128: ", time0, time1, u32(v[0]));
}

int main()
{
    constexpr u64 size = 256 * MB;

    Buffer buffer(size);

    for (u64 i = 0; i < size; ++i)
    {
        buffer[i] = i;
    }

    printf("%s\n", getPlatformInfo().c_str());
    validate(buffer);

    test_md5(buffer);
    test_sha1(buffer);
    test_sha2(buffer);
    test_xxhash32(buffer);
    test_xxhash64(buffer);
    test_xx3hash64(buffer);
    test_xx3hash128(buffer);
}
