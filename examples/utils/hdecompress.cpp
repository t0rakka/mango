/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <atomic>
#include <filesystem>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <mango/mango.hpp>
#include <mango/filesystem/hbs.hpp>

using namespace mango;
using namespace mango::filesystem;

namespace
{
    constexpr u64 KB = 1 << 10;
    constexpr u64 MB = 1 << 20;
    static constexpr u64 hbs_index_size = 24;
    static constexpr int status_line_width = 70;

    template <typename... T>
    void replaceLine(fmt::format_string<T...> fmt, T&&... args)
    {
        print("\r{:<{}}\n", fmt::format(fmt, std::forward<T>(args)...), status_line_width);
        std::fflush(stdout);
    }

    void progress(u64 i, u64 N, int width, const char* label = "Processing")
    {
        double fraction = N ? double(i) / N : 1.0;
        int filled = int(fraction * width);

        print("\r{}: [{:=>{}}{: <{}}] {:5.1f}%\r", label, "", filled, "", width - filled, fraction * 100.0);
        std::fflush(stdout);
    }

    struct BlockUse
    {
        size_t file_index;
        u32 segment_index;
        u64 file_offset;
        u64 block_offset;
        u64 size;
    };

    struct SegmentCrc
    {
        u32 crc { 0 };
        u64 size { 0 };
    };

    static
    void readArchive(ConstMemory archive, std::vector<hbs::Block>& blocks, std::vector<hbs::File>& files)
    {
        if (archive.size < hbs_index_size)
        {
            MANGO_EXCEPTION("[hdecompress] Archive is too small.");
        }

        LittleEndianConstPointer p = archive.address;
        u32 magic0 = p.read32();
        if (magic0 != HBS_MAGIC0)
        {
            MANGO_EXCEPTION("[hdecompress] Incorrect archive identifier ({:#x}).", magic0);
        }

        u64 index_offset = archive.size - hbs_index_size;
        p = archive.address + index_offset;

        u32 magic3 = p.read32();
        if (magic3 != HBS_MAGIC3)
        {
            MANGO_EXCEPTION("[hdecompress] Incorrect index identifier ({:#x}).", magic3);
        }

        p.read32(); // version
        u64 block_offset = p.read64();
        u64 file_offset = p.read64();

        blocks = hbs::readBlockArray(ConstMemory(archive.address + block_offset, file_offset - block_offset));
        files = hbs::readFileArray(ConstMemory(archive.address + file_offset, index_offset - file_offset));
    }

    static
    void appendBlockUse(std::vector<BlockUse>& uses,
        size_t file_index,
        u32 segment_index,
        const hbs::File& file,
        const hbs::File::Segment& segment,
        const std::vector<hbs::Block>& blocks)
    {
        const hbs::Block& block = blocks[segment.block];

        u64 file_offset = 0;
        u64 block_offset = 0;

        if (file.segments.size() > 1 || segment.size == block.uncompressed)
        {
            file_offset = segment.offset;
        }
        else
        {
            block_offset = segment.offset;
        }

        uses.push_back({ file_index, segment_index, file_offset, block_offset, segment.size });
    }

    static
    ConstMemory sliceBlock(ConstMemory block_memory, u64 block_offset, u64 size, const char* context)
    {
        if (block_offset > block_memory.size || size > block_memory.size - block_offset)
        {
            MANGO_EXCEPTION("[hdecompress] {} slice out of bounds (offset={}, size={}, block={}).",
                context, block_offset, size, block_memory.size);
        }

        return block_memory.slice(block_offset, size);
    }

    static
    void validateArchiveBlock(ConstMemory archive, const hbs::Block& block)
    {
        if (block.offset > archive.size || block.compressed > archive.size - block.offset)
        {
            MANGO_EXCEPTION("[hdecompress] Block offset out of archive bounds (offset={}, compressed={}, archive={}).",
                block.offset, block.compressed, archive.size);
        }
    }

