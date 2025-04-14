/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // https://github.com/qb40/deluxe-paint-animation-kit/blob/master/PCX.TXT
    // https://people.sc.fsu.edu/~jburkardt/txt/pcx_format.txt
    // https://www.fileformat.info/format/pcx/egff.htm

    // ------------------------------------------------------------
    // CGA/EGA 16 color palette
    // ------------------------------------------------------------

#if 0
    const Color g_cga_palette [] =
    {
        Color(0x00, 0x00, 0x00, 0xff), // black
        Color(0x00, 0x00, 0xaa, 0xff), // low blue
        Color(0x00, 0xaa, 0x00, 0xff), // low green
        Color(0x00, 0xaa, 0xaa, 0xff), // low cyan
        Color(0xaa, 0x00, 0x00, 0xff), // low red
        Color(0xaa, 0x00, 0xaa, 0xff), // low magenta
        Color(0xaa, 0x55, 0x00, 0xff), // brown
        Color(0xaa, 0xaa, 0xaa, 0xff), // light gray
        Color(0x55, 0x55, 0x55, 0xff), // dark gray
        Color(0x55, 0x55, 0xff, 0xff), // high blue
        Color(0x55, 0xff, 0x55, 0xff), // high green
        Color(0x55, 0xff, 0xff, 0xff), // high cyan
        Color(0xff, 0x55, 0x55, 0xff), // high red
        Color(0xff, 0x55, 0xff, 0xff), // high magenta
        Color(0xff, 0xff, 0x55, 0xff), // yellow
        Color(0xff, 0xff, 0xff, 0xff), // white
    };
