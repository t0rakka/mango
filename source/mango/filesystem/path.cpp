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
		// parse and create mappers
        std::string temp = pathname;
        m_basepath = parse(temp, password);
        m_pathname = pathname;
        m_basepath = temp;
        if (m_mapper)
        {
            m_mapper->getIndex(m_files, m_basepath);
        }
    }

    Path::Path(const Path& path, const std::string& pathname, const std::string& password)
    {
        // use parent's mapper
        m_mapper = path.m_mapper;

		// parse and create mappers
        std::string temp = path.m_basepath + pathname;
        m_basepath = parse(temp, password);
        m_pathname = path.m_pathname + pathname;
        if (m_mapper)
        {
            m_mapper->getIndex(m_files, m_basepath);
        }
    }

    Path::Path(const Memory& memory, const std::string& extension, const std::string& password)
    {
        // create mapper to raw memory
        m_mapper = createMemoryMapper(memory, extension, password);
        if (m_mapper)
        {
            m_mapper->getIndex(m_files, m_basepath);
        }
    }

    Path::~Path()
    {
    }

} // namespace mango
