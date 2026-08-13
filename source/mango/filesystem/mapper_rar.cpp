/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    RAR decompression code: Alexander L. Roshal / unRAR library.
*/
#include <map>
#include <memory>
#include <algorithm>
#include <mango/core/buffer.hpp>
#include <mango/core/string.hpp>
#include <mango/core/system.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/pointer.hpp>
#include <mango/filesystem/mapper.hpp>
#include <mango/filesystem/path.hpp>
#include "indexer.hpp"

#include "../../external/unrar/rar.hpp"

// -----------------------------------------------------------------
// UnRAR link stubs (not used by MapperRAR at runtime)
// -----------------------------------------------------------------
// Decompress-only UnRAR (SFX_MODULE/SILENT/NOVOLUME) still references UI
// symbols from errhnd, rdwrfn, find, and filefn. These no-ops satisfy the
// linker; the mapper path disables progress and never calls them.

#ifndef RARDLL
const wchar* St(MSGID StringId)
{
    return StringId;
}
#endif

void uiExtractProgress(int64 CurFileSize, int64 TotalFileSize, int64 CurSize, int64 TotalSize)
{
    MANGO_UNREFERENCED(CurFileSize);
    MANGO_UNREFERENCED(TotalFileSize);
    MANGO_UNREFERENCED(CurSize);
    MANGO_UNREFERENCED(TotalSize);
}

void uiAlarm(UIALARM_TYPE Type)
{
    MANGO_UNREFERENCED(Type);
}

void uiMsgStore::Msg()
{
}

// https://www.rarlab.com/technote.htm

namespace
{
    // -----------------------------------------------------------------
    // interface to "unrar" library to do the decompression
    // -----------------------------------------------------------------

    using namespace mango;
    using mango::Memory;
    using mango::ConstMemory;
    using mango::VirtualMemory;
    using mango::filesystem::Indexer;

    using mango::u8;
    using mango::u16;
    using mango::u32;
    using mango::u64;

    class VirtualMemoryRAR : public mango::VirtualMemory
    {
    protected:
        const u8* m_delete_address;

    public:
        VirtualMemoryRAR(const u8* address, const u8* delete_address, size_t size)
            : m_delete_address(delete_address)
        {
            m_memory = ConstMemory(address, size);
        }

        ~VirtualMemoryRAR()
        {
            delete [] m_delete_address;
        }
    };
    
    bool decompress(ComprDataIO& io, Unpack& unpack, u8* output, const u8* input,
        u64 unpacked_size, u64 packed_size, u8 unp_ver, u64 win_size, bool solid)
    {
        io.Init();
        io.EnableShowProgress(false);
        io.SetUnpackFromMemory(const_cast<byte*>(input), size_t(packed_size));
        io.SetUnpackToMemory(output, size_t(unpacked_size));
        io.SetPackedSizeToRead(packed_size);

        unpack.Init(win_size, solid);
        unpack.SetDestSize(unpacked_size);
        unpack.DoUnpack(unp_ver, solid);

        return true;
    }

    // -----------------------------------------------------------------
    // RAR unicode filename conversion code
    // -----------------------------------------------------------------

    void decodeUnicode(const u8* name, const u8* encName, size_t encSize, wchar_t* unicodeName, size_t maxDecSize)
    {
        size_t encPos = 0;
        size_t decPos = 0;
        int flagBits = 0;
        u8 flags = 0;
        u8 highByte = encName[encPos++];

        while (encPos < encSize && decPos < maxDecSize)
        {
            if (flagBits == 0)
            {
                flags = encName[encPos++];
                flagBits = 8;
            }

            switch(flags >> 6)
            {
                case 0:
                    unicodeName[decPos++] = encName[encPos++];
                    break;
                case 1:
                    unicodeName[decPos++] = static_cast<wchar_t>(encName[encPos++] + (highByte << 8));
                    break;
                case 2:
                    unicodeName[decPos++] = static_cast<wchar_t>(encName[encPos] + (encName[encPos + 1] << 8));
                    encPos += 2;
                    break;
                case 3:
                {
                    int length = encName[encPos++];
                    if (length & 0x80)
                    {
                        u8 correction = encName[encPos++];
                        for (length = (length & 0x7f) + 2; length > 0 && decPos < maxDecSize; length--, decPos++)
                        {
                            unicodeName[decPos] = static_cast<wchar_t>(((name[decPos] + correction) & 0xff) + (highByte << 8));
                        }
                    }
                    else
                    {
                        for (length += 2; length > 0 && decPos < maxDecSize; length--, decPos++)
                        {
                            unicodeName[decPos] = name[decPos];
                        }
                    }
                    break;
                }
            }

            flags <<= 2;
            flagBits -= 2;
        }

        unicodeName[decPos < maxDecSize ? decPos : maxDecSize - 1] = 0;
    }

