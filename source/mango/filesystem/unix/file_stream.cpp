/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#if __ANDROID_API__ < __ANDROID_API_N__
#define _FILE_OFFSET_BITS 64 /* LFS: 64 bit off_t */
#endif

#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <mango/core/string.hpp>
#include <mango/core/exception.hpp>
#include <mango/filesystem/file.hpp>

namespace mango::filesystem
{

    // -----------------------------------------------------------------
    // FileHandle
    // -----------------------------------------------------------------

    struct FileHandle
    {
        int m_file;
        std::string m_filename;

        FileHandle(const std::string& filename, int flags)
            : m_file(::open(filename.c_str(), flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH))
            , m_filename(filename)
        {
        }

        ~FileHandle()
        {
            ::close(m_file);
        }

        const std::string& filename() const
        {
            return m_filename;
        }

        u64 size() const
        {
            struct stat sb;
            int status = ::fstat(m_file, &sb);
            if (status < 0)
                return 0;
            return s64(sb.st_size);
        }

        u64 offset() const
        {
            auto offset = ::lseek(m_file, 0, SEEK_CUR);
            if (offset < 0)
                return 0;
            return u64(offset);
        }

        u64 seek(s64 distance, int method)
        {
            auto offset = ::lseek(m_file, distance, method);
            if (offset < 0)
                return 0;
            return u64(offset);
        }

        u64 read(void* dest, u64 size)
        {
            ssize_t bytes_read = ::read(m_file, dest, size_t(size));
            if (bytes_read < 0)
                return 0;
            return u64(bytes_read);
        }

        u64 write(const void* data, u64 size)
        {
            ssize_t bytes_written = ::write(m_file, data, size_t(size));
            if (bytes_written < 0)
                return 0;
            return u64(bytes_written);
        }
    };

    // -----------------------------------------------------------------
    // FileStream
    // -----------------------------------------------------------------

    FileStream::FileStream(const std::string& filename, OpenMode openmode)
        : m_handle(nullptr)
    {
       	switch (openmode)
        {
            case OpenMode::Read:
                m_handle = new FileHandle(filename, O_RDONLY);
                break;

            case OpenMode::Write:
                m_handle = new FileHandle(filename, O_WRONLY | O_CREAT | O_TRUNC);
                break;

            default:
                MANGO_EXCEPTION("[FileStream] Incorrect OpenMode.");
                break;
        }
    }

    FileStream::~FileStream()
    {
        delete m_handle;
    }

    const std::string& FileStream::filename() const
    {
        return m_handle->filename();
    }

    u64 FileStream::size() const
    {
        return m_handle->size();
    }

    u64 FileStream::offset() const
    {
        return m_handle->offset();
    }

    u64 FileStream::seek(s64 distance, SeekMode mode)
    {
        int method = 0;

        switch (mode)
        {
            case SeekMode::Begin:
                method = SEEK_SET;
                break;

            case SeekMode::Current:
                method = SEEK_CUR;
                break;

            case SeekMode::End:
                method = SEEK_END;
                break;
        }

        return m_handle->seek(distance, method);
    }

    u64 FileStream::read(void* dest, u64 bytes)
    {
        return m_handle->read(dest, bytes);
    }

    u64 FileStream::write(const void* data, u64 bytes)
    {
        return m_handle->write(data, bytes);
    }

} // namespace mango::filesystem
