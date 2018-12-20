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

    template <typename T>
    class Indexer
    {
    public:
        struct Folder
        {
            std::map<std::string, T> files;
        };

    protected:
        std::map<std::string, Folder> folders;

    public:
        void insert(const std::string& folder, const std::string& filename, const T& file)
        {
            folders[folder].files[filename] = file;
        }

        const T* file(const std::string& filename) const
        {
            const T* result = nullptr;

            std::string pathname = getPath(filename);
            auto iPath = folders.find(pathname);
            if (iPath != folders.end())
            {
                const Folder& folder = iPath->second;
                auto iFile = folder.files.find(filename);
                if (iFile != folder.files.end())
                {
                    result = &iFile->second;
                }
            }

            return result;
        }

        const Folder* folder(const std::string& pathname) const
        {
            const Folder* result = nullptr;

            auto iPath = folders.find(pathname);
            if (iPath != folders.end())
            {
                result = &iPath->second;
            }

            return result;
        }
    };

} // namespace filesystem
} // namespace mango
