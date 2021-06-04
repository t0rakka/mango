/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstring>
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // KTX Format Specification:
    // http://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/

    // OpenGL glTexImage2D specification:
    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml

    // ----------------------------------------------------------------------------
    // format conversion
    // ----------------------------------------------------------------------------

    enum : u32
    {
        KTX_BYTE                          = 0x1400,
        KTX_UNSIGNED_BYTE                 = 0x1401,
        KTX_SHORT                         = 0x1402,
        KTX_UNSIGNED_SHORT                = 0x1403,
        KTX_INT                           = 0x1404,
        KTX_UNSIGNED_INT                  = 0x1405,
        KTX_FLOAT                         = 0x1406,
        KTX_HALF_FLOAT                    = 0x140B,

        KTX_UNSIGNED_BYTE_3_3_2           = 0x8032,
        KTX_UNSIGNED_SHORT_4_4_4_4        = 0x8033,
        KTX_UNSIGNED_SHORT_5_5_5_1        = 0x8034,
        KTX_UNSIGNED_INT_8_8_8_8          = 0x8035,
        KTX_UNSIGNED_INT_10_10_10_2       = 0x8036,
        KTX_UNSIGNED_BYTE_2_3_3_REV       = 0x8362,
        KTX_UNSIGNED_SHORT_5_6_5          = 0x8363,
        KTX_UNSIGNED_SHORT_5_6_5_REV      = 0x8364,
        KTX_UNSIGNED_SHORT_4_4_4_4_REV    = 0x8365,
        KTX_UNSIGNED_SHORT_1_5_5_5_REV    = 0x8366,
        KTX_UNSIGNED_INT_8_8_8_8_REV      = 0x8367,
        KTX_UNSIGNED_INT_2_10_10_10_REV   = 0x8368,

        KTX_R11F_G11F_B10F                = 0x8C3A,
        KTX_UNSIGNED_INT_10F_11F_11F_REV  = 0x8C3B,
        KTX_RGB9_E5                       = 0x8C3D,
        KTX_UNSIGNED_INT_5_9_9_9_REV      = 0x8C3E,
        KTX_INT_2_10_10_10_REV            = 0x8D9F,

        KTX_RED                           = 0x1903,
        KTX_GREEN                         = 0x1904,
        KTX_BLUE                          = 0x1905,
        KTX_RG                            = 0x8227,
        KTX_RGB                           = 0x1907,
        KTX_RGBA                          = 0x1908,
        KTX_BGR                           = 0x80E0,
        KTX_BGRA                          = 0x80E1,
        //KTX_ABGR_EXT                      = 0x8000,

        KTX_RED_INTEGER                   = 0x8D94,
        KTX_GREEN_INTEGER                 = 0x8D95,
        KTX_BLUE_INTEGER                  = 0x8D96,
        KTX_RG_INTEGER                    = 0x8228,
        KTX_RGB_INTEGER                   = 0x8D98,
        KTX_RGBA_INTEGER                  = 0x8D99,
        KTX_BGR_INTEGER                   = 0x8D9A,
        KTX_BGRA_INTEGER                  = 0x8D9B,

#if 0 // sized internal format stuff; not needed here...
        KTX_R8                            = 0x8229,
        KTX_R16                           = 0x822A,
        KTX_RG8                           = 0x822B,
        KTX_RG16                          = 0x822C,
        KTX_R16F                          = 0x822D,
        KTX_R32F                          = 0x822E,
        KTX_RG16F                         = 0x822F,
        KTX_RG32F                         = 0x8230,
        KTX_R8I                           = 0x8231,
        KTX_R8UI                          = 0x8232,
        KTX_R16I                          = 0x8233,
        KTX_R16UI                         = 0x8234,
        KTX_R32I                          = 0x8235,
        KTX_R32UI                         = 0x8236,
        KTX_RG8I                          = 0x8237,
        KTX_RG8UI                         = 0x8238,
        KTX_RG16I                         = 0x8239,
        KTX_RG16UI                        = 0x823A,
        KTX_RG32I                         = 0x823B,
        KTX_RG32UI                        = 0x823C,

        KTX_R8_SNORM                      = 0x8F94,
        KTX_RG8_SNORM                     = 0x8F95,
        KTX_RGB8_SNORM                    = 0x8F96,
        KTX_RGBA8_SNORM                   = 0x8F97,
        KTX_R16_SNORM                     = 0x8F98,
        KTX_RG16_SNORM                    = 0x8F99,
        KTX_RGB16_SNORM                   = 0x8F9A,
        KTX_RGBA16_SNORM                  = 0x8F9B,

        KTX_R3_G3_B2                      = 0x2A10,
        KTX_RGB4                          = 0x804F,
        KTX_RGB5                          = 0x8050,
        KTX_RGB8                          = 0x8051,
        KTX_RGB10                         = 0x8052,
        KTX_RGB12                         = 0x8053,
        KTX_RGB16                         = 0x8054,
        KTX_RGBA2                         = 0x8055,
        KTX_RGBA4                         = 0x8056,
        KTX_RGB5_A1                       = 0x8057,
        KTX_RGBA8                         = 0x8058,
        KTX_RGB10_A2                      = 0x8059,
        KTX_RGBA12                        = 0x805A,
        KTX_RGBA16                        = 0x805B,

        KTX_SRGB                          = 0x8C40,
        KTX_SRGB8                         = 0x8C41,
        KTX_SRGB_ALPHA                    = 0x8C42,
        KTX_SRGB8_ALPHA8                  = 0x8C43,
        KTX_COMPRESSED_SRGB               = 0x8C48,
        KTX_COMPRESSED_SRGB_ALPHA         = 0x8C49,
        KTX_COMPRESSED_RED                = 0x8225,
        KTX_COMPRESSED_RG                 = 0x8226,
        KTX_RGBA32F                       = 0x8814,
        KTX_RGB32F                        = 0x8815,
        KTX_RGBA16F                       = 0x881A,
        KTX_RGB16F                        = 0x881B,

        KTX_RGBA32UI                      = 0x8D70,
        KTX_RGB32UI                       = 0x8D71,
        KTX_RGBA16UI                      = 0x8D76,
        KTX_RGB16UI                       = 0x8D77,
        KTX_RGBA8UI                       = 0x8D7C,
        KTX_RGB8UI                        = 0x8D7D,
        KTX_RGBA32I                       = 0x8D82,
        KTX_RGB32I                        = 0x8D83,
        KTX_RGBA16I                       = 0x8D88,
        KTX_RGB16I                        = 0x8D89,
        KTX_RGBA8I                        = 0x8D8E,
        KTX_RGB8I                         = 0x8D8F,
        KTX_RGB565                        = 0x8D62,
#endif
    };