    static
    std::vector<size_t> buildBlockSchedule(const std::vector<hbs::File>& files,
        const std::vector<hbs::Block>& blocks,
        const std::vector<std::vector<BlockUse>>& block_uses)
    {
        std::vector<size_t> file_order;
        file_order.reserve(files.size());

        for (size_t i = 0; i < files.size(); ++i)
        {
            if (!files[i].segments.empty())
            {
                file_order.push_back(i);
            }
        }

        std::sort(file_order.begin(), file_order.end(), [&](size_t a, size_t b)
        {
            return files[a].segments.size() > files[b].segments.size();
        });

        std::vector<bool> scheduled(blocks.size());
        std::vector<size_t> schedule;

        for (size_t file_index : file_order)
        {
            for (const auto& segment : files[file_index].segments)
            {
                const size_t block_index = segment.block;

                if (block_index >= blocks.size() || scheduled[block_index])
                {
                    continue;
                }

                if (block_uses[block_index].empty())
                {
                    continue;
                }

                scheduled[block_index] = true;
                schedule.push_back(block_index);
            }
        }

        for (size_t block_index = 0; block_index < blocks.size(); ++block_index)
        {
            if (!block_uses[block_index].empty() && !scheduled[block_index])
            {
                schedule.push_back(block_index);
            }
        }

        return schedule;
    }

    static
    Memory copySlice(ConstMemory slice)
    {
        Buffer buffer(slice.size);
        std::memcpy(buffer.data(), slice.address, slice.size);
        return buffer.acquire();
    }

    static
    void writeSlice(OutputFileStream& output, u64& write_end, u64 file_offset, ConstMemory slice,
        std::atomic<u64>* extract_progress = nullptr)
    {
        if (file_offset != write_end)
        {
            output.seek(s64(file_offset), Stream::SeekMode::Begin);
        }

        if (!extract_progress)
        {
            output.write(slice.address, slice.size);
        }
        else
        {
            constexpr u64 chunk_size = 4 * MB;
            u64 offset = 0;

            while (offset < slice.size)
            {
                u64 bytes = std::min(chunk_size, slice.size - offset);
                output.write(slice.address + offset, bytes);
                extract_progress->fetch_add(bytes, std::memory_order_relaxed);
                offset += bytes;
            }
        }

        write_end = file_offset + slice.size;
    }

    struct MultiFileExtract
    {
        const hbs::File* file = nullptr;
        std::unique_ptr<OutputFileStream> output;
        std::mutex mutex;
        u32 next_segment = 0;
        u32 segments_received = 0;
        u64 write_end = 0;
        std::vector<bool> segments_seen;
        std::map<u32, Memory> pending;
        std::vector<SegmentCrc> segment_crcs;
    };

    static
    void drainPending(MultiFileExtract& multi, std::atomic<u64>* extract_progress = nullptr)
    {
        while (!multi.pending.empty() && multi.pending.begin()->first == multi.next_segment)
        {
            auto it = multi.pending.begin();
            Memory memory = it->second;
            u64 file_offset = multi.file->segments[multi.next_segment].offset;

            writeSlice(*multi.output, multi.write_end, file_offset, memory, extract_progress);
            Buffer::release(memory);

            multi.pending.erase(it);
            ++multi.next_segment;
        }
    }

