/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/string.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/compress.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/aes.hpp>
#include <mango/core/hash.hpp>
#include <mango/core/print.hpp>
#include <mango/filesystem/mapper.hpp>
#include <mango/filesystem/path.hpp>
#include "indexer.hpp"

/*
https://courses.cs.ut.ee/MTAT.07.022/2015_fall/uploads/Main/dmitri-report-f15-16.pdf

[1] PKWARE Inc. APPNOTE.TXT – .ZIP File Format Specification, version 6.3.4
https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT

[2] PKWARE Inc. .ZIP Application Note
https://www.pkware.com/support/zip-app-note/

[3] PKWARE Inc. APPNOTE.TXT, version 1.0
https://pkware.cachefly.net/webdocs/APPNOTE/APPNOTE-1.0.txt

[4] WinZip. AES Encryption Information: Encryption Specification AE-1 and AE-2
http://www.winzip.com/aes_info.htm
*/

namespace
{
    using namespace mango;
    using mango::filesystem::Indexer;

    enum
    {
        DCKEY_SIZE = 12,
        AES_PWVERIFY_SIZE = 2,  // Password verification value size
        AES_MAC_SIZE = 10,     // Authentication tag size
    };

    enum Encryption : u8
    {
        ENCRYPTION_NONE    = 0,
        ENCRYPTION_CLASSIC = 1,
        ENCRYPTION_AES128  = 2,
        ENCRYPTION_AES192  = 3,
        ENCRYPTION_AES256  = 4,
    };

    enum Compression : u16
    {
        COMPRESSION_NONE      = 0,  // stored (no compression)
        COMPRESSION_SHRUNK    = 1,  // Shrunk
        COMPRESSION_REDUCE_1  = 2,  // Reduced with compression factor 1
        COMPRESSION_REDUCE_2  = 3,  // Reduced with compression factor 2
        COMPRESSION_REDUCE_3  = 4,  // Reduced with compression factor 3
        COMPRESSION_REDUCE_4  = 5,  // Reduced with compression factor 4
        COMPRESSION_IMPLODE   = 6,  // Imploded
        COMPRESSION_DEFLATE   = 8,  // Deflated
        COMPRESSION_DEFLATE64 = 9,  // Enhanced Deflating using Deflate64(tm)
        COMPRESSION_DCLI      = 10, // PKWARE Data Compression Library Imploding (old IBM TERSE)
        COMPRESSION_BZIP2     = 12, // compressed using BZIP2 algorithm
        COMPRESSION_LZMA      = 14, // LZMA
        COMPRESSION_CMPSC     = 16, // IBM z/OS CMPSC Compression
        COMPRESSION_TERSE     = 18, // IBM TERSE (new)
        COMPRESSION_LZ77      = 19, // IBM LZ77 z Architecture 
        COMPRESSION_ZSTD      = 93, // Zstandard (zstd) Compression 
        COMPRESSION_MP3       = 94, // MP3 Compression 
        COMPRESSION_XZ        = 95, // XZ Compression 
        COMPRESSION_JPEG      = 96, // JPEG variant
        COMPRESSION_WAVPACK   = 97, // WavPack compressed data
        COMPRESSION_PPMD      = 98, // PPMd version I, Rev 1
        COMPRESSION_AES       = 99, // AE-x encryption marker
    };

    bool isCompressionSupported(u16 compression)
    {
        switch (compression)
        {
            case COMPRESSION_NONE:
            case COMPRESSION_DEFLATE:
            case COMPRESSION_BZIP2:
            case COMPRESSION_LZMA:
            case COMPRESSION_ZSTD:
            case COMPRESSION_PPMD:
                return true;

            default:
                return false;
        }
    }

    size_t getSaltLength(Encryption encryption)
    {
        size_t length = 0;
        switch (encryption)
        {
            case ENCRYPTION_AES128:
                length = 8;
                break;
            case ENCRYPTION_AES192:
                length = 12;
                break;
            case ENCRYPTION_AES256:
                length = 16;
                break;
            default:
                break;
        }
        return length;
    }

