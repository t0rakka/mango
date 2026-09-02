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

// Must match COMPILE_DEFINITIONS used for source/external/unrar/*.cpp
// (SFX_MODULE;SILENT;NOVOLUME). Otherwise rar.hpp enables RAR_SMP here while
// unpack.cpp was built without it — Unpack layout / inline SetDestSize diverge
// and RAR5 decode silently corrupts on MSVC (Linux/macOS can appear fine).
#ifndef SFX_MODULE
#define SFX_MODULE
#endif
#ifndef SILENT
#define SILENT
#endif
#ifndef NOVOLUME
#define NOVOLUME
#endif

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
    
    std::wstring passwordToWide(const std::string& password)
    {
        const std::u16string utf16 = utf16_from_utf8(password);
        return std::wstring(utf16.begin(), utf16.end());
    }

    bool setupDecryption(ComprDataIO& io, CRYPT_METHOD method, const u8* salt, const u8* init_v,
        u32 lg2_count, const u8* expected_psw_check, bool use_psw_check, const std::string& password)
    {
        if (password.empty())
        {
            return false;
        }

        const std::wstring wide = passwordToWide(password);
        SecPassword sec;
        sec.Set(wide.c_str());

        byte hash_key[SHA256_DIGEST_SIZE];
        byte psw_check[SIZE_PSWCHECK];

        if (method == CRYPT_RAR50)
        {
            if (!io.SetEncryption(false, CRYPT_RAR50, &sec, salt, init_v, lg2_count, hash_key, psw_check))
            {
                return false;
            }

            if (use_psw_check && std::memcmp(psw_check, expected_psw_check, SIZE_PSWCHECK) != 0)
            {
                return false;
            }
        }
        else if (method == CRYPT_RAR30)
        {
            if (!io.SetEncryption(false, CRYPT_RAR30, &sec, salt, nullptr, 0, nullptr, nullptr))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

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
        bool has_salt { false };
        u8   salt[SIZE_SALT30] {};

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
                    if (flags & MHD_PASSWORD)
                    {
                        // encrypted headers / password protected archive
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

                        std::replace(filename.begin(), filename.end(), '\\', '/');
                    }

                    if (flags & LHD_SALT)
                    {
                        std::memcpy(salt, p, SIZE_SALT30);
                        p += SIZE_SALT30;
                        has_salt = true;
                    }

                    if (flags & LHD_PASSWORD)
                    {
                        is_encrypted = true;
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

        bool encrypted { false };
        CRYPT_METHOD crypt_method { CRYPT_NONE };
        u8 salt[SIZE_SALT50] {};
        u8 init_v[SIZE_INITV] {};
        u8 psw_check[SIZE_PSWCHECK] {};
        u32 lg2_count { 0 };
        bool use_psw_check { false };

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
        bool m_headers_encrypted { false };

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

                is_encrypted = is_encrypted || header.is_encrypted;

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

                                if (header.is_encrypted)
                                {
                                    file.encrypted = true;
                                    file.crypt_method = CRYPT_RAR30;
                                    if (header.has_salt)
                                    {
                                        std::memcpy(file.salt, header.salt, SIZE_SALT30);
                                    }
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

        void parse_rar5_extra_crypt(mango::LittleEndianConstPointer& p, const u8* rec_end, RarEntry& file)
        {
            const u64 version = vint(p);
            const u64 flags = vint(p);
            MANGO_UNREFERENCED(version);

            if (p >= rec_end)
            {
                return;
            }

            file.lg2_count = *p++;
            if (size_t(rec_end - p) < SIZE_SALT50 + SIZE_INITV)
            {
                return;
            }

            std::memcpy(file.salt, p, SIZE_SALT50);
            p += SIZE_SALT50;
            std::memcpy(file.init_v, p, SIZE_INITV);
            p += SIZE_INITV;

            if (flags & FHEXTRA_CRYPT_PSWCHECK)
            {
                if (size_t(rec_end - p) < SIZE_PSWCHECK + SIZE_PSWCHECK_CSUM)
                {
                    return;
                }

                std::memcpy(file.psw_check, p, SIZE_PSWCHECK);
                p += SIZE_PSWCHECK;
                p += SIZE_PSWCHECK_CSUM; // integrity of check value; unused here
                file.use_psw_check = true;
            }

            file.encrypted = true;
            file.crypt_method = CRYPT_RAR50;
            is_encrypted = true;
        }

        void parse_rar5_extra(ConstMemory extra, RarEntry& file)
        {
            if (!extra.address || !extra.size)
            {
                return;
            }

            mango::LittleEndianConstPointer p = extra.address;
            const u8* end = extra.end();

            while (p < end)
            {
                const u64 rec_size = vint(p);
                if (rec_size == 0 || p + rec_size > end)
                {
                    break;
                }

                const u8* rec_end = p + rec_size;
                const u64 type = vint(p);

                if (type == FHEXTRA_CRYPT)
                {
                    parse_rar5_extra_crypt(p, rec_end, file);
                }

                p = rec_end;
            }
        }

        void parse_rar5_file_header(mango::LittleEndianConstPointer p, ConstMemory compressed_data, ConstMemory extra)
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

            parse_rar5_extra(extra, file);

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

                if (flags & HFL_EXTRA)
                {
                    extra_size = vint(p);
                }

                if (flags & HFL_DATA)
                {
                    data_size = vint(p);
                }

                ConstMemory compressed_data(base + header_size, size_t(data_size));
                ConstMemory extra;
                if (extra_size && extra_size <= header_size)
                {
                    extra = ConstMemory(base + header_size - size_t(extra_size), size_t(extra_size));
                }

                MANGO_UNREFERENCED(crc);

                switch (type)
                {
                    case HEAD_MAIN:
                        // Main archive header
                        break;
                    case HEAD_FILE:
                        // File header
                        parse_rar5_file_header(p, compressed_data, extra);
                        break;
                    case HEAD_SERVICE:
                        // Service header
                        break;
                    case HEAD_CRYPT:
                        // Archive encryption header (-hp): subsequent headers are encrypted.
                        // Per-file encryption (-p) uses FHEXTRA_CRYPT and is supported.
                        is_encrypted = true;
                        m_headers_encrypted = true;
                        p = end;
                        continue;
                    case HEAD_ENDARC:
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

                    if (is_encrypted || header.encrypted)
                    {
                        flags |= FileInfo::Encrypted;
                    }

                    index.emplace(header.filename, size, flags);
                }
            }
        }

        std::unique_ptr<VirtualMemory> mapFile(size_t file_index) const
        {
            if (m_headers_encrypted)
            {
                MANGO_EXCEPTION("[mapper.rar] Archive headers are encrypted (-hp); not supported.");
            }

            const RarEntry& file = m_files[file_index];

            if (file.folder)
            {
                MANGO_EXCEPTION("[mapper.rar] Cannot map directory \"{}\".", file.filename);
            }

            if (file.encrypted && m_password.empty())
            {
                MANGO_EXCEPTION("[mapper.rar] File \"{}\" is encrypted; password required.", file.filename);
            }

            if (!file.compressed() && !file.encrypted)
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

                if (!current.compressed() && !current.encrypted)
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

                io.Init();
                io.EnableShowProgress(false);

                if (current.encrypted)
                {
                    if (!setupDecryption(io, current.crypt_method, current.salt, current.init_v,
                        current.lg2_count, current.psw_check, current.use_psw_check, m_password))
                    {
                        MANGO_EXCEPTION("[mapper.rar] Incorrect password for \"{}\".", current.filename);
                    }
                }

                io.SetUnpackFromMemory(const_cast<byte*>(current.data), size_t(current.packed_size));
                io.SetUnpackToMemory(buffer, size_t(current.unpacked_size));
                io.SetPackedSizeToRead(current.packed_size);

                unpack.Init(current.win_size, current.solid_continue);
                unpack.SetDestSize(current.unpacked_size);
                unpack.DoUnpack(current.unp_ver, current.solid_continue);

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
            if (m_headers_encrypted)
            {
                MANGO_EXCEPTION("[mapper.rar] Archive headers are encrypted (-hp); not supported.");
            }

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
