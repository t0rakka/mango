/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

// Specification:
// https://en.wikipedia.org/wiki/BMP_file_format
// https://www.digicamsoft.com/bmp/bmp.html

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // .bmp parser
    // ------------------------------------------------------------

    enum Compression
    {
        BIC_RGB            = 0,
        BIC_RLE8           = 1,
        BIC_RLE4           = 2,
        BIC_BITFIELDS      = 3, // Huffman with OS/2 header
        BIC_JPEG           = 4, // RLE24 with OS/2 header
        BIC_PNG            = 5,
        BIC_ALPHABITFIELDS = 6,
        BIC_CMYK           = 11,
        BIC_CMYKRLE8       = 12,
        BIC_CMYKRLE4       = 13
    };

    struct FileHeader
    {
        u16 magic;
        u32 filesize;
        u32 offset;

        FileHeader(ConstMemory memory)
        {
            LittleEndianConstPointer p = memory.address;
            magic = p.read16();
            filesize = p.read32();
            p += 4;
            offset = p.read32();
        }
    };

    struct Header : mango::Status
    {
        // HeaderSize
        u32 headerSize = 0;

        // WinBitmapHeader1
        int width = 0;
        int height = 0;
        int numPlanes = 0;
        int bitsPerPixel = 0;
        int compression = 0;
        int imageDataSize = 0;
        int xResolution = 0;
        int yResolution = 0;
        u32 paletteSize = 0;
        u32 importantColorCount = 0;

        // WinBitmapHeader2
        u32 redMask = 0;
        u32 greenMask = 0;
        u32 blueMask = 0;

        // WinBitmapHeader3
        u32 alphaMask = 0;

        // WinBitmapHeader4
        u32 csType = 0;
        u32 endpoints[9] = { 0 };
        u32 gammaRed = 0;
        u32 gammaGreen = 0;
        u32 gammaBlue = 0;

        // WinBitmapHeader5
        u32 intent = 0;
        u32 profileData = 0;
        u32 profileSize = 0;
        u32 reserved3 = 0;

        // OS2BitmapHeader1
        /*
        u16 width;
        u16 height;
        u16 numPlanes;
        u16 bitsPerPixel;
        */

        // OS2BitmapHeader2
        u16 units = 0;
        u16 reserved = 0;
        u16 recording = 0;
        u16 rendering = 0;
        u32 size1 = 0;
        u32 size2 = 0;
        u32 colorEncoding = 0;
        u32 identifier = 0;

        void parseHeaderSize(LittleEndianConstPointer& p)
        {
            headerSize = p.read32();
        }

        void WinBitmapHeader1(LittleEndianConstPointer& p)
        {
            printLine(Print::Info, "[WinBitmapHeader1]");

            width = p.read32();
            height = p.read32();
            numPlanes = p.read16();
            bitsPerPixel = p.read16();
            compression = p.read32();
            imageDataSize = p.read32();
            xResolution = p.read32();
            yResolution = p.read32();
            paletteSize = p.read32();
            importantColorCount = p.read32();

            if (width < 0 || height < -65535 || width > 65535 || height > 65535)
            {
                setError("Incorrect image dimensions ({} x {})", width, height);
                return;
            }

            if (bitsPerPixel < 1 || bitsPerPixel > 32)
            {
                setError("Incorrect bits per pixel ({})", bitsPerPixel);
                return;
            }

            if (paletteSize > 256)
            {
                setError("Incorrect palette size ({})", paletteSize);
                return;
            }

            if (importantColorCount > 256)
            {
                setError("Incorrect palette size ({})", importantColorCount);
                return;
            }

            switch (compression)
            {
                case BIC_RGB:
                case BIC_RLE8:
                case BIC_RLE4:
                case BIC_BITFIELDS:
                case BIC_ALPHABITFIELDS:
                    break;

                case BIC_JPEG:
                case BIC_PNG:
                case BIC_CMYK:
                case BIC_CMYKRLE8:
                case BIC_CMYKRLE4:
                {
                    setError("[ImageDecoder.BMP] Unsupported compression ({}).", compression);
                    return;
                }

                default:
                {
                    setError("[ImageDecoder.BMP] Incorrect compression ({}).", compression);
                    return;
                }
            }

            if (compression == BIC_BITFIELDS || compression == BIC_ALPHABITFIELDS)
            {
                if (bitsPerPixel == 16 || bitsPerPixel == 32)
                {
                    LittleEndianConstPointer x = p;
                    redMask = x.read32();
                    greenMask = x.read32();
                    blueMask = x.read32();
                    if (compression == BIC_ALPHABITFIELDS)
                    {
                        alphaMask = x.read32();
                    }
                }
            }

            printLine(Print::Info, "  image: {} x {}, planes: {}, bits: {}", width, height, numPlanes, bitsPerPixel);
            printLine(Print::Info, "  compression: {}, imageDataSize: {}", compression, imageDataSize);
            printLine(Print::Info, "  resolution: {} x {}", xResolution, yResolution);
            printLine(Print::Info, "  palette: {}, importantColorCount: {}", paletteSize, importantColorCount);
        }

        void WinBitmapHeader2(LittleEndianConstPointer& p)
        {
            printLine(Print::Info, "[WinBitmapHeader2]");

            redMask = p.read32();
            greenMask = p.read32();
            blueMask = p.read32();

            printLine(Print::Info, "  redMask:   {:#010x}", redMask);
            printLine(Print::Info, "  greenMask: {:#010x}", greenMask);
            printLine(Print::Info, "  blueMask:  {:#010x}", blueMask);
        }

        void WinBitmapHeader3(LittleEndianConstPointer& p)
        {
            printLine(Print::Info, "[WinBitmapHeader3]");

            alphaMask = p.read32();

            printLine(Print::Info, "  alphaMask: {:#010x}", alphaMask);
        }

        void WinBitmapHeader4(LittleEndianConstPointer& p)
        {
            printLine(Print::Info, "[WinBitmapHeader4]");

            csType = p.read32();
            for (int i = 0; i < 9; ++i)
            {
                endpoints[i] = p.read32();
            }
            gammaRed = p.read32();
            gammaGreen = p.read32();
            gammaBlue = p.read32();

            printLine(Print::Info, "  gamma: {} {} {}", gammaRed, gammaGreen, gammaBlue);
        }

        void WinBitmapHeader5(LittleEndianConstPointer& p)
        {
            printLine(Print::Info, "[WinBitmapHeader5]");

            intent = p.read32();
            profileData = p.read32();
            profileSize = p.read32();
            reserved3 = p.read32();

            printLine(Print::Info, "  intent: {}", intent);
            printLine(Print::Info, "  profile data: {}, size: {}", profileData, profileSize);
        }

        void OS2BitmapHeader1(LittleEndianConstPointer& p)
        {
            printLine(Print::Info, "[OS2BitmapHeader1]");

            if (headerSize == 16)
            {
                width    = p.read32();
                height   = p.read32();
            }
            else
            {
                width    = p.read16();
                height   = p.read16();
            }

            numPlanes    = p.read16();
            bitsPerPixel = p.read16();

            printLine(Print::Info, "  image: {} x {}, planes: {}, bits: {}", width, height, numPlanes, bitsPerPixel);
        }

        void OS2BitmapHeader2(LittleEndianConstPointer& p)
        {
            printLine(Print::Info, "[OS2BitmapHeader2]");

            units         = p.read16();
            reserved      = p.read16();
            recording     = p.read16();
            rendering     = p.read16();
            size1         = p.read32();
            size2         = p.read32();
            colorEncoding = p.read32();
            identifier    = p.read32();
        }
    };

    struct BitmapHeader : Header
    {
        Format format;
        int paletteComponents = 0;
        const u8* palette = nullptr;
        bool yflip = false;
        bool os2 = false;

        BitmapHeader(ConstMemory memory, bool isIcon)
        {
            paletteComponents = 0;

            LittleEndianConstPointer p = memory.address;

            parseHeaderSize(p);

            printLine(Print::Info, "  HeaderSize: {}", headerSize);

            switch (headerSize)
            {
                case 12:
                {
                    OS2BitmapHeader1(p);
                    paletteComponents = 3;
                    os2 = true;
                    break;
                }

                case 16:
                {
                    OS2BitmapHeader1(p);
                    OS2BitmapHeader2(p);
                    paletteComponents = 4;
                    os2 = true;
                    break;
                }

                case 64:
                {
                    WinBitmapHeader1(p);
                    OS2BitmapHeader2(p);
                    paletteComponents = 4;
                    os2 = true;
                    break;
                }

                case 124:
                {
                    WinBitmapHeader1(p);
                    WinBitmapHeader2(p);
                    WinBitmapHeader3(p);
                    WinBitmapHeader4(p);
                    WinBitmapHeader5(p);
                    break;
                }

                case 108:
                {
                    WinBitmapHeader1(p);
                    WinBitmapHeader2(p);
                    WinBitmapHeader3(p);
                    WinBitmapHeader4(p);
                    break;
                }

                case 56:
                {
                    WinBitmapHeader1(p);
                    WinBitmapHeader2(p);
                    WinBitmapHeader3(p);
                    break;
                }

                case 52:
                {
                    WinBitmapHeader1(p);
                    WinBitmapHeader2(p);
                    break;
                }

                case 40:
                {
                    WinBitmapHeader1(p);
                    paletteComponents = 4;
                    break;
                }

                default:
                    setError("[ImageDecoder.BMP] Incorrect header size ({}).", headerSize);
                    return;
            }

            if (!success)
            {
                // parsing headers failed
                return;
            }

            if (numPlanes != 1)
            {
                setError("[ImageDecoder.BMP] Incorrect number of planes ({}).", numPlanes);
                return;
            }

            if (isIcon)
            {
                height /= 2;
            }

            if (height < 0)
            {
                height = -height;
                yflip = false;
            }
            else
            {
                yflip = true;
            }

            if (!imageDataSize)
            {
                int bytesPerScan = div_ceil(width * bitsPerPixel, 32) * 4;
                imageDataSize = height * bytesPerScan;
                printLine(Print::Info, "  computed imageDataSize: {}", imageDataSize);
            }

            if (bitsPerPixel <= 8)
            {
                if (!paletteSize)
                {
                    paletteSize = 1 << bitsPerPixel;
                }

                if (!importantColorCount || importantColorCount > 256)
                {
                    importantColorCount = paletteSize;
                }

                // Convert indexed bmp files to bgra
                format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 0);
                palette = memory.address + headerSize;
            }
            else
            {
                // no palette
                palette = nullptr;

                if (isMaskedFormat())
                {
                    // mask alpha with complete mask
                    u32 pixelMask = u32((1ull << bitsPerPixel) - 1);
                    alphaMask &= pixelMask;

                    // WinBitmapHeader2 or later store the component masks
                    format = Format(bitsPerPixel, redMask, greenMask, blueMask, alphaMask);
                }
                else
                {
                    // WinBitmapHeader1 uses fixed pixel formats
                    switch (bitsPerPixel)
                    {
                        case 16:
                            format = Format(16, Format::UNORM, Format::BGR, 5, 5, 5, 0);
                            break;
                        case 24:
                            format = Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0);
                            break;
                        case 32:
                            format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 0);
                            break;
                        default:
                            setError("[ImageDecoder.BMP] Incorrect number of color bits ({}).", bitsPerPixel);
                            return;
                    }
                }
            }
        }

        bool isMaskCorrect(u32 mask) const
        {
            // mask must be continuous
            bool is_solid = u32_is_solid_mask(mask);
            return is_solid;
        }

        bool isMaskedFormat() const
        {
            u32 mask = redMask | greenMask | blueMask;
            if (!mask)
            {
                // no color channels
                return false;
            }

            mask = redMask & greenMask & blueMask;
            if (mask)
            {
                // channels overlap
                return false;
            }

            if (!isMaskCorrect(redMask) || !isMaskCorrect(greenMask) || !isMaskCorrect(blueMask) || !isMaskCorrect(alphaMask))
            {
                return false;
            }

            return true;
        }

        bool isPalette() const
        {
            return palette != nullptr;
        }
    };

    // ------------------------------------------------------------
    // .bmp decoder
    // ------------------------------------------------------------

    void readRLE4(Surface& surface, const BitmapHeader& header, size_t bytesPerScan, ConstMemory memory)
    {
        MANGO_UNREFERENCED(bytesPerScan);

        const u8* data = memory.address;
        const u32 size = u32(memory.size);

        int x = 0;
        int y = 0;
        u32 offset = 0;

        while (y < header.height)
        {
            u8* image = surface.address<u8>(0, y);

            if (x >= header.width)
            {
                x = 0;
            }

            if (offset > size - 2)
                return;
            u8 n = data[offset + 0];
            u8 c = data[offset + 1];
            offset += 2;

            if (n > 0)
            {
                // RLE run
                int ad = 4;
                while (n--)
                {
                    image[x++] = (c >> ad) & 0xf;
                    ad = 4 - ad;
                }
            }
            else
            {
                switch (c)
                {
                    case 0:
                        // end of scanline
                        x = 0;
                        ++y;
                        break;

                    case 1:
                        // end of image
                        y = header.height;
                        break;

                    case 2:
                    {
                        // position delta
                        if (offset > size - 2)
                            return;
                        int dx = data[offset + 0];
                        int dy = data[offset + 1];
                        offset += 2;
                        x += dx;
                        y += dy;
                        break;
                    }

                    default:
                    {
                        // linear imagedata
                        int offset0 = offset;
                        int count = c >> 1;

                        while (count-- > 0)
                        {
                            if (offset > size - 1)
                                return;
                            u8 s = data[offset++];
                            image[x++] = s >> 4;
                            image[x++] = s & 0xf;
                        }

                        if (c & 1)
                        {
                            if (offset > size - 1)
                                return;
                            u8 s = data[offset++];
                            image[x++] = s >> 4;
                        }

                        int padding = (offset - offset0) & 1;
                        offset += padding;
                        break;
                    }
                }
            }
        }
    }

    void readRLE8(Surface& surface, const BitmapHeader& header, size_t bytesPerScan, ConstMemory memory)
    {
        MANGO_UNREFERENCED(bytesPerScan);

        const u8* data = memory.address;
        const u8* end = memory.end();

        int x = 0;
        int y = 0;

        while (y < header.height)
        {
            u8* image = surface.address<u8>(0, y);

            if (x >= header.width)
            {
                x = 0;
            }

            if (data > end - 2)
                return;
            u8 n = data[0];
            u8 c = data[1];
            data += 2;

            if (n > 0)
            {
                // RLE run
                while (n--)
                {
                    image[x++] = c;
                }
            }
            else
            {
                switch (c)
                {
                    case 0:
                        // end of scanline
                        x = 0;
                        ++y;
                        break;

                    case 1:
                        // end of image
                        y = header.height;
                        break;

                    case 2:
                    {
                        // position delta
                        if (data > end - 2)
                            return;
                        int dx = data[0];
                        int dy = data[1];
                        data += 2;
                        x += dx;
                        y += dy;
                        break;
                    }

                    default:
                    {
                        // linear imagedata
                        size_t bytes = c + (c & 1);
                        if (data > end - bytes)
                            return;

                        std::memcpy(image + x, data, c);
                        x += c;
                        data += bytes;
                        break;
                    }
                }
            }
        }
    }

    void readRLE24(const Surface& surface, const BitmapHeader& header, ConstMemory memory)
    {
        const u8* data = memory.address;
        const u8* end = memory.end();

        int x = 0;
        int y = 0;

        while (y < header.height)
        {
            u8* image = surface.address<u8>(0, y);

            if (x >= header.width)
            {
                x = 0;
            }

            if (data > end - 1)
                return;
            u8 n = data[0];
            ++data;

            if (n)
            {
                if (data > end - 3)
                    return;
                u8 r = data[0];
                u8 g = data[1];
                u8 b = data[2];
                data += 3;

                // RLE run
                while (n--)
                {
                    image[x * 3 + 0] = b;
                    image[x * 3 + 1] = g;
                    image[x * 3 + 2] = r;
                    ++x;
                }
            }
            else
            {
                if (data > end - 1)
                    return;
                n = data[0];
                ++data;

                switch (n)
                {
                    case 0:
                        // end of scanline
                        x = 0;
                        ++y;
                        break;

                    case 1:
                        // end of image
                        y = header.height;
                        break;

                    case 2:
                    {
                        // position delta
                        if (data > end - 2)
                            return;
                        x += data[0];
                        y += data[1];
                        data += 2;
                        break;
                    }

                    default:
                    {
                        // linear imagedata
                        if (data > end - n * 3)
                            return;
                        for (int i = 0; i < n; ++i)
                        {
                            image[x * 3 + 0] = data[2];
                            image[x * 3 + 1] = data[1];
                            image[x * 3 + 2] = data[0];
                            data += 3;
                            ++x;
                        }
                        data += ((n * 3) & 1); // skip padding byte
                        break;
                    }
                }
            }
        }
    }

    void readIndexed(Surface& surface, const BitmapHeader& header, size_t bytesPerScan, ConstMemory memory)
    {
        const u8* data = memory.address;

        const int width = header.width;
        const int bits = header.bitsPerPixel;
        const u32 mask = (1 << bits) - 1;

        // clip height to available data
        const int height = std::min(header.height, int(memory.size / bytesPerScan));
        if (height != header.height)
        {
            printLine(Print::Warning, "  WARNING: clipped height: {} -> {} (not enough data)", header.height, height);
        }

        for (int y = 0; y < height; ++y)
        {
            BigEndianConstPointer p(data + y * bytesPerScan);
            u8* dest = surface.address<u8>(0, y);

            u32 value = 0;
            int left = 0;

            for (int x = 0; x < width; ++x)
            {
                if (!left)
                {
                    value = p.read32();
                    left = 32;
                }

                left -= bits;
                dest[x] = (value >> left) & mask;
            }
        }
    }

    void readRGB(const Surface& surface, const BitmapHeader& header, size_t bytesPerScan, ConstMemory memory)
    {
        // clip height to available data
        int height = std::min(header.height, int(memory.size / bytesPerScan));
        if (height != header.height)
        {
            printLine(Print::Warning, "  WARNING: clipped height: {} -> {} (not enough data)", header.height, height);
        }

        Surface source(header.width, height, header.format, bytesPerScan, memory.address);
        surface.blit(0, 0, source);
    }

    void blitPalette(const Surface& dest, const Surface& indices, const Palette& palette)
    {
        const int width = dest.width;
        const int height = dest.height;

        Bitmap temp(width, height, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        for (int y = 0; y < height; ++y)
        {
            u8* s = indices.address<u8>(0, y);
            Color* d = temp.address<Color>(0, y);
            for (int x = 0; x < width; ++x)
            {
                d[x] = palette[s[x]];
            }
        }

        dest.blit(0, 0, temp);
    }

    mango::Status decodeBitmap(const Surface& surface, ConstMemory memory, size_t offset, bool isIcon)
    {
        BitmapHeader header(memory, isIcon);
        if (!header)
        {
            return std::move(header);
        }

        Palette palette;
        Palette* ptr_palette = nullptr; // not supported

        if (header.palette)
        {
            printLine(Print::Info, "[Palette]");
            printLine(Print::Info, "  size: {}", header.importantColorCount);
            printLine(Print::Info, "  components: {}", header.paletteComponents);

            int components = header.paletteComponents;
            if (!components)
            {
                // default to 4 components (last one is padding; forced to 0xff below)
                components = 4;
            }

            palette.size = header.importantColorCount;

            // read palette
            const u8* p = header.palette;
            for (u32 i = 0; i < palette.size; ++i)
            {
                palette[i] = Color(p[2], p[1], p[0], 0xff);
                p += components;
            }
        }

        const size_t bytesPerScan = div_ceil(header.width * header.bitsPerPixel, 32) * 4; // rounded to next 32 bits

        if (offset >= memory.size)
        {
            header.setError("[ImageDecoder.BMP] Out of data.");
            return std::move(header);
        }

        ConstMemory data(memory.address + offset, memory.size - offset);

        Surface mirror(surface, header.yflip);

        if (header.os2)
        {
            if (header.compression == BIC_BITFIELDS)
            {
                header.setError("[ImageDecoder.BMP] Unsupported compression (OS/2 Huffman).");
                return std::move(header);
            }

            if (header.compression == BIC_JPEG)
            {
                readRLE24(mirror, header, data);
                return std::move(header);
            }
        }

        switch (header.compression)
        {
            case BIC_RGB:
            case BIC_BITFIELDS:
            case BIC_ALPHABITFIELDS:
            {
                switch (header.bitsPerPixel)
                {
                    case 1:
                    case 2:
                    case 4:
                    case 8:
                    {
                        if (ptr_palette)
                        {
                            *ptr_palette = palette;
                            readIndexed(mirror, header, bytesPerScan, data);
                        }
                        else
                        {
                            Bitmap temp(header.width, header.height, LuminanceFormat(8, Format::UNORM, 8, 0));
                            readIndexed(temp, header, bytesPerScan, data);
                            blitPalette(mirror, temp, palette);
                        }
                        break;
                    }

                    case 16:
                    case 24:
                    case 32:
                    {
                        readRGB(mirror, header, bytesPerScan, data);
                        break;
                    }

                    default:
                    {
                        header.setError("[ImageDecoder.BMP] Incorrect number of color bits ({}).", header.bitsPerPixel);
                        return std::move(header);
                    }
                }
                break;
            }

            case BIC_RLE8:
            {
                if (ptr_palette)
                {
                    *ptr_palette = palette;
                    std::memset(surface.image, 0, surface.width * surface.height);
                    readRLE8(mirror, header, bytesPerScan, data);
                }
                else
                {
                    Bitmap temp(header.width, header.height, LuminanceFormat(8, Format::UNORM, 8, 0));
                    std::memset(temp.image, 0, temp.width * temp.height);
                    readRLE8(temp, header, bytesPerScan, data);
                    blitPalette(mirror, temp, palette);
                }
                break;
            }

            case BIC_RLE4:
            {
                if (ptr_palette)
                {
                    *ptr_palette = palette;
                    std::memset(surface.image, 0, surface.width * surface.height);
                    readRLE4(mirror, header, bytesPerScan, data);
                }
                else
                {
                    Bitmap temp(header.width, header.height, LuminanceFormat(8, Format::UNORM, 8, 0));
                    std::memset(temp.image, 0, temp.width * temp.height);
                    readRLE4(temp, header, bytesPerScan, data);
                    blitPalette(mirror, temp, palette);
                }
                break;
            }

            // These are checked when header is parsed
            case BIC_JPEG:
            case BIC_PNG:
            case BIC_CMYK:
            case BIC_CMYKRLE8:
            case BIC_CMYKRLE4:
            default:
                break;
        }

        return std::move(header);
    }

    // ------------------------------------------------------------
    // support for embedded format files
    // ------------------------------------------------------------

    ImageHeader getHeader(ConstMemory memory, std::string extension)
    {
        ImageDecoder decoder(memory, extension);
        ImageHeader header;

        if (decoder.isDecoder())
        {
            header = decoder.header();
        }

        return header;
    }

    void getImage(const Surface& surface, ConstMemory memory, std::string extension)
    {
        ImageDecoder decoder(memory, extension);

        if (decoder.isDecoder())
        {
            decoder.decode(surface);
        }
    }

    // ------------------------------------------------------------
    // .ico parser
    // ------------------------------------------------------------

    const char* parseIco(ImageHeader* imageHeader, const Surface* surface, ConstMemory memory)
    {
        LittleEndianConstPointer p = memory.address;

        u32 magic = p.read32();
        int size = p.read16();

        switch (magic)
        {
            case 0x10000:
                // .ico
                break;
            case 0x20000:
                // .cur
                break;
            default:
                return "[ImageDecoder.BMP] Incorrect ICO/CUR identifier.";
        }

        u32 bestScore = 0;
        int bestOffset = 0;
        int bestSize = 0;
        int bestColors = 0;

        for (int i = 0; i < size; ++i)
        {
            int planes = 0;
            int bpp = 0;

            int width = p[0];
            int height = p[1];
            int colors = p[2];
            p += 4;

            if (magic == 0x10000)
            {
                planes = p.read16();
                bpp = p.read16();
                if (planes < 0 || planes > 1)
                {
                    return "[ImageDecoder.BMP] Incorrect number of planes in ICO/CUR directory.";
                }
            }
            else
            {
                // skip cursor hotspot
                p += 4;
            }

            int c_size = p.read32();
            int c_offset = p.read32();

            if (!width)
            {
                width = 256;
            }

            if (!height)
            {
                height = 256;
            }

            if (!bpp)
            {
                bpp = u32_ceil_power_of_two(colors);
            }

            //printf("  + icon: %d x %d, bits: %d, colors: %d\n", width, height, bpp, colors);

            u32 score = width * height * (bpp + 4) * (bpp + 4);
            if (score > bestScore)
            {
                bestScore = score;
                bestOffset = c_offset;
                bestSize = c_size;
                bestColors = colors;
            }
        }

        ConstMemory block = memory.slice(bestOffset, bestSize);

        LittleEndianConstPointer pa = block.address;

        Header header;
        header.parseHeaderSize(pa);
        header.WinBitmapHeader1(pa);

        u32 headersize = header.headerSize & 0xffff;

        int palettesize = std::max(int(header.paletteSize), bestColors);

        switch (headersize)
        {
            case 0x28:
            {
                BitmapHeader bitmap_header(block, true);
                if (!bitmap_header)
                {
                    return "[ImageDecoder.BMP] Incorrect ICO/CUR header.";
                }

                if (imageHeader)
                {
                    imageHeader->width   = bitmap_header.width;
                    imageHeader->height  = bitmap_header.height;
                    imageHeader->depth   = 0;
                    imageHeader->levels  = 0;
                    imageHeader->faces   = 0;
                    imageHeader->format  = bitmap_header.format;
                    imageHeader->compression = TextureCompression::NONE;
                }

                if (surface)
                {
                    mango::Status status = decodeBitmap(*surface, block, headersize + palettesize * 4, true);
                    if (!status)
                    {
                        return "[ImageDecoder.BMP] ICO/CUR decoding failed.";
                    }
                }

                break;
            }
            case 0x5089:
                if (imageHeader)
                {
                    *imageHeader = getHeader(block, ".png");
                }

                if (surface)
                {
                    getImage(*surface, block, ".png");
                }

                break;

            default:
                return "[ImageDecoder.BMP] Incorrect ICO/CUR magic.";
        }

        return nullptr;
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        FileHeader m_file_header;

        Interface(ConstMemory memory)
            : m_memory(memory)
            , m_file_header(memory)
        {
            printLine(Print::Info, "magic: {:#x}", m_file_header.magic);

            switch (m_file_header.magic)
            {
                case 0x4d42: // BM - Windows Bitmap
                case 0x4142: // BA - OS/2 Bitmap
                case 0x4943: // CI - OS/2 Color Icon
                case 0x5043: // CP - OS/2 Color Pointer
                case 0x4349: // IC - OS/2 Icon
                case 0x5450: // PT - OS/2 Pointer
                {
                    ConstMemory bitmapMemory = m_memory.slice(14);
                    BitmapHeader bmp_header(bitmapMemory, false);
                    if (!bmp_header)
                    {
                        header.setError(bmp_header.info);
                        return;
                    }

                    header.width   = bmp_header.width;
                    header.height  = bmp_header.height;
                    header.depth   = 0;
                    header.levels  = 0;
                    header.faces   = 0;
                    header.format  = bmp_header.format;
                    header.compression = TextureCompression::NONE;

                    printLine(Print::Info, "[Header]");
                    printLine(Print::Info, "  image: {} x {}, bits: {}",
                        header.width,
                        header.height,
                        header.format.bits);
                    printLine(Print::Info, "[Format]");
                    printLine(Print::Info, "  bits: {}, bytes: {}, type: {:#x}, flags: {:#x}",
                        header.format.bits,
                        header.format.bytes(),
                        u16(header.format.type),
                        header.format.flags);
                    printLine(Print::Info, "  size: {} {} {} {}",
                        header.format.size.r,
                        header.format.size.g,
                        header.format.size.b,
                        header.format.size.a);
                    printLine(Print::Info, "  offset: {} {} {} {}",
                        header.format.offset.r,
                        header.format.offset.g,
                        header.format.offset.b,
                        header.format.offset.a);
                    break;
                }

                case 0x0000:
                {
                    const char* error = parseIco(&header, nullptr, m_memory);
                    if (error)
                    {
                        header.setError(error);
                    }
                    break;
                }

                case 0x5089:
                    header = getHeader(m_memory, ".png");
                    break;

                case 0xd8ff:
                    header = getHeader(m_memory, ".jpg");
                    break;

                case 0x4947:
                    header = getHeader(m_memory, ".gif");
                    break;

                case 0xcdd7:
                    header = getHeader(m_memory, ".apm");
                    break;

                default:
                    header.setError("[ImageDecoder.BMP] Incorrect header identifier.");
                    break;
            }
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

            if (!header)
            {
                status.setError(header.info);
                return status;
            }

            switch (m_file_header.magic)
            {
                case 0x4d42: // BM - Windows Bitmap
                case 0x4142: // BA - OS/2 Bitmap
                case 0x4943: // CI - OS/2 Color Icon
                case 0x5043: // CP - OS/2 Color Pointer
                case 0x4349: // IC - OS/2 Icon
                case 0x5450: // PT - OS/2 Pointer
                    break;

                case 0x0000:
                {
                    const char* error = parseIco(nullptr, &dest, m_memory);
                    if (error)
                    {
                        status.setError(error);
                    }
                    return status;
                }

                case 0x5089:
                    getImage(dest, m_memory, ".png");
                    return status;

                case 0xd8ff:
                    getImage(dest, m_memory, ".jpg");
                    return status;

                case 0x4947:
                    getImage(dest, m_memory, ".gif");
                    return status;

                case 0xcdd7:
                    getImage(dest, m_memory, ".apm");
                    return status;

                default:
                    status.setError("[ImageDecoder.BMP] Incorrect header identifier.");
                    return status;
            }

            ConstMemory block = m_memory.slice(14);
            mango::Status result = decodeBitmap(dest, block, m_file_header.offset - 14, false);
            if (!result)
            {
                status.setError(result.info);
            }

            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

        int width = surface.width;
        int height = surface.height;
        Format format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);

        int magicsize = 14;
        int headersize = 56;
        int dataoffset = magicsize + headersize;

        u32 stride = width * format.bytes();
        u32 imagesize = height * stride;
        u32 filesize = dataoffset + imagesize;

        LittleEndianStream s(stream);

        s.write16(0x4d42);      // 'BM'
        s.write32(filesize);    // filesize
        s.write32(0);           // reserved
        s.write32(dataoffset);  // data offset
        s.write32(headersize);  // header size
        s.write32(width);       // xsize
        s.write32(height);      // ysize
        s.write16(1);           // planes
        s.write16(32);          // bits
        s.write32(3);           // compression
        s.write32(imagesize);   // imagesize
        s.write32(0xb13);       // xres
        s.write32(0xb13);       // yres
        s.write32(0);           // colorused
        s.write32(0);           // colorimportant
        s.write32(0x00ff0000);  // red mask
        s.write32(0x0000ff00);  // green mask
        s.write32(0x000000ff);  // blue mask
        s.write32(0xff000000);  // alpha mask

        TemporaryBitmap temp(surface, format);

        for (int y = 0; y < temp.height; ++y)
        {
            u8* buffer = temp.image + (temp.height - y - 1) * stride;
            s.write(buffer, stride);
        }

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecBMP()
    {
        registerImageDecoder(createInterface, ".bmp");
        registerImageDecoder(createInterface, ".ico");
        registerImageDecoder(createInterface, ".cur");
        registerImageEncoder(imageEncode, ".bmp");
    }

} // namespace mango::image
