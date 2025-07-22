/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <algorithm>
#include <mango/core/string.hpp>
#include <mango/core/system.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/pointer.hpp>
#include <mango/filesystem/mapper.hpp>
#include <mango/filesystem/path.hpp>

// TODO: Check record flags why .img files in [BOOT] directory are not visible
// TODO: Check that we add containers (.zip, rar, etc.) as files AND containers into the index
// TODO: Implement proper Joliet/Rock Ridge support for lowercase and long filenames

namespace
{
    using namespace mango;

    using mango::Memory;
    using mango::ConstMemory;
    using mango::VirtualMemory;

    using mango::u8;
    using mango::u16;
    using mango::u32;
    using mango::u64;

    // -----------------------------------------------------------------
    // ISO 9660 structures
    // -----------------------------------------------------------------

    // Volume Descriptor Types
    enum VolumeDescriptorType : u8
    {
        BOOT_RECORD = 0,
        PRIMARY_VOLUME_DESCRIPTOR = 1,
        SUPPLEMENTARY_VOLUME_DESCRIPTOR = 2,
        VOLUME_PARTITION_DESCRIPTOR = 3,
        VOLUME_DESCRIPTOR_SET_TERMINATOR = 255
    };

    // Directory Record Flags
    enum DirectoryRecordFlags : u8
    {
        HIDDEN = 0x01,
        DIRECTORY = 0x02,
        ASSOCIATED_FILE = 0x04,
        EXTENDED_ATTRIBUTES = 0x08,
        OWNER_PERMISSIONS = 0x10,
        GROUP_PERMISSIONS = 0x20,
        SYSTEM_USE = 0x40,
        MULTI_EXTENT = 0x80
    };

    // Volume Descriptor Header
    struct VolumeDescriptorHeader
    {
        u8 type;
        char identifier[5];  // "CD001"
        u8 version;

        bool isValid() const
        {
            return std::memcmp(identifier, "CD001", 5) == 0 && version == 1;
        }
    };

    // Primary Volume Descriptor
    struct PrimaryVolumeDescriptor
    {
        VolumeDescriptorHeader header;
        u8 unused1[1];
        char system_identifier[32];
        char volume_identifier[32];
        u8 unused2[8];
        u32 volume_space_size_lsb;
        u32 volume_space_size_msb;
        u8 unused3[32];
        u16 volume_set_size_lsb;
        u16 volume_set_size_msb;
        u16 volume_sequence_number_lsb;
        u16 volume_sequence_number_msb;
        u16 logical_block_size_lsb;
        u16 logical_block_size_msb;
        u32 path_table_size_lsb;
        u32 path_table_size_msb;
        u32 path_table_location_lsb;
        u32 path_table_location_msb;
        u32 path_table_location_opt_lsb;
        u32 path_table_location_opt_msb;
        u8 root_directory_record[34];
        char volume_set_identifier[128];
        char publisher_identifier[128];
        char data_preparer_identifier[128];
        char application_identifier[128];
        char copyright_file_identifier[37];
        char abstract_file_identifier[37];
        char bibliographic_file_identifier[37];
        char creation_date[17];
        char modification_date[17];
        char expiration_date[17];
        char effective_date[17];
        u8 file_structure_version;
        u8 unused4[1];
        u8 application_use[512];
        u8 reserved[653];
    };

