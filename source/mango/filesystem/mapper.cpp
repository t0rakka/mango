/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <vector>
#include <algorithm>
#include <mango/core/string.hpp>
#include <mango/filesystem/mapper.hpp>
#include <mango/filesystem/path.hpp>

namespace mango
{

    // -----------------------------------------------------------------
    // extension registry
    // -----------------------------------------------------------------

    AbstractMapper* createMapperZIP(Memory parent, const std::string& password);
#ifdef MANGO_ENABLE_LICENSE_GPL
    AbstractMapper* createMapperRAR(Memory parent, const std::string& password);
#endif
    //AbstractMapper* createMapperMGX(Memory parent, const std::string& password);

    typedef AbstractMapper* (*CreateMapperFunc)(Memory, const std::string&);

    struct MapperExtension
    {
        std::string extension;
        std::string decorated_extension;
        CreateMapperFunc createMapper;

        MapperExtension(const std::string& extension, CreateMapperFunc func)
            : extension(extension)
        {
            decorated_extension = std::string(".") + extension + "/";
            createMapper = func;
        }

        ~MapperExtension()
        {
        }

        AbstractMapper* create(Memory parent, const std::string& password) const
        {
            AbstractMapper* mapper = createMapper(parent, password);
            return mapper;
        }
    };

    static std::vector<MapperExtension> g_extensions = []
    {
        std::vector<MapperExtension> extensions;

        extensions.push_back(MapperExtension("zip", createMapperZIP));
        extensions.push_back(MapperExtension("cbz", createMapperZIP));

#ifdef MANGO_ENABLE_LICENSE_GPL
        extensions.push_back(MapperExtension("rar", createMapperRAR));
        extensions.push_back(MapperExtension("cbr", createMapperRAR));
#endif

        return extensions;
    } ();

    // -----------------------------------------------------------------
    // FileInfo
    // -----------------------------------------------------------------

    FileInfo::FileInfo()
        : size(0)
        , flags(0)
    {
    }

    FileInfo::FileInfo(const std::string& name, uint64 size, uint32 flags)
        : size(size)
        , flags(flags)
        , name(name)
    {
    }

    FileInfo::~FileInfo()
    {
    }

    bool FileInfo::isDirectory() const
    {
        return (flags & DIRECTORY) != 0;
    }

    bool FileInfo::isContainer() const
    {
        return (flags & CONTAINER) != 0;
    }

    bool FileInfo::isCompressed() const
    {
        return (flags & COMPRESSED) != 0;
    }

    // -----------------------------------------------------------------
    // FileIndex
    // -----------------------------------------------------------------

    void FileIndex::emplace(const std::string &name, uint64 size, uint32 flags)
    {
        emplace_back(name, size, flags);

        const bool isFile = (flags & FileInfo::DIRECTORY) == 0;
        if (isFile && Mapper::isCustomMapper(name))
        {
            // file is a container; add it into the index again as such
            emplace_back(name + "/", 0, FileInfo::DIRECTORY | FileInfo::CONTAINER);
        }
    }

    // -----------------------------------------------------------------
    // Mapper
    // -----------------------------------------------------------------

    Mapper::Mapper()
		: m_mapper(nullptr)
        , m_parent_memory(nullptr)
        , m_mappers()
        , m_pathname()
    {
    }

    Mapper::~Mapper()
    {
		delete m_parent_memory;
    }

    std::string Mapper::parse(const std::string& pathname, const std::string& password)
    {
        std::string filename = pathname;

        for ( ; !filename.empty(); )
        {
            AbstractMapper* custom_mapper = create(m_mapper, filename, password);
            if (custom_mapper)
            {
                m_mappers.emplace_back(custom_mapper);
                m_mapper = custom_mapper;
            }
            else
            {
                break;
            }
        }

        return filename;
    }

    AbstractMapper* Mapper::create(AbstractMapper* parent, std::string& filename, const std::string& password)
    {
        std::string f = toLower(filename);

        for (auto &extension : g_extensions)
        {
            size_t n = f.find(extension.decorated_extension);
            if (n != std::string::npos)
            {
                // update string position to skip decorated extension (example: ".zip/")
                n += extension.decorated_extension.length();

                // resolve container filename (example: "foo/bar/data.zip")
                std::string container_filename = filename.substr(0, n - 1);

                AbstractMapper* custom_mapper = nullptr;

                if (parent->isfile(container_filename))
                {
                    m_parent_memory = parent->mmap(container_filename);
                    custom_mapper = extension.create(*m_parent_memory, password);
                    filename = filename.substr(n, std::string::npos);
                }

                return custom_mapper;
            }
        }

        return nullptr;
    }

    Mapper::operator AbstractMapper* () const
    {
        return m_mapper;
    }

    bool Mapper::isCustomMapper(const std::string& filename)
    {
        const std::string extension = toLower(getExtension(filename));

        for (auto &node : g_extensions) {
            if (extension == node.extension) {
                return true;
            }
        }

        return false;
    }

} // namespace mango
