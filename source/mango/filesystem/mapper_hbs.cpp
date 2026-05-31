/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/filesystem/filesystem.hpp>
#include <mango/filesystem/hbs.hpp>
#include <mango/image/fourcc.hpp>
#include "indexer.hpp"

namespace
{
    using namespace mango;
    namespace fs = mango::filesystem;

    static constexpr u64 hbs_index_size = 24;
    static constexpr u32 block_cache_size = 96;

    using Segment = fs::hbs::File::Segment;

    struct Block
    {
        ConstMemory compressed;
        u64 uncompressed;
        u32 method;

        void decompress(Memory dest) const
        {
            assert(dest.size == uncompressed);
            Compressor compressor = getCompressor(Compressor::Method(method));
            compressor.decompress(dest, compressed);
        }
    };

    struct FileHeader
    {
        u64 size;
        u32 checksum;
        bool is_compressed;
        std::vector<Segment> segments;
        std::string filename;

        bool isCompressed() const
        {
            return is_compressed;
        }

        bool isMultiSegment() const
        {
            return segments.size() > 1;
        }

        bool isFolder() const
        {
            return segments.empty();
        }
    };

    struct IndexHBS
    {
        ConstMemory m_memory;
        fs::Indexer<FileHeader> m_folders;
        std::vector<Block> m_blocks;
        u32 m_version { 0 };

        IndexHBS(ConstMemory memory)
            : m_memory(memory)
        {
            if (!memory.address)
            {
                // MANGO_EXCEPTION("[mapper.hbs] Parent container doesn't have memory");
                return;
            }

            LittleEndianConstPointer p = memory.address;
            u32 magic0 = p.read32();
            if (magic0 != fs::HBS_MAGIC0)
            {
                //MANGO_EXCEPTION("[mapper.hbs] Incorrect file identifier (%x)", magic0);
                return;
            }

            u64 index_offset = memory.size - hbs_index_size;
            p = memory.address + index_offset;

            u32 magic3 = p.read32();
            if (magic3 != fs::HBS_MAGIC3)
            {
                //MANGO_EXCEPTION("[mapper.hbs] Incorrect index identifier (%x)", magic3);
                return;
            }

            m_version = p.read32();
            u64 block_offset = p.read64();
            u64 file_offset = p.read64();

            u64 block_size = file_offset - block_offset;
            u64 file_size = index_offset - file_offset;

            ConstMemory block_memory = m_memory.slice(block_offset, block_size);
            ConstMemory file_memory = m_memory.slice(file_offset, file_size);

            parseBlocks(block_memory);
            parseFileArray(file_memory);
        }

        ~IndexHBS()
        {
        }

        void parseBlocks(ConstMemory block_memory)
        {
            std::vector<fs::hbs::Block> blocks = fs::hbs::readBlockArray(block_memory);

            m_blocks.reserve(blocks.size());

            const u8* archive_end = m_memory.address + m_memory.size;

            for (const auto& desc : blocks)
            {
                const u8* block_address = m_memory.address + desc.offset;
                const u8* block_end = block_address + desc.compressed;

                if (block_address < m_memory.address || block_end > archive_end)
                {
                    MANGO_EXCEPTION("[mapper.hbs] Block at offset {} exceeds archive bounds.", desc.offset);
                }

                Block block;
                block.compressed = ConstMemory(block_address, desc.compressed);
                block.uncompressed = desc.uncompressed;
                block.method = desc.method;
                m_blocks.push_back(block);
            }
        }

        void parseFileArray(ConstMemory file_memory)
        {
            std::vector<fs::hbs::File> files = fs::hbs::readFileArray(file_memory);

            for (const auto& entry : files)
            {
                FileHeader header;

                const std::string& filename = entry.filename;
                u32 length = u32(filename.length());

                header.size = entry.size;
                header.checksum = entry.checksum;
                header.is_compressed = false;

                for (const auto& segment : entry.segments)
                {
                    if (segment.block >= m_blocks.size())
                    {
                        MANGO_EXCEPTION("[mapper.hbs] File \"{}\" references block {} ({} blocks).",
                            filename, segment.block, m_blocks.size());
                    }

                    header.segments.push_back({ segment.block, segment.offset, segment.size });

                    Block& block = m_blocks[segment.block];
                    if (block.method > 0)
                    {
                        // if ANY of the blocks in the file segments is compressed
                        // the whole file is considered compressed (= cannot be mapped directly)
                        header.is_compressed = true;
                    }
                }

                std::string folder = header.isFolder() ?
                    fs::getPath(filename.substr(0, length - 1)) :
                    fs::getPath(filename);

                header.filename = filename.substr(folder.length());
                m_folders.insert(folder, filename, header);
            }
        }
    };

} // namespace

namespace mango::filesystem
{

    // -----------------------------------------------------------------
    // VirtualMemoryHBS
    // -----------------------------------------------------------------

    class VirtualMemoryHBS : public mango::VirtualMemory
    {
    protected:
        std::shared_ptr<Buffer> m_buffer;

    public:
        VirtualMemoryHBS(ConstMemory memory)
        {
            m_memory = memory;
        }

        VirtualMemoryHBS(std::shared_ptr<Buffer> buffer, ConstMemory memory)
            : m_buffer(buffer)
        {
            m_memory = memory;
        }

        ~VirtualMemoryHBS()
        {
        }
    };

    // -----------------------------------------------------------------
    // MapperHBS
    // -----------------------------------------------------------------

    class MapperHBS : public AbstractMapper
    {
    protected:
        IndexHBS m_index;
        std::string m_password;

        // block cache
        LRUCache<u32, std::shared_ptr<Buffer>> m_cache { block_cache_size };
        std::mutex m_cache_mutex;

