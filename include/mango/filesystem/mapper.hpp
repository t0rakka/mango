/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include <mango/core/configure.hpp>
#include <mango/core/memory.hpp>

namespace mango::filesystem
{

    struct FileInfo
    {
        enum Flags : u32
        {
            Directory  = 0x00000001,
            Container  = 0x00000002,
            Compressed = 0x00000004,
            Encrypted  = 0x00000008,
        };

        std::string name;
        u64 size;
        u32 flags;
        u32 checksum;

        FileInfo();
        FileInfo(const std::string& name, u64 size, u32 flags = 0, u32 checksum = 0);
        ~FileInfo();

        bool isFile() const;
        bool isDirectory() const;
        bool isContainer() const;
        bool isCompressed() const;
        bool isEncrypted() const;
    };

    struct FileIndex
    {
        std::vector<FileInfo> files;

        void emplace(const std::string& name, u64 size, u32 flags, u32 checksum);
        void emplace(const std::string& name, u64 size, u32 flags);
        void emplace(const std::string& name);

        size_t size() const
        {
            return files.size();
        }

        bool empty() const
        {
            return files.empty();
        }

        void clear()
        {
            files.clear();
        }

        FileInfo& operator [] (size_t index)
        {
            return files[index];
        }

        const FileInfo& operator [] (size_t index) const
        {
            return files[index];
        }

        auto begin()
        {
            return files.begin();
        }

        auto end()
        {
            return files.end();
        }

        auto begin() const
        {
            return files.begin();
        }

        auto end() const
        {
            return files.end();
        }
    };

    class AbstractMapper : protected NonCopyable
    {
    public:
        AbstractMapper() = default;
        virtual ~AbstractMapper() = default;

        // interface
        virtual u64 getSize(const std::string& filename) const = 0;
        virtual bool isFile(const std::string& filename) const = 0;
        virtual void getIndex(FileIndex& index, const std::string& pathname) = 0;
        virtual std::unique_ptr<VirtualMemory> map(const std::string& filename) = 0;
    };

    class Mapper : public AbstractMapper
    {
    protected:
        std::shared_ptr<Mapper> m_parent_mapper;
        std::vector<std::unique_ptr<AbstractMapper>> m_mappers;
        std::unique_ptr<VirtualMemory> m_parent_memory { nullptr };
        AbstractMapper* m_current_mapper { nullptr };

        std::string m_basepath;
        std::string m_pathname;

        AbstractMapper* createFileMapper(const std::string& basepath);
        std::string parse(const std::string& pathname, const std::string& password);

    public:
        Mapper(const std::string& pathname, const std::string& password);
        Mapper(std::shared_ptr<Mapper> mapper, const std::string& filename, const std::string& password);
        Mapper(ConstMemory memory, const std::string& extension, const std::string& password);
        ~Mapper();

        static bool isCustomMapper(const std::string& filename);

        const std::string& basepath() const;
        const std::string& pathname() const;

        // interface
        u64 getSize(const std::string& filename) const override;
        bool isFile(const std::string& filename) const override;
        void getIndex(FileIndex& index, const std::string& pathname) override;
        std::unique_ptr<VirtualMemory> map(const std::string& filename) override;
    };

} // namespace mango::filesystem
