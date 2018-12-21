/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#define ID "ImageDecoder.PCX: "

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
            MANGO_EXCEPTION(ID"Incorrect file size.");
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

    void decode4(const Surface& s, uint8* buffer, int scansize)
    {
        const int width = s.width;
        const int height = s.height;
        const int stride = s.stride;
        uint8* image = s.image;

        const int sn = scansize / 4;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const uint8* src = buffer + (x >> 3);
                const uint8 mask = 0x80 >> (x & 7);
                int index = 0;
                if (src[sn * 0] & mask) index |= 1;
                if (src[sn * 1] & mask) index |= 2;
                if (src[sn * 2] & mask) index |= 4;
                if (src[sn * 3] & mask) index |= 8;
                image[x] = index;
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
        HeaderPCX m_header;

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
            bool isPaletteMarker = getPaletteMarker(m_memory);

            if (m_header.Manufacturer != 10)
            {
                MANGO_EXCEPTION(ID"Incorrect manufacturer.");
            }

            ImageHeader header;

            header.width   = m_header.getWidth();
            header.height  = m_header.getHeight();
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = isPaletteMarker || (m_header.BitsPerPixel == 1 && m_header.NPlanes == 4);
            header.format  = m_header.getFormat(isPaletteMarker);
            header.compression = TextureCompression::NONE;

            return header;
        }

        void decode(Surface& dest, Palette* ptr_palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            bool isPaletteMarker = getPaletteMarker(m_memory);

            int width = m_header.getWidth();
            int height = m_header.getHeight();
            Format format = m_header.getFormat(isPaletteMarker);

            Bitmap temp(width, height, format);

            // RLE scanline buffer
            int bytesPerLine = m_header.BytesPerLine;
            if (!bytesPerLine) bytesPerLine = width * round_to_next(m_header.BitsPerPixel, 8);
            int scansize = m_header.NPlanes * bytesPerLine;

            Buffer buffer(scansize * height);

            scanRLE(buffer, scansize * height, m_memory.address + 128);

            switch (m_header.BitsPerPixel)
            {
                case 1:
                    switch (m_header.NPlanes)
                    {
                        case 4:
                        {
                            Palette palette;
                            palette.size = 16;

                            // read palette
                            const uint8* pal = m_header.ColorMap;
                            for (uint32 i = 0; i < palette.size; ++i)
                            {
                                palette[i] = BGRA(pal[0], pal[1], pal[2], 0xff);
                                pal += 3;
                            }

                            if (ptr_palette)
                            {
                                *ptr_palette = palette;
                                decode4(dest, buffer, scansize);
                            }
                            else
                            {
                                Bitmap indices(width, height, FORMAT_L8);
                                decode4(indices, buffer, scansize);

                                for (int y = 0; y < height; ++y)
                                {
                                    uint32* d = temp.address<uint32>(0, y);
                                    uint8* s = indices.address<uint8>(0, y);
                                    for (int x = 0; x < width; ++x)
                                    {
                                        d[x] = palette[s[x]];
                                    }
                                }
                                dest.blit(0, 0, temp);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    break;

                case 4:
                    switch (m_header.NPlanes)
                    {
                        case 1:
                            // TODO: 16 color palette (need sample files for testing)
                            break;
                        case 4:
                            // TODO: 4096 color ARGB4444 (need sample files for testing)
                            break;
                        default:
                            break;
                    }
                    break;

                case 8:
                    switch (m_header.NPlanes)
                    {
                        case 1:
                            if (isPaletteMarker)
                            {
                                Palette palette;
                                palette.size = 256;

                                // read palette
                                const uint8* pal = m_memory.address + m_memory.size - 768;
                                for (uint32 i = 0; i < palette.size; ++i)
                                {
                                    palette[i] = BGRA(pal[0], pal[1], pal[2], 0xff);
                                    pal += 3;
                                }

                                if (ptr_palette)
                                {
                                    *ptr_palette = palette;
                                    decode8(dest, buffer, scansize);
                                }
                                else
                                {
                                    Bitmap indices(width, height, FORMAT_L8);
                                    decode8(indices, buffer, scansize);

                                    for (int y = 0; y < height; ++y)
                                    {
                                        uint32* d = temp.address<uint32>(0, y);
                                        uint8* s = indices.address<uint8>(0, y);
                                        for (int x = 0; x < width; ++x)
                                        {
                                            d[x] = palette[s[x]];
                                        }
                                    }
                                    dest.blit(0, 0, temp);
                                }
                            }
                            else
                            {
                                decode8(temp, buffer, scansize);
                                dest.blit(0, 0, temp);
                            }
                            break;
                        case 3:
                            decode24(temp, buffer, scansize);
                            dest.blit(0, 0, temp);
                            break;
                        case 4:
                            decode32(temp, buffer, scansize);
                            dest.blit(0, 0, temp);
                            break;
                        default:
                            break;
                    }
                    break;

                default:
                    break;
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

    void registerImageDecoderPCX()
    {
        registerImageDecoder(createInterface, ".pcx");
    }

} // namespace mango
