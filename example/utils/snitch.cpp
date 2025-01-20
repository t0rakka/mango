/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cinttypes>
#include <algorithm>
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;

#define DISABLE_MMAP

namespace
{

    // unit helpers
    constexpr u64 KB = 1 << 10;
    constexpr u64 MB = 1 << 20;
    constexpr u64 GB = 1 << 30;

    // configuration
    constexpr u64 large_block_size = 4 * MB;
    constexpr u64 small_file_max_size = 512 * KB;
    constexpr u64 small_block_size = 2 * MB;

    constexpr size_t store_threshold_default = 95; // percent

} // namespace

/*

~/data/code/ 9981 files (4927.9 MB)

------------------------------------------------------------------------------------------

zip:    deflate-9
        942 MB  

real    7m52.331s
user    7m50.543s
sys     0m1.208s

------------------------------------------------------------------------------------------

7z:     -m0=lzma2 -mx9 -ms=on
        568 MB

real    3m6.777s
user    47m19.115s
sys     0m12.374s

------------------------------------------------------------------------
method:  lzma2-10   bzip2-10     zstd-5    miniz-6    lzfse-5     zstd-1
------------------------------------------------------------------------
size:      667 MB     830 MB     827 MB     944 MB     943 MB     918 MB
speed:    56 MB/s   260 MB/s   562 MB/s   624 MB/s   979 MB/s  4038 MB/s

real:      87.8 s     18.9 s      8.8 s      7.9 s     5.0 s       1.2 s
user:    1682.3 s    367.2 s    169.8 s    154.7 s     90.1 s     18.5 s
sys:       18.7 s      4.9 s      2.3 s      1.7 s      3.6 s      3.7 s
-------------------------------------------------------------------------

*/

// ------------------------------------------------------------------------------------------
// indexing
// ------------------------------------------------------------------------------------------

struct State
{
    bool verbose { false };
    size_t total_bytes = 0;
    std::vector<FileInfo> files;
    std::vector<FileInfo> containers;
    std::vector<FileInfo> folders;
};

bool isContainer(const std::string& filename, u64 size)
{
    // NOTE: This just means we STORE these files without compression
    bool is = Mapper::isCustomMapper(filename) || (size > 2 * GB);
    if (!is)
    {
        std::string extension = getExtension(filename);
        if (extension == ".jpg" || extension == ".jpeg" || extension == ".png")
        {
            // JPEG and PNG are never compressed
            is = true;
        }
    }
    return is;
}

void enumerate(const Path& path, const std::string& prefix, State& state, int depth)
{
    for (auto& node : path)
    {
        std::string filename = removePrefix(path.pathname() + node.name, prefix);
        if (!node.isContainer())
        {
            if (node.isDirectory())
            {
                if (state.verbose)
                {
                    printLine(depth * 2, "+ {}", node.name);
                }

                enumerate(Path(path, node.name), prefix, state, depth + 1);
                state.folders.emplace_back(filename, 0, 0);
            }
            else
            {
                if (state.verbose)
                {
                    printLine(depth * 2, "- {}", node.name);
                }

                bool is_container = isContainer(node.name, node.size);
                if (is_container)
                {
                    state.containers.emplace_back(filename, node.size, 0);
                }
                else
                {
                    state.files.emplace_back(filename, node.size, 0);
                }

                state.total_bytes += node.size;
            }
        }
    }
}

// ------------------------------------------------------------------------------------------
// compression
// ------------------------------------------------------------------------------------------

struct Block
{
    struct Segment
    {
        std::string filename;
        u64 offset;
        u64 size;
    };

    u64 bytes { 0 };
    std::vector<Segment> segments;

    u64 offset;
    u64 compressed;
    u64 uncompressed;
    u32 method;

    void append(const Segment& segment)
    {
        segments.push_back(segment);
        bytes += segment.size;
    }
};

struct SegFile
{
    struct Segment
    {
        u32 block;
        u32 offset;
        u32 size;
    };

    std::string filename;
    u64 size;
    u32 checksum;
    std::vector<Segment> segments;
};

struct BlockManager
{
    std::vector<Block> blocks;
    std::vector<SegFile> files;

    void flush(Block& block)
    {
        if (!block.segments.empty())
        {
            blocks.push_back(block);
            block = Block();
        }
    }

    void segment(u32 offset, u32 size)
    {
        const u32 block_index = u32(blocks.size());
        files.back().segments.push_back({block_index, offset, size});
    }
};

