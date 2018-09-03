/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstring>
#include <mango/core/pointer.hpp>
#include <mango/core/exception.hpp>
#include <mango/image/image.hpp>
#include <mango/opengl/opengl.hpp>

#define ID "ImageStream.KTX: "

namespace
{
    using namespace mango;

	// http://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/

    // ----------------------------------------------------------------------------
    // xxx
    // ----------------------------------------------------------------------------

#if 0

glType:
    0 - compressed texture

    GL_UNSIGNED_BYTE
    GL_BYTE
    GL_UNSIGNED_SHORT
    GL_SHORT
    GL_UNSIGNED_INT
    GL_INT
    GL_HALF_FLOAT
    GL_FLOAT
    GL_UNSIGNED_BYTE_3_3_2
    GL_UNSIGNED_BYTE_2_3_3_REV
    GL_UNSIGNED_SHORT_5_6_5
    GL_UNSIGNED_SHORT_5_6_5_REV
    GL_UNSIGNED_SHORT_4_4_4_4
    GL_UNSIGNED_SHORT_4_4_4_4_REV
    GL_UNSIGNED_SHORT_5_5_5_1
    GL_UNSIGNED_SHORT_1_5_5_5_REV
    GL_UNSIGNED_INT_8_8_8_8
    GL_UNSIGNED_INT_8_8_8_8_REV
    GL_UNSIGNED_INT_10_10_10_2
    GL_UNSIGNED_INT_2_10_10_10_REV
    GL_UNSIGNED_INT_24_8
    GL_UNSIGNED_INT_10F_11F_11F_REV
    GL_UNSIGNED_INT_5_9_9_9_REV
    GL_FLOAT_32_UNSIGNED_INT_24_8_REV


glTypeSize:
    number of bytes to byteswap (1 - no endian conversion, 2 - 16 bits, 4 - 32 bits)


glFormat:
    0 - compressed texture

    GL_STENCIL_INDEX
    GL_DEPTH_COMPONENT
    GL_DEPTH_STENCIL
    GL_RED
    GL_GREEN
    GL_BLUE
    GL_RG
    GL_RGB
    GL_RGBA
    GL_BGR
    GL_BGRA
    GL_RED_INTEGER
    GL_GREEN_INTEGER
    GL_BLUE_INTEGER
    GL_RG_INTEGER
    GL_RGB_INTEGER
    GL_RGBA_INTEGER
    GL_BGR_INTEGER
    GL_BGRA_INTEGER

glInternalFormat:

    uncompressed:

    RGB16
    RGB16_SNORM
    RGBA2
    RGBA4
    RGB5_A1
    RGBA8
    RGBA8_SNORM
    RGB10_A2
    RGB10_A2UI
    RGBA12
    RGBA16
    RGBA16_SNORM
    SRGB8
    SRGB8_ALPHA8

    R16F
    RG16F
    RGB16F
    RGBA16F

    R32F
    RG32F
    RGB32F
    RGBA32F

    R11F_G11F_B10F
    RGB9_E5

    R8I
    R8UI
    R16I
    R16UI
    R32I
    R32UI
    RG8I
    RG8UI
    RG16I
    RG16UI
    RG32I
    RG32UI
    RGB8I
    RGB8UI
    RGB16I
    RGB16UI
    RGB32I
    RGB32UI
    RGBA8I
    RGBA8UI
    RGBA16I
    RGBA16UI
    RGBA32I
    RGBA32UI

    DEPTH_COMPONENT16
    DEPTH_COMPONENT24
    DEPTH_COMPONENT32
    DEPTH_COMPONENT32F
    DEPTH24_STENCIL8
    DEPTH32F_STENCIL8
    STENCIL_INDEX1
    STENCIL_INDEX4
    STENCIL_INDEX8
    STENCIL_INDEX16

glBaseInternalFormat:

    DEPTH_COMPONENT
    DEPTH_STENCIL
    RED
    RG
    RGB
    RGBA
    STENCIL_INDEX

#endif

    // ----------------------------------------------------------------------------
    // header
    // ----------------------------------------------------------------------------

    struct HeaderKTX
    {
		uint8 identifier[12];
		uint32 endianness;
		uint32 glType;
		uint32 glTypeSize;
		uint32 glFormat;
		uint32 glInternalFormat;
		uint32 glBaseInternalFormat;
		uint32 pixelWidth;
		uint32 pixelHeight;
		uint32 pixelDepth;
		uint32 numberOfArrayElements;
		uint32 numberOfFaces;
		uint32 numberOfMipmapLevels;
		uint32 bytesOfKeyValueData;

