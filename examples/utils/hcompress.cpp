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

struct BlockMeta
{
    struct Source
    {
        std::string filename;
        u64 offset;
        u64 size;
    };

    u64 bytes { 0 };
    std::vector<Source> sources;

    bool store { false }; // store raw data as is, no compression
    bool written { false };

    size_t file_index { ~size_t(0) };
    u32 part_index { 0 };
    u32 part_count { 1 };

    void append(const Source& source)
    {
        sources.push_back(source);
        bytes += source.size;
    }
};

struct BlockManager
{
    std::vector<hbs::Block> blocks;
    std::vector<BlockMeta> meta;
    std::vector<hbs::File> files;

    void flush(BlockMeta& work)
    {
        if (!work.sources.empty())
        {
            blocks.push_back({});
            meta.push_back(work);
            work = BlockMeta();
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

    void appendStoreNoCompression(BlockMeta& work, const FileInfo& node)
    {
        segment(0, node.size);
        work.append({ node.name, 0, node.size });

        work.store = true;
        flush(work);

        hbs::Block& desc = blocks.back();
        const BlockMeta& stored = meta.back();
        desc.uncompressed = stored.bytes;
        desc.compressed = stored.bytes;
        desc.method = Compressor::NONE;
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
void readBlockSource(const BlockMeta& work, const Path& path, Buffer& source)
{
    u8* ptr = source.data();

    for (auto segment : work.sources)
    {
        File file(path, segment.filename);
        ConstMemory memory = ConstMemory(file).slice(segment.offset, segment.size);
        std::memcpy(ptr, memory.address, memory.size);
        ptr += memory.size;
    }
}

static
void compactBlocks(BlockManager& manager)
{
    std::vector<u32> remap(manager.blocks.size(), u32(-1));
    std::vector<hbs::Block> compact_blocks;
    std::vector<BlockMeta> compact_meta;

    for (size_t i = 0; i < manager.blocks.size(); ++i)
    {
        if (!manager.meta[i].written)
        {
            continue;
        }

        remap[i] = u32(compact_blocks.size());
        compact_blocks.push_back(manager.blocks[i]);
        compact_meta.push_back(manager.meta[i]);
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

    manager.blocks = std::move(compact_blocks);
    manager.meta = std::move(compact_meta);
}

static
void writeStoreNoCompression(const BlockMeta& work, const Path& path, Stream& output)
{
    for (const auto& source : work.sources)
    {
        File file(path, source.filename);
        output.write(file.data() + source.offset, source.size);
    }
}

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
    BlockMeta work;
    std::vector<size_t> store_small_files;
    std::vector<std::unique_ptr<FileGroup>> file_groups;

    for (auto node : state.files)
    {
        manager.files.push_back({ node.name, node.size, 0 });
        file_groups.push_back(std::make_unique<FileGroup>());

        if (!isCompressible(node.name, node.size))
        {
            manager.flush(work);

            if (node.size > small_file_max_size)
            {
                // one archive block per large file (mmap-friendly)
                manager.appendStoreNoCompression(work, node);
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

                    work.file_index = file_index;
                    work.part_index = part++;
                    work.part_count = part_count;

                    manager.segment(offset, size);
                    work.append({ node.name, offset, size });
                    manager.flush(work);
                }
            }
            else
            {
                // compress the file as one block
                manager.segment(0, node.size);
                work.append({ node.name, 0, node.size });
                manager.flush(work);
            }
        }
        else
        {
            // merge small files into one block
            if (work.bytes >= small_block_size)
            {
                manager.flush(work);
            }

            manager.segment(work.bytes, node.size);
            work.append({ node.name, 0, node.size });
        }
    }

    manager.flush(work);

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

    auto commit_block = [&](size_t block_index, const u8* data, u64 size, u32 method, char glyph)
    {
        std::unique_lock<std::mutex> write_lock(mutex);

        hbs::Block& desc = manager.blocks[block_index];
        BlockMeta& block = manager.meta[block_index];

        desc.offset = output.offset();
        desc.uncompressed = block.bytes;
        desc.compressed = size;
        desc.method = method;
        output.write(data, size);
        block.written = true;

        print("{}", glyph);
        std::fflush(stdout);
    };

    auto commit_stored_block = [&](size_t block_index, char glyph)
    {
        std::unique_lock<std::mutex> write_lock(mutex);

        hbs::Block& desc = manager.blocks[block_index];
        BlockMeta& block = manager.meta[block_index];

        desc.offset = output.offset();
        desc.uncompressed = block.bytes;
        desc.compressed = block.bytes;
        desc.method = Compressor::NONE;
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

            commit_stored_block(group.first_block_index + i, 's');
            group.pending_stored[i] = false;
        }
    };

    auto fuse_stored_file = [&](FileGroup& group, size_t file_index)
    {
        hbs::Block& desc = manager.blocks[group.first_block_index];
        const u64 file_size = manager.files[file_index].size;

        {
            std::unique_lock<std::mutex> write_lock(mutex);

            desc.offset = output.offset();
            desc.uncompressed = file_size;
            desc.compressed = file_size;
            desc.method = Compressor::NONE;

            for (u32 i = 0; i < group.part_count; ++i)
            {
                writeStoreNoCompression(manager.meta[group.first_block_index + i], path, output);
                print("s");
                std::fflush(stdout);
            }

            manager.meta[group.first_block_index].written = true;
        }

        for (u32 i = 1; i < group.part_count; ++i)
        {
            manager.meta[group.first_block_index + i].written = false;
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
            BlockMeta& block = manager.meta[block_index];

            if (block.store)
            {
                std::unique_lock<std::mutex> write_lock(mutex);

                hbs::Block& desc = manager.blocks[block_index];

                desc.offset = output.offset();
                desc.uncompressed = block.bytes;
                desc.compressed = block.bytes;
                desc.method = Compressor::NONE;

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
                        commit_stored_block(block_index, '+');
                    }
                    else
                    {
                        commit_block(block_index, compressed.address, compressed.size, compressor.method, '+');
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

                        commit_block(block_index, compressed.address, compressed.size, compressor.method, '+');
                    }
                    else
                    {
                        std::lock_guard<std::mutex> lock(group.mutex);

                        if (group.any_compressed)
                        {
                            commit_stored_block(block_index, '+');
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
        BlockMeta raw;
        raw.store = true;

        for (size_t file_index : store_small_files)
        {
            const auto& file = manager.files[file_index];
            manager.segment(file_index, raw.bytes, file.size);
            raw.append({ file.filename, 0, file.size });
        }

        hbs::Block desc;
        desc.offset = output.offset();
        desc.uncompressed = raw.bytes;
        desc.compressed = raw.bytes;
        desc.method = Compressor::NONE;

        writeStoreNoCompression(raw, path, output);
        raw.written = true;
        manager.blocks.push_back(desc);
        manager.meta.push_back(raw);

        print("s");
        std::fflush(stdout);
    }

    for (auto node : state.folders)
    {
        manager.files.push_back({ node.name, 0 });
        work.append({ node.name, 0, 0 });
    }

    work.written = true;
    manager.flush(work);

    hbs::Block& folder_desc = manager.blocks.back();
    folder_desc.method = Compressor::NONE;
    folder_desc.offset = output.offset();
    folder_desc.compressed = 0;
    folder_desc.uncompressed = 0;

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

    // write block data

    u64 block_data_offset = output.offset();
    hbs::writeBlockArray(str, manager.blocks);

    // write file data

    u64 file_data_offset = output.offset();
    hbs::writeFileArray(str, manager.files);

    // write index

    hbs::writeIndex(str, block_data_offset, file_data_offset);
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