    std::string decodeUnicodeFilename(const char* data, size_t filename_size)
    {
        constexpr size_t UNICODE_FILENAME_MAX_LENGTH = 1024;

        if (filename_size >= UNICODE_FILENAME_MAX_LENGTH)
        {
            // empty filename is used later to signify file is not present
            return "";
        }

        char buffer[UNICODE_FILENAME_MAX_LENGTH];
        std::memcpy(buffer, data, filename_size);

        size_t length;
        for (length = 0; length < filename_size; ++length)
        {
            if (!buffer[length])
                break;
        }
        buffer[length++] = 0;

        std::string s;

        if (length <= filename_size)
        {
            wchar_t temp[UNICODE_FILENAME_MAX_LENGTH];
            const u8* u = reinterpret_cast<const u8*>(buffer);
            decodeUnicode(u, u + length, filename_size - length, temp, UNICODE_FILENAME_MAX_LENGTH);
            s = mango::u16_toBytes(temp);
        }
        else
        {
            s = buffer;
        }

        return s;
    }

    // -----------------------------------------------------------------
    // RAR parsing code
    // -----------------------------------------------------------------

    struct Header
    {
        // common
        u16  crc;
        u8   type;
        u16  flags;
        u16  size;

        // type: FILE_HEAD
        u64  packed_size;
        u64  unpacked_size;
        u32  file_crc;
        u8   version;
        u8   method;
        std::string filename;
        bool is_encrypted { false };

        Header(const u8* address)
        {
            mango::LittleEndianConstPointer p = address;

            crc   = p.read16();
            type  = p.read8();
            flags = p.read16();
            size  = p.read16();

            if (flags & SKIP_IF_UNKNOWN)
            {
                return;
            }

            if (flags & LONG_BLOCK && type != HEAD3_FILE)
            {
                size = u16(size + p.read32());
            }

            switch (type)
            {
                case HEAD3_MAIN:
                {
                    if (flags & 0x0200)
                    {
                        // encrypted
                        is_encrypted = true;
                    }
                    break;
                }

                case HEAD3_FILE:
                {
                    packed_size = p.read32();
                    unpacked_size = p.read32();
                    ++p; // Host OS
                    file_crc = p.read32();
                    p += 4; // FileTime
                    version = p.read8();
                    method = p.read8();
                    int filename_size = p.read16();
                    p += 4; // FileAttr

                    if (flags & LHD_LARGE)
                    {
                        // 64 bit files
                        u64 packed_high = p.read32();
                        u64 unpacked_high = p.read32();
                        packed_size |= (packed_high << 32);
                        unpacked_size |= (unpacked_high << 32);
                    }

                    const u8* us = p;
                    const char* s = reinterpret_cast<const char*>(us);
                    p += filename_size;

                    //printLine(Print::Info, "[RAR] version: {:#x}, method: {:#x}", version, method);

                    if (isSupportedVersion())
                    {
                        if (flags & LHD_UNICODE)
                        {
                            // unicode filename
                            filename = decodeUnicodeFilename(s, filename_size);
                        }
                        else
                        {
                            // ascii filename
                            filename = std::string(s, filename_size);
                        }

                        //printLine(Print::Info, "  Filename: {}", filename);
                        std::replace(filename.begin(), filename.end(), '\\', '/');
                    }

                    if (flags & LHD_SALT)
                    {
                        // encryption salt is present
                        p += 8;
                    }

                    if (flags & LHD_EXTTIME)
                    {
                        p += 2;
                    }

                    break;
                }

                case HEAD3_MARK:
                case HEAD3_CMT:
                case HEAD3_AV:
                case HEAD3_OLDSERVICE:
                case HEAD3_PROTECT:
                case HEAD3_SIGN:
                case HEAD3_SERVICE:
                case HEAD3_ENDARC:
                    break;
            }
        }

