/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        std::string ext;
        std::string extid;
        CreateMapperFunc createMapper;

        MapperExtension(const char* extension, CreateMapperFunc func)
        {
            ext = extension;
            extid = std::string(".") + extension + "/";
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

    bool Mapper::isCustomMapper(const std::string& filename)
    {
        const std::string ext = toLower(getExtension(filename));

        for (auto& node : g_extensions) {
            if (ext == node.ext) {
                return true;
            }
        }

        return false;
    }

    // -----------------------------------------------------------------
    // Mapper
    // -----------------------------------------------------------------

    Mapper::Mapper()
    : m_mapper(nullptr), m_parent_memory(nullptr)
    {
    }

    Mapper::~Mapper()
    {
		delete m_parent_memory;
    }

    std::string Mapper::parse(const std::string& pathname, const std::string& password)
    {
        std::string filename = pathname;

        for (; !filename.empty();)
        {
            AbstractMapper* custom = create(m_mapper, filename, password);
            if (custom)
            {
                m_mappers.emplace_back(custom);
                m_mapper = custom;
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

        for (auto& i : g_extensions)
        {
            size_t n = f.find(i.extid);

            if (n != std::string::npos)
            {
                MapperExtension* extension = &i;

                n += extension->extid.length();
                std::string fn = filename.substr(0, n - 1);

                AbstractMapper* mapper = nullptr;

                if (parent->isfile(fn))
                {
                    m_parent_memory = parent->mmap(fn);
                    mapper = extension->create(*m_parent_memory, password);
                    filename = filename.substr(n, std::string::npos);
                }

                return mapper;
            }
        }

        return nullptr;
    }

    Mapper::operator AbstractMapper* () const
    {
        return m_mapper;
    }

} // namespace mango