#if 0
    enum : u32
    {
        KTX_COMPRESSED_RGB                            = 0x84ED,
        KTX_COMPRESSED_RGBA                           = 0x84EE,

        KTX_COMPRESSED_RED_RGTC1                      = 0x8DBB,
        KTX_COMPRESSED_SIGNED_RED_RGTC1               = 0x8DBC,
        KTX_COMPRESSED_RG_RGTC2                       = 0x8DBD,
        KTX_COMPRESSED_SIGNED_RG_RGTC2                = 0x8DBE,

        KTX_COMPRESSED_RGBA_BPTC_UNORM                = 0x8E8C,
        KTX_COMPRESSED_SRGB_ALPHA_BPTC_UNORM          = 0x8E8D,
        KTX_COMPRESSED_RGB_BPTC_SIGNED_FLOAT          = 0x8E8E,
        KTX_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT        = 0x8E8F,

        KTX_COMPRESSED_RGB8_ETC2                      = 0x9274,
        KTX_COMPRESSED_SRGB8_ETC2                     = 0x9275,
        KTX_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2  = 0x9276,
        KTX_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9277,
        KTX_COMPRESSED_RGBA8_ETC2_EAC                 = 0x9278,
        KTX_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC          = 0x9279,
        KTX_COMPRESSED_R11_EAC                        = 0x9270,
        KTX_COMPRESSED_SIGNED_R11_EAC                 = 0x9271,
        KTX_COMPRESSED_RG11_EAC                       = 0x9272,
        KTX_COMPRESSED_SIGNED_RG11_EAC                = 0x9273,

        KTX_COMPRESSED_RGBA_ASTC_4x4_KHR              = 0x93B0,
        KTX_COMPRESSED_RGBA_ASTC_5x4_KHR              = 0x93B1,
        KTX_COMPRESSED_RGBA_ASTC_5x5_KHR              = 0x93B2,
        KTX_COMPRESSED_RGBA_ASTC_6x5_KHR              = 0x93B3,
        KTX_COMPRESSED_RGBA_ASTC_6x6_KHR              = 0x93B4,
        KTX_COMPRESSED_RGBA_ASTC_8x5_KHR              = 0x93B5,
        KTX_COMPRESSED_RGBA_ASTC_8x6_KHR              = 0x93B6,
        KTX_COMPRESSED_RGBA_ASTC_8x8_KHR              = 0x93B7,
        KTX_COMPRESSED_RGBA_ASTC_10x5_KHR             = 0x93B8,
        KTX_COMPRESSED_RGBA_ASTC_10x6_KHR             = 0x93B9,
        KTX_COMPRESSED_RGBA_ASTC_10x8_KHR             = 0x93BA,
        KTX_COMPRESSED_RGBA_ASTC_10x10_KHR            = 0x93BB,
        KTX_COMPRESSED_RGBA_ASTC_12x10_KHR            = 0x93BC,
        KTX_COMPRESSED_RGBA_ASTC_12x12_KHR            = 0x93BD,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR      = 0x93D0,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR      = 0x93D1,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR      = 0x93D2,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR      = 0x93D3,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR      = 0x93D4,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR      = 0x93D5,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR      = 0x93D6,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR      = 0x93D7,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR     = 0x93D8,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR     = 0x93D9,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR     = 0x93DA,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR    = 0x93DB,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR    = 0x93DC,
        KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR    = 0x93DD,

        KTX_COMPRESSED_RGB_S3TC_DXT1_EXT              = 0x83F0,
        KTX_COMPRESSED_RGBA_S3TC_DXT1_EXT             = 0x83F1,
        KTX_COMPRESSED_RGBA_S3TC_DXT3_EXT             = 0x83F2,
        KTX_COMPRESSED_RGBA_S3TC_DXT5_EXT             = 0x83F3,
    };
