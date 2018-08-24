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
    {
        // use parent path's mapper
        m_mapper = path;
        m_pathname = path.pathname();

        // parse and create mappers
        m_filename = parse(m_pathname + filename, "");

        // memory map the file
        VirtualMemory* vmemory = m_mapper->mmap(m_filename);
        m_memory = UniqueObject<VirtualMemory>(vmemory);
    }

    File::File(const std::string& filename)
    {
		// create mapper to raw filesystem
        m_mapper = getFileMapper();

        // parse and create mappers
        m_filename = parse(filename, "");

        // memory map the file
        VirtualMemory* memory = m_mapper->mmap(m_filename);
        m_memory = UniqueObject<VirtualMemory>(memory);
    }

    File::File(const Memory& memory, const std::string& extension, const std::string& filename)
    {
        Path path(memory, extension, filename);

        // use temporary path's mapper
        m_mapper = path;
        m_pathname = path.pathname();

        // parse and create mappers
        m_filename = parse(filename, "");

        // memory map the file
        VirtualMemory* vmemory = m_mapper->mmap(m_filename);
        m_memory = UniqueObject<VirtualMemory>(vmemory);
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
