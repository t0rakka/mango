/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <filesystem>
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;

struct State
{
    bool print_list { false };
    bool print_tree { false };
    bool verify { false };
    bool decompress { false };
    u64 total_bytes_out = 0;
};

void tabs(int depth)
{
    while (depth-- > 0)
        printf("  ");
}

void enumerate(const Path& path, State& state, std::string destination, std::string prefix, int depth)
{
    std::string pathname = removePrefix(path.pathname(), prefix);
    std::string current = destination + pathname;

    if (state.decompress)
    {
        printf("Folder: %s\n", current.c_str());

        // create folder
        bool status = std::filesystem::create_directory(current);
        MANGO_UNREFERENCED(status);
    }

    for (auto& node : path)
    {
        if (node.isDirectory())
        {
            if (!node.isContainer())
            {
                if (state.print_tree)
                {
                    tabs(depth);
                    printf("+ %s\n", node.name.c_str());
                }

                Path temp(path, node.name);
                enumerate(temp, state, current, prefix + pathname, depth + 1);
            }
        }
        else
        {
            std::string filename = current + node.name;

            if (state.print_list)
            {
                printf("%12" PRIu64 "  %s\n", node.size, filename.c_str());
            }

            if (state.print_tree)
            {
                tabs(depth);
                printf("- %s\n", node.name.c_str());
            }

            if (state.verify)
            {
                // NOTE: Stored files don't have a checksum (0), this is lame so it must be improved in the future
                if (node.checksum)
                {
                    File file(path, node.name);
                    u32 checksum = crc32c(0, file);

                    if (checksum != node.checksum)
                    {
                        printf("ERROR: %s checksum: %08x, expected: %08x\n", filename.c_str(), checksum, node.checksum);
                    }
                }
            }

            if (state.decompress)
            {
                printf("Create: %s\n", filename.c_str());

                File file(path, node.name);
                ConstMemory memory = file;

                // create file
                OutputFileStream outfile(filename);
                outfile.write(memory.address, memory.size);
            }

            state.total_bytes_out += node.size;
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::string program_name = removePath(argv[0]);
        printf("\n");
        printf("MGX/SNITCH Decompression Tool version 0.2 \n");
        printf("Copyright (C) 2018 Fapware, inc. All rights reserved.\n");
        printf("Usage: %s [archive] [destination] \n", program_name.c_str());
        printf("       %s [archive] --list \n", program_name.c_str());
        printf("       %s [archive] --tree \n", program_name.c_str());
        printf("       %s [archive] --verify \n", program_name.c_str());
        return 0;
    }

    std::string filename = argv[1];
    std::string destination = argv[2];

    State state;

    if (destination.empty())
    {
        return 0;
    }
    else if (destination == "--verify")
    {
        state.verify = true;
        destination = "";
    }
    else if (destination == "--list")
    {
        state.print_list = true;
        destination = "";
    }
    else if (destination == "--tree")
    {
        state.print_tree = true;
    }
    else
    {
        state.decompress = true;
        destination += "/";
    }

    // Create a memory view of the container
    File file(filename);
    Path path(file, ".snitch");

    Timer timer;
    u64 time0 = timer.ms();

    std::string prefix = path.pathname();
    enumerate(path, state, destination, prefix, 0);

    u64 time1 = timer.ms();

    MANGO_UNREFERENCED(time0);
    MANGO_UNREFERENCED(time1);

    printf("\n");

    constexpr u64 KB = 1 << 10;

    u64 total_time = time1 - time0;
    u64 rate = total_time ? state.total_bytes_out / (total_time * KB) : 0;

    printf("%12" PRIu64 " KB  %8" PRIu64 " ms     (%" PRIu64 " MB/s) \n",
        state.total_bytes_out / KB,
        total_time, rate);
}