    size_t getKeyLength(Encryption encryption)
    {
        size_t length = 0;
        switch (encryption)
        {
            case ENCRYPTION_AES128:
                length = 16;
                break;
            case ENCRYPTION_AES192:
                length = 24;
                break;
            case ENCRYPTION_AES256:
                length = 32;
                break;
            default:
                break;
        }
        return length;
    }

    struct LocalFileHeader
    {
        u32  signature;         // 0x04034b50 ("PK..")
        u16  versionNeeded;     // version needed to extract
        u16  flags;             //
        u16  compression;       // compression method
        u16  lastModTime;       // last mod file time
        u16  lastModDate;       // last mod file date
        u32  crc;               //
        u64  compressedSize;    //
        u64  uncompressedSize;  //
        u16  filenameLen;       // length of the filename field following this structure
        u16  extraFieldLen;     // length of the extra field following the filename field

        u16 aes_vendor = 0;     // 0x4145 'AE'
        u8 aes_strength = 0;    // 1 = AES-128, 2 = AES-192, 3 = AES-256

        LocalFileHeader(LittleEndianConstPointer p)
        {
            signature = p.read32();
            if (status())
			{
			    versionNeeded    = p.read16();
			    flags            = p.read16();
			    compression      = p.read16();
			    lastModTime      = p.read16();
			    lastModDate      = p.read16();
			    crc              = p.read32();
			    compressedSize   = p.read32(); // ZIP64: 0xffffffff
			    uncompressedSize = p.read32(); // ZIP64: 0xffffffff
			    filenameLen      = p.read16();
			    extraFieldLen    = p.read16();

                p += filenameLen;

                // read extra fields
                const u8* ext = p;
                const u8* end = p + extraFieldLen;
                for ( ; ext < end;)
                {
                    LittleEndianConstPointer e = ext;
                    u16 magic = e.read16();
                    u16 size = e.read16();
                    const u8* next = e + size;
                    switch (magic)
                    {
                        case 0x0001:
                        {
                            // ZIP64 extended field
                            if (uncompressedSize == 0xffffffff)
                            {
                                uncompressedSize = e.read64();
                            }
                            if (compressedSize == 0xffffffff)
                            {
                                compressedSize = e.read64();
                            }
                            break;
                        }

                        case 0x9901:
                        {
                            // AES header
                            e += 2; // skip version number (vendor specific)
                            aes_vendor = e.read16();
                            aes_strength = e.read8();
                            compression = e.read16(); // actual compression method (AES encrypted files store 99 in local header)
                            break;
                        }
                    }
                    ext = next;
                }
            }
        }

        bool status() const
        {
            return signature == 0x04034b50;
        }
    };

    struct FileHeader
    {
        u32	signature;         // 0x02014b50
        u16	versionUsed;       //
        u16	versionNeeded;     //
        u16	flags;             //
        u16	compression;       // compression method
        u16	lastModTime;       //
        u16	lastModDate;       //
        u32	crc;               //
        u64	compressedSize;    // ZIP64: 0xffffffff
        u64	uncompressedSize;  // ZIP64: 0xffffffff
        u16	filenameLen;       // length of the filename field following this structure
        u16	extraFieldLen;     // length of the extra field following the filename field
        u16	commentLen;        // length of the file comment field following the extra field
        u16	diskStart;         // the number of the disk on which this file begins, ZIP64: 0xffff
        u16	internal;          // internal file attributes
        u32	external;          // external file attributes
        u64	localOffset;       // relative offset of the local file header, ZIP64: 0xffffffff

        std::string filename;      // filename is stored after the header
        bool        is_folder;     // if the last character of filename is "/", it is a folder
        Encryption  encryption;

