/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#if defined(MANGO_ENABLE_LZMA)

#include <algorithm>
#include <cstring>
#include <mutex>
#include <optional>
#include <vector>

#include <mango/core/buffer.hpp>
#include <mango/core/compress.hpp>
#include <mango/core/container.hpp>
#include <mango/core/crc32.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/pointer.hpp>
#include <mango/core/string.hpp>
#include <mango/filesystem/mapper.hpp>
#include <mango/filesystem/path.hpp>
#include "indexer.hpp"

#include "../../external/lzma/Alloc.h"
#include "../../external/lzma/Ppmd7.h"

namespace
{
    using namespace mango;
    using mango::filesystem::Indexer;

    static constexpr u64 kSignatureHeaderSize = 32;
    static constexpr u32 kFolderCacheSize = 8;

    static constexpr u8 kSignature[6] = { '7', 'z', 0xBC, 0xAF, 0x27, 0x1C };

    enum PropertyID : u8
    {
        kEnd                   = 0x00,
        kHeader                = 0x01,
        kArchiveProperties     = 0x02,
        kAdditionalStreamsInfo = 0x03,
        kMainStreamsInfo       = 0x04,
        kFilesInfo             = 0x05,
        kPackInfo              = 0x06,
        kUnpackInfo            = 0x07,
        kSubStreamsInfo        = 0x08,
        kSize                  = 0x09,
        kCRC                   = 0x0A,
        kFolder                = 0x0B,
        kCodersUnpackSize      = 0x0C,
        kNumUnpackStream       = 0x0D,
        kEmptyStream           = 0x0E,
        kEmptyFile             = 0x0F,
        kAnti                  = 0x10,
        kName                  = 0x11,
        kCTime                 = 0x12,
        kATime                 = 0x13,
        kMTime                 = 0x14,
        kWinAttributes         = 0x15,
        kComment               = 0x16,
        kEncodedHeader         = 0x17,
        kStartPos              = 0x18,
        kDummy                 = 0x19,
    };

    // Codec IDs stored as big-endian integers (7-Zip convention)
    enum CodecID : u64
    {
        CODEC_COPY      = 0x00,
        CODEC_DELTA     = 0x03,
        CODEC_BCJ_SHORT = 0x04,       // xz / short form
        CODEC_LZMA2     = 0x21,
        CODEC_SWAP2     = 0x020302,
        CODEC_SWAP4     = 0x020304,
        CODEC_LZMA      = 0x030101,
        CODEC_PPMD      = 0x030401,
        CODEC_BCJ       = 0x03030103,
        CODEC_BCJ2      = 0x0303011B,
        CODEC_DEFLATE   = 0x040108,
        CODEC_DEFLATE64 = 0x040109,
        CODEC_BZIP2     = 0x040202,
        CODEC_ZSTD      = 0x04F71101, // 7-Zip-zstd plugin
        CODEC_LZ4       = 0x04F71104, // 7-Zip-zstd plugin
        CODEC_AES       = 0x06F10701,
    };

    // -----------------------------------------------------------------
    // Stream reader
    // -----------------------------------------------------------------

    class Stream7z
    {
    protected:
        ConstMemory m_memory;
        size_t m_offset { 0 };

    public:
        explicit Stream7z(ConstMemory memory)
            : m_memory(memory)
        {
        }

        size_t tell() const
        {
            return m_offset;
        }

        size_t remain() const
        {
            return m_memory.size - m_offset;
        }

        void seek(size_t absolute)
        {
            if (absolute > m_memory.size)
            {
                MANGO_EXCEPTION("[mapper.7z] Seek past end of stream.");
            }
            m_offset = absolute;
        }

        void skip(size_t bytes)
        {
            if (bytes > remain())
            {
                MANGO_EXCEPTION("[mapper.7z] Skip past end of stream.");
            }
            m_offset += bytes;
        }

        const u8* ptr() const
        {
            return m_memory.address + m_offset;
        }

        u8 read8()
        {
            if (remain() < 1)
            {
                MANGO_EXCEPTION("[mapper.7z] Unexpected end of stream.");
            }
            return m_memory.address[m_offset++];
        }

        u32 read32()
        {
            if (remain() < 4)
            {
                MANGO_EXCEPTION("[mapper.7z] Unexpected end of stream.");
            }
            LittleEndianConstPointer p = m_memory.address + m_offset;
            u32 value = p.read32();
            m_offset += 4;
            return value;
        }

        u64 read64()
        {
            if (remain() < 8)
            {
                MANGO_EXCEPTION("[mapper.7z] Unexpected end of stream.");
            }
            LittleEndianConstPointer p = m_memory.address + m_offset;
            u64 value = p.read64();
            m_offset += 8;
            return value;
        }

        void read(u8* dest, size_t bytes)
        {
            if (bytes > remain())
            {
                MANGO_EXCEPTION("[mapper.7z] Unexpected end of stream.");
            }
            std::memcpy(dest, m_memory.address + m_offset, bytes);
            m_offset += bytes;
        }

        ConstMemory readMemory(size_t bytes)
        {
            if (bytes > remain())
            {
                MANGO_EXCEPTION("[mapper.7z] Unexpected end of stream.");
            }
            ConstMemory memory(m_memory.address + m_offset, bytes);
            m_offset += bytes;
            return memory;
        }

        // 7z variable-length UINT64
        u64 readNumber()
        {
            u8 first = read8();
            u8 mask = 0x80;
            u64 value = 0;

            for (int i = 0; i < 8; ++i)
            {
                if ((first & mask) == 0)
                {
                    return value | (u64(first & (mask - 1)) << (8 * i));
                }
                value |= u64(read8()) << (8 * i);
                mask >>= 1;
            }

            return value;
        }
    };

    std::vector<bool> readBitVector(Stream7z& s, u64 count, bool check_all_defined)
    {
        std::vector<bool> bits;
        bits.resize(size_t(count), false);

        if (check_all_defined)
        {
            if (s.read8() != 0)
            {
                std::fill(bits.begin(), bits.end(), true);
                return bits;
            }
        }

        u8 value = 0;
        u8 mask = 0;

        for (u64 i = 0; i < count; ++i)
        {
            if (mask == 0)
            {
                value = s.read8();
                mask = 0x80;
            }
            bits[size_t(i)] = (value & mask) != 0;
            mask >>= 1;
        }

        return bits;
    }

    std::vector<std::optional<u32>> readDigests(Stream7z& s, u64 count)
    {
        auto defined = readBitVector(s, count, true);
        std::vector<std::optional<u32>> digests;
        digests.resize(size_t(count));

        for (u64 i = 0; i < count; ++i)
        {
            if (defined[size_t(i)])
            {
                digests[size_t(i)] = s.read32();
            }
        }

        return digests;
    }

    // -----------------------------------------------------------------
    // Archive structures
    // -----------------------------------------------------------------

    struct Coder
    {
        u64 codec_id { 0 };
        u64 num_in_streams { 1 };
        u64 num_out_streams { 1 };
        std::vector<u8> properties;
    };

    struct BindPair
    {
        u64 in_index { 0 };
        u64 out_index { 0 };
    };

    struct Folder
    {
        std::vector<Coder> coders;
        std::vector<BindPair> bind_pairs;
        std::vector<u64> packed_indices;
        std::vector<u64> unpack_sizes; // per out-stream
        std::optional<u32> crc;

        u64 totalInStreams() const
        {
            u64 total = 0;
            for (const auto& coder : coders)
            {
                total += coder.num_in_streams;
            }
            return total;
        }

        u64 totalOutStreams() const
        {
            u64 total = 0;
            for (const auto& coder : coders)
            {
                total += coder.num_out_streams;
            }
            return total;
        }

