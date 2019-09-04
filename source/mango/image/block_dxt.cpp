/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/endian.hpp>
#include <mango/image/compression.hpp>

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // Based on NVIDIA DXT decoder
    // ------------------------------------------------------------

    struct DXTColBlock
    {
        u16 color[2];
        u32 data;
    };

    struct DXTAlphaBlockExplicit
    {
        u16 data[4];
    };

    struct DXTAlphaBlock3BitLinear
    {
        u8 alpha[2];
        u8 stuff[6];
    };

    void unpack565(u8* dest, u16 packed)
    {
        u32 r = (packed >> 11) & 0x1f;
        u32 g = (packed >>  5) & 0x3f;
        u32 b = (packed >>  0) & 0x1f;
        dest[0] = u8((r << 3) | (r >> 2));
        dest[1] = u8((g << 2) | (g >> 4));
        dest[2] = u8((b << 3) | (b >> 2));
    }

    // ------------------------------------------------------------
    // block decoders
    // ------------------------------------------------------------

    void GetColorBlockColors(u32* color, const DXTColBlock* block, u8 alpha)
    {
        u8* dest = reinterpret_cast<u8*>(color);

        u16 a = uload16le(&block->color[0]);
        u16 b = uload16le(&block->color[1]);

        unpack565(dest + 0, a); // bit code 00
        unpack565(dest + 4, b); // bit code 01
        dest[3] = alpha;
        dest[7] = alpha;

        if (a > b)
        {
            // four-color block: derive the other two colors

            // bit code 10
            dest[8] = u8(u16(dest[0] * 2 + dest[4]) / 3);
            dest[9] = u8(u16(dest[1] * 2 + dest[5]) / 3);
            dest[10] = u8(u16(dest[2] * 2 + dest[6]) / 3);
            dest[11] = alpha;

            // bitcode 11
            dest[12] = u8(u16(dest[0] + dest[4] * 2) / 3);
            dest[13] = u8(u16(dest[1] + dest[5] * 2) / 3);
            dest[14] = u8(u16(dest[2] + dest[6] * 2) / 3);
            dest[15] = alpha;
        }
        else
        {
            // three-color block: derive the other color

            // bit code 10
            dest[8] = u8(u16(dest[0] + dest[4]) / 2);
            dest[9] = u8(u16(dest[1] + dest[5]) / 2);
            dest[10] = u8(u16(dest[2] + dest[6]) / 2);
            dest[11] = alpha;

            // bit code 11: transparent
            dest[12] = 0;
            dest[13] = 0;
            dest[14] = 0;
            dest[15] = 0;
        }
    }

    void DecodeColorBlock(u8* dest, int stride, const DXTColBlock* colorBlock, u8 alpha)
    {
        u32 color[4];
        GetColorBlockColors(color, colorBlock, alpha);

        u32 data = uload32le(&colorBlock->data);

        for (int y = 0; y < 4; ++y)
        {
            u32* d = reinterpret_cast<u32*>(dest);
            d[0] = color[(data >> 0) & 3];
            d[1] = color[(data >> 2) & 3];
            d[2] = color[(data >> 4) & 3];
            d[3] = color[(data >> 6) & 3];
            data >>= 8;
            dest += stride;
        }
    }

    void DecodeAlphaTable(u8* alpha, const DXTAlphaBlock3BitLinear* alphaBlock)
    {
        alpha[0] = alphaBlock->alpha[0];
        alpha[1] = alphaBlock->alpha[1];

        if (alpha[0] > alpha[1])
        {
            const int delta = alpha[1] - alpha[0];
            int alpha7 = 6 * alpha[0] + alpha[1];

            // 8-alpha block
            for (int i = 2; i < 8; ++i)
            {
                alpha[i] = u8(alpha7 / 7);
                alpha7 += delta;
            }
        }
        else
        {
            const int delta = alpha[1] - alpha[0];
            int alpha5 = 4 * alpha[0] + alpha[1];

            // 6-alpha block
            for (int i = 2; i < 6; ++i)
            {
                alpha[i] = u8(alpha5 / 5);
                alpha5 += delta;
            }
            
            alpha[6] = 0;
            alpha[7] = 0xff;
        }
    }

    inline u8 ExtendAlpha(u32 alpha)
    {
        return u8(alpha |= (alpha << 4));
    }

