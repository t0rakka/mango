/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/endian.hpp>
#include <mango/image/compression.hpp>

// https://www.khronos.org/registry/OpenGL/extensions/3DFX/3DFX_texture_compression_FXT1.txt

namespace
{
    using namespace mango;
    using namespace mango::image;

    struct BlockFXT
    {
        u8 data[16];

        u32 getMode() const
        {
            // mode is stored in the 3 last bits of FXT block
            return data[15] >> 5;
        }
    };

    struct BlockHI
    {
        u8 indices[12];

        u32 blue0  : 5;
        u32 green0 : 5;
        u32 red0   : 5;
        u32 blue1  : 5;
        u32 green1 : 5;
        u32 red1   : 5;
        u32 mode   : 2;
    };

    struct BlockCHROMA
    {
        u8 indices[8];

        u64 blue0  : 5;
        u64 green0 : 5;
        u64 red0   : 5;
        u64 blue1  : 5;
        u64 green1 : 5;
        u64 red1   : 5;
        u64 blue2  : 5;
        u64 green2 : 5;
        u64 red2   : 5;
        u64 blue3  : 5;
        u64 green3 : 5;
        u64 red3   : 5;
        u64 unused : 1;
        u64 mode   : 3;
    };

    struct BlockMIXED
    {
        u8 indices[8];

        u64 blue0  : 5;
        u64 green0 : 5;
        u64 red0   : 5;
        u64 blue1  : 5;
        u64 green1 : 5;
        u64 red1   : 5;
        u64 blue2  : 5;
        u64 green2 : 5;
        u64 red2   : 5;
        u64 blue3  : 5;
        u64 green3 : 5;
        u64 red3   : 5;
        u64 alpha  : 1;
        u64 lsb1   : 1;
        u64 lsb3   : 1;
        u64 mode   : 1;
    };

    struct BlockALPHA
    {
        u8 indices[8];

        u64 blue0  : 5;
        u64 green0 : 5;
        u64 red0   : 5;
        u64 blue1  : 5;
        u64 green1 : 5;
        u64 red1   : 5;
        u64 blue2  : 5;
        u64 green2 : 5;
        u64 red2   : 5;
        u64 alpha0 : 5;
        u64 alpha1 : 5;
        u64 alpha2 : 5;
        u64 lerp   : 1;
        u64 mode   : 3;
    };

    inline u32 expand5to8(u32 v)
    {
        return (v << 3) | (v >> 2);
    }

    inline u32 expand6to8(u32 v)
    {
        return (v << 2) | (v >> 4);
    }

    void decode_hi(u8* out, size_t stride, const BlockHI& block, u8 alphaMask)
    {
        u32 b0 = expand5to8(block.blue0);
        u32 g0 = expand5to8(block.green0);
        u32 r0 = expand5to8(block.red0);

        u32 b1 = expand5to8(block.blue1);
        u32 g1 = expand5to8(block.green1);
        u32 r1 = expand5to8(block.red1);

        Color color[8];

        color[0] = Color(r0, g0, b0, 0xff);
        color[1] = Color((5 * r0 + 1 * r1 + 3) / 6, (5 * g0 + 1 * g1 + 3) / 6, (5 * b0 + 1 * b1 + 3) / 6, 0xff);
        color[2] = Color((4 * r0 + 2 * r1 + 3) / 6, (4 * g0 + 2 * g1 + 3) / 6, (4 * b0 + 2 * b1 + 3) / 6, 0xff);
        color[3] = Color((3 * r0 + 3 * r1 + 3) / 6, (3 * g0 + 3 * g1 + 3) / 6, (3 * b0 + 3 * b1 + 3) / 6, 0xff);
        color[4] = Color((2 * r0 + 4 * r1 + 3) / 6, (2 * g0 + 4 * g1 + 3) / 6, (2 * b0 + 4 * b1 + 3) / 6, 0xff);
        color[5] = Color((1 * r0 + 5 * r1 + 3) / 6, (1 * g0 + 5 * g1 + 3) / 6, (1 * b0 + 5 * b1 + 3) / 6, 0xff);
        color[6] = Color(r1, g1, b1, 0xff);
        color[7] = Color(0, 0, 0, alphaMask);

        u64 indices0 = uload64le(block.indices + 0);
        u64 indices1 = uload64le(block.indices + 6);

        for (int y = 0; y < 4; ++y)
        {
            Color* dest = reinterpret_cast<Color *>(out + y * stride);

            // left 4x4 block
            dest[0] = color[(indices0 >> 0) & 7];
            dest[1] = color[(indices0 >> 3) & 7];
            dest[2] = color[(indices0 >> 6) & 7];
            dest[3] = color[(indices0 >> 9) & 7];
            indices0 >>= 12;

            // right 4x4 block
            dest[4] = color[(indices1 >> 0) & 7];
            dest[5] = color[(indices1 >> 3) & 7];
            dest[6] = color[(indices1 >> 6) & 7];
            dest[7] = color[(indices1 >> 9) & 7];
            indices1 >>= 12;
        }
    }

