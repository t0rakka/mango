/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <atomic>
#include <filesystem>
#include <mutex>
#include <vector>
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;

namespace
{
    constexpr u64 KB = 1 << 10;
    constexpr u64 MB = 1 << 20;
    static constexpr int status_line_width = 70;

    template <typename... T>
    void replaceLine(fmt::format_string<T...> fmt, T&&... args)
    {
        print("\r{:<{}}\n", fmt::format(fmt, std::forward<T>(args)...), status_line_width);
        std::fflush(stdout);
    }

    void progress(u64 i, u64 N, int width)
    {
        double fraction = double(i + 1) / N;
        int filled = int(fraction * width);

        print("\rVerifying: [{:=>{}}{: <{}}] {:5.1f}%\r", "", filled, "", width - filled, fraction * 100.0);
        std::fflush(stdout);
    }

} // namespace

struct State
{
    bool print_list { false };
    bool print_tree { false };
    bool verify { false };
    bool decompress { false };
    u64 total_bytes = 0;
    u64 file_count = 0;
    u32 verify_errors { 0 };
};

struct VerifyEntry
{
    std::string name;
    u64 size;
    u32 checksum;
};

void collect(const Path& path, const std::string& prefix, std::vector<VerifyEntry>& entries)
{
    for (auto& node : path)
    {
        if (node.isDirectory())
        {
            if (!node.isContainer())
            {
                collect(Path(path, node.name), prefix, entries);
            }
        }
        else
        {
            std::string name = removePrefix(path.pathname() + node.name, prefix);
            entries.push_back({ name, node.size, node.checksum });
        }
    }
}

void verify(Path& path, std::vector<VerifyEntry>& entries, State& state)
{
    u64 checksum_time0 = Time::ms();

    const u64 totalBytes = [&]
    {
        u64 total = 0;
        for (const auto& entry : entries)
        {
            total += entry.size;
        }
        return total;
    } ();

    std::atomic<u64> counterBytes { 0 };
    std::atomic<u32> verify_errors { 0 };

    ConcurrentQueue q;

    for (const auto& entry : entries)
    {
        q.enqueue([&, entry] ()
        {
            File file(path, entry.name);
            u32 checksum = crc32c(0, file);

            if (entry.checksum && checksum != entry.checksum)
            {
                ++verify_errors;
            }

            counterBytes += entry.size;
        });
    }

    while (counterBytes < totalBytes)
    {
        progress(counterBytes, totalBytes, 42);
        mango::Sleep::ms(120);
    }

    q.wait();

    u64 checksum_time1 = Time::ms();
    u64 checksum_dt = checksum_time1 - checksum_time0;

    state.file_count = entries.size();
    state.total_bytes = totalBytes;
    state.verify_errors = verify_errors;

    replaceLine("Verified {} files ({:0.1f} MB) in {:0.2f} seconds ({} MB/s)",
        state.file_count,
        totalBytes / double(MB),
        checksum_dt / 1000.0,
        totalBytes / (std::max(u64(1), checksum_dt) * 1024));
}

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

            if (state.print_tree)
            {
                printLine(depth * 2, "- {}", node.name);
            }

            if (state.decompress)
            {
                printLine("Create: {}", filename);

                File file(path, node.name);
                ConstMemory memory = file;

                if (node.checksum)
                {
                    u32 checksum = crc32c(0, memory);

                    if (checksum != node.checksum)
                    {
                        ++state.verify_errors;
                        printLine(Print::Error, "ERROR: {} checksum: {:08x}, expected: {:08x}",
                            filename, checksum, node.checksum);
                    }
                }

                OutputFileStream outfile(filename);
                outfile.write(memory.address, memory.size);
            }

            state.total_bytes += node.size;
            ++state.file_count;
        }
    }
}

void printList(const std::vector<VerifyEntry>& entries)
{
    for (const auto& entry : entries)
    {
        printLine("{:12}  0x{:08x}  {}", entry.size, entry.checksum, entry.name);
    }
}