    static
    void commitMultiSegment(MultiFileExtract& multi,
        u32 segment_index,
        u64 file_offset,
        ConstMemory slice,
        bool extract,
        bool check_checksums,
        u32 expected_checksum,
        std::atomic<u64>* extract_progress = nullptr)
    {
        std::lock_guard<std::mutex> lock(multi.mutex);

        if (check_checksums && expected_checksum)
        {
            u32 crc = crc32c(0, slice);
            multi.segment_crcs[segment_index] = { crc, slice.size };
        }

        if (!extract)
        {
            if (!multi.segments_seen[segment_index])
            {
                multi.segments_seen[segment_index] = true;
                ++multi.segments_received;
            }

            return;
        }

        if (segment_index == multi.next_segment && multi.pending.empty())
        {
            writeSlice(*multi.output, multi.write_end, file_offset, slice, extract_progress);
            ++multi.next_segment;
            drainPending(multi, extract_progress);
            return;
        }

        if (!multi.pending.emplace(segment_index, copySlice(slice)).second)
        {
            MANGO_EXCEPTION("[hdecompress] Duplicate segment index {}.", segment_index);
        }

        drainPending(multi, extract_progress);
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

struct ListEntry
{
    std::string name;
    u64 size;
    u32 checksum;
};

void collectList(const Path& path, const std::string& prefix, std::vector<ListEntry>& entries)
{
    for (auto& node : path)
    {
        if (node.isDirectory())
        {
            if (!node.isContainer())
            {
                collectList(Path(path, node.name), prefix, entries);
            }
        }
        else
        {
            std::string name = removePrefix(path.pathname() + node.name, prefix);
            entries.push_back({ name, node.size, node.checksum });
        }
    }
}

void enumerate(const Path& path, State& state, std::string destination, std::string prefix, int depth)
{
    std::string pathname = removePrefix(path.pathname(), prefix);
    std::string current = destination + pathname;

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
            if (state.print_tree)
            {
                printLine(depth * 2, "- {}", node.name);
            }

            state.total_bytes += node.size;
            ++state.file_count;
        }
    }
}

void printList(const std::vector<ListEntry>& entries)
{
    for (const auto& entry : entries)
    {
        printLine("{:12}  0x{:08x}  {}", entry.size, entry.checksum, entry.name);
    }
}

namespace
{

