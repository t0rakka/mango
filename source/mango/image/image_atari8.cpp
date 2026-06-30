/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    Atari 8-bit (400/800/XL/XE) image decoders.

    These are the classic raw GTIA/ANTIC screen dumps used by the Atari 8-bit
    demo and art scene. The byte stream is not self describing, so the screen
    mode (and therefore the pixel layout) is selected from the file extension:

        .gr8    GRAPHICS 8   320x192  1 bpp, 2 luminances of one hue
        .gr9    GRAPHICS 9    80x192  GTIA 16 luminances of one hue (4x wide)
        .mic    "MicroPainter" / GRAPHICS 15 (ANTIC E) 160x192, 4 colors
        .hip    "Hard Interlace Picture" - two interleaved GR9/GR10 fields
        .pi9    GRAPHICS 9 picture with trailing data
        .tip    "Taquart Interlace Picture" - GR9/GR10/GR11 interlace blend
        .rip    "Rocky Interlace Picture" - LZ77 packed, GR8/9/10/11/15 (+blend)
        .ice    "Interlace Character Editor" - two character-set fields, blended

    Colors come from the Altirra-generated 256-entry GTIA palette (PAL by
    default; the NTSC table is kept for completeness). Each screen mode builds
    a frame of Atari color bytes (hue<<4 | luma) which is then mapped through
    that palette.

    Companion files (palette / raster sidecars) and the rarer container
    variants are intentionally ignored - this is a single file decoder.

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
    // Altirra GTIA palettes (256 colors, R8G8B8), indexed by color byte
    // ------------------------------------------------------------

    static const u8 ATARI8_PAL [768] =
    {
        0, 0, 0, 17, 17, 17, 34, 34, 34, 51, 51, 51, 68, 68, 68, 85, 85, 85,
        102, 102, 102, 119, 119, 119, 136, 136, 136, 153, 153, 153, 170, 170, 170, 187, 187, 187,
        204, 204, 204, 221, 221, 221, 238, 238, 238, 255, 255, 255, 63, 0, 0, 80, 5, 0,
        97, 22, 0, 114, 39, 0, 131, 56, 0, 148, 73, 0, 165, 90, 1, 182, 107, 18,
        199, 124, 35, 216, 141, 52, 233, 158, 69, 250, 175, 86, 255, 192, 103, 255, 209, 120,
        255, 226, 137, 255, 243, 154, 80, 0, 0, 97, 0, 0, 114, 3, 0, 131, 20, 3,
        148, 37, 20, 165, 54, 37, 182, 71, 54, 199, 88, 71, 216, 105, 88, 233, 122, 105,
        250, 139, 122, 255, 156, 139, 255, 173, 156, 255, 190, 173, 255, 207, 190, 255, 224, 207,
        84, 0, 3, 101, 0, 20, 118, 0, 37, 135, 8, 54, 152, 25, 71, 169, 42, 88,
        186, 59, 105, 203, 76, 122, 220, 93, 139, 237, 110, 156, 254, 127, 173, 255, 144, 190,
        255, 161, 207, 255, 178, 224, 255, 195, 241, 255, 212, 255, 79, 0, 53, 96, 0, 70,
        113, 0, 87, 130, 1, 104, 147, 18, 121, 164, 35, 138, 181, 52, 155, 198, 69, 172,
        215, 86, 189, 232, 103, 206, 249, 120, 223, 255, 137, 240, 255, 154, 255, 255, 171, 255,
        255, 188, 255, 255, 205, 255, 61, 0, 104, 78, 0, 121, 95, 0, 138, 112, 0, 155,
        129, 17, 172, 146, 34, 189, 163, 51, 206, 180, 68, 223, 197, 85, 240, 214, 102, 255,
        231, 119, 255, 248, 136, 255, 255, 153, 255, 255, 170, 255, 255, 187, 255, 255, 204, 255,
        32, 0, 139, 49, 0, 156, 66, 0, 173, 83, 8, 190, 100, 25, 207, 117, 42, 224,
        134, 59, 241, 151, 76, 255, 168, 93, 255, 185, 110, 255, 202, 127, 255, 219, 144, 255,
        236, 161, 255, 253, 178, 255, 255, 195, 255, 255, 212, 255, 0, 0, 137, 0, 8, 154,
        0, 25, 171, 16, 42, 188, 33, 59, 205, 50, 76, 222, 67, 93, 239, 84, 110, 255,
        101, 127, 255, 118, 144, 255, 135, 161, 255, 152, 178, 255, 169, 195, 255, 186, 212, 255,
        203, 229, 255, 220, 246, 255, 0, 12, 101, 0, 29, 118, 0, 46, 135, 0, 63, 152,
        5, 80, 169, 22, 97, 186, 39, 114, 203, 56, 131, 220, 73, 148, 237, 90, 165, 254,
        107, 182, 255, 124, 199, 255, 141, 216, 255, 158, 233, 255, 175, 250, 255, 192, 255, 255,
        0, 31, 48, 0, 48, 65, 0, 65, 82, 0, 82, 99, 0, 99, 116, 5, 116, 133,
        22, 133, 150, 39, 150, 167, 56, 167, 184, 73, 184, 201, 90, 201, 218, 107, 218, 235,
        124, 235, 252, 141, 252, 255, 158, 255, 255, 175, 255, 255, 0, 43, 0, 0, 60, 14,
        0, 77, 31, 0, 94, 48, 0, 111, 65, 1, 128, 82, 18, 145, 99, 35, 162, 116,
        52, 179, 133, 69, 196, 150, 86, 213, 167, 103, 230, 184, 120, 247, 201, 137, 255, 218,
        154, 255, 235, 171, 255, 252, 0, 51, 0, 0, 68, 0, 0, 85, 0, 0, 102, 0,
        7, 119, 0, 24, 136, 0, 41, 153, 0, 58, 170, 15, 75, 187, 32, 92, 204, 49,
        109, 221, 66, 126, 238, 83, 143, 255, 100, 160, 255, 117, 177, 255, 134, 194, 255, 151,
        0, 43, 0, 0, 60, 0, 2, 77, 0, 19, 94, 0, 36, 111, 0, 53, 128, 0,
        70, 145, 0, 87, 162, 0, 104, 179, 0, 121, 196, 14, 138, 213, 31, 155, 230, 48,
        172, 247, 65, 189, 255, 82, 206, 255, 99, 223, 255, 116, 1, 28, 0, 18, 45, 0,
        35, 62, 0, 52, 79, 0, 69, 96, 0, 86, 113, 0, 103, 130, 0, 120, 147, 0,
        137, 164, 0, 154, 181, 3, 171, 198, 20, 188, 215, 37, 205, 232, 54, 222, 249, 71,
        239, 255, 88, 255, 255, 105, 35, 9, 0, 52, 26, 0, 69, 43, 0, 86, 60, 0,
        103, 77, 0, 120, 94, 0, 137, 111, 0, 154, 128, 0, 171, 145, 0, 188, 162, 16,
        205, 179, 33, 222, 196, 50, 239, 213, 67, 255, 230, 84, 255, 247, 101, 255, 255, 118,
        63, 0, 0, 80, 5, 0, 97, 22, 0, 114, 39, 0, 131, 56, 0, 148, 73, 0,
        165, 90, 1, 182, 107, 18, 199, 124, 35, 216, 141, 52, 233, 158, 69, 250, 175, 86,
        255, 192, 103, 255, 209, 120, 255, 226, 137, 255, 243, 154
    };

    static const u8 ATARI8_NTSC [768] =
    {
        0, 0, 0, 1, 1, 1, 22, 21, 23, 42, 41, 43, 62, 60, 63, 81, 78, 83,
        103, 100, 106, 122, 118, 124, 131, 126, 133, 149, 144, 152, 170, 164, 173, 188, 181, 192,
        205, 198, 210, 223, 215, 227, 243, 235, 248, 255, 252, 255, 0, 2, 0, 0, 20, 0,
        7, 42, 0, 33, 61, 0, 54, 79, 0, 75, 96, 0, 98, 117, 0, 118, 134, 0,
        127, 142, 0, 146, 159, 0, 167, 179, 39, 186, 196, 63, 203, 213, 85, 221, 230, 106,
        242, 249, 129, 255, 255, 148, 12, 0, 0, 35, 2, 0, 61, 24, 0, 81, 43, 0,
        101, 61, 0, 120, 79, 0, 143, 100, 0, 161, 117, 0, 170, 126, 0, 188, 143, 28,
        210, 163, 58, 228, 180, 80, 245, 197, 100, 255, 214, 120, 255, 234, 143, 255, 251, 162,
        40, 0, 0, 63, 0, 0, 90, 0, 0, 111, 21, 0, 131, 41, 0, 150, 60, 0,
        173, 82, 22, 192, 100, 47, 200, 109, 57, 219, 126, 78, 240, 147, 101, 255, 164, 121,
        255, 182, 140, 255, 199, 158, 255, 219, 180, 255, 236, 198, 54, 0, 0, 77, 0, 0,
        103, 0, 0, 124, 0, 19, 144, 24, 41, 164, 46, 62, 187, 69, 86, 205, 87, 105,
        214, 96, 114, 233, 114, 133, 254, 135, 155, 255, 153, 173, 255, 170, 191, 255, 188, 209,
        255, 208, 231, 255, 225, 248, 51, 0, 16, 73, 0, 37, 99, 2, 61, 120, 8, 81,
        140, 26, 101, 160, 44, 120, 183, 66, 142, 201, 85, 160, 210, 93, 169, 229, 111, 187,
        250, 132, 209, 255, 150, 227, 255, 167, 244, 255, 184, 255, 255, 204, 255, 255, 221, 255,
        26, 10, 66, 50, 15, 86, 76, 21, 109, 98, 27, 128, 119, 39, 147, 139, 54, 166,
        161, 74, 187, 180, 91, 205, 189, 100, 214, 207, 117, 232, 229, 137, 253, 246, 154, 255,
        255, 171, 255, 255, 188, 255, 255, 208, 255, 255, 225, 255, 0, 19, 95, 0, 25, 114,
        34, 31, 137, 59, 39, 156, 81, 52, 174, 102, 67, 193, 125, 86, 214, 144, 103, 232,
        153, 111, 241, 172, 128, 255, 193, 148, 255, 211, 165, 255, 229, 182, 255, 246, 198, 255,
        255, 218, 255, 255, 235, 255, 0, 20, 99, 0, 26, 118, 0, 35, 141, 0, 47, 160,
        18, 62, 178, 49, 78, 196, 76, 98, 218, 97, 115, 236, 106, 124, 244, 125, 141, 255,
        148, 161, 255, 166, 178, 255, 185, 194, 255, 203, 211, 255, 224, 231, 255, 241, 248, 255,
        0, 14, 77, 0, 21, 97, 0, 37, 120, 0, 54, 139, 0, 72, 157, 0, 90, 176,
        0, 110, 197, 23, 128, 216, 42, 136, 224, 69, 153, 242, 96, 174, 255, 117, 191, 255,
        137, 208, 255, 156, 224, 255, 179, 244, 255, 197, 255, 255, 0, 4, 33, 0, 21, 54,
        0, 44, 77, 0, 63, 97, 0, 82, 116, 0, 100, 135, 0, 121, 157, 0, 139, 175,
        0, 147, 184, 0, 165, 202, 34, 185, 223, 69, 202, 241, 94, 219, 255, 117, 236, 255,
        142, 255, 255, 162, 255, 255, 0, 11, 0, 0, 31, 0, 0, 53, 18, 0, 72, 40,
        0, 91, 60, 0, 109, 80, 0, 130, 103, 0, 148, 122, 0, 156, 131, 0, 173, 150,
        0, 194, 171, 40, 211, 190, 74, 227, 208, 100, 244, 225, 127, 255, 246, 148, 255, 255,
        0, 18, 0, 0, 37, 0, 0, 59, 0, 0, 77, 0, 0, 95, 0, 0, 113, 14,
        0, 134, 43, 0, 151, 65, 0, 159, 74, 0, 177, 94, 38, 197, 117, 72, 214, 136,
        97, 230, 154, 119, 247, 173, 144, 255, 194, 164, 255, 212, 0, 15, 0, 0, 35, 0,
        0, 57, 0, 0, 75, 0, 0, 93, 0, 0, 110, 0, 0, 131, 0, 33, 148, 0,
        49, 156, 13, 75, 173, 43, 101, 193, 70, 122, 210, 90, 142, 227, 110, 161, 243, 130,
        183, 255, 152, 201, 255, 171, 0, 5, 0, 0, 24, 0, 0, 47, 0, 4, 65, 0,
        35, 83, 0, 58, 100, 0, 83, 121, 0, 103, 138, 0, 112, 146, 0, 132, 163, 0,
        154, 183, 41, 172, 200, 65, 190, 217, 86, 208, 233, 107, 229, 253, 130, 247, 255, 149,
        3, 0, 0, 25, 7, 0, 50, 30, 0, 70, 48, 0, 90, 67, 0, 109, 84, 0,
        132, 105, 0, 150, 122, 0, 159, 131, 0, 177, 148, 13, 199, 168, 49, 217, 185, 72,
        234, 202, 93, 252, 219, 113, 255, 239, 136, 255, 255, 155
    };

    // ------------------------------------------------------------
    // GTIA color registers
    // ------------------------------------------------------------

    // ANTIC/GTIA hardware color registers. We store the Atari color byte
    // (hue<<4 | luma, low bit ignored) in each; rendering combines these into
    // per-pixel color bytes that index the 256-entry GTIA palette.
    struct Gtia
    {
        u8 reg [16] = { 0 };

        void setColor(int index, int value)
        {
            value &= 254;
            switch (index)
            {
                case 0: case 1: case 2: case 3:
                    reg[index] = u8(value);
                    break;
                case 4: case 5: case 6: case 7:
                    reg[8 + index] = reg[index] = u8(value);
                    break;
                case 8:
                    reg[8] = reg[9] = reg[10] = reg[11] = u8(value);
                    break;
            }
        }

        // 9 consecutive bytes: COLBK followed by the 8 player/playfield registers.
        void setColors(const u8* p)
        {
            reg[0] = u8(p[0] & 254);
            for (int i = 0; i < 8; ++i)
                setColor(1 + i, p[1 + i]);
        }

        // GRAPHICS 15 default playfield (BAK, PF0, PF1, PF2).
        void setGr15Default()
        {
            reg[8] = 0;
            reg[4] = 4;
            reg[5] = 8;
            reg[6] = 12;
        }

        // BAK, PF0, PF1, PF2 packed as 4 bytes
        void setBakPF012(const u8* p)
        {
            reg[8] = u8(p[0] & 254);
            reg[4] = u8(p[1] & 254);
            reg[5] = u8(p[2] & 254);
            reg[6] = u8(p[3] & 254);
        }

        // PF0, PF1, PF2, BAK packed as 4 bytes
        void setPF012Bak(const u8* p)
        {
            reg[4] = u8(p[0] & 254);
            reg[5] = u8(p[1] & 254);
            reg[6] = u8(p[2] & 254);
            reg[8] = u8(p[3] & 254);
        }
    };

    // Atari executable (.xex) block header: 0xFFFF then little-endian start/end.
    // Returns the block byte length, or -1 when no such header is present.
    static int parseExecutableHeader(const u8* data, size_t size, size_t offset)
    {
        if (offset + 6 > size || data[offset] != 0xff || data[offset + 1] != 0xff)
            return -1;
        int start = data[offset + 2] | (data[offset + 3] << 8);
        int end = data[offset + 4] | (data[offset + 5] << 8);
        return end - start + 1;
    }

    // Some raw dumps are wrapped in a single executable block; skip its header.
    static size_t executableOffset(const u8* data, size_t size)
    {
        if (size >= 7)
        {
            int blockLength = parseExecutableHeader(data, size, 0);
            if (blockLength > 0 && size_t(6 + blockLength) == size)
                return 6;
        }
        return 0;
    }

    // ------------------------------------------------------------
    // Mode renderers - produce a frame of Atari color bytes
    // ------------------------------------------------------------

    static const int WIDTH = 320;

    // GRAPHICS 8: 1 bpp, 40 bytes/row. Two luminances of the COLBK hue.
    static void renderGr8(const Gtia& g, const u8* src, u8* frame, int height)
    {
        u8 colors [2];
        colors[0] = g.reg[6];
        colors[1] = u8((g.reg[6] & 0xf0) | (g.reg[5] & 0x0e));

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                int c = (src[x >> 3] >> (~x & 7)) & 1;
                frame[x] = colors[c];
            }
            src += WIDTH >> 3;
            frame += WIDTH;
        }
    }

    // GRAPHICS 9: GTIA 16 luminances of the COLBK hue. Each 4-bit value spans
    // 4 horizontal pixels (40 bytes/row). leftSkip shifts the read window.
    static void renderGr9(const Gtia& g, const u8* src, int srcStride, u8* frame, int frameStride, int height, int leftSkip)
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                int i = x + leftSkip;
                int c = (i < 0 || i >= WIDTH) ? 0 : (src[i >> 3] >> (~i & 4)) & 15;
                frame[x] = u8(g.reg[8] | c);
            }
            src += srcStride;
            frame += frameStride;
        }
    }

    // GRAPHICS 15 (ANTIC E): 2 bpp, 40 bytes/row, 4 playfield colors.
    static void renderGr15(const Gtia& g, const u8* src, int srcStride, u8* frame, int frameStride, int height)
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                int c = (src[x >> 3] >> (~x & 6)) & 3;
                frame[x] = g.reg[c == 0 ? 8 : c + 3];
            }
            src += srcStride;
            frame += frameStride;
        }
    }

    // GRAPHICS 10: 4-bit indices into the 9 GTIA registers. Each value spans 4
    // pixels (40 bytes/row). The output is shifted by (2 - leftSkip) pixels to
    // align with the companion GR9 field in interlaced (HIP) pictures.
    static void renderGr10(const Gtia& g, const u8* src, u8* frame, int frameStride, int height, int leftSkip)
    {
        for (int y = 0; y < height; ++y)
        {
            u8* row = frame + (2 - leftSkip);
            int x = leftSkip - 2;
            for (; x < 0; ++x)
                row[x] = g.reg[0];
            for (; x < WIDTH + leftSkip - 2; ++x)
            {
                int c = (src[x >> 3] >> (~x & 4)) & 15;
                row[x] = g.reg[c];
            }
            src += WIDTH >> 3;
            frame += frameStride;
        }
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    enum class Mode
    {
        GR8, GR9, MIC, HIP, PI9
    };

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        Mode m_mode;
        const u8* m_palette = ATARI8_PAL;

        // Resolved per file in the constructor.
        size_t m_offset = 0;        // start of pixel data
        int m_mic_color_mode = 0;   // MIC: 0 = default, 4 = BakPF012, 5 = PF012Bak

        Interface(ConstMemory memory, Mode mode)
            : m_memory(memory)
            , m_mode(mode)
        {
            const u8* data = memory.address;
            const size_t size = memory.size;

            int width = WIDTH;
            int height = 0;

            switch (mode)
            {
                case Mode::GR8:
                case Mode::GR9:
                {
                    m_offset = executableOffset(data, size);
                    size_t pixels = size - m_offset;
                    height = int(pixels / 40);
                    // 7682 carries two trailing luma bytes; otherwise expect whole rows.
                    if (height < 1 || height > 240 || (pixels % 40 != 0 && pixels % 40 != 2))
                    {
                        height = 0;
                    }
                    break;
                }

                case Mode::MIC:
                {
                    m_offset = 0;
                    bool ok = true;
                    switch (size % 40)
                    {
                        case 0:
                        case 3:
                            m_mic_color_mode = 0;
                            break;
                        case 4:
                            m_mic_color_mode = 4;
                            break;
                        case 5:
                            m_mic_color_mode = 5;
                            break;
                        default:
                            ok = false;
                            break;
                    }
                    height = ok ? int(size / 40) : 0;
                    if (height < 1 || height > 240)
                        height = 0;
                    break;
                }

                case Mode::PI9:
                {
                    // GR9 picture with trailing data we ignore (other PI9
                    // variants such as APC / Fuckpaint are not supported).
                    if (size == 7684 || size == 7808 || size == 7936)
                    {
                        m_offset = 0;
                        height = 192;
                    }
                    break;
                }

                case Mode::HIP:
                {
                    height = hipHeight(data, size);
                    break;
                }
            }

            if (height <= 0)
            {
                header.setError("[ImageDecoder.ATARI8] Unsupported or corrupt file.");
                return;
            }

            header.width  = width;
            header.height = height;
            header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }

        ~Interface()
        {
        }

        // Returns the HIP image height (rows) or 0 if the layout is not valid.
        static int hipHeight(const u8* data, size_t size)
        {
            if (size < 80)
                return 0;

            int frameLength = parseExecutableHeader(data, size, 0);
            if (frameLength > 0 && frameLength % 40 == 0 &&
                size_t(12 + frameLength * 2) == size &&
                parseExecutableHeader(data, size, 6 + frameLength) == frameLength)
            {
                int height = frameLength / 40;
                return height <= 240 ? height : 0;
            }

            int height = int(size / 80);
            return height <= 240 ? height : 0;
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);
            return ConstMemory();
        }

        Color color(u8 byte) const
        {
            const u8* p = m_palette + size_t(byte) * 3;
            return Color(p[0], p[1], p[2], 0xff);
        }

        void writeFrame(const Surface& dest, const u8* frame)
        {
            for (int y = 0; y < header.height; ++y)
            {
                const u8* src = frame + size_t(y) * header.width;
                u8* d = dest.address<u8>(0, y);
                for (int x = 0; x < header.width; ++x)
                {
                    Color c = color(src[x]);
                    d[0] = c.r;
                    d[1] = c.g;
                    d[2] = c.b;
                    d[3] = 0xff;
                    d += 4;
                }
            }
        }

        // Average two GTIA fields
        void writeBlend(const Surface& dest, const u8* frame1, const u8* frame2)
        {
            for (int y = 0; y < header.height; ++y)
            {
                const u8* s1 = frame1 + size_t(y) * header.width;
                const u8* s2 = frame2 + size_t(y) * header.width;
                u8* d = dest.address<u8>(0, y);
                for (int x = 0; x < header.width; ++x)
                {
                    Color a = color(s1[x]);
                    Color b = color(s2[x]);
                    d[0] = u8((a.r + b.r) >> 1);
                    d[1] = u8((a.g + b.g) >> 1);
                    d[2] = u8((a.b + b.b) >> 1);
                    d[3] = 0xff;
                    d += 4;
                }
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
            const u8* data = m_memory.address;
            const size_t size = m_memory.size;
            const int height = header.height;

            std::vector<u8> frame(size_t(header.width) * height, 0);
            Gtia gtia;

            switch (m_mode)
            {
                case Mode::GR8:
                {
                    if (size == 7682)
                    {
                        gtia.reg[6] = u8(data[7680] & 0x0e);
                        gtia.reg[5] = u8(data[7681] & 0x0e);
                    }
                    else
                    {
                        gtia.reg[6] = 0;
                        gtia.reg[5] = 14;
                    }
                    renderGr8(gtia, data + m_offset, frame.data(), height);
                    writeFrame(dest, frame.data());
                    break;
                }

                case Mode::GR9:
                case Mode::PI9:
                {
                    gtia.reg[8] = 0;
                    renderGr9(gtia, data + m_offset, 40, frame.data(), WIDTH, height, 0);
                    writeFrame(dest, frame.data());
                    break;
                }

                case Mode::MIC:
                {
                    switch (m_mic_color_mode)
                    {
                        case 4: gtia.setBakPF012(data + size - 4); break;
                        case 5: gtia.setPF012Bak(data + size - 5); break;
                        default: gtia.setGr15Default(); break;
                    }
                    renderGr15(gtia, data + m_offset, 40, frame.data(), WIDTH, height);
                    writeFrame(dest, frame.data());
                    break;
                }

                case Mode::HIP:
                {
                    static const u8 GR10_COLORS [9] = { 0, 0, 2, 4, 6, 8, 10, 12, 14 };

                    std::vector<u8> frame2(size_t(WIDTH) * height, 0);

                    int frameLength = parseExecutableHeader(data, size, 0);
                    bool executable = frameLength > 0 && frameLength % 40 == 0 &&
                        size_t(12 + frameLength * 2) == size &&
                        parseExecutableHeader(data, size, 6 + frameLength) == frameLength;

                    if (executable)
                    {
                        gtia.setColors(GR10_COLORS);
                        renderGr10(gtia, data + 6, frame.data(), WIDTH, height, 1);
                        gtia.reg[8] = 0;
                        renderGr9(gtia, data + 12 + frameLength, 40, frame2.data(), WIDTH, height, 1);
                    }
                    else
                    {
                        gtia.reg[8] = 0;
                        renderGr9(gtia, data, 40, frame.data(), WIDTH, height, 1);
                        if (size % 80 == 9)
                            gtia.setColors(data + size - 9);
                        else
                            gtia.setColors(GR10_COLORS);
                        renderGr10(gtia, data + size_t(height) * 40, frame2.data(), WIDTH, height, 1);
                    }

                    writeBlend(dest, frame.data(), frame2.data());
                    break;
                }
            }
        }
    };

    template <Mode mode>
    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        return new Interface(memory, mode);
    }

    // ====================================================================
    // Interlace / GTIA frame decoders (TIP, RIP, ICE)
    //
    // These formats build one or two "frames" of Atari color bytes through a
    // shared set of ANTIC/GTIA line renderers (GR.8/9/10/11/15), then map the
    // bytes through the GTIA palette - interlaced formats blend two fields.
    // ====================================================================

    // Atari 8-bit character ROM (uppercase/graphics set), 128 glyphs x 8 rows.
    const u8 g_atari8_font [1024] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 24, 24, 24, 0, 24, 0,
        0, 102, 102, 102, 0, 0, 0, 0, 0, 102, 255, 102, 102, 255, 102, 0,
        24, 62, 96, 60, 6, 124, 24, 0, 0, 102, 108, 24, 48, 102, 70, 0,
        28, 54, 28, 56, 111, 102, 59, 0, 0, 24, 24, 24, 0, 0, 0, 0,
        0, 14, 28, 24, 24, 28, 14, 0, 0, 112, 56, 24, 24, 56, 112, 0,
        0, 102, 60, 255, 60, 102, 0, 0, 0, 24, 24, 126, 24, 24, 0, 0,
        0, 0, 0, 0, 0, 24, 24, 48, 0, 0, 0, 126, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 24, 24, 0, 0, 6, 12, 24, 48, 96, 64, 0,
        0, 60, 102, 110, 118, 102, 60, 0, 0, 24, 56, 24, 24, 24, 126, 0,
        0, 60, 102, 12, 24, 48, 126, 0, 0, 126, 12, 24, 12, 102, 60, 0,
        0, 12, 28, 60, 108, 126, 12, 0, 0, 126, 96, 124, 6, 102, 60, 0,
        0, 60, 96, 124, 102, 102, 60, 0, 0, 126, 6, 12, 24, 48, 48, 0,
        0, 60, 102, 60, 102, 102, 60, 0, 0, 60, 102, 62, 6, 12, 56, 0,
        0, 0, 24, 24, 0, 24, 24, 0, 0, 0, 24, 24, 0, 24, 24, 48,
        6, 12, 24, 48, 24, 12, 6, 0, 0, 0, 126, 0, 0, 126, 0, 0,
        96, 48, 24, 12, 24, 48, 96, 0, 0, 60, 102, 12, 24, 0, 24, 0,
        0, 60, 102, 110, 110, 96, 62, 0, 0, 24, 60, 102, 102, 126, 102, 0,
        0, 124, 102, 124, 102, 102, 124, 0, 0, 60, 102, 96, 96, 102, 60, 0,
        0, 120, 108, 102, 102, 108, 120, 0, 0, 126, 96, 124, 96, 96, 126, 0,
        0, 126, 96, 124, 96, 96, 96, 0, 0, 62, 96, 96, 110, 102, 62, 0,
        0, 102, 102, 126, 102, 102, 102, 0, 0, 126, 24, 24, 24, 24, 126, 0,
        0, 6, 6, 6, 6, 102, 60, 0, 0, 102, 108, 120, 120, 108, 102, 0,
        0, 96, 96, 96, 96, 96, 126, 0, 0, 99, 119, 127, 107, 99, 99, 0,
        0, 102, 118, 126, 126, 110, 102, 0, 0, 60, 102, 102, 102, 102, 60, 0,
        0, 124, 102, 102, 124, 96, 96, 0, 0, 60, 102, 102, 102, 108, 54, 0,
        0, 124, 102, 102, 124, 108, 102, 0, 0, 60, 96, 60, 6, 6, 60, 0,
        0, 126, 24, 24, 24, 24, 24, 0, 0, 102, 102, 102, 102, 102, 126, 0,
        0, 102, 102, 102, 102, 60, 24, 0, 0, 99, 99, 107, 127, 119, 99, 0,
        0, 102, 102, 60, 60, 102, 102, 0, 0, 102, 102, 60, 24, 24, 24, 0,
        0, 126, 12, 24, 48, 96, 126, 0, 0, 30, 24, 24, 24, 24, 30, 0,
        0, 64, 96, 48, 24, 12, 6, 0, 0, 120, 24, 24, 24, 24, 120, 0,
        0, 8, 28, 54, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0,
        0, 54, 127, 127, 62, 28, 8, 0, 24, 24, 24, 31, 31, 24, 24, 24,
        3, 3, 3, 3, 3, 3, 3, 3, 24, 24, 24, 248, 248, 0, 0, 0,
        24, 24, 24, 248, 248, 24, 24, 24, 0, 0, 0, 248, 248, 24, 24, 24,
        3, 7, 14, 28, 56, 112, 224, 192, 192, 224, 112, 56, 28, 14, 7, 3,
        1, 3, 7, 15, 31, 63, 127, 255, 0, 0, 0, 0, 15, 15, 15, 15,
        128, 192, 224, 240, 248, 252, 254, 255, 15, 15, 15, 15, 0, 0, 0, 0,
        240, 240, 240, 240, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 240, 240, 240, 240,
        0, 28, 28, 119, 119, 8, 28, 0, 0, 0, 0, 31, 31, 24, 24, 24,
        0, 0, 0, 255, 255, 0, 0, 0, 24, 24, 24, 255, 255, 24, 24, 24,
        0, 0, 60, 126, 126, 126, 60, 0, 0, 0, 0, 0, 255, 255, 255, 255,
        192, 192, 192, 192, 192, 192, 192, 192, 0, 0, 0, 255, 255, 24, 24, 24,
        24, 24, 24, 255, 255, 0, 0, 0, 240, 240, 240, 240, 240, 240, 240, 240,
        24, 24, 24, 31, 31, 0, 0, 0, 120, 96, 120, 96, 126, 24, 30, 0,
        0, 24, 60, 126, 24, 24, 24, 0, 0, 24, 24, 24, 126, 60, 24, 0,
        0, 24, 48, 126, 48, 24, 0, 0, 0, 24, 12, 126, 12, 24, 0, 0,
        0, 24, 60, 126, 126, 60, 24, 0, 0, 0, 60, 6, 62, 102, 62, 0,
        0, 96, 96, 124, 102, 102, 124, 0, 0, 0, 60, 96, 96, 96, 60, 0,
        0, 6, 6, 62, 102, 102, 62, 0, 0, 0, 60, 102, 126, 96, 60, 0,
        0, 14, 24, 62, 24, 24, 24, 0, 0, 0, 62, 102, 102, 62, 6, 124,
        0, 96, 96, 124, 102, 102, 102, 0, 0, 24, 0, 56, 24, 24, 60, 0,
        0, 6, 0, 6, 6, 6, 6, 60, 0, 96, 96, 108, 120, 108, 102, 0,
        0, 56, 24, 24, 24, 24, 60, 0, 0, 0, 102, 127, 127, 107, 99, 0,
        0, 0, 124, 102, 102, 102, 102, 0, 0, 0, 60, 102, 102, 102, 60, 0,
        0, 0, 124, 102, 102, 124, 96, 96, 0, 0, 62, 102, 102, 62, 6, 6,
        0, 0, 124, 102, 96, 96, 96, 0, 0, 0, 62, 96, 60, 6, 124, 0,
        0, 24, 126, 24, 24, 24, 14, 0, 0, 0, 102, 102, 102, 102, 62, 0,
        0, 0, 102, 102, 102, 60, 24, 0, 0, 0, 99, 107, 127, 62, 54, 0,
        0, 0, 102, 60, 24, 60, 102, 0, 0, 0, 102, 102, 102, 62, 12, 120,
        0, 0, 126, 12, 24, 48, 126, 0, 0, 24, 60, 126, 126, 24, 60, 0,
        24, 24, 24, 24, 24, 24, 24, 24, 0, 126, 120, 124, 110, 102, 6, 0,
        8, 24, 56, 120, 56, 24, 8, 0, 16, 24, 28, 30, 28, 24, 16, 0,
    };

    // ICE per-frame ANTIC/GTIA rendering mode.
    enum class IceMode
    {
        Gr0, Gr0Gtia9, Gr0Gtia10, Gr0Gtia11,
        Gr12, Gr12Gtia9, Gr12Gtia10, Gr12Gtia11,
        Gr13Gtia9, Gr13Gtia10, Gr13Gtia11
    };

    static int gr12GtiaNibbleToGr8(int nibble, int ch, bool gtia10)
    {
        switch (nibble)
        {
            case 0: case 1: case 4: case 5: return 0;
            case 2: case 6: return 1;
            case 3: case 7: return (ch & 0x80) == 0 ? 2 : 3;
            case 8: return gtia10 ? 8 : 4;
            case 9: return 4;
            case 10: return 5;
            case 11: return (ch & 0x80) == 0 ? 6 : 7;
            case 12: return gtia10 || (ch & 0x80) == 0 ? 8 : 12;
            case 13: return (ch & 0x80) == 0 ? 8 : 12;
            case 14: return (ch & 0x80) == 0 ? 9 : 13;
            case 15: return (ch & 0x80) == 0 ? 10 : 15;
            default: return 0;
        }
    }

    static int gr12GtiaByteToGr8(int b, int ch, bool gtia10)
    {
        return (gr12GtiaNibbleToGr8(b >> 4, ch, gtia10) << 4)
             | gr12GtiaNibbleToGr8(b & 0xf, ch, gtia10);
    }

    // Shannon-Fano bit-length tree + bit stream for RIP "PCK" LZ77 compression.
    struct A8BitStream
    {
        const u8* content;
        int offset;
        int length;
        int bits = 0;

        int readBit()
        {
            if ((bits & 0x7f) == 0)
            {
                if (offset >= length)
                    return -1;
                bits = content[offset++] << 1 | 1;
            }
            else
            {
                bits <<= 1;
            }
            return bits >> 8 & 1;
        }
    };

    struct A8FanoTree
    {
        int count [16];
        u8 values [256];

        static int getNibble(const u8* c, int off, int index)
        {
            int b = c[off + (index >> 1)];
            return (index & 1) == 0 ? b >> 4 : b & 0xf;
        }

        void create(const u8* content, int contentOffset, int codeCount)
        {
            for (int i = 0; i < 16; ++i)
                count[i] = 0;
            for (int code = 0; code < codeCount; ++code)
                count[getNibble(content, contentOffset, code)]++;
            int positions [16];
            int position = 0;
            for (int bits = 0; bits < 16; ++bits)
            {
                positions[bits] = position;
                position += count[bits];
            }
            for (int code = 0; code < codeCount; ++code)
                values[positions[getNibble(content, contentOffset, code)]++] = u8(code);
        }

        int readCode(A8BitStream& bs) const
        {
            int code = 0;
            int valuesOffset = count[0];
            for (int bits = 1; bits < 16; ++bits)
            {
                int bit = bs.readBit();
                if (bit < 0)
                    return -1;
                code = code << 1 | bit;
                int c = count[bits];
                if (code < c)
                    return values[valuesOffset + code];
                code -= c;
                valuesOffset += c;
            }
            return -1;
        }
    };

    static bool copyPrevious(u8* unpacked, int offset, int distance, int count)
    {
        if (distance > offset)
            return false;
        do
        {
            unpacked[offset] = unpacked[offset - distance];
            offset++;
        }
        while (--count > 0);
        return true;
    }

    struct A8Decoder
    {
        const u8* m_pal = ATARI8_PAL;
        u8 gtia [16] = { 0 };
        int Width = 0;
        int Height = 0;
        int LeftSkip = 0;
        std::vector<u32> pixels; // 0x00RRGGBB

        void setSize(int width, int height)
        {
            Width = width;
            Height = height;
            pixels.assign(size_t(width) * height, 0);
        }

        u32 paletteRGB(int colorByte) const
        {
            const u8* p = m_pal + (colorByte & 0xff) * 3;
            return (u32(p[0]) << 16) | (u32(p[1]) << 8) | p[2];
        }

        // ---- GTIA color register setters ----

        void setGtiaColor(int reg, int value)
        {
            value &= 0xfe;
            switch (reg)
            {
                case 0: case 1: case 2: case 3:
                    gtia[reg] = u8(value);
                    break;
                case 4: case 5: case 6: case 7:
                    gtia[8 + reg] = gtia[reg] = u8(value);
                    break;
                case 8:
                    gtia[11] = gtia[10] = gtia[9] = gtia[8] = u8(value);
                    break;
            }
        }

        void setPM123PF0123Bak(const u8* c, int off)
        {
            for (int i = 0; i < 8; ++i)
                setGtiaColor(1 + i, c[off + i]);
        }

        void setGtiaColors(const u8* c, int off)
        {
            gtia[0] = c[off] & 0xfe;
            setPM123PF0123Bak(c, off + 1);
        }

        void setPF21(const u8* c, int off)
        {
            gtia[6] = c[off] & 0xfe;
            gtia[5] = c[off + 1] & 0xfe;
        }

        void setGr15DefaultColors()
        {
            gtia[8] = 0x00;
            gtia[4] = 0x04;
            gtia[5] = 0x08;
            gtia[6] = 0x0c;
        }

        void setBakPF012(const u8* c, int off, int stride)
        {
            for (int i = 0; i < 4; ++i)
                gtia[i == 0 ? 8 : 3 + i] = c[off + i * stride] & 0xfe;
        }

        void setBakPF0123(const u8* c, int off)
        {
            for (int i = 0; i < 5; ++i)
                gtia[i == 0 ? 8 : 3 + i] = c[off + i] & 0xfe;
        }

        void setPF0123Bak(const u8* c, int off)
        {
            for (int i = 0; i < 5; ++i)
                gtia[4 + i] = c[off + i] & 0xfe;
        }

        void setPF0123Even(const u8* c, int off)
        {
            for (int i = 0; i < 4; ++i)
                gtia[4 + i] = c[off + i * 2] & 0xfe;
        }

        // ---- ANTIC/GTIA line renderers (write Atari color bytes to frame) ----

        void gr8(const u8* content, int off, u8* frame, int frameOff, int height)
        {
            u8 colors [2];
            colors[0] = gtia[6];
            colors[1] = (gtia[6] & 0xf0) | (gtia[5] & 0x0e);
            frameOff -= LeftSkip;
            for (int y = 0; y < height; ++y)
            {
                int x;
                for (x = LeftSkip; x < Width; ++x)
                {
                    int c = content[off + (x >> 3)] >> (~x & 7) & 1;
                    frame[frameOff + x] = colors[c];
                }
                for ( ; x < Width + LeftSkip; ++x)
                    frame[frameOff + x] = gtia[8];
                off += (Width + 7) >> 3;
                frameOff += Width;
            }
        }

        void gr15(const u8* content, int off, int stride, u8* frame, int frameOff, int frameStride, int height)
        {
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < Width; ++x)
                {
                    int c = content[off + (x >> 3)] >> (~x & 6) & 3;
                    frame[frameOff + x] = gtia[c == 0 ? 8 : c + 3];
                }
                off += stride;
                frameOff += frameStride;
            }
        }

        void gr9(const u8* content, int off, int stride, u8* frame, int frameOff, int frameStride, int width, int height)
        {
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    int i = x + LeftSkip;
                    int c = (i < 0 || i >= width) ? 0 : content[off + (i >> 3)] >> (~i & 4) & 0x0f;
                    frame[frameOff + x] = gtia[8] | c;
                }
                off += stride;
                frameOff += frameStride;
            }
        }

        void gr11(const u8* content, int off, u8* frame, int frameOff, int frameStride, int height)
        {
            frameOff -= LeftSkip;
            for (int y = 0; y < height; ++y)
            {
                int x;
                for (x = LeftSkip; x < Width; ++x)
                {
                    int c = content[off + (x >> 3)] << (x & 4) & 0xf0;
                    c = c == 0 ? (gtia[8] & 0xf0) : (gtia[8] | c);
                    frame[frameOff + x] = u8(c);
                }
                for ( ; x < Width + LeftSkip; ++x)
                    frame[frameOff + x] = gtia[8] & 0xf0;
                off += Width >> 3;
                frameOff += frameStride;
            }
        }

        void gr10(const u8* content, int off, u8* frame, int frameOff, int frameStride, int height)
        {
            frameOff += 2 - LeftSkip;
            for (int y = 0; y < height; ++y)
            {
                int x;
                for (x = LeftSkip - 2; x < 0; ++x)
                    frame[frameOff + x] = gtia[0];
                for ( ; x < Width + LeftSkip - 2; ++x)
                {
                    int c = content[off + (x >> 3)] >> (~x & 4) & 0x0f;
                    frame[frameOff + x] = gtia[c];
                }
                off += Width >> 3;
                frameOff += frameStride;
            }
        }

        void gr11PalBlend(const u8* content, int off, int stride, u8* frame, int y)
        {
            for ( ; y < Height; y += 2)
            {
                int frameOff = y * Width - LeftSkip;
                for (int x = LeftSkip; x < Width; ++x)
                {
                    int c = content[off + (x >> 3)] << (x & 4) & 0xf0;
                    int i =
                        ((y == 0 ? 0 : frame[frameOff - Width + x] & 0x0f) +
                         (y == Height - 1 ? 0 : frame[frameOff + Width + x] & 0x0f)) >> 1;
                    frame[frameOff + x] = u8(c | i);
                    if (y < Height - 1)
                        frame[frameOff + Width + x] = u8(c | (frame[frameOff + Width + x] & 0x0f));
                }
                for (int k = 0; k < LeftSkip; ++k)
                    frame[frameOff + Width + k] = 0;
                off += stride;
            }
        }

        // ---- frame -> RGB ----

        void applyPalette(const u8* frame)
        {
            int n = Width * Height;
            for (int i = 0; i < n; ++i)
                pixels[i] = paletteRGB(frame[i]);
        }

        void applyBlend(const u8* f1, const u8* f2)
        {
            int n = Width * Height;
            for (int i = 0; i < n; ++i)
            {
                u32 a = paletteRGB(f1[i]);
                u32 b = paletteRGB(f2[i]);
                pixels[i] = (a & b) + ((a ^ b) >> 1 & 0x7f7f7f);
            }
        }

        // ================= TIP =================

        bool tip(const u8* content, size_t length)
        {
            if (length < 129
             || content[0] != 'T' || content[1] != 'I' || content[2] != 'P'
             || content[3] != 1 || content[4] != 0)
                return false;
            int width = content[5];
            int height = content[6];
            if (width > 160 || (width & 3) != 0 || height > 119)
                return false;
            int contentStride = width >> 2;
            int frameLength = content[7] | content[8] << 8;
            if (frameLength != contentStride * height
             || length != size_t(9 + 3 * frameLength))
                return false;

            setSize(width << 1, height << 1);
            LeftSkip = 1;
            const u8 colors [9] = { 0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x00 };
            setGtiaColors(colors, 0);

            std::vector<u8> frame1(size_t(Width) * Height + Width + 16, 0);
            std::vector<u8> frame2(size_t(Width) * Height + Width + 16, 0);

            gr9(content, 9, contentStride, frame1.data(), width << 1, width << 2, width << 1, height);
            gr11PalBlend(content, 9 + 2 * frameLength, contentStride, frame1.data(), 0);
            gr10(content, 9 + frameLength, frame2.data(), width << 1, width << 2, height);
            gr11PalBlend(content, 9 + 2 * frameLength, contentStride, frame2.data(), 0);
            applyBlend(frame1.data(), frame2.data());
            return true;
        }

        // ================= RIP =================

        static bool unpackRip(const u8* content, int contentOffset, int contentLength, u8* unpacked, int unpackedLength)
        {
            if (contentOffset + 304 > contentLength || std::memcmp(content + contentOffset, "PCK", 3) != 0)
                return false;

            A8FanoTree lengthTree;
            lengthTree.create(content, contentOffset + 16, 64);
            A8FanoTree distanceTree;
            distanceTree.create(content, contentOffset + 16 + 32, 256);
            A8FanoTree literalTree;
            literalTree.create(content, contentOffset + 16 + 32 + 128, 256);

            A8BitStream bs { content, contentOffset + 16 + 288, contentLength, 0 };
            for (int o = 0; o < unpackedLength; )
            {
                switch (bs.readBit())
                {
                    case 0:
                    {
                        int literal = literalTree.readCode(bs);
                        if (literal < 0)
                            return false;
                        unpacked[o++] = u8(literal);
                        break;
                    }
                    case 1:
                    {
                        int distance = distanceTree.readCode(bs);
                        if (distance < 0)
                            return false;
                        distance += 2;
                        int count = lengthTree.readCode(bs);
                        if (count < 0)
                            return false;
                        count = std::min(count + 2, unpackedLength - o);
                        if (!copyPrevious(unpacked, o, distance, count))
                            return false;
                        o += count;
                        if (o >= unpackedLength)
                            return true;
                        break;
                    }
                    default:
                        return false;
                }
            }
            return true;
        }

        bool rip(const u8* content, size_t length)
        {
            int contentLength = int(length);
            if (length < 34
             || content[0] != 'R' || content[1] != 'I' || content[2] != 'P'
             || content[18] != 'T' || content[19] != ':')
                return false;
            int headerLength = content[11] | content[12] << 8;
            int contentStride = content[13];
            int height = content[15];
            int textLength = content[17];
            if (headerLength >= contentLength
             || contentStride == 0 || contentStride > 80 || (contentStride & 1) != 0 || height == 0 || height > 239
             || 33 + textLength >= contentLength
             || content[20 + textLength] != 9
             || std::memcmp(content + 21 + textLength, "CM:", 3) != 0)
                return false;

            if (content[7] < 0x10)
                contentStride >>= 1;
            int unpackedLength = contentStride * height;
            if (content[7] == 0x30)
                unpackedLength += ((height + 1) >> 1) << 3;

            std::vector<u8> unpacked(239 * 80 + 120 * 8, 0);
            switch (content[9])
            {
                case 0:
                    if (headerLength + unpackedLength > contentLength)
                        return false;
                    std::memcpy(unpacked.data(), content + headerLength, unpackedLength);
                    break;
                case 1:
                    // Many RIP files (Atari Interlace Studio) are corrupted; ignore unpack errors.
                    unpackRip(content, headerLength, contentLength, unpacked.data(), unpackedLength);
                    break;
                default:
                    return false;
            }

            setGtiaColors(content, 24 + textLength);
            contentStride = content[13] >> 1;
            int width = contentStride << 3;
            const u8* u = unpacked.data();

            std::vector<u8> frame1(size_t(320) * 239 + 320 + 16, 0);
            std::vector<u8> frame2(size_t(320) * 239 + 320 + 16, 0);

            switch (content[7])
            {
                case 0x0e: // GR.15
                    setSize(width, height);
                    gr15(u, 0, contentStride, frame1.data(), 0, width, height);
                    applyPalette(frame1.data());
                    return true;
                case 0x0f: // GR.8
                    setSize(width, height);
                    gr8(u, 0, frame1.data(), 0, height);
                    applyPalette(frame1.data());
                    return true;
                case 0x4f: // GR.9
                    setSize(width, height);
                    gr9(u, 0, contentStride, frame1.data(), 0, width, width, height);
                    applyPalette(frame1.data());
                    return true;
                case 0x8f: // GR.10
                    setSize(width, height);
                    LeftSkip = 2;
                    gr10(u, 0, frame1.data(), 0, width, height);
                    applyPalette(frame1.data());
                    return true;
                case 0xcf: // GR.11
                    setSize(width, height);
                    gr11(content, 0, frame1.data(), 0, width, height);
                    applyPalette(frame1.data());
                    return true;
                case 0x1e: // GR.15 blend (one palette)
                    setSize(width, height);
                    gr15(u, 0, contentStride, frame1.data(), 0, width, height);
                    gr15(u, height * contentStride, contentStride, frame2.data(), 0, width, height);
                    applyBlend(frame1.data(), frame2.data());
                    return true;
                case 0x10: // GR.15 blend (different palettes)
                    setSize(width, height);
                    setBakPF012(content, 28 + textLength, 1);
                    gr15(u, 0, contentStride << 1, frame1.data(), 0, width << 1, height >> 1);
                    setBakPF012(content, 24 + textLength, 1);
                    gr15(u, contentStride, contentStride << 1, frame1.data(), width, width << 1, height >> 1);
                    gr15(u, height * contentStride, contentStride << 1, frame2.data(), 0, width << 1, height >> 1);
                    setBakPF012(content, 28 + textLength, 1);
                    gr15(u, (height + 1) * contentStride, contentStride << 1, frame2.data(), width, width << 1, height >> 1);
                    applyBlend(frame1.data(), frame2.data());
                    return true;
                case 0x20: // HIP/RIP
                    setSize(width, height);
                    LeftSkip = 1;
                    gr10(u, 0, frame1.data(), 0, width, height);
                    gtia[8] = 0x00;
                    gr9(u, height * contentStride, contentStride, frame2.data(), 0, width, width, height);
                    applyBlend(frame1.data(), frame2.data());
                    return true;
                case 0x30: // Multi RIP
                {
                    setSize(width, height);
                    LeftSkip = 1;
                    gtia[0] = 0x00;
                    int colorsOffset = height * contentStride << 1;
                    for (int y = 0; y < height; y += 2)
                    {
                        setPM123PF0123Bak(u, colorsOffset + (y << 2));
                        gr10(u, y * contentStride, frame1.data(), y * width, width, y + 1 < height ? 2 : 1);
                    }
                    gtia[8] = 0x00;
                    gr9(u, height * contentStride, contentStride, frame2.data(), 0, width, width, height);
                    applyBlend(frame1.data(), frame2.data());
                    return true;
                }
                default:
                    return false;
            }
        }

        // ================= ICE =================

        static constexpr int IceFontFrame1 = -1;
        static constexpr int IceFontFrame2 = -2;

        void iceFrame(const u8* content, int charactersOffset, int fontOffset, u8* frame, IceMode mode)
        {
            int doubleLine =
                (mode == IceMode::Gr13Gtia9 || mode == IceMode::Gr13Gtia10 || mode == IceMode::Gr13Gtia11) ? 1 : 0;
            int frameOff = 0;
            u8 bitmap [48];
            for (int y = 0; y < Height; ++y)
            {
                for (int col = 0; col < Width >> 3; ++col)
                {
                    int ch;
                    if (charactersOffset == IceFontFrame1)
                    {
                        static const u8 row2char1 [16] =
                            { 0x40, 0x00, 0x20, 0x60, 0xc0, 0x80, 0xa0, 0xe0, 0x40, 0x00, 0x20, 0x60, 0xc0, 0x80, 0xa0, 0xe0 };
                        ch = row2char1[y >> (3 + doubleLine)] + col;
                    }
                    else if (charactersOffset == IceFontFrame2)
                    {
                        static const u8 row2char2 [16] =
                            { 0x40, 0x00, 0x20, 0x60, 0xc0, 0x80, 0xa0, 0xe0, 0xc0, 0x80, 0xa0, 0xe0, 0x40, 0x00, 0x20, 0x60 };
                        ch = row2char2[y >> (3 + doubleLine)] + col;
                    }
                    else
                    {
                        ch = (y / 24 << 8) + content[charactersOffset + (y >> 3) * 40 + col];
                    }
                    int b = content[fontOffset + ((ch & ~0x80) << 3) + (y >> doubleLine & 7)];
                    switch (mode)
                    {
                        case IceMode::Gr0:
                        case IceMode::Gr0Gtia9:
                        case IceMode::Gr0Gtia10:
                        case IceMode::Gr0Gtia11:
                            if (charactersOffset < 0 && (ch & 0x80) != 0)
                                b ^= 0xff;
                            bitmap[col] = u8(b);
                            break;
                        case IceMode::Gr12:
                            for (int x = (col == 0 ? LeftSkip : 0); x < 8; ++x)
                            {
                                int c = b >> (~x & 6) & 3;
                                int regs = (ch & 0x80) == 0 ? 0x6548 : 0x7548;
                                frame[frameOff + (col << 3) + x - LeftSkip] = gtia[regs >> (c << 2) & 0xf];
                            }
                            break;
                        case IceMode::Gr12Gtia9:
                        case IceMode::Gr12Gtia11:
                        case IceMode::Gr13Gtia9:
                        case IceMode::Gr13Gtia11:
                            bitmap[col] = u8(gr12GtiaByteToGr8(b, ch, false));
                            break;
                        case IceMode::Gr12Gtia10:
                        case IceMode::Gr13Gtia10:
                            bitmap[col] = u8(gr12GtiaByteToGr8(b, ch, true));
                            break;
                    }
                }
                switch (mode)
                {
                    case IceMode::Gr0:
                        gr8(bitmap, 0, frame, frameOff, 1);
                        break;
                    case IceMode::Gr12:
                        for (int x = Width; x < Width + LeftSkip; ++x)
                            frame[frameOff + x] = gtia[8];
                        break;
                    case IceMode::Gr0Gtia9:
                    case IceMode::Gr12Gtia9:
                    case IceMode::Gr13Gtia9:
                        gr9(bitmap, 0, 0, frame, frameOff, 0, Width, 1);
                        break;
                    case IceMode::Gr0Gtia10:
                    case IceMode::Gr12Gtia10:
                    case IceMode::Gr13Gtia10:
                        gr10(bitmap, 0, frame, frameOff, 0, 1);
                        break;
                    case IceMode::Gr0Gtia11:
                    case IceMode::Gr12Gtia11:
                    case IceMode::Gr13Gtia11:
                        gr11(bitmap, 0, frame, frameOff, 0, 1);
                        break;
                }
                frameOff += Width;
            }
        }

        void ice20Frame(const u8* content, bool second, int fontOffset, u8* frame, int mode)
        {
            u8 bitmap [32];
            for (int y = 0; y < 288; ++y)
            {
                int row = y >> 5;
                int c = (second ? row / 3 : row % 3) + 1;
                for (int col = 0; col < 32; ++col)
                {
                    int ch = ((y & 0x18) << 1) + (col >> 1);
                    int b = content[fontOffset + (ch << 3) + (y & 7)];
                    b = (col & 1) == 0 ? b >> 4 : b & 0xf;
                    b = ((b & 8) << 3 | (b & 4) << 2 | (b & 2) << 1 | (b & 1)) * c;
                    if (mode == 10)
                    {
                        if ((b & 0x70) == 0x40)
                            b = 0x80 + (b & 0xf);
                        if ((b & 7) == 4)
                            b = (b & 0xf0) + 8;
                    }
                    bitmap[col] = u8(b);
                }
                switch (mode)
                {
                    case 9:  gr9(bitmap, 0, 0, frame, y << 8, 0, 256, 1); break;
                    case 10: gr10(bitmap, 0, frame, y << 8, 0, 1); break;
                    case 11: gr11(bitmap, 0, frame, y << 8, 0, 1); break;
                }
            }
        }

        bool ice(const u8* content, size_t length, bool font, int mode)
        {
            int contentLength = int(length);
            std::vector<u8> f1(size_t(256) * 288 + 256 + 16, 0);
            std::vector<u8> f2(size_t(256) * 288 + 256 + 16, 0);
            u8* frame1 = f1.data();
            u8* frame2 = f2.data();

            auto verify = [&](int fontLength, int imageLength) -> bool
            {
                if (font)
                {
                    if (contentLength != fontLength)
                        return false;
                    setSize(256, 128);
                }
                else
                {
                    if (contentLength != imageLength || content[0] != 1)
                        return false;
                    setSize(320, 192);
                }
                return true;
            };

            static const u8 ice20Gtia11Colors [7] = { 0, 1, 2, 3, 5, 7, 8 };

            switch (mode)
            {
                case 0:
                    if (contentLength != 5 + 2048) return false;
                    setSize(256, 128);
                    gtia[5] = content[1] & 0xfe; gtia[6] = content[3] & 0xfe;
                    iceFrame(content, IceFontFrame1, 5, frame1, IceMode::Gr0);
                    gtia[5] = content[2] & 0xfe; gtia[6] = content[4] & 0xfe;
                    iceFrame(content, IceFontFrame2, 5 + 1024, frame2, IceMode::Gr0);
                    break;
                case 1:
                    if (!verify(6 + 2048, 6 + 16384 + 2 * 960)) return false;
                    setBakPF0123(content, 1);
                    iceFrame(content, font ? IceFontFrame1 : 6 + 16384, 6, frame1, IceMode::Gr12);
                    iceFrame(content, font ? IceFontFrame2 : 6 + 16384 + 960, 6 + 1024, frame2, IceMode::Gr12);
                    break;
                case 2:
                    if (!verify(10 + 2048, 10 + 16384 + 2 * 960)) return false;
                    gtia[8] = content[1] & 0xfe;
                    setPF0123Even(content, 2);
                    iceFrame(content, font ? IceFontFrame1 : 10 + 16384, 10, frame1, IceMode::Gr12);
                    setPF0123Even(content, 3);
                    iceFrame(content, font ? IceFontFrame2 : 10 + 16384 + 960, 10 + 1024, frame2, IceMode::Gr12);
                    break;
                case 3:
                    if (font)
                    {
                        if (contentLength != 7 + 2048) return false;
                        setSize(256, 128);
                    }
                    else
                    {
                        if (contentLength != 7 + 16384 + 960 || content[0] != 3) return false;
                        setSize(320, 192);
                    }
                    setPF21(content, 1);
                    iceFrame(content, font ? IceFontFrame1 : 7 + 16384, 7, frame1, IceMode::Gr0);
                    setBakPF0123(content, 2);
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, font ? IceFontFrame2 : 7 + 16384, 7 + 1024, frame2, IceMode::Gr12);
                    break;
                case 4:
                    if (contentLength != 10 + 2048) return false;
                    setSize(256, 128);
                    LeftSkip = 2;
                    setGtiaColors(content, 1);
                    iceFrame(content, IceFontFrame1, 10, frame1, IceMode::Gr0Gtia10);
                    iceFrame(content, IceFontFrame2, 10 + 1024, frame2, IceMode::Gr0Gtia10);
                    break;
                case 5:
                    if (contentLength != 17 + 2048 && contentLength != 18 + 2048) return false;
                    setSize(256, 128);
                    LeftSkip = 2;
                    gtia[0] = content[1] & 0xfe;
                    for (int i = 0; i < 8; ++i) setGtiaColor(i + 1, content[2 + i * 2]);
                    if (contentLength == 17 + 2048)
                    {
                        iceFrame(content, IceFontFrame1, 17, frame1, IceMode::Gr0Gtia10);
                        for (int i = 0; i < 7; ++i) setGtiaColor(i + 1, content[3 + i * 2]);
                        iceFrame(content, IceFontFrame2, 17 + 1024, frame2, IceMode::Gr0Gtia10);
                    }
                    else
                    {
                        iceFrame(content, IceFontFrame1, 18, frame1, IceMode::Gr0Gtia10);
                        for (int i = 0; i < 8; ++i) setGtiaColor(i + 1, content[3 + i * 2]);
                        iceFrame(content, IceFontFrame2, 18 + 1024, frame2, IceMode::Gr0Gtia10);
                    }
                    break;
                case 6:
                    if (contentLength != 3 + 2048) return false;
                    setSize(256, 128); LeftSkip = 0;
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame1, 3, frame1, IceMode::Gr0Gtia9);
                    gtia[8] = content[2] & 0xfe;
                    iceFrame(content, IceFontFrame2, 3 + 1024, frame2, IceMode::Gr0Gtia9);
                    break;
                case 7:
                    if (contentLength != 3 + 2048) return false;
                    setSize(256, 128);
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame1, 3, frame1, IceMode::Gr0Gtia11);
                    gtia[8] = content[2] & 0xfe;
                    iceFrame(content, IceFontFrame2, 3 + 1024, frame2, IceMode::Gr0Gtia11);
                    break;
                case 8:
                    if (contentLength != 10 + 2048) return false;
                    setSize(256, 128); LeftSkip = 1;
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame1, 10, frame1, IceMode::Gr0Gtia9);
                    setGtiaColors(content, 1);
                    iceFrame(content, IceFontFrame2, 10 + 1024, frame2, IceMode::Gr0Gtia10);
                    break;
                case 9:
                    if (contentLength != 10 + 2048) return false;
                    setSize(256, 128); LeftSkip = 1;
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame1, 10, frame1, IceMode::Gr0Gtia11);
                    gtia[0] = 0x00;
                    setPM123PF0123Bak(content, 2);
                    iceFrame(content, IceFontFrame2, 10 + 1024, frame2, IceMode::Gr0Gtia10);
                    break;
                case 10:
                    if (contentLength != 3 + 2048) return false;
                    setSize(256, 128);
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame1, 3, frame1, IceMode::Gr0Gtia9);
                    gtia[8] = content[2] & 0xfe;
                    iceFrame(content, IceFontFrame2, 3 + 1024, frame2, IceMode::Gr0Gtia11);
                    break;
                case 11:
                    if (contentLength != 3 + 2048) return false;
                    setSize(256, 128);
                    gtia[6] = 0x00; gtia[5] = content[2] & 0xfe;
                    iceFrame(content, IceFontFrame1, 3, frame1, IceMode::Gr0);
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame2, 3 + 1024, frame2, IceMode::Gr0Gtia11);
                    break;
                case 12:
                    if (contentLength != 3 + 2048) return false;
                    setSize(256, 128);
                    setPF21(content, 1);
                    iceFrame(content, IceFontFrame1, 3, frame1, IceMode::Gr0);
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame2, 3 + 1024, frame2, IceMode::Gr0Gtia9);
                    break;
                case 13:
                    if (contentLength != 11 + 2048) return false;
                    setSize(256, 128);
                    setPF21(content, 1);
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame1, 11, frame1, IceMode::Gr0);
                    LeftSkip = 2;
                    gtia[0] = content[1] & 0xfe;
                    setPM123PF0123Bak(content, 3);
                    iceFrame(content, IceFontFrame2, 11 + 1024, frame2, IceMode::Gr0Gtia10);
                    break;
                case 14:
                    if (contentLength != 6 + 2048) return false;
                    setSize(256, 128); LeftSkip = 0;
                    setBakPF0123(content, 1);
                    iceFrame(content, IceFontFrame2, 6 + 1024, frame2, IceMode::Gr12Gtia11);
                    gtia[8] = 0x00;
                    iceFrame(content, IceFontFrame1, 6, frame1, IceMode::Gr12);
                    break;
                case 15:
                    if (contentLength != 6 + 2048) return false;
                    setSize(256, 128);
                    setBakPF0123(content, 1);
                    iceFrame(content, IceFontFrame1, 6, frame1, IceMode::Gr12);
                    iceFrame(content, IceFontFrame2, 6 + 1024, frame2, IceMode::Gr12Gtia9);
                    break;
                case 16:
                    if (contentLength != 10 + 2048) return false;
                    setSize(256, 128); LeftSkip = 2;
                    setGtiaColors(content, 1);
                    iceFrame(content, IceFontFrame2, 10 + 1024, frame2, IceMode::Gr12Gtia10);
                    LeftSkip = 0;
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame1, 10, frame1, IceMode::Gr12);
                    break;
                case 17:
                    if (!verify(6 + 2048, 6 + 16384 + 960)) return false;
                    setBakPF0123(content, 1);
                    iceFrame(content, font ? IceFontFrame2 : 6 + 16384, 6 + 1024, frame2, IceMode::Gr0Gtia11);
                    gtia[8] = 0x00;
                    iceFrame(content, font ? IceFontFrame1 : 6 + 16384, 6, frame1, IceMode::Gr12);
                    break;
                case 18:
                    if (!verify(6 + 2048, 6 + 16384 + 960)) return false;
                    setBakPF0123(content, 1);
                    iceFrame(content, font ? IceFontFrame1 : 6 + 16384, 6, frame1, IceMode::Gr12);
                    iceFrame(content, font ? IceFontFrame2 : 6 + 16384, 6 + 1024, frame2, IceMode::Gr0Gtia9);
                    break;
                case 19:
                    if (!verify(10 + 2048, 10 + 16384 + 960)) return false;
                    setPF0123Bak(content, 5);
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, font ? IceFontFrame1 : 10 + 16384, 10, frame1, IceMode::Gr12);
                    LeftSkip = 2;
                    setGtiaColors(content, 1);
                    iceFrame(content, font ? IceFontFrame2 : 10 + 16384, 10 + 1024, frame2, IceMode::Gr0Gtia10);
                    break;
                case 22:
                    if (contentLength != 10 + 2048) return false;
                    setSize(256, 256); LeftSkip = 2;
                    setGtiaColors(content, 1);
                    iceFrame(content, IceFontFrame1, 10, frame1, IceMode::Gr13Gtia10);
                    iceFrame(content, IceFontFrame2, 10 + 1024, frame2, IceMode::Gr13Gtia10);
                    break;
                case 23:
                    if (contentLength != 17 + 2048) return false;
                    setSize(256, 256); LeftSkip = 2;
                    gtia[0] = content[1] & 0xfe;
                    for (int i = 0; i < 8; ++i) setGtiaColor(i + 1, content[2 + i * 2]);
                    iceFrame(content, IceFontFrame1, 17, frame1, IceMode::Gr13Gtia10);
                    for (int i = 0; i < 7; ++i) setGtiaColor(i + 1, content[3 + i * 2]);
                    iceFrame(content, IceFontFrame2, 17 + 1024, frame2, IceMode::Gr13Gtia10);
                    break;
                case 24:
                    if (contentLength != 3 + 2048) return false;
                    setSize(256, 256);
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame1, 3, frame1, IceMode::Gr13Gtia9);
                    gtia[8] = content[2] & 0xfe;
                    iceFrame(content, IceFontFrame2, 3 + 1024, frame2, IceMode::Gr13Gtia9);
                    break;
                case 25:
                    if (contentLength != 3 + 2048) return false;
                    setSize(256, 256);
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame1, 3, frame1, IceMode::Gr13Gtia11);
                    gtia[8] = content[2] & 0xfe;
                    iceFrame(content, IceFontFrame2, 3 + 1024, frame2, IceMode::Gr13Gtia11);
                    break;
                case 26:
                    if (contentLength != 10 + 2048) return false;
                    setSize(256, 256); LeftSkip = 1;
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame1, 10, frame1, IceMode::Gr13Gtia9);
                    setGtiaColors(content, 1);
                    iceFrame(content, IceFontFrame2, 10 + 1024, frame2, IceMode::Gr13Gtia10);
                    break;
                case 27:
                    if (contentLength != 10 + 2048) return false;
                    setSize(256, 256); LeftSkip = 1;
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame1, 10, frame1, IceMode::Gr13Gtia11);
                    gtia[0] = 0x00;
                    setPM123PF0123Bak(content, 2);
                    iceFrame(content, IceFontFrame2, 10 + 1024, frame2, IceMode::Gr13Gtia10);
                    break;
                case 28:
                    if (contentLength != 3 + 2048) return false;
                    setSize(256, 256);
                    gtia[8] = content[1] & 0xfe;
                    iceFrame(content, IceFontFrame1, 3, frame1, IceMode::Gr13Gtia9);
                    gtia[8] = content[2] & 0xfe;
                    iceFrame(content, IceFontFrame2, 3 + 1024, frame2, IceMode::Gr13Gtia11);
                    break;
                case 31:
                    if (contentLength != 8 + 1024) return false;
                    setSize(256, 288); LeftSkip = 2;
                    for (int i = 0; i < 7; ++i) setGtiaColor(ice20Gtia11Colors[i], content[1 + i]);
                    ice20Frame(content, false, 8, frame1, 10);
                    ice20Frame(content, true, 8 + 512, frame2, 10);
                    break;
                case 32:
                    if (contentLength != 14 + 1024) return false;
                    setSize(256, 288); LeftSkip = 2;
                    gtia[0] = content[1] & 0xfe;
                    for (int i = 1; i < 7; ++i) setGtiaColor(ice20Gtia11Colors[i], content[i * 2]);
                    ice20Frame(content, false, 14, frame1, 10);
                    for (int i = 1; i < 7; ++i) setGtiaColor(ice20Gtia11Colors[i], content[1 + i * 2]);
                    ice20Frame(content, true, 14 + 512, frame2, 10);
                    break;
                case 33:
                    if (contentLength != 3 + 1024) return false;
                    setSize(256, 288);
                    gtia[8] = content[1] & 0xfe;
                    ice20Frame(content, false, 3, frame1, 9);
                    gtia[8] = content[2] & 0xfe;
                    ice20Frame(content, true, 3 + 512, frame2, 9);
                    break;
                case 34:
                    if (contentLength != 3 + 1024) return false;
                    setSize(256, 288);
                    gtia[8] = content[1] & 0xfe;
                    ice20Frame(content, false, 3, frame1, 11);
                    gtia[8] = content[2] & 0xfe;
                    ice20Frame(content, true, 3 + 512, frame2, 11);
                    break;
                case 35:
                    if (contentLength != 8 + 1024) return false;
                    setSize(256, 288); LeftSkip = 1;
                    gtia[8] = content[1] & 0xfe;
                    ice20Frame(content, false, 8, frame1, 9);
                    for (int i = 0; i < 7; ++i) setGtiaColor(ice20Gtia11Colors[i], content[1 + i]);
                    ice20Frame(content, true, 8 + 512, frame2, 10);
                    break;
                case 36:
                    if (contentLength != 8 + 1024) return false;
                    setSize(256, 288); LeftSkip = 1;
                    gtia[8] = content[1] & 0xfe;
                    ice20Frame(content, false, 8, frame1, 11);
                    gtia[0] = 0;
                    for (int i = 1; i < 7; ++i) setGtiaColor(ice20Gtia11Colors[i], content[1 + i]);
                    ice20Frame(content, true, 8 + 512, frame2, 10);
                    break;
                case 37:
                    if (contentLength != 3 + 1024) return false;
                    setSize(256, 288);
                    gtia[8] = content[1] & 0xfe;
                    ice20Frame(content, false, 3, frame1, 9);
                    gtia[8] = content[2] & 0xfe;
                    ice20Frame(content, true, 3 + 512, frame2, 11);
                    break;
                default:
                    return false;
            }

            applyBlend(frame1, frame2);
            return true;
        }
    };

    enum class A8Multi { TIP, RIP, ICE };

    struct InterfaceA8Multi : ImageDecodeInterface
    {
        A8Decoder m_decoder;

        InterfaceA8Multi(ConstMemory memory, A8Multi kind)
        {
            const u8* content = memory.address;
            size_t length = memory.size;

            bool ok = false;
            switch (kind)
            {
                case A8Multi::TIP:
                    ok = m_decoder.tip(content, length);
                    break;
                case A8Multi::RIP:
                    ok = m_decoder.rip(content, length);
                    break;
                case A8Multi::ICE:
                    ok = length > 1024 && m_decoder.ice(content, length, true, content[0]);
                    break;
            }

            if (!ok)
            {
                header.setError("[ImageDecoder.Atari8] Incorrect or unsupported file.");
                return;
            }

            header.width = m_decoder.Width;
            header.height = m_decoder.Height;
            header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
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

            for (int y = 0; y < m_decoder.Height; ++y)
            {
                const u32* src = &m_decoder.pixels[size_t(y) * m_decoder.Width];
                u8* d = target.address<u8>(0, y);
                for (int x = 0; x < m_decoder.Width; ++x)
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

    template <A8Multi kind>
    ImageDecodeInterface* createInterfaceA8Multi(ConstMemory memory)
    {
        return new InterfaceA8Multi(memory, kind);
    }

} // namespace

namespace mango::image
{

    void registerImageCodecATARI8()
    {
        registerImageDecoder(createInterface<Mode::GR8>, ".gr8");
        registerImageDecoder(createInterface<Mode::GR9>, ".gr9");
        registerImageDecoder(createInterface<Mode::MIC>, ".mic");
        registerImageDecoder(createInterface<Mode::HIP>, ".hip");
        registerImageDecoder(createInterface<Mode::PI9>, ".pi9");

        // Interlace / GTIA frame formats
        registerImageDecoder(createInterfaceA8Multi<A8Multi::TIP>, ".tip");
        registerImageDecoder(createInterfaceA8Multi<A8Multi::RIP>, ".rip");
        registerImageDecoder(createInterfaceA8Multi<A8Multi::ICE>, ".ice");
    }

} // namespace mango::image
