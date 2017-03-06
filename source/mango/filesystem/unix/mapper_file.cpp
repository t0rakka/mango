/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include <mango/filesystem/mapper.hpp>
#include <mango/filesystem/path.hpp>

#define ID ""

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

namespace
{

    using namespace mango;

    // -----------------------------------------------------------------
	// get_pagesize()
    // -----------------------------------------------------------------

	inline long get_pagesize()
	{
		// NOTE: could be _SC_PAGE_SIZE for some platforms according to Linux Programmer's Manual
		static long x = sysconf(_SC_PAGESIZE);
		return x;
	}

    // -----------------------------------------------------------------
    // FileMemory
    // -----------------------------------------------------------------

    class FileMemory : public Memory
    {
    protected:
        int m_file;
		size_t m_size;
		void* m_address;

    public:
        FileMemory(const std::string& filename, uint64 _offset, uint64 _size)
        : m_file(-1), m_size(0), m_address(NULL)
        {
            m_file = open(filename.c_str(), O_RDONLY);

            if (m_file != -1)
            {
                struct stat sb;

                if (fstat(m_file, &sb) == -1)
                {
                    close(m_file);
                    m_file = -1;
	                MANGO_EXCEPTION(ID"File cannot be read.");
                }
                else
                {
					const size_t file_size = static_cast<size_t>(sb.st_size);
					const size_t file_offset = static_cast<size_t>(_offset);

					size_t page_offset = 0;
					if (file_offset > 0)
					{
						const long page_size = get_pagesize();
						const long page_number = file_offset / page_size;
						page_offset = page_number * page_size;
					}

					m_size = file_size - file_offset;
					if (_size > 0)
					{
						m_size = std::min(size_t(_size), m_size);
					}

                    if (m_size > 0)
                    {
                        m_address = ::mmap(NULL, m_size, PROT_READ, MAP_PRIVATE, m_file, page_offset);

                        if (m_address == MAP_FAILED)
                        {
                            std::string msg = ID"Memory mapping \"";
                            msg += filename;
                            msg += "\" failed.";
                            MANGO_EXCEPTION(msg);
                        }

                        size = m_size;
                        address = reinterpret_cast<uint8*>(m_address) + (file_offset - page_offset);
                    }
                    else
                    {
                        size = 0;
                        address = NULL;
                    }
                }
            }
            else
            {
                std::string msg = ID"Opening \"";
                msg += filename;
                msg += "\" failed.";
                MANGO_EXCEPTION(msg);
            }
        }

        ~FileMemory()
        {
            if (m_address)
            {
                ::munmap(m_address, m_size);
            }

            if (m_file != -1)
            {
                close(m_file);
            }
        }
    };

    // -----------------------------------------------------------------
    // FileMapper
    // -----------------------------------------------------------------

    class FileMapper : public AbstractMapper
    {
    protected:
        void emplace(FileIndex& index, const std::string& pathname, const std::string& filename)
        {
            const std::string testname = pathname + filename;

            struct stat s;
            if (stat(testname.c_str(), &s) != -1)
            {
                if ((s.st_mode & S_IFDIR) == 0)
                {
                    size_t size = static_cast<size_t>(s.st_size);
                    ::emplace(index, filename, size, 0);
                }
                else
                {
                    ::emplace(index, filename + "/", 0, FileInfo::DIRECTORY);
                }
            }
        }

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

            struct stat s;

            if (stat(filename.c_str(), &s) == 0)
            {
                is = (s.st_mode & S_IFDIR) == 0;
            }

            return is;
        }

#if defined(MANGO_PLATFORM_OSX) || defined(MANGO_PLATFORM_IOS) || defined(MANGO_PLATFORM_BSD)

        void index(FileIndex& index, const std::string& pathname)
        {
            struct dirent** namelist = NULL;

            const int n = scandir(pathname.c_str(), &namelist, [] (const struct dirent* e)
            {
                // filter out "." and ".."
                if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
                    return 0;
                return 1;
            }, 0);
            if (n < 0)
            {
                // Unable to open directory.
                free(namelist);
                return;
            }

            for (int i = 0; i < n; ++i)
            {
                const std::string filename(namelist[i]->d_name);
                free(namelist[i]);
                emplace(index, pathname, filename);
            }

            free(namelist);
        }

#else

        void index(FileIndex& index, const std::string& pathname)
        {
            DIR* dirp = opendir(pathname.c_str());
            if (!dirp)
            {
                // Unable to open directory.
                return;
            }

            dirent* dp;

            while ((dp = readdir(dirp)))
            {
                std::string filename(dp->d_name);

                // skip "." and ".."
                if (filename != "." && filename != "..")
                {
                    emplace(index, pathname, filename);
                }
            }

            closedir(dirp);
        }

#endif

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
