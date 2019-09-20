/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
//#define MANGO_ENABLE_DEBUG_PRINT

#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#ifdef MANGO_ENABLE_IMAGE_BMP

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // .bmp parser
    // ------------------------------------------------------------

    enum Compression
    {
        BIC_RGB = 0,
        BIC_RLE8 = 1,
        BIC_RLE4 = 2,
        BIC_BITFIELDS = 3,
        BIC_JPEG = 4,
        BIC_PNG = 5,
        BIC_ALPHABITFIELDS = 6,
        BIC_CMYK = 11,
        BIC_CMYKRLE8 = 12,
        BIC_CMYKRLE4 = 13
    };

    struct FileHeader
    {
        u16 magic;
        u32 filesize;
        u32 offset;

        FileHeader(Memory memory)
        {
            LittleEndianConstPointer p = memory.address;
            magic = p.read16();
            filesize = p.read32();
            p += 4;
            offset = p.read32();
        }
    };

    struct Header
    {
        // HeaderSize
        u32 headerSize;

        // WinBitmapHeader1
        int width;
        int height;
        int numPlanes;
        int bitsPerPixel;
        int compression;
        int imageDataSize;
        int xResolution;
        int yResolution;
        int paletteSize;
        int importantColorCount;

        // WinBitmapHeader2
        u32 redMask;
        u32 greenMask;
        u32 blueMask;

        // WinBitmapHeader3
        u32 alphaMask;

        // WinBitmapHeader4
        u32 csType;
        u32 endpoints[9];
        u32 gammaRed;
        u32 gammaGreen;
        u32 gammaBlue;

        // WinBitmapHeader5
        u32 intent;
        u32 profileData;
        u32 profileSize;
        u32 reserved3;

        // OS2BitmapHeader1
        /*
        u16 width;
        u16 height;
        u16 numPlanes;
        u16 bitsPerPixel;
        */

        // OS2BitmapHeader2
        u16 units;
        u16 reserved;
        u16 recording;
        u16 rendering;
        u32 size1;
        u32 size2;
        u32 colorEncoding;
        u32 identifier;

        Header()
        {
            headerSize = 0;

            width = 0;
            height = 0;
            numPlanes = 0;
            bitsPerPixel = 0;
            compression = 0;
            imageDataSize = 0;
            xResolution = 0;
            yResolution = 0;
            paletteSize = 0;
            importantColorCount = 0;

            redMask = 0;
            greenMask = 0;
            blueMask = 0;

            alphaMask = 0;

            csType = 0;
            for (int i = 0; i < 9; ++i)
            {
                endpoints[i] = 0;
            }
            gammaRed = 0;
            gammaGreen = 0;
            gammaBlue = 0;

            intent = 0;
            profileData = 0;
            profileSize = 0;
            reserved3 = 0;

            units         = 0;
            reserved      = 0;
            recording     = 0;
            rendering     = 0;
            size1         = 0;
            size2         = 0;
            colorEncoding = 0;
            identifier    = 0;
        }

        void parseHeaderSize(LittleEndianConstPointer& p)
        {
            headerSize = p.read32();
        }

        void WinBitmapHeader1(LittleEndianConstPointer& p)
        {
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

            debugPrint("[WinBitmapHeader1]\n");
            debugPrint("  image: %d x %d, planes: %d, bits: %d\n", width, height, numPlanes, bitsPerPixel);
            debugPrint("  compression: %d, imageDataSize: %d\n", compression, imageDataSize);
            debugPrint("  resolution: %d x %d\n", xResolution, yResolution);
            debugPrint("  palette: %d, importantColorCount: %d\n", paletteSize, importantColorCount);
        }

        void WinBitmapHeader2(LittleEndianConstPointer& p)
        {
            redMask = p.read32();
            greenMask = p.read32();
            blueMask = p.read32();

            debugPrint("[WinBitmapHeader2]\n");
            debugPrint("  redMask: 0x%x, greenMask: 0x%x, blueMask: 0x%x\n", redMask, greenMask, blueMask);
        }

        void WinBitmapHeader3(LittleEndianConstPointer& p)
        {
            alphaMask = p.read32();

            debugPrint("[WinBitmapHeader3]\n");
            debugPrint("  alphaMask: 0x%x\n", alphaMask);
        }

        void WinBitmapHeader4(LittleEndianConstPointer& p)
        {
            csType = p.read32();
            for (int i = 0; i < 9; ++i)
            {
                endpoints[i] = p.read32();
            }
            gammaRed = p.read32();
            gammaGreen = p.read32();
            gammaBlue = p.read32();

            debugPrint("[WinBitmapHeader4]\n");
            debugPrint("  gamma: %d %d %d\n", gammaRed, gammaGreen, gammaBlue);
        }

        void WinBitmapHeader5(LittleEndianConstPointer& p)
        {
            intent = p.read32();
            profileData = p.read32();
            profileSize = p.read32();
            reserved3 = p.read32();

            debugPrint("[WinBitmapHeader5]\n");
            debugPrint("  intent: %d\n", intent);
            debugPrint("  profile data: %d, size: %d\n", profileData, profileSize);
        }

        void OS2BitmapHeader1(LittleEndianConstPointer& p, int headerSize)
        {
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

            debugPrint("[OS2BitmapHeader1]\n");
            debugPrint("  image: %d x %d, planes: %d, bits: %d\n", width, height, numPlanes, bitsPerPixel);
        }

        void OS2BitmapHeader2(LittleEndianConstPointer& p)
        {
            units         = p.read16();
            reserved      = p.read16();
            recording     = p.read16();
            rendering     = p.read16();
            size1         = p.read32();
            size2         = p.read32();
            colorEncoding = p.read32();
            identifier    = p.read32();

            debugPrint("[OS2BitmapHeader2]\n");
        }
    };

    struct BitmapHeader : Header
    {
        Format format;
        int paletteComponents;
        const u8* palette;
        bool yflip;

        const char* error = nullptr;

        BitmapHeader(Memory memory, bool isIcon)
        {
            paletteComponents = 0;

            LittleEndianConstPointer p = memory.address;

            parseHeaderSize(p);

            debugPrint("  HeaderSize: %d\n", headerSize);

            switch (headerSize)
            {
                case 12:
                {
                    OS2BitmapHeader1(p, headerSize);
                    paletteComponents = 3;
                    break;
                }

                case 16:
                {
                    OS2BitmapHeader1(p, headerSize);
                    OS2BitmapHeader2(p);
                    paletteComponents = 4;
                    break;
                }

                case 64:
                {
                    WinBitmapHeader1(p);
                    OS2BitmapHeader2(p);
                    paletteComponents = 4;
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
                    error = "[ImageDecoder.BMP] Incorrect header size.";
                    return;
            }

            debugPrint("  numPlanes: %d\n", numPlanes);
            debugPrint("  bitsPerPixel: %d\n", bitsPerPixel);

            if (numPlanes != 1)
            {
                error = "[ImageDecoder.BMP] Incorrect number of planes.";
                return;
            }

            if (isIcon)
            {
                height /= 2;
            }

            if (!imageDataSize)
            {
                // TODO: compute
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
                format = Format(32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
                palette = memory.address + headerSize;
            }
            else
            {
                // no palette
                palette = nullptr;

                u32 colorMask = redMask | greenMask | blueMask;
                if (colorMask)
                {
                    // Filter out alpha if it doesn't fit into the pixel
                    u32 pixelSizeMask = u32((1ull << bitsPerPixel) - 1);
                    alphaMask &= pixelSizeMask;

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
                            error = "[ImageDecoder.BMP] Incorrect number of color bits.";
                            return;
                    }
                }
            }
        }

        bool isPalette() const
        {
            return palette != nullptr;
        }
    };

    // ------------------------------------------------------------
    // .bmp decoder
    // ------------------------------------------------------------

    void readRLE4(Surface& surface, const BitmapHeader& header, int stride, const u8* data)
    {
        MANGO_UNREFERENCED(stride);

        int x = 0;
        int y = 0;
        int offset = 0;

        while (y < header.height)
        {
            u8* image = surface.address<u8>(0, y);

            if (x >= header.width)
            {
                x = 0;
            }

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
                            u8 s = data[offset++];
                            image[x++] = s >> 4;
                            image[x++] = s & 0xf;
                        }

                        if (c & 1)
                        {
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

    void readRLE8(Surface& surface, const BitmapHeader& header, int stride, const u8* data)
    {
        MANGO_UNREFERENCED(stride);

        int x = 0;
        int y = 0;

        while (y < header.height)
        {
            u8* image = surface.address<u8>(0, y);

            if (x >= header.width)
            {
                x = 0;
            }

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
                        for (int i = 0; i < c; ++i)
                        {
                            image[x++] = data[i];
                        }
                        data += c;
                        data += (c & 1); // skip padding byte
                        break;
                    }
                }
            }
        }
    }

    void readIndexed(Surface& surface, const BitmapHeader& header, int stride, const u8* data)
    {
        const int bits = header.bitsPerPixel;
        const u32 mask = (1 << bits) - 1;

        for (int y = 0; y < header.height; ++y)
        {
            BigEndianConstPointer p(data + y * stride);
            u8* dest = surface.address<u8>(0, y);

            u32 value = 0;
            int left = 0;

            for (int x = 0; x < header.width; ++x)
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

    void readRGB(const Surface& surface, const BitmapHeader& header, int stride, u8* data)
    {
        Surface source(header.width, header.height, header.format, stride, data);
        surface.blit(0, 0, source);
    }

    void blitPalette(const Surface& dest, const Surface& indices, const Palette& palette)
    {
        const int width = dest.width;
        const int height = dest.height;

        Bitmap temp(width, height, FORMAT_B8G8R8A8);

        for (int y = 0; y < height; ++y)
        {
            u8* s = indices.address<u8>(0, y);
            ColorBGRA* d = temp.address<ColorBGRA>(0, y);
            for (int x = 0; x < width; ++x)
            {
                d[x] = palette[s[x]];
            }
        }

        dest.blit(0, 0, temp);
    }

    const char* decodeBitmap(Surface& surface, Memory memory, int offset, bool isIcon, Palette* ptr_palette)
    {
        BitmapHeader header(memory, isIcon);
        if (header.error)
        {
            return header.error;
        }

        Palette palette;

        if (header.palette)
        {
            debugPrint("[Palette]\n");
            debugPrint("  size: %d\n", header.importantColorCount);
            debugPrint("  components: %d\n", header.paletteComponents);

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
                palette[i] = ColorBGRA(p[2], p[1], p[0], 0xff);
                p += components;
            }
        }

        const int stride = ((header.bitsPerPixel * header.width + 31) / 32) * 4;
        u8* data = memory.address + offset;

        Surface mirror = surface;
        
        if (header.yflip)
        {
            mirror.image += (header.height - 1) * surface.stride;
            mirror.stride = -surface.stride;
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
                            readIndexed(mirror, header, stride, data);
                        }
                        else
                        {
                            Bitmap temp(header.width, header.height, FORMAT_L8);
                            readIndexed(temp, header, stride, data);
                            blitPalette(mirror, temp, palette);
                        }
                        break;
                    }

                    case 16:
                    case 24:
                    case 32:
                    {
                        readRGB(mirror, header, stride, data);
                        break;
                    }

                    default:
                        return "[ImageDecoder.BMP] Incorrect number of color bits.";
                }
                break;
            }

            case BIC_RLE8:
            {
                if (ptr_palette)
                {
                    *ptr_palette = palette;
                    std::memset(surface.image, 0, surface.width * surface.height);
                    readRLE8(mirror, header, stride, data);
                }
                else
                {
                    Bitmap temp(header.width, header.height, FORMAT_L8);
                    std::memset(temp.image, 0, temp.width * temp.height);
                    readRLE8(temp, header, stride, data);
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
                    readRLE4(mirror, header, stride, data);
                }
                else
                {
                    Bitmap temp(header.width, header.height, FORMAT_L8);
                    std::memset(temp.image, 0, temp.width * temp.height);
                    readRLE4(temp, header, stride, data);
                    blitPalette(mirror, temp, palette);
                }
                break;
            }

            case BIC_JPEG:
            case BIC_PNG:
            case BIC_CMYK:
            case BIC_CMYKRLE8:
            case BIC_CMYKRLE4:
                return "[ImageDecoder.BMP] Unsupported compression.";

            default:
                return "[ImageDecoder.BMP] Incorrect compression.";
        }

        return nullptr;
    }

	// ------------------------------------------------------------
	// support for embedded format files
	// ------------------------------------------------------------

    ImageHeader getHeader(Memory memory, std::string extension)
    {
        ImageDecoder decoder(memory, extension);
        ImageHeader header;

        if (decoder.isDecoder())
        {
            header = decoder.header();
        }

        return header;
    }

    void getImage(Surface& surface, Memory memory, std::string extension)
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

    const char* parseIco(ImageHeader* imageHeader, Surface* surface, Memory memory)
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

            int size = p.read32();
            int offset = p.read32();

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
                bestOffset = offset;
                bestSize = size;
                bestColors = colors;
            }
        }

        Memory block = memory.slice(bestOffset, bestSize);

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
                BitmapHeader header(block, true);
                if (header.error)
                {
                    return header.error;
                }

                if (imageHeader)
                {
                    imageHeader->width   = header.width;
                    imageHeader->height  = header.height;
                    imageHeader->depth   = 0;
                    imageHeader->levels  = 0;
                    imageHeader->faces   = 0;
                    imageHeader->palette = false;
                    imageHeader->format  = header.format;
                    imageHeader->compression = TextureCompression::NONE;
                }

                if (surface)
                {
                    const char* error = decodeBitmap(*surface, block, headersize + palettesize * 4, true, nullptr);
                    if (error)
                    {
                        return error;
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

    struct Interface : ImageDecoderInterface
    {
        Memory m_memory;
        FileHeader m_file_header;
        ImageHeader m_image_header;

        Interface(Memory memory)
            : m_memory(memory)
            , m_file_header(memory)
        {
            debugPrint("magic: 0x%x\n", m_file_header.magic);

            switch (m_file_header.magic)
            {
                case 0x4d42: // BM - Windows Bitmap
                case 0x4142: // BA - OS/2 Bitmap
                case 0x4943: // CI - OS/2 Color Icon
                case 0x5043: // CP - OS/2 Color Pointer
                case 0x4349: // IC - OS/2 Icon
                case 0x5450: // PT - OS/2 Pointer
                {
                    Memory bitmapMemory = m_memory.slice(14);
                    BitmapHeader bmp_header(bitmapMemory, false);
                    if (bmp_header.error)
                    {
                        m_image_header.setError(bmp_header.error);
                        return;
                    }

                    m_image_header.width   = bmp_header.width;
                    m_image_header.height  = bmp_header.height;
                    m_image_header.depth   = 0;
                    m_image_header.levels  = 0;
                    m_image_header.faces   = 0;
        			m_image_header.palette = bmp_header.isPalette();
                    m_image_header.format  = bmp_header.format;
                    m_image_header.compression = TextureCompression::NONE;

                    debugPrint("[header]\n");
                    debugPrint("  image: %d x %d, bits: %d\n", 
                        m_image_header.width, m_image_header.height, m_image_header.format.bits);
                    break;
                }

                case 0x0000:
                {
                    const char* error = parseIco(&m_image_header, nullptr, m_memory);
                    if (error)
                    {
                        m_image_header.setError(error);
                    }
                    break;
                }

                case 0x5089:
                    m_image_header = getHeader(m_memory, ".png");
                    break;

                case 0xd8ff:
                    m_image_header = getHeader(m_memory, ".jpg");
                    break;

                case 0x4947:
                    m_image_header = getHeader(m_memory, ".gif");
                    break;

                case 0xcdd7:
                    m_image_header = getHeader(m_memory, ".apm");
                    break;

                default:
                    m_image_header.setError("[ImageDecoder.BMP] Incorrect header identifier.");
                    break;
            }
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_image_header;
        }

        ImageDecodeStatus decode(Surface& dest, Palette* ptr_palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!m_image_header.success)
            {
                status.setError(m_image_header.info);
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

            Memory block = m_memory.slice(14);
            const char* error = decodeBitmap(dest, block, m_file_header.offset - 14, false, ptr_palette);
            if (error)
            {
                status.setError(error);
            }

            return status;
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

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

        int width = surface.width;
        int height = surface.height;
        Format format = FORMAT_B8G8R8A8;

        int magicsize = 14;
        int headersize = 56;
        int dataoffset = magicsize + headersize;

        int stride = width * format.bytes();
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

        Bitmap temp(width, height, format);
        temp.blit(0, 0, surface);

        for (int y = 0; y < temp.height; ++y)
        {
            u8* buffer = temp.image + (temp.height - y - 1) * stride;
            s.write(buffer, stride);
        }

        return status;
    }

} // namespace

namespace mango
{

    void registerImageDecoderBMP()
    {
        registerImageDecoder(createInterface, ".bmp");
        registerImageDecoder(createInterface, ".ico");
        registerImageDecoder(createInterface, ".cur");
        registerImageEncoder(imageEncode, ".bmp");
    }

} // namespace mango

#endif // MANGO_ENABLE_IMAGE_BMP