    // Supplementary Volume Descriptor (Joliet)
    struct SupplementaryVolumeDescriptor
    {
        VolumeDescriptorHeader header;
        u8 unused1[1];
        char system_identifier[32];
        char volume_identifier[32];
        u8 unused2[8];
        u32 volume_space_size_lsb;
        u32 volume_space_size_msb;
        u8 unused3[32];
        u16 volume_set_size_lsb;
        u16 volume_set_size_msb;
        u16 volume_sequence_number_lsb;
        u16 volume_sequence_number_msb;
        u16 logical_block_size_lsb;
        u16 logical_block_size_msb;
        u32 path_table_size_lsb;
        u32 path_table_size_msb;
        u32 path_table_location_lsb;
        u32 path_table_location_msb;
        u32 path_table_location_opt_lsb;
        u32 path_table_location_opt_msb;
        u8 root_directory_record[34];
        char volume_set_identifier[128];
        char publisher_identifier[128];
        char data_preparer_identifier[128];
        char application_identifier[128];
        char copyright_file_identifier[37];
        char abstract_file_identifier[37];
        char bibliographic_file_identifier[37];
        char creation_date[17];
        char modification_date[17];
        char expiration_date[17];
        char effective_date[17];
        u8 file_structure_version;
        u8 unused4[1];
        u8 application_use[512];
        u8 reserved[653];
    };

    struct DirectoryRecord
    {
        u8 length;
        u8 extended_attribute_length;
        u32 extent_location_lsb;
        u32 extent_location_msb;
        u32 data_length_lsb;
        u32 data_length_msb;
        char date[7];
        u8 flags;
        u8 file_unit_size;
        u8 interleave_gap_size;
        u16 volume_sequence_number_lsb;
        u16 volume_sequence_number_msb;
        u8 file_identifier_length;
        char file_identifier[1]; // Variable length, minimum 1

        u32 getExtentLocation() const
        {
            // Manual reading to avoid structure alignment issues
            const u8* data = reinterpret_cast<const u8*>(this);
            return data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
        }

        u32 getDataLength() const
        {
            // Manual reading to avoid structure alignment issues
            const u8* data = reinterpret_cast<const u8*>(this);
            return data[10] | (data[11] << 8) | (data[12] << 16) | (data[13] << 24);
        }

        bool isDirectory() const
        {
            // Manual reading to avoid structure alignment issues
            const u8* data = reinterpret_cast<const u8*>(this);
            return (data[25] & DIRECTORY) != 0;
        }

        u8 getFlags() const
        {
            // Manual reading to avoid structure alignment issues
            const u8* data = reinterpret_cast<const u8*>(this);
            return data[25];
        }

        std::string getFileName() const
        {
            // Manual reading to avoid structure alignment issues
            const u8* data = reinterpret_cast<const u8*>(this);
            u8 filename_length = data[32];
            
            if (filename_length == 0)
                return "";

            std::string name(reinterpret_cast<const char*>(data + 33), filename_length);

            // Remove version suffix (;1, ;2, etc.)
            size_t pos = name.find(';');
            if (pos != std::string::npos)
            {
                name = name.substr(0, pos);
            }

            return name;
        }

        std::string getJolietFileName() const
        {
            // Manual reading to avoid structure alignment issues
            const u8* data = reinterpret_cast<const u8*>(this);
            u8 filename_length = data[32];

            if (filename_length == 0)
                return "";

            // Joliet uses UCS-2 (16-bit Unicode), but we'll read as UTF-8 for simplicity
            // In practice, most Joliet filenames are ASCII-compatible
            std::string name(reinterpret_cast<const char*>(data + 33), filename_length);

            // Remove version suffix (;1, ;2, etc.)
            size_t pos = name.find(';');
            if (pos != std::string::npos)
            {
                name = name.substr(0, pos);
            }

            return name;
        }

        std::string getRockRidgeFileName() const
        {
            // Manual reading to avoid structure alignment issues
            const u8* data = reinterpret_cast<const u8*>(this);
            u8 filename_length = data[32];
            u8 system_use_length = data[33 + filename_length];

            if (system_use_length == 0)
                return "";

            const u8* system_use = data + 33 + filename_length;

            // Parse Rock Ridge System Use area
            for (u8 i = 0; i < system_use_length - 3; ++i)
            {
                u8 signature1 = system_use[i];
                u8 signature2 = system_use[i + 1];
                u8 length = system_use[i + 2];
                u8 version = system_use[i + 3];

                // Look for NM (Name) records - these are Rock Ridge extensions
                if (signature1 == 'N' && signature2 == 'M')
                {
                    // NM record - Name field
                    if (length >= 5 && version == 1)
                    {
                        u8 flags = system_use[i + 4];
                        u8 name_length = length - 5;

                        if (name_length > 0 && (i + 5 + name_length) <= system_use_length)
                        {
                            std::string name(reinterpret_cast<const char*>(system_use + i + 5), name_length);

                            // Check for continuation flag
                            if (flags & 0x01)
                            {
                                // This is a continuation, look for more NM records
                                // For now, just return the first part
                            }

                            return name;
                        }
                    }
                }

                // Skip to next record
                if (length > 0 && length <= system_use_length - i)
                    i += length - 1;
                else
                    break; // Prevent infinite loop
            }

            return "";
        }
    };

