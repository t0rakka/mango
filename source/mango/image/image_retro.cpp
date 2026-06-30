/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    Assorted retro computer image formats.

    This is the catch-all home for the long tail of vintage formats that don't
    warrant a dedicated translation unit. Platform families with many modes -
    Atari (image_atari / image_atari8), Commodore 64 (image_c64) and MSX
    (image_msx) - keep their own units; everything else lives here.

    Currently implemented:

        .mag            MAKIchan Graphics
        .shr .3200      Apple IIGS Super Hi-Res
        .scr            ZX Spectrum screen

    ------------------------------------------------------------------------
    MAKIchan Graphics (.MAG)

    MAKIchan is one of the most common art formats of the Japanese personal
    computer scene (NEC PC-88/PC-98, Sharp X68000, MSX2/2+, ...). A single
    container ("MAKI02") carries images for all of these machines; the target
    platform - and therefore the pixel aspect and palette precision - is stored
    in the header.

    The bitmap is stored as a flag-plane compressed delta stream: a bit plane
    (flag A) selects which 4-pixel groups carry a new delta byte (flag B); each
    4-bit delta either copies a literal palette byte or back-references an
    earlier pixel (LZ-style). The palette is stored G,R,B and is restricted to
    the target platform's color precision.

    Standard 16-color (nibble) and 256-color (byte) pictures are supported,
    including the MSX2 1x2 / 2x1 display-aspect variants. The rarer MSX YJK and
    SCREEN 6 sub-modes are not handled.