        ~Header()
        {
        }

        bool isSupportedVersion() const
        {
            return method >= 0x30 && method <= 0x35 && version <= 36;
        }
    };

    struct RarEntry
    {
        u64 packed_size;
        u64 unpacked_size;
        u32 crc;
        u8  unp_ver;
        u8  method;
        bool is_rar5;
        bool solid_continue;
        u64 win_size;
        size_t index;
        std::string filename;

        bool folder;
        const u8* data;

        bool compressed() const
        {
            if (is_rar5)
            {
                return method != 0;
            }
            return method != 0x30;
        }
    };

    size_t solidGroupStart(const std::vector<RarEntry>& files, size_t file_index)
    {
        size_t start = file_index;

        while (start > 0 && files[start].solid_continue)
        {
            --start;
        }

        return start;
    }

} // namespace

namespace mango::filesystem
{

    // -----------------------------------------------------------------
    // MapperRAR
    // -----------------------------------------------------------------

    class MapperRAR : public AbstractMapper
    {
    public:
        std::string m_password;
        std::vector<RarEntry> m_files;
        Indexer<RarEntry> m_folders;
        bool is_encrypted { false };

        MapperRAR(ConstMemory parent, const std::string& password)
            : m_password(password)
        {
            if (parent.address)
            {
                const u8* ptr = parent.address;
                const u8* end = parent.end();

                const u8 rar4_signature[] = { 0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00 };
                const u8 rar5_signature[] = { 0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x01, 0x00 };

                if (!std::memcmp(ptr, rar4_signature, 7))
                {
                    // RAR 4.x
                    //printLine(Print::Info, "[RAR] Signature: 4");
                    parse_rar4(ptr + 7, end);
                }
                else if (!std::memcmp(ptr, rar5_signature, 8))
                {
                    // RAR 5.0
                    //printLine(Print::Info, "[RAR] Signature: 4");
                    parse_rar5(ptr + 8, end);
                }
                else
                {
                    // Incorrect signature
                    //printLine(Print::Info, "[RAR] Incorrect signature.");
                }

                for (size_t i = 0; i < m_files.size(); ++i)
                {
                    m_files[i].index = i;
                }

                for (auto& header : m_files)
                {
                    std::string filename = header.filename;
                    bool is_leaf = true;

                    while (!filename.empty())
                    {
                        std::string folder = getPath(filename.substr(0, filename.length() - 1));

                        RarEntry entry = header;
                        entry.filename = filename.substr(folder.length());
                        if (!is_leaf)
                        {
                            entry.folder = true;
                        }

                        m_folders.insert(folder, filename, entry);
                        is_leaf = false;
                        filename = folder;
                    }
                }
            }
        }

        ~MapperRAR()
        {
        }

        void parse_rar4(const u8* start, const u8* end)
        {
            const u8* p = start;

            for ( ; p < end; )
            {
                const u8* h = p;
                Header header(p);
                p = h + header.size;

                is_encrypted = header.is_encrypted;

                switch (header.type)
                {
                    case HEAD3_FILE:
                    {
                        if (header.isSupportedVersion())
                        {
                            if (!header.filename.empty())
                            {
                                RarEntry file;

                                file.packed_size = header.packed_size;
                                file.unpacked_size = header.unpacked_size;
                                file.crc = header.file_crc;
                                file.unp_ver = header.version;
                                file.method  = header.method;
                                file.is_rar5 = false;
                                file.solid_continue = (header.flags & LHD_SOLID) != 0;
                                file.win_size = 0;

                                int dict_flags = (header.flags >> 5) & 7;
                                file.folder = (dict_flags == 7);
                                if (!file.folder)
                                {
                                    file.win_size = 0x10000ULL << ((header.flags & LHD_WINDOWMASK) >> 5);
                                }
                                file.data = p;

                                file.filename = header.filename;
                                if (file.folder)
                                {
                                    file.filename += "/";
                                }

                                m_files.push_back(file);
                            }
                        }
                        else
                        {
                            // ignore file (unsupported compression -or- incorrect filename)
                        }

                        // skip compressed data
                        p += header.packed_size;

                        break;
                    }

                    case HEAD3_ENDARC:
                    {
                        p = end; // terminate parsing
                        break;
                    }
                }
            }
        }