        u64 mainUnpackSize() const
        {
            std::vector<bool> bound(size_t(totalOutStreams()), false);
            for (const auto& bp : bind_pairs)
            {
                if (bp.out_index < bound.size())
                {
                    bound[size_t(bp.out_index)] = true;
                }
            }

            for (size_t i = 0; i < unpack_sizes.size(); ++i)
            {
                if (!bound[i])
                {
                    return unpack_sizes[i];
                }
            }

            return unpack_sizes.empty() ? 0 : unpack_sizes.front();
        }

        bool isCompressed() const
        {
            for (const auto& coder : coders)
            {
                if (coder.codec_id != CODEC_COPY)
                {
                    return true;
                }
            }
            return false;
        }

        bool isEncrypted() const
        {
            for (const auto& coder : coders)
            {
                if (coder.codec_id == CODEC_AES)
                {
                    return true;
                }
            }
            return false;
        }
    };

    struct PackInfo
    {
        u64 pack_pos { 0 };
        std::vector<u64> sizes;
        std::vector<std::optional<u32>> crcs;
    };

    struct SubStreamsInfo
    {
        std::vector<u64> num_unpack_streams; // per folder
        std::vector<u64> unpack_sizes;       // flat list across folders
        std::vector<std::optional<u32>> crcs; // aligned with unpack_sizes
    };

    struct FileEntry
    {
        std::string name;
        u64 size { 0 };
        u32 checksum { 0 };
        bool has_stream { true };
        bool is_dir { false };
        bool is_anti { false };
        bool has_checksum { false };
        bool compressed { false };
        bool encrypted { false };
        s32 folder_index { -1 };
        s32 stream_index { -1 };
    };

    struct ArchiveHeader
    {
        PackInfo pack_info;
        bool has_pack_info { false };
        std::vector<Folder> folders;
        SubStreamsInfo substreams;
        bool has_substreams { false };
        std::vector<FileEntry> files;
    };

    // -----------------------------------------------------------------
    // Parsing
    // -----------------------------------------------------------------

    Coder parseCoder(Stream7z& s)
    {
        Coder coder;
        u8 flags = s.read8();
        u32 id_size = flags & 0x0f;
        bool is_complex = (flags & 0x10) != 0;
        bool has_attrs = (flags & 0x20) != 0;

        if (id_size == 0 || id_size > 8 || id_size > s.remain())
        {
            MANGO_EXCEPTION("[mapper.7z] Invalid codec id size.");
        }

        for (u32 i = 0; i < id_size; ++i)
        {
            coder.codec_id = (coder.codec_id << 8) | s.read8();
        }

        if (is_complex)
        {
            coder.num_in_streams = s.readNumber();
            coder.num_out_streams = s.readNumber();
        }

        if (has_attrs)
        {
            u64 props_size = s.readNumber();
            if (props_size > s.remain())
            {
                MANGO_EXCEPTION("[mapper.7z] Coder properties exceed stream.");
            }
            coder.properties.resize(size_t(props_size));
            if (props_size)
            {
                s.read(coder.properties.data(), size_t(props_size));
            }
        }

        return coder;
    }

    Folder parseFolder(Stream7z& s)
    {
        Folder folder;
        u64 num_coders = s.readNumber();
        if (num_coders == 0 || num_coders > 64)
        {
            MANGO_EXCEPTION("[mapper.7z] Invalid coder count ({}).", num_coders);
        }

        folder.coders.reserve(size_t(num_coders));
        for (u64 i = 0; i < num_coders; ++i)
        {
            folder.coders.push_back(parseCoder(s));
        }

        const u64 total_in = folder.totalInStreams();
        const u64 total_out = folder.totalOutStreams();
        if (total_out == 0 || total_out > total_in + 64)
        {
            MANGO_EXCEPTION("[mapper.7z] Invalid stream counts.");
        }

        const u64 num_bind_pairs = total_out - 1;
        folder.bind_pairs.resize(size_t(num_bind_pairs));
        for (u64 i = 0; i < num_bind_pairs; ++i)
        {
            folder.bind_pairs[size_t(i)].in_index = s.readNumber();
            folder.bind_pairs[size_t(i)].out_index = s.readNumber();
        }

        const u64 num_packed = total_in - num_bind_pairs;
        if (num_packed == 0)
        {
            MANGO_EXCEPTION("[mapper.7z] Folder has no packed streams.");
        }

        if (num_packed == 1)
        {
            std::vector<bool> bound(size_t(total_in), false);
            for (const auto& bp : folder.bind_pairs)
            {
                if (bp.in_index < bound.size())
                {
                    bound[size_t(bp.in_index)] = true;
                }
            }
            for (u64 i = 0; i < total_in; ++i)
            {
                if (!bound[size_t(i)])
                {
                    folder.packed_indices.push_back(i);
                    break;
                }
            }
        }
        else
        {
            folder.packed_indices.resize(size_t(num_packed));
            for (u64 i = 0; i < num_packed; ++i)
            {
                folder.packed_indices[size_t(i)] = s.readNumber();
            }
        }

        if (folder.packed_indices.size() != size_t(num_packed))
        {
            MANGO_EXCEPTION("[mapper.7z] Failed to resolve packed stream indices.");
        }

        return folder;
    }

    PackInfo parsePackInfo(Stream7z& s)
    {
        PackInfo info;
        info.pack_pos = s.readNumber();
        u64 num_streams = s.readNumber();
        info.sizes.assign(size_t(num_streams), 0);
        info.crcs.assign(size_t(num_streams), std::nullopt);

        for (;;)
        {
            u8 id = s.read8();
            if (id == kEnd)
            {
                break;
            }

            if (id == kSize)
            {
                for (u64 i = 0; i < num_streams; ++i)
                {
                    info.sizes[size_t(i)] = s.readNumber();
                }
            }
            else if (id == kCRC)
            {
                info.crcs = readDigests(s, num_streams);
            }
            else
            {
                u64 size = s.readNumber();
                s.skip(size_t(size));
            }
        }

        return info;
    }

    std::vector<Folder> parseUnpackInfo(Stream7z& s)
    {
        if (s.read8() != kFolder)
        {
            MANGO_EXCEPTION("[mapper.7z] Expected Folder property.");
        }

        u64 num_folders = s.readNumber();
        if (num_folders > 1'000'000)
        {
            MANGO_EXCEPTION("[mapper.7z] Excessive folder count.");
        }

        u8 external = s.read8();
        if (external != 0)
        {
            MANGO_EXCEPTION("[mapper.7z] External folder data is not supported.");
        }

        std::vector<Folder> folders;
        folders.reserve(size_t(num_folders));
        for (u64 i = 0; i < num_folders; ++i)
        {
            folders.push_back(parseFolder(s));
        }

        if (s.read8() != kCodersUnpackSize)
        {
            MANGO_EXCEPTION("[mapper.7z] Expected CodersUnpackSize property.");
        }

        for (auto& folder : folders)
        {
            folder.unpack_sizes.resize(size_t(folder.totalOutStreams()));
            for (auto& size : folder.unpack_sizes)
            {
                size = s.readNumber();
            }
        }

        for (;;)
        {
            u8 id = s.read8();
            if (id == kEnd)
            {
                break;
            }

            if (id == kCRC)
            {
                auto digests = readDigests(s, num_folders);
                for (u64 i = 0; i < num_folders; ++i)
                {
                    folders[size_t(i)].crc = digests[size_t(i)];
                }
            }
            else
            {
                u64 size = s.readNumber();
                s.skip(size_t(size));
            }
        }

        return folders;
    }

