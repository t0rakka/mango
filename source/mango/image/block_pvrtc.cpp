/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/image/compression.hpp>
#include <mango/image/color.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    /******************************************************************************
     @File         PVRTDecompress.cpp
     @Title        PVRTDecompress
     @Version
     @Copyright    Copyright (c) Imagination Technologies Limited.
     @Platform     ANSI compatible
     @Description  PVRTC Texture Decompression.
     ******************************************************************************/

    // PVRTC decompressor (C) Imagination Technologies Limited.
    // Adapted and optimized for MANGO in December 2016.

    struct Pixel32
    {
        u8 r, g, b, a;
    };

    struct Pixel128S
    {
        s32 r, g, b, a;
    };

    struct PVRTCWord
    {
        u32 u32ModulationData;
        u32 u32ColorData;
    };

    constexpr u32 PUNCHTHROUGH_ALPHA = 0x10;

    static Pixel32 getColorA(u32 u32ColorData)
    {
        Pixel32 color;

        if ((u32ColorData & 0x8000) != 0)
        {
            // Opaque Color Mode - RGB 554
            color.r = u8((u32ColorData & 0x7c00) >> 10); // 5->5 bits
            color.g = u8((u32ColorData & 0x3e0)  >> 5); // 5->5 bits
            color.b = u8(u32ColorData  & 0x1e) | ((u32ColorData & 0x1e) >> 4); // 4->5 bits
            color.a = u8(0xf);// 0->4 bits
        }
        else
        {
            // Transparent Color Mode - ARGB 3443
            color.r = u8((u32ColorData & 0xf00)  >> 7) | ((u32ColorData & 0xf00) >> 11); // 4->5 bits
            color.g = u8((u32ColorData & 0xf0)   >> 3) | ((u32ColorData & 0xf0)  >> 7); // 4->5 bits
            color.b = u8((u32ColorData & 0xe)    << 1) | ((u32ColorData & 0xe)   >> 2); // 3->5 bits
            color.a = u8((u32ColorData & 0x7000) >> 11);// 3->4 bits - note 0 at right
        }
        
        return color;
    }

    static Pixel32 getColorB(u32 u32ColorData)
    {
        Pixel32 color;

        if (u32ColorData & 0x80000000)
        {
            // Opaque Color Mode - RGB 555
            color.r = u8((u32ColorData & 0x7c000000) >> 26); // 5->5 bits
            color.g = u8((u32ColorData & 0x3e00000)  >> 21); // 5->5 bits
            color.b = u8((u32ColorData & 0x1f0000)   >> 16); // 5->5 bits
            color.a = u8(0xf);// 0 bits
        }
        else
        {
            // Transparent Color Mode - ARGB 3444
            color.r = u8(((u32ColorData & 0xf000000) >> 23) | ((u32ColorData & 0xf000000) >> 27)); // 4->5 bits
            color.g = u8(((u32ColorData & 0xf00000)  >> 19) | ((u32ColorData & 0xf00000)  >> 23)); // 4->5 bits
            color.b = u8(((u32ColorData & 0xf0000)   >> 15) | ((u32ColorData & 0xf0000)   >> 19)); // 4->5 bits
            color.a = u8((u32ColorData & 0x70000000) >> 27);// 3->4 bits - note 0 at right
        }

        return color;
    }

    static
    void interpolateColors(Pixel32 color[4], Pixel128S* pPixel, u8 ui8Bpp)
    {
        const u32 ui32WordWidth = (ui8Bpp == 2) ? 8 : 4;
        const u32 ui32WordHeight = 4;

        //Convert to int 32.
        Pixel128S hP = { (s32)color[0].r, (s32)color[0].g, (s32)color[0].b, (s32)color[0].a };
        Pixel128S hQ = { (s32)color[1].r, (s32)color[1].g, (s32)color[1].b, (s32)color[1].a };
        Pixel128S hR = { (s32)color[2].r, (s32)color[2].g, (s32)color[2].b, (s32)color[2].a };
        Pixel128S hS = { (s32)color[3].r, (s32)color[3].g, (s32)color[3].b, (s32)color[3].a };

        //Get vectors.
        Pixel128S QminusP = { hQ.r - hP.r, hQ.g - hP.g, hQ.b - hP.b, hQ.a - hP.a };
        Pixel128S SminusR = { hS.r - hR.r, hS.g - hR.g, hS.b - hR.b, hS.a - hR.a };

        //Multiply colors.
        hP.r *= ui32WordWidth;
        hP.g *= ui32WordWidth;
        hP.b *= ui32WordWidth;
        hP.a *= ui32WordWidth;
        hR.r *= ui32WordWidth;
        hR.g *= ui32WordWidth;
        hR.b *= ui32WordWidth;
        hR.a *= ui32WordWidth;

        if (ui8Bpp == 2)
        {
            //Loop through pixels to achieve results.
            for (u32 x = 0; x < ui32WordWidth; x++)
            {
                Pixel128S result = { 4 * hP.r, 4 * hP.g, 4 * hP.b, 4 * hP.a };
                Pixel128S dY = { hR.r - hP.r, hR.g - hP.g, hR.b - hP.b, hR.a - hP.a };

                u32 offset = x;

                for (u32 y = 0; y < ui32WordHeight; y++)
                {
                    pPixel[offset].r = s32((result.r >> 7) + (result.r >> 2));
                    pPixel[offset].g = s32((result.g >> 7) + (result.g >> 2));
                    pPixel[offset].b = s32((result.b >> 7) + (result.b >> 2));
                    pPixel[offset].a = s32((result.a >> 5) + (result.a >> 1));

                    offset += ui32WordWidth;

                    result.r += dY.r;
                    result.g += dY.g;
                    result.b += dY.b;
                    result.a += dY.a;
                }

                hP.r += QminusP.r;
                hP.g += QminusP.g;
                hP.b += QminusP.b;
                hP.a += QminusP.a;

                hR.r += SminusR.r;
                hR.g += SminusR.g;
                hR.b += SminusR.b;
                hR.a += SminusR.a;
            }
        }
        else
        {
            //Loop through pixels to achieve results.
            for (u32 y = 0; y < ui32WordHeight; y++)
            {
                Pixel128S result = { 4 * hP.r, 4 * hP.g, 4 * hP.b, 4 * hP.a };
                Pixel128S dY = { hR.r - hP.r, hR.g - hP.g, hR.b - hP.b, hR.a - hP.a };

                u32 offset = y;

                for (u32 x = 0; x < ui32WordWidth; x++)
                {
                    pPixel[offset].r = s32((result.r >> 6) + (result.r >> 1));
                    pPixel[offset].g = s32((result.g >> 6) + (result.g >> 1));
                    pPixel[offset].b = s32((result.b >> 6) + (result.b >> 1));
                    pPixel[offset].a = s32((result.a >> 4) + (result.a));

                    offset += ui32WordWidth;

                    result.r += dY.r;
                    result.g += dY.g;
                    result.b += dY.b;
                    result.a += dY.a;
                }

                hP.r += QminusP.r;
                hP.g += QminusP.g;
                hP.b += QminusP.b;
                hP.a += QminusP.a;

                hR.r += SminusR.r;
                hR.g += SminusR.g;
                hR.b += SminusR.b;
                hR.a += SminusR.a;
            }
        }
    }

    static
    void unpackModulations(const PVRTCWord& word, int offsetX, int offsetY, u8 i32ModulationValues[8][16], u8 ui8Bpp)
    {
        u32 WordModMode = word.u32ColorData & 0x1;
        u32 ModulationBits = word.u32ModulationData;

        const u8 modulation_table[] =
        {
            0, 3, 5, 8,
            0, 4, 4 | PUNCHTHROUGH_ALPHA, 8
        };

        // Unpack differently depending on 2bpp or 4bpp modes.
        if (ui8Bpp == 2)
        {
            // WordModeMode: 0 = store, 1 = HV, 2 = H, 3 = V
            if (WordModMode)
            {
                if (ModulationBits & 0x1)
                {
                    WordModMode += ((ModulationBits >> 20) & 1) + 1;
                    ModulationBits = (ModulationBits & ~0x00100000) | ((ModulationBits & 0x00200000) >> 1);
                }

                ModulationBits = (ModulationBits & ~0x00000001) | ((ModulationBits & 0x00000002) >> 1);

                // Store mode in 2 MSB
                WordModMode <<= 6;

                for (int y = 0; y < 4; y++)
                {
                    u8* dest = &i32ModulationValues[y + offsetY][0 + offsetX];
                    const int s = y & 1;
                    for (int x = 0; x < 4; x++)
                    {
                        dest[1 - s] = WordModMode;
                        dest[0 + s] = modulation_table[ModulationBits & 3];
                        dest += 2;
                        ModulationBits >>= 2;
                    }
                }
            }
            else
            {
                // else if direct encoded 2bit mode - i.e. 1 mode bit per pixel
                for (int y = 0; y < 4; y++)
                {
                    u8* dest = &i32ModulationValues[y + offsetY][0 + offsetX];
                    for (int x = 0; x < 8; x++)
                    {
                        dest[x] = (ModulationBits & 1) * 8;
                        ModulationBits >>= 1;
                    }
                }
            }
        }
        else
        {
            const u8* table = modulation_table + WordModMode * 4;
            for (int y = 0; y < 4; y++)
            {
                u8* dest = &i32ModulationValues[y + offsetY][0 + offsetX];
                for (int x = 0; x < 4; x++)
                {
                    dest[x] = table[ModulationBits & 3];
                    ModulationBits >>= 2;
                }
            }
        }
    }

    static
    s32 getModulationValues(u8 i32ModulationValues[8][16], u32 xPos, u32 yPos, u8 bpp)
    {
        int value = i32ModulationValues[yPos][xPos];
        if (bpp == 2)
        {
            const int mode = value >> 6;
            if (mode != 0)
            {
                if (((xPos ^ yPos) & 1) == 0)
                {
                    // stored value
                }
                else if (mode == 3)
                {
                    // else it's V-Only
                    value = (i32ModulationValues[yPos - 1][xPos] +
                             i32ModulationValues[yPos + 1][xPos] + 1) / 2;
                }
                else if (mode == 2)
                {
                    // else if H-Only
                    value = (i32ModulationValues[yPos][xPos - 1] +
                             i32ModulationValues[yPos][xPos + 1] + 1) / 2;
                }
                else
                {
                    // if H&V interpolation...
                    value = (i32ModulationValues[yPos - 1][xPos] +
                             i32ModulationValues[yPos + 1][xPos] +
                             i32ModulationValues[yPos][xPos - 1] +
                             i32ModulationValues[yPos][xPos + 1] + 2) / 4;
                }
            }
        }

        return value & 0xf;
    }

    constexpr int lerp(int a, int b, int mod)
    {
        return a + ((b - a) * mod) / 8;
    }

    static
    void pvrtcGetDecompressedPixels(u8 i32ModulationValues[8][16],
                                    Pixel128S upscaledColorA[32],
                                    Pixel128S upscaledColorB[32],
                                    u8* pColorData, size_t stride,
                                    int xoffset, int yoffset, int width, int height, u8 bpp)
    {
        const u32 ui32WordWidth = (bpp == 2) ? 8 : 4;
        const u32 ui32WordHeight = 4;

        xoffset = xoffset * ui32WordWidth - ui32WordWidth / 2;
        yoffset = yoffset * ui32WordHeight - ui32WordHeight / 2;
        const int xmask = width - 1;
        const int ymask = height - 1;

        for (u32 y = 0; y < ui32WordHeight; ++y)
        {
            const Pixel128S* colorA = upscaledColorA + y * ui32WordWidth;
            const Pixel128S* colorB = upscaledColorB + y * ui32WordWidth;

            const size_t sy = ((yoffset + y) & ymask) * stride;
            Pixel32* dest = reinterpret_cast<Pixel32 *>(pColorData + sy);

            for (u32 x = 0; x < ui32WordWidth; ++x)
            {
                s32 mod = getModulationValues(i32ModulationValues, x + ui32WordWidth / 2, y + ui32WordHeight / 2, bpp);

                s32 r = lerp(colorA[x].r, colorB[x].r, mod);
                s32 g = lerp(colorA[x].g, colorB[x].g, mod);
                s32 b = lerp(colorA[x].b, colorB[x].b, mod);
                s32 a = mod & PUNCHTHROUGH_ALPHA ? 0 : lerp(colorA[x].a, colorB[x].a, mod);

                const int offset = (xoffset + x) & xmask;
                dest[offset].r = u8(r);
                dest[offset].g = u8(g);
                dest[offset].b = u8(b);
                dest[offset].a = u8(a);
            }
        }
    }

    constexpr unsigned int wrapWordIndex(unsigned int numWords, int word)
    {
        //return ((word + numWords) % numWords);
        return word & (numWords - 1); // numWords must be power of two
    }

    static
    void moveModulationValues(u8 i32ModulationValues[8][16], u32 ui32WordWidth, u8 ui8bpp)
    {
        u32* d = (u32*) &i32ModulationValues[0][0];
        u32* s = (u32*) &i32ModulationValues[0][ui32WordWidth];
        for (int i = 0; i < 8; ++i)
        {
            d[0] = s[0];
            if (ui8bpp)
                d[1] = s[1];
            d += 4;
            s += 4;
        }
    }

    static
    void pvrtc_decompress(const u8* pCompressedData, u8* pDecompressedData, size_t stride,
                          u32 ui32Width, u32 ui32Height, u8 ui8Bpp)
    {
        const u32 ui32WordWidth = (ui8Bpp == 2) ? 8 : 4;
        const u32 ui32WordHeight = 4;

        PVRTCWord* pWordMembers = (PVRTCWord*)pCompressedData;

        // Calculate number of words
        int i32NumXWords = (int)(ui32Width / ui32WordWidth);
        int i32NumYWords = (int)(ui32Height / ui32WordHeight);

        // For each row of words
        for (int wordY = 0; wordY < i32NumYWords; wordY++)
        {
            int x0 = i32NumXWords - 1;
            int x1 = 0;
            int y0 = wrapWordIndex(i32NumYWords, wordY - 1);
            int y1 = wrapWordIndex(i32NumYWords, wordY);

            PVRTCWord* P = pWordMembers + u32_interleave_bits(y0, x0);
            PVRTCWord* Q = pWordMembers + u32_interleave_bits(y0, x1);
            PVRTCWord* R = pWordMembers + u32_interleave_bits(y1, x0);
            PVRTCWord* S = pWordMembers + u32_interleave_bits(y1, x1);

            u8 i32ModulationValues[8][16];

            unpackModulations(*P, 0,             0,              i32ModulationValues, ui8Bpp);
            unpackModulations(*Q, ui32WordWidth, 0,              i32ModulationValues, ui8Bpp);
            unpackModulations(*R, 0,             ui32WordHeight, i32ModulationValues, ui8Bpp);
            unpackModulations(*S, ui32WordWidth, ui32WordHeight, i32ModulationValues, ui8Bpp);

            Pixel32 colorA[4];
            Pixel32 colorB[4];

            colorA[0] = getColorA(P->u32ColorData);
            colorA[1] = getColorA(Q->u32ColorData);
            colorA[2] = getColorA(R->u32ColorData);
            colorA[3] = getColorA(S->u32ColorData);
            colorB[0] = getColorB(P->u32ColorData);
            colorB[1] = getColorB(Q->u32ColorData);
            colorB[2] = getColorB(R->u32ColorData);
            colorB[3] = getColorB(S->u32ColorData);

            // for each column of words
            for (int wordX = 0; wordX < i32NumXWords; wordX++)
            {
                Pixel128S upscaledColorA[32];
                Pixel128S upscaledColorB[32];
 
                // Bilinear upscale image data from 2x2 -> 4x4
                interpolateColors(colorA, upscaledColorA, ui8Bpp);
                interpolateColors(colorB, upscaledColorB, ui8Bpp);
 
                // assemble 4 words into struct to get decompressed pixels from
                pvrtcGetDecompressedPixels(i32ModulationValues, upscaledColorA, upscaledColorB,
                                           pDecompressedData, stride, wordX, wordY, ui32Width, ui32Height, ui8Bpp);
 
                x1 = wrapWordIndex(i32NumXWords, wordX + 1);
 
                P = Q;
                R = S;
                Q = pWordMembers + u32_interleave_bits(y0, x1);
                S = pWordMembers + u32_interleave_bits(y1, x1);

                moveModulationValues(i32ModulationValues, ui32WordWidth, ui8Bpp);
                unpackModulations(*Q, ui32WordWidth, 0,              i32ModulationValues, ui8Bpp);
                unpackModulations(*S, ui32WordWidth, ui32WordHeight, i32ModulationValues, ui8Bpp);

                colorA[0] = colorA[1];
                colorA[1] = getColorA(Q->u32ColorData);
                colorA[2] = colorA[3];
                colorA[3] = getColorA(S->u32ColorData);

                colorB[0] = colorB[1];
                colorB[1] = getColorB(Q->u32ColorData);
                colorB[2] = colorB[3];
                colorB[3] = getColorB(S->u32ColorData);
            }
        }
    }

	// ----------------------------------------------------------------------------
    // pvrtc2
    // ----------------------------------------------------------------------------

    // References:
    // https://s3.amazonaws.com/pvr-sdk-live/sdk-documentation/PVRTC%20Specification%20and%20User%20Guide.pdf
    // http://sv-journal.org/2014-1/06/en/index.php?lang=en#7-3

    constexpr u32 pvrtc2_extend(u32 value, int from, int to)
    {
        return value * ((1 << to) - 1) / ((1 << from) - 1);
    }