#endif

    bool resolve_format(Format& format, TextureCompression& compression, u32 gltype, u32 glformat)
    {
        Format::Type type = Format::NONE;
        Format::Order order = Format::RGBA;
        int components = 0;
        bool valid = true;

        switch (glformat)
        {
            case KTX_RED:
                type = Format::UNORM;
                order = Format::R;
                components = 1;
                break;
            case KTX_GREEN:
                type = Format::UNORM;
                order = Format::G;
                components = 1;
                break;
            case KTX_BLUE:
                type = Format::UNORM;
                order = Format::B;
                components = 1;
                break;
            case KTX_RG:
                type = Format::UNORM;
                order = Format::RG;
                components = 2;
                break;
            case KTX_RGB:
                type = Format::UNORM;
                order = Format::RGB;
                components = 3;
                break;
            case KTX_BGR:
                type = Format::UNORM;
                order = Format::BGR;
                components = 3;
                break;
            case KTX_RGBA:
                type = Format::UNORM;
                order = Format::RGBA;
                components = 4;
                break;
            case KTX_BGRA:
                type = Format::UNORM;
                order = Format::BGRA;
                components = 4;
                break;

            case KTX_RED_INTEGER:
                type = Format::UINT;
                order = Format::R;
                components = 1;
                break;
            case KTX_GREEN_INTEGER:
                type = Format::UINT;
                order = Format::G;
                components = 1;
                break;
            case KTX_BLUE_INTEGER:
                type = Format::UINT;
                order = Format::B;
                components = 1;
                break;
            case KTX_RG_INTEGER:
                type = Format::UINT;
                order = Format::RG;
                components = 2;
                break;
            case KTX_RGB_INTEGER:
                type = Format::UINT;
                order = Format::RGB;
                components = 3;
                break;
            case KTX_BGR_INTEGER:
                type = Format::UINT;
                order = Format::BGR;
                components = 3;
                break;
            case KTX_RGBA_INTEGER:
                type = Format::UINT;
                order = Format::RGBA;
                components = 4;
                break;
            case KTX_BGRA_INTEGER:
                type = Format::UINT;
                order = Format::BGRA;
                components = 4;
                break;

            default:
                valid = false;
                break;
        }

        int size = 0;

        switch (gltype)
        {
            case KTX_BYTE:
                size = 8;
                if (type == Format::UNORM) type = Format::SNORM;
                if (type == Format::UINT) type = Format::SINT;
                break;
            case KTX_UNSIGNED_BYTE:
                size = 8;
                break;
            case KTX_SHORT:
                size = 16;
                if (type == Format::UNORM) type = Format::SNORM;
                if (type == Format::UINT) type = Format::SINT;
                break;
            case KTX_UNSIGNED_SHORT:
                size = 16;
                break;
            case KTX_INT:
                size = 32;
                if (type == Format::UNORM) type = Format::SNORM;
                if (type == Format::UINT) type = Format::SINT;
                break;
            case KTX_UNSIGNED_INT:
                size = 32;
                break;
            case KTX_FLOAT:
                size = 32;
                type = Format::FLOAT32;
                break;
            case KTX_HALF_FLOAT:
                size = 16;
                type = Format::FLOAT16;
                break;

            case KTX_UNSIGNED_BYTE_3_3_2:
                format = Format(8,  Format::UNORM, Format::BGR, 2, 3, 3, 0);
                return true;
            case KTX_UNSIGNED_BYTE_2_3_3_REV:
                format = Format(8,  Format::UNORM, Format::RGB, 3, 3, 2, 0);
                return true;
            case KTX_UNSIGNED_SHORT_5_6_5:
                format = Format(16, Format::UNORM, Format::BGR, 5, 6, 5, 0);
                return true;
            case KTX_UNSIGNED_SHORT_5_6_5_REV:
                format = Format(16, Format::UNORM, Format::RGB, 5, 6, 5, 0);
                return true;

            case KTX_UNSIGNED_SHORT_4_4_4_4:
                if (order == Format::RGBA)
                    format = Format(16, Format::UNORM, Format::ABGR, 4, 4, 4, 4);
                else
                    format = Format(16, Format::UNORM, Format::ARGB, 4, 4, 4, 4);
                return true;
            case KTX_UNSIGNED_SHORT_4_4_4_4_REV:
                if (order == Format::RGBA)
                    format = Format(16, Format::UNORM, Format::RGBA, 4, 4, 4, 4);
                else
                    format = Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4);
                return true;
            case KTX_UNSIGNED_SHORT_5_5_5_1:
                if (order == Format::RGBA)
                    format = Format(16, Format::UNORM, Format::ABGR, 1, 5, 5, 5);
                else
                    format = Format(16, Format::UNORM, Format::ARGB, 1, 5, 5, 5);
                return true;
            case KTX_UNSIGNED_SHORT_1_5_5_5_REV:
                if (order == Format::RGBA)
                    format = Format(16, Format::UNORM, Format::RGBA, 5, 5, 5, 1);
                else
                    format = Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 1);
                return true;
            case KTX_UNSIGNED_INT_8_8_8_8:
                if (order == Format::RGBA)
                    format = Format(32, Format::UNORM, Format::ABGR, 8, 8, 8, 8);
                else
                    format = Format(32, Format::UNORM, Format::ARGB, 8, 8, 8, 8);
                return true;
            case KTX_UNSIGNED_INT_8_8_8_8_REV:
                if (order == Format::RGBA)
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                else
                    format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
                return true;
            case KTX_UNSIGNED_INT_10_10_10_2:
                if (order == Format::RGBA)
                    format = Format(32, Format::UNORM, Format::ABGR, 2, 10, 10, 10);
                else
                    format = Format(32, Format::UNORM, Format::ARGB, 2, 10, 10, 10);
                return true;
            case KTX_UNSIGNED_INT_2_10_10_10_REV:
                if (order == Format::RGBA)
                    format = Format(32, Format::UNORM, Format::RGBA, 10, 10, 10, 2);
                else
                    format = Format(32, Format::UNORM, Format::BGRA, 10, 10, 10, 2);
                return true;

            case KTX_R11F_G11F_B10F:
                compression = TextureCompression::R11F_G11F_B10F;
                return true;
            case KTX_UNSIGNED_INT_10F_11F_11F_REV:
                compression = TextureCompression::R10F_G11F_B11F;
                return true;
            case KTX_RGB9_E5:
                compression = TextureCompression::RGB9_E5;
                return true;
            case KTX_UNSIGNED_INT_5_9_9_9_REV:
                if (order == Format::RGBA)
                    format = Format(16, Format::UNORM, Format::RGBA, 9, 9, 9, 5);
                else
                    format = Format(16, Format::UNORM, Format::BGRA, 9, 9, 9, 5);
                return true;

            case KTX_INT_2_10_10_10_REV:
                if (order == Format::RGBA)
                    format = Format(32, Format::SNORM, Format::RGBA, 10, 10, 10, 2);
                else
                    format = Format(32, Format::SNORM, Format::BGRA, 10, 10, 10, 2);
                return true;
        }

        if (valid)
        {
            int bits = components * size;
            int s[4] = { 0 };
            for (int i = 0; i < components; ++i)
            {
                s[i] = size;
            }
            format = Format(bits, type, order, s[0], s[1], s[2], s[3]);
        }

        return valid;
    }

    // ----------------------------------------------------------------------------
    // header
    // ----------------------------------------------------------------------------

    constexpr int KTX_HEADER_SIZE = 64;

    struct HeaderKTX
    {
        u8 reserved_identifier[12];
        u32 endianness;
        u32 glType;
        u32 glTypeSize;
        u32 glFormat;
        u32 glInternalFormat;
        u32 glBaseInternalFormat;
        u32 pixelWidth;
        u32 pixelHeight;
        u32 pixelDepth;
        u32 numberOfArrayElements;
        u32 numberOfFaces;
        u32 numberOfMipmapLevels;
        u32 bytesOfKeyValueData;

        ImageHeader header;

        HeaderKTX(ConstMemory memory)
        {
            const u8 ktxIdentifier[] =
            {
                0xab, 0x4b, 0x54, 0x58, 0x20, 0x31,
                0x31, 0xbb, 0x0d, 0x0a, 0x1a, 0x0a
            };

            if (std::memcmp(ktxIdentifier, memory.address, sizeof(ktxIdentifier)))
            {
                header.setError("[ImageDecoder.KTX] Incorrect identifier.");
                return;
            }

            const u8* ptr = memory.address + sizeof(ktxIdentifier);

            endianness = u32_mask(ptr[0], ptr[1], ptr[2], ptr[3]);
            ptr += 4;

            if (endianness == 0x04030201)
            {
                // same endianness
                ConstPointer p = ptr;

                glType = p.read32();
                glTypeSize = p.read32();
                glFormat = p.read32();
                glInternalFormat = p.read32();
                glBaseInternalFormat = p.read32();
                pixelWidth = p.read32();
                pixelHeight = p.read32();
                pixelDepth = p.read32();
                numberOfArrayElements = p.read32();
                numberOfFaces = p.read32();
                numberOfMipmapLevels = p.read32();
                bytesOfKeyValueData = p.read32();
            }
            else
            {
                if (endianness != 0x01020304)
                {
                    header.setError("[ImageDecoder.KTX] Incorrect endianness.");
                    return;
                }
                else
                {
                    // different endianness
                    SwapEndianConstPointer p = ptr;

                    glType = p.read32();
                    glTypeSize = p.read32();
                    glFormat = p.read32();
                    glInternalFormat = p.read32();
                    glBaseInternalFormat = p.read32();
                    pixelWidth = p.read32();
                    pixelHeight = p.read32();
                    pixelDepth = p.read32();
                    numberOfArrayElements = p.read32();
                    numberOfFaces = p.read32();
                    numberOfMipmapLevels = p.read32();
                    bytesOfKeyValueData = p.read32();
                }
            }

            debugPrint("endianness: 0x%x\n", endianness);
            debugPrint("glType: 0x%x\n", glType);
            debugPrint("glTypeSize: 0x%x\n", glTypeSize);
            debugPrint("glFormat: 0x%x\n", glFormat);
            debugPrint("glInternalFormat: 0x%x\n", glInternalFormat);
            debugPrint("glBaseInternalFormat: 0x%x\n", glBaseInternalFormat);
            debugPrint("pixelWidth: %d\n", pixelWidth);
            debugPrint("pixelHeight: %d\n", pixelHeight);
            debugPrint("pixelDepth: %d\n", pixelDepth);
            debugPrint("numberOfArrayElements: %d\n", numberOfArrayElements);
            debugPrint("numberOfFaces: %d\n", numberOfFaces);
            debugPrint("numberOfMipmapLevels: %d\n", numberOfMipmapLevels);
            debugPrint("bytesOfKeyValueData: %d\n", bytesOfKeyValueData);

            if (numberOfFaces != 1 && numberOfFaces != 6)
            {
                header.setError("[ImageDecoder.KTX] Incorrect number of faces.");
                return;
            }

            if (numberOfArrayElements != 0)
            {
                header.setError("[ImageDecoder.KTX] Incorrect number of array elements (not supported).");
                return;
            }

            numberOfMipmapLevels = std::max(1U, numberOfMipmapLevels);

            Format format;
            TextureCompression compression = opengl::getTextureCompression(glInternalFormat);

            if (compression != TextureCompression::NONE)
            {
                TextureCompressionInfo info(compression);
                format = info.format;
            }
            else
            {
                bool valid = resolve_format(format, compression, glType, glFormat);
                if (!valid)
                {
                    format = Format();
                }
            }

            header.width   = pixelWidth;
            header.height  = pixelHeight;
            header.depth   = 0;
            header.levels  = numberOfMipmapLevels;
            header.faces   = numberOfFaces;
            header.palette = false;
            header.format  = format;
            header.compression = compression;
        }

        ~HeaderKTX()
        {
        }

        u32 read32ktx(const u8* p) const
        {
            u32 value = uload32(p);
            if (endianness != 0x04030201)
            {
                value = byteswap(value);
            }
            return value;
        }

        ConstMemory getMemory(ConstMemory memory, int level, int depth, int face) const
        {
            const u8* address = memory.address;
            address += KTX_HEADER_SIZE + bytesOfKeyValueData;

            const int maxLevel = int(numberOfMipmapLevels);
            const int maxFace = int(numberOfFaces);

            MANGO_UNREFERENCED(depth); // TODO

            ConstMemory data;

            for (int iLevel = 0; iLevel < maxLevel; ++iLevel)
            {
                const int imageSize = read32ktx(address);
                const int imageSizeRounded = (imageSize + 3) & ~3;
                address += 4;

                for (int iFace = 0; iFace < maxFace; ++iFace)
                {
                    if (iLevel == level && iFace == face)
                    {
                        // Store selected address
                        data = ConstMemory(address, imageSizeRounded);
                    }

                    address += imageSizeRounded;
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
        ConstMemory m_memory;
        HeaderKTX m_ktx_header;

        Interface(ConstMemory memory)
            : m_memory(memory)
            , m_ktx_header(memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_ktx_header.header;
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            ConstMemory data = m_ktx_header.getMemory(m_memory, level, depth, face);
            return data;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);

            ImageDecodeStatus status;

			const ImageHeader& header = m_ktx_header.header;
            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            ConstMemory data = m_ktx_header.getMemory(m_memory, level, depth, face);

            Format format = header.format;
            TextureCompressionInfo info = header.compression;

            if (info.compression != TextureCompression::NONE)
            {
                TextureCompressionStatus cs = info.decompress(dest, data);

                status.info = cs.info;
                status.success = cs.success;
                status.direct = cs.direct;
            }
            else if (format != Format())
            {
                int width = std::max(1U, m_ktx_header.pixelWidth >> level);
                int height = std::max(1U, m_ktx_header.pixelHeight >> level);
                size_t stride = width * format.bytes();

                // KTX format stores data with GL_UNPACK_ALIGNMENT of 4
                stride = (stride + 3) & ~3;

                Surface source(width, height, format, stride, data.address);
                dest.blit(0, 0, source);
            }
            else
            {
                status.setError("[ImageDecoder.KTX] Incorrect format.");
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

    void registerImageDecoderKTX()
    {
        registerImageDecoder(createInterface, ".ktx");
    }

} // namespace mango::image
