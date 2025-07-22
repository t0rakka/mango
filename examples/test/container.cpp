/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;

static inline
std::string tabs(int depth)
{
    return std::string(depth * 2, ' ');
}

u64 g_file_count = 0;
u64 g_directory_count = 0;
u64 g_container_count = 0;

void enumerateDirectory(const Path& path, std::string prefix, int depth)
{
    for (auto& node : path)
    {
        if (node.isDirectory())
        {
            printLine("{}+ {}", tabs(depth), node.name);
            ++g_directory_count;

            if (node.isContainer())
            {
                ++g_container_count;
            }
            else
            {
                std::string subpath_str = path.pathname() + node.name;
                Path subpath(subpath_str);
                enumerateDirectory(subpath, prefix + node.name + "/", depth + 1);
            }
        }
        else
        {
            printLine("{}- {} ({} KB)", tabs(depth), node.name, node.size / 1024);
            ++g_file_count;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printLine("Usage: {} <container/folder>", argv[0]);
        return 1;
    }

    std::string container = argv[1];
    //mango::printEnable(mango::Print::Info, true);

    try
    {
        Path path(container + "/");
        enumerateDirectory(path, "", 0);

        printLine("");
        printLine("Files: {}, Directories: {}, Containers: {}", g_file_count, g_directory_count, g_container_count);
    }
    catch (const std::exception& e)
    {
        printLine("Exception: {}", e.what());
        return 1;
    }

    return 0;
}
