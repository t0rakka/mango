/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/image/compression.hpp>

namespace
{
    using namespace mango;

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

    static void interpolateColors(Pixel32 color[4], Pixel128S* pPixel, u8 ui8Bpp)
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
        hP.r *=	ui32WordWidth;
        hP.g *=	ui32WordWidth;
        hP.b *=	ui32WordWidth;
        hP.a *=	ui32WordWidth;
        hR.r *=	ui32WordWidth;
        hR.g *=	ui32WordWidth;
        hR.b *=	ui32WordWidth;
        hR.a *=	ui32WordWidth;

        if (ui8Bpp == 2)
        {
            //Loop through pixels to achieve results.
            for (unsigned int x = 0; x < ui32WordWidth; x++)
            {
                Pixel128S result = { 4 * hP.r, 4 * hP.g, 4 * hP.b, 4 * hP.a };
                Pixel128S dY = { hR.r - hP.r, hR.g - hP.g, hR.b - hP.b, hR.a - hP.a };

                for (unsigned int y = 0; y < ui32WordHeight; y++)
                {
                    pPixel[y * ui32WordWidth + x].r = s32((result.r >> 7) + (result.r >> 2));
                    pPixel[y * ui32WordWidth + x].g = s32((result.g >> 7) + (result.g >> 2));
                    pPixel[y * ui32WordWidth + x].b = s32((result.b >> 7) + (result.b >> 2));
                    pPixel[y * ui32WordWidth + x].a = s32((result.a >> 5) + (result.a >> 1));

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
            for (unsigned int y = 0; y < ui32WordHeight; y++)
            {
                Pixel128S result = { 4 * hP.r, 4 * hP.g, 4 * hP.b, 4 * hP.a };
                Pixel128S dY = { hR.r - hP.r, hR.g - hP.g, hR.b - hP.b, hR.a - hP.a };

                for (unsigned int x = 0; x < ui32WordWidth; x++)
                {
                    pPixel[x * ui32WordWidth + y].r = s32((result.r >> 6) + (result.r >> 1));
                    pPixel[x * ui32WordWidth + y].g = s32((result.g >> 6) + (result.g >> 1));
                    pPixel[x * ui32WordWidth + y].b = s32((result.b >> 6) + (result.b >> 1));
                    pPixel[x * ui32WordWidth + y].a = s32((result.a >> 4) + (result.a));

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

    static void unpackModulations(const PVRTCWord& word, int offsetX, int offsetY, u8 i32ModulationValues[8][16], u8 ui8Bpp)
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
                        dest[1-s] = WordModMode;
                        dest[0+s] = modulation_table[ModulationBits & 3];
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

    static s32 getModulationValues(u8 i32ModulationValues[8][16], u32 xPos, u32 yPos, u8 ui8Bpp)
    {
        int value = i32ModulationValues[yPos][xPos];
        if (ui8Bpp == 2)
        {
            const int mode = value >> 6;
            if (mode == 0)
            {
                value &= 0x0f;
            }
            else
            {
                if (((xPos ^ yPos) & 1) == 0)
                {
                    // if this is a stored value
                    value &= 0x0f;
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

        return value;
    }

    constexpr int lerp(int a, int b, int mod)
    {
        return a + ((b - a) * mod) / 8;
    }

    static void pvrtcGetDecompressedPixels(u8 i32ModulationValues[8][16],
                                           Pixel128S upscaledColorA[32],
                                           Pixel128S upscaledColorB[32],
                                           u8* pColorData, size_t stride,
                                           int xoffset, int yoffset, int width, int height,
                                           u8 ui8Bpp)
    {
        const u32 ui32WordWidth = (ui8Bpp == 2) ? 8 : 4;
        const u32 ui32WordHeight = 4;
        
        xoffset = xoffset * ui32WordWidth - ui32WordWidth / 2;
        yoffset = yoffset * ui32WordHeight - ui32WordHeight / 2;
        const int xmask = width - 1;
        const int ymask = height - 1;

        for (unsigned int y = 0; y < ui32WordHeight; y++)
        {
            const Pixel128S* colorA = upscaledColorA + y * ui32WordWidth;
            const Pixel128S* colorB = upscaledColorB + y * ui32WordWidth;

            const size_t sy = ((yoffset + y) & ymask) * stride;
            Pixel32* dest = reinterpret_cast<Pixel32 *>(pColorData + sy);

            for (unsigned int x = 0; x < ui32WordWidth; x++)
            {
                s32 mod = getModulationValues(i32ModulationValues, x + ui32WordWidth / 2, y + ui32WordHeight / 2, ui8Bpp);
                mod &= 0xf;

                Pixel128S result;
                result.r = lerp(colorA[x].r, colorB[x].r, mod);
                result.g = lerp(colorA[x].g, colorB[x].g, mod);
                result.b = lerp(colorA[x].b, colorB[x].b, mod);
                result.a = mod & PUNCHTHROUGH_ALPHA ? 0 : lerp(colorA[x].a, colorB[x].a, mod);

                const int offset = (xoffset + x) & xmask;
                dest[offset].r = u8(result.r);
                dest[offset].g = u8(result.g);
                dest[offset].b = u8(result.b);
                dest[offset].a = u8(result.a);
            }
        }
    }

    constexpr unsigned int wrapWordIndex(unsigned int numWords, int word)
    {
        //return ((word + numWords) % numWords);
        return word & (numWords - 1); // numWords must be power of two
    }

    static void moveModulationValues(u8 i32ModulationValues[8][16], u32 ui32WordWidth, u8 ui8bpp)
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

    static void pvrtcDecompress(const u8* pCompressedData,
                                u8* pDecompressedData,
                                size_t stride,
                                u32 ui32Width,
                                u32 ui32Height,
                                u8 ui8Bpp)
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

            unpackModulations(*P, 0, 0,                          i32ModulationValues, ui8Bpp);
            unpackModulations(*Q, ui32WordWidth, 0,              i32ModulationValues, ui8Bpp);
            unpackModulations(*R, 0, ui32WordHeight,             i32ModulationValues, ui8Bpp);
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

} // namespace

namespace mango::image
{

    void decode_block_pvrtc(const TextureCompressionInfo& info, u8* out, const u8* in, size_t stride)
    {
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
                // incorrect compression
                return;
        }

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
        pvrtcDecompress(in, out, stride, info.width, info.height, bpp);
    }

} // namespace mango::image