    SubStreamsInfo parseSubStreamsInfo(Stream7z& s, const std::vector<Folder>& folders)
    {
        SubStreamsInfo info;
        info.num_unpack_streams.assign(folders.size(), 1);

        bool saw_sizes = false;
        std::vector<std::optional<u32>> raw_crcs;

        for (;;)
        {
            u8 id = s.read8();
            if (id == kEnd)
            {
                break;
            }

            if (id == kNumUnpackStream)
            {
                for (size_t i = 0; i < folders.size(); ++i)
                {
                    info.num_unpack_streams[i] = s.readNumber();
                }
            }
            else if (id == kSize)
            {
                saw_sizes = true;
                for (size_t fi = 0; fi < folders.size(); ++fi)
                {
                    const u64 ns = info.num_unpack_streams[fi];
                    u64 sum = 0;
                    for (u64 i = 0; i + 1 < ns; ++i)
                    {
                        u64 size = s.readNumber();
                        info.unpack_sizes.push_back(size);
                        sum += size;
                    }

                    const u64 folder_size = folders[fi].mainUnpackSize();
                    if (sum > folder_size)
                    {
                        MANGO_EXCEPTION("[mapper.7z] Substream sizes exceed folder unpack size.");
                    }
                    info.unpack_sizes.push_back(folder_size - sum);
                }
            }
            else if (id == kCRC)
            {
                u64 num_digests = 0;
                for (size_t fi = 0; fi < folders.size(); ++fi)
                {
                    const u64 ns = info.num_unpack_streams[fi];
                    if (!(ns == 1 && folders[fi].crc.has_value()))
                    {
                        num_digests += ns;
                    }
                }
                raw_crcs = readDigests(s, num_digests);
            }
            else
            {
                u64 size = s.readNumber();
                s.skip(size_t(size));
            }
        }

        if (!saw_sizes)
        {
            for (size_t fi = 0; fi < folders.size(); ++fi)
            {
                const u64 ns = info.num_unpack_streams[fi];
                if (ns == 1)
                {
                    info.unpack_sizes.push_back(folders[fi].mainUnpackSize());
                }
                else
                {
                    // sizes must be present when folder has multiple unpack streams
                    for (u64 i = 0; i < ns; ++i)
                    {
                        info.unpack_sizes.push_back(0);
                    }
                }
            }
        }

        // Expand CRCs to one entry per unpack stream
        info.crcs.resize(info.unpack_sizes.size());
        size_t digest_index = 0;
        size_t stream_index = 0;

        for (size_t fi = 0; fi < folders.size(); ++fi)
        {
            const u64 ns = info.num_unpack_streams[fi];
            if (ns == 1 && folders[fi].crc.has_value())
            {
                info.crcs[stream_index++] = folders[fi].crc;
            }
            else
            {
                for (u64 i = 0; i < ns; ++i)
                {
                    if (digest_index < raw_crcs.size())
                    {
                        info.crcs[stream_index++] = raw_crcs[digest_index++];
                    }
                    else
                    {
                        info.crcs[stream_index++] = std::nullopt;
                    }
                }
            }
        }

        return info;
    }

    void parseStreamsInfo(Stream7z& s, ArchiveHeader& header)
    {
        for (;;)
        {
            u8 id = s.read8();
            if (id == kEnd)
            {
                break;
            }

            if (id == kPackInfo)
            {
                header.pack_info = parsePackInfo(s);
                header.has_pack_info = true;
            }
            else if (id == kUnpackInfo)
            {
                header.folders = parseUnpackInfo(s);
            }
            else if (id == kSubStreamsInfo)
            {
                header.substreams = parseSubStreamsInfo(s, header.folders);
                header.has_substreams = true;
            }
            else
            {
                u64 size = s.readNumber();
                s.skip(size_t(size));
            }
        }
    }

    std::string readUtf16Name(Stream7z& s)
    {
        std::u16string name;
        for (;;)
        {
            if (s.remain() < 2)
            {
                MANGO_EXCEPTION("[mapper.7z] Truncated filename.");
            }
            u8 lo = s.read8();
            u8 hi = s.read8();
            char16_t ch = char16_t(lo | (u16(hi) << 8));
            if (ch == 0)
            {
                break;
            }
            name.push_back(ch);
        }

        std::string utf8 = utf8_from_utf16(name);
        replace(utf8, "\\", "/");
        return utf8;
    }

    std::vector<FileEntry> parseFilesInfo(Stream7z& s, u64 num_files)
    {
        std::vector<FileEntry> files;
        files.resize(size_t(num_files));
        bool saw_empty_file = false;

        for (;;)
        {
            u8 id = s.read8();
            if (id == kEnd)
            {
                break;
            }

            u64 size = s.readNumber();
            size_t begin = s.tell();
            size_t end = begin + size_t(size);
            if (end > begin + s.remain())
            {
                MANGO_EXCEPTION("[mapper.7z] FilesInfo property exceeds stream.");
            }

            if (id == kName)
            {
                u8 external = s.read8();
                if (external != 0)
                {
                    MANGO_EXCEPTION("[mapper.7z] External filenames are not supported.");
                }

                for (auto& file : files)
                {
                    file.name = readUtf16Name(s);
                }
            }
            else if (id == kEmptyStream)
            {
                auto flags = readBitVector(s, num_files, false);
                for (u64 i = 0; i < num_files; ++i)
                {
                    files[size_t(i)].has_stream = !flags[size_t(i)];
                }
            }
            else if (id == kEmptyFile)
            {
                saw_empty_file = true;
                u64 empty_count = 0;
                for (const auto& file : files)
                {
                    if (!file.has_stream)
                    {
                        ++empty_count;
                    }
                }

                auto flags = readBitVector(s, empty_count, false);
                size_t idx = 0;
                for (auto& file : files)
                {
                    if (!file.has_stream)
                    {
                        // empty-file bit set => empty file; clear => directory
                        file.is_dir = !flags[idx];
                        ++idx;
                    }
                }
            }
            else if (id == kAnti)
            {
                u64 empty_count = 0;
                for (const auto& file : files)
                {
                    if (!file.has_stream)
                    {
                        ++empty_count;
                    }
                }

                auto flags = readBitVector(s, empty_count, false);
                size_t idx = 0;
                for (auto& file : files)
                {
                    if (!file.has_stream)
                    {
                        file.is_anti = flags[idx++];
                    }
                }
            }
            else if (id == kWinAttributes)
            {
                auto defined = readBitVector(s, num_files, true);
                u8 external = s.read8();
                if (external != 0)
                {
                    s.seek(end);
                    continue;
                }

                for (u64 i = 0; i < num_files; ++i)
                {
                    if (defined[size_t(i)])
                    {
                        u32 attrib = s.read32();
                        if (attrib & 0x10)
                        {
                            files[size_t(i)].is_dir = true;
                        }
                    }
                }
            }
            else if (id == kCTime || id == kATime || id == kMTime || id == kDummy || id == kComment)
            {
                s.seek(end);
            }
            else
            {
                s.seek(end);
            }

            if (s.tell() != end)
            {
                s.seek(end);
            }
        }

        // Without EmptyFile, all empty streams are directories (7-Zip convention)
        if (!saw_empty_file)
        {
            for (auto& file : files)
            {
                if (!file.has_stream)
                {
                    file.is_dir = true;
                }
            }
        }

        for (auto& file : files)
        {
            if (!file.name.empty() && file.name.back() == '/')
            {
                file.is_dir = true;
            }
        }

        return files;
    }

    void parseHeader(Stream7z& s, ArchiveHeader& header)
    {
        for (;;)
        {
            u8 id = s.read8();
            if (id == kEnd)
            {
                break;
            }

            if (id == kArchiveProperties)
            {
                for (;;)
                {
                    u8 type = s.read8();
                    if (type == kEnd)
                    {
                        break;
                    }
                    u64 size = s.readNumber();
                    s.skip(size_t(size));
                }
            }
            else if (id == kAdditionalStreamsInfo)
            {
                // Rare; parse and discard (External refs unsupported)
                ArchiveHeader unused;
                parseStreamsInfo(s, unused);
            }
            else if (id == kMainStreamsInfo)
            {
                parseStreamsInfo(s, header);
            }
            else if (id == kFilesInfo)
            {
                u64 num_files = s.readNumber();
                header.files = parseFilesInfo(s, num_files);
            }
            else
            {
                u64 size = s.readNumber();
                s.skip(size_t(size));
            }
        }
    }

