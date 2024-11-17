/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/filesystem/filesystem.hpp>
#include <mango/image/fourcc.hpp>
#include "indexer.hpp"

namespace
{
    using namespace mango;
    namespace fs = mango::filesystem;

    static constexpr u64 mgx_header_size = 24;

    struct Segment
    {
        u32 block;
        u32 offset;
        u32 size;
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

    struct HeaderMGX
    {
        ConstMemory m_memory;
        fs::Indexer<FileHeader> m_folders;
        std::vector<Block> m_blocks;

        HeaderMGX(ConstMemory memory)
            : m_memory(memory)
        {
            if (!memory.address)
            {
                // MANGO_EXCEPTION("[mapper.mgx] Parent container doesn't have memory");
                return;
            }

            LittleEndianConstPointer p = memory.address;
            u32 magic0 = p.read32();
            if (magic0 != u32_mask('m', 'g', 'x', '0'))
            {
                //MANGO_EXCEPTION("[mapper.mgx] Incorrect file identifier (%x)", magic0);
                return;
            }

            u64 header_offset = memory.size - mgx_header_size;
            p = memory.address + header_offset;

            u32 magic3 = p.read32();
            if (magic3 != u32_mask('m', 'g', 'x', '3'))
            {
                //MANGO_EXCEPTION("[mapper.mgx] Incorrect header identifier (%x)", magic3);
                return;
            }

            u32 version = p.read32();
            u64 block_offset = p.read64();
            u64 file_offset = p.read64();

            parseBlocks(memory.address + block_offset);
            parseFiles(memory.address + file_offset);

            MANGO_UNREFERENCED(version);
        }

        ~HeaderMGX()
        {
        }

        void parseBlocks(LittleEndianConstPointer p)
        {
            u32 magic1 = p.read32();
            if (magic1 != u32_mask('m', 'g', 'x', '1'))
            {
                MANGO_EXCEPTION("[mapper.mgx] Incorrect block identifier (:#x)", magic1);
            }

            u32 num_blocks = p.read32();
            for (u32 i = 0; i < num_blocks; ++i)
            {
                Block block;

                u64 offset = p.read64();
                u64 compressed = p.read64();
                block.compressed = ConstMemory(m_memory.address + offset, compressed);
                block.uncompressed = p.read64();
                block.method = p.read32();

                m_blocks.push_back(block);
            }

            u32 magic2 = p.read32();
            if (magic2 != u32_mask('m', 'g', 'x', '2'))
            {
                MANGO_EXCEPTION("[mapper.mgx] Incorrect block terminator (:#x)", magic2);
            }
        }

        void parseFiles(LittleEndianConstPointer p)
        {
            u32 magic2 = p.read32();
            if (magic2 != u32_mask('m', 'g', 'x', '2'))
            {
                MANGO_EXCEPTION("[mapper.mgx] Incorrect block identifier (:#x)", magic2);
            }

            u64 compressed = p.read64();
            u64 uncompressed = p.read64();

            ConstMemory source(p, compressed);
            Buffer temp(uncompressed);

            CompressionStatus status = zstd::decompress(temp, source);
            if (status.size != uncompressed)
            {
                MANGO_EXCEPTION("[mapper.mgx] Incorrect compressed index");
            }

            parseFileArray(temp.data());
            p += compressed;

            u32 magic3 = p.read32();
            if (magic3 != u32_mask('m', 'g', 'x', '3'))
            {
                MANGO_EXCEPTION("[mapper.mgx] Incorrect block terminator (:#x)", magic3);
            }
        }

