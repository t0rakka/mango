/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/string.hpp>
#include <mango/core/print.hpp>
#include <mango/core/exception.hpp>
#include <mango/filesystem/file.hpp>

namespace mango::filesystem
{

    // -----------------------------------------------------------------
    // FileHandle
    // -----------------------------------------------------------------

    struct FileHandle
    {
        std::string m_filename;
        HANDLE m_handle;

        FileHandle(const std::string& filename, HANDLE handle)
            : m_filename(filename)
            , m_handle(handle)
        {
        }

        ~FileHandle()
        {
            CloseHandle(m_handle);
        }

        const std::string& filename() const
        {
            return m_filename;
        }

        s64 size() const
        {
            LARGE_INTEGER result = {};
            BOOL status = GetFileSizeEx(m_handle, &result);
            if (!status)
                return -1ll;
            return s64(result.QuadPart);
        }

        s64 offset() const
        {
            LARGE_INTEGER dist = {};
            LARGE_INTEGER result = {};
            BOOL status = SetFilePointerEx(m_handle, dist, &result, FILE_CURRENT);
            if (!status)
                return -1ll;
            return s64(result.QuadPart);
        }

        s64 seek(s64 distance, DWORD method)
        {
            LARGE_INTEGER dist = {};
            dist.QuadPart = distance;
            LARGE_INTEGER result = {};
            BOOL status = SetFilePointerEx(m_handle, dist, &result, method);
            if (!status)
                return -1ll;
            return s64(result.QuadPart);
        }

        s64 read(void* dest, u32 size)
        {
            DWORD bytes_read = 0;
            BOOL status = ReadFile(m_handle, dest, DWORD(size), &bytes_read, NULL);
            if (!status)
                return -1ll;
            return s64(bytes_read);
        }

        s64 write(const void* data, u32 size)
        {
            DWORD bytes_written = 0;
            BOOL status = WriteFile(m_handle, data, DWORD(size), &bytes_written, NULL);
            if (!status)
                return -1ll;
            return s64(bytes_written);
        }
    };

    // -----------------------------------------------------------------
    // FileStream
    // -----------------------------------------------------------------

    FileStream::FileStream(const std::string& filename, OpenMode mode)
        : m_handle(nullptr)
    {
        DWORD access;
        DWORD disposition;

        switch (mode)
        {
            case OpenMode::Read:
                access = GENERIC_READ;
                disposition = OPEN_EXISTING;
                break;

            case OpenMode::Write:
                access = GENERIC_WRITE;
                disposition = CREATE_ALWAYS;
                break;

            default:
                MANGO_EXCEPTION("[FileStream] Incorrect OpenMode.");
                break;
        }

        HANDLE handle = CreateFileW(u16_fromBytes(filename).c_str(), access, FILE_SHARE_READ, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
        if (handle == INVALID_HANDLE_VALUE)
        {
            MANGO_EXCEPTION("[FileStream] CreateFileW(\"{}\") failed.", filename);
        }

        m_handle = new FileHandle(filename, handle);
    }

    FileStream::~FileStream()
    {
        delete m_handle;
    }

    const std::string& FileStream::filename() const
    {
        return m_handle->filename();
    }

    s64 FileStream::size() const
    {
        return m_handle->size();
    }

    s64 FileStream::offset() const
    {
        return m_handle->offset();
    }

    s64 FileStream::seek(s64 distance, SeekMode mode)
    {
        DWORD method = 0;

        switch (mode)
        {
            case SeekMode::Begin:
                method = FILE_BEGIN;
                break;

            case SeekMode::Current:
                method = FILE_CURRENT;
                break;

            case SeekMode::End:
                method = FILE_END;
                break;
        }

        return m_handle->seek(distance, method);
    }

    // NOTE: WIN32 ReadFile and WriteFile are limited to 4 GB maximum read and write
    //       so we split larger operations into smaller chunks.

    s64 FileStream::read(void* dest, u64 bytes)
    {
        s64 total = 0;

        s64 bytes_left = bytes;
        u8* output = reinterpret_cast<u8*>(dest);

        while (bytes_left > 0)
        {
            u64 commit = std::min(u64(bytes_left), 0xffffffffull);
            s64 result = read(output, u32(commit));
            if (result < 0)
            {
                return result;
            }

            bytes_left -= result;
            output += result;
            total += result;
        }

        return total;
    }

    s64 FileStream::write(const void* data, u64 bytes)
    {
        s64 total = 0;

        s64 bytes_left = bytes;
        const u8* input = reinterpret_cast<const u8*>(data);

        while (bytes_left > 0)
        {
            u64 commit = std::min(u64(bytes_left), 0xffffffffull);
            s64 result = write(input, u32(commit));
            if (result < 0)
            {
                return result;
            }

            bytes_left -= result;
            input += result;
            total += result;
        }

        return total;
    }

} // namespace mango::filesystem
