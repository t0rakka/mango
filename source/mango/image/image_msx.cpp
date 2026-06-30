/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    MSX SCREEN image decoders.

    Decodes the native VRAM dumps produced by the MSX BASIC BSAVE command for
    the SCREEN 2/4/5/6/7/8/10/11/12 video modes. All of these share a 7-byte
    BSAVE header (0xFE, start, end, exec addresses) followed by the raw VRAM
    contents. The depth and layout are determined by the SCREEN mode, which we
    select from the file extension (the byte stream itself is not self
    describing).

    Companion files (separate palette / interlace fields), animation frames and
    overlaid hardware sprites are intentionally ignored - this is a single file,
    single image decoder.

    Also handles the V9990 ("GRAPHIC9000") .G9B format, which carries its own
    self-describing header and an optional LZ compression layer, and the MSX2 /
    MSX2+ .MIF / .MIG interchange formats (compressed, any SCREEN mode including
    the 424-line interlaced hi-color variants).

    Reference: RECOIL (Retro Computer Image Library), recoil.c.
*/
#include <vector>
#include <cstring>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // MSX color helpers
    // ------------------------------------------------------------

    // The VDP palette stores 3 bits per channel; replicate to 8 bits.
    static inline u8 expand3(int v)
    {
        return u8((v << 5) | (v << 2) | (v >> 1));
    }

    // YJK / sprite color uses 5 bits per channel.
    static inline u8 expand5(int v)
    {
        return u8((v << 3) | (v >> 2));
    }

    static inline int clampU5(int x)
    {
        return x < 0 ? 0 : (x > 31 ? 31 : x);
    }

    // One VDP palette entry: byte 0 = 0RRR0BBB, byte 1 = 00000GGG.
    static Color msxColor(int rb, int g)
    {
        return Color(expand3((rb >> 4) & 7), expand3(g & 7), expand3(rb & 7), 0xff);
    }

    // The MSX2 default 16-color palette in VDP register format (16 x 2 bytes).
    static const u8 MSX2_DEFAULT_PALETTE [32] =
    {
        0, 0, 0, 0, 17, 6, 51, 7, 23, 1, 39, 3, 81, 1, 39, 6,
        113, 1, 115, 3, 97, 6, 100, 6, 17, 4, 101, 2, 85, 5, 119, 7
    };

    static bool isMsxPalette(const u8* data, int offset)
    {
        int ored = 0;
        for (int i = 0; i < 16; ++i)
        {
            int rb = data[offset + (i << 1)];
            int g = data[offset + (i << 1) + 1];
            if ((rb & 136) != 0 || (g & 248) != 0)
                return false;
            ored |= rb | g;
        }
        return ored != 0;
    }

    static void setMsxPalette(Color* palette, const u8* data, int offset, int colors)
    {
        for (int i = 0; i < colors; ++i)
        {
            palette[i] = msxColor(data[offset + (i << 1)], data[offset + (i << 1) + 1]);
        }
    }

    // SCREEN 8 is GRB332: 3-bit green/red, 2-bit blue (mapped through BLUES).
    static void setSc8Palette(Color* palette)
    {
        static const int BLUES [4] = { 0, 2, 4, 7 };
        for (int c = 0; c < 256; ++c)
        {
            palette[c] = Color(expand3((c >> 2) & 7), expand3((c >> 5) & 7), expand3(BLUES[c & 3]), 0xff);
        }
    }

    // Decode a single MSX YJK pixel. "base" points at the start of the row,
    // "x" is the column and "width" the row length (needed to detect the last,
    // partial quad). When "usePalette" is set, an odd luma nibble selects a
    // direct palette color instead of YJK chroma (SCREEN 10/11 behaviour).
    static Color msxYjkColor(const u8* base, int x, int width, const Color* palette, bool usePalette)
    {
        int yv = base[x] >> 3;
        if (usePalette && (yv & 1))
            return palette[yv >> 1];

        int r, g, b;
        if ((x | 3) >= width)
        {
            r = g = b = yv;
        }
        else
        {
            const u8* p = base + (x & ~3);
            int k = (p[0] & 7) | ((p[1] & 7) << 3);
            int j = (p[2] & 7) | ((p[3] & 7) << 3);
            k -= (k & 32) << 1;
            j -= (j & 32) << 1;
            r = clampU5(yv + j);
            g = clampU5(yv + k);
            b = clampU5((5 * yv - 2 * j - k + 2) >> 2);
        }

        return Color(expand5(r), expand5(g), expand5(b), 0xff);
    }

    static int getMsxHeader(const u8* data, size_t size)
    {
        if (size < 7 || data[1] != 0 || data[2] != 0 || data[5] != 0 || data[6] != 0)
            return -1;
        return data[3] | (data[4] << 8);
    }

    static int getMsx128Height(const u8* data, size_t size)
    {
        if (size < 135 || data[0] != 0xfe)
            return -1;
        int header = getMsxHeader(data, size);
        if (header < 127)
            return -1;
        int height = (header + 1) >> 7;
        if (size < size_t(7 + (height << 7)))
            return -1;
        return height < 212 ? height : 212;
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    enum class Screen
    {
        SC2, SC4, SC5, SR5, SC6, SC7, SC8, SCA, SCC
    };

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        Screen m_screen;
        int m_decode_height = 0; // stored rows (may differ from header.height)

        Interface(ConstMemory memory, Screen screen)
            : m_memory(memory)
            , m_screen(screen)
        {
            const u8* data = memory.address;
            const size_t size = memory.size;

            int width = 0;
            int height = 0;

            switch (screen)
            {
                case Screen::SC2:
                case Screen::SC4:
                    if (size < 14343 || data[0] != 0xfe || getMsxHeader(data, size) < 14335)
                    {
                        header.setError("[ImageDecoder.MSX] Invalid SCREEN 2/4 file.");
                        return;
                    }
                    width = 256;
                    height = 192;
                    break;

                case Screen::SC5:
                case Screen::SR5:
                    height = getMsx128Height(data, size);
                    if (height <= 0)
                    {
                        header.setError("[ImageDecoder.MSX] Invalid SCREEN 5 file.");
                        return;
                    }
                    width = 256;
                    break;

                case Screen::SC6:
                    height = getMsx128Height(data, size);
                    if (height <= 0)
                    {
                        header.setError("[ImageDecoder.MSX] Invalid SCREEN 6 file.");
                        return;
                    }
                    width = 512;
                    break;

                case Screen::SC7:
                    if (size < 54279 || data[0] != 0xfe || getMsxHeader(data, size) < 54271)
                    {
                        header.setError("[ImageDecoder.MSX] Invalid SCREEN 7 file.");
                        return;
                    }
                    width = 512;
                    height = 212;
                    break;

                case Screen::SC8:
                    if (size < 54279 || data[0] != 0xfe || getMsxHeader(data, size) < 54271)
                    {
                        header.setError("[ImageDecoder.MSX] Invalid SCREEN 8 file.");
                        return;
                    }
                    width = 256;
                    height = 212;
                    break;

                case Screen::SCA:
                    if (size < 64167 || data[0] != 0xfe || getMsxHeader(data, size) < 54271)
                    {
                        header.setError("[ImageDecoder.MSX] Invalid SCREEN 10/11 file.");
                        return;
                    }
                    width = 256;
                    height = 212;
                    break;

                case Screen::SCC:
                    if (size >= 49159 && data[0] == 0xfe && getMsxHeader(data, size) == 49151)
                    {
                        height = 192;
                    }
                    else if (size >= 54279 && data[0] == 0xfe && getMsxHeader(data, size) >= 54271)
                    {
                        height = 212;
                    }
                    else
                    {
                        header.setError("[ImageDecoder.MSX] Invalid SCREEN 12 file.");
                        return;
                    }
                    width = 256;
                    break;
            }

            m_decode_height = height;

            header.width  = width;
            header.height = height;
            header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }

        ~Interface()
        {
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);
            return ConstMemory();
        }

        void write(u8*& d, Color color)
        {
            d[0] = color.r;
            d[1] = color.g;
            d[2] = color.b;
            d[3] = 0xff;
            d += 4;
        }

        Color yjk(const u8* row, int x, int width, const Color* palette, bool usePalette)
        {
            return msxYjkColor(row, x, width, palette, usePalette);
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

            DecodeTargetBitmap target(dest, header.width, header.height, header.format);
            decodeImage(target);
            target.resolve();

            status.direct = target.isDirect();
            return status;
        }

        void decodeImage(const Surface& dest)
        {
            const u8* data = m_memory.address;
            const size_t size = m_memory.size;

            Color palette [256];

            switch (m_screen)
            {
                case Screen::SC2:
                {
                    if (isMsxPalette(data, 7047))
                        setMsxPalette(palette, data, 7047, 16);
                    else
                        setMsx1Palette(palette);
                    decodeTiles(dest, data, palette);
                    break;
                }

                case Screen::SC4:
                {
                    if (isMsxPalette(data, 7047))
                        setMsxPalette(palette, data, 7047, 16);
                    else
                        setMsxPalette(palette, MSX2_DEFAULT_PALETTE, 0, 16);
                    decodeTiles(dest, data, palette);
                    break;
                }

                case Screen::SC5:
                {
                    if (size >= 30343 + 32)
                        setMsxPalette(palette, data, 30343, 16);
                    else
                        setMsxPalette(palette, MSX2_DEFAULT_PALETTE, 0, 16);
                    decodeNibbles(dest, data + 7, 128, palette);
                    break;
                }

                case Screen::SR5:
                {
                    // SR5 stores no palette of its own; the colors live in a sibling
                    // ".PL5" file (16 entries x 2 bytes). Fall back to the MSX2 default
                    // palette when the companion is unavailable.
                    ConstMemory pl5 = acquireCompanion ? acquireCompanion(".pl5") : ConstMemory();
                    if (pl5.size >= 32)
                        setMsxPalette(palette, pl5.address, 0, 16);
                    else
                        setMsxPalette(palette, MSX2_DEFAULT_PALETTE, 0, 16);
                    decodeNibbles(dest, data + 7, 128, palette);
                    break;
                }

                case Screen::SC6:
                {
                    if (size >= 30351)
                        setMsxPalette(palette, data, 30343, 4);
                    else
                        setMsx6Palette(palette);
                    decode2bpp(dest, data + 7, 128, palette);
                    break;
                }

                case Screen::SC7:
                {
                    if (size >= 64167)
                        setMsxPalette(palette, data, 64135, 16);
                    else
                        setMsxPalette(palette, MSX2_DEFAULT_PALETTE, 0, 16);
                    decodeNibbles(dest, data + 7, 256, palette);
                    break;
                }

                case Screen::SC8:
                {
                    setSc8Palette(palette);
                    for (int y = 0; y < header.height; ++y)
                    {
                        const u8* row = data + 7 + size_t(y) * 256;
                        u8* d = dest.address<u8>(0, y);
                        for (int x = 0; x < 256; ++x)
                            write(d, palette[row[x]]);
                    }
                    break;
                }

                case Screen::SCA:
                {
                    setMsxPalette(palette, data, 64135, 16);
                    for (int y = 0; y < header.height; ++y)
                    {
                        const u8* row = data + 7 + size_t(y) * 256;
                        u8* d = dest.address<u8>(0, y);
                        for (int x = 0; x < 256; ++x)
                            write(d, yjk(row, x, 256, palette, true));
                    }
                    break;
                }

                case Screen::SCC:
                {
                    for (int y = 0; y < header.height; ++y)
                    {
                        const u8* row = data + 7 + size_t(y) * 256;
                        u8* d = dest.address<u8>(0, y);
                        for (int x = 0; x < 256; ++x)
                            write(d, yjk(row, x, 256, palette, false));
                    }
                    break;
                }
            }
        }

        void setMsx1Palette(Color* palette)
        {
            static const u32 colors [16] =
            {
                0x000000, 0x000000, 0x3eb849, 0x74d07d, 0x5955e0, 0x8076f1, 0xb95e51, 0x65dbef,
                0xdb6559, 0xff897d, 0xccc35e, 0xded087, 0x3aa241, 0xb766b5, 0xcccccc, 0xffffff
            };
            for (int i = 0; i < 16; ++i)
            {
                u32 c = colors[i];
                palette[i] = Color((c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff, 0xff);
            }
        }

        void setMsx6Palette(Color* palette)
        {
            static const u32 colors [4] = { 0x000000, 0x249224, 0x24db24, 0x6dff6d };
            for (int i = 0; i < 4; ++i)
            {
                u32 c = colors[i];
                palette[i] = Color((c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff, 0xff);
            }
        }

        // SCREEN 2/4 tile mode: 256x192, pattern generator + name + color tables.
        void decodeTiles(const Surface& dest, const u8* data, const Color* palette)
        {
            for (int y = 0; y < 192; ++y)
            {
                const int fontOffset = 7 + ((y & 192) << 5) + (y & 7);
                u8* d = dest.address<u8>(0, y);

                for (int x = 0; x < 256; ++x)
                {
                    const int name = data[7 + 6144 + ((y & ~7) << 2) + (x >> 3)];
                    const int b = fontOffset + (name << 3);
                    const int c = data[8192 + b];
                    const int idx = ((data[b] >> (~x & 7)) & 1) == 0 ? (c & 15) : (c >> 4);
                    write(d, palette[idx]);
                }
            }
        }

        // 4 bits per pixel, high nibble first.
        void decodeNibbles(const Surface& dest, const u8* data, int stride, const Color* palette)
        {
            for (int y = 0; y < header.height; ++y)
            {
                const u8* row = data + size_t(y) * stride;
                u8* d = dest.address<u8>(0, y);
                for (int x = 0; x < header.width; ++x)
                {
                    const int b = row[x >> 1];
                    const int nibble = (x & 1) == 0 ? (b >> 4) : (b & 15);
                    write(d, palette[nibble]);
                }
            }
        }

        // 2 bits per pixel, most significant pixel first.
        void decode2bpp(const Surface& dest, const u8* data, int stride, const Color* palette)
        {
            for (int y = 0; y < header.height; ++y)
            {
                const u8* row = data + size_t(y) * stride;
                u8* d = dest.address<u8>(0, y);
                for (int x = 0; x < header.width; ++x)
                {
                    const int b = row[x >> 2];
                    const int index = (b >> ((~x & 3) << 1)) & 3;
                    write(d, palette[index]);
                }
            }
        }
    };

    template <Screen screen>
    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        return new Interface(memory, screen);
    }

    // ------------------------------------------------------------
    // GRAPHIC9000 / V9990 (.G9B)
    // ------------------------------------------------------------
    //
    // The V9990 ("GRAPHIC9000") was a much more capable VDP than the stock
    // MSX2 V9938; .G9B is the de-facto interchange format for the homebrew
    // graphics it produces. The 16-byte header is followed by an optional
    // palette and then the (optionally compressed) pixel data:
    //
    //     0   "G9B"
    //     3   11, 0           (fixed)
    //     5   depth           bits per pixel: 2, 4, 8 or 16
    //     6   mode            for 8bpp/0-color: 0x40 GRB332, 0x80 YJK, 0xc0 YUV
    //     7   colors          palette entry count (0, 4, 16 or 64)
    //     8   width           little-endian u16
    //     10  height          little-endian u16
    //     12  compression     0 = none, 1 = LZ (G9bStream)
    //     16  palette         colors * 3 bytes (5-bit R, G, B)
    //
    // Reference: RECOIL, DecodeG9b().

    // LZ-style bit/byte stream used by the compressed variant.
    struct G9bStream
    {
        static constexpr int BlockEnd = -2;

        const u8* content;
        int contentLength;
        int contentOffset = 0;
        int bits = 0; // 8 bits sliding left with a trailing 1

        int readBit()
        {
            if ((bits & 0x7f) == 0)
            {
                if (contentOffset >= contentLength)
                    return -1;
                bits = (content[contentOffset++] << 1) | 1;
            }
            else
            {
                bits <<= 1;
            }
            return (bits >> 8) & 1;
        }

        int readByte()
        {
            if (contentOffset >= contentLength)
                return -1;
            return content[contentOffset++];
        }

        int readBits(int count)
        {
            int result = 0;
            while (--count >= 0)
            {
                int bit = readBit();
                if (bit < 0)
                    return -1;
                result = (result << 1) | bit;
            }
            return result;
        }

        int readLength()
        {
            for (int length = 1; length < (1 << 16); )
            {
                switch (readBit())
                {
                    case 0: return length + 1;
                    case 1: break;
                    default: return -1;
                }
                length <<= 1;
                switch (readBit())
                {
                    case 0: break;
                    case 1: length++; break;
                    default: return -1;
                }
            }
            return BlockEnd;
        }

        bool unpack(u8* unpacked, int headerLength, int unpackedLength)
        {
            contentOffset = headerLength + 3;
            for (int offset = headerLength; offset < unpackedLength; )
            {
                int b;
                switch (readBit())
                {
                    case 0:
                        b = readByte();
                        if (b < 0)
                            return false;
                        unpacked[offset++] = u8(b);
                        break;

                    case 1:
                    {
                        int length = readLength();
                        if (length == BlockEnd)
                        {
                            contentOffset += 2; // skip block length
                            bits = 0;           // reset bit buffer
                            break;
                        }
                        if (length < 0 || offset + length > unpackedLength)
                            return false;
                        int distance = readByte();
                        if (distance < 0)
                            return false;
                        if (distance >= 128)
                        {
                            b = readBits(4);
                            if (b < 0)
                                return false;
                            distance += (b - 1) << 7;
                        }
                        distance++;
                        if (offset - distance < headerLength)
                            return false;
                        do
                        {
                            unpacked[offset] = unpacked[offset - distance];
                            offset++;
                        }
                        while (--length > 0);
                        break;
                    }

                    default:
                        return false;
                }
            }
            return true;
        }
    };

    struct InterfaceG9B : ImageDecodeInterface
    {
        enum class Mode
        {
            MSX6,   // 2 bpp, 4-color palette
            NIBBLE, // 4 bpp, 16-color palette
            BYTE,   // 8 bpp, palette (64-color or GRB332)
            YJK,    // 8 bpp YJK
            YUV,    // 8 bpp YUV
            RGB16,  // 15-bit direct color
        };

        ConstMemory m_memory;
        Buffer m_unpacked;          // holds decompressed stream when needed
        const u8* m_content = nullptr;
        int m_header_length = 0;
        Mode m_mode = Mode::BYTE;
        Color m_palette [256];

        InterfaceG9B(ConstMemory memory)
            : m_memory(memory)
        {
            const u8* content = memory.address;
            const int length = int(memory.size);

            if (length < 17 ||
                content[0] != 'G' || content[1] != '9' || content[2] != 'B' ||
                content[3] != 11 || content[4] != 0)
            {
                header.setError("[ImageDecoder.MSX] Incorrect G9B identifier.");
                return;
            }

            int depth = content[5];
            int colors = content[7];
            int headerLength = 16 + colors * 3;
            int width = content[8] | (content[9] << 8);
            int height = content[10] | (content[11] << 8);

            if (length <= headerLength || width <= 0 || height <= 0 ||
                width > 2048 || height > 2048)
            {
                header.setError("[ImageDecoder.MSX] Incorrect G9B header.");
                return;
            }

            // The unpacked size is computed from the original (pre-substitution)
            // depth, before YJK / YUV remap the 8bpp modes below.
            const int unpackedLength = headerLength + ((width * depth + 7) >> 3) * height;

            for (int c = 0; c < 256; ++c)
                m_palette[c] = Color(0, 0, 0, 0xff);

            switch (depth)
            {
                case 2:
                    if (colors != 4 || !setG9bPalette(content, 4))
                    {
                        header.setError("[ImageDecoder.MSX] Incorrect G9B palette.");
                        return;
                    }
                    m_mode = Mode::MSX6;
                    break;

                case 4:
                    if (colors != 16 || !setG9bPalette(content, 16))
                    {
                        header.setError("[ImageDecoder.MSX] Incorrect G9B palette.");
                        return;
                    }
                    m_mode = Mode::NIBBLE;
                    break;

                case 8:
                    if (colors == 0)
                    {
                        if (width & 3)
                        {
                            header.setError("[ImageDecoder.MSX] Incorrect G9B width.");
                            return;
                        }
                        switch (content[6])
                        {
                            case 0x40:
                                setSc8Palette(m_palette);
                                m_mode = Mode::BYTE;
                                break;
                            case 0x80:
                                m_mode = Mode::YJK;
                                break;
                            case 0xc0:
                                m_mode = Mode::YUV;
                                break;
                            default:
                                header.setError("[ImageDecoder.MSX] Incorrect G9B mode.");
                                return;
                        }
                    }
                    else if (colors == 64)
                    {
                        if (!setG9bPalette(content, 64))
                        {
                            header.setError("[ImageDecoder.MSX] Incorrect G9B palette.");
                            return;
                        }
                        m_mode = Mode::BYTE;
                    }
                    else
                    {
                        header.setError("[ImageDecoder.MSX] Incorrect G9B palette.");
                        return;
                    }
                    break;

                case 16:
                    m_mode = Mode::RGB16;
                    break;

                default:
                    header.setError("[ImageDecoder.MSX] Incorrect G9B depth.");
                    return;
            }

            switch (content[12])
            {
                case 0:
                    if (length != unpackedLength)
                    {
                        header.setError("[ImageDecoder.MSX] Incorrect G9B length.");
                        return;
                    }
                    m_content = content;
                    break;

                case 1:
                {
                    m_unpacked.resize(unpackedLength);
                    G9bStream stream { content, length };
                    if (!stream.unpack(m_unpacked.data(), headerLength, unpackedLength))
                    {
                        header.setError("[ImageDecoder.MSX] G9B decompression failed.");
                        return;
                    }
                    m_content = m_unpacked.data();
                    break;
                }

                default:
                    header.setError("[ImageDecoder.MSX] Incorrect G9B compression.");
                    return;
            }

            m_header_length = headerLength;

            header.width  = width;
            header.height = height;
            header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }

        ~InterfaceG9B()
        {
        }

        // Palette: 3 bytes per entry, each a 5-bit channel. Reject if any
        // channel uses the top three bits (RECOIL treats this as malformed).
        bool setG9bPalette(const u8* content, int colors)
        {
            for (int c = 0; c < colors; ++c)
            {
                int r = content[16 + c * 3 + 0];
                int g = content[16 + c * 3 + 1];
                int b = content[16 + c * 3 + 2];
                if (((r | g | b) & 0xe0) != 0)
                    return false;
                m_palette[c] = Color(expand5(r), expand5(g), expand5(b), 0xff);
            }
            return true;
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);
            return ConstMemory();
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

            DecodeTargetBitmap target(dest, header.width, header.height, header.format);
            decodeImage(target);
            target.resolve();

            status.direct = target.isDirect();
            return status;
        }

        void decodeImage(const Surface& dest)
        {
            const int width = header.width;
            const int height = header.height;
            const u8* content = m_content;

            for (int y = 0; y < height; ++y)
            {
                u8* scan = dest.address<u8>(0, y);

                switch (m_mode)
                {
                    case Mode::MSX6:
                        for (int x = 0; x < width; ++x)
                        {
                            const int offset = y * width + x;
                            const int b = content[m_header_length + (offset >> 2)];
                            store(scan, m_palette[(b >> ((~offset & 3) << 1)) & 3]);
                        }
                        break;

                    case Mode::NIBBLE:
                    {
                        const int stride = (width + 1) >> 1;
                        const u8* row = content + 64 + size_t(y) * stride;
                        for (int x = 0; x < width; ++x)
                        {
                            const int b = row[x >> 1];
                            const int nibble = (x & 1) == 0 ? (b >> 4) : (b & 15);
                            store(scan, m_palette[nibble]);
                        }
                        break;
                    }

                    case Mode::BYTE:
                    {
                        const u8* row = content + m_header_length + size_t(y) * width;
                        for (int x = 0; x < width; ++x)
                            store(scan, m_palette[row[x]]);
                        break;
                    }

                    case Mode::YJK:
                    {
                        const u8* row = content + 16 + size_t(y) * width;
                        for (int x = 0; x < width; ++x)
                            store(scan, msxYjkColor(row, x, width, m_palette, false));
                        break;
                    }

                    case Mode::YUV:
                    {
                        const u8* row = content + 16 + size_t(y) * width;
                        for (int x = 0; x < width; ++x)
                        {
                            const int yv = row[x] >> 3;
                            const u8* p = row + (x & ~3);
                            int v = (p[0] & 7) | ((p[1] & 7) << 3);
                            int u = (p[2] & 7) | ((p[3] & 7) << 3);
                            u -= (u & 0x20) << 1;
                            v -= (v & 0x20) << 1;
                            const int r = clampU5(yv + u);
                            const int g = clampU5((((5 * yv - v) >> 1) - u) >> 1);
                            const int b = clampU5(yv + v);
                            store(scan, Color(expand5(r), expand5(g), expand5(b), 0xff));
                        }
                        break;
                    }

                    case Mode::RGB16:
                    {
                        const u8* row = content + 16 + size_t(y) * width * 2;
                        for (int x = 0; x < width; ++x)
                        {
                            const int c = row[x * 2] | (row[x * 2 + 1] << 8);
                            // XGGGGGRR RRRBBBBB
                            int rgb = ((c & 0x3e0) << 14) | ((c & 0x7c00) << 1) | ((c & 0x1f) << 3);
                            rgb |= (rgb >> 5) & 0x070707;
                            store(scan, Color((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff, 0xff));
                        }
                        break;
                    }
                }
            }
        }

        void store(u8*& d, Color color)
        {
            d[0] = color.r;
            d[1] = color.g;
            d[2] = color.b;
            d[3] = 0xff;
            d += 4;
        }
    };

    ImageDecodeInterface* createInterfaceG9B(ConstMemory memory)
    {
        return new InterfaceG9B(memory);
    }

    // ------------------------------------------------------------
    // MSX interchange formats (.MIF, .MIG)
    // ------------------------------------------------------------
    //
    // Two compressed container formats that can carry any MSX2 / MSX2+ SCREEN
    // mode (2, 3, 5, 6, 7, 8, 10, 12) including the 424-line interlaced
    // hi-color variants. Both unpack to a raw VRAM image which is then rendered
    // with the standard MSX screen decoders.
    //
    //   .MIF  starts with a 1-byte SCREEN selector, an optional 32-byte VDP
    //         palette and an LZW-compressed bitmap.
    //   .MIG  "MSXMIG" magic + a custom LZ stream that unpacks to a sequence of
    //         tagged chunks (VDP registers / palette / bitmap); the SCREEN mode
    //         is recovered from the emulated register state.
    //
    // Reference: RECOIL, DecodeMif / DecodeMig.

    // Shared MSB-first bit reader (8 bits sliding left with a trailing 1).
    struct MsxBitStream
    {
        const u8* content;
        int contentOffset;
        int contentLength;
        int bits = 0;

        int readBit()
        {
            if ((bits & 0x7f) == 0)
            {
                if (contentOffset >= contentLength)
                    return -1;
                bits = (content[contentOffset++] << 1) | 1;
            }
            else
            {
                bits <<= 1;
            }
            return (bits >> 8) & 1;
        }

        int readByte()
        {
            if (contentOffset >= contentLength)
                return -1;
            return content[contentOffset++];
        }

        int readBits(int count)
        {
            int result = 0;
            while (--count >= 0)
            {
                int bit = readBit();
                if (bit < 0)
                    return -1;
                result = (result << 1) | bit;
            }
            return result;
        }
    };

    struct InterfaceMSXi : ImageDecodeInterface
    {
        enum class Kind { MIF, MIG };
        enum Scale { PLAIN, DOUBLE_X, DOUBLE_Y };

        ConstMemory m_memory;
        Kind m_kind;
        std::vector<Color> m_pixels;
        int m_width = 0;
        int m_height = 0;
        Scale m_scale = PLAIN;
        Color m_palette [256];
        int m_colors = 0;

        InterfaceMSXi(ConstMemory memory, Kind kind)
            : m_memory(memory)
            , m_kind(kind)
        {
            for (int i = 0; i < 256; ++i)
                m_palette[i] = Color(0, 0, 0, 0xff);

            bool ok = (kind == Kind::MIF) ? decodeMif() : decodeMig();
            if (!ok)
            {
                header.setError("[ImageDecoder.MSX] Unsupported or invalid MSX interchange image.");
                return;
            }

            header.width  = m_width;
            header.height = m_height;
            header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }

        ~InterfaceMSXi()
        {
        }

        // --- geometry / scaling (mirrors RECOIL SetScaledPixel) ---

        int originalWidth() const { return m_scale == DOUBLE_X ? (m_width >> 1) : m_width; }
        int originalHeight() const { return m_scale == DOUBLE_Y ? (m_height >> 1) : m_height; }

        void setSize(int width, int height, Scale scale)
        {
            m_width = width;
            m_height = height;
            m_scale = scale;
            m_pixels.assign(size_t(width) * height, Color(0, 0, 0, 0xff));
        }

        void setScaledPixel(int x, int y, Color rgb)
        {
            switch (m_scale)
            {
                case DOUBLE_X:
                {
                    int o = y * m_width + (x << 1);
                    m_pixels[o] = m_pixels[o + 1] = rgb;
                    break;
                }
                case DOUBLE_Y:
                {
                    int o = (y * m_width << 1) + x;
                    m_pixels[o] = m_pixels[o + m_width] = rgb;
                    break;
                }
                default:
                    m_pixels[y * m_width + x] = rgb;
                    break;
            }
        }

        void loadPalette(const u8* buf, int offset, int colors)
        {
            setMsxPalette(m_palette, buf, offset, colors);
            m_colors = colors;
        }

        // --- pixel renderers (operate on the unpacked VRAM image) ---

        void decodeNibbles(const u8* buf, int offset, int stride)
        {
            const int ow = originalWidth();
            const int oh = originalHeight();
            for (int y = 0; y < oh; ++y)
                for (int x = 0; x < ow; ++x)
                {
                    int b = buf[offset + y * stride + (x >> 1)];
                    int nibble = (x & 1) == 0 ? (b >> 4) : (b & 0xf);
                    setScaledPixel(x, y, m_palette[nibble]);
                }
        }

        void decodeMsx6(const u8* buf, int offset)
        {
            const int oh = originalHeight();
            for (int y = 0; y < oh; ++y)
                for (int x = 0; x < m_width; ++x)
                {
                    int o = y * m_width + x;
                    int b = buf[offset + (o >> 2)];
                    int index = (b >> ((~x & 3) << 1)) & 3;
                    setScaledPixel(x, y, m_palette[index]);
                }
        }

        void decodeBytes(const u8* buf, int offset)
        {
            const int ow = originalWidth();
            const int oh = originalHeight();
            for (int y = 0; y < oh; ++y)
                for (int x = 0; x < ow; ++x)
                    setScaledPixel(x, y, m_palette[buf[offset + y * ow + x]]);
        }

        void decodeYjkScreen(const u8* buf, int offset, bool usePalette)
        {
            const int ow = originalWidth();
            const int oh = originalHeight();
            for (int y = 0; y < oh; ++y)
                for (int x = 0; x < ow; ++x)
                    setScaledPixel(x, y, msxYjkColor(buf + offset + y * ow, x, ow, m_palette, usePalette));
        }

        // SCREEN 2/4 tile mode (256x192).
        void decodeSc2Sc4(const u8* buf, int offset)
        {
            setSize(256, 192, PLAIN);
            for (int y = 0; y < 192; ++y)
            {
                int fontOffset = offset + ((y & 0xc0) << 5) + (y & 7);
                for (int x = 0; x < 256; ++x)
                {
                    int b = fontOffset + (buf[offset + 0x1800 + ((y & ~7) << 2) + (x >> 3)] << 3);
                    int c = buf[0x2000 + b];
                    int idx = ((buf[b] >> (~x & 7)) & 1) == 0 ? (c & 0xf) : (c >> 4);
                    m_pixels[(y << 8) + x] = m_palette[idx];
                }
            }
        }

        // SCREEN 3 multicolor (256x192). The "long" name-table form is never
        // used by MIF/MIG, so only the implicit layout is handled.
        void decodeSc3Screen(const u8* buf, int offset)
        {
            setSize(256, 192, PLAIN);
            for (int y = 0; y < 192; ++y)
                for (int x = 0; x < 256; ++x)
                {
                    int c = (y & 0xe0) + (x >> 3);
                    c = (buf[offset + (c << 3) + ((y >> 2) & 7)] >> (~x & 4)) & 0xf;
                    m_pixels[(y << 8) + x] = m_palette[c];
                }
        }

        // The MIG renderer: writes the final (possibly interlaced) image
        // directly, recovering vertical doubling / field interleave from the
        // pixel offset arithmetic.
        void decodeMsxScreen(const u8* buf, int offset, int height, int mode, int interlaceMask)
        {
            if (interlaceMask != 0)
            {
                Scale scale = (mode >= 10) ? DOUBLE_X
                            : (mode >> 1 == 3) ? PLAIN
                            : DOUBLE_X;
                setSize(512, height << 1, scale);
            }
            else if (mode >> 1 == 3)
            {
                setSize(512, height << 1, DOUBLE_Y);
            }
            else
            {
                setSize(256, height, PLAIN);
            }

            for (int y = 0; y < m_height; ++y)
            {
                int screenOffset = (y & interlaceMask) == 0
                    ? offset
                    : offset + (mode <= 6 ? 0x6a07 : 0xd407);

                for (int x = 0; x < m_width; ++x)
                {
                    Color rgb(0, 0, 0, 0xff);
                    switch (mode)
                    {
                        case 5:
                        {
                            int o = screenOffset + ((y >> interlaceMask) << 7);
                            int idx = x >> interlaceMask;
                            int b = buf[o + (idx >> 1)];
                            rgb = m_palette[(idx & 1) == 0 ? (b >> 4) : (b & 0xf)];
                            break;
                        }
                        case 6:
                            rgb = m_palette[(buf[screenOffset + ((y >> 1) << 7) + (x >> 2)] >> ((~x & 3) << 1)) & 3];
                            break;
                        case 7:
                        {
                            int o = screenOffset + ((y >> 1) << 8);
                            int b = buf[o + (x >> 1)];
                            rgb = m_palette[(x & 1) == 0 ? (b >> 4) : (b & 0xf)];
                            break;
                        }
                        case 8:
                            rgb = m_palette[buf[screenOffset + ((y >> interlaceMask) << 8) + (x >> interlaceMask)]];
                            break;
                        case 10:
                            rgb = msxYjkColor(buf + screenOffset + ((y >> interlaceMask) << 8), x >> interlaceMask, 256, m_palette, true);
                            break;
                        case 12:
                            rgb = msxYjkColor(buf + screenOffset + ((y >> interlaceMask) << 8), x >> interlaceMask, 256, m_palette, false);
                            break;
                    }
                    m_pixels[y * m_width + x] = rgb;
                }
            }
        }

        // --- MIF (LZW) ---

        bool unpackMif(std::vector<u8>& unpacked, int unpackedLength, bool palette)
        {
            const u8* content = m_memory.address;
            const int length = int(m_memory.size);

            MsxBitStream s { content, palette ? 34 : 2, length };
            std::vector<int> offsets(16384 / 3);
            int codes = 0;
            int codeBits = 2;

            for (int offset = 0; offset < unpackedLength; )
            {
                if (codes >= int(offsets.size()))
                    return false;
                offsets[codes++] = offset;
                if (codes >> codeBits)
                    codeBits++;

                int bit = s.readBit();
                if (bit == 0)
                {
                    int code = s.readBits(codeBits);
                    if (code < 0 || code >= codes)
                        return false;
                    if (code == codes - 1)
                    {
                        codes = 0;
                        codeBits = 2;
                        continue;
                    }
                    int sourceOffset = offsets[code];
                    int endOffset = offsets[code + 1];
                    if (offset + endOffset - sourceOffset >= unpackedLength)
                        return false;
                    do
                    {
                        unpacked[offset++] = unpacked[sourceOffset++];
                    }
                    while (sourceOffset <= endOffset);
                }
                else if (bit == 1)
                {
                    int b = s.readBits(8);
                    if (b < 0)
                        return false;
                    unpacked[offset++] = u8(b);
                }
                else
                {
                    return false;
                }
            }

            if (s.readBit() != 0 || s.readBits(codeBits) <= codes || s.contentOffset != length)
                return false;
            if (palette)
                loadPalette(content, 2, 16);
            return true;
        }

        bool decodeMif()
        {
            const u8* content = m_memory.address;
            const int length = int(m_memory.size);
            if (length < 35)
                return false;

            std::vector<u8> unpacked(108544);

            switch (content[0])
            {
                case 0x00:
                    if (!unpackMif(unpacked, 27136, true)) return false;
                    setSize(256, 212, PLAIN);
                    decodeNibbles(unpacked.data(), 0, 128);
                    return true;
                case 0x01:
                    if (!unpackMif(unpacked, 27136, true)) return false;
                    setSize(512, 424, DOUBLE_Y);
                    decodeMsx6(unpacked.data(), 0);
                    return true;
                case 0x02:
                    if (!unpackMif(unpacked, 54272, true)) return false;
                    setSize(512, 424, DOUBLE_Y);
                    decodeNibbles(unpacked.data(), 0, 256);
                    return true;
                case 0x03:
                    if (!unpackMif(unpacked, 54272, false)) return false;
                    setSc8Palette(m_palette);
                    setSize(256, 212, PLAIN);
                    decodeBytes(unpacked.data(), 0);
                    return true;
                case 0x04:
                    if (!unpackMif(unpacked, 54272, true)) return false;
                    setSize(256, 212, PLAIN);
                    decodeYjkScreen(unpacked.data(), 0, true);
                    return true;
                case 0x05:
                    if (!unpackMif(unpacked, 54272, false)) return false;
                    setSize(256, 212, PLAIN);
                    decodeYjkScreen(unpacked.data(), 0, false);
                    return true;
                case 0x08:
                    if (!unpackMif(unpacked, 14336, true)) return false;
                    decodeSc2Sc4(unpacked.data(), 0);
                    return true;
                case 0x09:
                    if (!unpackMif(unpacked, 1536, true)) return false;
                    decodeSc3Screen(unpacked.data(), 0);
                    return true;
                case 0x10:
                    if (!unpackMif(unpacked, 54272, true)) return false;
                    setSize(512, 424, DOUBLE_X);
                    decodeNibbles(unpacked.data(), 0, 128);
                    return true;
                case 0x11:
                    if (!unpackMif(unpacked, 54272, true)) return false;
                    setSize(512, 424, PLAIN);
                    decodeMsx6(unpacked.data(), 0);
                    return true;
                case 0x12:
                    if (!unpackMif(unpacked, 108544, true)) return false;
                    setSize(512, 424, PLAIN);
                    decodeNibbles(unpacked.data(), 0, 256);
                    return true;
                case 0x13:
                    if (!unpackMif(unpacked, 108544, false)) return false;
                    setSc8Palette(m_palette);
                    setSize(512, 424, DOUBLE_X);
                    decodeBytes(unpacked.data(), 0);
                    return true;
                case 0x14:
                    if (!unpackMif(unpacked, 108544, true)) return false;
                    setSize(512, 424, DOUBLE_X);
                    decodeYjkScreen(unpacked.data(), 0, true);
                    return true;
                case 0x15:
                    if (!unpackMif(unpacked, 108544, false)) return false;
                    setSize(512, 424, DOUBLE_X);
                    decodeYjkScreen(unpacked.data(), 0, false);
                    return true;
                default:
                    return false;
            }
        }

        // --- MIG (register state machine + custom LZ) ---

        static int migMode(int reg0, int reg1, int reg19, int length)
        {
            return (reg0 & 0x0e) | ((reg1 & 0x18) << 1) | ((reg19 & 0x18) << 3) | (length << 8);
        }

        // MIG LZ. Returns unpacked length or -1 on error.
        int unpackMig(std::vector<u8>& unpacked)
        {
            const int maxUnpacked = 108800;
            const u8* content = m_memory.address;
            const int length = int(m_memory.size);

            MsxBitStream s { content, 11 + 4, length };

            for (int offset = 0; offset < maxUnpacked; )
            {
                int c = s.readBit();
                if (c < 0)
                    return -1;
                int b = s.readByte();
                if (b < 0)
                    return -1;

                if (c == 0)
                {
                    unpacked[offset++] = u8(b);
                }
                else
                {
                    if (b >= 128)
                    {
                        c = s.readBits(4);
                        if (c < 0)
                            return -1;
                        b += (c - 1) << 7;
                    }
                    int distance = b + 1;
                    if (offset - distance < 0)
                        return -1;

                    c = -1;
                    do
                    {
                        b = s.readBit();
                        if (b < 0)
                            return -1;
                        c++;
                    }
                    while (b != 0);

                    int len = s.readBits(c);
                    if (len < 0)
                        return -1;

                    if (c >= 16)
                    {
                        s.contentOffset += 4; // skip block lengths
                        if (s.contentOffset >= s.contentLength)
                            return offset;
                        s.bits = 0;
                    }
                    else
                    {
                        len += (1 << c) + 1;
                        if (offset + len > maxUnpacked)
                            return -1;
                        do
                        {
                            unpacked[offset] = unpacked[offset - distance];
                            offset++;
                        }
                        while (--len > 0);
                    }
                }
            }
            return -1;
        }

        bool decodeMig()
        {
            const u8* content = m_memory.address;
            const int length = int(m_memory.size);

            if (length < 16 ||
                std::memcmp(content, "MSXMIG", 6) != 0 ||
                (content[6] | (content[7] << 8) | (content[8] << 16) | (content[9] << 24)) != length - 6)
                return false;

            std::vector<u8> unpacked(108800);
            int unpackedLength = unpackMig(unpacked);
            if (unpackedLength <= 0)
                return false;

            const u8* buf = unpacked.data();
            int colors = 0;
            u8 registers [256] = { 0 };

            for (int offset = 0; offset < unpackedLength; )
            {
                switch (buf[offset])
                {
                    case 0: // VDP registers
                    {
                        if (offset + 1 >= unpackedLength)
                            return false;
                        int c = buf[offset + 1];
                        if (offset + 2 + c * 3 > unpackedLength)
                            return false;
                        for (int i = 0; i < c; ++i)
                        {
                            int o = offset + 2 + i * 3;
                            int r = buf[o];
                            int m = buf[o + 2];
                            registers[r] = u8((registers[r] & ~m) | (buf[o + 1] & m));
                        }
                        offset += 2 + c * 3;
                        break;
                    }

                    case 1: // palette
                    {
                        if (offset + 2 >= unpackedLength || buf[offset + 1] != 0)
                            return false;
                        colors = buf[offset + 2];
                        if (offset + 3 + (colors << 1) > unpackedLength)
                            return false;
                        loadPalette(buf, offset + 3, colors);
                        offset += 3 + (colors << 1);
                        break;
                    }

                    case 2: // bitmap
                    {
                        if (offset + 7 >= unpackedLength ||
                            buf[offset + 1] || buf[offset + 2] || buf[offset + 3] ||
                            buf[offset + 4] || buf[offset + 6])
                            return false;
                        int blockLength = buf[offset + 5];
                        offset += 7;

                        int interlaceMask;
                        switch (registers[9] & 0x0c)
                        {
                            case 0x00:
                                if (offset + (blockLength << 8) + 1 != unpackedLength)
                                    return false;
                                interlaceMask = 0;
                                break;
                            case 0x0c:
                                if (offset + (blockLength << 9) + 7 + 1 != unpackedLength ||
                                    buf[offset + (blockLength << 8)] != 2 ||
                                    buf[offset + (blockLength << 8) + 1] != 0 ||
                                    buf[offset + (blockLength << 8) + 4] != 0 ||
                                    buf[offset + (blockLength << 8) + 5] != blockLength ||
                                    buf[offset + (blockLength << 8) + 6] != 0)
                                    return false;
                                interlaceMask = 1;
                                break;
                            default:
                                return false;
                        }

                        int key = migMode(registers[0], registers[1], registers[0x19], blockLength);

                        if (key == migMode(0x02, 0x00, 0x00, 0x38))
                        {
                            if (colors < 16 || interlaceMask != 0) return false;
                            decodeSc2Sc4(buf, offset);
                            return true;
                        }
                        if (key == migMode(0x00, 0x08, 0x00, 0x06))
                        {
                            if (colors < 16 || interlaceMask != 0) return false;
                            decodeSc3Screen(buf, offset);
                            return true;
                        }

                        int mode;
                        if (key == migMode(0x06, 0x00, 0x00, 0x6a)) { if (colors < 16) return false; mode = 5; }
                        else if (key == migMode(0x08, 0x00, 0x00, 0x6a)) { if (colors < 4) return false; mode = 6; }
                        else if (key == migMode(0x0a, 0x00, 0x00, 0xd4)) { if (colors < 16) return false; mode = 7; }
                        else if (key == migMode(0x0e, 0x00, 0x00, 0xd4)) { setSc8Palette(m_palette); mode = 8; }
                        else if (key == migMode(0x0e, 0x00, 0x18, 0xd4)) { if (colors < 16) return false; mode = 10; }
                        else if (key == migMode(0x0e, 0x00, 0x08, 0xd4)) { mode = 12; }
                        else return false;

                        decodeMsxScreen(buf, offset, 212, mode, interlaceMask);
                        return true;
                    }

                    default:
                        return false;
                }
            }
            return false;
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);
            return ConstMemory();
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

            DecodeTargetBitmap target(dest, header.width, header.height, header.format);

            for (int y = 0; y < m_height; ++y)
            {
                const Color* src = &m_pixels[size_t(y) * m_width];
                u8* d = target.address<u8>(0, y);
                for (int x = 0; x < m_width; ++x)
                {
                    d[0] = src[x].r;
                    d[1] = src[x].g;
                    d[2] = src[x].b;
                    d[3] = 0xff;
                    d += 4;
                }
            }

            target.resolve();

            status.direct = target.isDirect();
            return status;
        }
    };

    template <InterfaceMSXi::Kind kind>
    ImageDecodeInterface* createInterfaceMSXi(ConstMemory memory)
    {
        return new InterfaceMSXi(memory, kind);
    }

} // namespace

namespace mango::image
{

    void registerImageCodecMSX()
    {
        registerImageDecoder(createInterface<Screen::SC2>, ".sc2");
        registerImageDecoder(createInterface<Screen::SC4>, ".sc4");
        registerImageDecoder(createInterface<Screen::SC5>, ".sc5");
        registerImageDecoder(createInterface<Screen::SR5>, ".sr5");
        registerImageDecoder(createInterface<Screen::SC6>, ".sc6");
        registerImageDecoder(createInterface<Screen::SC7>, ".sc7");
        registerImageDecoder(createInterface<Screen::SC8>, ".sc8");
        registerImageDecoder(createInterface<Screen::SCA>, ".sca");
        registerImageDecoder(createInterface<Screen::SCC>, ".scc");
        registerImageDecoder(createInterfaceG9B, ".g9b");
        registerImageDecoder(createInterfaceMSXi<InterfaceMSXi::Kind::MIF>, ".mif");
        registerImageDecoder(createInterfaceMSXi<InterfaceMSXi::Kind::MIG>, ".mig");
    }

} // namespace mango::image
