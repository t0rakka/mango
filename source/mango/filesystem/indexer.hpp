/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <map>
#include <mango/core/string.hpp>

namespace mango {
namespace filesystem {

    template <typename Header>
    class Indexer
    {
    public:
        struct Folder
        {
            std::map<std::string, int> index;
            std::vector<Header> headers;
        };

    protected:
        std::map<std::string, Folder> folders;

    public:
        void insert(const std::string& folder, const std::string& filename, const Header& header)
        {
            Folder& f = folders[folder];
            f.index[filename] = int(f.headers.size());
            f.headers.push_back(header);
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

            std::string pathname = getPath(filename);
            const Folder* folder = getFolder(pathname);
            if (folder)
            {
                auto i = folder->index.find(filename);
                if (i != folder->index.end())
                {
                    result = &folder->headers[i->second];
                }
            }

            return result;
        }
    };

} // namespace filesystem
} // namespace mango