        bool read(LittleEndianConstPointer& p)
        {
            signature = p.read32();
            if (signature != 0x02014b50)
            {
                return false;
            }

            versionUsed      = p.read16();
            versionNeeded    = p.read16();
            flags            = p.read16();
            compression      = p.read16();
            lastModTime      = p.read16();
            lastModDate      = p.read16();
            crc              = p.read32();
            compressedSize   = p.read32();
            uncompressedSize = p.read32();
            filenameLen      = p.read16();
            extraFieldLen    = p.read16();
            commentLen       = p.read16();
            diskStart        = p.read16();
            internal         = p.read16();
            external         = p.read32();
            localOffset      = p.read32();

            // read filename
            const u8* us = p;
            const char* s = reinterpret_cast<const char*>(us);
            p += filenameLen;

            if (s[filenameLen - 1] == '/')
            {
                is_folder = true;
            }
            else
            {
                is_folder = false;
            }

            filename = std::string(s, filenameLen);
            encryption = flags & 1 ? ENCRYPTION_CLASSIC : ENCRYPTION_NONE;

            // read extra fields
            const u8* ext = p;
            const u8* end = p + extraFieldLen;
            for ( ; ext < end;)
            {
                LittleEndianConstPointer e = ext;
                u16 magic = e.read16();
                u16 size = e.read16();
                const u8* next = e + size;
                switch (magic)
                {
                    case 0x0001:
                    {
                        // ZIP64 extended field
                        if (uncompressedSize == 0xffffffff)
                        {
                            uncompressedSize = e.read64();
                        }
                        if (compressedSize == 0xffffffff)
                        {
                            compressedSize = e.read64();
                        }
                        if (localOffset == 0xffffffff)
                        {
                            localOffset = e.read64();
                        }
                        if (diskStart == 0xffff)
                        {
                            e += 4;
                        }
                        break;
                    }

                    case 0x9901:
                    {
                        // AES header
                        u16 aes_version = e.read16();
                        u16 aes_magic = e.read16(); // must be 'AE' (0x41, 0x45)
                        u8 mode = e.read8();
                        compression = e.read8(); // override compression algorithm

                        if (aes_version < 1 || aes_version > 2 || aes_magic != 0x4541)
                        {
                            MANGO_EXCEPTION("[mapper.zip] Incorrect AES header.");
                        }

                        // select encryption mode
                        switch (mode)
                        {
                            case 1:
                                encryption = ENCRYPTION_AES128;
                                break;
                            case 2:
                                encryption = ENCRYPTION_AES192;
                                break;
                            case 3:
                                encryption = ENCRYPTION_AES256;
                                break;
                            default:
                                MANGO_EXCEPTION("[mapper.zip] Incorrect AES encryption mode.");
                        }

                        break;
                    }
                }
                ext = next;
            }

            p += extraFieldLen;
            p += commentLen;

            return true;
        }
    };

    struct DirEndRecord
    {
        u32	signature;         // 0x06054b50
        u16	thisDisk;          // number of this disk
        u16	dirStartDisk;      // number of the disk containing the start of the central directory
        u64	numEntriesOnDisk;  // # of entries in the central directory on this disk
        u64	numEntriesTotal;   // total # of entries in the central directory
        u64	dirSize;           // size of the central directory
        u64	dirStartOffset;    // offset of the start of central directory on the disk
        u16	commentLen;        // zip file comment length

        DirEndRecord(ConstMemory memory)
        {
            std::memset(this, 0, sizeof(DirEndRecord));

            // find central directory end record signature
            // by scanning backwards from the end of the file
            const u8* start = memory.address;
            const u8* end = memory.end();

            end -= 22; // header size is 22 bytes

            for ( ; end >= start; --end)
            {
                LittleEndianConstPointer p = end;

                signature = p.read32();
                if (status())
                {
                    // central directory detected
                    thisDisk         = p.read16();
                    dirStartDisk     = p.read16();
                    numEntriesOnDisk = p.read16();
                    numEntriesTotal  = p.read16();
                    dirSize          = p.read32();
                    dirStartOffset   = p.read32();
                    commentLen       = p.read16();

                    if (thisDisk != 0 || dirStartDisk != 0 || numEntriesOnDisk != numEntriesTotal)
                    {
                        // multi-volume archives are not supported
                        signature = 0;
                    }

                    if (dirStartOffset == 0xffffffff)
                    {
                        p = end - 20;
                        u32 magic = p.read32();
                        if (magic == 0x07064b50)
                        {
                            // ZIP64 detected
                            p += 4;
                            u64 offset = p.read64();

                            p = start + offset;
                            magic = p.read32();
                            if (magic == 0x06064b50)
                            {
                                // ZIP64 End of Central Directory
                                p += 20;
                                numEntriesOnDisk = p.read64();
                                numEntriesTotal  = p.read64();
                                dirSize          = p.read64();
                                dirStartOffset   = p.read64();
                            }
                        }
                    }

                    break;
                }
            }
        }