    // -----------------------------------------------------------------
    // Filters / decompressors
    // -----------------------------------------------------------------

    void filterDelta(u8* data, size_t size, u32 distance)
    {
        if (distance == 0)
        {
            return;
        }

        for (size_t i = distance; i < size; ++i)
        {
            data[i] = u8(data[i] + data[i - distance]);
        }
    }

    void filterSwap2(u8* data, size_t size)
    {
        size &= ~size_t(1);
        for (size_t i = 0; i < size; i += 2)
        {
            std::swap(data[i], data[i + 1]);
        }
    }

    void filterSwap4(u8* data, size_t size)
    {
        size &= ~size_t(3);
        for (size_t i = 0; i < size; i += 4)
        {
            std::swap(data[i + 0], data[i + 3]);
            std::swap(data[i + 1], data[i + 2]);
        }
    }

    // x86 BCJ decoder (7-Zip / LZMA SDK Bra86 encoding=0)
    void filterBcjX86(u8* data, size_t size, u32 ip)
    {
        if (size < 5)
        {
            return;
        }

        static constexpr u8 kMaskToBitNumber[] = { 0, 1, 2, 2, 3, 3, 3, 3 };

        u32 state = 0;
        size_t pos = 0;
        const size_t limit = size - 4;

        while (pos < limit)
        {
            if ((data[pos] & 0xFE) != 0xE8)
            {
                ++pos;
                continue;
            }

            u32 prev_pos = u32(pos) - state;
            state = (prev_pos > 3) ? 0 : (state << u8(prev_pos));

            u8 b = data[pos + 4];
            if ((b != 0x00 && b != 0xFF) || (((state >> 1) & 0x07) == 0x07))
            {
                state = (state & 0x77777777) | 1;
                ++pos;
                continue;
            }

            u32 src = u32(data[pos + 1])
                    | (u32(data[pos + 2]) << 8)
                    | (u32(data[pos + 3]) << 16)
                    | (u32(data[pos + 4]) << 24);

            for (;;)
            {
                u32 dest = src - (ip + u32(pos) + 5);
                if (state == 0)
                {
                    src = dest;
                    break;
                }

                u32 index = kMaskToBitNumber[(state >> 1) & 0x07];
                b = u8(dest >> (24 - index * 8));
                if (b != 0x00 && b != 0xFF)
                {
                    break;
                }
                src = dest ^ ((u32(1) << (32 - index * 8)) - 1);
            }

            data[pos + 1] = u8(src);
            data[pos + 2] = u8(src >> 8);
            data[pos + 3] = u8(src >> 16);
            data[pos + 4] = u8(((0 - ((src >> 24) & 1)) & 0xFF));
            pos += 5;
            state = 0;
        }
    }

    std::shared_ptr<Buffer> decompressCodec(
        u64 codec_id,
        ConstMemory input,
        const std::vector<u8>& props,
        u64 unpack_size)
    {
        const size_t out_size = size_t(unpack_size);
        auto output = std::make_shared<Buffer>(out_size);

        switch (codec_id)
        {
            case CODEC_COPY:
            {
                if (input.size < out_size)
                {
                    MANGO_EXCEPTION("[mapper.7z] Stored stream is truncated.");
                }
                std::memcpy(output->data(), input.address, out_size);
                return output;
            }

            case CODEC_DELTA:
            {
                if (input.size < out_size)
                {
                    MANGO_EXCEPTION("[mapper.7z] Delta stream is truncated.");
                }
                std::memcpy(output->data(), input.address, out_size);
                u32 distance = props.empty() ? 1u : u32(props[0]) + 1u;
                filterDelta(output->data(), out_size, distance);
                return output;
            }

            case CODEC_SWAP2:
            {
                if (input.size < out_size)
                {
                    MANGO_EXCEPTION("[mapper.7z] Swap2 stream is truncated.");
                }
                std::memcpy(output->data(), input.address, out_size);
                filterSwap2(output->data(), out_size);
                return output;
            }

            case CODEC_SWAP4:
            {
                if (input.size < out_size)
                {
                    MANGO_EXCEPTION("[mapper.7z] Swap4 stream is truncated.");
                }
                std::memcpy(output->data(), input.address, out_size);
                filterSwap4(output->data(), out_size);
                return output;
            }

            case CODEC_BCJ:
            case CODEC_BCJ_SHORT:
            {
                if (input.size < out_size)
                {
                    MANGO_EXCEPTION("[mapper.7z] BCJ stream is truncated.");
                }
                std::memcpy(output->data(), input.address, out_size);
                u32 ip = 0;
                if (props.size() >= 4)
                {
                    LittleEndianConstPointer p = props.data();
                    ip = p.read32();
                }
                filterBcjX86(output->data(), out_size, ip);
                return output;
            }

            case CODEC_LZMA:
            {
                if (props.size() != 5)
                {
                    MANGO_EXCEPTION("[mapper.7z] Invalid LZMA properties.");
                }

                Buffer packed(5 + input.size);
                std::memcpy(packed.data(), props.data(), 5);
                std::memcpy(packed.data() + 5, input.address, input.size);

                Compressor compressor = getCompressor(Compressor::LZMA);
                CompressionStatus status = compressor.decompress(*output, packed);
                if (!status)
                {
                    MANGO_EXCEPTION("[mapper.7z] {}", status.info);
                }
                return output;
            }

            case CODEC_LZMA2:
            {
                if (props.size() != 1)
                {
                    MANGO_EXCEPTION("[mapper.7z] Invalid LZMA2 properties.");
                }

                Buffer packed(1 + input.size);
                packed.data()[0] = props[0];
                std::memcpy(packed.data() + 1, input.address, input.size);

                Compressor compressor = getCompressor(Compressor::LZMA2);
                CompressionStatus status = compressor.decompress(*output, packed);
                if (!status)
                {
                    MANGO_EXCEPTION("[mapper.7z] {}", status.info);
                }
                return output;
            }

            case CODEC_PPMD:
            {
                if (props.size() < 5)
                {
                    MANGO_EXCEPTION("[mapper.7z] Invalid PPMd properties.");
                }

                const u32 order = props[0];
                LittleEndianConstPointer pp = props.data() + 1;
                const u32 mem_size = pp.read32();

                if (order < PPMD7_MIN_ORDER || order > PPMD7_MAX_ORDER)
                {
                    MANGO_EXCEPTION("[mapper.7z] Invalid PPMd order ({}).", order);
                }

                struct InputStreamPPMD7 : IByteIn
                {
                    ConstMemory memory;
                    size_t offset;

                    InputStreamPPMD7(ConstMemory memory)
                        : memory(memory)
                        , offset(0)
                    {
                        Read = read_byte;
                    }

                    static Byte read_byte(const IByteIn* p)
                    {
                        auto* stream = (InputStreamPPMD7*)p;
                        if (stream->offset < stream->memory.size)
                        {
                            return stream->memory.address[stream->offset++];
                        }
                        return 0;
                    }
                };

                InputStreamPPMD7 stream(input);

                CPpmd7 ppmd;
                Ppmd7_Construct(&ppmd);
                if (!Ppmd7_Alloc(&ppmd, mem_size, &g_Alloc))
                {
                    MANGO_EXCEPTION("[mapper.7z] PPMd allocation failed.");
                }

                ppmd.rc.dec.Stream = &stream;
                Ppmd7_Init(&ppmd, order);
                if (!Ppmd7z_RangeDec_Init(&ppmd.rc.dec))
                {
                    Ppmd7_Free(&ppmd, &g_Alloc);
                    MANGO_EXCEPTION("[mapper.7z] PPMd range decoder init failed.");
                }

                for (size_t i = 0; i < out_size; ++i)
                {
                    int symbol = Ppmd7z_DecodeSymbol(&ppmd);
                    if (symbol < 0)
                    {
                        Ppmd7_Free(&ppmd, &g_Alloc);
                        MANGO_EXCEPTION("[mapper.7z] PPMd decode failed.");
                    }
                    output->data()[i] = u8(symbol);
                }

                Ppmd7_Free(&ppmd, &g_Alloc);
                return output;
            }

            case CODEC_DEFLATE:
            {
                Compressor compressor = getCompressor(Compressor::DEFLATE);
                CompressionStatus status = compressor.decompress(*output, input);
                if (!status)
                {
                    MANGO_EXCEPTION("[mapper.7z] {}", status.info);
                }
                return output;
            }

            case CODEC_DEFLATE64:
            {
                CompressionStatus status = deflate64::decompress(*output, input);
                if (!status)
                {
                    MANGO_EXCEPTION("[mapper.7z] {}", status.info);
                }
                return output;
            }

            case CODEC_BZIP2:
            {
                Compressor compressor = getCompressor(Compressor::BZIP2);
                if (!compressor.decompress)
                {
                    MANGO_EXCEPTION("[mapper.7z] BZIP2 support is not enabled.");
                }
                CompressionStatus status = compressor.decompress(*output, input);
                if (!status)
                {
                    MANGO_EXCEPTION("[mapper.7z] {}", status.info);
                }
                return output;
            }

            case CODEC_ZSTD:
            {
                Compressor compressor = getCompressor(Compressor::ZSTD);
                CompressionStatus status = compressor.decompress(*output, input);
                if (!status)
                {
                    MANGO_EXCEPTION("[mapper.7z] {}", status.info);
                }
                return output;
            }

            case CODEC_LZ4:
            {
                Compressor compressor = getCompressor(Compressor::LZ4);
                if (!compressor.decompress)
                {
                    MANGO_EXCEPTION("[mapper.7z] LZ4 support is not enabled.");
                }
                CompressionStatus status = compressor.decompress(*output, input);
                if (!status)
                {
                    MANGO_EXCEPTION("[mapper.7z] {}", status.info);
                }
                return output;
            }

            case CODEC_AES:
                MANGO_EXCEPTION("[mapper.7z] Encrypted archives are not supported yet.");

            default:
                MANGO_EXCEPTION("[mapper.7z] Unsupported codec (0x{:x}).", codec_id);
        }
    }