    // -----------------------------------------------------------------
    // VirtualMemoryISO
    // -----------------------------------------------------------------

    class VirtualMemoryISO : public mango::VirtualMemory
    {
    protected:
        const u8* m_delete_address;

    public:
        VirtualMemoryISO(const u8* address, const u8* delete_address, size_t size)
            : m_delete_address(delete_address)
        {
            m_memory = ConstMemory(address, size);
        }

        ~VirtualMemoryISO()
        {
            delete[] m_delete_address;
        }
    };

    // -----------------------------------------------------------------
    // File Entry for direct traversal
    // -----------------------------------------------------------------

    struct FileEntry
    {
        u32 extent_location;
        u32 data_length;
        std::string filename;
        bool is_directory;

        FileEntry(u32 location, u32 length, const std::string& name, bool directory)
            : extent_location(location)
            , data_length(length)
            , filename(name)
            , is_directory(directory)
        {
        }

        std::unique_ptr<mango::VirtualMemory> map(const u8* iso_data, u32 logical_block_size) const
        {
            if (is_directory || data_length == 0)
            {
                return nullptr;
            }

            const u8* address = iso_data + (extent_location * logical_block_size);
            const u8* delete_address = nullptr;
            size_t size = data_length;

            // For files that don't start at block boundary, we need to copy
            if (data_length < logical_block_size)
            {
                u8* buffer = new u8[size];
                std::memcpy(buffer, address, size);
                address = buffer;
                delete_address = buffer;
            }

            return std::make_unique<VirtualMemoryISO>(address, delete_address, size);
        }
    };

    // -----------------------------------------------------------------
    // MapperISODirect
    // -----------------------------------------------------------------

    class MapperISO : public mango::filesystem::AbstractMapper
    {
    protected:
        ConstMemory m_parent_memory;
        u32 m_logical_block_size;
        u32 m_root_extent;
        u32 m_root_length;

        // Joliet support
        bool m_has_joliet;
        u32 m_joliet_root_extent;
        u32 m_joliet_root_length;

        mutable std::map<std::string, std::vector<FileEntry>> m_directory_cache;

        bool checkForRockRidge(const u8* dir_data, u32 data_length) const
        {
            const u8* ptr = dir_data;
            const u8* end = dir_data + data_length;

            while (ptr < end)
            {
                if (ptr + 1 > end)
                    break;

                u8 record_length = ptr[0];
                if (record_length == 0)
                {
                    ptr = dir_data + ((ptr - dir_data + 2047) & ~2047);
                    continue;
                }

                if (ptr + record_length > end)
                    break;

                // Check for Rock Ridge extensions in System Use area
                const u8* data = ptr;
                u8 filename_length = data[32];
                u8 system_use_length = data[33 + filename_length];

                if (system_use_length > 0)
                {
                    const u8* system_use = data + 33 + filename_length;

                    // Look for Rock Ridge signature "RR" or "SP"
                    for (u8 i = 0; i < system_use_length - 1; ++i)
                    {
                        if ((system_use[i] == 'R' && system_use[i + 1] == 'R') ||
                            (system_use[i] == 'S' && system_use[i + 1] == 'P'))
                        {
                            return true;
                        }
                    }
                }

                ptr += record_length;
            }
            return false;
        }

