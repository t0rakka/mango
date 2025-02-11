/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>

using namespace mango;

void test_compression(size_t size, int level)
{
    Buffer buffer(size);

    // Create the best possible test data (this is fairly impressive achievement)
    int xd = rand();
    for (size_t i = 0; i < size; ++i)
    {
        buffer[i] = ((i + 2) * 0x123456) & 0xff;
        buffer[i] += xd;
        if (!(i % 5)) xd = rand();
    }

    printLine("------------------------------------------------------------");
    printLine("Method      Ratio        Compress       Decompress    Status");
    printLine("------------------------------------------------------------");

    std::vector<Compressor> compressors = getCompressors();

    for (const auto& compressor : compressors)
    {
        if (compressor.name.find('.') != std::string::npos)
        {
            // filter out compressor where name contains "."; those are just different variations
            continue;
        }

        // compute maximum compressed size
        size_t bound = compressor.bound(size);

        // allocate compressed buffer
        Buffer compressed(bound);

        // allocate output buffer
        Buffer output(size);

        u64 time0 = Time::us();

        // compress
        size_t bytes = compressor.compress(compressed, buffer, level);

        u64 time1 = Time::us();

        // decompress
        compressor.decompress(output, Memory(compressed, bytes));

        u64 time2 = Time::us();

        bool correct = std::memcmp(buffer, output, size) == 0;
        const char* status = correct ? "PASSED" : "FAILED";

        float ratio = bytes * 100.0f / size;
        float rate0 = size / float(time1 - time0);
        float rate1 = size / float(time2 - time1);

        printLine("{:<9} {:>6.1f}% {:>10.1f} MB/s {:>11.1f} MB/s    {}", compressor.name, ratio, rate0, rate1, status);
    }
}

int main(int argc, const char* argv[])
{
    const size_t size = 1024 * 1024;
    int level = 4;

    if (argc == 2)
    {
        level = std::atoi(argv[1]);
    }

    test_compression(size, level);
}