    void decode_chroma(u8* out, size_t stride, const BlockCHROMA& block, u8 alphaMask)
    {
        u32 b0 = expand5to8(block.blue0);
        u32 g0 = expand5to8(block.green0);
        u32 r0 = expand5to8(block.red0);

        u32 b1 = expand5to8(block.blue1);
        u32 g1 = expand5to8(block.green1);
        u32 r1 = expand5to8(block.red1);

        u32 b2 = expand5to8(block.blue2);
        u32 g2 = expand5to8(block.green2);
        u32 r2 = expand5to8(block.red2);

        u32 b3 = expand5to8(block.blue3);
        u32 g3 = expand5to8(block.green3);
        u32 r3 = expand5to8(block.red3);

        Color color[4];

        color[0] = Color(r0, g0, b0, 0xff);
        color[1] = Color(r1, g1, b1, 0xff);
        color[2] = Color(r2, g2, b2, 0xff);
        color[3] = Color(r3, g3, b3, 0xff);

        u32 indices0 = uload32le(block.indices + 0);
        u32 indices1 = uload32le(block.indices + 4);

        for (int y = 0; y < 4; ++y)
        {
            Color* dest = reinterpret_cast<Color *>(out + y * stride);

            // left 4x4 block
            dest[0] = color[(indices0 >> 0) & 3];
            dest[1] = color[(indices0 >> 2) & 3];
            dest[2] = color[(indices0 >> 4) & 3];
            dest[3] = color[(indices0 >> 6) & 3];
            indices0 >>= 8;

            // right 4x4 block
            dest[4] = color[(indices1 >> 0) & 3];
            dest[5] = color[(indices1 >> 2) & 3];
            dest[6] = color[(indices1 >> 4) & 3];
            dest[7] = color[(indices1 >> 6) & 3];
            indices1 >>= 8;
        }
    }