        u64 vint(mango::LittleEndianConstPointer& p)
        {
            u64 value = 0;
            int shift = 0;
            for (int i = 0; i < 10; ++i)
            {
                u8 sample = *p++;
                value |= ((sample & 0x7f) << shift);
                shift += 7;
                if ((sample & 0x80) != 0x80)
                    break;
            }
            return value;
        }

        void parse_rar5_file_header(mango::LittleEndianConstPointer p, ConstMemory compressed_data)
        {
            u64 flags = vint(p);
            u64 unpacked_size = vint(p);
            u64 attributes = vint(p);

            u32 mtime = 0;
            u32 crc = 0;

            if (flags & 2)
            {
                mtime = p.read32();
            }
            
            if (flags & 4)
            {
                crc = p.read32();
            }

            u64 compression = vint(p);
            u64 host_os = vint(p);
            u64 length = vint(p);

            MANGO_UNREFERENCED(attributes);
            MANGO_UNREFERENCED(mtime);
            MANGO_UNREFERENCED(host_os);

            bool is_directory = (flags & 1) != 0;

            u32 comp_info = u32(compression);
            u32 unp_subver = comp_info & 0x3f;
            u32 method = (comp_info >> 7) & 7;
            bool solid_continue = (comp_info & 0x40) != 0;

            if (flags & 8)
            {
                // unpacked_size is undefined
                return;
            }

            if (!compressed_data.size && !is_directory && method != 0)
            {
                // empty non-directory files are not supported
                return;
            }

            u8 unp_ver = 0;
            u64 win_size = 0;

            if (unp_subver == 0)
            {
                unp_ver = VER_PACK5;
            }
            else if (unp_subver == 1)
            {
                unp_ver = VER_PACK7;
            }
            else
            {
                return;
            }

            if (!is_directory && unp_subver <= 1)
            {
                win_size = 0x20000ULL << ((comp_info >> 10) & (unp_subver == 0 ? 0x0f : 0x1f));
                if (unp_subver == 1)
                {
                    win_size += win_size / 32 * ((comp_info >> 15) & 0x1f);
                }
            }

            // read filename
            const u8* ptr = p;
            const char* s = reinterpret_cast<const char *>(ptr);
            std::string filename(s, int(length));

            //printf("  %s%s [algorithm: %d, solid: %d, method: %d]\n", 
            //    filename.c_str(), is_directory ? "/" : "", algorithm, is_solid, method);

            RarEntry file;

            file.packed_size = compressed_data.size;
            file.unpacked_size = unpacked_size;
            file.crc = crc;
            file.unp_ver = unp_ver;
            file.method  = u8(method);
            file.is_rar5 = true;
            file.solid_continue = solid_continue;
            file.win_size = win_size;

            file.folder = is_directory;
            file.data = compressed_data.address;

            file.filename = filename;
            if (file.folder)
            {
                file.filename += "/";
            }

            m_files.push_back(file);
        }

        void parse_rar5(const u8* start, const u8* end)
        {
            mango::LittleEndianConstPointer p = start;

            for ( ; p < end; )
            {
                u32 crc = p.read32();
                u64 header_size = vint(p);
                const u8* base = p;

                u32 type = u32(vint(p));
                u32 flags = u32(vint(p));

                u64 extra_size = 0;
                u64 data_size = 0;

                if (flags & 1)
                {
                    extra_size = vint(p);
                }

                if (flags & 2)
                {
                    data_size = vint(p);
                }

                ConstMemory compressed_data(base + header_size, size_t(data_size));

                //printf("crc: %.8x, type: %x, flags: %x, header: %x, extra: %x, data: %x\n", 
                //    crc, type, flags, (int)header_size, (int)extra_size, (int)data_size);

                MANGO_UNREFERENCED(crc);
                MANGO_UNREFERENCED(extra_size);

                // MANGO TODO: add support for AES decryption headers
                // MANGO TODO: add support for RAR 5.0 compression

                switch (type)
                {
                    case 1:
                        // Main archive header
                        break;
                    case 2:
                        // File header
                        parse_rar5_file_header(p, compressed_data);
                        break;
                    case 3:
                        // Service header
                        break;
                    case 4:
                        // Archive encryption header
                        is_encrypted = true;
                        break;
                    case 5:
                        // End of archive header
                        break;
                }

                p = base + header_size + data_size;
            }
        }