    s32 outStreamToCoder(const Folder& folder, u64 out_stream)
    {
        u64 index = 0;
        for (size_t i = 0; i < folder.coders.size(); ++i)
        {
            if (out_stream < index + folder.coders[i].num_out_streams)
            {
                return s32(i);
            }
            index += folder.coders[i].num_out_streams;
        }
        MANGO_EXCEPTION("[mapper.7z] Out-stream index out of range.");
    }

    u64 coderFirstStream(const Folder& folder, size_t coder_index, bool output)
    {
        u64 index = 0;
        for (size_t i = 0; i < folder.coders.size(); ++i)
        {
            if (i == coder_index)
            {
                return index;
            }
            index += output ? folder.coders[i].num_out_streams : folder.coders[i].num_in_streams;
        }
        MANGO_EXCEPTION("[mapper.7z] Coder index out of range.");
    }

    const BindPair* findBindPair(const Folder& folder, u64 stream, bool by_input)
    {
        for (const auto& bp : folder.bind_pairs)
        {
            if ((by_input ? bp.in_index : bp.out_index) == stream)
            {
                return &bp;
            }
        }
        return nullptr;
    }

    std::vector<size_t> resolveCoderChain(const Folder& folder)
    {
        const u64 total_out = folder.totalOutStreams();
        std::vector<bool> bound_out(size_t(total_out), false);
        for (const auto& bp : folder.bind_pairs)
        {
            if (bp.out_index < total_out)
            {
                bound_out[size_t(bp.out_index)] = true;
            }
        }

        u64 main_out = 0;
        bool found = false;
        for (u64 i = 0; i < total_out; ++i)
        {
            if (!bound_out[size_t(i)])
            {
                main_out = i;
                found = true;
                break;
            }
        }
        if (!found)
        {
            MANGO_EXCEPTION("[mapper.7z] Folder has no unbound output stream.");
        }

        std::vector<size_t> chain;
        u64 current_out = main_out;

        for (;;)
        {
            s32 coder_idx = outStreamToCoder(folder, current_out);
            chain.push_back(size_t(coder_idx));

            const Coder& coder = folder.coders[size_t(coder_idx)];
            if (coder.num_in_streams != 1)
            {
                break;
            }

            u64 in_stream = coderFirstStream(folder, size_t(coder_idx), false);
            const BindPair* bp = findBindPair(folder, in_stream, true);
            if (!bp)
            {
                break;
            }
            current_out = bp->out_index;
        }

        std::reverse(chain.begin(), chain.end());
        return chain;
    }

    size_t packedStreamForCoder(const Folder& folder, size_t coder_index)
    {
        const u64 in_stream = coderFirstStream(folder, coder_index, false);
        const u64 total_in = folder.totalInStreams();

        std::vector<bool> bound_in(size_t(total_in), false);
        for (const auto& bp : folder.bind_pairs)
        {
            if (bp.in_index < total_in)
            {
                bound_in[size_t(bp.in_index)] = true;
            }
        }

        size_t pack_index = 0;
        for (u64 s = 0; s < total_in; ++s)
        {
            if (!bound_in[size_t(s)])
            {
                if (s == in_stream)
                {
                    return pack_index;
                }
                ++pack_index;
            }
        }

        return 0;
    }

    ConstMemory packedForInStream(
        const Folder& folder,
        const std::vector<ConstMemory>& packed_streams,
        u64 in_stream)
    {
        for (size_t i = 0; i < folder.packed_indices.size(); ++i)
        {
            if (folder.packed_indices[i] == in_stream)
            {
                if (i >= packed_streams.size())
                {
                    MANGO_EXCEPTION("[mapper.7z] Packed stream index out of range.");
                }
                return packed_streams[i];
            }
        }

        MANGO_EXCEPTION("[mapper.7z] In-stream {} is not packed.", in_stream);
    }

