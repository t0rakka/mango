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

        .mag    MAKIchan Graphics

    Reference: RECOIL (Retro Computer Image Library), recoil.c.

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

    Reference: RECOIL (Retro Computer Image Library), recoil.c.
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

    // MSB-first bit reader matching RECOIL's BitStream (sentinel bit in bit 7).
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

    // Apply the target platform's color-channel precision (RECOIL
    // RestrictPlatformColor). rgb is 0x00RRGGBB.
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

        // Decode one MSX2+ YJK pixel (RECOIL DecodeMsxYjk). Groups of 4 pixels
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

} // namespace

namespace mango::image
{

    void registerImageCodecRETRO()
    {
        // MAKIchan Graphics
        registerImageDecoder(createInterfaceMAG, ".mag");
    }

} // namespace mango::image