        u64 getSize(const std::string& filename) const override
        {
            const RarEntry* ptrHeader = m_folders.getHeader(filename);
            if (ptrHeader)
            {
                return ptrHeader->unpacked_size;
            }

            return 0;
        }

        bool isFile(const std::string& filename) const override
        {
            const RarEntry* ptrHeader = m_folders.getHeader(filename);
            if (ptrHeader)
            {
                return !ptrHeader->folder;
            }

            return false;
        }

        void getIndex(FileIndex& index, const std::string& pathname) override
        {
            const Indexer<RarEntry>::Folder* ptrFolder = m_folders.getFolder(pathname);
            if (ptrFolder)
            {
                for (auto i : ptrFolder->headers)
                {
                    const RarEntry& header = *i.second;

                    u32 flags = 0;
                    u64 size = header.unpacked_size;

                    if (header.folder)
                    {
                        flags |= FileInfo::Directory;
                        size = 0;
                    }

                    if (header.compressed())
                    {
                        flags |= FileInfo::Compressed;
                    }

                    if (is_encrypted)
                    {
                        flags |= FileInfo::Encrypted;
                    }

                    index.emplace(header.filename, size, flags);
                }
            }
        }

        std::unique_ptr<VirtualMemory> mapFile(size_t file_index) const
        {
            const RarEntry& file = m_files[file_index];

            if (file.folder)
            {
                MANGO_EXCEPTION("[mapper.rar] Cannot map directory \"{}\".", file.filename);
            }

            if (!file.compressed())
            {
                return std::make_unique<VirtualMemoryRAR>(
                    file.data, nullptr, size_t(file.unpacked_size));
            }

            const size_t group_start = solidGroupStart(m_files, file_index);

            ComprDataIO io;
            Unpack unpack(&io);
            Buffer scratch;

            for (size_t i = group_start; i <= file_index; ++i)
            {
                const RarEntry& current = m_files[i];

                if (!current.compressed())
                {
                    if (i == file_index)
                    {
                        return std::make_unique<VirtualMemoryRAR>(
                            current.data, nullptr, size_t(current.unpacked_size));
                    }
                    continue;
                }

                u8* buffer = nullptr;
                std::unique_ptr<u8[]> owned;

                if (i == file_index)
                {
                    owned = std::make_unique<u8[]>(size_t(current.unpacked_size));
                    buffer = owned.get();
                }
                else
                {
                    scratch.resize(size_t(current.unpacked_size));
                    buffer = scratch.data();
                }

                if (!decompress(io, unpack, buffer, current.data, current.unpacked_size,
                    current.packed_size, current.unp_ver, current.win_size, current.solid_continue))
                {
                    MANGO_EXCEPTION("[mapper.rar] Decompression failed.");
                }

                if (i == file_index)
                {
                    u8* memory = owned.release();
                    return std::make_unique<VirtualMemoryRAR>(
                        memory, memory, size_t(current.unpacked_size));
                }
            }

            MANGO_EXCEPTION("[mapper.rar] Decompression failed.");
        }

        std::unique_ptr<VirtualMemory> map(const std::string& filename) override
        {
            const RarEntry* ptrHeader = m_folders.getHeader(filename);
            if (!ptrHeader)
            {
                MANGO_EXCEPTION("[mapper.rar] File \"{}\" not found.", filename);
            }

            return mapFile(ptrHeader->index);
        }
    };

    // -----------------------------------------------------------------
    // functions
    // -----------------------------------------------------------------

    AbstractMapper* createMapperRAR(ConstMemory parent, const std::string& password)
    {
        AbstractMapper* mapper = new MapperRAR(parent, password);
        return mapper;
    }

} // namespace mango::filesystem
