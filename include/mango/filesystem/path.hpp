/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <mango/core/configure.hpp>
#include <mango/filesystem/mapper.hpp>

namespace mango::filesystem
{

    class Path : protected NonCopyable
    {
    protected:
        std::shared_ptr<Mapper> m_mapper;

        mutable FileIndex m_index;
        mutable bool m_index_is_dirty = true;

        void updateIndex() const;

    public:
        Path(const std::string& pathname, const std::string& password = "");
        Path(const Path& path, const std::string& filename, const std::string& password = "");
        Path(ConstMemory memory, const std::string& extension, const std::string& password = "");
        ~Path();

        operator Mapper& () const
        {
            return *m_mapper;
        }

        u64 getSize(const std::string& filename) const
        {
            return m_mapper->getSize(filename);
        }

        bool isFile(const std::string& filename) const
        {
            return m_mapper->isFile(filename);
        }

        const FileIndex& getIndex() const
        {
            updateIndex();
            return m_index;
        }

        operator const FileIndex&() const
        {
            return getIndex();
        }

        const std::string& pathname() const
        {
            return m_mapper->pathname();
        }

        auto begin() const -> decltype(m_index.begin())
        {
            updateIndex();
            return m_index.begin();
        }

        auto end() const -> decltype(m_index.end())
        {
            updateIndex();
            return m_index.end();
        }
 
        auto size() const -> decltype(m_index.size())
        {
            updateIndex();
            return m_index.size();
        }

        bool empty() const
        {
            updateIndex();
            return m_index.empty();
        }

        const FileInfo& operator [] (size_t index) const
        {
            updateIndex();
            return m_index[index];
        }
    };

    // -----------------------------------------------------------------------
    // filename manipulation functions
    // -----------------------------------------------------------------------

    size_t getPathSeparatorIndex(std::string_view filename);

    // example: "foo/bar/readme.txt"
    std::string_view getPath(std::string_view filename);           // "foo/bar/"
    std::string_view removePath(std::string_view filename);        // "readme.txt"
    std::string_view getExtension(std::string_view filename);      // ".txt"
    std::string_view removeExtension(std::string_view filename);   // "foo/bar/readme"

    std::string getPath(const std::string& filename);
    std::string removePath(const std::string& filename);
    std::string getExtension(const std::string& filename);
    std::string removeExtension(const std::string& filename);

} // namespace mango::filesystem
