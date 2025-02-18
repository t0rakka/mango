/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/bits.hpp>
#include <mango/core/endian.hpp>
#include <mango/image/compression.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // Fixed based on Khronos specification:
    // https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html#s3tc_bc2

    struct ColorBlock
    {
        u16 color[2];
        u32 data;
    };

    struct AlphaBlockExplicit
    {
        u16 data[4];
    };

    struct AlphaBlockLinear
    {
        u8 alpha[2];
        u8 stuff[6];
    };

    // ------------------------------------------------------------
    // block decoders
    // ------------------------------------------------------------

    void GetBlockColors(u32* color, const ColorBlock* block, u8 alpha, bool isFourColorBlock)
    {
        u16 c0 = littleEndian::uload16(block->color + 0);
        u16 c1 = littleEndian::uload16(block->color + 1);

        u32 r0 = u32_extend((c0 >> 11) & 0x1f, 5, 8);
        u32 g0 = u32_extend((c0 >>  5) & 0x3f, 6, 8);
        u32 b0 = u32_extend((c0 >>  0) & 0x1f, 5, 8);

        u32 r1 = u32_extend((c1 >> 11) & 0x1f, 5, 8);
        u32 g1 = u32_extend((c1 >>  5) & 0x3f, 6, 8);
        u32 b1 = u32_extend((c1 >>  0) & 0x1f, 5, 8);

        color[0] = makeRGBA(r0, g0, b0, 0xff);
        color[1] = makeRGBA(r1, g1, b1, 0xff);

        if ((c0 > c1) || isFourColorBlock)
        {
            // four-color block
            color[2] = makeRGBA((r0 * 2 + r1) / 3, (g0 * 2 + g1) / 3, (b0 * 2 + b1) / 3, 0xff);
            color[3] = makeRGBA((r0 + 2 * r1) / 3, (g0 + 2 * g1) / 3, (b0 + 2 * b1) / 3, 0xff);
        }
        else
        {
            // three-color block
            color[2] = makeRGBA((r0 + r1) / 2, (g0 + g1) / 2, (b0 + b1) / 2, 0xff);
            color[3] = makeRGBA(0, 0, 0, alpha);
        }
    }

    void DecodeColor(u8* dest, size_t stride, const ColorBlock* colorBlock, u8 alpha, bool isFourColorBlock)
    {
        u32 color[4];
        GetBlockColors(color, colorBlock, alpha, isFourColorBlock);

        u32 data = littleEndian::uload32(&colorBlock->data);

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

    void DecodeAlphaTable(u8* alpha, const AlphaBlockLinear* alphaBlock)
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

#ifdef MANGO_CPU_64BIT
    
    void DecodeAlpha(u8* dest, size_t stride, const AlphaBlockExplicit* alphaBlock)
    {
        u64 data = littleEndian::uload64(&alphaBlock->data[0]);

        for (int y = 0; y < 4; ++y)
        {
            dest[0]  = u8_extend((data >>  0) & 0xf, 4, 8);
            dest[4]  = u8_extend((data >>  4) & 0xf, 4, 8);
            dest[8]  = u8_extend((data >>  8) & 0xf, 4, 8);
            dest[12] = u8_extend((data >> 12) & 0xf, 4, 8);
            data >>= 16;
            dest += stride;
        }
    }

    void DecodeAlpha(u8* dest, int bpp, size_t stride, const AlphaBlockLinear* alphaBlock)
    {
        u8 table[8];
        DecodeAlphaTable(table, alphaBlock);

        // decode the codes into values
        u64 data = littleEndian::uload64(alphaBlock) >> 16; // load whole block and discard first 16 bits

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

    void DecodeAlpha(u8* dest, size_t stride, const AlphaBlockExplicit* alphaBlock)
    {
        for (int y = 0; y < 4; ++y)
        {
            u32 data = littleEndian::uload16(&alphaBlock->data[y]);
            dest[0]  = u8_extend((data >>  0) & 0xf, 4, 8);
            dest[4]  = u8_extend((data >>  4) & 0xf, 4, 8);
            dest[8]  = u8_extend((data >>  8) & 0xf, 4, 8);
            dest[12] = u8_extend((data >> 12) & 0xf, 4, 8);
            dest += stride;
        }
    }

    void DecodeAlpha(u8* dest, int bpp, size_t stride, const AlphaBlockLinear* alphaBlock)
    {
        u8 table[8];
        DecodeAlphaTable(table, alphaBlock);

        // decode the codes into values
        const u8* stuff = alphaBlock->stuff;
        
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

    void DecodeATC(u8* dest, size_t stride, const u8* src)
    {
        u32 a = littleEndian::uload16(src + 0);
        u32 b = littleEndian::uload16(src + 2);
        u32 indices = littleEndian::uload32(src + 4);

        u8 color[16];

        if (a & 0x8000)
        {
            color[ 0] = 0;
            color[ 1] = 0;
            color[ 2] = 0;
            color[ 3] = 0xff;

            color[ 8] = u8_extend((a >>  0) & 0x1f, 5, 8);
            color[ 9] = u8_extend((a >>  5) & 0x1f, 5, 8);
            color[10] = u8_extend((a >> 10) & 0x1f, 5, 8);
            color[11] = 0xff;

            color[12] = u8_extend((b >>  0) & 0x1f, 5, 8);
            color[13] = u8_extend((b >>  5) & 0x3f, 6, 8);
            color[14] = u8_extend((b >> 11) & 0x1f, 5, 8);
            color[15] = 0xff;

            color[ 4] = color[ 8] - color[12] / 4;
            color[ 5] = color[ 9] - color[13] / 4;
            color[ 6] = color[10] - color[14] / 4;
            color[ 7] = 0xff;
        }
        else
        {
            color[ 0] = u8_extend((a >>  0) & 0x1f, 5, 8);
            color[ 1] = u8_extend((a >>  5) & 0x1f, 5, 8);
            color[ 2] = u8_extend((a >> 10) & 0x1f, 5, 8);
            color[ 3] = 0xff;

            color[12] = u8_extend((b >>  0) & 0x1f, 5, 8);
            color[13] = u8_extend((b >>  5) & 0x3f, 6, 8);
            color[14] = u8_extend((b >> 11) & 0x1f, 5, 8);
            color[15] = 0xff;

            color[ 4] = (2 * color[0] + color[12]) / 3;
            color[ 5] = (2 * color[1] + color[13]) / 3;
            color[ 6] = (2 * color[2] + color[14]) / 3;
            color[ 7] = 0xff;

            color[ 8] = (color[0] + 2 * color[12]) / 3;
            color[ 9] = (color[1] + 2 * color[13]) / 3;
            color[10] = (color[2] + 2 * color[14]) / 3;
            color[11] = 0xff;
        }

        for (int y = 0; y < 4; ++y)
        {
            for (int x = 0; x < 4; ++x)
            {
                int idx = indices & 3;
                indices >>= 2;
                dest[x * 4 + 0] = color[idx * 4 + 2];
                dest[x * 4 + 1] = color[idx * 4 + 1];
                dest[x * 4 + 2] = color[idx * 4 + 0];
                dest[x * 4 + 3] = color[idx * 4 + 3];
            }
            dest += stride;
        }
    }

} // namespace

namespace mango::image
{

    void decode_block_dxt1(const TextureCompression& info, u8* out, const u8* in, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        const ColorBlock* blockColor = reinterpret_cast<const ColorBlock*>(in + 0);
        DecodeColor(out, stride, blockColor, 0xff, false);
    }

    void decode_block_dxt1a(const TextureCompression& info, u8* out, const u8* in, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        const ColorBlock* blockColor = reinterpret_cast<const ColorBlock*>(in + 0);
        DecodeColor(out, stride, blockColor, 0, false);
    }

    void decode_block_dxt3(const TextureCompression& info, u8* out, const u8* in, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        const AlphaBlockExplicit* alphaBlock = reinterpret_cast<const AlphaBlockExplicit *>(in + 0);
        const ColorBlock* colorBlock = reinterpret_cast<const ColorBlock*>(in + 8);
        DecodeColor(out + 0, stride, colorBlock, 0, true);
        DecodeAlpha(out + 3, stride, alphaBlock);
    }

    void decode_block_dxt5(const TextureCompression& info, u8* out, const u8* in, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        const AlphaBlockLinear* alphaBlock = reinterpret_cast<const AlphaBlockLinear*>(in + 0);
        const ColorBlock* colorBlock = reinterpret_cast<const ColorBlock*>(in + 8);
        DecodeColor(out + 0, stride, colorBlock, 0, true);
        DecodeAlpha(out + 3, 4, stride, alphaBlock);
    }

    void decode_block_3dc_x(const TextureCompression& info, u8* out, const u8* in, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        const AlphaBlockLinear* redBlock = reinterpret_cast<const AlphaBlockLinear*>(in + 0);
        DecodeAlpha(out + 0, 1, stride, redBlock);
    }

    void decode_block_3dc_xy(const TextureCompression& info, u8* out, const u8* in, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        const AlphaBlockLinear* redBlock = reinterpret_cast<const AlphaBlockLinear*>(in + 0);
        const AlphaBlockLinear* greenBlock = reinterpret_cast<const AlphaBlockLinear*>(in + 8);
        DecodeAlpha(out + 0, 2, stride, redBlock);
        DecodeAlpha(out + 1, 2, stride, greenBlock);
    }

    void decode_block_atc(const TextureCompression& info, u8* out, const u8* in, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        DecodeATC(out + 0, stride, in + 0);
    }

    void decode_block_atc_e(const TextureCompression& info, u8* out, const u8* in, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        const AlphaBlockExplicit* alphaBlock = reinterpret_cast<const AlphaBlockExplicit*>(in + 0);
        DecodeATC(out + 0, stride, in + 8);
        DecodeAlpha(out + 3, stride, alphaBlock);
    }

    void decode_block_atc_i(const TextureCompression& info, u8* out, const u8* in, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        const AlphaBlockLinear* alphaBlock = reinterpret_cast<const AlphaBlockLinear*>(in + 0);
        DecodeATC(out + 0, stride, in + 8);
        DecodeAlpha(out + 3, 4, stride, alphaBlock);
    }

} // namespace mango::image
