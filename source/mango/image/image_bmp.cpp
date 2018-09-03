/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#define ID "ImageStream.BMP: "

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
        uint16 magic;
        uint32 filesize;
        uint32 offset;

        FileHeader(Memory memory)
        {
            LittleEndianPointer p = memory.address;
            magic = p.read16();
            filesize = p.read32();
            p += 4;
            offset = p.read32();
        }
    };

    struct Header
    {
        // HeaderSize
        uint32 headerSize;

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
        uint32 redMask;
        uint32 greenMask;
        uint32 blueMask;

        // WinBitmapHeader3
        uint32 alphaMask;

        // WinBitmapHeader4
        uint32 csType;
        uint32 endpoints[9];
        uint32 gammaRed;
        uint32 gammaGreen;
        uint32 gammaBlue;

        // WinBitmapHeader5
        uint32 intent;
        uint32 profileData;
        uint32 profileSize;
        uint32 reserved3;

        // OS2BitmapHeader1
        /*
        uint16 width;
        uint16 height;
        uint16 numPlanes;
        uint16 bitsPerPixel;
        */

        // OS2BitmapHeader2
        uint16 units;
        uint16 reserved;
        uint16 recording;
        uint16 rendering;
        uint32 size1;
        uint32 size2;
        uint32 colorEncoding;
        uint32 identifier;

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

        void HeaderSize(LittleEndianPointer& p)
        {
            headerSize = p.read32();
        }

        void WinBitmapHeader1(LittleEndianPointer& p)
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
        }

        void WinBitmapHeader2(LittleEndianPointer& p)
        {
            redMask = p.read32();
            greenMask = p.read32();
            blueMask = p.read32();
        }

        void WinBitmapHeader3(LittleEndianPointer& p)
        {
            alphaMask = p.read32();
        }

        void WinBitmapHeader4(LittleEndianPointer& p)
        {
            csType = p.read32();
            for (int i = 0; i < 9; ++i)
            {
                endpoints[i] = p.read32();
            }
            gammaRed = p.read32();
            gammaGreen = p.read32();
            gammaBlue = p.read32();
        }

        void WinBitmapHeader5(LittleEndianPointer& p)
        {
            intent = p.read32();
            profileData = p.read32();
            profileSize = p.read32();
            reserved3 = p.read32();
        }

        void OS2BitmapHeader1(LittleEndianPointer& p)
        {
            width        = p.read16();
            height       = p.read16();
            numPlanes    = p.read16();
            bitsPerPixel = p.read16();
        }

        void OS2BitmapHeader2(LittleEndianPointer& p)
        {
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
        int paletteComponents;
        const uint8* palette;
        bool yflip;

        BitmapHeader(Memory memory, bool isIcon)
        {
            paletteComponents = 0;

            LittleEndianPointer p = memory.address;

            HeaderSize(p);

            switch (headerSize)
            {
                case 64:
                {
                    WinBitmapHeader1(p);
                    OS2BitmapHeader2(p);
                    paletteComponents = 4;
                    break;
                }

                case 12:
                {
                    OS2BitmapHeader1(p);
                    paletteComponents = 3;
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
                    MANGO_EXCEPTION(ID"Incorrect header size.");
                    break;
            }

            if (numPlanes != 1)
            {
                MANGO_EXCEPTION(ID"Incorrect number of planes.");
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

                if (!importantColorCount)
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

                uint32 colorMask = redMask | greenMask | blueMask;
                if (colorMask)
                {
                    // WinBitmapHeader2 or later store the component masks
                    format = Format(bitsPerPixel, redMask, greenMask, blueMask, alphaMask);
                }
                else
                {
                    // WinBitmapHeader1 uses fixed pixel formats
                    switch (bitsPerPixel)
                    {
                        case 16:
                            format = Format(16, 0x7c00, 0x03e0, 0x001f, 0);
                            break;
                        case 24:
                            format = Format(24, 0xff0000, 0x00ff00, 0x0000ff, 0);
                            break;
                        case 32:
                            format = Format(32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
                            break;
                        default:
                            MANGO_EXCEPTION(ID"Incorrect number of color bits.");
                            break;
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

    void readRLE4(Surface& surface, const BitmapHeader& header, int stride, const uint8* data)
    {
        MANGO_UNREFERENCED_PARAMETER(stride);

        int x = 0;
        int y = 0;
        int offset = 0;

        while (y < header.height)
        {
            uint8* image = surface.address<uint8>(0, y);

            if (x >= header.width)
            {
                x = 0;
            }

            uint8 n = data[offset + 0];
            uint8 c = data[offset + 1];
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
                            uint8 s = data[offset++];
                            image[x++] = s >> 4;
                            image[x++] = s & 0xf;
                        }

                        if (c & 1)
                        {
                            uint8 s = data[offset++];
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

    void readRLE8(Surface& surface, const BitmapHeader& header, int stride, const uint8* data)
    {
        MANGO_UNREFERENCED_PARAMETER(stride);

        int x = 0;
        int y = 0;

        while (y < header.height)
        {
            uint8* image = surface.address<uint8>(0, y);

            if (x >= header.width)
            {
                x = 0;
            }

            uint8 n = data[0];
            uint8 c = data[1];
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

    void readIndexed(Surface& surface, const BitmapHeader& header, int stride, uint8* data)
    {
        const int bits = header.bitsPerPixel;
        const uint32 mask = (1 << bits) - 1;

        for (int y = 0; y < header.height; ++y)
        {
            BigEndianPointer p(data + y * stride);
            uint8* dest = surface.address<uint8>(0, y);

            uint32 value = 0;
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

    void readRGB(Surface surface, const BitmapHeader& header, int stride, uint8* data)
    {
        uint8* image = data;
        Surface source(header.width, header.height, header.format, stride, image);
        surface.blit(0, 0, source);
    }

    void blitPalette(Surface& dest, Surface& indices, const Palette& palette)
    {
        const int width = dest.width;
        const int height = dest.height;

        Bitmap temp(width, height, FORMAT_B8G8R8A8);

        for (int y = 0; y < height; ++y)
        {
            uint8* s = indices.address<uint8>(0, y);
            BGRA* d = temp.address<BGRA>(0, y);
            for (int x = 0; x < width; ++x)
            {
                d[x] = palette[s[x]];
            }
        }

        dest.blit(0, 0, temp);
    }

    void decodeBitmap(Surface& surface, Memory memory, int offset, bool isIcon, Palette* ptr_palette)
    {
        BitmapHeader header(memory, isIcon);

        Palette palette;

        if (header.palette)
        {
            palette.size = header.importantColorCount;

            // read palette
            const uint8* p = header.palette;
            for (uint32 i = 0; i < palette.size; ++i)
            {
                palette[i] = BGRA(p[2], p[1], p[0], 0xff);
                p += header.paletteComponents;
            }
        }

        const int stride = ((header.bitsPerPixel * header.width + 31) / 32) * 4;
        uint8* data = memory.address + offset;

        Surface mirror = surface;

        if (header.yflip)
        {
            mirror.image += (header.height - 1) * mirror.stride;
            mirror.stride = -mirror.stride;
        }

        switch (header.compression)
        {
            case BIC_RGB:
            case BIC_BITFIELDS:
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
                        MANGO_EXCEPTION(ID"Incorrect number of color bits.");
                        break;
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
            case BIC_ALPHABITFIELDS:
            case BIC_CMYK:
            case BIC_CMYKRLE8:
            case BIC_CMYKRLE4:
                MANGO_EXCEPTION(ID"Unsupported compression.");
                break;

            default:
                MANGO_EXCEPTION(ID"Incorrect compression.");
                break;
        }
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
        else
        {
            header = ImageHeader();
        }

        return header;
    }

    void getImage(Surface& surface, Memory memory, std::string extension)
    {
        ImageDecoder decoder(memory, extension);

        if (decoder.isDecoder())
        {
            decoder.decode(surface, nullptr, 0, 0, 0);
        }
    }

    // ------------------------------------------------------------
    // .ico parser
    // ------------------------------------------------------------

    void parseIco(ImageHeader* imageHeader, Surface* surface, Memory memory)
    {
        LittleEndianPointer p = memory.address;

        uint32 magic = p.read32();
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
                MANGO_EXCEPTION(ID"Incorrect ICO/CUR identifier.");
                break;
        }

        uint32 bestScore = 0;
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
                    MANGO_EXCEPTION(ID"Incorrect number of planes in ICO/CUR directory.");
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

            uint32 score = width * height * (bpp + 4) * (bpp + 4);
            if (score > bestScore)
            {
                bestScore = score;
                bestOffset = offset;
                bestSize = size;
                bestColors = colors;
            }
        }

        Memory block = memory.slice(bestOffset, bestSize);

        LittleEndianPointer pa = block.address;

        Header header;
        header.HeaderSize(pa);
        header.WinBitmapHeader1(pa);

        uint32 headersize = header.headerSize & 0xffff;

        int palettesize = std::max(int(header.paletteSize), bestColors);

        switch (headersize)
        {
            case 0x28:
            {
                BitmapHeader header(block, true);

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
                    decodeBitmap(*surface, block, headersize + palettesize * 4, true, nullptr);
                }

                break;
            }
            case 0x5089:
                if (imageHeader)
                {
                    *imageHeader = getHeader(block, "png");
                }

                if (surface)
                {
                    getImage(*surface, block, "png");
                }

                break;

            default:
                MANGO_EXCEPTION(ID"Incorrect ICO/CUR magic.");
                break;
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
            FileHeader fileHeader(m_memory);
            ImageHeader header;

            switch (fileHeader.magic)
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

                    header.width   = bmp_header.width;
                    header.height  = bmp_header.height;
                    header.depth   = 0;
                    header.levels  = 0;
                    header.faces   = 0;
        			header.palette = bmp_header.isPalette();
                    header.format  = bmp_header.format;
                    header.compression = TextureCompression::NONE;
                    break;
                }

                case 0x0000:
                    parseIco(&header, nullptr, m_memory);
                    break;

                case 0x5089:
                    header = getHeader(m_memory, "png");
                    break;

                case 0xd8ff:
                    header = getHeader(m_memory, "jpg");
                    break;

                case 0x4947:
                    header = getHeader(m_memory, "gif");
                    break;

                case 0xcdd7:
                    header = getHeader(m_memory, "apm");
                    break;

                default:
                    MANGO_EXCEPTION(ID"Incorrect header identifier.");
                    break;
            }

            return header;
        }

        void decode(Surface& dest, Palette* ptr_palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            FileHeader fileHeader(m_memory);

            switch (fileHeader.magic)
            {
                case 0x4d42: // BM - Windows Bitmap
                case 0x4142: // BA - OS/2 Bitmap
                case 0x4943: // CI - OS/2 Color Icon
                case 0x5043: // CP - OS/2 Color Pointer
                case 0x4349: // IC - OS/2 Icon
                case 0x5450: // PT - OS/2 Pointer
                    break;

                case 0x0000:
                    parseIco(nullptr, &dest, m_memory);
                    return;

                case 0x5089:
                    getImage(dest, m_memory, "png");
                    return;

                case 0xd8ff:
                    getImage(dest, m_memory, "jpg");
                    return;

                case 0x4947:
                    getImage(dest, m_memory, "gif");
                    return;

                case 0xcdd7:
                    getImage(dest, m_memory, "apm");
                    return;

                default:
                    MANGO_EXCEPTION(ID"Incorrect header identifier.");
                    break;
            }

            Memory block = m_memory.slice(14);
            decodeBitmap(dest, block, fileHeader.offset - 14, false, ptr_palette);
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

    void imageEncode(Stream& stream, const Surface& surface, float quality)
    {
        MANGO_UNREFERENCED_PARAMETER(quality);

        int width = surface.width;
        int height = surface.height;
        Format format = FORMAT_B8G8R8A8;

        int magicsize = 14;
        int headersize = 56;
        int dataoffset = magicsize + headersize;

        int stride = width * format.bytes();
        uint32 imagesize = height * stride;
        uint32 filesize = dataoffset + imagesize;

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
            uint8* buffer = temp.image + (temp.height - y - 1) * stride;
            s.write(buffer, stride);
        }
    }

} // namespace

namespace mango
{

    void registerImageDecoderBMP()
    {
        registerImageDecoder(createInterface, "bmp");
        registerImageDecoder(createInterface, "ico");
        registerImageDecoder(createInterface, "cur");
        registerImageEncoder(imageEncode, "bmp");
    }

} // namespace mango