*/
#include <vector>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // Target machine color precision (selects the palette restriction).
    enum class Platform
    {
        Identity,   // PC-80, Macintosh: use stored bytes as-is
        MSX2,       // 3 bits per channel
        PC88,       // 4 bits per channel
        PC88VA,     // 5/6/5-ish
        PC98,       // 4 bits per channel when 16 colors, else identity
        X68K        // 5 bits per channel + extra low bit
    };

    enum class PixelMode
    {
        Nibble,     // 16 colors, 4 bpp
        Byte,       // 256 colors, 8 bpp
        Yjk,        // MSX2+ YJK (optionally with YAE palette pixels)
        Msx6,       // MSX SCREEN 6, 4 colors, 2 bpp
        Unsupported
    };

    static inline int clampU5(int x)
    {
        return x < 0 ? 0 : (x > 31 ? 31 : x);
    }

    // 5-bit channel to 8-bit.
    static inline u8 expand5(int v)
    {
        return u8((v << 3) | (v >> 2));
    }

    // MSB-first bit reader (sentinel bit in bit 7).
    struct BitReader
    {
        const u8* p;
        const u8* end;
        u32 bits = 0;

        BitReader(const u8* begin, const u8* finish)
            : p(begin)
            , end(finish)
        {
        }

        int readBit()
        {
            if ((bits & 127) == 0)
            {
                if (p >= end)
                    return -1;
                bits = (u32(*p++) << 1) | 1;
            }
            else
            {
                bits <<= 1;
            }
            return (bits >> 8) & 1;
        }
    };

    // Apply the target platform's color-channel precision. rgb is 0x00RRGGBB.
    static u32 restrictColor(Platform platform, u32 rgb, int colors)
    {
        switch (platform)
        {
            case Platform::MSX2:
                rgb &= 0xe0e0e0;
                return rgb | (rgb >> 3) | ((rgb >> 6) & 0x030303);

            case Platform::PC88:
                rgb &= 0xf0f0f0;
                return rgb | (rgb >> 4);

            case Platform::PC88VA:
                return (rgb & 0xf8fcf8) | ((rgb >> 5) & 0x070007) | ((rgb >> 6) & 0x000300);

            case Platform::PC98:
                if (colors == 16)
                {
                    rgb &= 0xf0f0f0;
                    rgb |= rgb >> 4;
                }
                return rgb;

            case Platform::X68K:
                return (rgb & 0xf8f8f8) | (((rgb >> 10) & 1) * 0x040404) | ((rgb >> 6) & 0x030303);

            case Platform::Identity:
            default:
                return rgb;
        }
    }

    static u32 le32(const u8* p)
    {
        return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct MagDecoder : ImageDecodeInterface
    {
        ConstMemory m_memory;

        bool m_valid = false;
        size_t m_header = 0;            // offset of the binary header (past the 0x1A text)
        int m_colors = 16;
        int m_bytes_per_line = 0;
        int m_width = 0;                // original (unscaled) pixel dimensions
        int m_height = 0;
        int m_scale_x = 1;
        int m_scale_y = 1;
        Platform m_platform = Platform::Identity;
        PixelMode m_mode = PixelMode::Nibble;
        bool m_yjk_palette = false;     // YJK: use YAE palette pixels

        MagDecoder(ConstMemory memory)
            : m_memory(memory)
        {
            parse(memory.address, memory.size);

            if (!m_valid)
            {
                header.setError("[ImageDecoder.MAKICHAN] Unsupported or corrupt file.");
                return;
            }

            header.width  = m_width * m_scale_x;
            header.height = m_height * m_scale_y;
            header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }

        ~MagDecoder()
        {
        }

        void parse(const u8* data, size_t size)
        {
            if (size < 8 || std::memcmp(data, "MAKI02  ", 8) != 0)
                return;

            // The variable-length comment ends with a 0x1A (EOF) byte; the
            // binary header follows.
            size_t ho = 0;
            do
            {
                if (ho >= size)
                    return;
            }
            while (data[ho++] != 0x1a);

            if (ho + 80 > size || data[ho] != 0)
                return;

            const int machine = data[ho + 1];
            const int screen = data[ho + 2];
            const int flags = data[ho + 3];

            const int left = data[ho + 4] | (data[ho + 5] << 8);
            int width = (data[ho + 8] | (data[ho + 9] << 8)) + 1;

            if (flags < 128)
            {
                width -= left & ~7;
                m_bytes_per_line = (width + 1) >> 1;
                m_colors = 16;
            }
            else
            {
                if (ho + 800 >= size)
                    return;
                m_bytes_per_line = width -= left & ~3;
                m_colors = 256;
            }

            m_mode = m_colors == 16 ? PixelMode::Nibble : PixelMode::Byte;

            // Resolve target platform (palette precision) and display aspect.
            int msxMode = 0;
            switch (machine)
            {
                case 0:
                case 136:
                    m_platform = (flags & 1) == 0 ? Platform::PC88VA : Platform::PC88;
                    if ((flags & 1) != 0)
                        m_scale_y = 2; // PC881X2
                    break;

                case 3:
                    m_platform = Platform::MSX2;
                    msxMode = screen & 252;
                    switch (msxMode)
                    {
                        case 0:
                        case 20:
                        case 84:
                            break; // MSX2 1x1
                        case 4:
                            m_scale_y = 2; // MSX2 1x2
                            break;
                        case 16:
                        case 80:
                            m_scale_x = 2; // MSX2 2x1 interlaced
                            break;
                        case 32: // YJK + YAE, 2x1
                        case 36: // YJK + YAE, 1x1
                            if (m_colors == 16)
                                width >>= 1;
                            m_mode = PixelMode::Yjk;
                            m_yjk_palette = true;
                            m_scale_x = msxMode == 32 ? 2 : 1;
                            break;
                        case 64: // YJK, 2x1
                        case 68: // YJK, 1x1
                            if (m_colors == 16)
                                width >>= 1;
                            m_mode = PixelMode::Yjk;
                            m_yjk_palette = false;
                            m_scale_x = msxMode == 64 ? 2 : 1;
                            break;
                        case 96:  // SCREEN 6, 1x1
                        case 100: // SCREEN 6, 1x2
                            width = m_bytes_per_line << 2;
                            m_mode = PixelMode::Msx6;
                            m_scale_y = msxMode == 100 ? 2 : 1;
                            break;
                        default:
                            m_mode = PixelMode::Unsupported;
                            break;
                    }
                    break;

                case 98:
                case 112:
                    m_platform = Platform::PC98;
                    break;

                case 104:
                    m_platform = Platform::X68K;
                    break;

                case 128:
                    m_platform = Platform::Identity; // PC-80
                    m_scale_y = 2;
                    break;

                case 153:
                    m_platform = Platform::Identity; // Macintosh
                    break;

                default:
                    m_platform = Platform::MSX2;
                    if ((flags & 1) != 0)
                        m_scale_y = 2;
                    break;
            }

            if (m_mode == PixelMode::Unsupported)
                return;

            const int height = (data[ho + 10] | (data[ho + 11] << 8)) - (data[ho + 6] | (data[ho + 7] << 8)) + 1;
            if (width <= 0 || height <= 0 || width > 8192 || height > 8192)
                return;

            m_header = ho;
            m_width = width;
            m_height = height;
            m_valid = true;
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);
            return ConstMemory();
        }

        // Reconstruct the linear pixel buffer (bytes_per_line * height) from the
        // flag-plane delta stream. Returns false on malformed data.
        bool unpack(std::vector<u8>& unpacked)
        {
            const u8* data = m_memory.address;
            const size_t size = m_memory.size;
            const size_t ho = m_header;

            const u8* flagA = data + ho + le32(data + ho + 12);
            size_t deltaOffset = ho + le32(data + ho + 16);
            size_t colorOffset = ho + le32(data + ho + 24);

            if (flagA < data || flagA > data + size)
                return false;

            BitReader haveDelta(flagA, data + size);

            const int bpl = m_bytes_per_line;
            const int height = m_height;
            unpacked.assign(size_t(bpl) * height, 0);

            std::vector<u8> deltas((bpl + 3) >> 2, 0);

            static const u8 DELTA_X [16] = { 0, 2, 4, 8, 0, 2, 0, 2, 4, 0, 2, 4, 0, 2, 4, 0 };
            static const u8 DELTA_Y [16] = { 0, 0, 0, 0, 1, 1, 2, 2, 2, 4, 4, 4, 8, 8, 8, 16 };

            for (int y = 0; y < height; ++y)
            {
                int delta = 0;
                for (int x = 0; x < bpl; ++x)
                {
                    if ((x & 1) == 0)
                    {
                        delta = deltas[x >> 2];
                        if ((x & 2) == 0)
                        {
                            int bit = haveDelta.readBit();
                            if (bit < 0)
                                return false;
                            if (bit == 1)
                            {
                                if (deltaOffset >= size)
                                    return false;
                                delta ^= data[deltaOffset++];
                                deltas[x >> 2] = u8(delta);
                            }
                            delta >>= 4;
                        }
                        else
                        {
                            delta &= 15;
                        }
                    }

                    if (delta == 0)
                    {
                        if (colorOffset >= size)
                            return false;
                        unpacked[size_t(y) * bpl + x] = data[colorOffset++];
                    }
                    else
                    {
                        int sx = x - DELTA_X[delta];
                        int sy = y - DELTA_Y[delta];
                        if (sx < 0 || sy < 0)
                            return false;
                        unpacked[size_t(y) * bpl + x] = unpacked[size_t(sy) * bpl + sx];
                    }
                }

                if ((bpl & 1) != 0 && delta == 0)
                    colorOffset++;
                if (((bpl + 1) & 2) != 0 && (deltas[bpl >> 2] & 15) == 0)
                    colorOffset += 2;
            }

            return true;
        }

        // Decode one MSX2+ YJK pixel. Groups of 4 pixels
        // share the J/K chroma; odd luma with YAE selects a palette color.
        Color yjkColor(const u8* row, int x, int width, const Color* palette)
        {
            int yy = row[x] >> 3;
            if (m_yjk_palette && (yy & 1))
                return palette[yy >> 1];

            int r, g, b;
            if ((x | 3) >= width)
            {
                r = g = b = yy;
            }
            else
            {
                const u8* p = row + (x & ~3);
                int k = (p[0] & 7) | ((p[1] & 7) << 3);
                int j = (p[2] & 7) | ((p[3] & 7) << 3);
                k -= (k & 32) << 1;
                j -= (j & 32) << 1;
                r = clampU5(yy + j);
                g = clampU5(yy + k);
                b = clampU5((5 * yy - 2 * j - k + 2) >> 2);
            }

            return Color(expand5(r), expand5(g), expand5(b), 0xff);
        }

        void buildPalette(Color* palette)
        {
            const u8* data = m_memory.address;
            const u8* p = data + m_header + 32;

            for (int c = 0; c < m_colors; ++c)
            {
                // Stored as G, R, B.
                u32 rgb = (u32(p[1]) << 16) | (u32(p[0]) << 8) | u32(p[2]);
                rgb = restrictColor(m_platform, rgb, m_colors);
                palette[c] = Color((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff, 0xff);
                p += 3;
            }
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
            std::vector<u8> unpacked;
            if (!unpack(unpacked))
            {
                // Leave the (zero-initialized) target as-is on failure.
                return;
            }

            Color palette [256];
            buildPalette(palette);

            const int bpl = m_bytes_per_line;

            for (int y = 0; y < m_height; ++y)
            {
                for (int x = 0; x < m_width; ++x)
                {
                    Color color;

                    switch (m_mode)
                    {
                        case PixelMode::Nibble:
                        {
                            const u8* row = unpacked.data() + size_t(y) * bpl;
                            int index = (x & 1) == 0 ? (row[x >> 1] >> 4) : (row[x >> 1] & 15);
                            color = palette[index];
                            break;
                        }

                        case PixelMode::Byte:
                        {
                            const u8* row = unpacked.data() + size_t(y) * bpl;
                            color = palette[row[x]];
                            break;
                        }

                        case PixelMode::Yjk:
                        {
                            const u8* row = unpacked.data() + size_t(y) * m_width;
                            color = yjkColor(row, x, m_width, palette);
                            break;
                        }

                        case PixelMode::Msx6:
                        {
                            size_t offset = size_t(y) * m_width + x;
                            int b = unpacked[offset >> 2];
                            int index = (b >> ((~offset & 3) << 1)) & 3;
                            color = palette[index];
                            break;
                        }

                        default:
                            color = Color(0, 0, 0, 0xff);
                            break;
                    }

                    // Expand to the display aspect (1x2 / 2x1).
                    for (int dy = 0; dy < m_scale_y; ++dy)
                    {
                        u8* d = dest.address<u8>(x * m_scale_x, y * m_scale_y + dy);
                        for (int dx = 0; dx < m_scale_x; ++dx)
                        {
                            d[0] = color.r;
                            d[1] = color.g;
                            d[2] = color.b;
                            d[3] = 0xff;
                            d += 4;
                        }
                    }
                }
            }
        }
    };

    ImageDecodeInterface* createInterfaceMAG(ConstMemory memory)
    {
        return new MagDecoder(memory);
    }

    // ------------------------------------------------------------------------
    // Apple IIGS Super Hi-Res (.SHR, .3200)
    //
    // The Apple IIGS Super Hi-Res screen is 320x200 with 16 colors per line,
    // each color a 12-bit $0RGB value. A 200-byte "scanline control byte" table
    // selects one of 16 palettes (each 32 bytes) per line. Several on-disk
    // wrappers exist:
    //
    //     $C1/APF    Apple Preferred Format - chunked, PackBytes compressed,
    //                optional per-line MULTIPAL palettes, 320 or 640 modes.
    //     $2000      Raw 32000-byte bitmap + SCB table + palettes (optionally
    //                PackBytes compressed to 0x8000 bytes).
    //     3200       3200-color: one full 16-color palette per scanline. Either
    //                uncompressed (38400 bytes, "Brooks") or the "$3201" packed
    //                variant ($C1 D0 D0 00 header).
    //
    // The .shr extension is additionally used by the (unrelated) TRS-80
    // MagicDraw program for a 640x240 monochrome PGC-RLE bitmap; it is handled
    // as a magic-less fallback.
    //
    // ------------------------------------------------------------------------

    // Apple PackBytes run-length decoder.
    struct PackBytesStream
    {
        const u8* content;
        int contentOffset;
        int contentLength;
        int count = 1;
        int pattern = 0;

        int readByte()
        {
            if (contentOffset >= contentLength)
                return -1;
            return content[contentOffset++];
        }

        int readUnpacked()
        {
            if (--count == 0)
            {
                if (contentOffset >= contentLength)
                    return -1;
                int b = content[contentOffset++];
                count = (b & 0x3f) + 1;
                if (b >= 0x80)
                    count <<= 2;
                static const int patterns [4] = { 0, 1, 4, 1 };
                pattern = patterns[b >> 6];
            }
            else if ((count & (pattern - 1)) == 0)
            {
                contentOffset -= pattern;
            }
            return readByte();
        }
    };

    // PGC byte-oriented RLE used by the TRS-80 MagicDraw .SHR variant.
    struct PgcStream
    {
        const u8* content;
        int contentOffset;
        int contentLength;
        int repeatCount = 0;
        int repeatValue = 0;

        int readByte()
        {
            if (contentOffset >= contentLength)
                return -1;
            return content[contentOffset++];
        }

        bool readCommand()
        {
            int b = readByte();
            if (b < 0)
                return false;
            if (b < 128)
            {
                repeatCount = b;
                repeatValue = -1; // literal run
            }
            else
            {
                repeatCount = b - 128;
                repeatValue = readByte();
            }
            return true;
        }

        int readRle()
        {
            while (repeatCount == 0)
            {
                if (!readCommand())
                    return -1;
            }
            repeatCount--;
            if (repeatValue >= 0)
                return repeatValue;
            return readByte();
        }
    };

    struct InterfaceSHR : ImageDecodeInterface
    {
        ConstMemory m_memory;
        std::vector<u32> m_pixels; // 0x00RRGGBB
        int m_width = 0;
        int m_height = 0;
        bool m_doubled = false;    // 640 mode: source line drawn on two rows
        u32 m_palette [16];

        InterfaceSHR(ConstMemory memory)
            : m_memory(memory)
        {
            const u8* content = memory.address;
            const int length = int(memory.size);

            // Try the wrappers in an order that is safe for both .shr and
            // .3200 (strong magics first, size-only formats last). The .shr
            // extension is also used by the TRS-80 MagicDraw program, whose
            // mono PGC-RLE bitmap has no magic - it goes last as a fallback.
            bool ok = decodeApf(content, length)
                   || decode3201(content, length)
                   || decodeSh3(content, length)
                   || decodeAppleIIShr(content, length)
                   || decodeTrsShr(content, length);

            if (!ok)
            {
                header.setError("[ImageDecoder.SHR] Unsupported or invalid Super Hi-Res image.");
                return;
            }

            header.width  = m_width;
            header.height = m_height;
            header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }

        ~InterfaceSHR()
        {
        }

        static bool isString(const u8* content, int offset, const char* s)
        {
            for ( ; *s; ++s, ++offset)
                if (content[offset] != u8(*s))
                    return false;
            return true;
        }

        static int get32le(const u8* content, int offset)
        {
            return content[offset] | (content[offset + 1] << 8) |
                   (content[offset + 2] << 16) | (content[offset + 3] << 24);
        }

        bool setSize(int width, int height, bool doubled)
        {
            if (width <= 0 || height <= 0 || width > 1024 || height > 1024)
                return false;
            m_width = width;
            m_height = height;
            m_doubled = doubled;
            m_pixels.assign(size_t(width) * height, 0);
            return true;
        }

        // One IIGS palette: 16 entries of $0RGB (low nibble of each byte).
        void setPalette(const u8* content, int contentOffset, int reverse)
        {
            for (int c = 0; c < 16; ++c)
            {
                int offset = contentOffset + ((c ^ reverse) << 1);
                int gb = content[offset];
                int r = content[offset + 1] & 0xf;
                int g = gb >> 4;
                int b = gb & 0xf;
                int rgb = (r << 16) | (g << 8) | b;
                rgb |= rgb << 4; // 4-bit -> 8-bit per channel
                m_palette[c] = u32(rgb);
            }
        }

        // 320-mode scanline: 160 bytes, 2 pixels per byte (high nibble first).
        void decodeShrLine(const u8* content, int y)
        {
            const u8* row = content + y * 160;
            u32* dst = &m_pixels[size_t(y) * 320];
            for (int x = 0; x < 320; ++x)
            {
                int b = row[x >> 1];
                int nibble = (x & 1) == 0 ? (b >> 4) : (b & 0xf);
                dst[x] = m_palette[nibble];
            }
        }

        bool decodePackBytes(PackBytesStream& stream, int pixelsOffset, int unpackedBytes)
        {
            const int size = int(m_pixels.size());
            for (int x = 0; x < unpackedBytes; ++x)
            {
                int b = stream.readUnpacked();
                if (b < 0)
                    return false;

                if (m_doubled)
                {
                    int offset = (pixelsOffset << 1) + (x << 2);
                    if (offset < 0 || offset + m_width + 3 >= size)
                        return false;
                    m_pixels[offset + m_width + 0] = m_pixels[offset + 0] = m_palette[8 + (b >> 6)];
                    m_pixels[offset + m_width + 1] = m_pixels[offset + 1] = m_palette[12 + ((b >> 4) & 3)];
                    m_pixels[offset + m_width + 2] = m_pixels[offset + 2] = m_palette[(b >> 2) & 3];
                    m_pixels[offset + m_width + 3] = m_pixels[offset + 3] = m_palette[4 + (b & 3)];
                }
                else
                {
                    int offset = pixelsOffset + (x << 1);
                    if (offset < 0 || offset + 1 >= size)
                        return false;
                    m_pixels[offset + 0] = m_palette[b >> 4];
                    m_pixels[offset + 1] = m_palette[b & 0xf];
                }
            }
            return true;
        }

        // $2000: raw bitmap + SCB table (0x7d00) + 16 palettes (0x7e00).
        bool decodeAppleIIShrUnpacked(const u8* content)
        {
            if (!setSize(320, 200, false))
                return false;
            for (int y = 0; y < 200; ++y)
            {
                setPalette(content, 0x7e00 + ((content[0x7d00 + y] & 0xf) << 5), 0);
                decodeShrLine(content, y);
            }
            return true;
        }

        bool decodeAppleIIShr(const u8* content, int length)
        {
            if (length == 32768)
                return decodeAppleIIShrUnpacked(content);

            // PackBytes compressed to exactly 0x8000 bytes.
            std::vector<u8> unpacked(32768);
            PackBytesStream stream { content, 0, length };
            for (int o = 0; o < 32768; ++o)
            {
                int b = stream.readUnpacked();
                if (b < 0)
                    return false;
                unpacked[o] = u8(b);
            }
            if (stream.readUnpacked() >= 0)
                return false; // trailing data: not this format
            return decodeAppleIIShrUnpacked(unpacked.data());
        }

        // 3200-color: one 16-color palette per scanline (38400 bytes).
        bool decodeSh3(const u8* content, int length)
        {
            if (length != 38400)
                return false;
            if (!setSize(320, 200, false))
                return false;
            for (int y = 0; y < 200; ++y)
            {
                setPalette(content, 0x7d00 + (y << 5), 0xf);
                decodeShrLine(content, y);
            }
            return true;
        }

        // $3201: packed 3200-color (per-line palette + PackBytes bitmap).
        bool decode3201(const u8* content, int length)
        {
            if (length < 6404 + 160 * 200 / 128 ||
                content[0] != 0xc1 || content[1] != 0xd0 || content[2] != 0xd0 || content[3] != 0)
                return false;
            if (!setSize(320, 200, false))
                return false;
            PackBytesStream stream { content, 0x1904, length };
            for (int y = 0; y < 200; ++y)
            {
                setPalette(content, 4 + (y << 5), 0xf);
                if (!decodePackBytes(stream, y * 320, 160))
                    return false;
            }
            return true;
        }

        // TRS-80 MagicDraw: 640x240 mono PGC-RLE bitmap, displayed 640x480.
        // There is no header/magic, so this is only reached as a last resort.
        bool decodeTrsShr(const u8* content, int length)
        {
            if (!setSize(640, 480, false))
                return false;
            PgcStream rle { content, 0, length };
            for (int y = 0; y < 240; ++y)
            {
                int b = 0;
                u32* row0 = &m_pixels[size_t(2 * y) * 640];
                u32* row1 = &m_pixels[size_t(2 * y + 1) * 640];
                for (int x = 0; x < 640; ++x)
                {
                    if ((x & 7) == 0)
                    {
                        b = rle.readRle();
                        if (b < 0)
                            return false;
                    }
                    u32 c = ((b >> (~x & 7)) & 1) ? 0xffffffu : 0x000000u;
                    row0[x] = c;
                    row1[x] = c;
                }
            }
            return true;
        }

        // Apple Preferred Format (chunked, PackBytes, optional MULTIPAL).
        bool decodeApf(const u8* content, int length)
        {
            if (length < 1249 || content[4] != 4 || !isString(content, 5, "MAIN") || content[14] != 0)
                return false;

            int paletteCount = content[13];
            if (paletteCount > 16)
                return false;

            int dirOffset = 0x11 + (paletteCount << 5);
            if (dirOffset >= length)
                return false;

            int mode = content[9] & 0xf0;
            int width = content[11] | (content[12] << 8);
            int height = content[dirOffset - 2] | (content[dirOffset - 1] << 8);

            int bytesPerLine;
            switch (mode)
            {
                case 0x00:
                    if ((width & 1) || !setSize(width, height, false))
                        return false;
                    bytesPerLine = width >> 1;
                    break;
                case 0x80:
                    if ((width & 3) || !setSize(width, height << 1, true))
                        return false;
                    bytesPerLine = width >> 2;
                    break;
                default:
                    return false;
            }

            // Locate an optional per-line MULTIPAL palette chunk (200-line art).
            int multipalOffset = -1;
            int contentOffset = 0;
            if (height == 200)
            {
                for (int chunkLength = get32le(content, 0); ; )
                {
                    if (chunkLength <= 0)
                        return false;
                    contentOffset += chunkLength;
                    if (contentOffset < 0 || contentOffset > length - 6415)
                        break;
                    chunkLength = get32le(content, contentOffset);
                    if (chunkLength == 6415 && content[contentOffset + 4] == 8 &&
                        isString(content, contentOffset + 5, "MULTIPAL") &&
                        content[contentOffset + 13] == 200 && content[contentOffset + 14] == 0)
                    {
                        multipalOffset = contentOffset + 15;
                        break;
                    }
                }
            }

            contentOffset = dirOffset + (height << 2);
            if (contentOffset >= length)
                return false;

            PackBytesStream stream { content, contentOffset, length };
            for (int y = 0; y < height; ++y)
            {
                if (multipalOffset >= 0)
                {
                    setPalette(content, multipalOffset + (y << 5), 0);
                }
                else
                {
                    int lineMode = content[dirOffset + (y << 2) + 2];
                    int palette = lineMode & 0xf;
                    if ((lineMode & 0xf0) != mode || palette >= paletteCount ||
                        content[dirOffset + (y << 2) + 3] != 0)
                        return false;
                    setPalette(content, 0xf + (palette << 5), 0);
                }

                int nextLineOffset = stream.contentOffset +
                    content[dirOffset + (y << 2)] + (content[dirOffset + (y << 2) + 1] << 8);
                if (!decodePackBytes(stream, y * width, bytesPerLine))
                    return false;
                stream.contentOffset = nextLineOffset;
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

            for (int y = 0; y < m_height; ++y)
            {
                const u32* src = &m_pixels[size_t(y) * m_width];
                u8* d = target.address<u8>(0, y);
                for (int x = 0; x < m_width; ++x)
                {
                    u32 rgb = src[x];
                    d[0] = u8(rgb >> 16);
                    d[1] = u8(rgb >> 8);
                    d[2] = u8(rgb);
                    d[3] = 0xff;
                    d += 4;
                }
            }

            target.resolve();

            status.direct = target.isDirect();
            return status;
        }
    };

    ImageDecodeInterface* createInterfaceSHR(ConstMemory memory)
    {
        return new InterfaceSHR(memory);
    }

    // ------------------------------------------------------------------------
    // ZX Spectrum screen (.SCR)
    //
    // The classic Sinclair ZX Spectrum 256x192 display: a 6144-byte bitmap with
    // a famously scrambled scanline order, followed by a 768-byte attribute
    // area (8x8 ink/paper/bright/flash cells - the source of "colour clash").
    // The .SCR extension covers several length-keyed variants; the genuine
    // Spectrum ones are handled here:
    //
    //     6144            bitmap only (monochrome)
    //     6912 / 6913     standard screen$ (bitmap + 8x8 attributes)
    //     6976            ULAplus (64-entry G3R3B2 palette at 0x1b00)
    //     12288           Timex 8x1 attributes
    //     12289           Timex hi-res 512x192 (mono, single ink color)
    //     16000           Electronika MC 0515 640x400 (mono, 1x2)
    //
    // A 32768-byte .SCR is an Apple IIGS Super Hi-Res image and is dispatched
    // to the SHR decoder (see createInterfaceSCR). Other .SCR sizes belong to
    // unrelated platforms (Atari GR0, Amstrad, ...) and are not handled here.
    //
    // ------------------------------------------------------------------------

    struct InterfaceSCR : ImageDecodeInterface
    {
        ConstMemory m_memory;
        std::vector<u32> m_pixels; // 0x00RRGGBB
        int m_width = 0;
        int m_height = 0;
        u32 m_palette [64];

        // ZX bitmap addressing modes (negative sentinels) and attribute modes.
        enum { BITMAP_LINEAR = -1 };
        enum { ATTR_NONE = -3, ATTR_TIMEX = -1, ATTR_8X1 = 0, ATTR_8X8 = 3 };

        InterfaceSCR(ConstMemory memory)
            : m_memory(memory)
        {
            const u8* content = memory.address;
            const int length = int(memory.size);

            bool ok = true;
            switch (length)
            {
                case 6144:
                    setZx(256, 192);
                    decodeZx(content, 0, -1, ATTR_NONE);
                    break;
                case 6912:
                case 6913: // trailing border color byte - ignored
                    setZx(256, 192);
                    decodeZx(content, 0, 0x1800, ATTR_8X8);
                    break;
                case 6976:
                    setUlaPlus(content, 0x1b00);
                    decodeZx(content, 0, 0x1800, ATTR_8X8);
                    break;
                case 12288:
                    setZx(256, 192);
                    decodeZx(content, 0, 0x1800, ATTR_TIMEX);
                    break;
                case 12289:
                    setSize(512, 384);
                    decodeTimexHires(content, 0);
                    break;
                case 16000:
                    // Electronika MC 0515: 640x200 monochrome, displayed 1x2.
                    decodeElectronika(content);
                    break;
                default:
                    ok = false;
                    break;
            }

            if (!ok)
            {
                header.setError("[ImageDecoder.ZXSpectrum] Unsupported or invalid .SCR image.");
                return;
            }

            header.width  = m_width;
            header.height = m_height;
            header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }

        ~InterfaceSCR()
        {
        }

        void setSize(int width, int height)
        {
            m_width = width;
            m_height = height;
            m_pixels.assign(size_t(width) * height, 0);
        }

        static u32 zxPaletteColor(int c)
        {
            return u32(((c >> 1) & 1) * 0xff0000 | ((c >> 2) & 1) * 0x00ff00 | (c & 1) * 0x0000ff);
        }

        // 16 Spectrum colors (8 normal + 8 bright) laid out as a 64-entry
        // ULAplus-style palette so the attribute decoder is shared.
        void setZx(int width, int height)
        {
            setSize(width, height);
            for (int i = 0; i < 64; ++i)
            {
                u32 rgb = zxPaletteColor(i);
                if ((i & 0x10) == 0) // not bright
                    rgb &= 0xcdcdcd;
                m_palette[i] = rgb;
            }
        }

        static u32 g3r3b2Color(int c)
        {
            // 0bGGGRRRBB -> 0xRRGGBB
            return u32((((c & 0x1c) * 0x49) >> 3) << 16
                     | (((c >> 5) * 0x49) >> 1) << 8
                     | ((c & 3) * 0x55));
        }

        void setUlaPlus(const u8* content, int paletteOffset)
        {
            setSize(256, 192);
            for (int i = 0; i < 64; ++i)
                m_palette[i] = g3r3b2Color(content[paletteOffset + i]);
        }

        // Electronika MC 0515 (Soviet PDP-11 clone): 640x200 monochrome
        // bitmap shown with a 1x2 vertical stretch (640x400).
        void decodeElectronika(const u8* content)
        {
            setSize(640, 400);
            for (int y = 0; y < 200; ++y)
            {
                for (int x = 0; x < 640; ++x)
                {
                    int bit = (content[y * 80 + (x >> 3)] >> (~x & 7)) & 1;
                    u32 rgb = bit ? 0xffffff : 0;
                    m_pixels[size_t(y * 2 + 0) * 640 + x] = rgb;
                    m_pixels[size_t(y * 2 + 1) * 640 + x] = rgb;
                }
            }
        }

        static int zxLineOffset(int y)
        {
            return ((y & 0xc0) << 5) + ((y & 7) << 8) + ((y & 0x38) << 2);
        }

        static int zxIndex(int c, int a)
        {
            return (a >> 2 & 0x30) | ((c & 1) == 0 ? (8 | (a >> 3 & 7)) : (a & 7));
        }

        void decodeZx(const u8* content, int bitmapOffset, int attributesOffset, int attributesMode)
        {
            for (int y = 0; y < 192; ++y)
            {
                for (int x = 0; x < 256; ++x)
                {
                    int col = x >> 3;
                    int c;
                    if (bitmapOffset == BITMAP_LINEAR)
                        c = content[(y << 5) | col] >> (~x & 7);
                    else
                        c = content[bitmapOffset + zxLineOffset(y) + col] >> (~x & 7);

                    u32 rgb;
                    if (attributesMode == ATTR_NONE)
                    {
                        rgb = (c & 1) ? 0xffffff : 0;
                    }
                    else
                    {
                        int a;
                        if (attributesMode == ATTR_TIMEX)
                            a = attributesOffset + zxLineOffset(y);
                        else
                            a = attributesOffset + (y >> attributesMode << 5);
                        a = content[a + col];
                        rgb = m_palette[zxIndex(c, a)];
                    }
                    m_pixels[(y << 8) + x] = rgb;
                }
            }
        }

        // Timex hi-res: 512x192 monochrome, displayed 512x384 (1x2), with a
        // single ink color taken from the attribute byte.
        void decodeTimexHires(const u8* content, int contentOffset)
        {
            u32 inkColor = zxPaletteColor(content[contentOffset + 0x3000] >> 3);
            for (int y = 0; y < 192; ++y)
            {
                for (int x = 0; x < 512; ++x)
                {
                    int c = content[contentOffset + (x & 8) * 768 + zxLineOffset(y) + (x >> 4)] >> (~x & 7) & 1;
                    int offset = (y << 10) + x;
                    m_pixels[offset + 512] = m_pixels[offset] = (c == 0) ? (inkColor ^ 0xffffff) : inkColor;
                }
            }
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
                const u32* src = &m_pixels[size_t(y) * m_width];
                u8* d = target.address<u8>(0, y);
                for (int x = 0; x < m_width; ++x)
                {
                    u32 rgb = src[x];
                    d[0] = u8(rgb >> 16);
                    d[1] = u8(rgb >> 8);
                    d[2] = u8(rgb);
                    d[3] = 0xff;
                    d += 4;
                }
            }

            target.resolve();

            status.direct = target.isDirect();
            return status;
        }
    };

    ImageDecodeInterface* createInterfaceSCR(ConstMemory memory)
    {
        // The .scr extension is shared across platforms. A 32768-byte file is
        // an Apple IIGS $2000 raw Super Hi-Res image, handled by the SHR path.
        if (memory.size == 32768)
            return createInterfaceSHR(memory);

        return new InterfaceSCR(memory);
    }

} // namespace

namespace mango::image
{

    void registerImageCodecRETRO()
    {
        // MAKIchan Graphics
        registerImageDecoder(createInterfaceMAG, ".mag");

        // Apple IIGS Super Hi-Res
        registerImageDecoder(createInterfaceSHR, ".shr");
        registerImageDecoder(createInterfaceSHR, ".3200");

        // ZX Spectrum screen
        registerImageDecoder(createInterfaceSCR, ".scr");
    }

} // namespace mango::image
