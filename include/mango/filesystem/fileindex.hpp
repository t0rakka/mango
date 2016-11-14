/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include "../core/configure.hpp"

namespace mango
{

    struct FileInfo
    {
        enum Flags
        {
            DIRECTORY  = 0x01,
            CONTAINER  = 0x02,
            COMPRESSED = 0x04,
        };

        uint64 size;
        uint32 flags;
        std::string name;

        FileInfo();
        FileInfo(const std::string name, uint64 size, uint32 flags = 0);
        ~FileInfo();

        bool isDirectory() const;
        bool isContainer() const;
        bool isCompressed() const;
    };

    class FileIndex
    {
    protected:
        std::vector<FileInfo> m_files;

    public:
        void emplace(const std::string& name, uint64 size, uint32 flags);

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

} // namespace mango

