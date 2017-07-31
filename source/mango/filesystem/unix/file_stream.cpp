/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#if __ANDROID_API__ < __ANDROID_API_N__
#define _FILE_OFFSET_BITS 64 /* LFS: 64 bit off_t */
#endif
#include <cstdio>

#include <mango/core/string.hpp>
#include <mango/core/exception.hpp>
#include <mango/filesystem/file.hpp>

#define ID "FileStream: "

namespace mango
{

    // -----------------------------------------------------------------
	// FileHandle
    // -----------------------------------------------------------------

	struct FileHandle
	{
		FILE* m_file;
		uint64 m_size;
        std::string m_filename;

        FileHandle(const std::string& filename, const char* mode)
        : m_filename(filename)
		{
			// open file
            m_file = std::fopen(filename.c_str(), mode);

			// cache file size
	        fseeko(m_file, 0, SEEK_END);
	        m_size = static_cast<uint64>(ftello(m_file));
	        fseeko(m_file, 0, SEEK_SET);
		}

		~FileHandle()
		{
            std::fclose(m_file);
		}

        const std::string& filename() const
        {
            return m_filename;
        }

        uint64 size() const
		{
			return m_size;
		}

		uint64 offset() const
		{
	        return ftello(m_file);
		}

		void seek(uint64 distance, int method)
		{
	        fseeko(m_file, distance, method);
		}

	    void read(void* dest, size_t size)
	    {
    	    size_t status = std::fread(dest, 1, size, m_file);
	        MANGO_UNREFERENCED_PARAMETER(status);
	    }

	    void write(const void* data, size_t size)
	    {
	        size_t status = std::fwrite(data, 1, size, m_file);
	        MANGO_UNREFERENCED_PARAMETER(status);
	    }
	};

    // -----------------------------------------------------------------
    // FileStream
    // -----------------------------------------------------------------

    FileStream::FileStream(const std::string& filename, OpenMode openmode)
    : m_handle(NULL)
    {
		const char* mode;

       	switch (openmode)
        {
   	        case READ:
                mode = "rb";
                break;

   	        case WRITE:
       	        mode = "wb";
           	    break;

            default:
	            MANGO_EXCEPTION(ID"Incorrect OpenMode.");
                break;
        }

		m_handle = new FileHandle(filename, mode);
    }

    FileStream::~FileStream()
    {
		delete m_handle;
    }

    const std::string& FileStream::filename() const
    {
        return m_handle->filename();
    }

    uint64 FileStream::size() const
    {
		return m_handle->size();
    }

    uint64 FileStream::offset() const
    {
		return m_handle->offset();
    }

    void FileStream::seek(uint64 distance, SeekMode mode)
    {
        int method;

        switch (mode)
        {
            case BEGIN:
                method = SEEK_SET;
                break;

            case CURRENT:
                method = SEEK_CUR;
                break;

            case END:
                method = SEEK_END;
                break;

            default:
                MANGO_EXCEPTION(ID"Invalid seek mode.");
        }

		m_handle->seek(distance, method);
    }

    void FileStream::read(void* dest, size_t size)
    {
		m_handle->read(dest, size);
    }

    void FileStream::write(const void* data, size_t size)
    {
		m_handle->write(data, size);
    }

} // namespace mango