    static
    void processArchive(ConstMemory archive,
        const std::string& destination,
        bool extract,
        bool check_checksums,
        State& state)
    {
        // 1. Read index
        std::vector<hbs::Block> blocks;
        std::vector<hbs::File> files;
        readArchive(archive, blocks, files);

        std::vector<std::vector<BlockUse>> block_uses(blocks.size());
        std::vector<std::unique_ptr<MultiFileExtract>> multi_states(files.size());
        std::vector<SegmentCrc> single_segment_crcs(files.size());

        u64 total_bytes = 0;
        u64 file_count = 0;

        // 2. Pass files: folders, block uses, multi-file setup
        for (size_t i = 0; i < files.size(); ++i)
        {
            const hbs::File& file = files[i];

            if (file.segments.empty())
            {
                continue;
            }

            total_bytes += file.size;
            ++file_count;

            if (extract)
            {
                std::string pathname = destination + file.filename;
                std::string folder = getPath(pathname);

                if (!folder.empty())
                {
                    std::filesystem::create_directories(folder);
                }
            }

            if (file.segments.size() > 1)
            {
                multi_states[i] = std::make_unique<MultiFileExtract>();
                multi_states[i]->file = &file;
                multi_states[i]->segment_crcs.resize(file.segments.size());
                multi_states[i]->segments_seen.resize(file.segments.size());

                if (extract)
                {
                    multi_states[i]->output = std::make_unique<OutputFileStream>(destination + file.filename);
                }
            }

            for (u32 segment_index = 0; segment_index < file.segments.size(); ++segment_index)
            {
                const auto& segment = file.segments[segment_index];
                appendBlockUse(block_uses[segment.block], i, segment_index, file, segment, blocks);
            }
        }

        u64 indexed_bytes = 0;

        for (const auto& uses : block_uses)
        {
            for (const auto& use : uses)
            {
                indexed_bytes += use.size;
            }
        }

        if (indexed_bytes != total_bytes)
        {
            MANGO_EXCEPTION("[hdecompress] Block coverage mismatch ({} bytes indexed, {} bytes in files).",
                indexed_bytes, total_bytes);
        }

        state.total_bytes = total_bytes;
        state.file_count = file_count;

        std::vector<size_t> schedule = buildBlockSchedule(files, blocks, block_uses);

        if (schedule.empty())
        {
            return;
        }

        // 3. Decode blocks: cap in-flight block tasks (~2× hardware concurrency).
        const size_t concurrency = ThreadPool::getHardwareConcurrency();
        const size_t max_in_flight = std::max(size_t(2), concurrency * 2);

        ConcurrentQueue q("hdecompress.blocks");
        std::atomic<size_t> schedule_pos { 0 };
        std::atomic<size_t> blocks_in_flight { 0 };
        std::atomic<u64> counter_bytes { 0 };
        std::atomic<u64> verified_bytes { 0 };
        std::atomic<size_t> blocks_decompressed { 0 };
        std::atomic<size_t> blocks_verified { 0 };
        std::atomic<size_t> blocks_extracted { 0 };
        std::atomic<u32> verify_errors { 0 };

        struct CompletedBlock
        {
            size_t block_index;
            Memory owned;
        };

        std::mutex completed_mutex;
        std::deque<CompletedBlock> completed;
        std::atomic<size_t> completed_count { 0 };

        u64 time0 = Time::ms();

        auto block_bytes = [&](size_t block_index) -> u64
        {
            u64 bytes = 0;

            for (const auto& use : block_uses[block_index])
            {
                bytes += use.size;
            }

            return bytes;
        };

        auto dispatch_block = [&](size_t block_index, ConstMemory block_memory)
        {
            const auto& uses = block_uses[block_index];
            std::atomic<u64>* extract_progress = extract ? &counter_bytes : nullptr;

            for (const auto& use : uses)
            {
                const hbs::File& file = files[use.file_index];
                MultiFileExtract* multi = multi_states[use.file_index].get();

                ConstMemory slice = sliceBlock(block_memory, use.block_offset, use.size, file.filename.c_str());

                if (multi)
                {
                    commitMultiSegment(*multi, use.segment_index, use.file_offset, slice,
                        extract, check_checksums, file.checksum, extract_progress);
                }
                else
                {
                    if (check_checksums && file.checksum)
                    {
                        u32 crc = crc32c(0, slice);
                        single_segment_crcs[use.file_index] = { crc, slice.size };

                        if (single_segment_crcs[use.file_index].size != file.size ||
                            single_segment_crcs[use.file_index].crc != file.checksum)
                        {
                            ++verify_errors;

                            if (extract || check_checksums)
                            {
                                printLine(Print::Error, "ERROR: {} checksum: {:08x}, expected: {:08x}",
                                    destination + file.filename,
                                    single_segment_crcs[use.file_index].crc,
                                    file.checksum);
                            }
                        }
                    }

                    if (extract)
                    {
                        OutputFileStream output(destination + file.filename);
                        u64 write_end = 0;
                        writeSlice(output, write_end, use.file_offset, slice, extract_progress);
                    }
                }
            }
        };

        auto show_progress = [&]()
        {
            if (extract)
            {
                progress(counter_bytes.load(std::memory_order_relaxed), total_bytes, 42, "Extracting");
            }
            else
            {
                progress(blocks_verified.load(std::memory_order_relaxed), schedule.size(), 42, "Verifying");
            }
        };

        auto pump_blocks = [&]()
        {
            for (;;)
            {
                size_t in_flight = blocks_in_flight.load(std::memory_order_relaxed);
                if (in_flight >= max_in_flight)
                {
                    break;
                }

                size_t pos = schedule_pos.load(std::memory_order_relaxed);
                if (pos >= schedule.size())
                {
                    break;
                }

                if (!blocks_in_flight.compare_exchange_weak(in_flight, in_flight + 1,
                        std::memory_order_acq_rel, std::memory_order_relaxed))
                {
                    continue;
                }

                if (!schedule_pos.compare_exchange_weak(pos, pos + 1,
                        std::memory_order_acq_rel, std::memory_order_relaxed))
                {
                    blocks_in_flight.fetch_sub(1, std::memory_order_relaxed);
                    continue;
                }

                size_t block_index = schedule[pos];

                q.enqueue([&, block_index = block_index]()
                {
                    const hbs::Block& block = blocks[block_index];
                    validateArchiveBlock(archive, block);

                    Memory owned;

                    if (block.method != Compressor::NONE)
                    {
                        ConstMemory compressed(archive.address + block.offset, block.compressed);
                        Buffer buffer(block.uncompressed);
                        Compressor compressor = getCompressor(Compressor::Method(block.method));
                        compressor.decompress(buffer, compressed);
                        owned = buffer.acquire();
                    }

                    if (extract)
                    {
                        ConstMemory payload = owned.address
                            ? ConstMemory(owned)
                            : ConstMemory(archive.address + block.offset, block.compressed);

                        dispatch_block(block_index, payload);

                        if (owned.address)
                        {
                            Buffer::release(owned);
                        }

                        blocks_extracted.fetch_add(1, std::memory_order_relaxed);
                    }
                    else
                    {
                        blocks_decompressed.fetch_add(1, std::memory_order_relaxed);

                        std::lock_guard<std::mutex> lock(completed_mutex);
                        completed.push_back({ block_index, std::move(owned) });
                        completed_count.fetch_add(1, std::memory_order_relaxed);
                    }

                    blocks_in_flight.fetch_sub(1, std::memory_order_relaxed);
                });
            }
        };

        auto drain_one = [&]() -> bool
        {
            CompletedBlock item;

            {
                std::lock_guard<std::mutex> lock(completed_mutex);

                if (completed.empty())
                {
                    return false;
                }

                item = std::move(completed.front());
                completed.pop_front();
                completed_count.fetch_sub(1, std::memory_order_relaxed);
            }

            const hbs::Block& block = blocks[item.block_index];
            ConstMemory payload = item.owned.address
                ? ConstMemory(item.owned)
                : ConstMemory(archive.address + block.offset, block.compressed);

            dispatch_block(item.block_index, payload);

            if (item.owned.address)
            {
                Buffer::release(item.owned);
            }

            verified_bytes.fetch_add(block_bytes(item.block_index), std::memory_order_relaxed);
            blocks_verified.fetch_add(1, std::memory_order_relaxed);
            return true;
        };

        pump_blocks();

        show_progress();

        if (extract)
        {
            while (blocks_extracted.load(std::memory_order_relaxed) < schedule.size())
            {
                u64 bytes_before = counter_bytes.load(std::memory_order_relaxed);

                pump_blocks();
                q.steal();
                show_progress();

                if (counter_bytes.load(std::memory_order_relaxed) == bytes_before)
                {
                    Sleep::ms(1);
                }
            }
        }
        else
        {
            const size_t drain_batch = 32;

            while (verified_bytes.load(std::memory_order_relaxed) < total_bytes ||
                completed_count.load(std::memory_order_relaxed) > 0)
            {
                size_t decompressed_before = blocks_decompressed.load(std::memory_order_relaxed);

                pump_blocks();
                q.steal();

                for (size_t i = 0; i < drain_batch; ++i)
                {
                    if (!drain_one())
                    {
                        break;
                    }

                    q.steal();
                }

                show_progress();

                if (verified_bytes.load(std::memory_order_relaxed) >= total_bytes &&
                    completed_count.load(std::memory_order_relaxed) == 0)
                {
                    break;
                }

                if (blocks_decompressed.load(std::memory_order_relaxed) == decompressed_before &&
                    completed_count.load(std::memory_order_relaxed) == 0)
                {
                    Sleep::ms(1);
                }
            }
        }

        q.wait();

        while (drain_one())
        {
        }

        show_progress();

        // 4. Verify multi-segment file checksums
        for (size_t i = 0; i < files.size(); ++i)
        {
            MultiFileExtract* multi = multi_states[i].get();
            if (!multi)
            {
                continue;
            }

            const hbs::File& file = files[i];

            if (extract)
            {
                if (multi->next_segment != file.segments.size() || !multi->pending.empty())
                {
                    MANGO_EXCEPTION("[hdecompress] Incomplete multi-segment extract for \"{}\" (next={}, pending={}, segments={}).",
                        file.filename, multi->next_segment, multi->pending.size(), file.segments.size());
                }
            }
            else if (multi->segments_received != file.segments.size())
            {
                MANGO_EXCEPTION("[hdecompress] Incomplete multi-segment verify for \"{}\" (received={}, segments={}).",
                    file.filename, multi->segments_received, file.segments.size());
            }

            if (!check_checksums || !file.checksum)
            {
                continue;
            }

            u32 crc = 0;
            u64 verified_bytes = 0;

            for (const SegmentCrc& segment : multi->segment_crcs)
            {
                crc = crc32c_combine(crc, segment.crc, segment.size);
                verified_bytes += segment.size;
            }

            if (verified_bytes != file.size || crc != file.checksum)
            {
                ++verify_errors;

                printLine(Print::Error, "ERROR: {} checksum: {:08x}, expected: {:08x}",
                    destination + file.filename, crc, file.checksum);
            }
        }

        u64 time1 = Time::ms();
        u64 elapsed = std::max(u64(1), time1 - time0);

        state.verify_errors = verify_errors;

        if (extract)
        {
            replaceLine("Extracted {} files ({:0.1f} MB) in {:0.2f} seconds ({} MB/s)",
                state.file_count,
                state.total_bytes / double(MB),
                elapsed / 1000.0,
                state.total_bytes / (elapsed * KB));
        }
        else if (check_checksums)
        {
            replaceLine("Verified {} archive entries ({:0.1f} MB) in {:0.2f} seconds ({} MB/s)",
                state.file_count,
                state.total_bytes / double(MB),
                elapsed / 1000.0,
                state.total_bytes / (elapsed * KB));
        }
    }

} // namespace

