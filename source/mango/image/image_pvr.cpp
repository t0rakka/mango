/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/system.hpp>
#include <mango/core/pointer.hpp>
#include <mango/core/string.hpp>
#include <mango/image/image.hpp>

// http://cdn.imgtec.com/sdk-documentation/PVR+File+Format.Specification.Legacy.pdf
// http://cdn.imgtec.com/sdk-documentation/PVR+File+Format.Specification.pdf

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ----------------------------------------------------------------------------
    // EightCC
    // ----------------------------------------------------------------------------

    Format eightcc_to_format(u64 eightcc)
    {
        const u8 cc_order[4] =
        {
            u8(eightcc >> 24),
            u8(eightcc >> 16),
            u8(eightcc >>  8),
            u8(eightcc >>  0),
        };

        const u8 cc_size[4] =
        {
            u8(eightcc >> 56),
            u8(eightcc >> 48),
            u8(eightcc >> 40),
            u8(eightcc >> 32),
        };

        Color size = 0;
        Color offset = 0;
        int bits = 0;

        for (int i = 0; i < 4; ++i)
        {
            int index;
            switch (cc_order[i])
            {
                case 'r': index = 0; break;
                case 'g': index = 1; break;
                case 'b': index = 2; break;
                case 'a': index = 3; break;
                default: index = -1; break;
            }

            if (index >= 0)
            {
                offset[index] = u8(bits);
                size[index] = cc_size[i];
                bits += cc_size[i];
            }
        }

        bits = div_ceil(bits, 8) * 8;
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
        TextureCompression::ASTC_UNORM_4x4,
        TextureCompression::ASTC_UNORM_5x4,
        TextureCompression::ASTC_UNORM_5x5,
        TextureCompression::ASTC_UNORM_6x5,
        TextureCompression::ASTC_UNORM_6x6,
        TextureCompression::ASTC_UNORM_8x5,
        TextureCompression::ASTC_UNORM_8x6,
        TextureCompression::ASTC_UNORM_8x8,
        TextureCompression::ASTC_UNORM_10x5,
        TextureCompression::ASTC_UNORM_10x6,
        TextureCompression::ASTC_UNORM_10x8,
        TextureCompression::ASTC_UNORM_10x10,
        TextureCompression::ASTC_UNORM_12x10,
        TextureCompression::ASTC_UNORM_12x12,
        TextureCompression::ASTC_UNORM_3x3x3,
        TextureCompression::ASTC_UNORM_4x3x3,
        TextureCompression::ASTC_UNORM_4x4x3,
        TextureCompression::ASTC_UNORM_4x4x4,
        TextureCompression::ASTC_UNORM_5x4x4,
        TextureCompression::ASTC_UNORM_5x5x4,
        TextureCompression::ASTC_UNORM_5x5x5,
        TextureCompression::ASTC_UNORM_6x5x5,
        TextureCompression::ASTC_UNORM_6x6x5,
        TextureCompression::ASTC_UNORM_6x6x6,
    };

    struct pvr_type_t
    {
        u8 size : 4;
        u8 sign : 1;
        u8 integer : 1;
        u8 normalized : 1;
    };

    const pvr_type_t typeTable[] =
    {
        { 1, 0, 1, 1 }, // u8 normalized
        { 1, 1, 1, 1 }, // s8 normalized
        { 1, 0, 1, 0 }, // u8
        { 1, 1, 1, 0 }, // s8
        { 2, 0, 1, 1 }, // u16 normalized
        { 2, 1, 1, 1 }, // s16 normalized
        { 2, 0, 1, 0 }, // u16
        { 2, 1, 1, 0 }, // s16
        { 4, 0, 1, 1 }, // u32 normalized
        { 4, 1, 1, 1 }, // s32 normalized
        { 4, 0, 1, 0 }, // u32
        { 4, 1, 1, 0 }, // s32
        { 4, 1, 0, 0 }  // float32
    };

    struct pvr_header3_t
    {
        u32 version;
        u32 flags;
        u64 pixelformat;
        u32 colorspace;
        u32 channeltype;
        u32 height;
        u32 width;
        u32 depth;
        u32 numsurfaces;
        u32 numfaces;
        u32 mipmapcount;
        u32 metadatasize;

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
        int m_surfaces;
        int m_data_offset;
        TextureCompression m_info;
        ImageHeader header;

        void read(ConstMemory memory)
        {
            LittleEndianConstPointer p = memory.address;

            u32 magic = p.read32();
            switch (magic)
            {
                case 44:
                case 52:
                    // version 1 and 2
                    parse_legacy(memory);
                    break;
                case 0x03525650:
                    // version 3, same endianess
                    parse_version3(memory, false);
                    break;
                case 0x50565203:
                    // version 3, different endianess
                    parse_version3(memory, true);
                    break;
                default:
                    header.setError("[ImageDecoder.PVR] Incorrect format version: {:#x}", magic);
                    return;
            }

            header.palette = false;
            header.format  = m_info.format;
            header.compression = m_info.compression;
        }

        void parse_legacy(ConstMemory memory)
        {
            LittleEndianConstPointer p = memory.address;

            u32 header_size = p.read32();
            header.height = p.read32();
            header.width = p.read32();
            header.levels = p.read32() + 1;

            u32 flags = p.read32();
            u8 fmt = flags & 0xff;

            /*
            0x00000100  MIP-Maps are present
            0x00000200  Data is twiddled
            0x00000400  Contains normal data
            0x00000800  Has a border
            0x00001000  Is a cube map (Every 6 surfaces make up one cube map)
            0x00002000  MIP-Maps have debug colouring
            0x00004000  Is a volume (3D) texture (numSurfaces is interpreted as a depth value)
            0x00008000  Alpha channel data is present (PVRTC only)
            */

            printLine(Print::Info, "flags: {:#x}", flags);
            printLine(Print::Info, "format: {:#x}", fmt);

            // compressed block default values
            TextureCompression compression = TextureCompression::NONE;
            Format format;

            // NOTE: these have NOT been tested
            switch (fmt)
            {
                case 0x00: // ARGB 4444
                    format = Format(16, Format::UNORM, Format::ARGB, 4, 4, 4, 4);
                    break;
                case 0x01: // ARGB 1555
                    format = Format(16, Format::UNORM, Format::ARGB, 1, 5, 5, 5);
                    break;
                case 0x02: // RGB 565
                    format = Format(16, Format::UNORM, Format::RGB, 5, 6, 5, 0);
                    break;
                case 0x03: // RGB 555
                    format = Format(16, Format::UNORM, Format::RGB, 5, 5, 5, 0);
                    break;
                case 0x04: // RGB 888
                    format = Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0);
                    break;
                case 0x05: // ARGB 8888
                    format = Format(32, Format::UNORM, Format::ARGB, 8, 8, 8, 8);
                    break;
                case 0x06: // ARGB 8332
                    format = Format(16, Format::UNORM, Format::ARGB, 8, 3, 3, 2);
                    break;
                case 0x07: // I 8
                    format = LuminanceFormat(8, Format::UNORM, 8, 0);
                    break;
                case 0x08: // AI 88
                    format = LuminanceFormat(16, Format::UNORM, 8, 8);
                    break;
                case 0x09: // 1BPP
                    compression = TextureCompression::BITPLANE1;
                    break;
                case 0x0A: // (V,Y1,U,Y0)
                    compression = TextureCompression::YUY2;
                    break;
                case 0x0B: // (Y1,V,Y0,U)
                    compression = TextureCompression::UYVY;
                    break;
                case 0x0C: // PVRTC2
                    compression = TextureCompression::PVRTC_RGBA_2BPP;
                    break;
                case 0x0D: // PVRTC4
                    compression = TextureCompression::PVRTC_RGBA_4BPP;
                    break;
                case 0x10: // ARGB 4444
                    format = Format(16, Format::UNORM, Format::ARGB, 4, 4, 4, 4);
                    break;
                case 0x11: // ARGB 1555
                    format = Format(16, Format::UNORM, Format::ARGB, 1, 5, 5, 5);
                    break;
                case 0x12: // ARGB 8888
                    format = Format(32, Format::UNORM, Format::ARGB, 8, 8, 8, 8);
                    break;
                case 0x13: // RGB 565
                    format = Format(16, Format::UNORM, Format::RGB, 5, 6, 5, 0);
                    break;
                case 0x14: // RGB 555
                    format = Format(16, Format::UNORM, Format::RGB, 5, 5, 5, 0);
                    break;
                case 0x15: // RGB 888
                    format = Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0);
                    break;
                case 0x16: // I 8
                    format = LuminanceFormat(8, Format::UNORM, 8, 0);
                    break;
                case 0x17: // AI 88
                    format = LuminanceFormat(16, Format::UNORM, 8, 8);
                    break;
                case 0x18: // PVRTC2
                    compression = TextureCompression::PVRTC_RGBA_2BPP;
                    break;
                case 0x19: // PVRTC4
                    compression = TextureCompression::PVRTC_RGBA_4BPP;
                    break;
                case 0x1A: // BGRA 8888
                    format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
                    break;
                case 0x20: // DXT1
                    compression = TextureCompression::DXT1;
                    break;
                case 0x21: // DXT2
                    compression = TextureCompression::DXT1;
                    break;
                case 0x22: // DXT3
                    compression = TextureCompression::DXT3;
                    break;
                case 0x23: // DXT4
                    compression = TextureCompression::DXT3;
                    break;
                case 0x24: // DXT5
                    compression = TextureCompression::DXT5;
                    break;
                case 0x25: // RGB 332
                    format = Format(8, Format::UNORM, Format::RGB, 3, 3, 2, 0);
                    break;
                case 0x26: // AL 44
                    format = LuminanceFormat(8, Format::UNORM, 4, 4);
                    break;
                case 0x27: // LVU 655
                    // MANGO TODO
                    break;
                case 0x28: // XLVU 8888
                    // MANGO TODO
                    break;
                case 0x29: // QWVU 8888
                    // MANGO TODO
                    break;
                case 0x2A: // ABGR 2101010
                    format = Format(32, Format::UNORM, Format::ABGR, 2, 10, 10, 10);
                    break;
                case 0x2B: // ARGB 2101010
                    format = Format(32, Format::UNORM, Format::ARGB, 2, 10, 10, 10);
                    break;
                case 0x2C: // AWVU 2101010
                    // MANGO TODO
                    break;
                case 0x2D: // GR 1616
                    format = Format(32, Format::UNORM, Format::GR, 16, 16, 0, 0);
                    break;
                case 0x2E: // VU 1616
                    // MANGO TODO
                    break;
                case 0x2F: // ABGR 16161616
                    format = Format(64, Format::UNORM, Format::ABGR, 16, 16, 16, 16);
                    break;
                case 0x30: // R 16F
                    format = Format(16, Format::FLOAT16, Format::R, 16, 0, 0, 0);
                    break;
                case 0x31: // GR 1616F
                    format = Format(32, Format::FLOAT16, Format::RG, 16, 16, 0, 0);
                    break;
                case 0x32: // ABGR 16161616F
                    format = Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16);
                    break;
                case 0x33: // R 32F
                    format = Format(32, Format::FLOAT32, Format::R, 32, 0, 0, 0);
                    break;
                case 0x34: // GR 3232F
                    format = Format(64, Format::FLOAT32, Format::RG, 32, 32, 0, 0);
                    break;
                case 0x35: // ABGR 32323232F
                    format = Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32);
                    break;
                case 0x36: // ETC
                    compression = TextureCompression::ETC1_RGB;
                    break;
                case 0x40: // A 8
                    format = Format(8, Format::UNORM, Format::A, 8, 0, 0, 0);
                    break;
                case 0x41: // VU 88
                    // MANGO TODO
                    break;
                case 0x42: // L16
                    format = LuminanceFormat(16, Format::UNORM, 16, 0);
                    break;
                case 0x43: // L8
                    format = LuminanceFormat(8, Format::UNORM, 8, 0);
                    break;
                case 0x44: // AL 88
                    format = LuminanceFormat(16, Format::UNORM, 8, 8);
                    break;
                case 0x45: // UYVY
                    compression = TextureCompression::UYVY;
                    break;
                case 0x46: // YUY2
                    compression = TextureCompression::YUY2;
                    break;
            }

            u32 surface_size = p.read32();
            u32 bits_per_pixel = p.read32();

            MANGO_UNREFERENCED(surface_size);
            MANGO_UNREFERENCED(bits_per_pixel);

            u32 mask[4];
            mask[0] = p.read32();
            mask[1] = p.read32();
            mask[2] = p.read32();
            mask[3] = p.read32();
            MANGO_UNREFERENCED(mask);

            if (header_size == 52)
            {
                u32 identifier = p.read32();
                u32 number_of_surfaces = p.read32();

                MANGO_UNREFERENCED(number_of_surfaces);

                if (identifier != 0x21525650)
                {
                    header.setError("[ImageDecoder.PVR] Incorrect format identifier: {:#x}", identifier);
                    return;
                }
            }

            header.depth = 1;
            header.faces = 1;
            m_surfaces = 1;
            m_data_offset = header_size;

            m_info = TextureCompression(compression);
            m_info.format = format;
        }

        void parse_version3(ConstMemory memory, bool swap_header)
        {
            pvr_header3_t pvr;
            std::memcpy(&pvr, memory.address, sizeof(pvr));
            if (swap_header)
            {
                pvr.byteswap_header();
            }

            if (pvr.flags & 0x02)
            {
                header.premultiplied = true;
            }

            // compressed block default values
            m_info = TextureCompression(TextureCompression::NONE);

            if (int(pvr.channeltype) < int(std::size(typeTable)))
            {
                // configure bytes per pixel
                m_info.bytes = typeTable[pvr.channeltype].size;
            }
            else
            {
                header.setError("[ImageDecoder.PVR] Incorrect channeltype: {}", int(pvr.channeltype));
                return;
            }

            if (pvr.pixelformat & 0xffffffff00000000)
            {
                m_info.format = eightcc_to_format(pvr.pixelformat);
                printLine(Print::Info, "eightcc format: {} ({},{},{},{})", m_info.format.bits,
                    m_info.format.size[0],
                    m_info.format.size[1],
                    m_info.format.size[2],
                    m_info.format.size[3]);
            }
            else
            {
                const int formatIndex = int(pvr.pixelformat);

                if (formatIndex < int(std::size(formatTable)))
                {
                    printLine(Print::Info, "pvr.pixelformat: {}", formatIndex);

                    TextureCompression compression = formatTable[formatIndex];
                    TextureCompression info(compression);

                    if (info.compression != TextureCompression::NONE)
                    {
                        // compressed format is supported; store compressed block information
                        m_info = info;
                    }
                }
                else
                {
                    header.setError("[ImageDecoder.PVR] Incorrect pixelformat: {}", formatIndex);
                    return;
                }
            }

            header.width  = pvr.width;
            header.height = pvr.height;
            header.depth  = pvr.depth;
            header.faces  = pvr.numfaces;
            header.levels = pvr.mipmapcount;

            m_surfaces = pvr.numsurfaces;

            if (m_surfaces > 1)
            {
                header.setError("[ImageDecoder.PVR] Surface arrays not supported.");
                return;
            }

            if (header.faces != 1 && header.faces != 6)
            {
                header.setError("[ImageDecoder.PVR] Incorrect number of faces: {}", header.faces);
                return;
            }

            switch (pvr.colorspace)
            {
                case 0:
                    header.linear = true;
                    break;
                case 1:
                    header.linear = false;
                    break;
                default:
                    header.setError("[ImageDecoder.PVR] Incorrect colorspace: {}", pvr.colorspace);
                    return;
            }

            m_data_offset = sizeof(pvr_header3_t) + pvr.metadatasize - 4;
        }

        ConstMemory getMemory(ConstMemory memory, int level, int depth, u32 face) const
        {
            const u8* p = memory.address + m_data_offset;

            ConstMemory data;

            for (int iLevel = 0; iLevel < header.levels; ++iLevel)
            {
                // compute mip level dimensions
                int width = std::max(1, header.width >> iLevel);
                int height = std::max(1, header.height >> iLevel);

                // align to next block size
                width = (width + m_info.width - 1) & ~(m_info.width - 1);
                height = (height + m_info.height - 1) & ~(m_info.height - 1);

                // compute mip level size in bytes
                int size = (width / m_info.width) * (height / m_info.height) * m_info.bytes;

                for (int iSurface = 0; iSurface < m_surfaces; ++iSurface)
                {
                    for (int iFace = 0; iFace < header.faces; ++iFace)
                    {
                        for (int iDepth = 0; iDepth < header.depth; ++iDepth)
                        {
                            if (iLevel == level && iDepth == depth && iFace == int(face) && iSurface == 0)
                            {
                                // Store selected address
                                data = ConstMemory(p, size);
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

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        HeaderPVR m_pvr_header;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            m_pvr_header.read(memory);
            header = m_pvr_header.header;
        }

        ~Interface()
        {
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            return m_pvr_header.getMemory(m_memory, level, depth, face);
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);

            ImageDecodeStatus status;

            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            ConstMemory data = m_pvr_header.getMemory(m_memory, level, depth, face);

            int width = std::max(1, header.width >> level);
            int height = std::max(1, header.height >> level);

            if (m_pvr_header.m_info.compression != TextureCompression::NONE)
            {
                TextureCompression::Status cs = m_pvr_header.m_info.decompress(dest, data);

                status.info = cs.info;
                status.success = cs.success;
                status.direct = cs.direct;
            }
            else
            {
                size_t stride = width * m_pvr_header.m_info.format.bytes();
                Surface source(width, height, m_pvr_header.m_info.format, stride, data.address);
                dest.blit(0, 0, source);
            }

            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecPVR()
    {
        registerImageDecoder(createInterface, ".pvr");
    }

} // namespace mango::image