        ~DirEndRecord()
        {
        }

        bool status() const
        {
            return signature == 0x06054b50;
        }
    };

    // --------------------------------------------------------------------
    // zip functions
    // --------------------------------------------------------------------

    inline u32 zip_crc32(u32 crc, u8 v)
    {
        static const u32 crc_table[] =
        {
            0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
            0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
        };

        u32 x = crc;
        x = (x >> 4) ^ crc_table[(x & 0xf) ^ (v & 0xf)];
        x = (x >> 4) ^ crc_table[(x & 0xf) ^ (v >> 4)];
        return x;
    }

    inline u8 zip_decrypt_value(u32* keys)
    {
        u32 temp = (keys[2] & 0xffff) | 2;
        return u8(((temp * (temp ^ 1)) >> 8) & 0xff);
    }

    inline void zip_update(u32* keys, u8 v)
    {
        keys[0] = zip_crc32(keys[0], v);
        keys[1] = 1 + (keys[1] + (keys[0] & 0xff)) * 134775813L;
        keys[2] = zip_crc32(keys[2], u8(keys[1] >> 24));
    }

    void zip_decrypt_buffer(u8* out, const u8* in, u64 size, u32* keys)
    {
        for (u64 i = 0; i < size; ++i)
        {
            u8 v = in[i] ^ zip_decrypt_value(keys);
            zip_update(keys, v);
            out[i] = v;
        }
    }

    void zip_init_keys(u32* keys, const char* password)
    {
        keys[0] = 305419896L;
        keys[1] = 591751049L;
        keys[2] = 878082192L;

        for (; *password; ++password)
        {
            zip_update(keys, *password);
        }
    }

    bool zip_decrypt(u8* out, const u8* in, u64 size, const u8* dcheader, int version, u32 crc, const std::string& password)
    {
        if (password.empty())
        {
            // missing password
            return false;
        }

        // decryption keys
        u32 keys[3];
        zip_init_keys(keys, password.c_str());

        // decrypt the 12 byte encryption header
        u8 keyfile[DCKEY_SIZE];
        zip_decrypt_buffer(keyfile, dcheader, DCKEY_SIZE, keys);

        // check that password is correct one
        if (version < 20)
        {
            u16* p = reinterpret_cast<u16*>(&keyfile[10]);
            if (*p != (crc >> 16))
            {
                // incorrect password
                return false;
            }
        }
        else if (version < 30)
        {
            if (keyfile[11] != (crc >> 24))
            {
                // incorrect password
                return false;
            }
        }
        else
        {
            // NOTE: the CRC/password check should be same for version >= 3.0
            //       but some compression programs don't create compatible
            //       dcheader so the check would fail above.
        }

        // read compressed data & decrypt
        zip_decrypt_buffer(out, in, size, keys);

        return true;
    }

    void __sha1(const u8* data, size_t len, u8* out20)
    {
        auto hash = sha1(ConstMemory(data, len));
        std::memcpy(out20, hash.data, 20);
    }

    void hmac_sha1(const u8* key, size_t key_len,
                   const u8* message, size_t msg_len,
                   u8* out20)
    {
        const size_t BLOCK_SIZE = 64;
        u8 k_ipad[BLOCK_SIZE];
        u8 k_opad[BLOCK_SIZE];
        u8 tk[20];

        if (key_len > BLOCK_SIZE)
        {
            __sha1(key, key_len, tk);
            key = tk;
            key_len = 20;
        }

        std::memset(k_ipad, 0x36, BLOCK_SIZE);
        std::memset(k_opad, 0x5c, BLOCK_SIZE);

        for (size_t i = 0; i < key_len; ++i)
        {
            k_ipad[i] ^= key[i];
            k_opad[i] ^= key[i];
        }

        u8 inner_hash[20];

        // Inner: SHA1(k_ipad || message)
        {
            std::vector<u8> inner_data(BLOCK_SIZE + msg_len);
            std::memcpy(inner_data.data(), k_ipad, BLOCK_SIZE);
            std::memcpy(inner_data.data() + BLOCK_SIZE, message, msg_len);
            __sha1(inner_data.data(), inner_data.size(), inner_hash);
        }

        // Outer: SHA1(k_opad || inner_hash)
        {
            u8 outer_data[BLOCK_SIZE + 20];
            std::memcpy(outer_data, k_opad, BLOCK_SIZE);
            std::memcpy(outer_data + BLOCK_SIZE, inner_hash, 20);
            __sha1(outer_data, BLOCK_SIZE + 20, out20);
        }
    }

