/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/pointer.hpp>
#include <mango/core/string.hpp>
#include <mango/core/compress.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/exception.hpp>
#include <mango/filesystem/mapper.hpp>
#include <mango/filesystem/path.hpp>
#include <mango/image/fourcc.hpp>

#define ID "[mapper.mgx] "

namespace
{
    using namespace mango;

    // TODO: the compressor query should be in the compress.hpp

    struct Compressor
    {
        enum Method
        {
            COMPRESS_NONE = 0,
            COMPRESS_MINIZ,
            COMPRESS_BZIP2,
            COMPRESS_LZ4,
            COMPRESS_LZO,
            COMPRESS_ZSTD,
            COMPRESS_LZFSE,
            COMPRESS_LZMA,
            COMPRESS_LZMA2,
            COMPRESS_PPMD8,
        };

        size_t (*bound)(size_t size) = nullptr;
        size_t (*compress)(Memory dest, Memory source, int level) = nullptr;
        void (*decompress)(Memory dest, Memory source) = nullptr;
        const char* name = nullptr;
        Method method = COMPRESS_NONE;
    };

    Compressor getCompressor(Compressor::Method method)
    {
        Compressor compressor;

        switch (method)
        {
            case Compressor::COMPRESS_NONE:
                // TODO: fill w/ memcpy "compression"
                break;
            case Compressor::COMPRESS_MINIZ:
                compressor.bound = miniz::bound;
                compressor.compress = miniz::compress;
                compressor.decompress = miniz::decompress;
                compressor.name = "miniz";
                compressor.method = Compressor::COMPRESS_MINIZ;
                break;
            case Compressor::COMPRESS_BZIP2:
                compressor.bound = bzip2::bound;
                compressor.compress = bzip2::compress;
                compressor.decompress = bzip2::decompress;
                compressor.name = "bzip2";
                compressor.method = Compressor::COMPRESS_BZIP2;
            case Compressor::COMPRESS_LZ4:
                compressor.bound = lz4::bound;
                compressor.compress = lz4::compress;
                compressor.decompress = lz4::decompress;
                compressor.name = "lz4";
                compressor.method = Compressor::COMPRESS_LZ4;
                break;
            case Compressor::COMPRESS_LZO:
                compressor.bound = lzo::bound;
                compressor.compress = lzo::compress;
                compressor.decompress = lzo::decompress;
                compressor.name = "lzo";
                compressor.method = Compressor::COMPRESS_LZO;
                break;
            case Compressor::COMPRESS_ZSTD:
                compressor.bound = zstd::bound;
                compressor.compress = zstd::compress;
                compressor.decompress = zstd::decompress;
                compressor.name = "zstd";
                compressor.method = Compressor::COMPRESS_ZSTD;
                break;
            case Compressor::COMPRESS_LZFSE:
                compressor.bound = lzfse::bound;
                compressor.compress = lzfse::compress;
                compressor.decompress = lzfse::decompress;
                compressor.name = "lzfse";
                compressor.method = Compressor::COMPRESS_LZFSE;
                break;
            case Compressor::COMPRESS_LZMA:
                compressor.bound = lzma::bound;
                compressor.compress = lzma::compress;
                compressor.decompress = lzma::decompress;
                compressor.name = "lzma";
                compressor.method = Compressor::COMPRESS_LZMA;
                break;
            case Compressor::COMPRESS_LZMA2:
                compressor.bound = lzma2::bound;
                compressor.compress = lzma2::compress;
                compressor.decompress = lzma2::decompress;
                compressor.name = "lzma2";
                compressor.method = Compressor::COMPRESS_LZMA2;
                break;
            case Compressor::COMPRESS_PPMD8:
                compressor.bound = ppmd8::bound;
                compressor.compress = ppmd8::compress;
                compressor.decompress = ppmd8::decompress;
                compressor.name = "ppmd8";
                compressor.method = Compressor::COMPRESS_PPMD8;
                break;
            default:
                MANGO_EXCEPTION("Incorrect compression method: %d", method);
        }

        return compressor;
    }

