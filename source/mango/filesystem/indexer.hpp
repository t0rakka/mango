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
            std::vector<Header> headers;
        };

    protected:
        struct HeaderIndex
        {
            Folder* folder;
            int index;
        };

        std::map<std::string, Folder> folders;
        std::map<std::string, HeaderIndex> indices;

    public:
        void insert(const std::string& folder, const std::string& filename, const Header& header)
        {
            if (getHeader(filename))
                return;

            Folder& f = folders[folder];

            HeaderIndex h;
            h.folder = &f;
            h.index = int(f.headers.size());

            indices[filename] = h;
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

            auto i = indices.find(filename);
            if (i != indices.end())
            {
                const HeaderIndex& h = i->second;
                result = &h.folder->headers[h.index];
            }

            return result;
        }
    };

} // namespace filesystem
} // namespace mango