        void parseVolumeDescriptors()
        {
            const u8* ptr = m_parent_memory.address;
            const u8* end = m_parent_memory.end();

            // ISO files start at sector 16 (0x8000 bytes)
            const u8* volume_start = ptr + 0x8000;

            m_has_joliet = false;
            bool has_primary = false;
            bool has_joliet = false;
            bool has_rock_ridge = false;
            int iso_level = 0;

            for (int i = 0; i < 100; ++i) // Limit to prevent infinite loop
            {
                const VolumeDescriptorHeader* header = reinterpret_cast<const VolumeDescriptorHeader*>(volume_start + i * 2048);

                if (header->type == VOLUME_DESCRIPTOR_SET_TERMINATOR)
                {
                    break;
                }

                if (header->type == PRIMARY_VOLUME_DESCRIPTOR && header->isValid())
                {
                    has_primary = true;
                    const PrimaryVolumeDescriptor* pvd = reinterpret_cast<const PrimaryVolumeDescriptor*>(header);
                    m_logical_block_size = pvd->logical_block_size_lsb;

                    // Extract root directory location and size
                    const u8* root_record = pvd->root_directory_record;
                    m_root_extent = root_record[2] | (root_record[3] << 8) | (root_record[4] << 16) | (root_record[5] << 24);
                    m_root_length = root_record[10] | (root_record[11] << 8) | (root_record[12] << 16) | (root_record[13] << 24);

                    // Check ISO level from file structure version
                    iso_level = pvd->file_structure_version;
                }
                else if (header->type == SUPPLEMENTARY_VOLUME_DESCRIPTOR && header->isValid())
                {
                    const SupplementaryVolumeDescriptor* svd = reinterpret_cast<const SupplementaryVolumeDescriptor*>(header);

                    // Check if this is a Joliet volume (system identifier should contain "JOLIET")
                    std::string system_id(svd->system_identifier, 32);
                    if (system_id.find("JOLIET") != std::string::npos)
                    {
                        has_joliet = true;
                        m_has_joliet = true;

                        // Extract Joliet root directory location and size
                        const u8* root_record = svd->root_directory_record;
                        m_joliet_root_extent = root_record[2] | (root_record[3] << 8) | (root_record[4] << 16) | (root_record[5] << 24);
                        m_joliet_root_length = root_record[10] | (root_record[11] << 8) | (root_record[12] << 16) | (root_record[13] << 24);
                    }
                }
            }

            // Check for Rock Ridge extensions by examining the root directory
            if (has_primary)
            {
                const u8* root_dir_data = m_parent_memory.address + (m_root_extent * m_logical_block_size);
                has_rock_ridge = checkForRockRidge(root_dir_data, m_root_length);
            }

            // Report ISO extensions
#if 0
            printLine("ISO Extensions detected:");
            printLine("  - Primary Volume Descriptor: {}", has_primary ? "YES" : "NO");
            printLine("  - ISO 9660 Level: {}", iso_level);
            printLine("  - Joliet (Unicode filenames): {}", has_joliet ? "YES" : "NO");
            printLine("  - Rock Ridge (Unix attributes): {}", has_rock_ridge ? "YES" : "NO");
#endif
        }

