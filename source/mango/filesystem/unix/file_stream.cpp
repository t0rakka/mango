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
            ::fstat(m_file, &sb);
            return sb.st_size;
        }

        u64 offset() const
        {
            return u64(::lseek(m_file, 0, SEEK_CUR));
        }

        void seek(s64 distance, int method)
        {
            ::lseek(m_file, distance, method);
        }

        void read(void* dest, u64 size)
        {
            ssize_t status = ::read(m_file, dest, size_t(size));
            MANGO_UNREFERENCED(status);
        }

        u64 write(const void* data, u64 size)
        {
            ssize_t status = ::write(m_file, data, size_t(size));
            u64 bytes_written = status < 0 ? 0 : u64(status);
            return bytes_written;
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

    void FileStream::seek(s64 distance, SeekMode mode)
    {
        int method;

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

            default:
                MANGO_EXCEPTION("[FileStream] Invalid seek mode.");
        }

        m_handle->seek(distance, method);
    }

    void FileStream::read(void* dest, u64 size)
    {
        m_handle->read(dest, size);
    }

    u64 FileStream::write(const void* data, u64 size)
    {
        return m_handle->write(data, size);
    }

} // namespace mango::filesystem
