/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
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
    static constexpr int status_line_width = 60;

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

template <typename... T>
static void replaceLine(fmt::format_string<T...> fmt, T&&... args)
{
    print("\r{:<{}}\n", fmt::format(fmt, std::forward<T>(args)...), status_line_width);
    std::fflush(stdout);
}

static
void progress(u64 i, u64 N, int width)
{
    // fraction done
    double fraction = double(i + 1) / N;
    int filled = int(fraction * width);

    // {:=>{}}  = '=' fill, right-aligned, dynamic width (filled)
    // {: <{}}  = space fill, left-aligned, dynamic width (remainder)
    print("\r[{:=>{}}{: <{}}] {:5.1f}%\r", "", filled, "", width - filled, fraction * 100.0);
    std::fflush(stdout);
}

// ------------------------------------------------------------------------------------------
// indexing
// ------------------------------------------------------------------------------------------

struct State
{
    bool verbose { false };
    size_t total_bytes = 0;
    std::vector<FileInfo> files;
    std::vector<FileInfo> folders;
};

bool isCompressible(const std::string& filename, u64 size)
{
    if (Mapper::isCustomMapper(filename) || size > 2 * GB)
    {
        return false;
    }

    std::string extension = getExtension(filename);
    if (extension == ".jpg" || extension == ".jpeg" || extension == ".png")
    {
        return false;
    }

    return true;
}

