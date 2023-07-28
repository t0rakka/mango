/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;

constexpr u64 MB = 1 << 20;

void validate_sha1()
{

    /*
    const u8 message[] = "abc";
    ConstMemory memory(message, sizeof(message));
    SHA1 v = mango::sha1(memory);

    for (int i = 0; i < 5; ++i)
    {
        printf("%x ", v.data[i]);
    }
    printf("\n");
    printf("\n");
    */

    // message: "abc"
    // sha1: a9993e36 4706816a ba3e2571 7850c26c 9cd0d89d
    // sha2: ba7816bf 8f01cfea 414140de 5dae2223 b00361a3 96177a9c b410ff61 f20015ad

    // message: ""
    // sha1: da39a3ee 5e6b4b0d 3255bfef 95601890 afd80709
    // sha2: e3b0c442 98fc1c14 9afbf4c8 996fb924 27ae41e4 649b934c a495991b 7852b855

    // message: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
    // sha1: 84983e44 1c3bd26e baae4aa1 f95129e5 e54670f1
    // sha2: 248d6a61 d20638b8 e5c02693 0c3e6039 a33ce459 64ff2167 f6ecedd4 19db06c1

    // message: "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
    // sha1: a49b2446 a02c645b f419f995 b6709125 3a04a259
    // sha2: cf5b16a7 78af8380 036ce59e 7b049237 0b249b11 e8f07a51 afac4503 7afee9d1

    // message: 1,000,000 times 0x61
    // sha1: 34aa973c d4c4daa4 f61eeb2b dbad2731 6534016f
    // sha2: cdc76e5c 9914fb92 81a1c7e2 84d73e67 f1809a48 a497200e 046d39cc c7112cd0
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

    validate_sha1();

    test_md5(buffer);
    test_sha1(buffer);
    test_sha2(buffer);
    test_xxhash32(buffer);
    test_xxhash64(buffer);
    test_xx3hash64(buffer);
    test_xx3hash128(buffer);
}