#if 0 // TODO: should use these (specification conforming)
    constexpr u32 pvrtc2_alpha0(u32 alpha)
    {
        alpha = (alpha << 1) | 0;
        return (alpha << 4) | alpha;
    }

    constexpr u32 pvrtc2_alpha1(u32 alpha)
    {
        alpha = (alpha << 1) | 1;
        return (alpha << 4) | alpha;
    }
#endif

    static inline
    Color pvrtc2_1bit_lerp(Color a, Color b, int mod)
    {
        return mod ? b : a;
    }

    static inline
    Color pvrtc2_2bit_lerp(Color a, Color b, int mod)
    {
        // TODO: specification
        // mod(0): a
        // mod(1): (a*5 + b*3) / 8
        // mod(2): (a*3 + b*5) / 8
        // mod(3): b
        Color c;

        switch (mod)
        {
            case 0:
                c = a;
                break;
            case 1:
                c.r = (a.r * 5 + b.r * 3) / 8;
                c.g = (a.g * 5 + b.g * 3) / 8;
                c.b = (a.b * 5 + b.b * 3) / 8;
                c.a = (a.a * 5 + b.a * 3) / 8;
                break;
            case 2:
                c.r = (a.r * 3 + b.r * 5) / 8;
                c.g = (a.g * 3 + b.g * 5) / 8;
                c.b = (a.b * 3 + b.b * 5) / 8;
                c.a = (a.a * 3 + b.a * 5) / 8;
                break;
            case 3:
                c = b;
                break;
        }

        return c;
    }

    static inline
    Color pvrtc2_punch(Color a, Color b, int mod)
    {
        Color c;
        switch (mod)
        {
            case 0:
                c = a;
                break;
            case 1:
                c.r = (a.r + b.r) / 2;
                c.g = (a.g + b.g) / 2;
                c.b = (a.b + b.b) / 2;
                c.a = (a.a + b.a) / 2;
                break;
            case 2:
                // punch-through
                c = Color(0, 0, 0, 0);
                break;
            case 3:
                c = b;
                break;
        }
        return c;
    }

    struct BlockPVRTC2
    {
        Color a;
        Color b;

        // --------------------------------------------
        // hard   mode    decoder
        // --------------------------------------------
        //   0      0     bilinear
        //   0      1     punch-through alpha
        //   1      0     non-interpolated
        //   1      1     local palette

        u32 mode;
        u32 hard;
        u32 modulation;
        
        BlockPVRTC2()
        {
        }

        BlockPVRTC2(const u8* data)
        {
            modulation = uload32le(data + 0);
            u32 packed = uload32le(data + 4);

            mode = packed & 0x0001;
            hard = packed & 0x8000;

            u32 opacity = packed & 0x80000000;

            if (opacity)
            {
                a.b = pvrtc2_extend((packed >>  1) & 0x0f, 4, 8);
                a.g = pvrtc2_extend((packed >>  5) & 0x1f, 5, 8);
                a.r = pvrtc2_extend((packed >> 10) & 0x1f, 5, 8);
                a.a = 0xff;

                b.b = pvrtc2_extend((packed >> 16) & 0x1f, 5, 8);
                b.g = pvrtc2_extend((packed >> 21) & 0x1f, 5, 8);
                b.r = pvrtc2_extend((packed >> 26) & 0x1f, 5, 8);
                b.a = 0xff;
            }
            else
            {
                a.b = pvrtc2_extend((packed >>  1) & 0x07, 3, 8);
                a.g = pvrtc2_extend((packed >>  4) & 0x0f, 4, 8);
                a.r = pvrtc2_extend((packed >>  8) & 0x0f, 4, 8);
                //a.a = pvrtc2_alpha1((packed >> 12) & 0x07);
                a.a = pvrtc2_extend((packed >> 12) & 0x07, 3, 8);

                b.b = pvrtc2_extend((packed >> 16) & 0xf, 4, 8);
                b.g = pvrtc2_extend((packed >> 20) & 0xf, 4, 8);
                b.r = pvrtc2_extend((packed >> 24) & 0xf, 4, 8);
                //b.a = pvrtc2_alpha0((packed >> 28) & 0x7);
                b.a = pvrtc2_extend((packed >> 28) & 0x7, 3, 8);
            }
        }

        BlockPVRTC2(const BlockPVRTC2& block)
        {
            a = block.a;
            b = block.b;
            mode = block.mode;
            hard = block.hard;
            modulation = block.modulation;
        }
    };

    template <int width, int scale>
    void pvrtc2_bilinear(Color* a, Color* b, int u, int v, const BlockPVRTC2* blocks)
    {
        for (int y = 0; y < 2; ++y)
        {
            int w0 = (width * 2 - u) * (4 - v);
            int w1 = u * (4 - v);
            int w2 = (width * 2 - u) * v;
            int w3 = u * v;

            for (int x = 0; x < width; ++x)
            {
                a->r = (blocks[0].a.r * w0 + blocks[1].a.r * w1 + blocks[2].a.r * w2 + blocks[3].a.r * w3) >> scale;
                a->g = (blocks[0].a.g * w0 + blocks[1].a.g * w1 + blocks[2].a.g * w2 + blocks[3].a.g * w3) >> scale;
                a->b = (blocks[0].a.b * w0 + blocks[1].a.b * w1 + blocks[2].a.b * w2 + blocks[3].a.b * w3) >> scale;
                a->a = (blocks[0].a.a * w0 + blocks[1].a.a * w1 + blocks[2].a.a * w2 + blocks[3].a.a * w3) >> scale;
                ++a;

                b->r = (blocks[0].b.r * w0 + blocks[1].b.r * w1 + blocks[2].b.r * w2 + blocks[3].b.r * w3) >> scale;
                b->g = (blocks[0].b.g * w0 + blocks[1].b.g * w1 + blocks[2].b.g * w2 + blocks[3].b.g * w3) >> scale;
                b->b = (blocks[0].b.b * w0 + blocks[1].b.b * w1 + blocks[2].b.b * w2 + blocks[3].b.b * w3) >> scale;
                b->a = (blocks[0].b.a * w0 + blocks[1].b.a * w1 + blocks[2].b.a * w2 + blocks[3].b.a * w3) >> scale;
                ++b;

                w0 -= (4 - v);
                w1 += (4 - v);
                w2 -= v;
                w3 += v;
            }

            ++v;
        }
    }

    void pvrtc2_2bit_bilinear(u8* image, size_t stride, int u, int v, const BlockPVRTC2* blocks, u32 modulation)
    {
        Color a[8];
        Color b[8];
        pvrtc2_bilinear<4, 5>(a, b, u, v, blocks);

        u32* scan = reinterpret_cast<u32*>(image);

        scan[0] = pvrtc2_1bit_lerp(a[0], b[0], (modulation >>  0) & 1);
        scan[1] = pvrtc2_1bit_lerp(a[1], b[1], (modulation >>  1) & 1);
        scan[2] = pvrtc2_1bit_lerp(a[2], b[2], (modulation >>  2) & 1);
        scan[3] = pvrtc2_1bit_lerp(a[3], b[3], (modulation >>  3) & 1);

        scan = reinterpret_cast<u32*>(image + stride);

        scan[0] = pvrtc2_1bit_lerp(a[4], b[4], (modulation >>  8) & 1);
        scan[1] = pvrtc2_1bit_lerp(a[5], b[5], (modulation >>  9) & 1);
        scan[2] = pvrtc2_1bit_lerp(a[6], b[6], (modulation >> 10) & 1);
        scan[3] = pvrtc2_1bit_lerp(a[7], b[7], (modulation >> 11) & 1);
    }

    void pvrtc2_2bit_bilinear(u8* image, size_t stride, int u, int v, const BlockPVRTC2* blocks, const u8* modulation)
    {
        Color a[8];
        Color b[8];
        pvrtc2_bilinear<4, 5>(a, b, u, v, blocks);

        u32* scan = reinterpret_cast<u32*>(image);

        scan[0] = pvrtc2_2bit_lerp(a[0], b[0], modulation[0]);
        scan[1] = pvrtc2_2bit_lerp(a[1], b[1], modulation[1]);
        scan[2] = pvrtc2_2bit_lerp(a[2], b[2], modulation[2]);
        scan[3] = pvrtc2_2bit_lerp(a[3], b[3], modulation[3]);

        scan = reinterpret_cast<u32*>(image + stride);

        scan[0] = pvrtc2_2bit_lerp(a[4], b[4], modulation[4]);
        scan[1] = pvrtc2_2bit_lerp(a[5], b[5], modulation[5]);
        scan[2] = pvrtc2_2bit_lerp(a[6], b[6], modulation[6]);
        scan[3] = pvrtc2_2bit_lerp(a[7], b[7], modulation[7]);
    }

    void pvrtc2_2bit_nearest(u8* image, size_t stride, Color a, Color b, u32 modulation)
    {
        u32* scan = reinterpret_cast<u32*>(image);

        scan[0] = pvrtc2_1bit_lerp(a, b, (modulation >>  0) & 1);
        scan[1] = pvrtc2_1bit_lerp(a, b, (modulation >>  1) & 1);
        scan[2] = pvrtc2_1bit_lerp(a, b, (modulation >>  2) & 1);
        scan[3] = pvrtc2_1bit_lerp(a, b, (modulation >>  3) & 1);

        scan = reinterpret_cast<u32*>(image + stride);

        scan[0] = pvrtc2_1bit_lerp(a, b, (modulation >>  8) & 1);
        scan[1] = pvrtc2_1bit_lerp(a, b, (modulation >>  9) & 1);
        scan[2] = pvrtc2_1bit_lerp(a, b, (modulation >> 10) & 1);
        scan[3] = pvrtc2_1bit_lerp(a, b, (modulation >> 11) & 1);
    }

    void pvrtc2_2bit_nearest(u8* image, size_t stride, Color a, Color b, const u8* modulation)
    {
        u32* scan = reinterpret_cast<u32*>(image);

        scan[0] = pvrtc2_2bit_lerp(a, b, modulation[0]);
        scan[1] = pvrtc2_2bit_lerp(a, b, modulation[1]);
        scan[2] = pvrtc2_2bit_lerp(a, b, modulation[2]);
        scan[3] = pvrtc2_2bit_lerp(a, b, modulation[3]);

        scan = reinterpret_cast<u32*>(image + stride);

        scan[0] = pvrtc2_2bit_lerp(a, b, modulation[4]);
        scan[1] = pvrtc2_2bit_lerp(a, b, modulation[5]);
        scan[2] = pvrtc2_2bit_lerp(a, b, modulation[6]);
        scan[3] = pvrtc2_2bit_lerp(a, b, modulation[7]);
    }

    void pvrtc2_4bit_bilinear(u8* image, size_t stride, int u, int v, const BlockPVRTC2* blocks, u32 modulation)
    {
        Color a[4];
        Color b[4];
        pvrtc2_bilinear<2, 4>(a, b, u, v, blocks);

        u32* scan = reinterpret_cast<u32*>(image);

        scan[0] = pvrtc2_2bit_lerp(a[0], b[0], (modulation >>  0) & 3);
        scan[1] = pvrtc2_2bit_lerp(a[1], b[1], (modulation >>  2) & 3);

        scan = reinterpret_cast<u32*>(image + stride);

        scan[0] = pvrtc2_2bit_lerp(a[2], b[2], (modulation >>  8) & 3);
        scan[1] = pvrtc2_2bit_lerp(a[3], b[3], (modulation >> 10) & 3);
    }

    void pvrtc2_4bit_punchthrough(u8* image, size_t stride, int u, int v, const BlockPVRTC2* blocks, u32 modulation)
    {
        Color a[4];
        Color b[4];
        pvrtc2_bilinear<2, 4>(a, b, u, v, blocks);

        u32* scan = reinterpret_cast<u32*>(image);

        scan[0] = pvrtc2_punch(a[0], b[0], (modulation >>  0) & 3);
        scan[1] = pvrtc2_punch(a[1], b[1], (modulation >>  2) & 3);

        scan = reinterpret_cast<u32*>(image + stride);

        scan[0] = pvrtc2_punch(a[2], b[2], (modulation >>  8) & 3);
        scan[1] = pvrtc2_punch(a[3], b[3], (modulation >> 10) & 3);
    }

    void pvrtc2_4bit_nearest(u8* image, size_t stride, Color a, Color b, u32 modulation)
    {
        u32* scan = reinterpret_cast<u32*>(image);

        scan[0] = pvrtc2_2bit_lerp(a, b, (modulation >>  0) & 3);
        scan[1] = pvrtc2_2bit_lerp(a, b, (modulation >>  2) & 3);

        scan = reinterpret_cast<u32*>(image + stride);

        scan[0] = pvrtc2_2bit_lerp(a, b, (modulation >>  8) & 3);
        scan[1] = pvrtc2_2bit_lerp(a, b, (modulation >> 10) & 3);
    }

    void pvrtc2_4bit_palette(u8* image, size_t stride, int index, const BlockPVRTC2* blocks, u32 modulation)
    {
        Color palette[16];

        switch (index)
        {
            case 0:
                palette[ 0] = blocks[0].a;
                palette[ 1].r = (blocks[0].a.r * 5 + blocks[0].b.r * 3) / 8;
                palette[ 1].g = (blocks[0].a.g * 5 + blocks[0].b.g * 3) / 8;
                palette[ 1].b = (blocks[0].a.b * 5 + blocks[0].b.b * 3) / 8;
                palette[ 1].a = (blocks[0].a.a * 5 + blocks[0].b.a * 3) / 8;
                palette[ 2].r = (blocks[0].a.r * 3 + blocks[0].b.r * 5) / 8;
                palette[ 2].g = (blocks[0].a.g * 3 + blocks[0].b.g * 5) / 8;
                palette[ 2].b = (blocks[0].a.b * 3 + blocks[0].b.b * 5) / 8;
                palette[ 2].a = (blocks[0].a.a * 3 + blocks[0].b.a * 5) / 8;
                palette[ 3] = blocks[0].b;

                palette[ 4] = blocks[0].a;
                palette[ 5] = blocks[0].b;
                palette[ 6] = blocks[1].a;
                palette[ 7] = blocks[1].b;

                palette[ 8] = blocks[0].a;
                palette[ 9] = blocks[0].b;
                palette[10] = blocks[2].a;
                palette[11] = blocks[2].b;

                palette[12] = blocks[0].a;
                palette[13] = blocks[0].b;
                palette[14] = blocks[1].a;
                palette[15] = blocks[2].b;
                break;
            case 1:
                palette[ 0] = blocks[0].a;
                palette[ 1] = blocks[0].b;
                palette[ 2] = blocks[1].a;
                palette[ 3] = blocks[1].b;

                palette[ 4] = blocks[0].a;
                palette[ 5] = blocks[0].b;
                palette[ 6] = blocks[1].a;
                palette[ 7] = blocks[1].b;

                palette[ 8] = blocks[0].a;
                palette[ 9] = blocks[0].b;
                palette[10] = blocks[1].a;
                palette[11] = blocks[1].b;

                palette[12] = blocks[3].a;
                palette[13] = blocks[0].b;
                palette[14] = blocks[1].a;
                palette[15] = blocks[1].b;
                break;
            case 2:
                palette[ 0] = blocks[0].a;
                palette[ 1] = blocks[0].b;
                palette[ 2] = blocks[2].a;
                palette[ 3] = blocks[2].b;

                palette[ 4] = blocks[0].a;
                palette[ 5] = blocks[0].b;
                palette[ 6] = blocks[2].a;
                palette[ 7] = blocks[2].b;

                palette[ 8] = blocks[0].a;
                palette[ 9] = blocks[0].b;
                palette[10] = blocks[2].a;
                palette[11] = blocks[2].b;

                palette[12] = blocks[0].a;
                palette[13] = blocks[3].b;
                palette[14] = blocks[2].a;
                palette[15] = blocks[2].b;
                break;
            case 3:
                palette[ 0] = blocks[0].a;
                palette[ 1] = blocks[3].b;
                palette[ 2] = blocks[2].a;
                palette[ 3] = blocks[1].b;

                palette[ 4] = blocks[3].a;
                palette[ 5] = blocks[3].b;
                palette[ 6] = blocks[1].a;
                palette[ 7] = blocks[1].b;

                palette[ 8] = blocks[3].a;
                palette[ 9] = blocks[3].b;
                palette[10] = blocks[2].a;
                palette[11] = blocks[2].b;

                palette[12] = blocks[3].a;
                palette[13] = blocks[3].b;
                palette[14] = blocks[2].a;
                palette[15] = blocks[1].b;
                break;
        }

        u32* scan = reinterpret_cast<u32*>(image);

        scan[0] = palette[((modulation >>  0) & 3) + 0];
        scan[1] = palette[((modulation >>  2) & 3) + 4];

        scan = reinterpret_cast<u32*>(image + stride);

        scan[0] = palette[((modulation >>  8) & 3) + 8];
        scan[1] = palette[((modulation >> 10) & 3) + 12];
    }

    static
    u32 unpackModulation(const BlockPVRTC2& block, u8* values)
    {
        u32 mode = block.mode;
        u32 modulation = block.modulation;

        if (mode)
        {
            if (modulation & 0x1)
            {
                if (modulation & 0x00100000)
                {
                    // V
                    mode = 3;
                }
                else
                {
                    // H
                    mode = 2;
                }

                modulation = (modulation & 0xffefffff) | ((modulation & 0x00200000) >> 1);
            }

            modulation = (modulation & 0xfffffffe) | ((modulation & 0x00000002) >> 1);

            // x-x-x-x-     x  2 bit modulation value
            // -x-x-x-x     -  filled later with interpolation
            // x-x-x-x-
            // -x-x-x-x

            // fill the checkerboard pattern
            for (int y = 0; y < 4; ++y)
            {
                for (int x = y & 1; x < 8; x += 2)
                {
                    values[x] = modulation & 3;
                    modulation >>= 2;
                }

                values += 16;
            }
        }
        else
        {
            // simple encoding (1 bit modulation values)
            for (int y = 0; y < 4; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    values[x] = (modulation & 1) * 3;
                    modulation >>= 1;
                }

                values += 16;
            }
        }

        return mode;
    }

    static
    void resolveModulation(u8* modulation, const u8* values, u32 mode)
    {
        constexpr int width = 4;
        constexpr int height = 2;

        for (int y = 0; y < height; ++y)
        {
            // fill the checkerboard pattern
            for (int x = y & 1; x < width; x += 2)
            {
                // stored value
                modulation[x] = values[x];
            }

            // fill the gaps in the checkerboard pattern
            for (int x = (y + 1) & 1; x < width; x += 2)
            {
                switch (mode)
                {
                    case 0:
                        // simple encoding (1 bit modulation values)
                        modulation[x] = values[x];
                        break;
                    case 1:
                        // HV
                        modulation[x] = (values[x - 1] + values[x + 1] + values[x - 16] + values[x + 16] + 2) / 4;
                        break;
                    case 2:
                        // H
                        modulation[x] = (values[x - 1] + values[x + 1] + 1) / 2;
                        break;
                    case 3:
                        // V
                        modulation[x] = (values[x - 16] + values[x + 16] + 1) / 2;
                        break;
                }
            }

            values += 16;
            modulation += 4;
        }
    }

    void pvrtc2_2bit_decompress(const u8* data, u8* image, size_t stride, u32 width, u32 height)
    {
        constexpr int block_width = 8;
        constexpr int block_height = 4;
        constexpr int quad_width = block_width / 2;
        constexpr int quad_height = block_height / 2;
        const u32 xblocks = ceil_div(width, block_width);
        const u32 yblocks = ceil_div(height, block_height);

        for (u32 y0 = 0; y0 < yblocks; ++y0)
        {
            u32 y1 = (y0 + 1) % yblocks;

            BlockPVRTC2 blocks[4];

            blocks[0] = BlockPVRTC2(data + (y0 * xblocks + 0) * 8);
            blocks[2] = BlockPVRTC2(data + (y1 * xblocks + 0) * 8);

            u8 values[128];
            u32 modes[4];

            modes[0] = unpackModulation(blocks[0], values);
            modes[2] = unpackModulation(blocks[2], values + block_height * 16);

            for (u32 x0 = 0; x0 < xblocks; ++x0)
            {
                u32 x1 = (x0 + 1) % xblocks;

                blocks[1] = BlockPVRTC2(data + (y0 * xblocks + x1) * 8);
                blocks[3] = BlockPVRTC2(data + (y1 * xblocks + x1) * 8);

                modes[1] = unpackModulation(blocks[1], values + block_width);
                modes[3] = unpackModulation(blocks[3], values + block_width + block_height * 16);

                const u8* block_values = values + quad_height * 16 + quad_width;

                for (int y = 0; y < 2; ++y)
                {
                    for (int x = 0; x < 2; ++x)
                    {
                        int index = y * 2 + x;
                        const BlockPVRTC2& block = blocks[index];

                        u8* scan = image + ((y0 * block_height + (y + 1) * quad_height) % height) * stride + 
                                           ((x0 * block_width  + (x + 1) * quad_width) % width) * 4;

                        if (block.mode)
                        {
                            u8 modulation[8];
                            resolveModulation(modulation, block_values + y * 32 + x * 4, modes[index]);

                            if (blocks[0].hard)
                            {
                                pvrtc2_2bit_nearest(scan, stride, block.a, block.b, modulation);
                            }
                            else
                            {
                                pvrtc2_2bit_bilinear(scan, stride, x * 4, y * 2, blocks, modulation);
                            }
                        }
                        else
                        {
                            constexpr int mods[] =
                            {
                                20, 16, 4, 0
                            };

                            u32 modulation = block.modulation >> mods[index];

                            if (blocks[0].hard)
                            {
                                pvrtc2_2bit_nearest(scan, stride, block.a, block.b, modulation);
                            }
                            else
                            {
                                pvrtc2_2bit_bilinear(scan, stride, x * 4, y * 2, blocks, modulation);
                            }
                        }
                    }
                }

                blocks[0] = blocks[1];
                blocks[2] = blocks[3];

                modes[0] = modes[1];
                modes[2] = modes[3];

                for (int y = 0; y < 8; ++y)
                {
                    std::memcpy(values + y * 16, values + y * 16 + 8, 8);
                }
            }
        }
    }

    void pvrtc2_4bit_decompress(const u8* data, u8* image, size_t stride, u32 width, u32 height)
    {
        const u32 block_width = 4;
        const u32 block_height = 4;
        const u32 quad_width = block_width / 2;
        const u32 quad_height = block_height / 2;
        const u32 xblocks = ceil_div(width, block_width);
        const u32 yblocks = ceil_div(height, block_height);

        for (u32 y0 = 0; y0 < yblocks; ++y0)
        {
            u32 y1 = (y0 + 1) % yblocks;

            BlockPVRTC2 blocks[4];

            blocks[0] = BlockPVRTC2(data + (y0 * xblocks + 0) * 8);
            blocks[2] = BlockPVRTC2(data + (y1 * xblocks + 0) * 8);

            for (u32 x0 = 0; x0 < xblocks; ++x0)
            {
                u32 x1 = (x0 + 1) % xblocks;

                blocks[1] = BlockPVRTC2(data + (y0 * xblocks + x1) * 8);
                blocks[3] = BlockPVRTC2(data + (y1 * xblocks + x1) * 8);

                for (int y = 0; y < 2; ++y)
                {
                    for (int x = 0; x < 2; ++x)
                    {
                        int index = y * 2 + x;
                        const BlockPVRTC2& block = blocks[index];

                        constexpr int mods[] =
                        {
                            20, 16, 4, 0
                        };
                        u32 modulation = block.modulation >> mods[index];

                        u8* scan = image + ((y0 * block_height + (y + 1) * quad_height) % height) * stride + 
                                           ((x0 * block_width  + (x + 1) * quad_width) % width) * 4;

                        if (blocks[0].hard)
                        {
                            if (block.mode)
                            {
                                pvrtc2_4bit_palette(scan, stride, index, blocks, modulation);
                            }
                            else
                            {
                                pvrtc2_4bit_nearest(scan, stride, block.a, block.b, modulation);
                            }
                        }
                        else
                        {
                            if (block.mode)
                            {
                                pvrtc2_4bit_punchthrough(scan, stride, x * 2, y * 2, blocks, modulation);
                            }
                            else
                            {
                                pvrtc2_4bit_bilinear(scan, stride, x * 2, y * 2, blocks, modulation);
                            }
                        }
                    }
                }

                blocks[0] = blocks[1];
                blocks[2] = blocks[3];
            }
        }
    }

} // namespace