        void parseFileArray(LittleEndianConstPointer p)
        {
            u32 num_files = p.read32();
            for (u32 i = 0; i < num_files; ++i)
            {
                FileHeader header;

                u32 length = p.read32();
                const u8* ptr = p;
                std::string filename(reinterpret_cast<const char *>(ptr), length);
                p += length;

                header.size = p.read64();
                header.checksum = p.read32();
                header.is_compressed = false;

                u32 num_segment = p.read32();
                for (u32 j = 0; j < num_segment; ++j)
                {
                    u32 block_idx = p.read32();
                    u32 offset = p.read32();
                    u32 size = p.read32();
                    header.segments.push_back({block_idx, offset, size});

                    // inspect block
                    Block& block = m_blocks[block_idx];
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
    // VirtualMemoryMGX
    // -----------------------------------------------------------------

    class VirtualMemoryMGX : public mango::VirtualMemory
    {
    protected:
        std::shared_ptr<Buffer> m_buffer;

    public:
        VirtualMemoryMGX(ConstMemory memory)
        {
            m_memory = memory;
        }

        VirtualMemoryMGX(std::shared_ptr<Buffer> buffer, ConstMemory memory)
            : m_buffer(buffer)
        {
            m_memory = memory;
        }

        ~VirtualMemoryMGX()
        {
        }
    };

    // -----------------------------------------------------------------
    // MapperMGX
    // -----------------------------------------------------------------

    class MapperMGX : public AbstractMapper
    {
    protected:
        HeaderMGX m_header;
        std::string m_password;

        // block cache
        LRUCache<u32, std::shared_ptr<Buffer>, 6> m_cache;
        std::mutex m_cache_mutex;

    public:
        MapperMGX(ConstMemory parent, const std::string& password)
            : m_header(parent)
            , m_password(password)
        {
        }

        bool isFile(const std::string& filename) const override
        {
            const FileHeader* header = m_header.m_folders.getHeader(filename);
            if (header)
            {
                return !header->isFolder();
            }
            return false;
        }

        void getIndex(FileIndex& index, const std::string& pathname) override
        {
            const fs::Indexer<FileHeader>::Folder* folder = m_header.m_folders.getFolder(pathname);
            if (folder)
            {
                for (auto i : folder->headers)
                {
                    const FileHeader& header = *i.second;

                    u32 flags = 0;

                    if (header.isFolder())
                    {
                        flags |= FileInfo::DIRECTORY;
                    }

                    if (header.isCompressed())
                    {
                        flags |= FileInfo::COMPRESSED;
                    }

                    index.emplace(header.filename, header.size, flags);
                }
            }
        }

        std::unique_ptr<VirtualMemory> map(const std::string& filename) override
        {
            const FileHeader* ptrHeader = m_header.m_folders.getHeader(filename);
            if (!ptrHeader)
            {
                MANGO_EXCEPTION("[mapper.mgx] File \"{}\" not found.", filename);
            }

            const FileHeader& file = *ptrHeader;

            if (!file.isMultiSegment())
            {
                const Segment& segment = file.segments[0];

                u32 blockIndex = segment.block;
                const Block& block = m_header.m_blocks[blockIndex];

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

                        ConstMemory memory(*buffer + segment.offset, segment.size);
                        return std::make_unique<VirtualMemoryMGX>(buffer, memory);
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
                    return std::make_unique<VirtualMemoryMGX>(memory);
                }
            }

            // generic compression case

            std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(file.size);
            u8* x = *buffer;

            ConcurrentQueue q("mgx.decompressor");

            for (const auto& segment : file.segments)
            {
                const Block& block = m_header.m_blocks[segment.block];

                if (block.method)
                {
                    q.enqueue([=, &block, &segment]
                    {
                        if (block.uncompressed == segment.size && segment.offset == 0)
                        {
                            // segment is full-block so we can decode directly w/o intermediate buffer
                            Memory dest(x, size_t(block.uncompressed));
                            block.decompress(dest);
                        }
                        else
                        {
                            // we must decompress the whole block so need a temporary buffer
                            Buffer dest(block.uncompressed);
                            block.decompress(dest);

                            // copy the segment out from the temporary buffer
                            std::memcpy(x, dest.data() + segment.offset, segment.size);
                        }
                    });
                }
                else
                {
                    q.enqueue([=, &block, &segment]
                    {
                        // no compression
                        std::memcpy(x, block.compressed.address + segment.offset, segment.size);
                    });
                }

                x += segment.size;
            }

            q.wait();

            ConstMemory memory = *buffer;
            return std::make_unique<VirtualMemoryMGX>(buffer, memory);
        }
    };

    // -----------------------------------------------------------------
    // functions
    // -----------------------------------------------------------------

    AbstractMapper* createMapperMGX(ConstMemory parent, const std::string& password)
    {
        AbstractMapper* mapper = new MapperMGX(parent, password);
        return mapper;
    }

} // namespace mango::filesystem
