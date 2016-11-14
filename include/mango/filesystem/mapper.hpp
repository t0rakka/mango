/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include "../core/configure.hpp"
#include "../core/memory.hpp"
#include "fileindex.hpp"

namespace mango
{

    class AbstractMapper
    {
    public:
        AbstractMapper() = default;
        virtual ~AbstractMapper() = default;

        virtual bool isfile(const std::string& filename) const = 0;
        virtual void index(FileIndex& index, const std::string& pathname) = 0;
        virtual Memory* mmap(const std::string& filename) = 0;
    };

    class Mapper
    {
    protected:
        AbstractMapper* m_mapper;
        Memory* m_parent_memory;
        std::vector<std::unique_ptr<AbstractMapper>> m_mappers;
        std::string m_pathname;

        std::string parse(const std::string& pathname, const std::string& password);
        AbstractMapper* create(AbstractMapper* parent, std::string& filename, const std::string& password);
        AbstractMapper* getFileMapper() const;

    public:
        Mapper();
        ~Mapper();

        operator AbstractMapper* () const;
        static bool isCustomMapper(const std::string& filename);
    };

} // namespace mango
