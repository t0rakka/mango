/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
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
    AbstractMapper* createMapperMGX(Memory parent, const std::string& password);

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
        extensions.push_back(MapperExtension("zipx", createMapperZIP));

        extensions.push_back(MapperExtension("mgx", createMapperMGX));

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

    bool FileInfo::isEncrypted() const
    {
        return (flags & ENCRYPTED) != 0;
    }

    // -----------------------------------------------------------------
    // FileIndex
    // -----------------------------------------------------------------

    void FileIndex::emplace(const std::string &name, uint64 size, uint32 flags)
    {
        files.emplace_back(name, size, flags);

        const bool isFile = (flags & FileInfo::DIRECTORY) == 0;
        if (isFile && Mapper::isCustomMapper(name))
        {
            // file is a container; add it into the index again as such
            files.emplace_back(name + "/", 0, flags | FileInfo::DIRECTORY | FileInfo::CONTAINER);
        }
    }

    // -----------------------------------------------------------------
    // Mapper
    // -----------------------------------------------------------------

    Mapper::Mapper()
    {
    }

    Mapper::~Mapper()
    {
		delete m_parent_memory;
    }

    std::string Mapper::parse(std::string& pathname, const std::string& password)
    {
        std::string filename = pathname;

        for ( ; !filename.empty(); )
        {
            AbstractMapper* custom_mapper = createCustomMapper(pathname, filename, password);
            if (!custom_mapper)
            {
                break;
            }
        }

        return filename;
    }

    AbstractMapper* Mapper::createCustomMapper(std::string& pathname, std::string& filename, const std::string& password)
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
                std::string container = filename.substr(0, n - 1);
                std::string postfix = filename.substr(n, std::string::npos);

                AbstractMapper* custom_mapper = nullptr;

                if (!m_mapper)
                {
                    size_t n = container.find_last_of("/\\:");
                    std::string head = container.substr(0, n + 1);
                    container = container.substr(n + 1, std::string::npos);
                    m_mapper = createFileMapper(head);
                }

                if (m_mapper->isFile(container))
                {
                    m_parent_memory = m_mapper->mmap(container);
                    custom_mapper = extension.create(*m_parent_memory, password);
                    m_mappers.emplace_back(custom_mapper);
                    m_mapper = custom_mapper;

                    filename = postfix;
                    pathname = postfix;
                }

                return custom_mapper;
            }
        }

        if (!m_mapper)
        {
            m_mapper = createFileMapper(pathname);
            pathname = "";
        }

        return nullptr;
    }

    AbstractMapper* Mapper::createMemoryMapper(Memory memory, const std::string& extension, const std::string& password)
    {
        std::string f = toLower(extension);

        for (auto &extension : g_extensions)
        {
            size_t n = f.find(extension.decorated_extension);
            if (n == std::string::npos)
            {
                // try again with a non-decorated extension
                n = f.find(extension.extension);
            }

            if (n != std::string::npos)
            {
                // found a container interface; let's create it
                AbstractMapper* custom_mapper = extension.create(memory, password);
                m_mappers.emplace_back(custom_mapper);
                return custom_mapper;
            }
        }

        return nullptr;
    }

    const std::string& Mapper::basepath() const
    {
        return m_basepath;
    }

    const std::string& Mapper::pathname() const
    {
        return m_pathname;
    }

    Mapper::operator AbstractMapper* () const
    {
        return m_mapper;
    }

    bool Mapper::isCustomMapper(const std::string& filename)
    {
        const std::string extension = toLower(getExtension(filename));

        for (auto &node : g_extensions)
        {
            if (extension == node.extension)
            {
                return true;
            }
        }

        return false;
    }

} // namespace mango
