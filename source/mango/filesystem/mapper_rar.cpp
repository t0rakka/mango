/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    RAR decompression code: Alexander L. Roshal / unRAR library.
*/
#include <map>
#include <algorithm>
#include <mango/core/string.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/pointer.hpp>
#include <mango/filesystem/mapper.hpp>
#include <mango/filesystem/path.hpp>

#ifdef MANGO_ENABLE_LICENSE_GPL

#include "../../external/unrar/rar.hpp"

#define ID ".rar mapper: "

namespace
{
    // -----------------------------------------------------------------
    // interface to "unrar" library to do the decompression
    // -----------------------------------------------------------------

    using mango::Memory;
    using mango::VirtualMemory;

    typedef unsigned char uint8;
    typedef unsigned short uint16;

    class VirtualMemoryRAR : public mango::VirtualMemory
    {
    protected:
        uint8* m_delete_address;

    public:
        VirtualMemoryRAR(uint8* address, uint8* delete_address, size_t size)
            : m_delete_address(delete_address)
        {
            m_memory = Memory(address, size);
        }

        ~VirtualMemoryRAR()
        {
            delete [] m_delete_address;
        }
    };
    
    bool decompress(uint8* output, uint8* input, uint64 unpacked_size, uint64 packed_size, uint8 version)
    {
        ComprDataIO subDataIO;
        subDataIO.Init();

        Unpack unpack(&subDataIO);
        unpack.Init();

        subDataIO.UnpackToMemory = true;
        subDataIO.UnpackToMemorySize = static_cast<size_t>(unpacked_size);
        subDataIO.UnpackToMemoryAddr = output;

        subDataIO.UnpackFromMemory = true;
        subDataIO.UnpackFromMemorySize = static_cast<size_t>(packed_size);
        subDataIO.UnpackFromMemoryAddr = input;

        subDataIO.UnpPackedSize = packed_size;
        unpack.SetDestSize(unpacked_size);

        unpack.DoUnpack(version, false);

        return true;
    }

    // -----------------------------------------------------------------
    // RAR unicode filename conversion code
    // -----------------------------------------------------------------

