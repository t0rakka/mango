/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;
using namespace mango::image;

// -----------------------------------------------------------------
// pipelined jpeg reader
// -----------------------------------------------------------------

static
inline bool isJPEG(const FileInfo& node)
{
    std::string ext = mango::toLower(filesystem::getExtension(node.name));
    return !node.isDirectory() && (ext == ".jpg" || ext == ".jpeg");
}

struct State
{
    std::atomic<size_t> total_input_files { 0 };
    std::atomic<size_t> total_input_bytes { 0 };
    std::atomic<size_t> total_image_bytes { 0 };

    ConcurrentQueue queue;

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

        printLine("Decoded: \"{}\" ({} KB -> {} KB).", filename, input_bytes >> 10, image_bytes >> 10);
    }

    void process(const Path& path, bool mmap, bool multithread)
    {
        for (auto node : path)
        {
            if (node.isDirectory())
            {
                if (!node.isContainer())
                {
                    Path child(path, node.name);
                    process(child, mmap, multithread);
                }
            }
            else
            {
                if (isJPEG(node))
                {
                    std::string filename = path.pathname() + node.name;

                    if (mmap)
                    {
                        queue.enqueue([this, filename, multithread]
                        {
                            File file(filename);

                            // decode directly from memory mapped file
                            decode(file, filename, multithread);
                        });
                    }
                    else
                    {
                        queue.enqueue([this, filename, multithread]
                        {
                            InputFileStream file(filename);
                            Buffer buffer(file);

                            // decode from a buffer
                            decode(buffer, filename, multithread);
                        });
                    }
                }
            }
        }
    }
};

void test_jpeg(const std::string& folder, bool mmap, bool multithread)
{
    State state;

    u64 time0 = Time::ms();

    Path path(folder);

    state.process(path, mmap, multithread);
    state.queue.wait();

    u64 time1 = Time::ms();

    printLine("\n{}", getSystemInfo());
    printLine("MMAP: {}", mmap ? "ENABLED" : "DISABLED");
    printLine("MT: {}", multithread ? "ENABLED" : "DISABLED");
    printLine("");
    printLine("Decoded {} files in {} ms ({} MB -> {} MB).",
        size_t(state.total_input_files),
        time1 - time0,
        state.total_input_bytes >> 20,
        state.total_image_bytes >> 20);
}

// -----------------------------------------------------------------
// main
// -----------------------------------------------------------------

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printLine("Too few arguments. Usage: {} <folder>", argv[0]);
        return 1;
    }

    std::string pathname = argv[1];

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
    }

    std::unique_ptr<filesystem::OutputFileStream> output;

    if (tracing)
    {
        output = std::make_unique<filesystem::OutputFileStream>("result.trace");
        startTrace(output.get());
    }

    test_jpeg(pathname, mmap, multithread);

    if (tracing)
    {
        stopTrace();
    }
}
