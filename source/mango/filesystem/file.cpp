/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/string.hpp>
#include <mango/core/exception.hpp>
#include <mango/filesystem/file.hpp>

namespace mango::filesystem
{

    // -----------------------------------------------------------------
    // File
    // -----------------------------------------------------------------

    File::File(const std::string& s)
    {
        // split s into pathname + filename
        size_t n = getPathSeparatorIndex(s);
        std::string filename = s.substr(n + 1);
        std::string filepath = s.substr(0, n + 1);

        m_filename = filename;

        // create a internal path
        m_path = std::make_unique<Path>(filepath);
        initMemory(*m_path);
    }

    File::File(const Path& path, const std::string& s)
    {
        // split s into pathname + filename
        size_t n = getPathSeparatorIndex(s);
        std::string filename = s.substr(n + 1);
        std::string filepath = s.substr(0, n + 1);

        m_filename = filename;

        // create a internal path
        m_path = std::make_unique<Path>(path, filepath);
        initMemory(*m_path);
    }

    File::File(ConstMemory memory, const std::string& extension, const std::string& s)
    {
        // use memory mapped path as parent
        // NOTE: the path goes out of scope but the mapper object is shared_ptr so it remains alive :)
        Path path(memory, extension);

        // split s into pathname + filename
        size_t n = getPathSeparatorIndex(s);
        std::string filename = s.substr(n + 1);
        std::string filepath = s.substr(0, n + 1);

        m_filename = filename;

        // create a internal path
        m_path = std::make_unique<Path>(path, filepath);
        initMemory(*m_path);
    }

    File::~File()
    {
    }

    void File::initMemory(Mapper& mapper)
    {
        m_virtual_memory = mapper.map(m_filename);
        if (m_virtual_memory)
        {
            m_memory = *m_virtual_memory;
        }
    }

    const Path& File::path() const
    {
        return *m_path;
    }

    const std::string& File::filename() const
    {
        return m_filename;
    }

    const std::string& File::pathname() const
    {
        return m_path->pathname();
    }

    File::operator ConstMemory () const
    {
        return m_memory;
    }

    File::operator const u8* () const
    {
        return m_memory.address;
    }

    const u8* File::data() const
    {
        return m_memory.address;
    }

    u64 File::size() const
    {
        return m_memory.size;
    }

    u64 getFileSize(const std::string& filename)
    {
        // Calls stat() to get the file size for raw files
        // Gets the file size from container if not a raw file
        Path path("./");
        return path.getSize(filename);
    }

} // namespace mango::filesystem
