/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/string.hpp>
#include <mango/core/exception.hpp>
#include <mango/filesystem/file.hpp>

#define ID "File: "

namespace mango
{

    // -----------------------------------------------------------------
    // File
    // -----------------------------------------------------------------

    File::File(const Path& path, const std::string& filename)
    : m_offset(0)
    {
        m_mapper = path;
        m_pathname = path.pathname();
        m_filename = parse(m_pathname + filename, "");
        m_memory = std::unique_ptr<Memory>(m_mapper->mmap(m_filename));
    }

    File::File(const std::string& filename)
    : m_offset(0)
    {
        m_mapper = getFileMapper();
        m_filename = parse(filename, "");
        m_memory = std::unique_ptr<Memory>(m_mapper->mmap(m_filename));
    }

    File::~File()
    {
    }

    const std::string& File::filename() const
    {
        return m_filename;
    }

    File::operator Memory () const
    {
        return m_memory->slice(0);
    }

	File::operator const uint8* () const
	{
		return m_memory->address;
	}

    const uint8* File::data() const
    {
        return m_memory->address;
    }

    uint64 File::size() const
    {
        return m_memory->size;
    }

    uint64 File::offset() const
    {
        return m_offset;
    }

    void File::seek(uint64 distance, SeekMode mode)
    {
        switch (mode)
        {
            case BEGIN:
                m_offset = distance;
                break;

            case CURRENT:
                m_offset += distance;
                break;

            case END:
                m_offset = m_memory->size - distance;
                break;

            default:
                MANGO_EXCEPTION(ID"Invalid seek mode.");
        }
    }

    void File::read(void* dest, size_t size)
    {
        const size_t left = m_memory->size - static_cast<size_t>(m_offset);
        if (left < size)
        {
            size = left;
        }
        std::memcpy(dest, m_memory->address + m_offset, size);
        m_offset += size;
    }

    void File::write(const void* data, size_t size)
    {
        MANGO_UNREFERENCED_PARAMETER(data);
        MANGO_UNREFERENCED_PARAMETER(size);
        MANGO_EXCEPTION(ID"Cannot write() in read-only file.");
    }

} // namespace mango