    void pbkdf2_hmac_sha1(const u8* password, size_t password_len,
                          const u8* salt, size_t salt_len,
                          u32 iterations, u8* out, size_t dk_len)
    {
        u32 block_count = (dk_len + 19) / 20; // SHA-1 outputs 20 bytes
        u8 U[20], T[20];
        u8 salt_block[20]; // max 20 bytes

        std::memcpy(salt_block, salt, salt_len);

        for (u32 block = 1; block <= block_count; ++block)
        {
            // salt || INT(block)
            salt_block[salt_len + 0] = (block >> 24) & 0xff;
            salt_block[salt_len + 1] = (block >> 16) & 0xff;
            salt_block[salt_len + 2] = (block >> 8) & 0xff;
            salt_block[salt_len + 3] = (block) & 0xff;

            hmac_sha1(password, password_len, salt_block, salt_len + 4, U);
            std::memcpy(T, U, 20);

            for (u32 i = 1; i < iterations; ++i)
            {
                hmac_sha1(password, password_len, U, 20, U);
                for (int j = 0; j < 20; ++j)
                {
                    T[j] ^= U[j];
                }
            }

            size_t offset = (block - 1) * 20;
            size_t to_copy = std::min(dk_len - offset, size_t(20));
            std::memcpy(out + offset, T, to_copy);
        }
    }

} // namespace

namespace mango::filesystem
{

    // -----------------------------------------------------------------
    // VirtualMemoryZIP
    // -----------------------------------------------------------------

    class VirtualMemoryZIP : public mango::VirtualMemory
    {
    protected:
        const u8* m_delete_address;

    public:
        VirtualMemoryZIP(const u8* address, const u8* delete_address, size_t size)
            : m_delete_address(delete_address)
        {
            m_memory = ConstMemory(address, size);
        }

        ~VirtualMemoryZIP()
        {
            delete [] m_delete_address;
        }
    };

    // -----------------------------------------------------------------
    // MapperZIP
    // -----------------------------------------------------------------

    class MapperZIP : public AbstractMapper
    {
    public:
        ConstMemory m_parent_memory;
        std::string m_password;
        Indexer<FileHeader> m_folders;

        MapperZIP(ConstMemory parent, const std::string& password)
            : m_parent_memory(parent)
            , m_password(password)
        {
            if (parent.address)
            {
                DirEndRecord record(parent);
                if (record.status())
                {
                    const int numFiles = int(record.numEntriesTotal);

                    // read file headers
                    LittleEndianConstPointer p = parent.address + record.dirStartOffset;

                    for (int i = 0; i < numFiles; ++i)
                    {
                        FileHeader header;
                        if (header.read(p))
                        {
                            // NOTE: Don't index files that can't be decompressed
                            if (isCompressionSupported(header.compression))
                            {
                                std::string filename = header.filename;
                                while (!filename.empty())
                                {
                                    std::string folder = getPath(filename.substr(0, filename.length() - 1));

                                    header.filename = filename.substr(folder.length());
                                    m_folders.insert(folder, filename, header);
                                    header.is_folder = true;
                                    filename = folder;
                                }
                            }
                        }
                    }
                }
            }
        }

        ~MapperZIP()
        {
        }