void enumerate(const Path& path, const std::string& prefix, State& state, int depth)
{
    for (auto& node : path)
    {
        std::string filename = removePrefix(path.pathname() + node.name, prefix);

        // NOTE: Custom mappers appear twice in the index (file + "name/" container mount).
        // Store the container file only; skip the synthetic directory entry entirely.
        if (node.isDirectory())
        {
            if (node.isContainer())
            {
                continue;
            }

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

            state.files.emplace_back(filename, node.size, 0);
            state.total_bytes += node.size;
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

    bool store { false }; // store raw data as is, no compression
    bool written { false };

    size_t file_index { ~size_t(0) };
    u32 part_index { 0 };
    u32 part_count { 1 };

    void append(const Segment& segment)
    {
        segments.push_back(segment);
        bytes += segment.size;
    }
};

struct SegmentedFile
{
    struct Segment
    {
        u32 block;
        u64 offset;
        u64 size;
    };

    std::string filename;
    u64 size;
    u32 checksum;
    std::vector<Segment> segments;
};

struct BlockManager
{
    std::vector<Block> blocks;
    std::vector<SegmentedFile> files;

    void flush(Block& block)
    {
        if (!block.segments.empty())
        {
            blocks.push_back(block);
            block = Block();
        }
    }

    void segment(size_t file_index, u64 offset, u64 size)
    {
        const u32 block_index = u32(blocks.size());
        files[file_index].segments.push_back({ block_index, offset, size });
    }

    void segment(u64 offset, u64 size)
    {
        segment(files.size() - 1, offset, size);
    }

    u64 getTotalBytes() const
    {
        u64 total = 0;
        for (auto node : files)
        {
            total += node.size;
        }
        return total;
    }

    void appendStoreNoCompression(Block& block, const FileInfo& node)
    {
        segment(0, node.size);
        block.append({ node.name, 0, node.size });

        block.store = true;
        block.uncompressed = block.bytes;
        block.compressed = block.bytes;
        block.method = Compressor::NONE;
        flush(block);
    }
};

struct FileGroup
{
    u32 part_count { 1 };
    u32 first_block_index { 0 };
    u32 completed { 0 };
    bool any_compressed { false };
    std::mutex mutex;
    std::vector<bool> pending_stored;
};

static
void readBlockSource(const Block& block, const Path& path, Buffer& source)
{
    u8* ptr = source.data();

    for (auto segment : block.segments)
    {
#ifdef DISABLE_MMAP
        std::string filename = path.pathname() + segment.filename;
        InputFileStream file(filename);
        file.seek(segment.offset, Stream::SeekMode::Begin);
        file.read(ptr, segment.size);
        ptr += segment.size;
#else
        File file(path, segment.filename);
        ConstMemory memory = ConstMemory(file).slice(segment.offset, segment.size);
        std::memcpy(ptr, memory.address, memory.size);
        ptr += memory.size;
#endif
    }
}

static
void compactBlocks(BlockManager& manager)
{
    std::vector<u32> remap(manager.blocks.size(), u32(-1));
    std::vector<Block> compact;

    for (size_t i = 0; i < manager.blocks.size(); ++i)
    {
        if (!manager.blocks[i].written)
        {
            continue;
        }

        remap[i] = u32(compact.size());
        compact.push_back(manager.blocks[i]);
    }

    for (auto& file : manager.files)
    {
        for (auto& segment : file.segments)
        {
            u32 block = segment.block;
            if (block < remap.size() && remap[block] != u32(-1))
            {
                segment.block = remap[block];
            }
        }
    }

    manager.blocks = std::move(compact);
}

#ifdef DISABLE_MMAP

static
void writeStoreNoCompression(const Block& block, const Path& path, Stream& output)
{
    for (auto segment : block.segments)
    {
        std::string filename = path.pathname() + segment.filename;
        InputFileStream file(filename);

        Buffer buffer(large_block_size);

        for (u64 offset = 0; offset < segment.size; offset += large_block_size)
        {
            u64 size = std::min(large_block_size, segment.size - offset);
            file.seek(segment.offset + offset, Stream::SeekMode::Begin);
            file.read(buffer.data(), size);
            output.write(buffer.data(), size);
        }
    }
}

#else

static
void writeStoreNoCompression(const Block& block, const Path& path, Stream& output)
{
    for (auto segment : block.segments)
    {
        File file(path, segment.filename);
        output.write(file.data() + segment.offset, segment.size);
    }
}

#endif

void compress(State& state, const std::string& folder, const std::string& archive, const std::string& compression, int level, size_t store_threshold)
{
    Compressor compressor = getCompressor(compression);

    print("Scanning files to compress...");
    std::fflush(stdout);

    if (state.verbose)
    {
        printLine("");
    }

    u64 scan_time0 = Time::ms();

    Path path(folder);
    enumerate(path, folder, state, 0);

    u64 scan_dt = Time::ms() - scan_time0;

    u64 total_files = state.files.size();
    if (!total_files)
    {
        printLine("[WARNING] Did not find anything to compress.");
        return;
    }

    replaceLine("Detected {} files ({:0.1f} MB) in {:0.2f} seconds",
        total_files,
        state.total_bytes / double(MB),
        scan_dt / 1000.0);

    // sort files by size
    std::sort(state.files.begin(), state.files.end(), [] (const FileInfo& a, const FileInfo& b)
    {
        return a.size > b.size;
    });

    BlockManager manager;
    Block block;
    std::vector<size_t> store_small_files;
    std::vector<std::unique_ptr<FileGroup>> file_groups;

    for (auto node : state.files)
    {
        manager.files.push_back({ node.name, node.size, 0 });
        file_groups.push_back(std::make_unique<FileGroup>());

        if (!isCompressible(node.name, node.size))
        {
            manager.flush(block);

            if (node.size > small_file_max_size)
            {
                // one archive block per large file (mmap-friendly)
                manager.appendStoreNoCompression(block, node);
            }
            else
            {
                // merge small store-raw files into one block after compression
                store_small_files.push_back(manager.files.size() - 1);
            }

            continue;
        }

        if (node.size > small_file_max_size)
        {
            if (node.size > large_block_size * 2)
            {
                size_t file_index = manager.files.size() - 1;
                u32 part_count = u32((node.size + large_block_size - 1) / large_block_size);

                FileGroup& group = *file_groups.back();
                group.part_count = part_count;
                group.first_block_index = u32(manager.blocks.size());
                group.pending_stored.resize(part_count, false);

                u32 part = 0;

                // split the file into multiple blocks
                for (u64 offset = 0; offset < node.size; offset += large_block_size)
                {
                    u64 size = std::min(large_block_size, node.size - offset);

                    block.file_index = file_index;
                    block.part_index = part++;
                    block.part_count = part_count;

                    manager.segment(0, size);
                    block.append({ node.name, offset, size });
                    manager.flush(block);
                }
            }
            else
            {
                // compress the file as one block
                manager.segment(0, node.size);
                block.append({ node.name, 0, node.size });
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

            manager.segment(block.bytes, node.size);
            block.append({ node.name, 0, node.size });
        }
    }

    manager.flush(block);

    // compute checksums

    print("Computing checksums...");
    std::fflush(stdout);

    u64 checksum_time0 = Time::ms();

    ConcurrentQueue q;

    const u64 totalBytes = manager.getTotalBytes();
    std::atomic<u64> counterBytes { 0 };

    for (size_t i = 0; i < manager.files.size(); ++i)
    {
        q.enqueue([&, i] ()
        {
            auto& node = manager.files[i];
            File file(path, node.filename);
            node.checksum = crc32c(0, file);
            counterBytes += node.size;
        });
    }

    while (counterBytes < totalBytes)
    {
        progress(counterBytes, totalBytes, 42);
        mango::Sleep::ms(120);
    }

    q.wait();

    progress(counterBytes, totalBytes, 42);

    u64 checksum_time1 = Time::ms();
    u64 checksum_dt = checksum_time1 - checksum_time0;

    replaceLine("Calculated checksum for {:0.1f} MB in {:0.2f} seconds ({} MB/s)",
        totalBytes / double(MB),
        checksum_dt / 1000.0,
        totalBytes / (std::max(u64(1), checksum_dt) * 1024));
    printLine("");

    // create output stream

    OutputFileStream output(archive);
    LittleEndianStream str = output;

    // write identifier

    str.write32(filesystem::HBS_MAGIC0);

    // compress

    std::mutex mutex; // write serialization mutex

    u64 compress_time0 = Time::ms();

    auto commit_block = [&](Block& block, const u8* data, u64 size, u32 method, char glyph)
    {
        std::unique_lock<std::mutex> write_lock(mutex);

        block.offset = output.offset();
        block.uncompressed = block.bytes;
        block.compressed = size;
        block.method = method;
        output.write(data, size);
        block.written = true;

        print("{}", glyph);
        std::fflush(stdout);
    };

    auto commit_stored_block = [&](Block& block, char glyph)
    {
        std::unique_lock<std::mutex> write_lock(mutex);

        block.offset = output.offset();
        block.uncompressed = block.bytes;
        block.compressed = block.bytes;
        block.method = Compressor::NONE;
        writeStoreNoCompression(block, path, output);
        block.written = true;

        print("{}", glyph);
        std::fflush(stdout);
    };

    auto flush_pending_stored = [&](FileGroup& group)
    {
        for (u32 i = 0; i < group.part_count; ++i)
        {
            if (!group.pending_stored[i])
            {
                continue;
            }

            Block& part = manager.blocks[group.first_block_index + i];
            commit_stored_block(part, 's');
            group.pending_stored[i] = false;
        }
    };

    auto fuse_stored_file = [&](FileGroup& group, size_t file_index)
    {
        Block& block = manager.blocks[group.first_block_index];
        const u64 file_size = manager.files[file_index].size;

        {
            std::unique_lock<std::mutex> write_lock(mutex);

            block.offset = output.offset();
            block.uncompressed = file_size;
            block.compressed = file_size;
            block.method = Compressor::NONE;

            for (u32 i = 0; i < group.part_count; ++i)
            {
                writeStoreNoCompression(manager.blocks[group.first_block_index + i], path, output);
                print("s");
                std::fflush(stdout);
            }

            block.written = true;
        }

        for (u32 i = 1; i < group.part_count; ++i)
        {
            manager.blocks[group.first_block_index + i].written = false;
            group.pending_stored[i] = false;
        }

        group.pending_stored[0] = false;

        manager.files[file_index].segments.clear();
        manager.files[file_index].segments.push_back({ group.first_block_index, 0, file_size });
    };

    for (size_t block_index = 0; block_index < manager.blocks.size(); ++block_index)
    {
        q.enqueue([&, block_index] ()
        {
            Block& block = manager.blocks[block_index];

            if (block.store)
            {
                std::unique_lock<std::mutex> write_lock(mutex);

                block.offset = output.offset();
                block.uncompressed = block.bytes;
                block.compressed = block.bytes;
                block.method = Compressor::NONE;

                writeStoreNoCompression(block, path, output);
                block.written = true;

                print("s");
                std::fflush(stdout);
            }
            else
            {
                auto data = std::make_unique<Buffer>(block.bytes);
                readBlockSource(block, path, *data);

                Memory uncompressed = *data;

                Memory compressed;
                Buffer dest;

                print(".");
                std::fflush(stdout);

                if (store_threshold > 0)
                {
                    size_t bound = compressor.bound(uncompressed.size);
                    dest.reset(bound);

                    compressed.size = compressor.compress(dest, uncompressed, level);
                    compressed.address = dest.data();
                }
                else
                {
                    compressed.address = data->data();
                    compressed.size = data->size();
                }

                const bool store = compressed.size > uncompressed.size * store_threshold / 100;
                const bool split_file = block.part_count > 1 && block.file_index != ~size_t(0);

                if (!split_file)
                {
                    if (store)
                    {
                        commit_stored_block(block, '+');
                    }
                    else
                    {
                        commit_block(block, compressed.address, compressed.size, compressor.method, '+');
                    }
                }
                else
                {
                    FileGroup& group = *file_groups[block.file_index];

                    if (!store)
                    {
                        {
                            std::lock_guard<std::mutex> lock(group.mutex);

                            if (!group.any_compressed)
                            {
                                group.any_compressed = true;
                                flush_pending_stored(group);
                            }
                        }

                        commit_block(block, compressed.address, compressed.size, compressor.method, '+');
                    }
                    else
                    {
                        std::lock_guard<std::mutex> lock(group.mutex);

                        if (group.any_compressed)
                        {
                            commit_stored_block(block, '+');
                        }
                        else
                        {
                            group.pending_stored[block.part_index] = true;

                            if (++group.completed == group.part_count)
                            {
                                fuse_stored_file(group, block.file_index);
                            }
                        }
                    }
                }
            }
        });
    }

    // synchronize
    q.wait();

    if (!store_small_files.empty())
    {
        Block raw;
        raw.store = true;

        for (size_t file_index : store_small_files)
        {
            const auto& file = manager.files[file_index];
            manager.segment(file_index, raw.bytes, file.size);
            raw.append({ file.filename, 0, file.size });
        }

        raw.offset = output.offset();
        raw.uncompressed = raw.bytes;
        raw.compressed = raw.bytes;
        raw.method = Compressor::NONE;

        writeStoreNoCompression(raw, path, output);
        raw.written = true;
        manager.blocks.push_back(raw);

        print("s");
        std::fflush(stdout);
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

    block.written = true;
    manager.flush(block);

    compactBlocks(manager);

    u64 compress_time1 = Time::ms();
    u64 compress_dt = compress_time1 - compress_time0;

    u64 total_compressed_bytes = output.offset();

    printLine("");
    printLine("");
    printLine("Compressed: {:0.1f} MB --> {:0.1f} MB ({:0.1f}%) in {:0.2f} seconds ({}-{}, {} MB/s)",
        state.total_bytes / double(MB),
        total_compressed_bytes / double(MB),
        total_compressed_bytes * 100.0 / state.total_bytes,
        double(compress_dt / 1000.0),
        compressor.name,
        level,
        state.total_bytes / (std::max(u64(1), compress_dt) * 1024));

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
        u64         offset
        u64         size

    File:
        char[]      filename
        u64         size
        u32         checksum
        Segment[]   segments

    --------------------------------------------------------------------------
    File Format Structure:
    --------------------------------------------------------------------------

    Compressed block data:
        u32         magic: hbs0
        u8[]        data     <-- written by the compressor, a raw binary blob w/o specific size or structure

    Block Info Array:
        u32         magic: hbs1
        u32         version
        block[]     blocks

    File Info Array:
        u32         magic: hbs2
        u32         version
        u64         compressed size (file array)
        u64         uncmpressed size (file array)
        File[]      files (compressed with zstd)

    Header:
        u32         magic: hbs3
        u32         version
        u64         offset to block info array
        u64         offset to file info array
    */

    // write block data

    u64 block_data_offset = output.offset();
    u32 num_blocks = u32(manager.blocks.size());

    str.write32(filesystem::HBS_MAGIC1);
    str.write32(filesystem::HBS_VERSION);
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

    str.write32(filesystem::HBS_MAGIC2);
    str.write32(filesystem::HBS_VERSION);

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
            le.write64(segment.offset);
            le.write64(segment.size);
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

    str.write32(filesystem::HBS_MAGIC3);
    str.write32(filesystem::HBS_VERSION);
    str.write64(block_data_offset);
    str.write64(file_data_offset);
}

void printHelp(const CommandLine& commands)
{
    std::string program = removePath(std::string(commands[0]));

    printLine("");
    printLine("HBS Compression Tool version 0.5.5");
    printLine("Copyright (C) 2018-2026 Fapware, inc. All rights reserved.");
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
    printLine("  --output <filename>");
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
    std::string output = "result.hbs";
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
        else if (commands[i] == "--output")
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