#ifdef MANGO_CPU_64BIT
    
    void DecodeAlphaExplicit(u8* dest, int stride, const DXTAlphaBlockExplicit* alphaBlock)
    {
        u64 data = uload64le(&alphaBlock->data[0]);

        for (int y = 0; y < 4; ++y)
        {
            dest[0] = ExtendAlpha((data >> 0) & 0xf);
            dest[4] = ExtendAlpha((data >> 4) & 0xf);
            dest[8] = ExtendAlpha((data >> 8) & 0xf);
            dest[12] = ExtendAlpha((data >> 12) & 0xf);
            data >>= 16;
            dest += stride;
        }
    }

    void Decode3BitLinear(u8* dest, int bpp, int stride, const DXTAlphaBlock3BitLinear* block)
    {
        u8 table[8];
        DecodeAlphaTable(table, block);

        // decode the codes into values
        u64 data = uload64le(block) >> 16; // load whole block and discard first 16 bits

        for (int y = 0; y < 4; ++y)
        {
            dest[bpp * 0] = table[(data >> 0) & 7];
            dest[bpp * 1] = table[(data >> 3) & 7];
            dest[bpp * 2] = table[(data >> 6) & 7];
            dest[bpp * 3] = table[(data >> 9) & 7];
            data >>= 12;
            dest += stride;
        }
	}

#else

    void DecodeAlphaExplicit(u8* dest, int stride, const DXTAlphaBlockExplicit* alphaBlock)
    {
        for (int y = 0; y < 4; ++y)
        {
            u32 data = uload16le(&alphaBlock->data[y]);
            dest[0] = ExtendAlpha((data >> 0) & 0xf);
            dest[4] = ExtendAlpha((data >> 4) & 0xf);
            dest[8] = ExtendAlpha((data >> 8) & 0xf);
            dest[12] = ExtendAlpha((data >> 12) & 0xf);
            dest += stride;
        }
    }

    void Decode3BitLinear(u8* dest, int bpp, int stride, const DXTAlphaBlock3BitLinear* block)
    {
        u8 table[8];
        DecodeAlphaTable(table, block);

        // decode the codes into values
        const u8* stuff = block->stuff;
        
        for (int i = 0; i < 2; ++i)
        {
            u32 data = (stuff[2] << 16) | (stuff[1] << 8) | stuff[0];
            stuff += 3;
            
            for (int y = 0; y < 2; ++y)
            {
                dest[bpp * 0] = table[data & 7];
                dest[bpp * 1] = table[(data >> 3) & 7];
                dest[bpp * 2] = table[(data >> 6) & 7];
                dest[bpp * 3] = table[(data >> 9) & 7];
                data >>= 12;
                dest += stride;
            }
        }
    }

#endif

} // namespace

namespace mango
{

    void decode_block_dxt1(const TextureCompressionInfo& info, u8* out, const u8* in, int stride)
    {
        MANGO_UNREFERENCED(info);
        const DXTColBlock* blockColor = reinterpret_cast<const DXTColBlock*>(in + 0);
        DecodeColorBlock(out, stride, blockColor, 0xff);
    }

    void decode_block_dxt3(const TextureCompressionInfo& info, u8* out, const u8* in, int stride)
    {
        MANGO_UNREFERENCED(info);
        const DXTAlphaBlockExplicit* alphaBlock = reinterpret_cast<const DXTAlphaBlockExplicit *>(in + 0);
        const DXTColBlock* colorBlock = reinterpret_cast<const DXTColBlock*>(in + 8);
        DecodeColorBlock(out + 0, stride, colorBlock, 0);
        DecodeAlphaExplicit(out + 3, stride, alphaBlock);
    }

    void decode_block_dxt5(const TextureCompressionInfo& info, u8* out, const u8* in, int stride)
    {
        MANGO_UNREFERENCED(info);
        const DXTAlphaBlock3BitLinear* alphaBlock = reinterpret_cast<const DXTAlphaBlock3BitLinear *>(in + 0);
        const DXTColBlock* colorBlock = reinterpret_cast<const DXTColBlock*>(in + 8);
        DecodeColorBlock(out + 0, stride, colorBlock, 0);
        Decode3BitLinear(out + 3, 4, stride, alphaBlock);
    }

    void decode_block_3dc_x(const TextureCompressionInfo& info, u8* out, const u8* in, int stride)
    {
        MANGO_UNREFERENCED(info);
        const DXTAlphaBlock3BitLinear* redBlock = reinterpret_cast<const DXTAlphaBlock3BitLinear*>(in + 0);
        Decode3BitLinear(out + 0, 1, stride, redBlock);
    }

    void decode_block_3dc_xy(const TextureCompressionInfo& info, u8* out, const u8* in, int stride)
    {
        MANGO_UNREFERENCED(info);
        const DXTAlphaBlock3BitLinear* redBlock = reinterpret_cast<const DXTAlphaBlock3BitLinear*>(in + 0);
        const DXTAlphaBlock3BitLinear* greenBlock = reinterpret_cast<const DXTAlphaBlock3BitLinear*>(in + 8);
        Decode3BitLinear(out + 0, 2, stride, redBlock);
        Decode3BitLinear(out + 1, 2, stride, greenBlock);
    }

} // namespace mango