        std::unique_ptr<VirtualMemory> map(FileHeader header, const u8* start, const std::string& password)
        {
            LittleEndianConstPointer p = start + header.localOffset;

            LocalFileHeader localHeader(p);
            if (!localHeader.status())
            {
                MANGO_EXCEPTION("[mapper.zip] Invalid local header.");
            }

            u64 offset = header.localOffset + 30 + localHeader.filenameLen + localHeader.extraFieldLen;

            const u8* address = start + offset;
            u64 size = 0;

            u8* buffer = nullptr; // remember allocated memory

            //printLine("[ZIP] compression: {}, encryption: {}", int(header.compression), int(header.encryption));

            switch (header.encryption)
            {
                case ENCRYPTION_NONE:
                    break;

                case ENCRYPTION_CLASSIC:
                {
                    // decryption header
                    const u8* dcheader = address;
                    address += DCKEY_SIZE;

                    // NOTE: decryption capability reduced on 32 bit platforms
                    const size_t compressed_size = size_t(header.compressedSize);
                    buffer = new u8[compressed_size];

                    bool status = zip_decrypt(buffer, address, header.compressedSize,
                        dcheader, header.versionUsed & 0xff, header.crc, password);
                    if (!status)
                    {
                        delete[] buffer;
                        MANGO_EXCEPTION("[mapper.zip] Decryption failed (probably incorrect password).");
                    }

                    address = buffer;
                    break;
                }

                case ENCRYPTION_AES128:
                case ENCRYPTION_AES192:
                case ENCRYPTION_AES256:
                {
                    if (!(localHeader.flags & 0x01))
                    {
                        MANGO_EXCEPTION("[mapper.zip] AES encrypted file should have bit 0 set.");
                    }

                    if (password.empty())
                    {
                        MANGO_EXCEPTION("[mapper.zip] AES encrypted file requires a password.");
                    }

                    size_t encrypted_size = size_t(header.compressedSize);
                    size_t salt_length = getSaltLength(header.encryption);
                    size_t key_length = getKeyLength(header.encryption);

                    const u8* salt = address;
                    address += salt_length;
                    encrypted_size -= salt_length;

                    const u8* pass_verify = address;
                    address += AES_PWVERIFY_SIZE;
                    encrypted_size -= AES_PWVERIFY_SIZE;
                    encrypted_size -= AES_MAC_SIZE;

                    const u8* encrypted_data = address;
                    const u8* hmac = address + encrypted_size;

                    const u8* pass = reinterpret_cast<const u8*>(password.c_str());

                    // key_length bytes: AES key
                    // key_length bytes: HMAC key
                    // 2 bytes: password verification
                    constexpr size_t max_derived_size = 66; // 32 + 32 + 2
                    u8 derived[max_derived_size];
                    pbkdf2_hmac_sha1(pass, password.length(), salt, salt_length, 1000, derived, max_derived_size);

                    if (std::memcmp(derived + key_length * 2, pass_verify, AES_PWVERIFY_SIZE))
                    {
                        MANGO_EXCEPTION("[mapper.zip] Password verification failed.");
                    }

                    // Compute HMAC using the derived HMAC key
                    u8 calculated_mac[20];
                    hmac_sha1(derived + key_length, key_length, // HMAC key
                              encrypted_data, encrypted_size, // message
                              calculated_mac);  // output

                    // Compare the first 10 bytes of calculated HMAC with stored MAC
                    if (std::memcmp(calculated_mac, hmac, 10))
                    {
                        MANGO_EXCEPTION("[mapper.zip] HMAC verification failed - file may be corrupted or password incorrect.");
                    }

                    // Allocate plaintext buffer
                    buffer = new u8[encrypted_size];

                    // Initialize AES with just the first 32 bytes (AES key)
                    AES aes(derived + 0, key_length * 8);

                    // Initial counter for CTR mode
                    u8 counter[16] = { 0 };
                    counter[0] = 1;

                    aes.ctr_decrypt(buffer, encrypted_data, encrypted_size, counter);

                    address = buffer;
                    break;
                }
            }

            Compressor compressor;

            switch (header.compression)
            {
                case COMPRESSION_NONE:
                {
                    // direct mapping to parent address
                    size = header.uncompressedSize;
                    break;
                }

                case COMPRESSION_DEFLATE:
                {
                    compressor = getCompressor(Compressor::DEFLATE);
                    break;
                }

                case COMPRESSION_PPMD:
                {
                    compressor = getCompressor(Compressor::PPMD8);
                    break;
                }

                case COMPRESSION_BZIP2:
                {
                    compressor = getCompressor(Compressor::BZIP2);
                    break;
                }

                case COMPRESSION_ZSTD:
                {
                    compressor = getCompressor(Compressor::ZSTD);
                    break;
                }

                case COMPRESSION_LZMA:
                {
                    // parse LZMA compression header
                    p = address;
                    p += 2; // skip LZMA version
                    u16 lzma_propsize = p.read16();
                    address = p;

                    if (lzma_propsize != 5)
                    {
                        delete[] buffer;
                        MANGO_EXCEPTION("[mapper.zip] Incorrect LZMA header.");
                    }

                    header.compressedSize -= 4;

                    compressor = getCompressor(Compressor::LZMA);
                    break;
                }

                case COMPRESSION_SHRUNK:
                case COMPRESSION_REDUCE_1:
                case COMPRESSION_REDUCE_2:
                case COMPRESSION_REDUCE_3:
                case COMPRESSION_REDUCE_4:
                case COMPRESSION_IMPLODE:
                case COMPRESSION_DCLI:
                case COMPRESSION_DEFLATE64:
                case COMPRESSION_CMPSC:
                case COMPRESSION_TERSE:
                case COMPRESSION_LZ77:
                case COMPRESSION_MP3:
                case COMPRESSION_XZ:
                case COMPRESSION_WAVPACK:
                case COMPRESSION_JPEG:
                case COMPRESSION_AES:
                    MANGO_EXCEPTION("[mapper.zip] Unsupported compression algorithm ({}).", header.compression);
                    break;
            }

            if (compressor.decompress)
            {
                const size_t uncompressed_size = size_t(header.uncompressedSize);
                u8* uncompressed_buffer = new u8[uncompressed_size];

                ConstMemory input(address, size_t(header.compressedSize));
                Memory output(uncompressed_buffer, size_t(header.uncompressedSize));

                CompressionStatus status = compressor.decompress(output, input);

                delete[] buffer;
                buffer = uncompressed_buffer;

                if (!status)
                {
                    delete[] buffer;
                    MANGO_EXCEPTION("[mapper.zip] {}", status.info);
                }

                // use decode_buffer as memory map
                address = buffer;
                size = header.uncompressedSize;
            }
            else if (size > 0)
            {
                // no compression -> mapped directly to parent address
            }
            else
            {
                MANGO_EXCEPTION("[mapper.zip] Unsupported compression algorithm ({}).", header.compression);
            }

            return std::make_unique<VirtualMemoryZIP>(address, buffer, size_t(size));
        }

