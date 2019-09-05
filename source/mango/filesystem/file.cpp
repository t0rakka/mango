/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/string.hpp>
#include <mango/core/exception.hpp>
#include <mango/filesystem/file.hpp>

namespace mango {
namespace filesystem {

    // -----------------------------------------------------------------
    // File
    // -----------------------------------------------------------------

    File::File(const std::string& s)
    {
        // split s into pathname + filename
        size_t n = s.find_last_of("/\\:");
        std::string filename = s.substr(n + 1);
        std::string filepath = s.substr(0, n + 1);

        m_filename = filename;

        // create a internal path
        m_path.reset(new Path(filepath));

        Mapper* path_mapper = m_path->m_mapper.get();
        if (!path_mapper)
        {
            MANGO_EXCEPTION("[File] Mapper interface missing.");
        }

        AbstractMapper* mapper = *path_mapper;
        if (mapper)
        {
            VirtualMemory* vmemory = mapper->mmap(path_mapper->basepath() + m_filename);
            m_memory = UniqueObject<VirtualMemory>(vmemory);
        }
    }

    File::File(const Path& path, const std::string& s)
    {
        // split s into pathname + filename
        size_t n = s.find_last_of("/\\:");
        std::string filename = s.substr(n + 1);
        std::string filepath = s.substr(0, n + 1);

        m_filename = filename;

        // create a internal path
        m_path.reset(new Path(path, filepath));

        Mapper* path_mapper = m_path->m_mapper.get();
        if (!path_mapper)
        {
            MANGO_EXCEPTION("[File] Mapper interface missing.");
        }

        AbstractMapper* mapper = *path_mapper;
        if (mapper)
        {
            VirtualMemory* vmemory = mapper->mmap(path_mapper->basepath() + m_filename);
            m_memory = UniqueObject<VirtualMemory>(vmemory);
        }
    }

    File::File(const Memory& memory, const std::string& extension, const std::string& filename)
    {
        std::string password;

        // create a internal path
        m_path.reset(new Path(memory, extension, password));

        Mapper* path_mapper = m_path->m_mapper.get();
        if (!path_mapper)
        {
            MANGO_EXCEPTION("[File] Mapper interface missing.");
        }

        // parse and create mappers
        std::string temp_filename = filename;
        m_filename = path_mapper->parse(temp_filename, "");

        // memory map the file
        AbstractMapper* mapper = *path_mapper;
        if (mapper)
        {
            VirtualMemory* vmemory = mapper->mmap(m_filename);
            m_memory = UniqueObject<VirtualMemory>(vmemory);
        }
    }

    File::~File()
    {
    }

    const std::string& File::filename() const
    {
        return m_filename;
    }

    const std::string& File::pathname() const
    {
        return m_path->m_mapper->pathname();
    }

    File::operator Memory () const
    {
        return getMemory();
    }

	File::operator const u8* () const
	{
        return getMemory().address;
	}

    const u8* File::data() const
    {
        return getMemory().address;
    }

    size_t File::size() const
    {
        return getMemory().size;
    }

    Memory File::getMemory() const
    {
        return m_memory ? *m_memory : Memory();
    }

} // namespace filesystem
} // namespace mango
