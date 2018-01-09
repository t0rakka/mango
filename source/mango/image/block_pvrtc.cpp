/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        uint8 red, green, blue, alpha;
    };
    
    struct Pixel128S
    {
        int32 red, green, blue, alpha;
    };
    
    struct PVRTCWord
    {
        uint32 u32ModulationData;
        uint32 u32ColorData;
    };
    
    static Pixel32 getColorA(uint32 u32ColorData)
    {
        Pixel32 color;
        
        if ((u32ColorData & 0x8000) != 0)
        {
            // Opaque Color Mode - RGB 554
            color.red   = (uint8)((u32ColorData & 0x7c00) >> 10); // 5->5 bits
            color.green = (uint8)((u32ColorData & 0x3e0)  >> 5); // 5->5 bits
            color.blue  = (uint8)(u32ColorData  & 0x1e) | ((u32ColorData & 0x1e) >> 4); // 4->5 bits
            color.alpha = (uint8)0xf;// 0->4 bits
        }
        else
        {
            // Transparent Color Mode - ARGB 3443
            color.red   = (uint8)((u32ColorData & 0xf00)  >> 7) | ((u32ColorData & 0xf00) >> 11); // 4->5 bits
            color.green = (uint8)((u32ColorData & 0xf0)   >> 3) | ((u32ColorData & 0xf0)  >> 7); // 4->5 bits
            color.blue  = (uint8)((u32ColorData & 0xe)    << 1) | ((u32ColorData & 0xe)   >> 2); // 3->5 bits
            color.alpha = (uint8)((u32ColorData & 0x7000) >> 11);// 3->4 bits - note 0 at right
        }
        
        return color;
    }
    
    static Pixel32 getColorB(uint32 u32ColorData)
    {
        Pixel32 color;
        
        if (u32ColorData & 0x80000000)
        {
            // Opaque Color Mode - RGB 555
            color.red   = (uint8)((u32ColorData & 0x7c000000) >> 26); // 5->5 bits
            color.green = (uint8)((u32ColorData & 0x3e00000)  >> 21); // 5->5 bits
            color.blue  = (uint8)((u32ColorData & 0x1f0000)   >> 16); // 5->5 bits
            color.alpha = (uint8)0xf;// 0 bits
        }
        else
        {
            // Transparent Color Mode - ARGB 3444
            color.red   = (uint8)(((u32ColorData & 0xf000000)  >> 23) | ((u32ColorData & 0xf000000) >> 27)); // 4->5 bits
            color.green = (uint8)(((u32ColorData & 0xf00000)   >> 19) | ((u32ColorData & 0xf00000)  >> 23)); // 4->5 bits
            color.blue  = (uint8)(((u32ColorData & 0xf0000)    >> 15) | ((u32ColorData & 0xf0000)   >> 19)); // 4->5 bits
            color.alpha = (uint8)((u32ColorData & 0x70000000) >> 27);// 3->4 bits - note 0 at right
        }
        
        return color;
    }
    
    static void interpolateColors(Pixel32 color[4], Pixel128S* pPixel, uint8 ui8Bpp)
    {
        const uint32 ui32WordWidth = (ui8Bpp == 2) ? 8 : 4;
        const uint32 ui32WordHeight = 4;
        
        //Convert to int 32.
        Pixel128S hP = { (int32)color[0].red, (int32)color[0].green, (int32)color[0].blue, (int32)color[0].alpha };
        Pixel128S hQ = { (int32)color[1].red, (int32)color[1].green, (int32)color[1].blue, (int32)color[1].alpha };
        Pixel128S hR = { (int32)color[2].red, (int32)color[2].green, (int32)color[2].blue, (int32)color[2].alpha };
        Pixel128S hS = { (int32)color[3].red, (int32)color[3].green, (int32)color[3].blue, (int32)color[3].alpha };
        
        //Get vectors.
        Pixel128S QminusP = {hQ.red - hP.red, hQ.green - hP.green, hQ.blue - hP.blue, hQ.alpha - hP.alpha};
        Pixel128S SminusR = {hS.red - hR.red, hS.green - hR.green, hS.blue - hR.blue, hS.alpha - hR.alpha};
        
        //Multiply colors.
        hP.red		*=	ui32WordWidth;
        hP.green	*=	ui32WordWidth;
        hP.blue		*=	ui32WordWidth;
        hP.alpha	*=	ui32WordWidth;
        hR.red		*=	ui32WordWidth;
        hR.green	*=	ui32WordWidth;
        hR.blue		*=	ui32WordWidth;
        hR.alpha	*=	ui32WordWidth;
        
        if (ui8Bpp == 2)
        {
            //Loop through pixels to achieve results.
            for (unsigned int x = 0; x < ui32WordWidth; x++)
            {
                Pixel128S result = {4 * hP.red, 4 * hP.green, 4 * hP.blue, 4 * hP.alpha};
                Pixel128S dY = {hR.red - hP.red, hR.green - hP.green, hR.blue - hP.blue, hR.alpha - hP.alpha};
                
                for (unsigned int y = 0; y < ui32WordHeight; y++)
                {
                    pPixel[y * ui32WordWidth + x].red   = (int32)((result.red   >> 7) + (result.red   >> 2));
                    pPixel[y * ui32WordWidth + x].green = (int32)((result.green >> 7) + (result.green >> 2));
                    pPixel[y * ui32WordWidth + x].blue  = (int32)((result.blue  >> 7) + (result.blue  >> 2));
                    pPixel[y * ui32WordWidth + x].alpha = (int32)((result.alpha >> 5) + (result.alpha >> 1));
                    
                    result.red   += dY.red;
                    result.green += dY.green;
                    result.blue  += dY.blue;
                    result.alpha += dY.alpha;
                }
                
                hP.red		+= QminusP.red;
                hP.green	+= QminusP.green;
                hP.blue		+= QminusP.blue;
                hP.alpha	+= QminusP.alpha;
                
                hR.red		+= SminusR.red;
                hR.green	+= SminusR.green;
                hR.blue		+= SminusR.blue;
                hR.alpha	+= SminusR.alpha;
            }
        }
        else
        {
            //Loop through pixels to achieve results.
            for (unsigned int y = 0; y < ui32WordHeight; y++)
            {
                Pixel128S result = {4 * hP.red, 4 * hP.green, 4 * hP.blue, 4 * hP.alpha};
                Pixel128S dY = {hR.red - hP.red, hR.green - hP.green, hR.blue - hP.blue, hR.alpha - hP.alpha};
                
                for (unsigned int x = 0; x < ui32WordWidth; x++)
                {
                    pPixel[x * ui32WordWidth + y].red   = (int32)((result.red   >> 6) + (result.red   >> 1));
                    pPixel[x * ui32WordWidth + y].green = (int32)((result.green >> 6) + (result.green >> 1));
                    pPixel[x * ui32WordWidth + y].blue  = (int32)((result.blue  >> 6) + (result.blue  >> 1));
                    pPixel[x * ui32WordWidth + y].alpha = (int32)((result.alpha >> 4) + (result.alpha));
                    
                    result.red   += dY.red;
                    result.green += dY.green;
                    result.blue  += dY.blue;
                    result.alpha += dY.alpha;
                }
                
                hP.red   += QminusP.red;
                hP.green += QminusP.green;
                hP.blue  += QminusP.blue;
                hP.alpha += QminusP.alpha;
                
                hR.red   += SminusR.red;
                hR.green += SminusR.green;
                hR.blue  += SminusR.blue;
                hR.alpha += SminusR.alpha;
            }
        }
    }
    
