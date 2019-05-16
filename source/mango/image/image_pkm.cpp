/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/bits.hpp>
#include <mango/core/exception.hpp>
#include <mango/image/image.hpp>

#define ID "[ImageDecoder.PKM] "

namespace
{
    using namespace mango;

    // ----------------------------------------------------------------------------
    // header
    // ----------------------------------------------------------------------------

    struct HeaderPKM
    {
        // ETC1 compression uses 4x4 blocks
        u16 extended_width;
        u16 extended_height;

        // original image size
        u16 original_width;
        u16 original_height;

        // preferred decode format
        Format format;
        TextureCompression compression;

        // compression type
        s16 type;

        void read(BigEndianPointer& p)
        {
            u32 magic = p.read32();
            if (magic != u32_mask_rev('P', 'K', 'M', ' '))
            {
                MANGO_EXCEPTION(ID"Incorrect header.");
            }

            p += 2; // skip version

            type = p.read16();
            switch (type)
            {
                case 0:
                    compression = TextureCompression::ETC1_RGB;
                    break;

                case 1:
                    compression = TextureCompression::ETC2_RGB;
                    break;

                case 2:
                    MANGO_EXCEPTION(ID"Unsupported compression.");

                case 3:
                    compression = TextureCompression::ETC2_RGBA;
                    break;

                case 4:
                    compression = TextureCompression::ETC2_RGB_ALPHA1;
                    break;

                default:
                    MANGO_EXCEPTION(ID"Incorrect compression.");
            }

            extended_width  = p.read16();
            extended_height = p.read16();
            original_width  = p.read16();
            original_height = p.read16();
            format = FORMAT_R8G8B8A8;
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        HeaderPKM m_header;
        Memory m_data;

        Interface(Memory memory)
        {
            BigEndianPointer p = memory.address;
            m_header.read(p);
            m_data = Memory(memory.address + 16, memory.size - 16);
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            ImageHeader header;

            header.width   = m_header.extended_width;
            header.height  = m_header.extended_height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = false;
            header.format  = m_header.format;
            header.compression = m_header.compression;

            return header;
        }

        Memory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            return m_data;
        }

        void decode(Surface& dest, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(palette);
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            TextureCompressionInfo info = m_header.compression;
            info.decompress(dest, m_data);
        }
    };

    ImageDecoderInterface* createInterface(Memory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    void imageEncode(Stream& stream, const Surface& surface, const ImageEncoderOptions& options)
    {
        MANGO_UNREFERENCED_PARAMETER(options);

        // ETC1 compression uses 4x4 blocks
        const int width = (surface.width + 3) & ~3;
        const int height = (surface.height + 3) & ~3;

        BigEndianStream s(stream);

        // write magic
        const u8 magic[] = { 'P', 'K', 'M', ' ', '1', '0' };
        stream.write(magic, 6);

        // write header
        s.write16(0);
        s.write16(width);
        s.write16(height);
        s.write16(surface.width);
        s.write16(surface.height);

        // select block compressor
        TextureCompressionInfo info(TextureCompression::ETC1_RGB);
        if (!info.encode)
        {
            MANGO_EXCEPTION(ID"No ETC1 compressor.");
        }

        // compute compressed data size
        const int blocks = (width / info.width) * (height / info.height);
        const int bytes = blocks * info.bytes;

        // compress
        Buffer buffer(bytes);
        info.compress(buffer, surface);

        // write results
        stream.write(buffer, bytes);
    }

} // namespace

namespace mango
{

    void registerImageDecoderPKM()
    {
        registerImageDecoder(createInterface, ".pkm");
        registerImageEncoder(imageEncode, ".pkm");
    }

} // namespace mango
