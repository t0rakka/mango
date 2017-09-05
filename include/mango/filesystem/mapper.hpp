/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include "../core/configure.hpp"
#include "../core/memory.hpp"

namespace mango
{

    struct FileInfo
    {
        enum Flags
        {
            DIRECTORY = 0x01,
            CONTAINER = 0x02,
            COMPRESSED = 0x04,
        };

        uint64 size;
        uint32 flags;
        std::string name;

        FileInfo();
        FileInfo(const std::string& name, uint64 size, uint32 flags = 0);
        ~FileInfo();

        bool isDirectory() const;
        bool isContainer() const;
        bool isCompressed() const;
    };

    struct FileIndex : std::vector<FileInfo>
    {
        void emplace(const std::string &name, uint64 size, uint32 flags);
    };

    class AbstractMapper : protected NonCopyable
    {
    public:
        AbstractMapper() = default;
        virtual ~AbstractMapper() = default;

        virtual bool isfile(const std::string& filename) const = 0;
        virtual void index(FileIndex& index, const std::string& pathname) = 0;
        virtual VirtualMemory* mmap(const std::string& filename) = 0;
    };

    class Mapper : protected NonCopyable
    {
    protected:
        AbstractMapper* m_mapper;
        VirtualMemory* m_parent_memory;
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