#endif

    const Color g_ega_palette [] =
    {
        Color(0x00, 0x00, 0x00, 0xff), // black
        Color(0x00, 0x00, 0xaa, 0xff), // blue
        Color(0x00, 0xaa, 0x00, 0xff), // green
        Color(0x00, 0xaa, 0xaa, 0xff), // cyan
        Color(0xaa, 0x00, 0x00, 0xff), // red
        Color(0xaa, 0x00, 0xaa, 0xff), // magenta
        Color(0xaa, 0xaa, 0x00, 0xff), // brown/yellow
        Color(0xaa, 0xaa, 0xaa, 0xff), // light gray
        Color(0x55, 0x55, 0x55, 0xff), // dark gray
        Color(0x55, 0x55, 0xff, 0xff), // bright blue
        Color(0x55, 0xff, 0x55, 0xff), // bright green
        Color(0x55, 0xff, 0xff, 0xff), // bright cyan
        Color(0xff, 0x55, 0x55, 0xff), // bright red
        Color(0xff, 0x55, 0xff, 0xff), // bright magenta
        Color(0xff, 0xff, 0x55, 0xff), // yellow
        Color(0xff, 0xff, 0xff, 0xff), // white
    };

    // ------------------------------------------------------------
    // .pcx parser
    // ------------------------------------------------------------

    struct HeaderPCX
    {
        u8      Manufacturer;
        u8      Version;
        u8      Encoding;
        u8      BitsPerPixel;
        u16     Xmin;
        u16     Ymin;
        u16     Xmax;
        u16     Ymax;
        u16     HDpi;
        u16     VDpi;
        const u8* ColorMap;
        u8      Reserved;
        u8      NPlanes;
        u16     BytesPerLine;
        u16     PaletteInfo;
        u16     HscreenSize;
        u16     VscreenSize;

        bool isPaletteMarker = false;
        ImageHeader header;

        HeaderPCX(ConstMemory memory)
        {
            LittleEndianConstPointer p = memory.address;

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

            ColorMap = p;
            p += 48;

            Reserved      = p.read8();
            NPlanes       = p.read8();
            BytesPerLine  = p.read16();
            PaletteInfo   = p.read16();
            HscreenSize   = p.read16();
            VscreenSize   = p.read16();

            if (Manufacturer != 10)
            {
                header.setError("[ImageDecoder.PCX] Incorrect manufacturer.");
                return;
            }

            if (memory.size > (128 + 768))
            {
                isPaletteMarker = memory.address[memory.size - 1 - 768] == 0x0c;
            }
            else if (memory.size < 128)
            {
                header.setError("[ImageDecoder.PCX] Incorrect file size.");
                return;
            }

            //printLine("PCX.Version: {}", Version);
            //printLine("PCX.BitsPerPixel: {}", BitsPerPixel);
            //printLine("PCX.NPlanes: {}", NPlanes);
            //printLine("PCX.isPaletteMarker: {}", isPaletteMarker);

            header.width   = int(Xmax - Xmin + 1);
            header.height  = int(Ymax - Ymin + 1);
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
            header.palette = isPaletteMarker || (BitsPerPixel == 1 && NPlanes == 4);
            header.format  = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            header.compression = TextureCompression::NONE;
        }

        ~HeaderPCX()
        {
        }
    };

    // ------------------------------------------------------------
    // scanline decoders
    // ------------------------------------------------------------

    void scanRLE(u8* dest, int bytes, const u8* p)
    {
        u8* end = dest + bytes;

        while (dest < end)
        {
            u8 sample = *p++;
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

    void decode2(const Surface& s, u8* buffer, int scansize)
    {
        const int width = s.width;
        const int height = s.height;
        const size_t stride = s.stride;
        u8* image = s.image;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; x += 4)
            {
                u8 data = buffer[x / 4];
                image[x + 0] = ((data >> 7) & 0x01) | ((data >> 5) & 0x02);
                image[x + 1] = ((data >> 5) & 0x01) | ((data >> 3) & 0x02);
                image[x + 2] = ((data >> 3) & 0x01) | ((data >> 1) & 0x02);
                image[x + 3] = ((data >> 1) & 0x01) | ((data << 1) & 0x02);
            }

            buffer += scansize;
            image += stride;
        }
    }

    void decode4(const Surface& s, u8* buffer, int scansize)
    {
        const int width = s.width;
        const int height = s.height;
        const size_t stride = s.stride;
        u8* image = s.image;

        const int sn = scansize / 4;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const u8* src = buffer + (x >> 3);
                const u8 mask = 0x80 >> (x & 7);
                int index = 0;
                if (src[sn * 0] & mask) index |= 1;
                if (src[sn * 1] & mask) index |= 2;
                if (src[sn * 2] & mask) index |= 4;
                if (src[sn * 3] & mask) index |= 8;
                image[x] = u8(index);
            }

            buffer += scansize;
            image += stride;
        }
    }

    void decode8(const Surface& s, u8* buffer, int scansize)
    {
        const int width = s.width;
        const int height = s.height;
        const size_t stride = s.stride;
        u8* image = s.image;

        for (int y = 0; y < height; ++y)
        {
            std::memcpy(image, buffer, width);
            buffer += scansize;
            image += stride;
        }
    }

    void decode24(const Surface& s, u8* buffer, int scansize)
    {
        const int width = s.width;
        const int height = s.height;
        const size_t stride = s.stride;
        u8* image = s.image;

        const int sn = scansize / 3;

        for (int y = 0; y < height; ++y)
        {
            const u8* src = buffer;
            u8* dest = image;

            for (int x = 0; x < width; ++x)
            {
                dest[0] = src[sn * 0];
                dest[1] = src[sn * 1];
                dest[2] = src[sn * 2];
                dest[3] = 0xff;
                dest += 4;
                ++src;
            }

            buffer += scansize;
            image += stride;
        }
    }

    void decode32(const Surface& s, u8* buffer, int scansize)
    {
        const int width = s.width;
        const int height = s.height;
        const size_t stride = s.stride;
        u8* image = s.image;

        const int sn = scansize / 4;

        for (int y = 0; y < height; ++y)
        {
            const u8* src = buffer;
            u8* dest = image;

            for (int x = 0; x < width; ++x)
            {
                dest[0] = src[sn * 0];
                dest[1] = src[sn * 1];
                dest[2] = src[sn * 2];
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

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        HeaderPCX m_pcx_header;

        Interface(ConstMemory memory)
            : m_memory(memory)
            , m_pcx_header(memory)
        {
            header = m_pcx_header.header;
        }

        ~Interface()
        {
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            int width = header.width;
            int height = header.height;
            Format format = header.format;

            Bitmap temp(width, height, format);

            // RLE scanline buffer
            int bytesPerLine = m_pcx_header.BytesPerLine ? m_pcx_header.BytesPerLine
                                                     : width * div_ceil(m_pcx_header.BitsPerPixel, 8);
            int scansize = m_pcx_header.NPlanes * bytesPerLine;

            Buffer buffer(scansize * height);

            scanRLE(buffer, scansize * height, m_memory.address + 128);

            switch (m_pcx_header.BitsPerPixel)
            {
                case 1:
                    switch (m_pcx_header.NPlanes)
                    {
                        case 4:
                        {
                            Palette palette;
                            palette.size = 16;

                            // read palette
                            const u8* pal = m_pcx_header.ColorMap;
                            for (u32 i = 0; i < palette.size; ++i)
                            {
                                palette[i] = Color(pal[0], pal[1], pal[2], 0xff);
                                pal += 3;
                            }

                            if (options.palette)
                            {
                                *options.palette = palette;
                                decode4(dest, buffer, scansize);
                            }
                            else
                            {
                                Bitmap indices(width, height, LuminanceFormat(8, Format::UNORM, 8, 0));
                                decode4(indices, buffer, scansize);

                                for (int y = 0; y < height; ++y)
                                {
                                    u32* d = temp.address<u32>(0, y);
                                    u8* s = indices.address<u8>(0, y);
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

                case 2:
                    switch (m_pcx_header.NPlanes)
                    {
                        case 1:
                        {
                            Palette palette;
                            palette.size = 4;

                            /*
                            if (m_pcx_header.isPaletteMarker)
                            {
                                // read palette
                                const u8* pal = m_memory.address + m_memory.size - 768;
                                for (u32 i = 0; i < palette.size; ++i)
                                {
                                    palette[i] = Color(pal[0], pal[1], pal[2], 0xff);
                                    pal += 3;
                                }
                            }
                            else
                            {
                                // read palette
                                const u8* pal = m_pcx_header.ColorMap;
                                for (u32 i = 0; i < palette.size; ++i)
                                {
                                    palette[i] = Color(pal[0], pal[1], pal[2], 0xff);
                                    pal += 3;
                                }
                            }
                            */

#if 1
                            // NOTE: This is a hack to get the correct palette for the 2-bit mode
                            //       The actual palette is stored in the EGA palette bytes
                            //       We need to extract the CGA settings from the EGA palette bytes
                            //       and then use the CGA palette to get the correct palette for the 2-bit mode
                            const u8* palette_data = m_pcx_header.ColorMap;

                            // Extract CGA settings from EGA palette bytes
                            const int background_color = palette_data[0] >> 4;  // 0-15
                            const int color_burst = (palette_data[3] & 0x80) >> 7;  // 0 or 1
                            const int palette_select = (palette_data[3] & 0x40) >> 6;  // 0 or 1
                            const int intensity = (palette_data[3] & 0x20) >> 5;  // 0 or 1
                            //printLine("background: {}, color_burst: {}, palette_select: {}, intensity: {}", background_color, color_burst, palette_select, intensity);

                            // Configure the 4-color palette based on CGA settings
                            palette[3] = g_ega_palette[background_color];  // background

                            if (color_burst == 1)  // color mode
                            {
                                if (palette_select == 0)  // yellow palette
                                {
                                    palette[1] = g_ega_palette[intensity ? 10 : 2];   // green
                                    palette[2] = g_ega_palette[intensity ? 12 : 4];   // red
                                    palette[0] = g_ega_palette[intensity ? 14 : 6];   // yellow/brown
                                }
                                else  // white palette
                                {
                                    palette[1] = g_ega_palette[intensity ? 11 : 3];   // cyan
                                    palette[2] = g_ega_palette[intensity ? 13 : 5];   // magenta
                                    palette[0] = g_ega_palette[intensity ? 15 : 7];   // white/light gray
                                }
                            }
                            else  // monochrome mode
                            {
                                // In monochrome, we use intensity levels of the background color
                                const int base_color = background_color & 7;
                                palette[1] = g_ega_palette[base_color];
                                palette[2] = g_ega_palette[base_color + 8];
                                palette[0] = g_ega_palette[15];  // white
                            }
#endif

                            if (options.palette)
                            {
                                *options.palette = palette;
                                decode2(dest, buffer, scansize);
                            }
                            else
                            {
                                Bitmap indices(width, height, LuminanceFormat(8, Format::UNORM, 8, 0));
                                decode2(indices, buffer, scansize);

                                for (int y = 0; y < height; ++y)
                                {
                                    u32* d = temp.address<u32>(0, y);
                                    u8* s = indices.address<u8>(0, y);
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
                    switch (m_pcx_header.NPlanes)
                    {
                        case 1:
                            // MANGO TODO: 16 color palette (need sample files for testing)
                            break;
                        case 4:
                            // MANGO TODO: 4096 color ARGB4444 (need sample files for testing)
                            break;
                        default:
                            break;
                    }
                    break;

                case 8:
                    switch (m_pcx_header.NPlanes)
                    {
                        case 1:
                            if (m_pcx_header.isPaletteMarker)
                            {
                                Palette palette;
                                palette.size = 256;

                                // read palette
                                const u8* pal = m_memory.address + m_memory.size - 768;
                                for (u32 i = 0; i < palette.size; ++i)
                                {
                                    palette[i] = Color(pal[0], pal[1], pal[2], 0xff);
                                    pal += 3;
                                }

                                if (options.palette)
                                {
                                    *options.palette = palette;
                                    decode8(dest, buffer, scansize);
                                }
                                else
                                {
                                    Bitmap indices(width, height, LuminanceFormat(8, Format::UNORM, 8, 0));
                                    decode8(indices, buffer, scansize);

                                    for (int y = 0; y < height; ++y)
                                    {
                                        u32* d = temp.address<u32>(0, y);
                                        u8* s = indices.address<u8>(0, y);
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

    void registerImageCodecPCX()
    {
        registerImageDecoder(createInterface, ".pcx");
    }

} // namespace mango::image
