/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
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

    Colors come from the Altirra-generated 256-entry GTIA palette (PAL by
    default; the NTSC table is kept for completeness). Each screen mode builds
    a frame of Atari color bytes (hue<<4 | luma) which is then mapped through
    that palette. Interlaced modes blend two fields per RECOIL.

    Companion files (palette / raster sidecars) and the rarer container
    variants are intentionally ignored - this is a single file decoder.

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

        // BAK, PF0, PF1, PF2 packed as 4 bytes (RECOIL SetBakPF012).
        void setBakPF012(const u8* p)
        {
            reg[8] = u8(p[0] & 254);
            reg[4] = u8(p[1] & 254);
            reg[5] = u8(p[2] & 254);
            reg[6] = u8(p[3] & 254);
        }

        // PF0, PF1, PF2, BAK packed as 4 bytes (RECOIL SetPF012Bak).
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

        // Average two GTIA fields (RECOIL ApplyAtari8PaletteBlend).
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
    }

} // namespace mango::image
