/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>

using namespace mango;

void example1(const u8* p)
{
    // unaligned little-endian loads
    // address must be manually computed and updated
    u16 a = littleEndian::uload16(p + 0);
    u16 b = littleEndian::uload16(p + 2);
    u32 c = littleEndian::uload32(p + 4);
    p += 8;

    MANGO_UNREFERENCED(a);
    MANGO_UNREFERENCED(b);
    MANGO_UNREFERENCED(c);
}

void example2(u8* address)
{
    // same as example1 but using "endian aware pointers"
    LittleEndianPointer p = address;
    u16 a = p.read16();
    u16 b = p.read16();
    u32 c = p.read32();

    MANGO_UNREFERENCED(a);
    MANGO_UNREFERENCED(b);
    MANGO_UNREFERENCED(c);
}

void example3(LittleEndianPointer& p)
{
    float f = p.read32f();
    p += 20; // skip 20 bytes
    float16 h = p.read16f();
    u64 a = p.read64();

    MANGO_UNREFERENCED(f);
    MANGO_UNREFERENCED(h);
    MANGO_UNREFERENCED(a);
}

void example4(Stream& stream)
{
    // Initialize endian aware streaming translator
    LittleEndianStream s = stream;

    // The translator extends Stream interface to understand endianess..
    u32 a = s.read32();
    float b = s.read32f();
    s.seek(20, Stream::CURRENT); // skip 20 bytes

    // read a block of memory
    u32 size = s.read32();
    std::vector<char> buffer(size);
    s.read(buffer.data(), size);

    MANGO_UNREFERENCED(a);
    MANGO_UNREFERENCED(b);
}

// Wait! There's more! If you clone MANGO from github.com now you will get endianess-aware types for FREE!
// NO ADDITIONAL CHARGE!

struct SomeHeader
{
    bigEndian::u16 a;
    bigEndian::u16 b;
    bigEndian::u32 c;
};

void example5(const u8* p)
{
    // reinterpret raw storage as SomeHeader
    const SomeHeader& header = *reinterpret_cast<const SomeHeader *>(p);

    // convert-on-read endianess conversion
    int a = header.a;
    int b = header.b;
    int c = header.c;

    MANGO_UNREFERENCED(a);
    MANGO_UNREFERENCED(b);
    MANGO_UNREFERENCED(c);
}

/*
    All of the streaming interfaces are super light-weight and
    every decision is done at compile time; the correct code is called
    directly without any dispatching logic.

    This line of code:
    u32 x = p.read32();

    Would see that we are reading from, say, Little Endian pointer and
    that we are compiling for little endian architecture, thus no
    translation will be required. The data will be fetched using
    unaligned memory read operation. The load operation is selected using
    compile time information about the architecture and how it handles
    alignment. It will just work. However, if the client can guarantee
    correct alignment the memory load operation could potentially be
    more efficient.

    This is equal to doing this:
    char* p = ...;
    u32 x = *reinterpret_cast<u32 *>(p);
    p += sizeof(u32);

    The difference is that our way is less typing and always guaranteed
    to be correct and use the best available implementation.
*/

int main()
{
    // NOTE: This code is compiled for validation purposes
}
