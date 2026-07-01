/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <cstring>
#include <mango/core/core.hpp>
#include <mango/filesystem/filesystem.hpp>

using namespace mango;
using namespace mango::filesystem;

static void collect_files(const Path& path, std::string prefix, std::vector<std::string>& filenames)
{
    for (const FileInfo& node : path)
    {
        if (node.isDirectory())
        {
            if (node.isContainer())
            {
                continue;
            }

            collect_files(Path(path, node.name), prefix + node.name + "/", filenames);
        }
        else
        {
            filenames.push_back(prefix + node.name);
        }
    }
}

static void load_folder(const std::string& folder, Buffer& buffer)
{
    std::string pathname = folder;
    if (!pathname.empty() && pathname.back() != '/')
    {
        pathname += '/';
    }

    Path path(pathname);

    std::vector<std::string> filenames;
    collect_files(path, "", filenames);
    std::sort(filenames.begin(), filenames.end());

    for (const std::string& filename : filenames)
    {
        File file(path, filename);
        buffer.append(file);
    }
}

static void print_help(const char* program)
{
    printLine("help");
    printLine("Usage: {} <folder> [level]", program);
    printLine("  folder  Directory whose files are loaded and compressed (recursive).");
    printLine("  level   Compression level (default: 4).");
}

void test_compression(ConstMemory input, int level)
{
    const size_t size = input.size;

    if (!size)
    {
        printLine("No file data loaded; nothing to compress.");
        return;
    }

    printLine("Input: {} bytes ({:.2f} MB)", size, size / (1024.0 * 1024.0));
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

        size_t bound = compressor.bound(size);
        Buffer compressed(bound);
        Buffer output(size);

        u64 time0 = Time::us();

        size_t bytes = compressor.compress(compressed, input, level);

        u64 time1 = Time::us();

        compressor.decompress(output, Memory(compressed, bytes));

        u64 time2 = Time::us();

        bool correct = std::memcmp(input.address, output, size) == 0;
        const char* status = correct ? "PASSED" : "FAILED";

        float ratio = bytes * 100.0f / size;
        float rate0 = size / float(time1 - time0);
        float rate1 = size / float(time2 - time1);

        printLine("{:<9} {:>6.1f}% {:>10.1f} MB/s {:>11.1f} MB/s    {}", compressor.name, ratio, rate0, rate1, status);
    }
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        print_help(argv[0]);
        return 1;
    }

    const std::string folder = argv[1];
    int level = 4;

    if (argc >= 3)
    {
        level = std::atoi(argv[2]);
    }

    try
    {
        Buffer buffer;
        load_folder(folder, buffer);
        test_compression(buffer, level);
    }
    catch (const std::exception& e)
    {
        printLine("Exception: {}", e.what());
        return 1;
    }

    return 0;
}