    // BCJ2 decoder (7-Zip Bcj2.c / public domain algorithm)
    std::shared_ptr<Buffer> decodeBcj2(
        ConstMemory main_data,
        ConstMemory call_data,
        ConstMemory jump_data,
        ConstMemory rc_data,
        u64 unpack_size)
    {
        constexpr u32 kTopValue = 1u << 24;
        constexpr u32 kBitModelTotal = 1u << 11;
        constexpr u32 kNumMoveBits = 5;
        constexpr size_t kNumProbs = 2 + 256;

        if (rc_data.size < 5)
        {
            MANGO_EXCEPTION("[mapper.7z] BCJ2 range coder stream too short.");
        }
        if (rc_data.address[0] != 0)
        {
            MANGO_EXCEPTION("[mapper.7z] BCJ2 range coder stream has invalid header.");
        }

        size_t main_pos = 0;
        size_t call_pos = 0;
        size_t jump_pos = 0;
        size_t rc_pos = 5;

        u32 code = 0;
        for (int i = 1; i < 5; ++i)
        {
            code = (code << 8) | rc_data.address[i];
        }
        u32 range = 0xffffffffu;

        u16 probs[kNumProbs];
        for (size_t i = 0; i < kNumProbs; ++i)
        {
            probs[i] = u16(kBitModelTotal >> 1);
        }

        const size_t out_size = size_t(unpack_size);
        auto output = std::make_shared<Buffer>(out_size);
        u8* dest = output->data();

        size_t out_pos = 0;
        u32 ip = 0;
        u8 prev_byte = 0;

        auto normalize = [&]()
        {
            if (range < kTopValue)
            {
                if (rc_pos >= rc_data.size)
                {
                    MANGO_EXCEPTION("[mapper.7z] BCJ2 unexpected end of range coder stream.");
                }
                range <<= 8;
                code = (code << 8) | rc_data.address[rc_pos++];
            }
        };

        while (out_pos < out_size)
        {
            normalize();

            bool found_branch = false;

            while (main_pos < main_data.size && out_pos < out_size)
            {
                u8 b = main_data.address[main_pos++];
                if (b == 0x0f && main_pos < main_data.size && (main_data.address[main_pos] & 0xf0) == 0x80)
                {
                    dest[out_pos++] = b;
                    b = main_data.address[main_pos++];
                    if (out_pos >= out_size)
                    {
                        MANGO_EXCEPTION("[mapper.7z] BCJ2 output overflow.");
                    }
                    dest[out_pos++] = b;
                    ip += 2;
                    prev_byte = b;
                    continue;
                }

                if ((b & 0xfe) == 0xe8)
                {
                    found_branch = true;
                    dest[out_pos++] = b;
                    ip += 1;
                    break;
                }

                dest[out_pos++] = b;
                ip += 1;
                prev_byte = b;
            }

            if (!found_branch)
            {
                break;
            }

            const u8 b = dest[out_pos - 1];
            size_t prob_idx;
            if (b == 0xe8)
            {
                prob_idx = 2 + prev_byte;
            }
            else if (b == 0xe9)
            {
                prob_idx = 1;
            }
            else
            {
                prob_idx = 0;
            }

            u32 ttt = probs[prob_idx];
            u32 bound = (range >> 11) * ttt;

            if (code < bound)
            {
                range = bound;
                probs[prob_idx] = u16(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
                prev_byte = b;
                continue;
            }

            range -= bound;
            code -= bound;
            probs[prob_idx] = u16(ttt - (ttt >> kNumMoveBits));

            u32 val = 0;
            if (b == 0xe8)
            {
                if (call_pos + 4 > call_data.size)
                {
                    MANGO_EXCEPTION("[mapper.7z] BCJ2 unexpected end of CALL stream.");
                }
                val = (u32(call_data.address[call_pos]) << 24)
                    | (u32(call_data.address[call_pos + 1]) << 16)
                    | (u32(call_data.address[call_pos + 2]) << 8)
                    | u32(call_data.address[call_pos + 3]);
                call_pos += 4;
            }
            else
            {
                if (jump_pos + 4 > jump_data.size)
                {
                    MANGO_EXCEPTION("[mapper.7z] BCJ2 unexpected end of JUMP stream.");
                }
                val = (u32(jump_data.address[jump_pos]) << 24)
                    | (u32(jump_data.address[jump_pos + 1]) << 16)
                    | (u32(jump_data.address[jump_pos + 2]) << 8)
                    | u32(jump_data.address[jump_pos + 3]);
                jump_pos += 4;
            }

            ip += 4;
            val -= ip;

            if (out_pos + 4 > out_size)
            {
                MANGO_EXCEPTION("[mapper.7z] BCJ2 output overflow.");
            }

            dest[out_pos + 0] = u8(val);
            dest[out_pos + 1] = u8(val >> 8);
            dest[out_pos + 2] = u8(val >> 16);
            dest[out_pos + 3] = u8(val >> 24);
            out_pos += 4;
            prev_byte = u8(val >> 24);
        }

        if (out_pos != out_size)
        {
            MANGO_EXCEPTION("[mapper.7z] BCJ2 produced {} bytes, expected {}.", out_pos, out_size);
        }

        return output;
    }

    std::shared_ptr<Buffer> resolveOutStream(
        const Folder& folder,
        u64 out_stream,
        const std::vector<ConstMemory>& packed_streams,
        std::vector<std::shared_ptr<Buffer>>& out_cache,
        std::vector<bool>& out_ready);

    std::shared_ptr<Buffer> decodeCoder(
        const Folder& folder,
        size_t coder_index,
        const std::vector<ConstMemory>& packed_streams,
        std::vector<std::shared_ptr<Buffer>>& out_cache,
        std::vector<bool>& out_ready)
    {
        const Coder& coder = folder.coders[coder_index];
        const u64 first_in = coderFirstStream(folder, coder_index, false);
        const u64 first_out = coderFirstStream(folder, coder_index, true);
        const u64 unpack_size = (first_out < folder.unpack_sizes.size())
            ? folder.unpack_sizes[size_t(first_out)]
            : folder.mainUnpackSize();

        std::vector<ConstMemory> inputs;
        std::vector<std::shared_ptr<Buffer>> owned_inputs;
        inputs.reserve(size_t(coder.num_in_streams));

        for (u64 s = 0; s < coder.num_in_streams; ++s)
        {
            const u64 global_in = first_in + s;
            const BindPair* bp = findBindPair(folder, global_in, true);
            if (bp)
            {
                auto buf = resolveOutStream(folder, bp->out_index, packed_streams, out_cache, out_ready);
                owned_inputs.push_back(buf);
                inputs.push_back(*buf);
            }
            else
            {
                inputs.push_back(packedForInStream(folder, packed_streams, global_in));
            }
        }

        if (coder.codec_id == CODEC_BCJ2)
        {
            if (inputs.size() < 4)
            {
                MANGO_EXCEPTION("[mapper.7z] BCJ2 requires 4 input streams.");
            }
            return decodeBcj2(inputs[0], inputs[1], inputs[2], inputs[3], unpack_size);
        }

        if (coder.num_in_streams != 1 || coder.num_out_streams != 1)
        {
            MANGO_EXCEPTION("[mapper.7z] Unsupported complex coder (0x{:x}).", coder.codec_id);
        }

        return decompressCodec(coder.codec_id, inputs[0], coder.properties, unpack_size);
    }

    std::shared_ptr<Buffer> resolveOutStream(
        const Folder& folder,
        u64 out_stream,
        const std::vector<ConstMemory>& packed_streams,
        std::vector<std::shared_ptr<Buffer>>& out_cache,
        std::vector<bool>& out_ready)
    {
        if (out_stream >= out_cache.size())
        {
            MANGO_EXCEPTION("[mapper.7z] Out-stream index out of range.");
        }

        if (out_ready[size_t(out_stream)])
        {
            return out_cache[size_t(out_stream)];
        }

        const size_t coder_index = size_t(outStreamToCoder(folder, out_stream));
        auto buffer = decodeCoder(folder, coder_index, packed_streams, out_cache, out_ready);

        // Cache all out-streams of this coder (currently always 1 except BCJ2 which has 1 out)
        const u64 first_out = coderFirstStream(folder, coder_index, true);
        const Coder& coder = folder.coders[coder_index];
        for (u64 s = 0; s < coder.num_out_streams; ++s)
        {
            const size_t idx = size_t(first_out + s);
            out_cache[idx] = buffer;
            out_ready[idx] = true;
        }

        return buffer;
    }

    std::shared_ptr<Buffer> decompressFolder(
        const Folder& folder,
        const std::vector<ConstMemory>& packed_streams)
    {
        if (folder.coders.empty())
        {
            MANGO_EXCEPTION("[mapper.7z] Folder has no coders.");
        }

        if (packed_streams.empty())
        {
            MANGO_EXCEPTION("[mapper.7z] Folder has no packed data.");
        }

        const u64 total_out = folder.totalOutStreams();
        std::vector<bool> bound_out(size_t(total_out), false);
        for (const auto& bp : folder.bind_pairs)
        {
            if (bp.out_index < total_out)
            {
                bound_out[size_t(bp.out_index)] = true;
            }
        }

        u64 main_out = 0;
        bool found = false;
        for (u64 i = 0; i < total_out; ++i)
        {
            if (!bound_out[size_t(i)])
            {
                main_out = i;
                found = true;
                break;
            }
        }
        if (!found)
        {
            MANGO_EXCEPTION("[mapper.7z] Folder has no unbound output stream.");
        }

        // Fast path: single linear coder chain (no BCJ2 / multi-input)
        bool simple = true;
        for (const auto& coder : folder.coders)
        {
            if (coder.num_in_streams != 1 || coder.num_out_streams != 1 || coder.codec_id == CODEC_BCJ2)
            {
                simple = false;
                break;
            }
        }

        if (simple)
        {
            auto chain = resolveCoderChain(folder);
            size_t pack_index = packedStreamForCoder(folder, chain.front());
            if (pack_index >= packed_streams.size())
            {
                MANGO_EXCEPTION("[mapper.7z] Packed stream index out of range.");
            }

            ConstMemory current = packed_streams[pack_index];
            std::shared_ptr<Buffer> owned;

            for (size_t coder_idx : chain)
            {
                const Coder& coder = folder.coders[coder_idx];
                const u64 out_stream = coderFirstStream(folder, coder_idx, true);
                const u64 unpack_size = (out_stream < folder.unpack_sizes.size())
                    ? folder.unpack_sizes[size_t(out_stream)]
                    : folder.mainUnpackSize();

                owned = decompressCodec(coder.codec_id, current, coder.properties, unpack_size);
                current = *owned;
            }

            return owned;
        }

        std::vector<std::shared_ptr<Buffer>> out_cache;
        out_cache.resize(size_t(total_out));
        std::vector<bool> out_ready(size_t(total_out), false);
        return resolveOutStream(folder, main_out, packed_streams, out_cache, out_ready);
    }

    // -----------------------------------------------------------------
    // Archive index builder
    // -----------------------------------------------------------------

    struct FileHeader
    {
        std::string filename;
        u64 size { 0 };
        u32 checksum { 0 };
        bool is_folder { false };
        bool compressed { false };
        bool encrypted { false };
        bool has_checksum { false };
        s32 folder_index { -1 };
        s32 stream_index { -1 };
        u64 stream_offset { 0 }; // byte offset within decompressed folder
    };

    void ensureDefaultSubstreams(ArchiveHeader& header)
    {
        if (header.has_substreams)
        {
            return;
        }

        header.substreams.num_unpack_streams.assign(header.folders.size(), 1);
        header.substreams.unpack_sizes.clear();
        header.substreams.crcs.clear();

        for (const auto& folder : header.folders)
        {
            header.substreams.unpack_sizes.push_back(folder.mainUnpackSize());
            header.substreams.crcs.push_back(folder.crc);
        }

        header.has_substreams = true;
    }

    void bindFilesToStreams(ArchiveHeader& header)
    {
        ensureDefaultSubstreams(header);

        size_t file_index = 0;
        size_t stream_offset = 0;

        for (size_t fi = 0; fi < header.folders.size(); ++fi)
        {
            const u64 ns = (fi < header.substreams.num_unpack_streams.size())
                ? header.substreams.num_unpack_streams[fi]
                : 1;

            for (u64 si = 0; si < ns; ++si)
            {
                while (file_index < header.files.size() && !header.files[file_index].has_stream)
                {
                    ++file_index;
                }

                if (file_index >= header.files.size())
                {
                    break;
                }

                FileEntry& entry = header.files[file_index];
                const size_t global = stream_offset + size_t(si);

                if (global < header.substreams.unpack_sizes.size())
                {
                    entry.size = header.substreams.unpack_sizes[global];
                }
                else if (ns == 1)
                {
                    entry.size = header.folders[fi].mainUnpackSize();
                }

                if (global < header.substreams.crcs.size() && header.substreams.crcs[global].has_value())
                {
                    entry.checksum = header.substreams.crcs[global].value();
                    entry.has_checksum = true;
                }

                entry.folder_index = s32(fi);
                entry.stream_index = s32(si);
                entry.compressed = header.folders[fi].isCompressed();
                entry.encrypted = header.folders[fi].isEncrypted();

                ++file_index;
            }

            stream_offset += size_t(ns);
        }
    }

    class Index7z
    {
    public:
        ConstMemory m_memory;
        ArchiveHeader m_header;
        Indexer<FileHeader> m_folders;
        std::vector<u64> m_folder_pack_start; // first pack-stream index per folder
        bool m_valid { false };

        explicit Index7z(ConstMemory memory)
            : m_memory(memory)
        {
            if (!memory.address || memory.size < kSignatureHeaderSize)
            {
                return;
            }

            if (std::memcmp(memory.address, kSignature, 6) != 0)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address + 6;
            u8 major = p.read8();
            p.read8(); // minor
            if (major != 0)
            {
                return;
            }

            u32 start_crc = p.read32();
            u64 next_offset = p.read64();
            u64 next_size = p.read64();
            u32 next_crc = p.read32();

            ConstMemory start_header(memory.address + 12, 20);
            if (crc32(0, start_header) != start_crc)
            {
                return;
            }

            if (next_size == 0)
            {
                m_valid = true;
                return;
            }

            const u64 header_offset = kSignatureHeaderSize + next_offset;
            if (header_offset > memory.size || next_size > memory.size - header_offset)
            {
                return;
            }

            ConstMemory header_memory(memory.address + header_offset, size_t(next_size));
            if (crc32(0, header_memory) != next_crc)
            {
                return;
            }

            try
            {
                parseTopLevel(header_memory);
                bindFilesToStreams(m_header);
                buildPackStarts();
                buildIndexer();
                m_valid = true;
            }
            catch (...)
            {
                m_valid = false;
            }
        }

    private:
        void parseTopLevel(ConstMemory header_memory)
        {
            Stream7z s(header_memory);
            u8 id = s.read8();

            if (id == kHeader)
            {
                parseHeader(s, m_header);
            }
            else if (id == kEncodedHeader)
            {
                ArchiveHeader encoded;
                parseStreamsInfo(s, encoded);
                auto decoded = decodeEncodedHeader(encoded);

                Stream7z inner(*decoded);
                u8 inner_id = inner.read8();
                if (inner_id == kHeader)
                {
                    parseHeader(inner, m_header);
                }
                else if (inner_id == kEncodedHeader)
                {
                    // nested encoded header
                    ArchiveHeader nested;
                    parseStreamsInfo(inner, nested);
                    auto nested_decoded = decodeEncodedHeader(nested);
                    Stream7z nested_stream(*nested_decoded);
                    if (nested_stream.read8() != kHeader)
                    {
                        MANGO_EXCEPTION("[mapper.7z] Expected Header after decoding.");
                    }
                    parseHeader(nested_stream, m_header);
                }
                else
                {
                    MANGO_EXCEPTION("[mapper.7z] Unexpected property in decoded header.");
                }
            }
            else
            {
                MANGO_EXCEPTION("[mapper.7z] Unexpected top-level property (0x{:x}).", id);
            }
        }

        std::shared_ptr<Buffer> decodeEncodedHeader(const ArchiveHeader& encoded)
        {
            if (!encoded.has_pack_info || encoded.folders.empty())
            {
                MANGO_EXCEPTION("[mapper.7z] Encoded header is incomplete.");
            }

            const Folder& folder = encoded.folders.front();
            std::vector<ConstMemory> packed;
            u64 offset = kSignatureHeaderSize + encoded.pack_info.pack_pos;

            for (u64 size : encoded.pack_info.sizes)
            {
                if (offset + size > m_memory.size)
                {
                    MANGO_EXCEPTION("[mapper.7z] Encoded header pack data out of range.");
                }
                packed.emplace_back(m_memory.address + offset, size_t(size));
                offset += size;
            }

            return decompressFolder(folder, packed);
        }

        void buildPackStarts()
        {
            m_folder_pack_start.assign(m_header.folders.size(), 0);
            u64 pack_index = 0;
            for (size_t i = 0; i < m_header.folders.size(); ++i)
            {
                m_folder_pack_start[i] = pack_index;
                pack_index += m_header.folders[i].packed_indices.size();
            }
        }

        void buildIndexer()
        {
            // Precompute stream offsets inside each folder for solid archives
            std::vector<std::vector<u64>> offsets(m_header.folders.size());
            if (m_header.has_substreams)
            {
                size_t global = 0;
                for (size_t fi = 0; fi < m_header.folders.size(); ++fi)
                {
                    const u64 ns = m_header.substreams.num_unpack_streams[fi];
                    u64 offset = 0;
                    offsets[fi].resize(size_t(ns));
                    for (u64 si = 0; si < ns; ++si)
                    {
                        offsets[fi][size_t(si)] = offset;
                        if (global < m_header.substreams.unpack_sizes.size())
                        {
                            offset += m_header.substreams.unpack_sizes[global];
                        }
                        ++global;
                    }
                }
            }

            for (const auto& entry : m_header.files)
            {
                if (entry.is_anti || entry.name.empty())
                {
                    continue;
                }

                FileHeader header;
                header.filename = entry.name;
                header.size = entry.size;
                header.checksum = entry.checksum;
                header.has_checksum = entry.has_checksum;
                header.is_folder = entry.is_dir;
                header.compressed = entry.compressed;
                header.encrypted = entry.encrypted;
                header.folder_index = entry.folder_index;
                header.stream_index = entry.stream_index;

                if (entry.folder_index >= 0 &&
                    size_t(entry.folder_index) < offsets.size() &&
                    entry.stream_index >= 0 &&
                    size_t(entry.stream_index) < offsets[size_t(entry.folder_index)].size())
                {
                    header.stream_offset = offsets[size_t(entry.folder_index)][size_t(entry.stream_index)];
                }

                if (header.is_folder && header.filename.back() != '/')
                {
                    header.filename.push_back('/');
                }

                std::string filename = header.filename;
                while (!filename.empty())
                {
                    std::string folder = mango::filesystem::getPath(filename.substr(0, filename.length() - 1));
                    header.filename = filename.substr(folder.length());
                    m_folders.insert(folder, filename, header);
                    header.is_folder = true;
                    header.size = 0;
                    header.folder_index = -1;
                    header.stream_index = -1;
                    filename = folder;
                }
            }
        }
    };

} // namespace

namespace mango::filesystem
{