    void decode_alpha(u8* out, size_t stride, const BlockALPHA& block, u8 alphaMask)
    {
        u32 b0 = expand5to8(block.blue0);
        u32 g0 = expand5to8(block.green0);
        u32 r0 = expand5to8(block.red0);
        u32 a0 = expand5to8(block.alpha0) | alphaMask;

        u32 b1 = expand5to8(block.blue1);
        u32 g1 = expand5to8(block.green1);
        u32 r1 = expand5to8(block.red1);
        u32 a1 = expand5to8(block.alpha1) | alphaMask;

        u32 b2 = expand5to8(block.blue2);
        u32 g2 = expand5to8(block.green2);
        u32 r2 = expand5to8(block.red2);
        u32 a2 = expand5to8(block.alpha2) | alphaMask;

        Color color[8];

        if (!block.lerp)
        {
            // colors for left 4x4 block
            color[0] = Color(r0, g0, b0, a0);
            color[1] = Color(r1, g1, b1, a1);
            color[2] = Color(r2, g2, b2, a2);
            color[3] = Color(0, 0, 0, alphaMask);

            // colors for right 4x4 block
            color[4] = color[0];
            color[5] = color[1];
            color[6] = color[2];
            color[7] = color[3];
        }
        else
        {
            // colors for left 4x4 block
            color[0] = Color(r0, g0, b0, a0);
            color[1] = Color((2 * r0 + r1 + 1) / 3, (2 * g0 + g1 + 1) / 3, (2 * b0 + b1 + 1) / 3, (2 * a0 + a1 + 1) / 3);
            color[2] = Color((r0 + 2 * r1 + 1) / 3, (g0 + 2 * g1 + 1) / 3, (b0 + 2 * b1 + 1) / 3, (a0 + 2 * a1 + 1) / 3);
            color[3] = Color(r1, g1, b1, a1);

            // colors for right 4x4 block
            color[4] = Color(r2, g2, b2, a2);
            color[5] = Color((2 * r2 + r1 + 1) / 3, (2 * g2 + g1 + 1) / 3, (2 * b2 + b1 + 1) / 3, (2 * a2 + a1 + 1) / 3);
            color[6] = Color((r2 + 2 * r1 + 1) / 3, (g2 + 2 * g1 + 1) / 3, (b2 + 2 * b1 + 1) / 3, (a2 + 2 * a1 + 1) / 3);
            color[7] = Color(r1, g1, b1, a1);
        }

        u32 indices0 = uload32le(block.indices + 0);
        u32 indices1 = uload32le(block.indices + 4);

        for (int y = 0; y < 4; ++y)
        {
            Color* dest = reinterpret_cast<Color *>(out + y * stride);

            // left 4x4 block
            dest[0] = color[0 + ((indices0 >> 0) & 3)];
            dest[1] = color[0 + ((indices0 >> 2) & 3)];
            dest[2] = color[0 + ((indices0 >> 4) & 3)];
            dest[3] = color[0 + ((indices0 >> 6) & 3)];
            indices0 >>= 8;

            // right 4x4 block
            dest[4] = color[4 + ((indices1 >> 0) & 3)];
            dest[5] = color[4 + ((indices1 >> 2) & 3)];
            dest[6] = color[4 + ((indices1 >> 4) & 3)];
            dest[7] = color[4 + ((indices1 >> 6) & 3)];
            indices1 >>= 8;
        }
    }

