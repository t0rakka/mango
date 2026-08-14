/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "core_test.hpp"

using namespace mango;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    bool test_buffer_append_and_growth()
    {
        Buffer buffer;

        for (int i = 0; i < 100; ++i)
        {
            buffer.append(4, u8(i));
        }

        CHECK(buffer.size() == 400);
        CHECK(buffer.capacity() >= 400);

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            CHECK(buffer.data()[i] == u8((i / 4) & 0xff));
        }

        return true;
    }

    bool test_buffer_resize_and_reserve()
    {
        Buffer buffer(8, 0xaa);

        buffer.resize(4);
        CHECK(buffer.size() == 4);
        CHECK(buffer.capacity() >= 4);
        for (size_t i = 0; i < 4; ++i)
        {
            CHECK(buffer.data()[i] == 0xaa);
        }

        buffer.resize(16);
        CHECK(buffer.size() == 16);
        CHECK(buffer.capacity() >= 16);
        for (size_t i = 0; i < 4; ++i)
        {
            CHECK(buffer.data()[i] == 0xaa);
        }

        const size_t capacity_before = buffer.capacity();
        buffer.reserve(capacity_before);
        CHECK(buffer.capacity() == capacity_before);
        CHECK(buffer.size() == 16);

        buffer.reserve(capacity_before + 64);
        CHECK(buffer.capacity() >= capacity_before + 64);
        CHECK(buffer.data()[0] == 0xaa);

        return true;
    }

    bool test_buffer_acquire_release()
    {
        Buffer buffer(16, 0x5a);

        Memory memory = buffer.acquire();
        CHECK(memory.address != nullptr);
        CHECK(memory.size == 16);
        CHECK(buffer.size() == 0);
        CHECK(buffer.capacity() == 0);
        CHECK(buffer.data() == nullptr);

        memory.address[0] = 0x11;
        Buffer::release(memory);

        buffer.reset(4, 0x22);
        CHECK(buffer.size() == 4);
        CHECK(buffer.data()[0] == 0x22);

        return true;
    }

    bool test_memory_stream_seek_and_read()
    {
        const u8 source[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
        ConstMemoryStream stream(ConstMemory(source, sizeof(source)));

        CHECK(stream.size() == 8);
        CHECK(stream.offset() == 0);

        stream.seek(2, Stream::SeekMode::Begin);
        CHECK(stream.offset() == 2);

        u8 value = 0;
        CHECK(stream.read(&value, 1) == 1);
        CHECK(value == 2);

        stream.seek(-1, Stream::SeekMode::Current);
        CHECK(stream.offset() == 2);

        stream.seek(-1, Stream::SeekMode::End);
        CHECK(stream.offset() == 7);

        CHECK(stream.read(&value, 1) == 1);
        CHECK(value == 7);

        CHECK(stream.read(&value, 1) == 0);

        stream.seek(16, Stream::SeekMode::Begin);
        CHECK(stream.offset() == 16);
        CHECK(stream.read(&value, 1) == 0);

        return true;
    }

    bool test_memory_stream_write_past_end()
    {
        MemoryStream stream;

        stream.seek(4, Stream::SeekMode::Begin);
        CHECK(stream.offset() == 4);

        const u8 payload[] = { 0xde, 0xad, 0xbe, 0xef };
        CHECK(stream.write(payload, 4) == 4);
        CHECK(stream.size() == 8);
        CHECK(stream.offset() == 8);

        for (size_t i = 0; i < 4; ++i)
        {
            CHECK(stream.data()[i] == 0);
        }

        CHECK(stream.data()[4] == 0xde);
        CHECK(stream.data()[5] == 0xad);
        CHECK(stream.data()[6] == 0xbe);
        CHECK(stream.data()[7] == 0xef);

        return true;
    }

    bool test_const_memory_stream_is_read_only()
    {
        const u8 source[] = { 1, 2, 3 };
        ConstMemoryStream stream(ConstMemory(source, sizeof(source)));

        bool threw = false;

        try
        {
            stream.write(source, 1);
        }
        catch (const Exception&)
        {
            threw = true;
        }

        CHECK(threw);
        return true;
    }

    bool test_buffer_stream_roundtrip()
    {
        MemoryStream stream;

        {
            BigEndianStream out(stream);
            out.write8(0x11);
            out.write16(0x2233);
            out.write24(0x445566);
            out.write32(0x778899aa);
            out.write64(0xbbccddeeff001122ULL);
        }

        Buffer loaded(stream);
        CHECK(loaded.size() == stream.size());

        ConstMemoryStream input(loaded);
        BigEndianStream in(input);

        CHECK(in.read8() == 0x11);
        CHECK(in.read16() == 0x2233);
        CHECK(in.read24() == 0x445566);
        CHECK(in.read32() == 0x778899aa);
        CHECK(in.read64() == 0xbbccddeeff001122ULL);

        return true;
    }

    const Case g_cases [] =
    {
        { "buffer_append_growth",         test_buffer_append_and_growth },
        { "buffer_resize_reserve",        test_buffer_resize_and_reserve },
        { "buffer_acquire_release",       test_buffer_acquire_release },
        { "memory_stream_seek_read",      test_memory_stream_seek_and_read },
        { "memory_stream_write_past_end", test_memory_stream_write_past_end },
        { "const_memory_stream_read_only", test_const_memory_stream_is_read_only },
        { "buffer_stream_roundtrip",      test_buffer_stream_roundtrip },
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("core_buffer", g_cases, sizeof(g_cases) / sizeof(g_cases[0]), argc, argv);
}
