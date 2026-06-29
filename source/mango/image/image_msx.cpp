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
            int yv = row[x] >> 3;
            if (usePalette && (yv & 1))
                return palette[yv >> 1];

            int r, g, b;
            if ((x | 3) >= width)
            {
                r = g = b = yv;
            }
            else
            {
                const u8* p = row + (x & ~3);
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
    }

} // namespace mango::image