#define PUNCHTHROUGH_ALPHA 0x10
    
    static void unpackModulations(const PVRTCWord& word, int offsetX, int offsetY, uint8 i32ModulationValues[8][16], uint8 ui8Bpp)
    {
        uint32 WordModMode = word.u32ColorData & 0x1;
        uint32 ModulationBits = word.u32ModulationData;
        
        const uint8 modulation_table[] =
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
                    uint8* dest = &i32ModulationValues[y + offsetY][0 + offsetX];
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
                    uint8* dest = &i32ModulationValues[y + offsetY][0 + offsetX];
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
            const uint8* table = modulation_table + WordModMode * 4;
            for (int y = 0; y < 4; y++)
            {
                uint8* dest = &i32ModulationValues[y + offsetY][0 + offsetX];
                for (int x = 0; x < 4; x++)
                {
                    dest[x] = table[ModulationBits & 3];
                    ModulationBits >>= 2;
                }
            }
        }
    }
    
    static int32 getModulationValues(uint8 i32ModulationValues[8][16], uint32 xPos, uint32 yPos, uint8 ui8Bpp)
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
    
    static void pvrtcGetDecompressedPixels(uint8 i32ModulationValues[8][16],
                                           Pixel128S upscaledColorA[32],
                                           Pixel128S upscaledColorB[32],
                                           uint8* pColorData, int stride,
                                           int xoffset, int yoffset, int width, int height,
                                           uint8 ui8Bpp)
    {
        const uint32 ui32WordWidth = (ui8Bpp == 2) ? 8 : 4;
        const uint32 ui32WordHeight = 4;
        
        xoffset = xoffset * ui32WordWidth - ui32WordWidth / 2;
        yoffset = yoffset * ui32WordHeight - ui32WordHeight / 2;
        const int xmask = width - 1;
        const int ymask = height - 1;
        
        for (unsigned int y = 0; y < ui32WordHeight; y++)
        {
            const Pixel128S* colorA = upscaledColorA + y * ui32WordWidth;
            const Pixel128S* colorB = upscaledColorB + y * ui32WordWidth;
            
            const int sy = ((yoffset + y) & ymask) * stride;
            Pixel32* dest = reinterpret_cast<Pixel32 *>(pColorData + sy);

            for (unsigned int x = 0; x < ui32WordWidth; x++)
            {
                int32 mod = getModulationValues(i32ModulationValues, x + ui32WordWidth / 2, y + ui32WordHeight / 2, ui8Bpp);
                bool punchthrough_alpha = (mod & PUNCHTHROUGH_ALPHA) != 0;
                mod &= 0xf;
                
                Pixel128S result;
                result.red   = lerp(colorA[x].red,   colorB[x].red,   mod);
                result.green = lerp(colorA[x].green, colorB[x].green, mod);
                result.blue  = lerp(colorA[x].blue,  colorB[x].blue,  mod);
                if (punchthrough_alpha)
                    result.alpha = 0;
                else
                    result.alpha = lerp(colorA[x].alpha, colorB[x].alpha, mod);
                
                const int offset = (xoffset + x) & xmask;
                dest[offset].red   = (uint8)result.red;
                dest[offset].green = (uint8)result.green;
                dest[offset].blue  = (uint8)result.blue;
                dest[offset].alpha = (uint8)result.alpha;
            }
        }
    }
    
    constexpr unsigned int wrapWordIndex(unsigned int numWords, int word)
    {
        //return ((word + numWords) % numWords);
        return word & (numWords - 1); // numWords must be power of two
    }
    
    static void moveModulationValues(uint8 i32ModulationValues[8][16], uint32 ui32WordWidth, uint8 ui8Bpp)
    {
        uint32* d = (uint32*) &i32ModulationValues[0][0];
        uint32* s = (uint32*) &i32ModulationValues[0][ui32WordWidth];
        for (int i = 0; i < 8; ++i)
        {
            d[0] = s[0];
            if (ui8Bpp)
                d[1] = s[1];
            d += 4;
            s += 4;
        }
    }
    
    static void pvrtcDecompress(const uint8* pCompressedData,
                               uint8* pDecompressedData,
                               int stride,
                               uint32 ui32Width,
                               uint32 ui32Height,
                               uint8 ui8Bpp)
    {
        const uint32 ui32WordWidth = (ui8Bpp == 2) ? 8 : 4;
        const uint32 ui32WordHeight = 4;
        
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
            
            uint8 i32ModulationValues[8][16];
            
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

namespace mango
{

    void decode_block_pvrtc(const TextureCompressionInfo& info, uint8* out, const uint8* in, int stride)
    {
        bool is2bpp = false;

        switch (info.compression) {
            case TextureCompression::PVRTC_RGB_4BPP:
            case TextureCompression::PVRTC_RGBA_4BPP:
            case TextureCompression::PVRTC_SRGB_4BPP:
            case TextureCompression::PVRTC_SRGB_ALPHA_4BPP:
                break;

            case TextureCompression::PVRTC_RGB_2BPP:
            case TextureCompression::PVRTC_RGBA_2BPP:
            case TextureCompression::PVRTC_SRGB_2BPP:
            case TextureCompression::PVRTC_SRGB_ALPHA_2BPP:
                is2bpp = true;
                break;

            default:
                // TODO: unsupported compression
                break;
        }

        // TODO: check that width and height are powers of two
        // TODO: check that width and height are at least 8x8 or 16x8
        // TODO: check that width and height are identical (pvrtc textures are square)

        pvrtcDecompress(in, out, stride, info.width, info.height, is2bpp ? 2 : 4);
    }

} // namespace mango