    constexpr u64 mgx_header_size = 24;

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
            u32 size;
        };

        u64 size;
        bool is_compressed;
        std::vector<Segment> segments;

        bool isCompressed() const
        {
            return is_compressed;
        }

        bool isDirectMapped() const
        {
            return !isCompressed() && !isMultiSegmnet();
        }

        bool isMultiSegmnet() const
        {
            return segments.size() > 1;
        }

        bool isFolder() const
        {
            return segments.empty();
        }
    };

    struct Folder
    {
        std::map<std::string, FileHeader> files;
    };

    struct HeaderMGX
    {
        Memory m_memory;
        std::map<std::string, Folder> m_folders;
        std::vector<Block> m_blocks;

        HeaderMGX(Memory memory)
            : m_memory(memory)
        {
            if (!memory.address)
            {
                MANGO_EXCEPTION(ID"Parent container doesn't have memory");
            }

            LittleEndianPointer p = memory.address;
            u32 magic0 = p.read32();
            if (magic0 != makeFourCC('m', 'g', 'x', '0'))
            {
                MANGO_EXCEPTION(ID"Incorrect file identifier (%x)", magic0);
            }

            u64 header_offset = memory.size - mgx_header_size;
            p = memory.address + header_offset;

            u32 magic3 = p.read32();
            if (magic3 != makeFourCC('m', 'g', 'x', '3'))
            {
                MANGO_EXCEPTION(ID"Incorrect header identifier (%x)", magic3);
            }

            u32 version = p.read32();
            u64 block_offset = p.read64();
            u64 file_offset = p.read64();

            read_blocks(memory.address + block_offset);
            read_files(memory.address + file_offset);

            MANGO_UNREFERENCED_PARAMETER(version);
        }

        void read_blocks(LittleEndianPointer p)
        {
            u32 magic1 = p.read32();
            if (magic1 != makeFourCC('m', 'g', 'x', '1'))
            {
                MANGO_EXCEPTION(ID"Incorrect block identifier (%x)", magic1);
            }

            u32 num_blocks = p.read32();
            for (u32 i = 0; i < num_blocks; ++i)
            {
                Block block;
                block.offset = p.read64();
                block.compressed = p.read64();
                block.uncompressed = p.read64();
                block.method = p.read32();
                m_blocks.push_back(block);
            }

            u32 magic2 = p.read32();
            if (magic2 != makeFourCC('m', 'g', 'x', '2'))
            {
                MANGO_EXCEPTION(ID"Incorrect block terminator (%x)", magic2);
            }
        }

        void read_files(LittleEndianPointer p)
        {
            u32 magic2 = p.read32();
            if (magic2 != makeFourCC('m', 'g', 'x', '2'))
            {
                MANGO_EXCEPTION(ID"Incorrect block identifier (%x)", magic2);
            }

            u32 num_files = p.read32();
            for (u32 i = 0; i < num_files; ++i)
            {
                FileHeader file;

                u32 length = p.read32();
                const u8* ptr = p;
                std::string filename(reinterpret_cast<const char *>(ptr), length);
                p += length;

                file.size = p.read64();
                file.is_compressed = false;

                u32 num_segment = p.read32();
                for (u32 j = 0; j < num_segment; ++j)
                {
                    u32 block_idx = p.read32();
                    u32 offset = p.read32();
                    u32 size = p.read32();
                    file.segments.push_back({block_idx, offset, size});

                    // inspect block
                    Block& block = m_blocks[block_idx];
                    if (block.method > 0)
                    {
                        // if ANY of the blocks in the file segments is compressed
                        // the whole file is considered compressed (= cannot be mapped directly)
                        file.is_compressed = true;
                    }
                }

                std::string folder = file.isFolder() ?
                    getPath(filename.substr(0, length - 1)) :
                    getPath(filename);

                m_folders[folder].files[filename] = file;
            }

            u32 magic3 = p.read32();
            if (magic3 != makeFourCC('m', 'g', 'x', '3'))
            {
                MANGO_EXCEPTION(ID"Incorrect block terminator (%x)", magic3);
            }
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
        HeaderMGX m_header;
        std::string m_password;

    public:
        MapperMGX(Memory parent, const std::string& password)
            : m_header(parent)
            , m_password(password)
        {
        }

        bool isFile(const std::string& filename) const override
        {
            std::string pn = getPath(filename);
            auto iPath = m_header.m_folders.find(pn);
            if (iPath != m_header.m_folders.end())
            {
                const Folder& folder = iPath->second;
                auto iFile = folder.files.find(filename);
                if (iFile != folder.files.end())
                {
                    const FileHeader& file = iFile->second;
                    return !file.isFolder();
                }
            }

            return false;
        }

        void getIndex(FileIndex& index, const std::string& pathname) override
        {
            auto iPath = m_header.m_folders.find(pathname);
            if (iPath != m_header.m_folders.end())
            {
                const Folder& folder = iPath->second;

                for (auto i : folder.files)
                {
                    const FileHeader& file = i.second;
                    std::string filename = i.first;

                    filename = filename.substr(pathname.length());

                    u32 flags = 0;

                    if (file.isFolder())
                    {
                        flags |= FileInfo::DIRECTORY;
                    }

                    if (file.isCompressed())
                    {
                        flags |= FileInfo::COMPRESSED;
                    }

                    index.emplace(filename, file.size, flags);
                }
            }
        }

        VirtualMemory* mmap(const std::string& filename) override
        {
            const FileHeader* ptrFile = nullptr;

            std::string pathname = getPath(filename);

            auto iPath = m_header.m_folders.find(pathname);
            if (iPath != m_header.m_folders.end())
            {
                const Folder& folder = iPath->second;

                auto iFile = folder.files.find(filename);
                if (iFile == folder.files.end())
                {
                    MANGO_EXCEPTION(ID"File \"%s\" not found.", filename.c_str());
                }

                ptrFile = &iFile->second;
            }

            const FileHeader& file = *ptrFile;

            if (file.isDirectMapped())
            {
                // file is encoded as a single, non-compressed block
                const auto& segment = file.segments[0];
                Block& block = m_header.m_blocks[segment.block];

                u8* ptr = m_header.m_memory.address + block.offset + segment.offset;
                u64 size = file.size;

#if 0
                printf("## direct mapped: %s (%" PRIu64 " bytes) block.offset: %" PRIu64 ", segment.offset: %d \n",
                       filename.c_str(), size, block.offset, segment.offset);
#endif
                if (block.offset + segment.offset + size > m_header.m_memory.size)
                {
                    MANGO_EXCEPTION(ID"File \"%s\" has mapped region outside of parent memory.", filename.c_str());
                }

                // return reference to parent's memory
                VirtualMemoryMGX* vm = new VirtualMemoryMGX(ptr, nullptr, size);
                return vm;
            }

            // TODO: remove debug prints
            // TODO: compute segment.size instead of storing it in .mgx container
            // TODO: decompression cache for small-file blocks

            u8* ptr = new u8[file.size];
            std::memset(ptr, 0, file.size);
            u64 bytes = file.size;
            u8* x = ptr;

#if 0
            printf("## segmented: %s\n", filename.c_str());
#endif
            for (auto &segment : file.segments)
            {
                const Block& block = m_header.m_blocks[segment.block];
                u32 offset = segment.offset;
                u32 size = segment.size;
                bytes -= size;

#if 0
                printf("  ## size: %d, block.offset: %d, segment.offset: %d, segment.size: %d, method: %d, c: %d, u: %d\n",
                    (u32)size, (u32)block.offset, offset, size, block.method, 
                    (u32)block.compressed, (u32)block.uncompressed);
#endif
                if (block.method)
                {
                    std::vector<u8> temp(block.uncompressed);
                    Memory d(temp.data(), block.uncompressed);
                    Memory s(m_header.m_memory.address + block.offset, block.compressed);

                    Compressor compressor = getCompressor(Compressor::Method(block.method));
                    compressor.decompress(d, s);

                    std::memcpy(x, d.address + offset, size);
                    x += size;
                }
                else
                {
                    std::memcpy(x, m_header.m_memory.address + block.offset + offset, size);
                    printf("%x %x \n", x[0], x[1]);
                    x += size;
                }
            }

            printf("  ## bytes left: %d\n", (u32)bytes);

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
