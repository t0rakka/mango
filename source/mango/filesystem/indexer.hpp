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
        struct File
        {
            Folder* folder;
            int index;
        };

        std::map<std::string, Folder> folders;
        std::map<std::string, File> files;

    public:
        void insert(const std::string& foldername, const std::string& filename, const Header& header)
        {
            if (getHeader(filename))
                return;

            Folder& folder = folders[foldername];

            File file;
            file.folder = &folder;
            file.index = int(folder.headers.size());
            folder.headers.push_back(header);

            files[filename] = file;
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

            auto i = files.find(filename);
            if (i != files.end())
            {
                const File& file = i->second;
                result = &file.folder->headers[file.index];
            }

            return result;
        }
    };

} // namespace filesystem
} // namespace mango
