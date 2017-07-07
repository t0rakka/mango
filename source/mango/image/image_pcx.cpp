/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#define ID "ImageStream.PCX: "

namespace
{

    using namespace mango;

    // ------------------------------------------------------------
    // .pcx parser
    // ------------------------------------------------------------

    struct HeaderPCX
    {
        uint8       Manufacturer;
        uint8       Version;
        uint8       Encoding;
        uint8       BitsPerPixel;
        uint16      Xmin;
        uint16      Ymin;
        uint16      Xmax;
        uint16      Ymax;
        uint16      HDpi;
        uint16      VDpi;
        uint8       ColorMap[48];
        uint8       Reserved;
        uint8       NPlanes;
        uint16      BytesPerLine;
        uint16      PaletteInfo;
        uint16      HscreenSize;
        uint16      VscreenSize;
        uint8       Padding[54];

        HeaderPCX(Memory memory)
        {
            LittleEndianPointer p = memory.address;

            Manufacturer  = p.read8();
            Version       = p.read8();
            Encoding      = p.read8();
            BitsPerPixel  = p.read8();
            Xmin          = p.read16();
            Ymin          = p.read16();
            Xmax          = p.read16();
            Ymax          = p.read16();
            HDpi          = p.read16();
            VDpi          = p.read16();

            p.read(ColorMap, 48);

            Reserved      = p.read8();
            NPlanes       = p.read8();
            BytesPerLine  = p.read16();
            PaletteInfo   = p.read16();
            HscreenSize   = p.read16();
            VscreenSize   = p.read16();
        }

        ~HeaderPCX()
        {
        }

        int getWidth() const
        {
            return int(Xmax - Xmin + 1);
        }

        int getHeight() const
        {
            return int(Ymax - Ymin + 1);
        }

        Format getFormat(bool isPaletteMarker) const
        {
            Format format;

            switch (BitsPerPixel)
            {
            case 1:
                switch (NPlanes)
                {
                case 4:
                    format = FORMAT_B8G8R8A8;
                    break;
                default:
                    MANGO_EXCEPTION(ID"Incorrect NPlanes.");
                    break;
                }
                break;

            case 4:
                switch (NPlanes)
                {
                case 1:
                    format = FORMAT_B8G8R8A8;
                    break;
                case 4:
                    format = FORMAT_B4G4R4A4;
                    break;
                default:
                    MANGO_EXCEPTION(ID"Incorrect NPlanes.");
                    break;
                }
                break;

            case 8:
                switch (NPlanes)
                {
                case 1:
                    format = isPaletteMarker ? FORMAT_B8G8R8A8 : FORMAT_L8;
                    break;
                case 3:
                    format = FORMAT_B8G8R8;
                    break;
                case 4:
                    format = FORMAT_B8G8R8A8;
                    break;
                default:
                    MANGO_EXCEPTION(ID"Incorrect NPlanes.");
                    break;
                }
                break;

            default:
                MANGO_EXCEPTION(ID"Incorrect BitsPerPixel.");
                break;
            }

            return FORMAT_B8G8R8A8;
        }
    };

    bool getPaletteMarker(Memory memory)
    {
        bool isPaletteMarker = false;

        if (memory.size > (128 + 768))
        {
            isPaletteMarker = memory.address[memory.size - 1 - 768] == 0x0c;
        }
        else if (memory.size < 128)
        {
            MANGO_EXCEPTION(ID"Incorrect filesize.");
        }

        return isPaletteMarker;
    }

    // ------------------------------------------------------------
    // scanline decoders
    // ------------------------------------------------------------

    void scanRLE(uint8* dest, int bytes, const uint8* p)
    {
        uint8* end = dest + bytes;

        while (dest < end)
        {
            uint8 sample = *p++;
            if (sample < 0xc0)
            {
                *dest++ = sample;
            }
            else
            {
                int count = sample & 0x3f;
                sample = *p++;
                std::memset(dest, sample, count);
                dest += count;
            }
        }
    }

    void decode4p(const Surface& s, uint8* buffer, int scansize, const uint8* palette)
    {
        const int width = s.width;
        const int height = s.height;
        const int stride = s.stride;
        uint8* image = s.image;

        const int sn = scansize / 4;

        for (int y = 0; y < height; ++y)
        {
            uint8* dest = image;

            for (int x = 0; x < width; ++x)
            {
                const uint8* src = buffer + (x >> 3);
                const uint8 mask = 0x80 >> (x & 7);
                int index = 0;
                if (src[sn * 0] & mask) index |= 1;
                if (src[sn * 1] & mask) index |= 2;
                if (src[sn * 2] & mask) index |= 4;
                if (src[sn * 3] & mask) index |= 8;

                dest[0] = palette[index * 3 + 2];
                dest[1] = palette[index * 3 + 1];
                dest[2] = palette[index * 3 + 0];
                dest[3] = 0xff;
                dest += 4;
            }

            buffer += scansize;
            image += stride;
        }
    }

