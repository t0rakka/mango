/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <set>
#include <map>

namespace mango {
namespace filesystem {

    template <typename Header>
    class Indexer
    {
    public:
        struct Folder
        {
            std::set<Header *> headers;
        };

    protected:
        std::map<std::string, Folder> folders;
        std::map<std::string, Header> headers;

    public:
        void insert(const std::string& foldername, const std::string& filename, const Header& header)
        {
            Header* ptr = &headers[filename];
            *ptr = header;
            folders[foldername].headers.emplace(ptr);
        }

        const Folder* getFolder(const std::string& pathname) const
        {
            const Folder* result = nullptr; // default: not found

            auto i = folders.find(pathname);
            if (i != folders.end())
            {
                result = &i->second;
            }

            return result;
        }

        const Header* getHeader(const std::string& filename) const
        {
            const Header* result = nullptr; // default: not found

            auto i = headers.find(filename);
            if (i != headers.end())
            {
                result = &i->second;
            }

            return result;
        }
    };

} // namespace filesystem
} // namespace mango
