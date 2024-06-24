/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ConstMemory m_memory;
        ImageHeader m_header;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            LittleEndianConstPointer p = memory.address;

            u32 magic = p.read32();
            if (magic != 0x706d6266)
            {
                m_header.setError("[ImageDecoder.FBMP] Incorrect header magic.");
                return;
            }

            u32 width = p.read32();
            u32 height = p.read32();

            if (memory.size != size_t(width) * height * 4 + 12)
            {
                m_header.setError("[ImageDecoder.FBMP] Incorrect data size.");
                return;
            }

            m_header.width   = width;
            m_header.height  = height;
            m_header.depth   = 0;
            m_header.levels  = 0;
            m_header.faces   = 0;
            m_header.palette = false;
            m_header.format  = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            m_header.compression = TextureCompression::NONE;
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!m_header.success)
            {
                status.setError(m_header.info);
                return status;
            }

            u32 width = m_header.width;
            u32 height = m_header.height;
            Format format = m_header.format;

            Surface source(width, height, format, width * 4, m_memory.address + 12);
            dest.blit(0, 0, source);

            status.direct = true;

            return status;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

        LittleEndianStream output(stream);

        u32 magic = 0x706d6266;
        u32 width = surface.width;
        u32 height = surface.height;

        output.write32(magic);
        output.write32(width);
        output.write32(height);

        Bitmap temp(width, height, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8));
        temp.blit(0, 0, surface);

        output.write(temp.image, width * height);

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecRAW()
    {
        registerImageDecoder(createInterface, ".fbmp");
        registerImageEncoder(imageEncode, ".fbmp");
    }

} // namespace mango::image