    void decodeUnicode(const uint8* name, const uint8* encName, int encSize, wchar_t* unicodeName, int maxDecSize)
    {
        int encPos = 0;
        int decPos = 0;
        int flagBits = 0;
        uint8 flags = 0;
        uint8 highByte = encName[encPos++];

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
                        uint8 correction = encName[encPos++];
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

    std::string decodeUnicodeFilename(const char* data, int filename_size)
    {
        if (filename_size >= 1024)
        {
            MANGO_EXCEPTION(ID"Too long unicode filename.");
        }

        char buffer[1024];
        std::memcpy(buffer, data, filename_size);

        int length;
        for (length = 0; length < filename_size; ++length)
        {
            if (!buffer[length])
                break;
        }
        buffer[length++] = 0;

        std::string s;

        if (length <= filename_size)
        {
            wchar_t temp[1024];
            const uint8* u = reinterpret_cast<const uint8*>(buffer);
            decodeUnicode(u, u + length, filename_size - length, temp, 1024);
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

    enum
    {
        MARK_HEAD    = 0x72,
        MAIN_HEAD    = 0x73,
        FILE_HEAD    = 0x74,
        COMM_HEAD    = 0x75,
        AV_HEAD      = 0x76,
        SUB_HEAD     = 0x77,
        PROTECT_HEAD = 0x78,
        SIGN_HEAD    = 0x79,
        NEWSUB_HEAD  = 0x7a,
        ENDARC_HEAD  = 0x7b
    };

    enum
    {
        MHD_VOLUME       = 0x0001,
        MHD_COMMENT      = 0x0002,
        MHD_LOCK         = 0x0004,
        MHD_SOLID        = 0x0008,
        MHD_PACK_COMMENT = 0x0010, // MHD_NEWNUMBERING
        MHD_AV           = 0x0020,
        MHD_PROTECT      = 0x0040,
        MHD_PASSWORD     = 0x0080,
        MHD_FIRSTVOLUME  = 0x0100,
        MHD_ENCRYPTVER   = 0x0200
    };

    enum
    {
        LHD_SPLIT_BEFORE = 0x0001, // TODO: not supported
        LHD_SPLIT_AFTER  = 0x0002, // TODO: not supported
        LHD_PASSWORD     = 0x0004,
        LHD_COMMENT      = 0x0008,
        LHD_SOLID        = 0x0010,
        LHD_LARGE        = 0x0100,
        LHD_UNICODE      = 0x0200,
        LHD_SALT         = 0x0400,
        LHD_VERSION      = 0x0800,
        LHD_EXTTIME      = 0x1000,
        LHD_EXTFLAGS     = 0x2000,
        LHD_SKIP_UNKNOWN = 0x4000
    };

    struct Header
    {
        // common
        uint16  crc;
        uint8   type;
        uint16  flags;
        uint16  size;

        // type: FILE_HEAD
        uint64  packed_size;
        uint64  unpacked_size;
        uint32  file_crc;
        uint8   version;
        uint8   method;
        std::string filename;

        Header(uint8* address)
        {
            mango::LittleEndianPointer p = address;

            crc   = p.read16();
            type  = p.read8();
            flags = p.read16();
            size  = p.read16();

            if (flags & LHD_SKIP_UNKNOWN)
            {
                return;
            }

            if (flags & 0x8000 && type != FILE_HEAD)
            {
			    size += p.read32();
            }

            // TODO: header CRC check

            switch (type)
            {
                case MAIN_HEAD:
                {
                    if (flags & MHD_ENCRYPTVER)
                    {
                        // encrypted
                    }
                    break;
                }

                case FILE_HEAD:
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
                        uint64 packed_high = p.read32();
                        uint64 unpacked_high = p.read32();
                        packed_size |= (packed_high << 32);
                        unpacked_size |= (unpacked_high << 32);
                    }

                    const uint8* us = p;
                    const char* s = reinterpret_cast<const char*>(us);
                    p += filename_size;

                    //printf("RAR version: %d, method: %d\n", version, method);
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
                        // encryption salt is present
                        p += 8;
                    }

                    if (flags & LHD_EXTTIME)
                    {
                        p += 2;
                    }

                    break;
                }

                case MARK_HEAD:
                case COMM_HEAD:
                case AV_HEAD:
                case SUB_HEAD:
                case PROTECT_HEAD:
                case SIGN_HEAD:
                case NEWSUB_HEAD:
                case ENDARC_HEAD:
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

    struct FileHeader
    {
        uint64  packed_size;
        uint64  unpacked_size;
        uint32  crc;
        uint8   version;
        uint8   method;
        bool    is_rar5;

        bool folder;
        uint8* data;

        bool compressed() const
        {
            if (is_rar5)
            {
                return method != 0;
            }
            return method != 0x30;
        }

        VirtualMemory* mmap()
        {
            VirtualMemory* memory;

            if (!compressed())
            {
                // no compression
                memory = new VirtualMemoryRAR(data, nullptr, size_t(unpacked_size));
            }
            else
            {
                size_t size = size_t(unpacked_size);
                uint8* buffer = new uint8[size];

                bool status = decompress(buffer, data, unpacked_size, packed_size, version);
                if (!status)
                {
                    delete[] buffer;
                    MANGO_EXCEPTION(ID"Decompression failed.");
                }

                memory = new VirtualMemoryRAR(buffer, buffer, size);
            }

            return memory;
        }
    };

} // namespace

namespace mango
{

    // -----------------------------------------------------------------
    // MapperRAR
    // -----------------------------------------------------------------

    class MapperRAR : public AbstractMapper
    {
    public:
        std::string m_password;
        std::map<std::string, FileHeader> m_files;

        MapperRAR(Memory parent, const std::string& password)
        : m_password(password)
        {
            uint8* start = parent.address;
            uint8* end = parent.address + parent.size;

            if (start)
            {
                parse(start, end);
            }
        }

        ~MapperRAR()
        {
        }

        void parse(uint8* start, uint8* end)
        {
            uint8* p = start;

            const uint8 rar4_signature[] = { 0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00 };
            const uint8 rar5_signature[] = { 0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x01, 0x00 };

            if (!std::memcmp(p, rar4_signature, 7))
            {
                // RAR 4.x
                parse_rar4(start + 7, end);
            }
            else if (!std::memcmp(p, rar5_signature, 8))
            {
                // RAR 5.0
                parse_rar5(start + 8, end);
            }
            else
            {
                MANGO_EXCEPTION(ID"Incorrect signature.");
            }
        }

        void parse_rar4(uint8* start, uint8* end)
        {
            uint8* p = start;

            for (; p < end;)
            {
                uint8* h = p;
                Header header(p);
                p = h + header.size;

                switch (header.type)
                {
                    case FILE_HEAD:
                    {
                        if (header.isSupportedVersion())
                        {
                            FileHeader file;

                            file.packed_size = header.packed_size;
                            file.unpacked_size = header.unpacked_size;
                            file.crc = header.file_crc;
                            file.version = header.version;
                            file.method  = header.method;
                            file.is_rar5 = false;

                            int dict_flags = (header.flags >> 5) & 7;
                            file.folder = (dict_flags == 7);
                            file.data = p;

                            // store file
                            m_files[header.filename] = file;
                        }
                        else
                        {
                            // ignore file (unsupported compression)
                        }

                        // skip compressed data
                        p += header.packed_size;

                        break;
                    }

                    case ENDARC_HEAD:
                    {
                        p = end; // terminate parsing
                        break;
                    }
                }
            }
        }

        uint64 vint(mango::LittleEndianPointer& p)
        {
            uint64 value = 0;
            int shift = 0;
            for (int i = 0; i < 10; ++i)
            {
                uint8 sample = *p++;
                value |= ((sample & 0x7f) << shift);
                shift += 7;
                if ((sample & 0x80) != 0x80)
                    break;
            }
            return value;
        }

        void parse_rar5_file_header(mango::LittleEndianPointer p, Memory compressed_data)
        {
            uint64 flags = vint(p);
            uint64 unpacked_size = vint(p);
            uint64 attributes = vint(p);

            uint32 mtime = 0;
            uint32 crc = 0;

            if (flags & 2)
            {
                mtime = p.read32();
            }
            
            if (flags & 4)
            {
                crc = p.read32();
            }

            uint64 compression = vint(p);
            uint64 host_os = vint(p);
            uint64 length = vint(p);

            MANGO_UNREFERENCED_PARAMETER(attributes);
            MANGO_UNREFERENCED_PARAMETER(mtime);
            MANGO_UNREFERENCED_PARAMETER(host_os);

            bool is_directory = (flags & 1) != 0;

            // compression
            uint32 algorithm = compression & 0x3f; // 0
            bool is_solid = (compression & 0x40) != 0;
            uint32 method = (compression & 0x380) >> 7; // 0..5
            //uint32 min_dict_size = (compression & 0x3c00) >> 10;

            if (flags & 8)
            {
                // unpacked_size is undefined
                return;
            }

            if (is_solid)
            {
                // solid archives are unsupported at this time
                return;
            }

            if (!compressed_data.size && !is_directory)
            {
                // empty non-directory files are not supported
                return;
            }

            // read filename
            uint8* ptr = p;
            const char* s = reinterpret_cast<const char *>(ptr);
            std::string filename(s, length);

            //printf("  %s%s [algorithm: %d, solid: %d, method: %d]\n", 
            //    filename.c_str(), is_directory ? "/" : "", algorithm, is_solid, method);

            FileHeader file;

            file.packed_size = compressed_data.size;
            file.unpacked_size = unpacked_size;
            file.crc = crc;
            file.version = algorithm;
            file.method  = method;
            file.is_rar5 = true;

            file.folder = is_directory;
            file.data = compressed_data.address;

            m_files[filename] = file;
        }

        void parse_rar5(uint8* start, uint8* end)
        {
            mango::LittleEndianPointer p = start;

            for (; p < end;)
            {
                uint32 crc = p.read32();
                uint64 header_size = vint(p);
                uint8* base = p;

                uint32 type = vint(p);
                uint32 flags = vint(p);

                uint64 extra_size = 0;
                uint64 data_size = 0;

                if (flags & 1)
                {
                    extra_size = vint(p);
                }

                if (flags & 2)
                {
                    data_size = vint(p);
                }

                Memory compressed_data(base + header_size, data_size);

                //printf("crc: %.8x, type: %x, flags: %x, header: %x, extra: %x, data: %x\n", 
                //    crc, type, flags, (int)header_size, (int)extra_size, (int)data_size);

                MANGO_UNREFERENCED_PARAMETER(crc);
                MANGO_UNREFERENCED_PARAMETER(extra_size);

                // TODO: add support for AES decryption headers
                // TODO: add support for RAR 5.0 compression

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
                        break;
                    case 5:
                        // End of archive header
                        break;
                }

                p = base + header_size + data_size;
            }
        }

        bool isfile(const std::string& filename) const override
        {
            auto i = m_files.find(filename);

            if (i != m_files.end())
            {
                return !i->second.folder;
            }

            return false;
        }

        void index(FileIndex& index, const std::string& pathname) override
        {
            for (auto i : m_files)
            {
                FileHeader& header = i.second;
                std::string filename = i.first;

                if (header.folder)
                {
                    filename += "/";
                }

                if (isPrefix(filename, pathname))
                {
                    filename = filename.substr(pathname.length());
                    size_t n = filename.find_first_of("/");

                    if (n == std::string::npos)
                    {
                        uint32 flags = 0;
                        if (header.compressed())
                        {
                            flags |= FileInfo::COMPRESSED;
                        }
                        index.emplace(filename, header.unpacked_size, flags);
                    }
                    else
                    {
                        if (n == filename.length() - 1)
                        {
                            index.emplace(filename, 0, FileInfo::DIRECTORY);
                        }
                    }
                }
            }
        }

        VirtualMemory* mmap(const std::string& filename) override
        {
            auto i = m_files.find(filename);
            if (i == m_files.end())
            {
                MANGO_EXCEPTION(ID"File not found.");
            }

            FileHeader& header = i->second;
            return header.mmap();
        }
    };

    // -----------------------------------------------------------------
    // functions
    // -----------------------------------------------------------------

    AbstractMapper* createMapperRAR(Memory parent, const std::string& password)
    {
        AbstractMapper* mapper = new MapperRAR(parent, password);
        return mapper;
    }

} // namespace mango

#endif // MANGO_ENABLE_LICENSE_GPL
