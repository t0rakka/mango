/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/system.hpp>
#include <mango/core/pointer.hpp>
#include <mango/core/exception.hpp>
#include <mango/image/image.hpp>

#define ID "ImageStream.PVR: "

// TODO: add support for pvr header v2

namespace
{

    using namespace mango;

    // ----------------------------------------------------------------------------
    // EightCC
    // ----------------------------------------------------------------------------

    Format eightcc_to_format(uint64 eightcc)
    {
        char* p = reinterpret_cast<char*>(&eightcc);

        int order[4];
        PackedColor size;
        PackedColor offset;

        for (int i = 0; i < 4; ++i)
        {
            int index;
            switch (p[3 - i])
            {
                default:
                case 'r': index = 0; break;
                case 'g': index = 1; break;
                case 'b': index = 2; break;
                case 'a': index = 3; break;
            }

            order[i] = index;
            size[index] = p[7 - i];
        }

        int bits = 0;

        for (int i = 0; i < 4; ++i)
        {
            int component = order[i];
            offset[component] = uint8(bits);
            bits += size[component];
        }

        bits = round_to_next(bits, 8) * 8;
        return Format(bits, Format::UNORM, size, offset);
    }

    // ----------------------------------------------------------------------------
    // header
    // ----------------------------------------------------------------------------

    const TextureCompression formatTable[] =
    {
        TextureCompression::PVRTC_RGB_2BPP,
        TextureCompression::PVRTC_RGBA_2BPP,
        TextureCompression::PVRTC_RGB_4BPP,
        TextureCompression::PVRTC_RGBA_4BPP,
        TextureCompression::PVRTC2_RGBA_2BPP,
        TextureCompression::PVRTC2_RGBA_4BPP,
        TextureCompression::ETC1_RGB,
        TextureCompression::DXT1,
        TextureCompression::DXT1,
        TextureCompression::DXT3,
        TextureCompression::DXT3,
        TextureCompression::DXT5,
        TextureCompression::RGTC1_RED,
        TextureCompression::RGTC2_RG,
        TextureCompression::BPTC_RGB_UNSIGNED_FLOAT,
        TextureCompression::BPTC_RGBA_UNORM,
        TextureCompression::UYVY,
        TextureCompression::NONE, // YUV2
        TextureCompression::NONE, // BW1BPP
        TextureCompression::RGB9_E5,
        TextureCompression::R8G8B8G8,
        TextureCompression::G8R8G8B8,
        TextureCompression::ETC2_RGB,
        TextureCompression::ETC2_RGBA,
        TextureCompression::ETC2_RGB_ALPHA1,
        TextureCompression::EAC_R11,
        TextureCompression::EAC_SIGNED_R11,
        TextureCompression::EAC_RG11,
        TextureCompression::EAC_SIGNED_RG11,
        TextureCompression::ASTC_RGBA_4x4,
        TextureCompression::ASTC_RGBA_5x4,
        TextureCompression::ASTC_RGBA_5x5,
        TextureCompression::ASTC_RGBA_6x5,
        TextureCompression::ASTC_RGBA_6x6,
        TextureCompression::ASTC_RGBA_8x5,
        TextureCompression::ASTC_RGBA_8x6,
        TextureCompression::ASTC_RGBA_8x8,
        TextureCompression::ASTC_RGBA_10x5,
        TextureCompression::ASTC_RGBA_10x6,
        TextureCompression::ASTC_RGBA_10x8,
        TextureCompression::ASTC_RGBA_10x10,
        TextureCompression::ASTC_RGBA_12x10,
        TextureCompression::ASTC_RGBA_12x12,
        TextureCompression::ASTC_RGBA_3x3x3,
        TextureCompression::ASTC_RGBA_4x3x3,
        TextureCompression::ASTC_RGBA_4x4x3,
        TextureCompression::ASTC_RGBA_4x4x4,
        TextureCompression::ASTC_RGBA_5x4x4,
        TextureCompression::ASTC_RGBA_5x5x4,
        TextureCompression::ASTC_RGBA_5x5x5,
        TextureCompression::ASTC_RGBA_6x5x5,
        TextureCompression::ASTC_RGBA_6x6x5,
        TextureCompression::ASTC_RGBA_6x6x6,
    };