        bool isFile(const std::string& filename) const override
        {
            const FileHeader* ptrHeader = m_folders.getHeader(filename);
            if (ptrHeader)
            {
                return !ptrHeader->is_folder;
            }
            return false;
        }

        void getIndex(FileIndex& index, const std::string& pathname) override
        {
            const Indexer<FileHeader>::Folder* ptrFolder = m_folders.getFolder(pathname);
            if (ptrFolder)
            {
                for (auto i : ptrFolder->headers)
                {
                    const FileHeader& header = *i.second;

                    u32 flags = 0;
                    u64 size = header.uncompressedSize;

                    if (header.is_folder)
                    {
                        flags |= FileInfo::Directory;
                        size = 0;
                    }

                    if (header.compression > 0)
                    {
                        flags |= FileInfo::Compressed;
                    }

                    if (header.encryption != ENCRYPTION_NONE)
                    {
                        flags |= FileInfo::Encrypted;
                    }

                    index.emplace(header.filename, size, flags);
                }
            }
        }

        std::unique_ptr<VirtualMemory> map(const std::string& filename) override
        {
            const FileHeader* ptrHeader = m_folders.getHeader(filename);
            if (!ptrHeader)
            {
                MANGO_EXCEPTION("[mapper.zip] File \"{}\" not found.", filename);
            }

            const FileHeader& header = *ptrHeader;
            return map(header, m_parent_memory.address, m_password);
        }
    };

    // -----------------------------------------------------------------
    // functions
    // -----------------------------------------------------------------

    AbstractMapper* createMapperZIP(ConstMemory parent, const std::string& password)
    {
        AbstractMapper* mapper = new MapperZIP(parent, password);
        return mapper;
    }

} // namespace mango::filesystem
