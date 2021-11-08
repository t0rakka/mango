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

    FileMapper* createMapperZIP(ConstMemory parent, const std::string& password);
    FileMapper* createMapperRAR(ConstMemory parent, const std::string& password);
    FileMapper* createMapperMGX(ConstMemory parent, const std::string& password);

    using CreateMapperFunc = FileMapper* (*)(ConstMemory, const std::string&);

    struct MapperExtension
    {
        std::string extension;
        CreateMapperFunc create;

        MapperExtension(const std::string& extension, CreateMapperFunc func)
            : extension(extension)
            , create(func)
        {
        }

        ~MapperExtension()
        {
        }
    };

    static std::vector<MapperExtension> g_mapper_extensions =
    {
        MapperExtension(".zip", createMapperZIP),
        MapperExtension(".cbz", createMapperZIP),
        MapperExtension(".apk", createMapperZIP),
        MapperExtension(".zipx", createMapperZIP),

        MapperExtension(".mgx", createMapperMGX),
        MapperExtension(".snitch", createMapperMGX),

        MapperExtension(".rar", createMapperRAR),
        MapperExtension(".cbr", createMapperRAR),
    };

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
        m_basepath = parse(pathname, password);
    }

    Mapper::Mapper(std::shared_ptr<Mapper> mapper, const std::string& pathname, const std::string& password)
    {
        // use parent's mapper
        m_parent_mapper = mapper;
        m_current_mapper = *mapper;

        // parse and create mappers
        m_pathname = m_parent_mapper->m_pathname;
        m_basepath = parse(pathname, password);
    }

    Mapper::Mapper(ConstMemory memory, const std::string& extension, const std::string& password)
    {
        // create mapper to raw memory
        m_current_mapper = createMemoryMapper(memory, extension, password);
        m_pathname = "@memory" + extension + "/";
    }

    Mapper::~Mapper()
    {
        delete m_parent_memory;
    }

    std::string Mapper::parse(const std::string& pathname, const std::string& password)
    {
        if (!m_current_mapper)
        {
            m_current_mapper = createFileMapper();
        }

printf("# parse | m_pathname: %s, pathname: %s \n", m_pathname.c_str(), pathname.c_str());

        std::string filename = m_basepath + pathname;

        for ( ; !filename.empty(); )
        {
            FileMapper* x = nullptr;

            std::string str = toLower(filename);

            for (const auto& node : g_mapper_extensions)
            {
                std::string decorated = node.extension + "/";

                size_t n = str.find(decorated);
                if (n != std::string::npos)
                {
                    // update string position to skip decorated extension (example: ".zip/")
                    n += decorated.length();

printf("# found: %s \n", filename.c_str());
                    // resolve container filename (example: "foo/bar/data.zip")
                    std::string container = filename.substr(0, n - 1);
                    std::string postfix = filename.substr(n, std::string::npos);
printf("# container: %s \n", container.c_str());

                    if (m_current_mapper->isFile(container))
                    {
printf("# isFile: %s \n", container.c_str());
                        m_parent_memory = m_current_mapper->mmap(container);
                        x = node.create(*m_parent_memory, password);
                        m_mappers.emplace_back(x);
                        m_current_mapper = x;

                        filename = postfix;
printf("# create | postfix: %s \n", postfix.c_str());
                    }
                }
            }

            if (!x)
            {
                break;
            }
        }

        m_pathname = m_pathname + pathname;

printf("# ~parse | m_pathname: %s, pathname: %s \n", m_pathname.c_str(), pathname.c_str());
        return filename;
    }

    FileMapper* Mapper::createMemoryMapper(ConstMemory memory, const std::string& extension, const std::string& password)
    {
        std::string str = toLower(extension);

        for (const auto& node : g_mapper_extensions)
        {
            size_t n = str.find(node.extension);
            if (n != std::string::npos)
            {
                // found a container interface; let's create it
                FileMapper* x = node.create(memory, password);
                m_mappers.emplace_back(x);
                return x;
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

    Mapper::operator FileMapper* () const
    {
        return m_current_mapper;
    }

    bool Mapper::isCustomMapper(const std::string& filename)
    {
        const std::string extension = toLower(getExtension(filename));

        for (const auto& node : g_mapper_extensions)
        {
            if (extension == node.extension)
            {
                return true;
            }
        }

        return false;
    }

} // namespace mango::filesystem