void compress(State& state, const std::string& folder, const std::string& archive, const std::string& compression, int level, size_t store_threshold)
{
    Compressor compressor = getCompressor(compression);

    printLine("Scanning files to compress...");

    Path path(folder);
    enumerate(path, folder, state, 0);

    u64 total_files = state.files.size() + state.containers.size();
    if (!total_files)
    {
        printLine("[WARNING] Did not find anything to compress.");
        return;
    }

    printLine("");
    printLine("Compressing {} files ({}.{} MB)", 
        total_files, 
        state.total_bytes / MB, (state.total_bytes * 10 / MB) % 10);
    printLine("");

    // sort files by size
    std::sort(state.files.begin(), state.files.end(),
        [] (const FileInfo& a, const FileInfo& b)
        {
            return a.size > b.size;
        });

    BlockManager manager;
    Block block;

    for (auto node : state.files)
    {

        // MANGO TODO: compute checksum
#if 0
        File file(path, node.name);
        u32 checksum = crc32c(0, file);
#else
        u32 checksum = 0;
#endif

        manager.files.push_back({node.name, node.size, checksum});

        if (node.size > small_file_max_size)
        {
            if (node.size > large_block_size * 2)
            {
                // split the file into multiple blocks
                for (u64 offset = 0; offset < node.size; offset += large_block_size)
                {
                    u64 size = std::min(large_block_size, node.size - offset);
                    manager.segment(0, size);
                    block.append({node.name, offset, size});
                    manager.flush(block);
                }
            }
            else
            {
                // compress the file as one block
                manager.segment(0, node.size);
                block.append({node.name, 0, node.size});
                manager.flush(block);
            }
        }
        else
        {
            // merge small files into one block
            if (block.bytes >= small_block_size)
            {
                manager.flush(block);
            }

            manager.segment(u32(block.bytes), node.size);
            block.append({node.name, 0, node.size});
        }
    }

    manager.flush(block);

    // create output stream

    OutputFileStream output(archive);
    LittleEndianStream str = output;

    // write identifier

    str.write32(u32_mask('m', 'g', 'x', '0'));

    // compress

    ConcurrentQueue q; // compression queue
    std::mutex mutex; // write serialization mutex

    u64 time0 = Time::ms();

    for (auto &block : manager.blocks)
    {
        q.enqueue([&] ()
        {
            Buffer source(block.bytes);
            u8* ptr = source.data();

            for (auto segment : block.segments)
            {
#ifdef DISABLE_MMAP
                std::string filename = path.pathname() + segment.filename;
                InputFileStream file(filename);
                file.seek(segment.offset, Stream::BEGIN);
                file.read(ptr, segment.size);
                ptr += segment.size;
#else
                File file(path, segment.filename);
                ConstMemory memory = ConstMemory(file).slice(segment.offset, segment.size);
                std::memcpy(ptr, memory.address, memory.size);
                ptr += memory.size;
#endif
            }

            Memory uncompressed = source;

            Memory compressed;
            Buffer dest;

            if (store_threshold > 0)
            {
                size_t bound = compressor.bound(uncompressed.size);
                dest.reset(bound);

                compressed.size = compressor.compress(dest, uncompressed, level);
                compressed.address = dest.data();
            }
            else
            {
                compressed = source;
            }

            if (compressed.size > uncompressed.size * store_threshold / 100)
            {
                // doesn't compress -> store
                compressed = uncompressed;

                block.uncompressed = uncompressed.size;
                block.compressed = compressed.size;
                block.method = Compressor::NONE;

                print("s");
            }
            else
            {
                block.uncompressed = uncompressed.size;
                block.compressed = compressed.size;
                block.method = compressor.method;

                print(".");
            }

            fflush(stdout);

            std::unique_lock<std::mutex> write_lock(mutex);

            block.offset = output.offset();
            output.write(compressed.address, compressed.size);

            print("+");
            fflush(stdout);
        });
    }

    // synchronize
    q.wait();

    for (auto node : state.containers)
    {
        // container: store w/o compressing
#ifdef DISABLE_MMAP
        Buffer buffer(large_block_size);
        std::string filename = path.pathname() + node.name;
        InputFileStream file(filename);
#else
        File file(path, node.name);
#endif

        block.method = Compressor::NONE;
        block.offset = output.offset();
        block.compressed = file.size();
        block.uncompressed = file.size();

        // write the file in multiple parts
        for (u64 offset = 0; offset < file.size(); offset += large_block_size)
        {
            u64 size = std::min(large_block_size, file.size() - offset);
#ifdef DISABLE_MMAP
            file.read(buffer.data(), size);
            output.write(buffer.data(), size);
#else
            output.write(file.data() + offset, size);
#endif
            print("s");
            fflush(stdout);
        }

        manager.files.push_back({node.name, file.size()});
        manager.segment(0, file.size());
        block.append({node.name, 0, file.size()});

        manager.flush(block);
    }

    block.method = Compressor::NONE;
    block.offset = output.offset();
    block.compressed = 0;
    block.uncompressed = 0;

    for (auto node : state.folders)
    {
        manager.files.push_back({node.name, 0});
        block.append({node.name, 0, 0});
    }

    manager.flush(block);

    u64 time1 = Time::ms();
    u64 dt = time1 - time0;

    u64 total_compressed_bytes = output.offset();

    printLine("");
    printLine("");
    printLine("Compressed: {:0.1f} MB --> {:0.1f} MB ({:0.1f}%) in {:0.2f} seconds ({}-{}, {} MB/s)",
        state.total_bytes / double(MB),
        total_compressed_bytes / double(MB),
        total_compressed_bytes * 100.0 / state.total_bytes,
        double(dt / 1000.0),
        compressor.name,
        level,
        state.total_bytes / (std::max(u64(1), dt) * 1024));

    /*

    --------------------------------------------------------------------------
    File Format Types:
    --------------------------------------------------------------------------

    Type[]:
        u32         count
        Type        data[count]

    Block:
        u64         offset
        u64         compressed
        u64         uncompressed
        u32         compression method

    Segment:
        u32         block_index
        u32         offset
        u32         size

    File:
        char[]      filename
        u64         size
        u32         checksum
        Segment[]   segments

    --------------------------------------------------------------------------
    File Format Structure:
    --------------------------------------------------------------------------

    Compressed block data:
        u32         magic: mgx0
        u8[]        data     <-- written by the compressor, a raw binary blob w/o specific size or structure

    Block Info Array:
        u32         magic: mgx1
        block[]     blocks

    File Info Array:
        u32         magic: mgx2
        u64         compressed size (file array)
        u64         uncmpressed size (file array)
        File[]      files (compressed with zstd)

    Header:
        u32         magic: mgx3
        u32         version
        u64         offset to block info array
        u64         offset to file info array
    */

    // write block data

    u64 block_data_offset = output.offset();
    u32 num_blocks = u32(manager.blocks.size());

    str.write32(u32_mask('m', 'g', 'x', '1'));
    str.write32(num_blocks);

    for (auto &block : manager.blocks)
    {
        str.write64(block.offset);
        str.write64(block.compressed);
        str.write64(block.uncompressed);
        str.write32(block.method);
    }

    // write file data

    u64 file_data_offset = output.offset();

    str.write32(u32_mask('m', 'g', 'x', '2'));

    // write file data into temporary buffer

    MemoryStream temp;
    LittleEndianStream le = temp; // LE adaptor for writing into the stream

    le.write32(u32(manager.files.size()));

    for (auto &file : manager.files)
    {
        std::string filename = removePrefix(file.filename, folder);
        u32 length = u32(filename.length());

        le.write32(length);
        le.write(filename.c_str(), length);

        le.write64(file.size);
        le.write32(file.checksum);
        le.write32(u32(file.segments.size()));

        for (auto &segment : file.segments)
        {
            le.write32(segment.block);
            le.write32(segment.offset);
            le.write32(segment.size);
        }
    }

    // compress the temporary buffer

    size_t bound = compressor.bound(temp.size());
    Buffer compressed(bound);

    CompressionStatus status = zstd::compress(compressed, temp, 10);

    // write compressed buffer

    str.write64(u64(status.size)); // compressed size
    str.write64(u64(temp.size())); // uncompressed size
    str.write(compressed, status.size);

    // write header

    str.write32(u32_mask('m', 'g', 'x', '3'));
    str.write32(1);
    str.write64(block_data_offset);
    str.write64(file_data_offset);
}

