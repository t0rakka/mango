/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/filesystem/hbs.hpp>

namespace mango::filesystem::hbs
{

    namespace
    {
        std::vector<File> readFileRecords(LittleEndianConstPointer p)
        {
            u32 count = p.read32();

            std::vector<File> files;
            files.reserve(count);

            for (u32 i = 0; i < count; ++i)
            {
                File file;

                u32 length = p.read32();
                const u8* ptr = p;
                file.filename.assign(reinterpret_cast<const char*>(ptr), length);
                p += length;

                file.size = p.read64();
                file.checksum = p.read32();

                u32 num_segments = p.read32();
                file.segments.reserve(num_segments);

                for (u32 j = 0; j < num_segments; ++j)
                {
                    File::Segment segment;
                    segment.block = p.read32();
                    segment.offset = p.read64();
                    segment.size = p.read64();
                    file.segments.push_back(segment);
                }

                files.push_back(std::move(file));
            }

            return files;
        }

    } // namespace

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

    std::vector<Block> readBlockArray(ConstMemory memory)
    {
        LittleEndianConstPointer start = memory.address;
        LittleEndianConstPointer p = start;

        u32 magic1 = p.read32();
        if (magic1 != filesystem::HBS_MAGIC1)
        {
            MANGO_EXCEPTION("[hbs] Incorrect block identifier ({:#x}).", magic1);
        }

        u32 version = p.read32();
        MANGO_UNREFERENCED(version);

        u32 count = p.read32();

        std::vector<Block> blocks;
        blocks.reserve(count);

        for (u32 i = 0; i < count; ++i)
        {
            Block block;
            block.offset = p.read64();
            block.compressed = p.read64();
            block.uncompressed = p.read64();
            block.method = p.read32();
            blocks.push_back(block);
        }

        if (size_t(p - start) != memory.size)
        {
            MANGO_EXCEPTION("[hbs] Incorrect block array size.");
        }

        return blocks;
    }

    std::vector<File> readFileArray(ConstMemory memory)
    {
        LittleEndianConstPointer start = memory.address;
        LittleEndianConstPointer p = start;

        u32 magic2 = p.read32();
        if (magic2 != filesystem::HBS_MAGIC2)
        {
            MANGO_EXCEPTION("[hbs] Incorrect file identifier ({:#x}).", magic2);
        }

        u32 version = p.read32();
        MANGO_UNREFERENCED(version);

        u64 compressed = p.read64();
        u64 uncompressed = p.read64();

        ConstMemory source(p, compressed);
        Buffer temp(uncompressed);

        CompressionStatus status = zstd::decompress(temp, source);
        if (status.size != uncompressed)
        {
            MANGO_EXCEPTION("[hbs] Incorrect compressed file array.");
        }

        p += compressed;

        if (size_t(p - start) != memory.size)
        {
            MANGO_EXCEPTION("[hbs] Incorrect file array size.");
        }

        return readFileRecords(temp.data());
    }

} // namespace mango::filesystem::hbs