    void decode_mixed(u8* out, size_t stride, const BlockMIXED& block, u8 alphaMask)
    {
        Color color[8];

        if (!block.alpha)
        {
            u32 bit01 = (block.indices[0] >> 1) & 1;
            u32 bit33 = (block.indices[4] >> 1) & 1;

            u32 b0 = expand5to8(block.blue0);
            u32 g0 = expand5to8(u32(block.green0 << 1) | u32(block.lsb1 ^ bit01));
            u32 r0 = expand5to8(block.red0);

            u32 b1 = expand5to8(block.blue1);
            u32 g1 = expand6to8(u32((block.green1 << 1) | block.lsb1));
            u32 r1 = expand5to8(block.red1);

            u32 b2 = expand5to8(block.blue2);
            u32 g2 = expand5to8(u32((block.green2 << 1) | (block.lsb3 ^ bit33)));
            u32 r2 = expand5to8(block.red2);

            u32 b3 = expand5to8(block.blue3);
            u32 g3 = expand6to8(u32((block.green3 << 1) | block.lsb3));
            u32 r3 = expand5to8(block.red3);

            // colors for left 4x4 block
            color[0] = Color(r0, g0, b0, 0xff);
            color[1] = Color((2 * r0 + r1 + 1) / 3, (2 * g0 + g1 + 1) / 3, (2 * b0 + b1 + 1) / 3, 0xff);
            color[2] = Color((r0 + 2 * r1 + 1) / 3, (g0 + 2 * g1 + 1) / 3, (b0 + 2 * b1 + 1) / 3, 0xff);
            color[3] = Color(r1, g1, b1, 0xff);

            // colors for right 4x4 block
            color[4] = Color(r2, g2, b2, 0xff);
            color[1] = Color((2 * r2 + r3 + 1) / 3, (2 * g2 + g3 + 1) / 3, (2 * b2 + b3 + 1) / 3, 0xff);
            color[2] = Color((r2 + 2 * r3 + 1) / 3, (g2 + 2 * g3 + 1) / 3, (b2 + 2 * b3 + 1) / 3, 0xff);
            color[7] = Color(r3, g3, b3, 0xff);
        }
        else
        {
            u32 b0 = expand5to8(block.blue0);
            u32 g0 = expand5to8(block.green0);
            u32 r0 = expand5to8(block.red0);

            u32 b1 = expand5to8(block.blue1);
            u32 g1 = expand6to8(u32((block.green1 << 1) | block.lsb1));
            u32 r1 = expand5to8(block.red1);

            u32 b2 = expand5to8(block.blue2);
            u32 g2 = expand5to8(block.green2);
            u32 r2 = expand5to8(block.red2);

            u32 b3 = expand5to8(block.blue3);
            u32 g3 = expand6to8(u32((block.green3 << 1) | block.lsb3));
            u32 r3 = expand5to8(block.red3);

            // colors for left 4x4 block
            color[0] = Color(r0, g0, b0, 0xff);
            color[1] = Color((r0 + r1) / 2, (g0 + g1) / 2, (b0 + b1) / 2, 0xff);
            color[2] = Color(r1, g1, b1, 0xff);
            color[3] = Color(0, 0, 0, alphaMask);

            // colors for right 4x4 block
            color[4] = Color(r2, g2, b2, 0xff);
            color[5] = Color((r2 + r3) / 2, (g2 + g3) / 2, (b2 + b3) / 2, 0xff);
            color[6] = Color(r3, g3, b3, 0xff);
            color[7] = Color(0, 0, 0, alphaMask);
        }

        u32 indices0 = uload32le(block.indices + 0);
        u32 indices1 = uload32le(block.indices + 4);

        for (int y = 0; y < 4; ++y)
        {
            Color* dest = reinterpret_cast<Color *>(out + y * stride);

            // left 4x4 block
            dest[0] = color[0 + ((indices0 >> 0) & 3)];
            dest[1] = color[0 + ((indices0 >> 2) & 3)];
            dest[2] = color[0 + ((indices0 >> 4) & 3)];
            dest[3] = color[0 + ((indices0 >> 6) & 3)];
            indices0 >>= 8;

            // right 4x4 block
            dest[4] = color[4 + ((indices1 >> 0) & 3)];
            dest[5] = color[4 + ((indices1 >> 2) & 3)];
            dest[6] = color[4 + ((indices1 >> 4) & 3)];
            dest[7] = color[4 + ((indices1 >> 6) & 3)];
            indices1 >>= 8;
        }
    }

    void decode_fxt1(u8* out, size_t stride, const BlockFXT& block, u8 alphaMask)
    {
        u32 mode = block.getMode();
        switch (mode)
        {
            case 0:
            case 1:
                // 00x
                decode_hi(out, stride, reinterpret_cast<const BlockHI &>(block), alphaMask);
                break;
            case 2:
                // 010
                decode_chroma(out, stride, reinterpret_cast<const BlockCHROMA &>(block), alphaMask);
                break;
            case 3:
                // 011
                decode_alpha(out, stride, reinterpret_cast<const BlockALPHA &>(block), alphaMask);
                break;
            default:
                // 1xx
                decode_mixed(out, stride, reinterpret_cast<const BlockMIXED &>(block), alphaMask);
                break;
        }
    }

} // namespace

namespace mango::image
{

    void decode_block_fxt1_rgb(const TextureCompressionInfo& info, u8* out, const u8* in, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        const BlockFXT& block = *reinterpret_cast<const BlockFXT *>(in);
        decode_fxt1(out, stride, block, 0xff);
    }

    void decode_block_fxt1_rgba(const TextureCompressionInfo& info, u8* out, const u8* in, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        const BlockFXT& block = *reinterpret_cast<const BlockFXT *>(in);
        decode_fxt1(out, stride, block, 0);
    }

} // namespace mango::image
