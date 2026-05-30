/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <mango/core/configure.hpp>
#include <mango/core/stream.hpp>
#include <mango/core/bits.hpp>

namespace mango::filesystem
{

    /*

    --------------------------------------------------------------------------
    File Format Types:
    --------------------------------------------------------------------------

    Type[]:
        u32         count
        Type        data[count]

    Block:
        u64         offset
        u64         compressed
        u64         uncompressed
        u32         compression method

    Segment:
        u32         block index
        u64         offset
        u64         size

    File:
        char[]      filename
        u64         size
        u32         checksum
        Segment[]   segments

    --------------------------------------------------------------------------
    File Format Structure:
    --------------------------------------------------------------------------

    Compressed block data:
        u32         magic: hbs0
        u8[]        data     <-- written by the compressor, a raw binary blob w/o specific size or structure

    Block Info Array:
        u32         magic: hbs1
        u32         version
        block[]     blocks

    File Info Array:
        u32         magic: hbs2
        u32         version
        u64         compressed size (file array)
        u64         uncmpressed size (file array)
        File[]      files (compressed with zstd)

    Index:
        u32         magic: hbs3
        u32         version
        u64         offset to block info array
        u64         offset to file info array
    */

    // major = high byte, minor = low byte (1.0 -> 0x0100)
    constexpr u32 HBS_VERSION = 0x0100;

    enum : u32
    {
        HBS_MAGIC0 = u32_mask('h', 'b', 's', '0'),
        HBS_MAGIC1 = u32_mask('h', 'b', 's', '1'),
        HBS_MAGIC2 = u32_mask('h', 'b', 's', '2'),
        HBS_MAGIC3 = u32_mask('h', 'b', 's', '3'),
    };

    namespace hbs
    {
        struct Block
        {
            u64 offset;
            u64 compressed;
            u64 uncompressed;
            u32 method;
        };

        struct File
        {
            struct Segment
            {
                u32 block;
                u64 offset;
                u64 size;
            };

            std::string filename;
            u64 size;
            u32 checksum;
            std::vector<Segment> segments;
        };    

        void writeBlockArray(LittleEndianStream& output, const std::vector<Block>& blocks);
        void writeFileArray(LittleEndianStream& output, const std::vector<File>& files);
        void writeIndex(LittleEndianStream& output, u64 block_offset, u64 file_offset);

        std::vector<Block> readBlockArray(ConstMemory memory);
        std::vector<File> readFileArray(ConstMemory memory);
    }

} // namespace mango::filesystem
