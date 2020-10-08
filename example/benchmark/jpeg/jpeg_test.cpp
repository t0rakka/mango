/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;

// -----------------------------------------------------------------
// pipelined jpeg reader
// -----------------------------------------------------------------

static inline bool isJPEG(const FileInfo& node)
{
    return !node.isDirectory() && filesystem::getExtension(node.name) == ".jpg";
}

void test_jpeg(const std::string& folder, bool mmap)
{
    Path path(folder);
    const size_t count = path.size();

    std::atomic<size_t> total_input_bytes { 0 };
    std::atomic<size_t> total_image_bytes { 0 };

    u64 time0 = Time::ms();

    ConcurrentQueue q;

    for (size_t i = 0; i < count; ++i)
    {
        const auto& node = path[i];
        if (isJPEG(node))
        {
            std::string filename = node.name;
            q.enqueue([=, &path, &total_input_bytes, &total_image_bytes]
            {
                size_t input_bytes = 0;
                size_t image_bytes = 0;

                if (mmap)
                {
                    File file(path, filename);
                    Bitmap bitmap(file, filename);

                    input_bytes = file.size();
                    image_bytes = bitmap.width * bitmap.height * 4; 
                }
                else
                {
                    FileStream file(path.pathname() + filename, Stream::READ);
                    Buffer buffer(file);
                    Bitmap bitmap(buffer, filename);

                    input_bytes = file.size();
                    image_bytes = bitmap.width * bitmap.height * 4; 
                }

                total_input_bytes += input_bytes;
                total_image_bytes += image_bytes;

                printf("Decoded: \"%s\" (%zu KB -> %zu KB).\n", 
                    filename.c_str(), 
                    input_bytes >> 10, image_bytes >> 10);
            });
        }
    }

    q.wait();

    u64 time1 = Time::ms();

    printf("\n%s\n", getSystemInfo().c_str());
    printf("MMAP: %s\n", mmap ? "ENABLED" : "DISABLED");
    printf("Decoded %d files in %d ms (%zu MB -> %zu MB).\n",
        u32(count), u32(time1 - time0), 
        total_input_bytes >> 20, 
        total_image_bytes >> 20);
}

// -----------------------------------------------------------------
// main
// -----------------------------------------------------------------

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printf("Too few arguments. Usage: %s <folder>\n", argv[0]);
        return 1;
    }

    std::string pathname = argv[1];

    bool mmap = false;

    if (argc > 2)
    {
        if (!strcmp(argv[2], "--mmap"))
        {
            mmap = true;
        }
    }

    test_jpeg(pathname, mmap);
}
