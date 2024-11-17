/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ----------------------------------------------------------------------------
    // FormatASTC
    // ----------------------------------------------------------------------------

    struct FormatASTC
    {
        int xblock;
        int yblock;
        u32 compression;
    }
    const g_astc_table[] =
    {
        {  4,  4, TextureCompression::ASTC_UNORM_4x4 },
        {  5,  4, TextureCompression::ASTC_UNORM_5x4 },
        {  5,  5, TextureCompression::ASTC_UNORM_5x5 },
        {  6,  5, TextureCompression::ASTC_UNORM_6x5 },
        {  6,  6, TextureCompression::ASTC_UNORM_6x6 },
        {  8,  5, TextureCompression::ASTC_UNORM_8x5 },
        {  8,  6, TextureCompression::ASTC_UNORM_8x6 },
        {  8,  8, TextureCompression::ASTC_UNORM_8x8 },
        { 10,  5, TextureCompression::ASTC_UNORM_10x5 },
        { 10,  6, TextureCompression::ASTC_UNORM_10x6 },
        { 10,  8, TextureCompression::ASTC_UNORM_10x8 },
        { 10, 10, TextureCompression::ASTC_UNORM_10x10 },
        { 12, 10, TextureCompression::ASTC_UNORM_12x10 },
        { 12, 12, TextureCompression::ASTC_UNORM_12x12 }
    };

    u32 select_astc_format(int width, int height)
    {
        for (const auto& astc : g_astc_table)
        {
            if (width == astc.xblock && height == astc.yblock)
            {
                return astc.compression;
            }
        }

        return TextureCompression::NONE;
    }

    u32 read24(LittleEndianConstPointer& p)
    {
        u32 value = (p[2] << 16) | (p[1] << 8) | p[0];
        p += 3;
        return value;
    }

    void write24(Stream& stream, u32 value)
    {
        UnsignedInt24 u = value;
        stream.write(u, 3);
    }

    // ----------------------------------------------------------------------------
    // HeaderASTC
    // ----------------------------------------------------------------------------

    struct HeaderASTC
    {
        int xblock;
        int yblock;
        int zblock;
        ImageHeader header;

        void read(LittleEndianConstPointer& p)
        {
            u32 magic = p.read32();
            if (magic != 0x5ca1ab13)
            {
                header.setError("[ImageDecoder.ASTC] Incorrect header.");
                return;
            }

            xblock = p.read8();
            yblock = p.read8();
            zblock = p.read8();
            header.width = read24(p);
            header.height = read24(p);
            header.depth = read24(p);

            u32 compression = select_astc_format(xblock, yblock);

            if (compression == TextureCompression::NONE)
            {
                header.setError("[ImageDecoder.ASTC] Incorrect block size.");
                return;
            }

            TextureCompression info(compression);
            header.format = info.format;
            header.linear = info.isLinear();
            header.compression = compression;
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        HeaderASTC m_astc_header;
        ConstMemory m_data;

        Interface(ConstMemory memory)
        {
            LittleEndianConstPointer p = memory.address;
            m_astc_header.read(p);
            m_data = ConstMemory(p, memory.address + memory.size - p);
            header = m_astc_header.header;
        }

        ~Interface()
        {
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            return m_data;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            TextureCompression info(header.compression);

            if (info.compression != TextureCompression::NONE)
            {
                Surface temp(dest, false);
                static_cast<Status&>(status) = info.decompress(temp, m_data);
            }

            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
        return x;
    }

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status;

        int block_width = options.astc_block_width;
        int block_height = options.astc_block_height;

        u32 compression = select_astc_format(block_width, block_height);
        if (compression == TextureCompression::NONE)
        {
            status.setError("[ImageEncoder.ASTC] Incorrect block size: {} x {}", block_width, block_height);
            return status;
        }

        TextureCompression texcomp(compression);

        Surface temp(surface, false);

        u64 bytes = texcomp.getBlockBytes(temp.width, temp.height);
        Buffer buffer(bytes);

        auto compressionStatus = texcomp.compress(buffer, temp);
        MANGO_UNREFERENCED(compressionStatus);

        LittleEndianStream output(stream);

        // write header
        output.write32(0x5ca1ab13);
        output.write8(block_width);
        output.write8(block_height);
        output.write8(0);
        write24(output, temp.width);
        write24(output, temp.height);
        write24(output, 0);

        // write compressed blocks
        output.write(buffer);

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecASTC()
    {
        registerImageDecoder(createInterface, ".astc");
        registerImageEncoder(imageEncode, ".astc");
    }

} // namespace mango::image
