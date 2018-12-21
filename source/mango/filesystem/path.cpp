/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <mango/filesystem/path.hpp>

namespace mango {
namespace filesystem {

    // -----------------------------------------------------------------
    // Path
    // -----------------------------------------------------------------

    Path::Path(const std::string& pathname, const std::string& password)
    {
		// parse and create mappers
        std::string temp = pathname.empty() ? "./" : pathname;
        m_pathname = temp;
        m_basepath = parse(temp, password);
        m_basepath = temp;

#if 0
        printf("# m_basepath: %s\n", m_basepath.c_str());
        printf("# m_pathname: %s\n", m_pathname.c_str());
        printf("\n");
#endif

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

#if 0
        printf("# m_basepath: %s\n", m_basepath.c_str());
        printf("# m_pathname: %s\n", m_pathname.c_str());
        printf("\n");
#endif

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

    // -----------------------------------------------------------------
    // filename manipulation functions
    // -----------------------------------------------------------------

    std::string getPath(const std::string& filename)
    {
        size_t n = filename.find_last_of("/\\:");
        std::string s;
        if (n != std::string::npos)
            s = filename.substr(0, n + 1);
        return s;
    }

    std::string removePath(const std::string& filename)
    {
        size_t n = filename.find_last_of("/\\:");
        std::string s;
        if (n != std::string::npos)
            s = filename.substr(n + 1);
        else
            s = filename;
        return s;
    }

    std::string getExtension(const std::string& filename)
    {
        size_t n = filename.find_last_of('.');
        std::string s;
        if (n != std::string::npos)
            s = filename.substr(n);
        return s;
    }

    std::string removeExtension(const std::string& filename)
    {
        size_t n = filename.find_last_of('.');
        return filename.substr(0, n);
    }

} // namespace filesystem
} // namespace mango