int main(int argc, char* argv[])
{
    std::string program_name = removePath(argv[0]);

    if (argc < 2)
    {
        printLine("");
        printLine("HBS Decompression Tool version 0.3");
        printLine("Copyright (C) 2026 Twilight 3D Finland, Oy. All rights reserved.");
        printLine("Usage: {} [archive] [destination]", program_name);
        printLine("       {} [archive] --list", program_name);
        printLine("       {} [archive] --tree", program_name);
        printLine("       {} [archive] --verify", program_name);
        return 0;
    }

    std::string filename = argv[1];
    std::string destination;

    State state;

    for (int i = 2; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--verify")
        {
            state.verify = true;
        }
        else if (arg == "--list")
        {
            state.print_list = true;
        }
        else if (arg == "--tree")
        {
            state.print_tree = true;
        }
        else if (!arg.empty() && arg[0] == '-')
        {
            printLine(Print::Error, "Unknown option: {}", arg);
            return 1;
        }
        else if (!arg.empty())
        {
            if (!destination.empty())
            {
                printLine(Print::Error, "Multiple destinations specified.");
                return 1;
            }

            destination = arg;
        }
    }

    if (state.print_list && state.print_tree)
    {
        printLine(Print::Error, "Use --list or --tree, not both.");
        return 1;
    }

    if (!destination.empty() && (state.print_list || state.print_tree))
    {
        printLine(Print::Error, "Cannot combine extract with --list or --tree.");
        return 1;
    }

    if (destination.empty() && !state.verify && !state.print_list && !state.print_tree)
    {
        printLine(Print::Error, "No mode selected. Use --list, --tree, --verify, or a destination folder.");
        return 1;
    }

    if (!destination.empty())
    {
        state.decompress = true;
        destination += "/";
    }

    // Create a memory view of the container
    File file(filename);
    Path path(file, ".hbs");

    u64 time0 = Time::ms();

    try
    {
        std::string prefix = path.pathname();

        if (state.decompress)
        {
            enumerate(path, state, destination, prefix, 0);
        }
        else
        {
            std::vector<VerifyEntry> entries;

            if (state.print_list)
            {
                u64 time0 = Time::ms();

                collect(path, prefix, entries);
                printList(entries);

                state.file_count = entries.size();
                for (const auto& entry : entries)
                {
                    state.total_bytes += entry.size;
                }

                u64 time1 = Time::ms();
                u64 total_time = time1 - time0;

                printLine("");
                printLine("List: {} files ({:0.1f} MB) in {:0.2f} seconds",
                    state.file_count,
                    state.total_bytes / double(MB),
                    total_time / 1000.0);
                printLine("");
            }
            else if (state.print_tree)
            {
                u64 time0 = Time::ms();

                enumerate(path, state, destination, prefix, 0);

                u64 time1 = Time::ms();
                u64 total_time = time1 - time0;

                printLine("");
                printLine("Tree: {} files ({:0.1f} MB) in {:0.2f} seconds",
                    state.file_count,
                    state.total_bytes / double(MB),
                    total_time / 1000.0);
                printLine("");
            }

            if (state.verify)
            {
                if (entries.empty())
                {
                    collect(path, prefix, entries);
                }

                verify(path, entries, state);
            }
        }
    }
    catch (Exception& e)
    {
        printLine(Print::Error, "{}", e.what());
        return 1;
    }

    u64 time1 = Time::ms();
    u64 total_time = time1 - time0;

    if (state.decompress)
    {
        u64 rate = total_time ? state.total_bytes / (total_time * KB) : 0;

        printLine("");
        printLine("Extracted {} files ({:0.1f} MB) in {:0.2f} seconds ({} MB/s)",
            state.file_count,
            state.total_bytes / double(MB),
            total_time / 1000.0,
            rate);
    }
    else if (state.verify)
    {
        if (state.verify_errors > 0)
        {
            printLine("Status: FAILED (count: {})", state.verify_errors);
        }
        else
        {
            printLine("Status: PASSED");
        }
    }
}
