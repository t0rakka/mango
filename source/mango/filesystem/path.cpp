/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <mango/filesystem/path.hpp>

namespace mango::filesystem
{

    // -----------------------------------------------------------------
    // Path
    // -----------------------------------------------------------------

    Path::Path(const std::string& pathname, const std::string& password)
        : m_mapper(std::make_shared<Mapper>(pathname, password))
    {
    }

    Path::Path(const Path& path, const std::string& pathname, const std::string& password)
        : m_mapper(std::make_shared<Mapper>(path.m_mapper, pathname, password))
    {
    }

    Path::Path(ConstMemory memory, const std::string& extension, const std::string& password)
        : m_mapper(std::make_shared<Mapper>(memory, extension, password))
    {
    }

    Path::~Path()
    {
    }

    // -----------------------------------------------------------------
    // filename manipulation functions
    // -----------------------------------------------------------------

    size_t getPathSeparatorIndex(std::string_view filename)
    {
#if defined(MANGO_PLATFORM_WINDOWS)
        // Split on directory separators. Do NOT treat ':' as a generic separator:
        // archive/logical paths may contain ':' in filenames, and "C:\foo" already
        // splits correctly on '\' or '/'.
        size_t n = filename.find_last_of("/\\");
        if (n != std::string::npos)
        {
            return n;
        }

        // DOS drive-relative path: "C:foo.txt" (no slash; current directory on drive C)
        if (filename.length() >= 2)
        {
            char drive = filename[0];
            if (filename[1] == ':' &&
                ((drive >= 'A' && drive <= 'Z') || (drive >= 'a' && drive <= 'z')))
            {
                return 1;
            }
        }

        return std::string::npos;
#else
        // NOTE: only '/' separates path components; ':' and '\\' are valid filename characters
        return filename.find_last_of('/');
#endif
    }

    std::string_view getPath(std::string_view filename)
    {
        const size_t n = getPathSeparatorIndex(filename);
        if (n != std::string_view::npos)
        {
            return filename.substr(0, n + 1);
        }
        return {};
    }

    std::string_view removePath(std::string_view filename)
    {
        const size_t n = getPathSeparatorIndex(filename);
        if (n != std::string_view::npos)
        {
            return filename.substr(n + 1);
        }
        return filename;
    }

    std::string_view getExtension(std::string_view filename)
    {
        const size_t n = filename.find_last_of('.');
        if (n != std::string_view::npos)
        {
            return filename.substr(n);
        }
        return {};
    }

    std::string_view removeExtension(std::string_view filename)
    {
        const size_t n = filename.find_last_of('.');
        return filename.substr(0, n);
    }

    std::string getPath(const std::string& filename)
    {
        std::string_view view = std::string_view(filename);
        return std::string(getPath(view));
    }

    std::string removePath(const std::string& filename)
    {
        std::string_view view = std::string_view(filename);
        return std::string(removePath(view));
    }

    std::string getExtension(const std::string& filename)
    {
        std::string_view view = std::string_view(filename);
        return std::string(getExtension(view));
    }

    std::string removeExtension(const std::string& filename)
    {
        std::string_view view = std::string_view(filename);
        return std::string(removeExtension(view));
    }

} // namespace mango::filesystem