        void decompressSegment(u8* base, const Segment& segment, const std::string& filename) const
        {
            if (segment.block >= m_index.m_blocks.size())
            {
                MANGO_EXCEPTION("[mapper.hbs] File \"{}\" references block {} ({} blocks).",
                    filename, segment.block, m_index.m_blocks.size());
            }

            const Block& block = m_index.m_blocks[segment.block];

            if (block.method)
            {
                if (block.uncompressed == segment.size)
                {
                    // segment owns the whole block
                    Memory dest(base + segment.offset, size_t(segment.size));
                    block.decompress(dest);
                }
                else
                {
                    // small files sharing one block
                    Buffer dest(block.uncompressed);
                    block.decompress(dest);
                    std::memcpy(base + segment.offset, dest.data() + segment.offset, size_t(segment.size));
                }
            }
            else
            {
                if (segment.size > block.compressed.size)
                {
                    MANGO_EXCEPTION("[mapper.hbs] File \"{}\" segment exceeds stored block (size {}, block {}).",
                        filename, segment.size, block.compressed.size);
                }

                std::memcpy(base + segment.offset, block.compressed.address, size_t(segment.size));
            }
        }

    public:
        MapperHBS(ConstMemory parent, const std::string& password)
            : m_index(parent)
            , m_password(password)
        {
        }

        u64 getSize(const std::string& filename) const override
        {
            const FileHeader* header = m_index.m_folders.getHeader(filename);
            if (header)
            {
                return header->size;
            }
            return 0;
        }

        bool isFile(const std::string& filename) const override
        {
            const FileHeader* header = m_index.m_folders.getHeader(filename);
            if (header)
            {
                return !header->isFolder();
            }
            return false;
        }

        void getIndex(FileIndex& index, const std::string& pathname) override
        {
            const fs::Indexer<FileHeader>::Folder* folder = m_index.m_folders.getFolder(pathname);
            if (folder)
            {
                for (auto i : folder->headers)
                {
                    const FileHeader& header = *i.second;

                    u32 flags = 0;

                    if (header.isFolder())
                    {
                        flags |= FileInfo::Directory;
                    }

                    if (header.isCompressed())
                    {
                        flags |= FileInfo::Compressed;
                    }

                    index.emplace(header.filename, header.size, flags, header.checksum);
                }
            }
        }

        static
        ConstMemory blockSlice(const Block& block, u64 offset, u64 size, const std::string& filename)
        {
            if (offset + size > block.compressed.size)
            {
                MANGO_EXCEPTION("[mapper.hbs] File \"{}\" segment exceeds stored block (offset {}, size {}, block {}).",
                    filename, offset, size, block.compressed.size);
            }

            return block.compressed.slice(offset, size);
        }

        std::unique_ptr<VirtualMemory> map(const std::string& filename) override
        {
            const FileHeader* ptrHeader = m_index.m_folders.getHeader(filename);
            if (!ptrHeader)
            {
                MANGO_EXCEPTION("[mapper.hbs] File \"{}\" not found.", filename);
            }

            const FileHeader& file = *ptrHeader;

            if (file.isFolder())
            {
                MANGO_EXCEPTION("[mapper.hbs] Cannot map directory \"{}\".", filename);
            }

            if (!file.isMultiSegment())
            {
                if (file.segments.empty())
                {
                    MANGO_EXCEPTION("[mapper.hbs] File \"{}\" has no data.", filename);
                }

                const Segment& segment = file.segments[0];

                if (segment.block >= m_index.m_blocks.size())
                {
                    MANGO_EXCEPTION("[mapper.hbs] File \"{}\" references block {} ({} blocks).",
                        filename, segment.block, m_index.m_blocks.size());
                }

                u32 blockIndex = segment.block;
                const Block& block = m_index.m_blocks[blockIndex];

                if (file.size != segment.size)
                {
                    MANGO_EXCEPTION("[mapper.hbs] File \"{}\" size mismatch ({} != {}).",
                        filename, file.size, segment.size);
                }

                if (file.isCompressed())
                {
                    if (segment.size != block.uncompressed && !file.isMultiSegment())
                    {
                        // a small file stored in one block with other small files

                        std::shared_ptr<Buffer> buffer;

                        std::lock_guard<std::mutex> cache_lock(m_cache_mutex);

                        auto value = m_cache.get(blockIndex);
                        if (value)
                        {
                            // cache hit
                            buffer = *value;
                        }
                        else
                        {
                            // cache miss
                            buffer = std::make_shared<Buffer>(block.uncompressed);
                            block.decompress(*buffer);
                            m_cache.insert(blockIndex, buffer);
                        }

                        ConstMemory block_memory = *buffer;
                        ConstMemory memory = block_memory.slice(segment.offset, segment.size);
                        return std::make_unique<VirtualMemoryHBS>(buffer, memory);
                    }
                    else
                    {
                        // fall-through into the generic case
                    }
                }
                else
                {
                    // The file is encoded as a single, non-compressed block
                    // we can simply map it into parent's memory
                    ConstMemory memory = blockSlice(block, segment.offset, segment.size, filename);
                    return std::make_unique<VirtualMemoryHBS>(memory);
                }
            }

            // generic multi-segment case (also single-segment compressed blocks)

            std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(file.size);
            u8* base = buffer->data();

            for (const auto& segment : file.segments)
            {
                decompressSegment(base, segment, filename);
            }

            ConstMemory memory = *buffer;
            return std::make_unique<VirtualMemoryHBS>(buffer, memory);
        }
    };

    // -----------------------------------------------------------------
    // functions
    // -----------------------------------------------------------------

    AbstractMapper* createMapperHBS(ConstMemory parent, const std::string& password)
    {
        AbstractMapper* mapper = new MapperHBS(parent, password);
        return mapper;
    }

} // namespace mango::filesystem
