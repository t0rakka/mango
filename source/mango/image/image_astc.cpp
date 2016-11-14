/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/exception.hpp>
#include <mango/image/image.hpp>

#define ID "ImageStream.ASTC: "

namespace
{
    using namespace mango;

    // ----------------------------------------------------------------------------
    // ASTC formats
    // ----------------------------------------------------------------------------

    struct astc_t
    {
        int xblock;
        int yblock;
        TextureCompression compression;
    };

    const astc_t astcTable[] =
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

    const int astcTableSize = sizeof(astcTable) / sizeof(astcTable[0]);

    TextureCompression select_astc_format(int width, int height)
    {
        for (int i = 0; i < astcTableSize; ++i)
        {
            if (width == astcTable[i].xblock && height == astcTable[i].yblock)
            {
                return astcTable[i].compression;
            }
        }

        return TextureCompression::NONE;
    }

    // ----------------------------------------------------------------------------
    // header
    // ----------------------------------------------------------------------------

    struct HeaderASTC
    {
        int xblock;
        int yblock;
        int zblock;
        int width;
        int height;
        int depth;
        TextureCompression compression;

        uint32 read24(LittleEndianConstPointer& p) const
        {
            uint32 value = (p[2] << 16) | (p[1] << 8) | p[0];
            p += 3;
            return value;
        }

        void read(LittleEndianConstPointer& p)
        {
            uint32 magic = p.read32();
            if (magic != 0x5ca1ab13)
            {
                MANGO_EXCEPTION(ID"Incorrect header.");
            }

            xblock = p.read8();
            yblock = p.read8();
            zblock = p.read8();
            width = read24(p);
            height = read24(p);
            depth = read24(p);

            compression = select_astc_format(xblock, yblock);
            if (compression == TextureCompression::NONE)
            {
                MANGO_EXCEPTION(ID"Incorrect block size.");
            }
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        HeaderASTC m_header;
        Memory m_data;

        Interface(const Memory& memory)
        {
            LittleEndianConstPointer p = memory.address;
            m_header.read(p);
            m_data = Memory(p, memory.address + memory.size - p);
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            TextureCompressionInfo info(m_header.compression);

            ImageHeader header;

            header.width  = m_header.width;
            header.height = m_header.height;
            header.depth  = 0;
            header.levels = 0;
            header.faces  = 0;
            header.format = info.format;
            header.compression = info.compression;

            return header;
        }

        Memory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            return m_data;
        }

        void decode(Surface& dest, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            TextureCompressionInfo info(m_header.compression);

            if (info.compression != TextureCompression::NONE)
            {
                info.decompress(dest, m_data);
            }
        }
    };

    ImageDecoderInterface* createInterface(const Memory& memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango
{

    void registerASTC()
    {
        registerImageDecoder(createInterface, "astc");
    }

} // namespace mango
