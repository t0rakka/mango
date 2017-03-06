/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include <mango/filesystem/mapper.hpp>
#include <mango/filesystem/path.hpp>

#define ID ""

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace
{

    using namespace mango;

    // -----------------------------------------------------------------
    // FileMemory
    // -----------------------------------------------------------------

    class FileMemory : public Memory
    {
    protected:
        LPVOID  m_address;
        HANDLE  m_file;
        HANDLE  m_map;

    public:
        FileMemory(const std::string& filename, uint64 _offset, uint64 _size)
        : m_address(NULL), m_file(INVALID_HANDLE_VALUE), m_map(NULL)
        {
			m_file = CreateFileW(u16_fromBytes(filename).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

			// special handling when too long filename
			if (m_file == INVALID_HANDLE_VALUE)
			{
				// TODO: use UNC filename or ShortPath
				if (filename.length() > MAX_PATH)
				{
					address = NULL;
					size = 0;
					return;
				}
			}

            if (m_file != INVALID_HANDLE_VALUE)
            {
                LARGE_INTEGER file_size;
                GetFileSizeEx(m_file, &file_size);

				if (file_size.QuadPart > 0)
				{
					DWORD maxSizeHigh = 0;
					DWORD maxSizeLow = 0;
					m_map = CreateFileMappingW(m_file, NULL, PAGE_READONLY, maxSizeHigh, maxSizeLow, NULL);

					if (m_map)
					{
						DWORD offsetHigh = 0;
						DWORD offsetLow = 0;
						SIZE_T bytes = 0;

						uint64 page_offset = 0;
						if (_offset > 0)
						{
							SYSTEM_INFO info;
							::GetSystemInfo(&info);
							const DWORD page_size = info.dwAllocationGranularity;
							const DWORD page_number = static_cast<DWORD>(_offset / page_size);
							page_offset = page_number * page_size;
							offsetHigh = DWORD(page_offset >> 32);
							offsetLow = DWORD(page_offset & 0xffffffff);
						}

						size = static_cast<size_t>(file_size.QuadPart);
						if (_size > 0)
						{
							size = std::min(size, static_cast<size_t>(_size));
							bytes = static_cast<SIZE_T>(size);
						}

						LPVOID address_ = MapViewOfFile(m_map, FILE_MAP_READ, offsetHigh, offsetLow, bytes);

						m_address = address_;
						address = reinterpret_cast<uint8*>(address_) + (_offset - page_offset);
					}
					else
					{
						MANGO_EXCEPTION(ID"Memory mapping failed.");
					}
				}
            }
            else
            {
                MANGO_EXCEPTION(ID"File cannot be opened.");
            }
        }

        ~FileMemory()
        {
            if (m_address)
            {
                UnmapViewOfFile(m_address);
            }

            if (m_map)
            {
                CloseHandle(m_map);
            }

            if (m_file != INVALID_HANDLE_VALUE)
            {
                CloseHandle(m_file);
            }
        }
    };

    // -----------------------------------------------------------------
    // FileMapper
    // -----------------------------------------------------------------

    class FileMapper : public AbstractMapper
    {
    public:
        FileMapper()
        {
        }

        ~FileMapper()
        {
        }

        bool isfile(const std::string& filename) const
        {
            bool is = false;

            struct _stati64 s;

            if (_wstat64(u16_fromBytes(filename).c_str(), &s) == 0)
            {
                is = (s.st_mode & _S_IFDIR) == 0;
            }

            return is;
        }

        void index(FileIndex& index, const std::string& pathname)
        {
            std::wstring filespec = u16_fromBytes(pathname + "*");

            _wfinddatai64_t cfile;

            intptr_t hfile = ::_wfindfirst64(filespec.c_str(), &cfile);

            // find first file in current directory
            if (hfile != -1L)
            {
                for (;;)
                {
                    std::string filename = u16_toBytes(cfile.name);

                    // skip "." and ".."
                    if (filename != "." && filename != "..")
                    {
                        bool isfile = (cfile.attrib & _A_SUBDIR) == 0;

                        if (isfile)
                        {
                            size_t filesize = static_cast<size_t>(cfile.size);
                            emplace(index, filename, filesize, 0);
                        }
                        else
                        {
                            emplace(index, filename + "/", 0, FileInfo::DIRECTORY);
                        }
                    }

                    if (::_wfindnext64(hfile, &cfile) != 0)
                        break;
                }

                ::_findclose(hfile);
            }
        }

        Memory* mmap(const std::string& filename)
        {
            Memory* memory = new FileMemory(filename, 0, 0);
            return memory;
        }
    };

} // namespace

namespace mango
{

    // -----------------------------------------------------------------
    // Mapper::createFileMapper()
    // -----------------------------------------------------------------

    FileMapper g_fileMapper;

    AbstractMapper* Mapper::getFileMapper() const
    {
        return &g_fileMapper;
    }

} // namespace mango