        std::vector<FileEntry> parseDirectoryContents(const u8* dir_data, u32 data_length, bool use_joliet = false) const
        {
            std::vector<FileEntry> entries;
            const u8* ptr = dir_data;
            const u8* end = dir_data + data_length;

            while (ptr < end)
            {
                // Bounds checking for record
                if (ptr + 1 > end)
                {
                    break;
                }

                const DirectoryRecord* record = reinterpret_cast<const DirectoryRecord*>(ptr);
                u8 record_length = ptr[0];

                // Check record length
                if (record_length == 0)
                {
                    // Skip to next sector boundary
                    ptr = dir_data + ((ptr - dir_data + 2047) & ~2047);
                    continue;
                }

                // Bounds checking for full record
                if (ptr + record_length > end)
                {
                    break;
                }

                u32 extent_location = record->getExtentLocation();
                u32 data_length = record->getDataLength();
                bool is_directory = record->isDirectory();

                std::string filename;
                if (use_joliet)
                {
                    filename = record->getJolietFileName();
                }
                else
                {
                    // Try Rock Ridge first, fall back to standard ISO filename
                    filename = record->getRockRidgeFileName();
                    if (filename.empty())
                    {
                        filename = record->getFileName();
                    }
                }

                // Skip "." and ".." entries and empty/whitespace filenames
                if (filename == "." || filename == ".." || filename.empty() || 
                    (filename.length() == 1 && (filename[0] == ' ' || filename[0] == '\0' || filename[0] == 1)))
                {
                    ptr += record_length;
                    continue;
                }

                // Safety check: if we can't advance, break to prevent infinite loop
                if (record_length == 0)
                {
                    break;
                }

                // Add trailing slash to directory names to match filesystem convention
                if (is_directory && !filename.empty() && filename.back() != '/')
                {
                    filename += "/";
                }

                entries.emplace_back(extent_location, data_length, filename, is_directory);
                ptr += record_length;
            }

            return entries;
        }

        std::vector<FileEntry> getDirectoryEntries(const std::string& pathname) const
        {
            // Check cache first
            auto it = m_directory_cache.find(pathname);
            if (it != m_directory_cache.end())
            {
                return it->second;
            }

            // Safety check to prevent infinite recursion
            static int recursion_count = 0;
            recursion_count++;
            if (recursion_count > 100)  // Back to normal limit
            {
                recursion_count = 0;
                return std::vector<FileEntry>();
            }

            std::vector<FileEntry> entries;

            if (pathname.empty() || pathname == "/" || pathname == "\\")
            {
                // Root directory - use standard ISO 9660 for now
                const u8* root_dir_data = m_parent_memory.address + (m_root_extent * m_logical_block_size);
                entries = parseDirectoryContents(root_dir_data, m_root_length, false);
            }
            else
            {
                // Handle path parsing for virtual filesystem
                // For "POOL/MAIN/", we want to find "MAIN" in the "POOL/" directory
                // Custom path parsing for container paths that end with slashes
                std::string parent_path;
                std::string dir_name;
                
                if (pathname.empty())
                {
                    parent_path = "";
                    dir_name = "";
                }
                else if (pathname == "/" || pathname == "\\")
                {
                    parent_path = "";
                    dir_name = "";
                }
                else
                {
                    // Find the second-to-last slash for multi-level paths
                    size_t last_slash = pathname.find_last_of("/\\");
                    if (last_slash != std::string::npos && last_slash > 0)
                    {
                        // Look for the previous slash
                        size_t prev_slash = pathname.find_last_of("/\\", last_slash - 1);
                        if (prev_slash != std::string::npos)
                        {
                            // Multi-level path: "POOL/MAIN/" -> parent="POOL/", dir="MAIN/"
                            parent_path = pathname.substr(0, prev_slash + 1);
                            dir_name = pathname.substr(prev_slash + 1);
                        }
                        else
                        {
                            // Single-level path: "POOL/" -> parent="", dir="POOL/"
                            parent_path = "";
                            dir_name = pathname;
                        }
                    }
                    else
                    {
                        // No slashes or only trailing slash
                        parent_path = "";
                        dir_name = pathname;
                    }
                }

                // Remove trailing slash from dir_name for lookup
                if (!dir_name.empty() && dir_name.back() == '/')
                {
                    dir_name.pop_back();
                }

                // Get parent directory entries
                auto parent_entries = getDirectoryEntries(parent_path);

                // Find the requested directory
                bool found = false;
                for (const auto& entry : parent_entries)
                {
                    // Remove trailing slash from entry name for comparison
                    std::string entry_name = entry.filename;
                    if (!entry_name.empty() && entry_name.back() == '/')
                    {
                        entry_name.pop_back();
                    }

                    if (entry_name == dir_name)
                    {
                        if (entry.is_directory)
                        {
                            const u8* dir_data = m_parent_memory.address + (entry.extent_location * m_logical_block_size);
                            
                            // Use standard ISO 9660 parsing
                            entries = parseDirectoryContents(dir_data, entry.data_length, false);
                            found = true;
                        }
                        break;
                    }
                }
            }

            // Cache the results
            m_directory_cache[pathname] = entries;
            recursion_count--;
            return entries;
        }