namespace mango::image
{

    void decode_surface_pvrtc(const TextureCompressionInfo& info, u8* out, const u8* in, size_t stride)
    {
#if 0
        if (info.width < 8 || info.heigbt < 8)
        {
            return;
        }

        if (info.width != info.height)
        {
            return;
        }

        if (!u32_is_power_of_two(info.width))
        {
            return;
        }
#endif

        u8 bpp = 0;

        switch (info.compression)
        {
            case TextureCompression::PVRTC_RGB_2BPP:
            case TextureCompression::PVRTC_RGBA_2BPP:
            case TextureCompression::PVRTC_SRGB_2BPP:
            case TextureCompression::PVRTC_SRGB_ALPHA_2BPP:
                bpp = 2;
                break;
            case TextureCompression::PVRTC_RGB_4BPP:
            case TextureCompression::PVRTC_RGBA_4BPP:
            case TextureCompression::PVRTC_SRGB_4BPP:
            case TextureCompression::PVRTC_SRGB_ALPHA_4BPP:
                bpp = 4;
                break;
            default:
                // Incorrect compression
                return;
        }

        pvrtc_decompress(in, out, stride, info.width, info.height, bpp);
    }

    void decode_surface_pvrtc2(const TextureCompressionInfo& info, u8* out, const u8* in, size_t stride)
    {
        switch (info.compression)
        {
            case TextureCompression::PVRTC2_RGBA_2BPP:
                pvrtc2_2bit_decompress(in, out, stride, info.width, info.height);
                break;
            case TextureCompression::PVRTC2_RGBA_4BPP:
                pvrtc2_4bit_decompress(in, out, stride, info.width, info.height);
                break;
            default:
                // Incorrect compression
                return;
        }
    }

} // namespace mango::image
