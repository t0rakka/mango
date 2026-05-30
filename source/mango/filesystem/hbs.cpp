/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/filesystem/hbs.hpp>

namespace mango::filesystem::hbs
{

    void writeBlockArray(LittleEndianStream& output, const std::vector<Block>& blocks)
    {
        u32 count = u32(blocks.size());

        output.write32(filesystem::HBS_MAGIC1);
        output.write32(filesystem::HBS_VERSION);
        output.write32(count);
    
        for (auto &block : blocks)
        {
            output.write64(block.offset);
            output.write64(block.compressed);
            output.write64(block.uncompressed);
            output.write32(block.method);
        }
    }

    void writeFileArray(LittleEndianStream& output, const std::vector<File>& files)
    {
        // write file data into temporary buffer

        MemoryStream temp;
        LittleEndianStream stream = temp;

        u32 count = u32(files.size());
        stream.write32(count);

        for (auto &file : files)
        {
            const std::string& filename = file.filename;
            u32 length = u32(filename.length());

            stream.write32(length);
            stream.write(filename.c_str(), length);

            stream.write64(file.size);
            stream.write32(file.checksum);
            stream.write32(u32(file.segments.size()));

            for (auto &segment : file.segments)
            {
                stream.write32(segment.block);
                stream.write64(segment.offset);
                stream.write64(segment.size);
            }
        }

        // compress the temporary buffer

        size_t bound = zstd::bound(temp.size());
        Buffer compressed(bound);

        CompressionStatus status = zstd::compress(compressed, temp, 10);

        // write compressed buffer

        output.write32(filesystem::HBS_MAGIC2);
        output.write32(filesystem::HBS_VERSION);

        output.write64(u64(status.size)); // compressed size
        output.write64(u64(temp.size())); // uncompressed size
        output.write(compressed, status.size);
    }

    void writeIndex(LittleEndianStream& output, u64 block_offset, u64 file_offset)
    {
        output.write32(filesystem::HBS_MAGIC3);
        output.write32(filesystem::HBS_VERSION);
        output.write64(block_offset);
        output.write64(file_offset);
    }

} // namespace mango::filesystem::hbs