    public:
        MapperISO(ConstMemory parent, const std::string& password)
            : m_parent_memory(parent)
            , m_logical_block_size(2048)
            , m_root_extent(0)
            , m_root_length(0)
            , m_has_joliet(false)
            , m_joliet_root_extent(0)
            , m_joliet_root_length(0)
        {
            MANGO_UNREFERENCED(password); // ISO files don't use passwords

            if (parent.address)
            {
                // Check for ISO signature
                const u8* ptr = parent.address;

                if (parent.size >= 0x8000 + sizeof(VolumeDescriptorHeader))
                {
                    const VolumeDescriptorHeader* header = reinterpret_cast<const VolumeDescriptorHeader*>(ptr + 0x8000);
                    if (header->isValid())
                    {
                        printLine(Print::Info, "[ISO] Valid ISO 9660 image detected");
                        parseVolumeDescriptors();
                        
                        if (m_has_joliet)
                        {
                            printLine(Print::Info, "[ISO] Joliet extension detected - using long filenames");
                        }
                    }
                    else
                    {
                        printLine(Print::Info, "[ISO] Invalid ISO 9660 signature");
                    }
                }
                else
                {
                    printLine(Print::Info, "[ISO] File too small to be valid ISO");
                }
            }
        }

        ~MapperISO()
        {
        }

        u64 getSize(const std::string& filename) const override
        {
            std::string pathname = mango::filesystem::getPath(filename);
            std::string name = filename.substr(pathname.length());
            
            auto entries = getDirectoryEntries(pathname);
            for (const auto& entry : entries)
            {
                if (entry.filename == name)
                {
                    return entry.data_length;
                }
            }
            return 0;
        }

        bool isFile(const std::string& filename) const override
        {
            std::string pathname = mango::filesystem::getPath(filename);
            std::string name = filename.substr(pathname.length());
            
            auto entries = getDirectoryEntries(pathname);
            for (const auto& entry : entries)
            {
                if (entry.filename == name)
                {
                    return !entry.is_directory;
                }
            }
            return false;
        }

        void getIndex(mango::filesystem::FileIndex& index, const std::string& pathname) override
        {
            auto entries = getDirectoryEntries(pathname);

            for (const auto& entry : entries)
            {
                u32 flags = 0;
                u64 size = entry.data_length;

                if (entry.is_directory)
                {
                    flags |= mango::filesystem::FileInfo::Directory;
                    size = 0;
                }

                index.emplace(entry.filename, size, flags);
            }
        }

        std::unique_ptr<mango::VirtualMemory> map(const std::string& filename) override
        {
            std::string pathname = mango::filesystem::getPath(filename);
            std::string name = filename.substr(pathname.length());
            
            auto entries = getDirectoryEntries(pathname);
            for (const auto& entry : entries)
            {
                if (entry.filename == name)
                {
                    if (entry.is_directory)
                    {
                        MANGO_EXCEPTION("[mapper.iso] Cannot map directory \"{}\".", filename);
                    }
                    return entry.map(m_parent_memory.address, m_logical_block_size);
                }
            }

            MANGO_EXCEPTION("[mapper.iso] File \"{}\" not found.", filename);
        }
    };

} // namespace

namespace mango::filesystem
{

    AbstractMapper* createMapperISO(ConstMemory parent, const std::string& password)
    {
        AbstractMapper* mapper = new MapperISO(parent, password);
        return mapper;
    }

} // namespace mango::filesystem 
