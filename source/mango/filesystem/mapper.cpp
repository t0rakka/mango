/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <vector>
#include <algorithm>
#include <mango/core/string.hpp>
#include <mango/filesystem/mapper.hpp>
#include <mango/filesystem/path.hpp>

namespace mango::filesystem
{

    // -----------------------------------------------------------------
    // extension registry
    // -----------------------------------------------------------------

    AbstractMapper* createMapperZIP(ConstMemory parent, const std::string& password);
    AbstractMapper* createMapperRAR(ConstMemory parent, const std::string& password);
    AbstractMapper* createMapperMGX(ConstMemory parent, const std::string& password);

    using CreateMapperFunc = AbstractMapper* (*)(ConstMemory, const std::string&);

    struct MapperExtension
    {
        CreateMapperFunc create;
        std::string extension;
        std::string decorated_extension;

        MapperExtension(CreateMapperFunc func, const std::string& extension)
            : create(func)
            , extension(extension)
            , decorated_extension(extension + "/")
        {
        }

        ~MapperExtension()
        {
        }
    };

    static std::vector<MapperExtension> g_extensions =
    {
        MapperExtension(createMapperZIP, ".zip"),
        MapperExtension(createMapperZIP, ".cbz"),
        MapperExtension(createMapperZIP, ".apk"),
        MapperExtension(createMapperZIP, ".zipx"),
        MapperExtension(createMapperMGX, ".mgx"),
        MapperExtension(createMapperMGX, ".snitch"),
        MapperExtension(createMapperRAR, ".rar"),
        MapperExtension(createMapperRAR, ".cbr"),
    };

    static inline
    const MapperExtension* findMapperExtension(const std::string& extension)
    {
        for (const auto& node : g_extensions)
        {
            if (extension == node.extension)
            {
                return &node;
            }
        }

        return nullptr;
    }

    // -----------------------------------------------------------------
    // FileInfo
    // -----------------------------------------------------------------

    FileInfo::FileInfo()
        : size(0)
        , flags(0)
    {
    }

    FileInfo::FileInfo(const std::string& name, u64 size, u32 flags)
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

    void FileIndex::emplace(const std::string& name, u64 size, u32 flags)
    {
        if (!name.empty())
        {
            files.emplace_back(name, size, flags);
 
            const bool isFile = (flags & FileInfo::DIRECTORY) == 0;
            const bool isContainer = (flags & FileInfo::CONTAINER) != 0;
 
            if (isFile && !isContainer && name.back() != '/' && Mapper::isCustomMapper(name))
            {
                // file is a container; add it into the index again
                files.emplace_back(name + "/", 0, flags | FileInfo::DIRECTORY | FileInfo::CONTAINER);
            }
        }
    }

    // -----------------------------------------------------------------
    // Mapper
    // -----------------------------------------------------------------

    Mapper::Mapper(const std::string& pathname, const std::string& password)
    {
        // parse and create mappers
        m_pathname = pathname;
        m_basepath = parse(pathname, password);
    }

    Mapper::Mapper(std::shared_ptr<Mapper> mapper, const std::string& pathname, const std::string& password)
    {
        // use parent's mapper
        m_parent_mapper = mapper;
        m_mapper = *mapper;

        // parse and create mappers
        m_pathname = mapper->m_pathname + pathname;
        m_basepath = parse(mapper->m_basepath + pathname, password);
    }

    Mapper::Mapper(ConstMemory memory, const std::string& extension, const std::string& password)
    {
        std::string ext = toLower(extension);
        const MapperExtension* node = findMapperExtension(ext);
        if (node)
        {
            AbstractMapper* mapper = node->create(memory, password);
            m_mappers.emplace_back(mapper);
            m_mapper = mapper;
            m_pathname = "@memory" + extension + "/";
        }
    }

    Mapper::~Mapper()
    {
        delete m_parent_memory;
    }

    std::string Mapper::parse(const std::string& pathname, const std::string& password)
    {
        if (!m_mapper)
        {
            m_mapper = createFileMapper("");
        }

        // TODO: rewrite using string_view
        std::string mixedcase_pathname = pathname;
        std::string lowercase_pathname = toLower(pathname);

        for ( ; !mixedcase_pathname.empty(); )
        {
            AbstractMapper* mapper = nullptr;

            for (const auto& node : g_extensions)
            {
                size_t n = lowercase_pathname.find(node.decorated_extension);
                if (n != std::string::npos)
                {
                    // update string position to skip decorated extension (example: ".zip/")
                    n += node.decorated_extension.length();

                    // resolve container filename (example: "foo/bar/data.zip")
                    std::string container = mixedcase_pathname.substr(0, n - 1);

                    if (m_mapper->isFile(container))
                    {
                        m_parent_memory = m_mapper->mmap(container);

                        mapper = node.create(*m_parent_memory, password);
                        m_mappers.emplace_back(mapper);
                        m_mapper = mapper;

                        mixedcase_pathname = mixedcase_pathname.substr(n, std::string::npos);
                        lowercase_pathname = lowercase_pathname.substr(n, std::string::npos);
                        break;
                    }
                }
            }

            if (!mapper)
            {
                // no containers detected in remaining pathname
                break;
            }
        }

        return mixedcase_pathname;
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
        const MapperExtension* node = findMapperExtension(extension);
        return node != nullptr;
    }

} // namespace mango::filesystem
