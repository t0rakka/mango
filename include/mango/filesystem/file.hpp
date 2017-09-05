/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include "../core/configure.hpp"
#include "../core/stream.hpp"
#include "mapper.hpp"
#include "path.hpp"

namespace mango
{

    class File : public Mapper
    {
    protected:
        uint64 m_offset;
        std::string m_filename;
        std::unique_ptr<VirtualMemory> m_memory;

    public:
        File(const Path& path, const std::string& filename);
        File(const std::string& filename);
        ~File();

        const std::string& filename() const;

        // memory
        operator Memory () const;
		operator const uint8* () const;
        const uint8* data() const;
    };

    class FileStream : public Stream
    {
    protected:
		struct FileHandle* m_handle;

    public:
        FileStream(const std::string& filename, OpenMode mode);
        ~FileStream();

        const std::string& filename() const;

        uint64 size() const;
        uint64 offset() const;
        void seek(uint64 distance, SeekMode mode);
        void read(void* dest, size_t size);
        void write(const void* data, size_t size);
    };

} // namespace mango
