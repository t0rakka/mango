/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>

using namespace mango;

/*
    NOTE!
    The compression API does not do any memory management; all
    input and output buffers have to be allocated by the caller.
*/

void compression_example(size_t size)
{
    // allocate a buffer and initialize it with junk

    Buffer buffer(size);

    for (size_t i = 0; i < size; ++i)
    {
        buffer[i] = ((i + 2) * 0x123456) & 0xff;
    }

    u64 time0 = Time::us();

    // compute maximum compressed size
    size_t bound = zstd::bound(size);

    // allocate compressed buffer
    Buffer compressed(bound);

    // compress
    size_t bytes = zstd::compress(compressed, buffer, 6);

    u64 time1 = Time::us();

    // allocate output buffer
    Buffer output(size);

    // decompress
    // We don't pass the whole compressed buffer but just slice of it
    // from the beginning which is the size of compressed data.
    zstd::decompress(output, Memory(compressed, bytes));

    u64 time2 = Time::us();

    // report results

    float rate0 = size / float(time1 - time0);
    float rate1 = size / float(time2 - time1);

    int result = std::memcmp(buffer, output, size);
    const char* status = result ? "FAILED" : "PASSED";

    printf("compressed %zu bytes to %zu bytes in %d us (%.1f MB/s).\n", size, bytes, u32(time1 - time0), rate0);
    printf("decompressed %zu bytes to %zu bytes in %d us (%.1f MB/s).\n", bytes, size, u32(time2 - time1), rate1);
    printf("status: %s\n", status);
}

int main(int argc, const char* argv[])
{
    const size_t size = 1024 * 1024;
    compression_example(size);
}
