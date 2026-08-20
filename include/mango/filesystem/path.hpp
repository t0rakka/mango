/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <mango/core/configure.hpp>
#include <mango/filesystem/mapper.hpp>

namespace mango::filesystem
{

    // Logical path handle. Copy and move are shallow: they share the same open
    // container mapping and directory index cache (internal Mapper).
    class Path
    {
    protected:
        std::shared_ptr<Mapper> m_mapper;

    public:
        Path(const std::string& pathname, const std::string& password = "");
        Path(const Path& path, const std::string& filename, const std::string& password = "");
        Path(ConstMemory memory, const std::string& extension, const std::string& password = "");

        Path(const Path&) = default;
        Path& operator=(const Path&) = default;
        Path(Path&&) noexcept = default;
        Path& operator=(Path&&) noexcept = default;

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
            return m_mapper->index();
        }

        operator const FileIndex&() const
        {
            return getIndex();
        }

        const std::string& pathname() const
        {
            return m_mapper->pathname();
        }

        auto begin() const
        {
            return m_mapper->index().begin();
        }

        auto end() const
        {
            return m_mapper->index().end();
        }
 
        auto size() const
        {
            return m_mapper->index().size();
        }

        bool empty() const
        {
            return m_mapper->index().empty();
        }

        const FileInfo& operator [] (size_t index) const
        {
            return m_mapper->index()[index];
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
