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

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            LittleEndianConstPointer p = memory.address;

            u32 magic = p.read32();
            if (magic != 0x706d6266)
            {
                header.setError("[ImageDecoder.FBMP] Incorrect header magic.");
                return;
            }

            u32 width = p.read32();
            u32 height = p.read32();

            if (memory.size != size_t(width) * height * 4 + 12)
            {
                header.setError("[ImageDecoder.FBMP] Incorrect data size.");
                return;
            }

            header.width   = width;
            header.height  = height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
            header.palette = false;
            header.format  = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            header.compression = TextureCompression::NONE;
        }

        ~Interface()
        {
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

            u32 width = header.width;
            u32 height = header.height;
            Format format = header.format;

            Surface source(width, height, format, width * 4, m_memory.address + 12);
            dest.blit(0, 0, source);

            status.direct = true;

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
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

        LittleEndianStream output(stream);

        u32 magic = 0x706d6266;
        u32 width = surface.width;
        u32 height = surface.height;

        output.write32(magic);
        output.write32(width);
        output.write32(height);

        TemporaryBitmap temp(surface, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8));

        for (int y = 0; y < surface.height; ++y)
        {
            u8* image = temp.address(0, y);
            output.write(image, width * 4);
        }

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