using CommandLine = std::vector<std::string_view>;

void printHelp(const CommandLine& commands)
{
    std::string program = removePath(std::string(commands[0]));

    printLine("");
    printLine("MGX/SNITCH Compression Tool version 0.5.3");
    printLine("Copyright (C) 2018-2024 Fapware, inc. All rights reserved.");
    printLine("");
    printLine("Usage: {} [input folder] [compression] [level:0..10] (options)", program);
    printLine("");

    printLine("Compression methods: ");
    print("  ");
    const char* separator = "";
    auto compressors = getCompressors();
    for (auto compressor : compressors)
    {
        print("{}{}", separator, compressor.name);
        separator = " ";
    }
    printLine("");

    printLine("");
    printLine("Options:");
    printLine("  -output <filename>");
    printLine("  --store");
    printLine("  --verbose");
    printLine("");
}

// ------------------------------------------------------------------------------------------
// main
// ------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    CommandLine commands(argv + 0, argv + argc);

    if (commands.size() < 4)
    {
        printHelp(commands);
        return 0;
    }

    State state;

    std::string folder = std::string(commands[1]);
    std::string output = "result.snitch";
    std::string compression = std::string(commands[2]);

    int level = std::stoi(commands[3].data());
    size_t store_threshold = store_threshold_default;

    for (size_t i = 4; i < commands.size(); ++i)
    {
        if (commands[i] == "--store")
        {
            store_threshold = 0;
        }
        else if (commands[i] == "--verbose")
        {
            state.verbose = true;
        }
        else if (commands[i] == "-output")
        {
            if (++i < commands.size())
            {
                output = std::string(commands[i]);
            }
            else
            {
                printLine("Output filename missing.");
                return 0;
            }
        }
    }

    try
    {
        compress(state, folder, output, compression, level, store_threshold);
    }
    catch (Exception& e)
    {
        printLine("{}", e.what());
    }
}
