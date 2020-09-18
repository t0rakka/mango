/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include <mango/core/configure.hpp>
#include <mango/filesystem/mapper.hpp>

namespace mango {
namespace filesystem {

    class Path : protected NonCopyable
    {
    protected:
        friend class File;

        std::shared_ptr<Mapper> m_mapper;
        FileIndex m_files;

    public:
        Path(const std::string& pathname, const std::string& password = "");
        Path(const Path& path, const std::string& filename, const std::string& password = "");
        Path(ConstMemory memory, const std::string& extension, const std::string& password = "");
        ~Path();

        const std::string& pathname() const
        {
            return m_mapper->pathname();
        }

        auto begin() const -> decltype(m_files.begin())
        {
            return m_files.begin();
        }

        auto end() const -> decltype(m_files.end())
        {
            return m_files.end();
        }
 
        auto size() const -> decltype(m_files.size())
        {
            return m_files.size();
        }

        bool empty() const
        {
            return m_files.empty();
        }

        const FileInfo& operator [] (int index) const
        {
            return m_files[index];
        }
    };

    // filename manipulation functions (example: "foo/bar/readme.txt")
    std::string getPath(const std::string& filename);           // "foo/bar/"
    std::string removePath(const std::string& filename);        // "readme.txt"
    std::string getExtension(const std::string& filename);      // ".txt"
    std::string removeExtension(const std::string& filename);   // "foo/bar/readme"

} // namespace filesystem
} // namespace mango
