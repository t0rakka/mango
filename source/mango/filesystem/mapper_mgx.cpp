/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/pointer.hpp>
#include <mango/core/string.hpp>
#include <mango/core/exception.hpp>
#include <mango/filesystem/mapper.hpp>
#include <mango/filesystem/path.hpp>
#include <mango/image/fourcc.hpp>

#define ID ".mgx mapper: "

namespace
{
    using namespace mango;

    constexpr u64 mgx_header_size = 24;

    struct HeaderMGX
    {
        Memory blocks;
        Memory files;
        bool status = false;

        HeaderMGX(Memory memory)
        {
            if (!memory.address)
                return;

            u64 header_offset = memory.size - mgx_header_size;
            LittleEndianPointer p = memory.address + header_offset;

            u32 magic = p.read32();
            u32 version = p.read32();
            u64 block_offset = p.read64();
            u64 file_offset = p.read64();

            if (magic != makeFourCC('m', 'g', 'x', '0'))
            {
                // Incorrect header magic
                return;
            }

            blocks = Memory(memory.address + block_offset, file_offset - block_offset);
            files = Memory(memory.address + file_offset, header_offset - file_offset);

            status = true;
            MANGO_UNREFERENCED_PARAMETER(version);
        }
    };

    struct Block
    {
        u64 offset;
        u64 compressed;
        u64 uncompressed;
        u32 method;
    };

    struct FileHeader
    {
        struct Segment
        {
            u32 block;
            u32 offset;
        };

        std::string filename;
        u64 size;
        std::vector<Segment> segments;

        bool isFolder() const
        {
            size_t n = filename.find_first_of("/");
            if (n != std::string::npos)
            {
                if (n == (filename.length() - 1))
                {
                    return true;
                }
            }

            return false;
            //return segments.empty();
        }
    };

} // namespace

namespace mango
{

    // -----------------------------------------------------------------
    // VirtualMemoryMGX
    // -----------------------------------------------------------------

    class VirtualMemoryMGX : public mango::VirtualMemory
    {
    protected:
        uint8* m_delete_address;

    public:
        VirtualMemoryMGX(uint8* address, uint8* delete_address, size_t size)
            : m_delete_address(delete_address)
        {
            m_memory = Memory(address, size);
        }

        ~VirtualMemoryMGX()
        {
            delete [] m_delete_address;
        }
    };

    // -----------------------------------------------------------------
    // MapperMGX
    // -----------------------------------------------------------------

    class MapperMGX : public AbstractMapper
    {
    public:
        Memory m_parent_memory;
        std::string m_password;
        std::map<std::string, FileHeader> m_files;
        std::vector<Block> m_blocks;

    protected:
        void read_blocks(const HeaderMGX& header)
        {
            LittleEndianPointer p = header.blocks.address;

            u32 magic = p.read32();
            if (magic != makeFourCC('m', 'g', 'x', '1'))
            {
                // Incorrect file index magic
                return;
            }

            u32 num_blocks = p.read32();
            for (u32 i = 0; i < num_blocks; ++i)
            {
                Block block;
                block.offset = p.read64();
                block.compressed = p.read64();
                block.uncompressed = p.read64();
                block.method = p.read64();
                m_blocks.push_back(block);
            }
        }

        void read_files(const HeaderMGX& header)
        {
            LittleEndianPointer p = header.files.address;

            u32 magic = p.read32();
            if (magic != makeFourCC('m', 'g', 'x', '2'))
            {
                // Incorrect file index magic
                return;
            }

            u32 num_files = p.read32();
            for (u32 i = 0; i < num_files; ++i)
            {
                FileHeader file;

                u32 length = p.read32();
                const u8* ptr = p;
                std::string filename(reinterpret_cast<const char *>(ptr), length);
                p += length;
                //printf(" # %s \n", filename.c_str());

                file.filename = filename;
                file.size = p.read64();
                u32 num_segment = p.read32();
                for (u32 j = 0; j < num_segment; ++j)
                {
                    u32 block = p.read32(); // block
                    u32 offset = p.read32(); // offset
                    //printf("   - blk: %d, off: %d\n", block, offset);
                    file.segments.push_back({block, offset});
                }

                m_files[filename] = file;
            }
        }

    public:
        MapperMGX(Memory parent, const std::string& password)
            : m_parent_memory(parent)
            , m_password(password)
        {
                HeaderMGX header(parent);
                if (header.status)
                {
                    read_blocks(header);
                    read_files(header);
                }
        }

        bool isFile(const std::string& filename) const override
        {
            auto i = m_files.find(filename);
            if (i != m_files.end())
            {
                const FileHeader& file = i->second;
                return !file.isFolder();
            }

            return false;
        }

        void getIndex(FileIndex& index, const std::string& pathname) override
        {
            for (auto i : m_files)
            {
                const FileHeader& file = i.second;
                std::string filename = file.filename;

                if (isPrefix(filename, pathname))
                {
                    filename = filename.substr(pathname.length());

                    size_t n = filename.find_first_of("/");

                    if (n != std::string::npos)
                    {
                        if (n == (filename.length() - 1))
                        {
                            index.emplace(filename, 0, FileInfo::DIRECTORY);
                        }
                    }
                    else
                    {
                        u32 flags = 0;
                        /*
                        if (file.compression > 0)
                        {
                            flags |= FileInfo::COMPRESSED;
                        }
                        */

                        index.emplace(filename, file.size, flags);
                    }
                }
            }
        }

        VirtualMemory* mmap(const std::string& filename) override
        {
            auto i = m_files.find(filename);
            if (i == m_files.end())
            {
                MANGO_EXCEPTION(ID"File not found.");
            }

            const FileHeader& file = i->second;

            u8* ptr = new u8[file.size];

            for (auto &segment : file.segments)
            {
                const Block& block = m_blocks[segment.block];
                u32 offset = segment.offset;

                //block.offset;
                //block.compressed;
                //block.uncompressed;
                //block.method;
                (void) block;
                (void) offset;
            }

            // TODO: decompress
            VirtualMemoryMGX* vm = new VirtualMemoryMGX(ptr, ptr, file.size);
            return vm;
        }
    };

    // -----------------------------------------------------------------
    // functions
    // -----------------------------------------------------------------

    AbstractMapper* createMapperMGX(Memory parent, const std::string& password)
    {
        AbstractMapper* mapper = new MapperMGX(parent, password);
        return mapper;
    }

} // namespace mango
