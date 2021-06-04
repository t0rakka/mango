/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
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
        TextureCompression compression;
    }
    const g_astc_table[] =
    {
        {  4,  4, TextureCompression::ASTC_RGBA_4x4 },
        {  5,  4, TextureCompression::ASTC_RGBA_5x4 },
        {  5,  5, TextureCompression::ASTC_RGBA_5x5 },
        {  6,  5, TextureCompression::ASTC_RGBA_6x5 },
        {  6,  6, TextureCompression::ASTC_RGBA_6x6 },
        {  8,  5, TextureCompression::ASTC_RGBA_8x5 },
        {  8,  6, TextureCompression::ASTC_RGBA_8x6 },
        {  8,  8, TextureCompression::ASTC_RGBA_8x8 },
        { 10,  5, TextureCompression::ASTC_RGBA_10x5 },
        { 10,  6, TextureCompression::ASTC_RGBA_10x6 },
        { 10,  8, TextureCompression::ASTC_RGBA_10x8 },
        { 10, 10, TextureCompression::ASTC_RGBA_10x10 },
        { 12, 10, TextureCompression::ASTC_RGBA_12x10 },
        { 12, 12, TextureCompression::ASTC_RGBA_12x12 }
    };

    TextureCompression select_astc_format(int width, int height)
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

    // ----------------------------------------------------------------------------
    // HeaderASTC
    // ----------------------------------------------------------------------------

    struct HeaderASTC
    {
        int xblock;
        int yblock;
        int zblock;
        ImageHeader header;

        u32 read24(LittleEndianConstPointer& p) const
        {
            u32 value = (p[2] << 16) | (p[1] << 8) | p[0];
            p += 3;
            return value;
        }

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

            header.compression = select_astc_format(xblock, yblock);
            if (header.compression == TextureCompression::NONE)
            {
                header.setError("[ImageDecoder.ASTC] Incorrect block size.");
                return;
            }

            TextureCompressionInfo info(header.compression);
            header.format = info.format;
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        HeaderASTC m_header;
        ConstMemory m_data;

        Interface(ConstMemory memory)
        {
            LittleEndianConstPointer p = memory.address;
            m_header.read(p);
            m_data = ConstMemory(p, memory.address + memory.size - p);
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header.header;
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

            if (!m_header.header.success)
            {
                status.setError(m_header.header.info);
                return status;
            }

            TextureCompressionInfo info(m_header.header.compression);

            if (info.compression != TextureCompression::NONE)
            {
                TextureCompressionStatus cs = info.decompress(dest, m_data);

                status.info = cs.info;
                status.success = cs.success;
                status.direct = cs.direct;
            }

            return status;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageDecoderASTC()
    {
        registerImageDecoder(createInterface, ".astc");
    }

} // namespace mango::image
