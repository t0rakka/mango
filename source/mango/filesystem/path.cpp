/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <mango/filesystem/path.hpp>

namespace mango
{

    // -----------------------------------------------------------------
    // FileInfo
    // -----------------------------------------------------------------

    FileInfo::FileInfo()
    : size(0), flags(0)
    {
    }

    FileInfo::FileInfo(const std::string& name, uint64 size, uint32 flags)
    : size(size), flags(flags), name(name)
    {
    }

    FileInfo::~FileInfo()
    {
    }

    bool FileInfo::isDirectory() const
    {
        return (flags & DIRECTORY) != 0;
    }

    bool FileInfo::isContainer() const
    {
        return (flags & CONTAINER) != 0;
    }

    bool FileInfo::isCompressed() const
    {
        return (flags & COMPRESSED) != 0;
    }

    void emplace(FileIndex &index, const std::string &name, uint64 size, uint32 flags)
    {
        index.emplace_back(name, size, flags);

        const bool isFile = (flags & FileInfo::DIRECTORY) == 0;
        if (isFile && Mapper::isCustomMapper(name))
        {
            // file is a container; add it into the index again as one
            index.emplace_back(name + "/", 0, FileInfo::DIRECTORY | FileInfo::CONTAINER);
        }
    }

    // -----------------------------------------------------------------
    // Path
    // -----------------------------------------------------------------

    Path::Path(const std::string& pathname, const std::string& password)
    {
		// create mapper to raw filesystem
        m_mapper = getFileMapper();

		// parse and create mappers
        m_pathname = parse(pathname, password);
        m_mapper->index(m_files, m_pathname);
    }

    Path::Path(const Path& path, const std::string& pathname, const std::string& password)
    {
        // use parent's mapper
        m_mapper = path.m_mapper;

		// parse and create mappers
        m_pathname = parse(path.m_pathname + pathname, password);
        m_mapper->index(m_files, m_pathname);
    }

    Path::~Path()
    {
    }

    void Path::updateIndex()
    {
        m_files.clear();
        m_mapper->index(m_files, m_pathname);
    }

    const std::string& Path::pathname() const
    {
        return m_pathname;
    }

} // namespace mango
