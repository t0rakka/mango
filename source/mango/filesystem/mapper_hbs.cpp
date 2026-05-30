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

    struct Segment
    {
        u32 block;
        u64 offset;
        u64 size;
    };

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

            parseBlocks(memory.address,
                ConstMemory(memory.address + block_offset, file_offset - block_offset));

            parseFileArray(fs::hbs::readFileArray(
                ConstMemory(memory.address + file_offset, index_offset - file_offset)));
        }

        ~IndexHBS()
        {
        }

        void parseBlocks(const u8* base, ConstMemory memory)
        {
            std::vector<fs::hbs::Block> blocks = fs::hbs::readBlockArray(memory);

            m_blocks.reserve(blocks.size());

            for (const auto& desc : blocks)
            {
                Block block;
                block.compressed = ConstMemory(base + desc.offset, desc.compressed);
                block.uncompressed = desc.uncompressed;
                block.method = desc.method;
                m_blocks.push_back(block);
            }
        }

        void parseFileArray(const std::vector<fs::hbs::File>& files)
        {
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
        LRUCache<u32, std::shared_ptr<Buffer>> m_cache { 6 };
        std::mutex m_cache_mutex;

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

                u32 blockIndex = segment.block;
                const Block& block = m_index.m_blocks[blockIndex];

                assert(file.size == segment.size);

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

                        ConstMemory memory(*buffer + segment.offset, size_t(segment.size));
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
                    ConstMemory memory(block.compressed.address + segment.offset, size_t(segment.size));
                    return std::make_unique<VirtualMemoryHBS>(memory);
                }
            }

            // generic multi-segment case

            std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(file.size);
            u8* base = buffer->data();

            ConcurrentQueue q("hbs.decompressor");

            for (const auto& segment : file.segments)
            {
                const Block& block = m_index.m_blocks[segment.block];

                q.enqueue([=, &block, &segment]
                {
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
                        std::memcpy(base + segment.offset, block.compressed.address, size_t(segment.size));
                    }
                });
            }

            q.wait();

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