    const int formatTableSize = sizeof(formatTable) / sizeof(formatTable[0]);

    struct pvr_type_t
    {
        int size : 8;
        int sign : 1;
        int integer : 1;
        int normalized : 1;
    };

    const pvr_type_t typeTable[] =
    {
        { 1, 0, 1, 1 }, // uint8 normalized
        { 1, 1, 1, 1 }, // int8 normalized
        { 1, 0, 1, 0 }, // uint8
        { 1, 1, 1, 0 }, // int8
        { 2, 0, 1, 1 }, // uint16 normalized
        { 2, 1, 1, 1 }, // int16 normalized
        { 2, 0, 1, 0 }, // uint16
        { 2, 1, 1, 0 }, // int16
        { 4, 0, 1, 1 }, // uint32 normalized
        { 4, 1, 1, 1 }, // int32 normalized
        { 4, 0, 1, 0 }, // uint32
        { 4, 1, 1, 0 }, // int32
        { 4, 1, 0, 0 }  // float32
    };

    const int typeTableSize = sizeof(typeTable) / sizeof(typeTable[0]);

    struct pvr_header3_t
    {
        uint32 version;
        uint32 flags;
        uint64 pixelformat;
        uint32 colorspace;
        uint32 channeltype;
        uint32 height;
        uint32 width;
        uint32 depth;
        uint32 numsurfaces;
        uint32 numfaces;
        uint32 mipmapcount;
        uint32 metadatasize;

        void byteswap_header()
        {
            flags = byteswap(flags);
            pixelformat = byteswap(pixelformat);
            colorspace = byteswap(colorspace);
            channeltype = byteswap(channeltype);
            height = byteswap(height);
            width = byteswap(width);
            depth = byteswap(depth);
            numsurfaces = byteswap(numsurfaces);
            numfaces = byteswap(numfaces);
            mipmapcount = byteswap(mipmapcount);
            metadatasize = byteswap(metadatasize);
        }
    };

    struct HeaderPVR
    {
        int m_width;
        int m_height;
        int m_depth;
        int m_surfaces;
        int m_faces;
        int m_mipmaps;
        int m_dataOffset;
        TextureCompressionInfo m_info;

        void read(Memory memory)
        {
            pvr_header3_t pvr;

            std::memcpy(&pvr, memory.address, sizeof(pvr));

            if (pvr.version == 0x03525650)
            {
                // same endian
            }
            else if (pvr.version == 0x50565203)
            {
                // different endian
                pvr.byteswap_header();
            }
            else
            {
                MANGO_EXCEPTION(ID"Incorrect header version.");
            }

            if (pvr.flags & 0x02)
            {
                // NOTE: pre-multiplied alpha
            }

            // compressed block default values
            m_info = TextureCompressionInfo(TextureCompression::NONE);

            if (int(pvr.channeltype) < typeTableSize)
            {
                // configure bytes per pixel
                m_info.bytes = typeTable[pvr.channeltype].size;
            }
            else
            {
                MANGO_EXCEPTION(ID"Incorrect channeltype.");
            }

            if (pvr.pixelformat & 0xffffffff00000000)
            {
                m_info.format = eightcc_to_format(pvr.pixelformat);
            }
            else
            {
                if (int(pvr.pixelformat) < formatTableSize)
                {
                    // TODO: support for COMPRESSED_NONE entries in the table (packed pixel formats, yuv, shared exponent, 1-bit b/w)
                    TextureCompression compression = formatTable[pvr.pixelformat];

                    TextureCompressionInfo info(compression);

                    if (info.compression != TextureCompression::NONE)
                    {
                        // compressed format is supported; store compressed block information
                        m_info = info;
                    }
                }
                else
                {
                    MANGO_EXCEPTION(ID"Incorrect pixelformat.");
                }
            }

            m_width    = pvr.width;
            m_height   = pvr.height;
            m_depth    = pvr.depth;
            m_surfaces = pvr.numsurfaces;
            m_faces    = pvr.numfaces;
            m_mipmaps  = pvr.mipmapcount;

            if (m_surfaces > 1)
            {
                MANGO_EXCEPTION(ID"Surface arrays not supported.");
            }

            if (m_faces != 1 && m_faces != 6)
            {
                MANGO_EXCEPTION(ID"Incorrect number of faces.");
            }

            switch (pvr.colorspace)
            {
                case 0:
                    // NOTE: linear
                    break;
                case 1:
                    // NOTE: sRGB
                    break;
                default:
                    MANGO_EXCEPTION(ID"Incorrect colorspace.");
            }

            m_dataOffset = sizeof(pvr_header3_t) + pvr.metadatasize - 4;
        }

