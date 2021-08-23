/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;

constexpr u64 MB = 1 << 20;

void print(const Buffer& buffer, const char* name, u64 time0, u64 time1, u32 value)
{
    u64 x = buffer.size() * 1000000; // buffer size in bytes * microseconds_in_second
    u32 delta = time1 - time0;
    printf("%s 0x%x %5d.%1d ms (%6d MB/s )\n", name, value, u32(delta/1000), u32(((delta+50)/100)%10), u32(x / (delta * MB)));
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

    test_md5(buffer);
    test_sha1(buffer);
    test_sha2(buffer);
    test_xxhash32(buffer);
    test_xxhash64(buffer);
    test_xx3hash64(buffer);
    test_xx3hash128(buffer);
}
