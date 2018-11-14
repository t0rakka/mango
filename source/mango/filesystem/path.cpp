/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <mango/filesystem/path.hpp>

namespace mango
{

    // -----------------------------------------------------------------
    // Path
    // -----------------------------------------------------------------

    Path::Path(const std::string& pathname, const std::string& password)
    {
		// create mapper to raw filesystem
        m_mapper = getFileMapper();

		// parse and create mappers
        m_pathname = parse(pathname, password);
        m_mapper->getIndex(m_files, m_pathname);
    }

    Path::Path(const Memory& memory, const std::string& extension, const std::string& password)
    {
        m_pathname = "";

        // create mapper to raw memory
        m_mapper = getMemoryMapper(memory, extension, password);
        if (m_mapper)
        {
            m_mapper->getIndex(m_files, m_pathname);
        }
    }

    Path::Path(const Path& path, const std::string& pathname, const std::string& password)
    {
        // use parent's mapper
        m_mapper = path.m_mapper;

		// parse and create mappers
        m_pathname = parse(path.m_pathname + pathname, password);
        m_mapper->getIndex(m_files, m_pathname);
    }

    Path::~Path()
    {
    }

    void Path::updateIndex()
    {
        m_files.clear();
        m_mapper->getIndex(m_files, m_pathname);
    }

    const std::string& Path::pathname() const
    {
        return m_pathname;
    }

} // namespace mango