    // -----------------------------------------------------------------
    // VirtualMemory7Z
    // -----------------------------------------------------------------

    class VirtualMemory7Z : public mango::VirtualMemory
    {
    protected:
        std::shared_ptr<Buffer> m_buffer;

    public:
        VirtualMemory7Z(ConstMemory memory)
        {
            m_memory = memory;
        }

        VirtualMemory7Z(std::shared_ptr<Buffer> buffer, ConstMemory memory)
            : m_buffer(std::move(buffer))
        {
            m_memory = memory;
        }

        ~VirtualMemory7Z() = default;
    };

    // -----------------------------------------------------------------
    // Mapper7Z
    // -----------------------------------------------------------------

    class Mapper7Z : public AbstractMapper
    {
    protected:
        Index7z m_index;
        std::string m_password;
        LRUCache<u32, std::shared_ptr<Buffer>> m_cache { kFolderCacheSize };
        std::mutex m_cache_mutex;

        std::shared_ptr<Buffer> decompressFolderCached(u32 folder_index)
        {
            {
                std::lock_guard lock(m_cache_mutex);
                auto cached = m_cache.get(folder_index);
                if (cached)
                {
                    return *cached;
                }
            }

            if (!m_index.m_header.has_pack_info)
            {
                MANGO_EXCEPTION("[mapper.7z] Archive has no pack info.");
            }

            if (folder_index >= m_index.m_header.folders.size())
            {
                MANGO_EXCEPTION("[mapper.7z] Folder index out of range.");
            }

            const Folder& folder = m_index.m_header.folders[folder_index];
            const PackInfo& pack = m_index.m_header.pack_info;

            u64 pack_start = m_index.m_folder_pack_start[folder_index];
            std::vector<ConstMemory> packed;

            for (size_t i = 0; i < folder.packed_indices.size(); ++i)
            {
                const u64 pack_index = pack_start + i;
                if (pack_index >= pack.sizes.size())
                {
                    MANGO_EXCEPTION("[mapper.7z] Pack stream index out of range.");
                }

                u64 offset = kSignatureHeaderSize + pack.pack_pos;
                for (u64 k = 0; k < pack_index; ++k)
                {
                    offset += pack.sizes[size_t(k)];
                }

                const u64 size = pack.sizes[size_t(pack_index)];
                if (offset + size > m_index.m_memory.size)
                {
                    MANGO_EXCEPTION("[mapper.7z] Pack stream exceeds archive.");
                }

                packed.emplace_back(m_index.m_memory.address + offset, size_t(size));
            }

            auto buffer = decompressFolder(folder, packed);

            {
                std::lock_guard lock(m_cache_mutex);
                m_cache.insert(folder_index, buffer);
            }

            return buffer;
        }

