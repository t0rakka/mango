/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;
using namespace mango::image;

// -----------------------------------------------------------------
// pipelined image reader
// -----------------------------------------------------------------

struct State
{
    std::atomic<size_t> total_input_files { 0 };
    std::atomic<size_t> total_input_bytes { 0 };
    std::atomic<size_t> total_image_bytes { 0 };

    FileIndex index;

    ConcurrentQueue queue;
    Trace trace { "", "batch image reading" };

    void decode(ConstMemory memory, const std::string& filename, bool multithread)
    {
        size_t image_bytes = 0;
        size_t input_bytes = 0;

        ImageDecoder decoder(memory, filename);
        if (decoder.isDecoder())
        {
            ImageHeader header = decoder.header();
            Bitmap bitmap(header.width, header.height, header.format);

            ImageDecodeOptions options;
            options.simd = true;
            options.multithread = multithread;

            ImageDecodeStatus status = decoder.decode(bitmap, options);
            if (!status)
            {
                printLine("  ERROR: {}", status.info);
            }

            input_bytes = memory.size;
            image_bytes = header.width * header.height * 4;
        }

        total_input_files ++;
        total_input_bytes += input_bytes;
        total_image_bytes += image_bytes;

        Trace trace("", "print");
        printLine("Decoded: \"{}\" ({} KB -> {} KB).", filename, input_bytes >> 10, image_bytes >> 10);
    }

    bool isImageFormat(const FileInfo& node, const std::string& format) const
    {
        std::string extension = mango::toLower(filesystem::getExtension(node.name));
        if (isImageDecoder(extension))
        {
            if (node.isDirectory())
            {
                return false;
            }

            return format.empty() ? true : extension == format;
        }

        return false;
    }

    void scan(const Path& path, const std::string& format)
    {
        for (auto node : path)
        {
            if (node.isDirectory() && !node.isContainer())
            {
                Path child(path, node.name);
                scan(child, format);
            }
            else
            {
                if (isImageFormat(node, format))
                {
                    FileInfo info;

                    info.size = node.size;
                    info.flags = node.flags;
                    info.name = path.pathname() + node.name;

                    index.files.push_back(info);
                }
            }
        }
    }

    void process(bool mmap, bool multithread)
    {
        // sort files by size; the largest decoding tasks should start first
        std::sort(index.begin(), index.end(), [] (const FileInfo& a, const FileInfo& b)
        {
            return a.size > b.size;
        });

        for (auto node : index)
        {
            const std::string& filename = node.name;

            if (mmap)
            {
                queue.enqueue([this, filename, multithread]
                {
                    File file(filename);
                    decode(file, filename, multithread);
                });
            }
            else
            {
                queue.enqueue([this, filename, multithread]
                {
                    Trace trace("", fmt::format("load:{}", filesystem::removePath(filename)));
                    InputFileStream file(filename);
                    Buffer buffer(file);
                    trace.stop();

                    decode(buffer, filename, multithread);
                });
            }
        }
    }

    void wait()
    {
        queue.wait();
        trace.stop();
    }
};

void test(const std::string& folder, const std::string& format, bool mmap, bool multithread)
{
    u64 time0 = Time::ms();

    State state;

    Path path(folder);
    state.scan(path, format);

    u64 time1 = Time::ms();
    printLine("Scanning: {} ms", time1 - time0);
    printLine("");

    state.process(mmap, multithread);
    state.wait();

    u64 time2 = Time::ms();

    printLine("");
    printLine("{}", getSystemInfo());
    printLine("MMAP: {}", mmap);
    printLine("MT:   {}", multithread);
    printLine("");

    printLine("Decoded {} files in {} ms ({} MB -> {} MB).",
        size_t(state.total_input_files),
        time2 - time1,
        state.total_input_bytes >> 20,
        state.total_image_bytes >> 20);
    printLine("");
}

// -----------------------------------------------------------------
// main
// -----------------------------------------------------------------

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printLine("Usage: {} <folder>", argv[0]);
        printLine("Options:");
        printLine("    -format <extension>  : select specific format");
        printLine("    --trace              : enable tracing");
        printLine("    --debug              : enable debug info");
        printLine("    --mmap               : enable memory mapping");
        printLine("    --mt                 : enable multi-threaded decoding");
        return 1;
    }

    std::string pathname = argv[1];

    // defaults
    std::string format;
    bool mmap = false;
    bool multithread = false;
    bool tracing = false;

    for (int i = 2; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--mmap"))
        {
            mmap = true;
        }
        else if (!strcmp(argv[i], "--mt"))
        {
            multithread = true;
        }
        else if (!strcmp(argv[i], "--debug"))
        {
            printEnable(Print::Info, true);
        }
        else if (!strcmp(argv[i], "--trace"))
        {
            tracing = true;
        }
        else if (!strcmp(argv[i], "-format"))
        {
            ++i;
            if (i < argc)
            {
                format = argv[i];
            }
            else
            {
                printLine("ERROR: missing extension parameter.");
                return 0;
            }
        }
    }

    std::unique_ptr<filesystem::OutputFileStream> output;

    if (tracing)
    {
        output = std::make_unique<filesystem::OutputFileStream>("result.trace");
        startTrace(output.get());
    }

    test(pathname, format, mmap, multithread);

    if (tracing)
    {
        stopTrace();
    }
}