		HeaderKTX(Memory memory)
		{
			const uint8 ktxIdentifier[] =
			{
				0xab, 0x4b, 0x54, 0x58, 0x20, 0x31,
                0x31, 0xbb, 0x0d, 0x0a, 0x1a, 0x0a
			};

			std::memcpy(this, memory.address, sizeof(HeaderKTX));

			if (std::memcmp(ktxIdentifier, identifier, 12))
			{
                MANGO_EXCEPTION(ID"Incorrect identifier.");
			}

			if (endianness != 0x04030201)
			{
				if (endianness != 0x01020304)
				{
                    MANGO_EXCEPTION(ID"Incorrect endianness.");
				}
				else
				{
					// convert endianness
					glType = byteswap(glType);
					glTypeSize = byteswap(glTypeSize);
					glFormat = byteswap(glFormat);
					glInternalFormat = byteswap(glInternalFormat);
					glBaseInternalFormat = byteswap(glBaseInternalFormat);
					pixelWidth = byteswap(pixelWidth);
					pixelHeight = byteswap(pixelHeight);
					pixelDepth = byteswap(pixelDepth);
					numberOfArrayElements = byteswap(numberOfArrayElements);
					numberOfFaces = byteswap(numberOfFaces);
					numberOfMipmapLevels = byteswap(numberOfMipmapLevels);
					bytesOfKeyValueData = byteswap(bytesOfKeyValueData);
				}
			}

            if (numberOfFaces != 1 && numberOfFaces != 6)
            {
                MANGO_EXCEPTION(ID"Incorrect number of faces.");
            }

            if (numberOfArrayElements != 0)
            {
                MANGO_EXCEPTION(ID"Incorrect number of array elements (not supported).");
            }

            numberOfMipmapLevels = std::max(1U, numberOfMipmapLevels);
		}

		~HeaderKTX()
		{
		}

        uint32 read32(const uint8* p) const
        {
            uint32 value = uload32(p);
            if (endianness != 0x04030201)
            {
                value = byteswap(value);
            }
            return value;
        }

        TextureCompression computeFormat(Format& format) const
        {
			TextureCompression compression = opengl::getTextureCompression(glInternalFormat);

            if (compression != TextureCompression::NONE)
            {
				TextureCompressionInfo info(compression);
                format = info.format;
            }
            else
            {
#if 0
                // TODO: make this work w/o OpenGL
                const opengl::InternalFormat* info = opengl::getInternalFormat(glInternalFormat);
                if (info)
                {
                    format = info->format;
                }
                else
#endif
                {
                    format = FORMAT_NONE;
                }
            }

            return compression;
        }

		Memory getMemory(Memory memory, int level, int depth, int face) const
		{
			uint8* address = memory.address;
			address += sizeof(HeaderKTX) + bytesOfKeyValueData;

            const int maxLevel = int(numberOfMipmapLevels);
            const int maxFace = int(numberOfFaces);

            MANGO_UNREFERENCED_PARAMETER(depth); // TODO

            Memory data;

            for (int iLevel = 0; iLevel < maxLevel; ++iLevel)
            {
                const int imageSize = read32(address);
                const int imageSizeRounded = (imageSize + 3) & ~3;
                address += 4;

                for (int iFace = 0; iFace < maxFace; ++iFace)
                {
                    if (iLevel == level && iFace == face)
                    {
                        // Store selected address
                        data = Memory(address, imageSizeRounded);
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
        Memory m_memory;
        HeaderKTX m_header;

        Interface(Memory memory)
            : m_memory(memory)
            , m_header(memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            ImageHeader header;

            header.width   = m_header.pixelWidth;
            header.height  = m_header.pixelHeight;
            header.depth   = 0;
            header.levels  = m_header.numberOfMipmapLevels;
            header.faces   = m_header.numberOfFaces;
			header.palette = false;
            header.format  = FORMAT_NONE;
            header.compression = m_header.computeFormat(header.format);

            return header;
        }

        Memory memory(int level, int depth, int face) override
        {
            Memory data = m_header.getMemory(m_memory, level, depth, face);
            return data;
        }

        void decode(Surface& dest, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(palette);

            Memory data = m_header.getMemory(m_memory, level, depth, face);

            Format format;
            TextureCompressionInfo info = m_header.computeFormat(format);

            if (info.compression != TextureCompression::NONE)
            {
                info.decompress(dest, data);
            }
            else if (format != FORMAT_NONE)
            {
                int width = std::max(1U, m_header.pixelWidth >> level);
                int height = std::max(1U, m_header.pixelHeight >> level);
                int stride = width * format.bytes();
                Surface source(width, height, format, stride, data.address);
                dest.blit(0, 0, source);
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

    void registerImageDecoderKTX()
    {
        registerImageDecoder(createInterface, "ktx");
    }

} // namespace mango