    public:
        Mapper7Z(ConstMemory parent, const std::string& password)
            : m_index(parent)
            , m_password(password)
        {
            MANGO_UNREFERENCED(m_password);
        }

        u64 getSize(const std::string& filename) const override
        {
            const FileHeader* header = m_index.m_folders.getHeader(filename);
            if (header)
            {
                return header->size;
            }
            return 0;
        }

        bool isFile(const std::string& filename) const override
        {
            const FileHeader* header = m_index.m_folders.getHeader(filename);
            if (header)
            {
                return !header->is_folder;
            }
            return false;
        }

        void getIndex(FileIndex& index, const std::string& pathname) override
        {
            const Indexer<FileHeader>::Folder* folder = m_index.m_folders.getFolder(pathname);
            if (!folder)
            {
                return;
            }

            for (auto i : folder->headers)
            {
                const FileHeader& header = *i.second;

                u32 flags = 0;
                u64 size = header.size;

                if (header.is_folder)
                {
                    flags |= FileInfo::Directory;
                    size = 0;
                }

                if (header.compressed)
                {
                    flags |= FileInfo::Compressed;
                }

                if (header.encrypted)
                {
                    flags |= FileInfo::Encrypted;
                }

                if (header.has_checksum)
                {
                    index.emplace(header.filename, size, flags, header.checksum);
                }
                else
                {
                    index.emplace(header.filename, size, flags);
                }
            }
        }

        std::unique_ptr<VirtualMemory> map(const std::string& filename) override
        {
            const FileHeader* ptr = m_index.m_folders.getHeader(filename);
            if (!ptr)
            {
                MANGO_EXCEPTION("[mapper.7z] File \"{}\" not found.", filename);
            }

            const FileHeader& header = *ptr;

            if (header.is_folder)
            {
                MANGO_EXCEPTION("[mapper.7z] Cannot map directory \"{}\".", filename);
            }

            if (header.folder_index < 0)
            {
                // empty file
                return std::make_unique<VirtualMemory7Z>(ConstMemory());
            }

            auto folder_buffer = decompressFolderCached(u32(header.folder_index));

            if (header.stream_offset + header.size > folder_buffer->size())
            {
                MANGO_EXCEPTION("[mapper.7z] File \"{}\" exceeds decompressed folder.", filename);
            }

            ConstMemory memory(folder_buffer->data() + header.stream_offset, size_t(header.size));

            if (header.has_checksum)
            {
                u32 sum = crc32(0, memory);
                if (sum != header.checksum)
                {
                    MANGO_EXCEPTION("[mapper.7z] CRC mismatch for \"{}\".", filename);
                }
            }

            return std::make_unique<VirtualMemory7Z>(folder_buffer, memory);
        }
    };

    AbstractMapper* createMapper7Z(ConstMemory parent, const std::string& password)
    {
        return new Mapper7Z(parent, password);
    }

} // namespace mango::filesystem

#endif // defined(MANGO_ENABLE_LZMA)
