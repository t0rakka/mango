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

#define ID ".mgx mapper: "

#if 0

namespace
{
    using namespace mango;


    // mgx format specification v0.1

    // | data   |
    // | index  |
    // | header |

    // The data consists of binary blobs ('Block') of data, which contain multiple smaller files
    // to constitute a "solid" archive so that the compression overhead is shared between
    // multiple files. This also improves compression ratio. Large files are broken into
    // many Blocks so that the compression and decompression can be distributed across larger
    // number of processing units.

    // The way this works is that when client memory maps a file in the container, in case of
    // a small file the decompressed Block is represented as VirtualMemory block which is
    // reference counted object in the heap. When the last reference to the Block goes out of
    // scope the object is "collected". The collection means it is placed at tail of LRU cache,
    // which protects against pathological use cases which would cause decompression to
    // to be performed on a Block that was just evicted (in case of no caching of mmap operations).

    // This approach improves compression ratio and allows multi-threaded compression and
    // decompression.

    // The following structs should be ignored, they are just draft notes..

	struct Block
	{
		u64 offset;
		u64 size;
	};

	struct Archive
	{
		u32 signature;
		u32 version;
		Block directory;
	};

	struct Directory
	{
		u32 signature;
	};

	struct FileHeader
	{
		Block compressed;
		u64 uncompressed;
		u32 checksum;
		u32 compression;
		u32 filenameSize;
	};

} // namespace

#endif

namespace mango
{

    // -----------------------------------------------------------------
    // MapperMGX
    // -----------------------------------------------------------------

    class MapperMGX : public AbstractMapper
    {
    public:
        Memory m_parent_memory;
        std::string m_password;

        MapperMGX(Memory parent, const std::string& password)
            : m_parent_memory(parent)
            , m_password(password)
        {
        }

        bool isFile(const std::string& filename) const override
        {
            MANGO_UNREFERENCED_PARAMETER(filename);
            return false;
        }

        void getIndex(FileIndex& index, const std::string& pathname) override
        {
            MANGO_UNREFERENCED_PARAMETER(index);
            MANGO_UNREFERENCED_PARAMETER(pathname);
        }

        VirtualMemory* mmap(const std::string& filename) override
        {
            MANGO_UNREFERENCED_PARAMETER(filename);
            return nullptr;
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
