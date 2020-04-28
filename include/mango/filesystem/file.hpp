/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include "../core/configure.hpp"
#include "../core/stream.hpp"
#include "mapper.hpp"
#include "path.hpp"

namespace mango {
namespace filesystem {

    class File : protected NonCopyable
    {
    protected:
        std::string m_filename;
        std::unique_ptr<Path> m_path;
        std::unique_ptr<VirtualMemory> m_memory;

        ConstMemory getMemory() const;

    public:
        File(const std::string& filename);
        File(const Path& path, const std::string& filename);
        File(ConstMemory memory, const std::string& extension, const std::string& filename);
        ~File();

        const Path& path() const;
        const std::string& filename() const;
        const std::string& pathname() const;

        // memory
        operator ConstMemory () const;
        operator const u8* () const;
        const u8* data() const;
        size_t size() const;
    };

    class FileStream : public Stream
    {
    protected:
		struct FileHandle* m_handle;

    public:
        FileStream(const std::string& filename, OpenMode mode);
        ~FileStream();

        const std::string& filename() const;

        u64 size() const;
        u64 offset() const;
        void seek(u64 distance, SeekMode mode);
        void read(void* dest, size_t size);
        void write(const void* data, size_t size);

        void write(ConstMemory memory)
        {
            Stream::write(memory);
        }
    };

} // namespace filesystem
} // namespace mango
