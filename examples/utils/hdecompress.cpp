/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
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

void enumerate(const Path& path, State& state, std::string destination, std::string prefix, int depth)
{
    std::string pathname = removePrefix(path.pathname(), prefix);
    std::string current = destination + pathname;

    if (state.decompress)
    {
        printLine("Folder: {}", current);

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
                    printLine(depth * 2, "+ {}", node.name);
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
                printLine("{:12}  0x{:08x}  {}", node.size, node.checksum, filename);
            }

            if (state.print_tree)
            {
                printLine(depth * 2, "- {}", node.name);
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
                        printLine(Print::Error, "ERROR: {} checksum: {:08x}, expected: {:08x}", filename, checksum, node.checksum);
                    }
                }
            }

            if (state.decompress)
            {
                printLine("Create: {}", filename);

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
        printLine("");
        printLine("HBS Decompression Tool version 0.2");
        printLine("Copyright (C) 2018 Fapware, inc. All rights reserved.");
        printLine("Usage: {} [archive] [destination]", program_name);
        printLine("       {} [archive] --list", program_name);
        printLine("       {} [archive] --tree", program_name);
        printLine("       {} [archive] --verify", program_name);
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
    Path path(file, ".hbs");

    Timer timer;
    u64 time0 = timer.ms();

    std::string prefix = path.pathname();
    enumerate(path, state, destination, prefix, 0);

    u64 time1 = timer.ms();

    MANGO_UNREFERENCED(time0);
    MANGO_UNREFERENCED(time1);

    printLine("");

    constexpr u64 KB = 1 << 10;

    u64 total_time = time1 - time0;
    u64 rate = total_time ? state.total_bytes_out / (total_time * KB) : 0;

    printLine("{:12} KB  {:8} ms     ({} MB/s)",
        state.total_bytes_out / KB,
        total_time, rate);
}
