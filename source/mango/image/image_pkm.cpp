/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/bits.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

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

        // compression type
        s16 type;

        ImageHeader header;

        void read(BigEndianConstPointer& p)
        {
            u32 magic = p.read32();
            if (magic != u32_mask_rev('P', 'K', 'M', ' '))
            {
                header.setError("[ImageDecoder.PKM] Incorrect header.");
                return;
            }

            u16 version = p.read16();
            if (version != u16_mask_rev('1','0') && version != u16_mask_rev('2','0'))
            {
                header.setError("[ImageDecoder.PKM] Incorrect version (0x%x).", version);
                return;
            }

            // preferred decode format
            Format format;
            TextureCompression compression;

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
                    header.setError("[ImageDecoder.PKM] Unsupported compression.");
                    return;

                case 3:
                    compression = TextureCompression::ETC2_RGBA;
                    break;

                case 4:
                    compression = TextureCompression::ETC2_RGB_ALPHA1;
                    break;

                case 5:
                    compression = TextureCompression::EAC_R11;
                    break;

                case 6:
                    compression = TextureCompression::EAC_RG11;
                    break;

                case 7:
                    compression = TextureCompression::EAC_SIGNED_R11;
                    break;

                case 8:
                    compression = TextureCompression::EAC_SIGNED_RG11;
                    break;

                default:
                    header.setError("[ImageDecoder.PKM] Incorrect compression.");
                    return;
            }

            extended_width  = p.read16();
            extended_height = p.read16();
            original_width  = p.read16();
            original_height = p.read16();
            format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

            header.width   = extended_width;
            header.height  = extended_height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = false;
            header.format  = format;
            header.compression = compression;
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        HeaderPKM m_header;
        ConstMemory m_data;

        Interface(ConstMemory memory)
        {
            BigEndianConstPointer p = memory.address;
            m_header.read(p);
            m_data = ConstMemory(memory.address + 16, memory.size - 16);
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

			const ImageHeader& header = m_header.header;
            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            TextureCompressionInfo info = header.compression;
            TextureCompressionStatus cs = info.decompress(dest, m_data);

            status.info = cs.info;
            status.success = cs.success;
            status.direct = cs.direct;

            return status;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

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
            status.setError("[ImageEncoder.PKM] No ETC1 compressor.");
            return status;
        }

        // compute compressed data size
        const int blocks = (width / info.width) * (height / info.height);
        const int bytes = blocks * info.bytes;

        // compress
        Buffer buffer(bytes);
        info.compress(buffer, surface);

        // write results
        stream.write(buffer, bytes);

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageDecoderPKM()
    {
        registerImageDecoder(createInterface, ".pkm");
        registerImageEncoder(imageEncode, ".pkm");
    }

} // namespace mango::image