        Memory getMemory(Memory memory, int level, int depth, uint32 face) const
        {
            uint8* p = memory.address + m_dataOffset;

            Memory data;

            for (int iLevel = 0; iLevel < m_mipmaps; ++iLevel)
            {
                // compute mip level dimensions
                int width = std::max(1, m_width >> iLevel);
                int height = std::max(1, m_height >> iLevel);

                // align to next block size
                width = (width + m_info.width - 1) & ~(m_info.width - 1);
                height = (height + m_info.height - 1) & ~(m_info.height - 1);

                // compute mip level size in bytes
                int size = (width / m_info.width) * (height / m_info.height) * m_info.bytes;

                for (int iSurface = 0; iSurface < m_surfaces; ++iSurface)
                {
                    for (int iFace = 0; iFace < m_faces; ++iFace)
                    {
                        for (int iDepth = 0; iDepth < m_depth; ++iDepth)
                        {
                            if (iLevel == level && iDepth == depth && iFace == int(face) && iSurface == 0)
                            {
                                // Store selected address
                                data.address = p;
                                data.size = size;
                            }

                            p += size;
                        }
                    }
                }
            }

            return data;
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------
    
    struct Interface : ImageDecoderInterface
    {
        Memory m_memory;
        HeaderPVR m_header;
        
        Interface(Memory memory)
            : m_memory(memory)
        {
            m_header.read(memory);
        }
        
        ~Interface()
        {
        }
        
        ImageHeader header() override
        {
            ImageHeader header;

            header.width  = m_header.m_width;
            header.height = m_header.m_height;
            header.depth  = m_header.m_depth;
            header.levels = m_header.m_mipmaps;
            header.faces  = m_header.m_faces;
            header.format = m_header.m_info.format;
            header.compression = m_header.m_info.compression;

            return header;
        }

        Memory memory(int level, int depth, int face) override
        {
            return m_header.getMemory(m_memory, level, depth, face);
        }

        void decode(Surface& dest, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(palette);

            Memory data = m_header.getMemory(m_memory, level, depth, face);

            int width = std::max(1, m_header.m_width >> level);
            int height = std::max(1, m_header.m_height >> level);

            if (m_header.m_info.compression == TextureCompression::NONE)
            {
                int stride = width * m_header.m_info.bytes; // .pvr parser stores bytesPerPixel in block information
                Surface source(width, height, m_header.m_info.format, stride, data.address);
                dest.blit(0, 0, source);
            }
            else
            {
                m_header.m_info.decompress(dest, data);
            }
        }
    };

    ImageDecoderInterface* createInterface(Memory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango
{

    void registerPVR()
    {
        registerImageDecoder(createInterface, "pvr");
    }

} // namespace mango
