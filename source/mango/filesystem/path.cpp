/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <mango/filesystem/path.hpp>

namespace mango {
namespace filesystem {

    // -----------------------------------------------------------------
    // Path
    // -----------------------------------------------------------------

    Path::Path(const std::string& pathname, const std::string& password)
        : m_mapper(std::make_shared<Mapper>(pathname, password))
    {
        AbstractMapper* mapper = *m_mapper;
        if (mapper)
        {
            mapper->getIndex(m_files, m_mapper->basepath());
        }
    }

    Path::Path(const Path& path, const std::string& pathname, const std::string& password)
        : m_mapper(std::make_shared<Mapper>(path.m_mapper, pathname, password))
    {
        AbstractMapper* mapper = *m_mapper;
        if (mapper)
        {
            mapper->getIndex(m_files, m_mapper->basepath());
        }
    }

    Path::Path(const Memory& memory, const std::string& extension, const std::string& password)
        : m_mapper(std::make_shared<Mapper>(memory, extension, password))
    {
        AbstractMapper* mapper = *m_mapper;
        if (mapper)
        {
            mapper->getIndex(m_files, m_mapper->basepath());
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