int main(int argc, char* argv[])
{
    std::string program_name = removePath(argv[0]);

    if (argc < 2)
    {
        printLine("");
        printLine("HBS Decompression Tool version 0.4");
        printLine("Copyright (C) 2026 Twilight 3D Finland, Oy. All rights reserved.");
        printLine("Usage: {} [archive] [destination]", program_name);
        printLine("       {} [archive] --list", program_name);
        printLine("       {} [archive] --tree", program_name);
        printLine("       {} [archive] [destination] [--verify]", program_name);
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

    File archive_file(filename);

    try
    {
        if (state.print_list || state.print_tree)
        {
            Path path(archive_file, ".hbs");
            std::string prefix = path.pathname();

            if (state.print_list)
            {
                u64 list_time0 = Time::ms();

                std::vector<ListEntry> entries;
                collectList(path, prefix, entries);
                printList(entries);

                state.file_count = entries.size();
                state.total_bytes = 0;

                for (const auto& entry : entries)
                {
                    state.total_bytes += entry.size;
                }

                u64 list_time1 = Time::ms();

                printLine("");
                printLine("List: {} files ({:0.1f} MB) in {:0.2f} seconds",
                    state.file_count,
                    state.total_bytes / double(MB),
                    (list_time1 - list_time0) / 1000.0);
                printLine("");
            }
            else
            {
                u64 tree_time0 = Time::ms();

                enumerate(path, state, destination, prefix, 0);

                u64 tree_time1 = Time::ms();

                printLine("");
                printLine("Tree: {} files ({:0.1f} MB) in {:0.2f} seconds",
                    state.file_count,
                    state.total_bytes / double(MB),
                    (tree_time1 - tree_time0) / 1000.0);
                printLine("");
            }
        }

        if (state.decompress || state.verify)
        {
            if (state.verify)
            {
                state.file_count = 0;
                state.total_bytes = 0;

                bool do_extract = false;
                bool do_checksum = true;
                processArchive(archive_file, destination, do_extract, do_checksum, state);

                if (state.verify_errors > 0)
                {
                    printLine("Status: FAILED (count: {})", state.verify_errors);
                }
                else
                {
                    printLine("Status: PASSED");

                    if (state.decompress)
                    {
                        printLine("");

                        state.file_count = 0;
                        state.total_bytes = 0;

                        bool do_extract = true;
                        bool do_checksum = false;
                        processArchive(archive_file, destination, do_extract, do_checksum, state);
                        printLine("");
                    }
                }
            }
            else
            {
                state.file_count = 0;
                state.total_bytes = 0;

                bool do_extract = true;
                bool do_checksum = false;
                processArchive(archive_file, destination, do_extract, do_checksum, state);
                printLine("");
            }
        }
    }
    catch (Exception& e)
    {
        printLine(Print::Error, "{}", e.what());
        return 1;
    }

    if (state.verify_errors > 0)
    {
        return 1;
    }

    return 0;
}
