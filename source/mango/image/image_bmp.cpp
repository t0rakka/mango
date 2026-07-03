/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
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
            magic = 0;
            filesize = 0;
            offset = 0;

            // the file header is 14 bytes; leave the fields zeroed if it is not present
            if (memory.size < 14)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address;
            magic = p.read16();
            filesize = p.read32();
            p += 4;
            offset = p.read32();
        }
    };

    struct Header : mango::Status
    {
        // end of the input buffer, used to bound the (peeking) bitfield mask reads
        const u8* parseEnd = nullptr;

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
                    // In an OS/2 v2 header (size 64) compression code 4 means RLE24, which
                    // is decoded later; only the Windows BI_JPEG meaning is unsupported.
                    if (headerSize == 64)
                    {
                        break;
                    }
                    [[fallthrough]];

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
                    const int mask_count = (compression == BIC_ALPHABITFIELDS) ? 4 : 3;

                    // these masks are read past the fixed header fields; make sure they
                    // are actually present in the buffer before peeking at them
                    if (!parseEnd || (const u8*)p + mask_count * 4 <= parseEnd)
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
            parseEnd = memory.end();

            if (memory.size < 4)
            {
                setError("[ImageDecoder.BMP] Truncated header.");
                return;
            }

            LittleEndianConstPointer p = memory.address;

            parseHeaderSize(p);

            printLine(Print::Info, "  HeaderSize: {}", headerSize);

            // every header variant reads exactly headerSize bytes; make sure they exist
            if (memory.size < headerSize)
            {
                setError("[ImageDecoder.BMP] Truncated header (size {}).", headerSize);
                return;
            }

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
        const int width = header.width;

        int x = 0;
        int y = 0;
        u32 offset = 0;

        while (y < header.height)
        {
            u8* image = surface.address<u8>(0, y);

            if (x >= width)
            {
                x = 0;
            }

            // NOTE: "offset + k > size" form avoids the unsigned underflow that
            // "offset > size - k" would suffer when size < k.
            if (offset + 2 > size)
                return;
            u8 n = data[offset + 0];
            u8 c = data[offset + 1];
            offset += 2;

            if (n > 0)
            {
                // RLE run (clamp writes to the scanline)
                int ad = 4;
                while (n--)
                {
                    if (x < width)
                        image[x] = (c >> ad) & 0xf;
                    ++x;
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
                        if (offset + 2 > size)
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
                            if (offset + 1 > size)
                                return;
                            u8 s = data[offset++];
                            if (x < width)
                                image[x] = s >> 4;
                            ++x;
                            if (x < width)
                                image[x] = s & 0xf;
                            ++x;
                        }

                        if (c & 1)
                        {
                            if (offset + 1 > size)
                                return;
                            u8 s = data[offset++];
                            if (x < width)
                                image[x] = s >> 4;
                            ++x;
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
        const int width = header.width;

        int x = 0;
        int y = 0;

        while (y < header.height)
        {
            u8* image = surface.address<u8>(0, y);

            if (x >= width)
            {
                x = 0;
            }

            if (data + 2 > end)
                return;
            u8 n = data[0];
            u8 c = data[1];
            data += 2;

            if (n > 0)
            {
                // RLE run (clamp writes to the scanline)
                while (n--)
                {
                    if (x < width)
                        image[x] = c;
                    ++x;
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
                        if (data + 2 > end)
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
                        if (data + bytes > end)
                            return;

                        // clamp the copy to the remaining scanline
                        int copy = c;
                        if (x < width)
                        {
                            if (copy > width - x)
                                copy = width - x;
                            std::memcpy(image + x, data, copy);
                        }
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
        const int width = header.width;

        int x = 0;
        int y = 0;

        while (y < header.height)
        {
            u8* image = surface.address<u8>(0, y);

            if (x >= width)
            {
                x = 0;
            }

            if (data + 1 > end)
                return;
            u8 n = data[0];
            ++data;

            if (n)
            {
                if (data + 3 > end)
                    return;
                u8 r = data[0];
                u8 g = data[1];
                u8 b = data[2];
                data += 3;

                // RLE run (clamp writes to the scanline)
                while (n--)
                {
                    if (x < width)
                    {
                        image[x * 3 + 0] = b;
                        image[x * 3 + 1] = g;
                        image[x * 3 + 2] = r;
                    }
                    ++x;
                }
            }
            else
            {
                if (data + 1 > end)
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
                        if (data + 2 > end)
                            return;
                        x += data[0];
                        y += data[1];
                        data += 2;
                        break;
                    }

                    default:
                    {
                        // linear imagedata
                        if (data + size_t(n) * 3 > end)
                            return;
                        for (int i = 0; i < n; ++i)
                        {
                            if (x < width)
                            {
                                image[x * 3 + 0] = data[2];
                                image[x * 3 + 1] = data[1];
                                image[x * 3 + 2] = data[0];
                            }
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

    // ------------------------------------------------------------
    // OS/2 Huffman 1D (CCITT Group 3 one-dimensional, "modified Huffman")
    // ------------------------------------------------------------

    struct HuffCode
    {
        int run;        // run length, or -2 for EOL
        u16 code;       // code bits (right aligned)
        u8 length;      // number of valid bits
    };

    // CCITT T.4 white run-length codes (terminating 0..63, then makeup 64..1728)
    static const HuffCode g_white_codes[] =
    {
        {  0, 0x35, 8 }, {  1, 0x07, 6 }, {  2, 0x07, 4 }, {  3, 0x08, 4 },
        {  4, 0x0b, 4 }, {  5, 0x0c, 4 }, {  6, 0x0e, 4 }, {  7, 0x0f, 4 },
        {  8, 0x13, 5 }, {  9, 0x14, 5 }, { 10, 0x07, 5 }, { 11, 0x08, 5 },
        { 12, 0x08, 6 }, { 13, 0x03, 6 }, { 14, 0x34, 6 }, { 15, 0x35, 6 },
        { 16, 0x2a, 6 }, { 17, 0x2b, 6 }, { 18, 0x27, 7 }, { 19, 0x0c, 7 },
        { 20, 0x08, 7 }, { 21, 0x17, 7 }, { 22, 0x03, 7 }, { 23, 0x04, 7 },
        { 24, 0x28, 7 }, { 25, 0x2b, 7 }, { 26, 0x13, 7 }, { 27, 0x24, 7 },
        { 28, 0x18, 7 }, { 29, 0x02, 8 }, { 30, 0x03, 8 }, { 31, 0x1a, 8 },
        { 32, 0x1b, 8 }, { 33, 0x12, 8 }, { 34, 0x13, 8 }, { 35, 0x14, 8 },
        { 36, 0x15, 8 }, { 37, 0x16, 8 }, { 38, 0x17, 8 }, { 39, 0x28, 8 },
        { 40, 0x29, 8 }, { 41, 0x2a, 8 }, { 42, 0x2b, 8 }, { 43, 0x2c, 8 },
        { 44, 0x2d, 8 }, { 45, 0x04, 8 }, { 46, 0x05, 8 }, { 47, 0x0a, 8 },
        { 48, 0x0b, 8 }, { 49, 0x52, 8 }, { 50, 0x53, 8 }, { 51, 0x54, 8 },
        { 52, 0x55, 8 }, { 53, 0x24, 8 }, { 54, 0x25, 8 }, { 55, 0x58, 8 },
        { 56, 0x59, 8 }, { 57, 0x5a, 8 }, { 58, 0x5b, 8 }, { 59, 0x4a, 8 },
        { 60, 0x4b, 8 }, { 61, 0x32, 8 }, { 62, 0x33, 8 }, { 63, 0x34, 8 },
        { 64, 0x1b, 5 }, { 128, 0x12, 5 }, { 192, 0x17, 6 }, { 256, 0x37, 7 },
        { 320, 0x36, 8 }, { 384, 0x37, 8 }, { 448, 0x64, 8 }, { 512, 0x65, 8 },
        { 576, 0x68, 8 }, { 640, 0x67, 8 }, { 704, 0xcc, 9 }, { 768, 0xcd, 9 },
        { 832, 0xd2, 9 }, { 896, 0xd3, 9 }, { 960, 0xd4, 9 }, { 1024, 0xd5, 9 },
        { 1088, 0xd6, 9 }, { 1152, 0xd7, 9 }, { 1216, 0xd8, 9 }, { 1280, 0xd9, 9 },
        { 1344, 0xda, 9 }, { 1408, 0xdb, 9 }, { 1472, 0x98, 9 }, { 1536, 0x99, 9 },
        { 1600, 0x9a, 9 }, { 1664, 0x18, 6 }, { 1728, 0x9b, 9 },
    };

    // CCITT T.4 black run-length codes (terminating 0..63, then makeup 64..1728)
    static const HuffCode g_black_codes[] =
    {
        {  0, 0x37, 10 }, {  1, 0x02, 3 }, {  2, 0x03, 2 }, {  3, 0x02, 2 },
        {  4, 0x03, 3 }, {  5, 0x03, 4 }, {  6, 0x02, 4 }, {  7, 0x03, 5 },
        {  8, 0x05, 6 }, {  9, 0x04, 6 }, { 10, 0x04, 7 }, { 11, 0x05, 7 },
        { 12, 0x07, 7 }, { 13, 0x04, 8 }, { 14, 0x07, 8 }, { 15, 0x18, 9 },
        { 16, 0x17, 10 }, { 17, 0x18, 10 }, { 18, 0x08, 10 }, { 19, 0x67, 11 },
        { 20, 0x68, 11 }, { 21, 0x6c, 11 }, { 22, 0x37, 11 }, { 23, 0x28, 11 },
        { 24, 0x17, 11 }, { 25, 0x18, 11 }, { 26, 0xca, 12 }, { 27, 0xcb, 12 },
        { 28, 0xcc, 12 }, { 29, 0xcd, 12 }, { 30, 0x68, 12 }, { 31, 0x69, 12 },
        { 32, 0x6a, 12 }, { 33, 0x6b, 12 }, { 34, 0xd2, 12 }, { 35, 0xd3, 12 },
        { 36, 0xd4, 12 }, { 37, 0xd5, 12 }, { 38, 0xd6, 12 }, { 39, 0xd7, 12 },
        { 40, 0x6c, 12 }, { 41, 0x6d, 12 }, { 42, 0xda, 12 }, { 43, 0xdb, 12 },
        { 44, 0x54, 12 }, { 45, 0x55, 12 }, { 46, 0x56, 12 }, { 47, 0x57, 12 },
        { 48, 0x64, 12 }, { 49, 0x65, 12 }, { 50, 0x52, 12 }, { 51, 0x53, 12 },
        { 52, 0x24, 12 }, { 53, 0x37, 12 }, { 54, 0x38, 12 }, { 55, 0x27, 12 },
        { 56, 0x28, 12 }, { 57, 0x58, 12 }, { 58, 0x59, 12 }, { 59, 0x2b, 12 },
        { 60, 0x2c, 12 }, { 61, 0x5a, 12 }, { 62, 0x66, 12 }, { 63, 0x67, 12 },
        { 64, 0x0f, 10 }, { 128, 0xc8, 12 }, { 192, 0xc9, 12 }, { 256, 0x5b, 12 },
        { 320, 0x33, 12 }, { 384, 0x34, 12 }, { 448, 0x35, 12 }, { 512, 0x6c, 13 },
        { 576, 0x6d, 13 }, { 640, 0x4a, 13 }, { 704, 0x4b, 13 }, { 768, 0x4c, 13 },
        { 832, 0x4d, 13 }, { 896, 0x72, 13 }, { 960, 0x73, 13 }, { 1024, 0x74, 13 },
        { 1088, 0x75, 13 }, { 1152, 0x76, 13 }, { 1216, 0x77, 13 }, { 1280, 0x52, 13 },
        { 1344, 0x53, 13 }, { 1408, 0x54, 13 }, { 1472, 0x55, 13 }, { 1536, 0x5a, 13 },
        { 1600, 0x5b, 13 }, { 1664, 0x64, 13 }, { 1728, 0x65, 13 },
    };

    // Extended makeup codes (1792..2560) and EOL, shared by both colors
    static const HuffCode g_shared_codes[] =
    {
        { 1792, 0x08, 11 }, { 1856, 0x0c, 11 }, { 1920, 0x0d, 11 },
        { 1984, 0x12, 12 }, { 2048, 0x13, 12 }, { 2112, 0x14, 12 },
        { 2176, 0x15, 12 }, { 2240, 0x16, 12 }, { 2304, 0x17, 12 },
        { 2368, 0x1c, 12 }, { 2432, 0x1d, 12 }, { 2496, 0x1e, 12 },
        { 2560, 0x1f, 12 },
        { -2, 0x01, 12 }, // EOL
    };

    struct HuffTable
    {
        // map (length << 16) | code -> run (or -2 for EOL)
        std::map<u32, int> table;

        void add(const HuffCode* codes, size_t count)
        {
            for (size_t i = 0; i < count; ++i)
            {
                u32 key = (u32(codes[i].length) << 16) | codes[i].code;
                table[key] = codes[i].run;
            }
        }
    };

    struct HuffBitReader
    {
        const u8* p;
        const u8* end;
        u32 buffer = 0;
        int count = 0;

        HuffBitReader(ConstMemory memory)
            : p(memory.address)
            , end(memory.end())
        {
        }

        // returns -1 at end of data
        int getbit()
        {
            if (!count)
            {
                if (p >= end)
                    return -1;
                buffer = *p++;
                count = 8;
            }
            --count;
            return (buffer >> count) & 1;
        }
    };

    // Decode one run-length code; returns the run (>= 0), -2 for EOL, or -1 on error/eof.
    static int huffMatch(HuffBitReader& br, const HuffTable& table)
    {
        u32 code = 0;
        for (int length = 1; length <= 14; ++length)
        {
            int bit = br.getbit();
            if (bit < 0)
                return -1;

            code = (code << 1) | u32(bit);

            auto it = table.table.find((u32(length) << 16) | code);
            if (it != table.table.end())
                return it->second;
        }

        return -1;
    }

    bool readHuffman1D(const Surface& surface, const BitmapHeader& header, ConstMemory memory)
    {
        static HuffTable white;
        static HuffTable black;
        static bool initialized = false;
        if (!initialized)
        {
            white.add(g_white_codes, std::size(g_white_codes));
            white.add(g_shared_codes, std::size(g_shared_codes));
            black.add(g_black_codes, std::size(g_black_codes));
            black.add(g_shared_codes, std::size(g_shared_codes));
            initialized = true;
        }

        const int width = header.width;
        const int height = header.height;

        HuffBitReader br(memory);

        for (int y = 0; y < height; ++y)
        {
            u8* image = surface.address<u8>(0, y);

            int x = 0;
            int color = 0; // each line starts with a white run

            while (x < width)
            {
                int run = 0;
                bool terminating = false;
                bool endLine = false;

                // accumulate makeup codes until a terminating code (< 64) is read
                for (;;)
                {
                    int value = huffMatch(br, color ? black : white);

                    if (value == -2)
                    {
                        // EOL is a scanline separator. Each line is preceded by an EOL
                        // (and lines may be padded with extra EOLs); skip it while still
                        // at the start of the line, otherwise it terminates the line.
                        if (run == 0 && x == 0 && color == 0)
                        {
                            continue;
                        }

                        endLine = true;
                        break;
                    }

                    if (value < 0)
                    {
                        // out of data: tolerate truncation once something was produced
                        return x > 0 || y > 0;
                    }

                    run += value;

                    if (value < 64)
                    {
                        terminating = true;
                        break;
                    }
                }

                if (endLine || !terminating)
                {
                    break;
                }

                if (run > width - x)
                {
                    run = width - x;
                }

                if (color)
                {
                    std::memset(image + x, 1, run);
                }

                x += run;
                color ^= 1;
            }
        }

        return true;
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

        // Some writers set bfOffBits to the end of the DIB header (start of palette)
        // instead of after the palette. Bump the pixel offset past the color table.
        if (header.bitsPerPixel <= 8 && header.palette)
        {
            int components = header.paletteComponents;
            if (!components)
            {
                components = 4;
            }

            u32 colors = header.paletteSize;
            if (!colors)
            {
                colors = 1u << header.bitsPerPixel;
            }

            size_t computed = size_t(header.headerSize) + size_t(colors) * size_t(components);

            if (offset < computed)
            {
                offset = computed;
            }
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

            // Clamp the palette to what the buffer actually holds. header.palette points at
            // memory.address + headerSize, so the available palette bytes are bounded by the
            // remaining file size; never read (or write past color[256]) more than that.
            const u8* palette_end = memory.end();
            u32 max_entries = 0;
            if (header.palette < palette_end)
            {
                max_entries = u32((palette_end - header.palette) / components);
            }
            if (palette.size > max_entries)
            {
                palette.size = max_entries;
            }

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
                // OS/2 compression code 3 is CCITT G3 1-D ("Huffman 1D"), not bitfields.
                // It is always 1 bit per pixel; decode run-lengths into palette indices.
                Bitmap temp(header.width, header.height, LuminanceFormat(8, Format::UNORM, 8, 0));
                std::memset(temp.image, 0, size_t(temp.width) * temp.height);
                if (!readHuffman1D(temp, header, data))
                {
                    header.setError("[ImageDecoder.BMP] Huffman 1D decoding failed.");
                    return std::move(header);
                }
                blitPalette(mirror, temp, palette);
                return std::move(header);
            }

            if (header.compression == BIC_JPEG)
            {
                // OS/2 RLE24: decode into a private BGR24 buffer, then blit/convert into the
                // caller's surface (readRLE24 writes raw BGR triplets and cannot target an
                // arbitrary destination format directly).
                Bitmap temp(header.width, header.height, Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0));
                std::memset(temp.image, 0, size_t(temp.width) * temp.height * 3);
                readRLE24(temp, header, data);
                mirror.blit(0, 0, temp);
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
        const u8* pend = memory.end();

        if (memory.size < 6)
        {
            return "[ImageDecoder.BMP] Truncated ICO/CUR header.";
        }

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
            // each directory entry is 16 bytes; stop if it would run past the buffer
            if ((const u8*)p + 16 > pend)
            {
                break;
            }

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

        // need the 4-byte header size plus a full 40-byte WinBitmapHeader1
        if (block.size < 40)
        {
            return "[ImageDecoder.BMP] Truncated ICO/CUR image block.";
        }

        LittleEndianConstPointer pa = block.address;

        Header header;
        header.parseEnd = block.end();
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

            if (m_memory.size < 14)
            {
                header.setError("[ImageDecoder.BMP] Not enough data.");
                return;
            }

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