    void decode8p(const Surface& s, uint8* buffer, int scansize, const uint8* palette)
    {
        const int width = s.width;
        const int height = s.height;
        const int stride = s.stride;
        uint8* image = s.image;

        for (int y = 0; y < height; ++y)
        {
            uint8* dest = image;

            for (int x = 0; x < width; ++x)
            {
                int index = buffer[x];
                dest[0] = palette[index * 3 + 2];
                dest[1] = palette[index * 3 + 1];
                dest[2] = palette[index * 3 + 0];
                dest[3] = 0xff;
                dest += 4;
            }

            buffer += scansize;
            image += stride;
        }
    }

    void decode8(const Surface& s, uint8* buffer, int scansize)
    {
        const int width = s.width;
        const int height = s.height;
        const int stride = s.stride;
        uint8* image = s.image;

        for (int y = 0; y < height; ++y)
        {
            std::memcpy(image, buffer, width);
            buffer += scansize;
            image += stride;
        }
    }

    void decode24(const Surface& s, uint8* buffer, int scansize)
    {
        const int width = s.width;
        const int height = s.height;
        const int stride = s.stride;
        uint8* image = s.image;

        const int sn = scansize / 3;

        for (int y = 0; y < height; ++y)
        {
            const uint8* src = buffer;
            uint8* dest = image;

            for (int x = 0; x < width; ++x)
            {
                dest[0] = src[sn * 2];
                dest[1] = src[sn * 1];
                dest[2] = src[sn * 0];
                dest[3] = 0xff;
                dest += 4;
                ++src;
            }

            buffer += scansize;
            image += stride;
        }
    }

    void decode32(const Surface& s, uint8* buffer, int scansize)
    {
        const int width = s.width;
        const int height = s.height;
        const int stride = s.stride;
        uint8* image = s.image;

        const int sn = scansize / 4;

        for (int y = 0; y < height; ++y)
        {
            const uint8* src = buffer;
            uint8* dest = image;

            for (int x = 0; x < width; ++x)
            {
                dest[0] = src[sn * 2];
                dest[1] = src[sn * 1];
                dest[2] = src[sn * 0];
                dest[3] = src[sn * 3];
                dest += 4;
                ++src;
            }

            buffer += scansize;
            image += stride;
        }
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        Memory m_memory;

        Interface(Memory memory)
        : m_memory(memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            HeaderPCX m_header(m_memory);

            bool isPaletteMarker = getPaletteMarker(m_memory);

            if (m_header.Manufacturer != 10)
            {
                MANGO_EXCEPTION(ID"Incorrect manufacturer.");
            }

            ImageHeader header;

            header.width  = m_header.getWidth();
            header.height = m_header.getHeight();
            header.depth  = 0;
            header.levels = 0;
            header.faces  = 0;
            header.format = m_header.getFormat(isPaletteMarker);
            header.compression = TextureCompression::NONE;

            return header;
        }

        void decode(Surface& dest, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(palette);
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            HeaderPCX header(m_memory);

            bool isPaletteMarker = getPaletteMarker(m_memory);

            int width = header.getWidth();
            int height = header.getHeight();
            Format format = header.getFormat(isPaletteMarker);

            Bitmap temp(width, height, format);

            // RLE scanline buffer
            int bytesPerLine = header.BytesPerLine;
            if (!bytesPerLine) bytesPerLine = width * round_to_next(header.BitsPerPixel, 8);
            int scansize = header.NPlanes * bytesPerLine;

            Buffer buffer(scansize * height);

            scanRLE(buffer, scansize * height, m_memory.address + 128);

            switch (header.BitsPerPixel)
            {
                case 1:
                    switch (header.NPlanes)
                    {
                        case 4:
                            decode4p(temp, buffer, scansize, header.ColorMap);
                            break;
                        default:
                            break;
                    }
                    break;

                case 4:
                    switch (header.NPlanes)
                    {
                        case 1:
                            // TODO: 16 color palette
                            break;
                        case 4:
                            // TODO: 4096 color ARGB4444
                            break;
                        default:
                            break;
                    }
                    break;

                case 8:
                    switch (header.NPlanes)
                    {
                        case 1:
                            if (isPaletteMarker)
                            {
                                const uint8* palette = m_memory.address + m_memory.size - 768;
                                decode8p(temp, buffer, scansize, palette);
                            }
                            else
                            {
                                decode8(temp, buffer, scansize); // TODO: testing required
                            }
                            break;
                        case 3:
                            decode24(temp, buffer, scansize);
                            break;
                        case 4:
                            decode32(temp, buffer, scansize); // TODO: testing required
                            break;
                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }

            dest.blit(0, 0, temp);
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

    void registerPCX()
    {
        registerImageDecoder(createInterface, "pcx");
    }

} // namespace mango
