/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        m_memory = std::unique_ptr<VirtualMemory>(m_mapper->mmap(m_filename));
    }

    File::File(const std::string& filename)
    : m_offset(0)
    {
        m_mapper = getFileMapper();
        m_filename = parse(filename, "");
        m_memory = std::unique_ptr<VirtualMemory>(m_mapper->mmap(m_filename));
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
        return *m_memory;
    }

	File::operator const uint8* () const
	{
        return (*m_memory)->address;
	}

    const uint8* File::data() const
    {
        return (*m_memory)->address;
    }

} // namespace mango
