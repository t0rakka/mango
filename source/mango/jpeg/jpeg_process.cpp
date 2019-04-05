/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "jpeg.hpp"

namespace jpeg
{
    using namespace mango;

// ----------------------------------------------------------------------------
// color conversion
// ----------------------------------------------------------------------------

/*

    Full-range YCbCr color conversion:

    |R|     |1.000  0.000  1.400|     | Y      |
    |G|  =  |1.000 -0.343 -0.711|  *  |Cb - 128|
    |B|     |1.000  1.765  0.000|     |Cr - 128|

    Pseudo code:

    R = Y +               Cr * 1.400   - (128 * 1.4)
    G = Y + Cb * -0.343 + Cr * -0.711  - (128 * (-0.343 - 0.711))
    B = Y + Cb * 1.765                 - (128 * 1.765)

    NOTE: ITU BT.601 would be the correct method :)
*/
/*
    CMYK to RGB color conversion:

    i = 1.0 - k
    r = 1.0 - min(1.0, c * i + k)
    g = 1.0 - min(1.0, m * i + k)
    b = 1.0 - min(1.0, y * i + k)

    NOTE: we use alternate conversion formula in COMPUTE_CMYK()
*/

#define COMPUTE_CBCR(cb, cr) \
    int r = (cr * 91750 - 11711232) >> 16; \
    int g = (cb * -22479 + cr * -46596 + 8874368) >> 16; \
    int b = (cb * 115671 - 14773120) >> 16;

#define COMPUTE_CMYK(y, k) \
    r = ((255 - r - y) * k) / 255; \
    g = ((255 - g - y) * k) / 255; \
    b = ((255 - b - y) * k) / 255;

#define PACK_BGRA(y) \
    0xff000000 | (byteclamp(y + r) << 16) | (byteclamp(y + g) << 8) | byteclamp(y + b);

// ----------------------------------------------------------------------------
// Generic C++ implementation
// ----------------------------------------------------------------------------

void process_Y(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    u8 result[64];
    state->idct(result, data, state->block[0].qt); // Y

    for (int y = 0; y < height; ++y)
    {
        std::memcpy(dest, result + y * 8, width);
        dest += stride;
    }
}

void process_YCbCr(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    u8 result[64 * JPEG_MAX_BLOCKS_IN_MCU];

    for (int i = 0; i < state->blocks; ++i)
    {
        Block& block = state->block[i];
        state->idct(result + i * 64, data, block.qt);
        data += 64;
    }

    // MCU size in blocks
    int xsize = (width + 7) / 8;
    int ysize = (height + 7) / 8;

    int cb_offset = state->frame[1].offset * 64;
    int cb_xshift = state->frame[1].Hsf;
    int cb_yshift = state->frame[1].Vsf;

    int cr_offset = state->frame[2].offset * 64;
    int cr_xshift = state->frame[2].Hsf;
    int cr_yshift = state->frame[2].Vsf;

    u8* cb_data = result + cb_offset;
    u8* cr_data = result + cr_offset;

    // process MCU
    for (int yb = 0; yb < ysize; ++yb)
    {
        // vertical clipping limit for current block
        const int ymax = std::min(8, height - yb * 8);

        for (int xb = 0; xb < xsize; ++xb)
        {
            u8* dest_block = dest + yb * 8 * stride + xb * 8 * sizeof(u32);
            u8* y_block = result + (yb * xsize + xb) * 64;
            u8* cb_block = cb_data + yb * (8 >> cb_yshift) * 8 + xb * (8 >> cb_xshift);
            u8* cr_block = cr_data + yb * (8 >> cr_yshift) * 8 + xb * (8 >> cr_xshift);

            // horizontal clipping limit for current block
            const int xmax = std::min(8, width - xb * 8);

            // process 8x8 block
            for (int y = 0; y < ymax; ++y)
            {
                u32* d = reinterpret_cast<u32*>(dest_block);

                u8* cb_scan = cb_block + (y >> cb_yshift) * 8;
                u8* cr_scan = cr_block + (y >> cr_yshift) * 8;

                for (int x = 0; x < xmax; ++x)
                {
                    u8 Y = y_block[x];
                    u8 cb = cb_scan[x >> cb_xshift];
                    u8 cr = cr_scan[x >> cr_xshift];
                    COMPUTE_CBCR(cb, cr);
                    d[x] = PACK_BGRA(Y);
                }
                dest_block += stride;
                y_block += 8;
            }
        }
    }
}

void process_CMYK(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    u8 result[64 * JPEG_MAX_BLOCKS_IN_MCU];

    for (int i = 0; i < state->blocks; ++i)
    {
        Block& block = state->block[i];
        state->idct(result + i * 64, data, block.qt);
        data += 64;
    }

    // MCU size in blocks
    int xsize = (width + 7) / 8;
    int ysize = (height + 7) / 8;

    int cb_offset = state->frame[1].offset * 64;
    int cb_xshift = state->frame[1].Hsf;
    int cb_yshift = state->frame[1].Vsf;

    int cr_offset = state->frame[2].offset * 64;
    int cr_xshift = state->frame[2].Hsf;
    int cr_yshift = state->frame[2].Vsf;

    int ck_offset = state->frame[3].offset * 64;
    int ck_xshift = state->frame[3].Hsf;
    int ck_yshift = state->frame[3].Vsf;

    u8* cb_data = result + cb_offset;
    u8* cr_data = result + cr_offset;
    u8* ck_data = result + ck_offset;

    // process MCU
    for (int yb = 0; yb < ysize; ++yb)
    {
        // vertical clipping limit for current block
        const int ymax = std::min(8, height - yb * 8);

        for (int xb = 0; xb < xsize; ++xb)
        {
            u8* dest_block = dest + yb * 8 * stride + xb * 8 * sizeof(u32);
            u8* y_block = result + (yb * xsize + xb) * 64;
            u8* cb_block = cb_data + yb * (8 >> cb_yshift) * 8 + xb * (8 >> cb_xshift);
            u8* cr_block = cr_data + yb * (8 >> cr_yshift) * 8 + xb * (8 >> cr_xshift);
            u8* ck_block = ck_data + yb * (8 >> ck_yshift) * 8 + xb * (8 >> ck_xshift);

            // horizontal clipping limit for current block
            const int xmax = std::min(8, width - xb * 8);

            // process 8x8 block
            for (int y = 0; y < ymax; ++y)
            {
                u32* d = reinterpret_cast<u32*>(dest_block);

                u8* cb_scan = cb_block + (y >> cb_yshift) * 8;
                u8* cr_scan = cr_block + (y >> cr_yshift) * 8;
                u8* ck_scan = ck_block + (y >> ck_yshift) * 8;

                for (int x = 0; x < xmax; ++x)
                {
                    u8 Y = y_block[x];
                    u8 cb = cb_scan[x >> cb_xshift];
                    u8 cr = cr_scan[x >> cr_xshift];
                    u8 ck = ck_scan[x >> ck_xshift];
                    COMPUTE_CBCR(cb, cr);
                    COMPUTE_CMYK(Y, ck);
                    d[x] = PACK_BGRA(0);
                }
                dest_block += stride;
                y_block += 8;
            }
        }
    }
}

void process_YCbCr_8x8(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    u8 result[64 * 3];

    state->idct(result + 64 * 0, data + 64 * 0, state->block[0].qt); // Y
    state->idct(result + 64 * 1, data + 64 * 1, state->block[1].qt); // Cb
    state->idct(result + 64 * 2, data + 64 * 2, state->block[2].qt); // Cr

    // color conversion
    const u8* src = result;

    for (int y = 0; y < 8; ++y)
    {
        const u8* s = src + y * 8;
        u32* d = reinterpret_cast<u32*>(dest);

        for (int x = 0; x < 8; ++x)
        {
            int cb = s[x + 64];
            int cr = s[x + 128];
            COMPUTE_CBCR(cb, cr);
            d[x] = PACK_BGRA(s[x]);
        }

        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void process_YCbCr_8x16(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    u8 result[64 * 4];

    state->idct(result +   0, data +   0, state->block[0].qt); // Y0
    state->idct(result +  64, data +  64, state->block[1].qt); // Y1
    state->idct(result + 128, data + 128, state->block[2].qt); // Cb
    state->idct(result + 192, data + 192, state->block[3].qt); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        u32* d0 = reinterpret_cast<u32*>(dest);
        u32* d1 = reinterpret_cast<u32*>(dest + stride);
        const u8* s = result + y * 16;
        const u8* c = result + y * 8 + 128;

        for (int x = 0; x < 8; ++x)
        {
            int cb = c[x + 0];
            int cr = c[x + 64];
            COMPUTE_CBCR(cb, cr);
            d0[x] = PACK_BGRA(s[x + 0]);
            d1[x] = PACK_BGRA(s[x + 8]);
        }

        dest += stride * 2;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void process_YCbCr_16x8(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    u8 result[64 * 4];

    state->idct(result +   0, data +   0, state->block[0].qt); // Y0
    state->idct(result +  64, data +  64, state->block[1].qt); // Y1
    state->idct(result + 128, data + 128, state->block[2].qt); // Cb
    state->idct(result + 192, data + 192, state->block[3].qt); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        u32* d = reinterpret_cast<u32*>(dest);
        u8* s = result + y * 8;
        u8* c = result + y * 8 + 128;

        for (int x = 0; x < 4; ++x)
        {
            int cb = c[x + 0];
            int cr = c[x + 64];
            COMPUTE_CBCR(cb, cr);
            d[x * 2 + 0] = PACK_BGRA(s[x * 2 + 0]);
            d[x * 2 + 1] = PACK_BGRA(s[x * 2 + 1]);
        }

        for (int x = 0; x < 4; ++x)
        {
            int cb = c[x + 4];
            int cr = c[x + 68];
            COMPUTE_CBCR(cb, cr);
            d[x * 2 + 8] = PACK_BGRA(s[x * 2 + 64]);
            d[x * 2 + 9] = PACK_BGRA(s[x * 2 + 65]);
        }

        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void process_YCbCr_16x16(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    u8 result[64 * 6];

    state->idct(result +   0, data +   0, state->block[0].qt); // Y0
    state->idct(result + 128, data +  64, state->block[1].qt); // Y1
    state->idct(result +  64, data + 128, state->block[2].qt); // Y2
    state->idct(result + 192, data + 192, state->block[3].qt); // Y3
    state->idct(result + 256, data + 256, state->block[4].qt); // Cb
    state->idct(result + 320, data + 320, state->block[5].qt); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        u32* d0 = reinterpret_cast<u32*>(dest);
        u32* d1 = reinterpret_cast<u32*>(dest + stride);
        const u8* s = result + y * 16;
        const u8* c = result + y * 8 + 256;

        for (int x = 0; x < 4; ++x)
        {
            int cb = c[x + 0];
            int cr = c[x + 64];
            COMPUTE_CBCR(cb, cr);
            d0[x * 2 + 0] = PACK_BGRA(s[x * 2 + 0]);
            d0[x * 2 + 1] = PACK_BGRA(s[x * 2 + 1]);
            d1[x * 2 + 0] = PACK_BGRA(s[x * 2 + 8]);
            d1[x * 2 + 1] = PACK_BGRA(s[x * 2 + 9]);
        }

        for (int x = 0; x < 4; ++x)
        {
            int cb = c[x + 4];
            int cr = c[x + 68];
            COMPUTE_CBCR(cb, cr);
            d0[x * 2 + 8] = PACK_BGRA(s[x * 2 + 128]);
            d0[x * 2 + 9] = PACK_BGRA(s[x * 2 + 129]);
            d1[x * 2 + 8] = PACK_BGRA(s[x * 2 + 136]);
            d1[x * 2 + 9] = PACK_BGRA(s[x * 2 + 137]);
        }

        dest += stride * 2;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

#undef COMPUTE_CBCR
#undef COMPUTE_CMYK
#undef PACK_BGRA

#if defined(JPEG_ENABLE_NEON)

    // ------------------------------------------------------------------------------------------------
    // NEON implementation
    // ------------------------------------------------------------------------------------------------

    constexpr s16 JPEG_PREC = 12;
    constexpr s16 JPEG_FIXED(double x) { return s16((x * double(1 << JPEG_PREC) + 0.5)); }

    static inline
    void convert_ycbcr_8x1_neon(u8* dest, int16x8_t y, int16x8_t cb, int16x8_t cr, int16x8_t s0, int16x8_t s1, int16x8_t s2, int16x8_t s3)
    {
        int16x8_t cb0 = vqdmulhq_s16(cb, s2);
        int16x8_t cr0 = vqdmulhq_s16(cr, s0);
        int16x8_t cb1 = vqdmulhq_s16(cb, s3);
        int16x8_t cr1 = vqdmulhq_s16(cr, s1);
        int16x8_t r = vaddq_s16(y, cr0);
        int16x8_t g = vaddq_s16(vaddq_s16(y, cb0), cr1);
        int16x8_t b = vaddq_s16(y, cb1);

        uint8x8x4_t packed;
        packed.val[0] = vqrshrun_n_s16(b, 4);
        packed.val[1] = vqrshrun_n_s16(g, 4);
        packed.val[2] = vqrshrun_n_s16(r, 4);
        packed.val[3] = vdup_n_u8(255);

        vst4_u8(dest, packed);
    }

    void process_YCbCr_8x8_neon(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        u8 result[64 * 3];

        state->idct(result +   0, data +   0, state->block[0].qt); // Y
        state->idct(result +  64, data +  64, state->block[1].qt); // Cb
        state->idct(result + 128, data + 128, state->block[2].qt); // Cr

        const uint8x8_t tosigned = vdup_n_u8(0x80);
        const int16x8_t s0 = vdupq_n_s16(JPEG_FIXED( 1.40200));
        const int16x8_t s1 = vdupq_n_s16(JPEG_FIXED(-0.71414));
        const int16x8_t s2 = vdupq_n_s16(JPEG_FIXED(-0.34414));
        const int16x8_t s3 = vdupq_n_s16(JPEG_FIXED( 1.77200));

        for (int y = 0; y < 8; ++y)
        {
            uint8x8_t u_y  = vld1_u8(result + y * 8 + 0);
            uint8x8_t u_cb = vld1_u8(result + y * 8 + 64);
            uint8x8_t u_cr = vld1_u8(result + y * 8 + 128);

            int16x8_t s_y = vreinterpretq_s16_u16(vshll_n_u8(u_y, 4));
            int16x8_t s_cb = vshll_n_s8(vreinterpret_s8_u8(vsub_u8(u_cb, tosigned)), 7);
            int16x8_t s_cr = vshll_n_s8(vreinterpret_s8_u8(vsub_u8(u_cr, tosigned)), 7);

            convert_ycbcr_8x1_neon(dest, s_y, s_cb, s_cr, s0, s1, s2, s3);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }

    void process_YCbCr_8x16_neon(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        u8 result[64 * 4];

        state->idct(result +   0, data +   0, state->block[0].qt); // Y0
        state->idct(result +  64, data +  64, state->block[1].qt); // Y1
        state->idct(result + 128, data + 128, state->block[2].qt); // Cb
        state->idct(result + 192, data + 192, state->block[3].qt); // Cr

        const uint8x8_t tosigned = vdup_n_u8(0x80);
        const int16x8_t s0 = vdupq_n_s16(JPEG_FIXED( 1.40200));
        const int16x8_t s1 = vdupq_n_s16(JPEG_FIXED(-0.71414));
        const int16x8_t s2 = vdupq_n_s16(JPEG_FIXED(-0.34414));
        const int16x8_t s3 = vdupq_n_s16(JPEG_FIXED( 1.77200));

        for (int y = 0; y < 8; ++y)
        {
            uint8x8_t u_y0 = vld1_u8(result + y * 16 + 0);
            uint8x8_t u_y1 = vld1_u8(result + y * 16 + 8);
            uint8x8_t u_cb = vld1_u8(result + y * 8 + 128);
            uint8x8_t u_cr = vld1_u8(result + y * 8 + 192);

            int16x8_t s_y0 = vreinterpretq_s16_u16(vshll_n_u8(u_y0, 4));
            int16x8_t s_y1 = vreinterpretq_s16_u16(vshll_n_u8(u_y1, 4));
            int16x8_t s_cb = vshll_n_s8(vreinterpret_s8_u8(vsub_u8(u_cb, tosigned)), 7);
            int16x8_t s_cr = vshll_n_s8(vreinterpret_s8_u8(vsub_u8(u_cr, tosigned)), 7);

            convert_ycbcr_8x1_neon(dest, s_y0, s_cb, s_cr, s0, s1, s2, s3);
            dest += stride;

            convert_ycbcr_8x1_neon(dest, s_y1, s_cb, s_cr, s0, s1, s2, s3);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }

    void process_YCbCr_16x8_neon(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        u8 result[64 * 4];

        state->idct(result +   0, data +   0, state->block[0].qt); // Y0
        state->idct(result +  64, data +  64, state->block[1].qt); // Y1
        state->idct(result + 128, data + 128, state->block[2].qt); // Cb
        state->idct(result + 192, data + 192, state->block[3].qt); // Cr

        const uint8x8_t tosigned = vdup_n_u8(0x80);
        const int16x8_t s0 = vdupq_n_s16(JPEG_FIXED( 1.40200));
        const int16x8_t s1 = vdupq_n_s16(JPEG_FIXED(-0.71414));
        const int16x8_t s2 = vdupq_n_s16(JPEG_FIXED(-0.34414));
        const int16x8_t s3 = vdupq_n_s16(JPEG_FIXED( 1.77200));

        for (int y = 0; y < 8; ++y)
        {
            uint8x8_t u_y0 = vld1_u8(result + y * 8 + 0);
            uint8x8_t u_y1 = vld1_u8(result + y * 8 + 64);
            uint8x8_t u_cb = vld1_u8(result + y * 8 + 128);
            uint8x8_t u_cr = vld1_u8(result + y * 8 + 192);

            int16x8_t s_y0 = vreinterpretq_s16_u16(vshll_n_u8(u_y0, 4));
            int16x8_t s_y1 = vreinterpretq_s16_u16(vshll_n_u8(u_y1, 4));
            int16x8_t s_cb = vshll_n_s8(vreinterpret_s8_u8(vsub_u8(u_cb, tosigned)), 7);
            int16x8_t s_cr = vshll_n_s8(vreinterpret_s8_u8(vsub_u8(u_cr, tosigned)), 7);

            int16x8x2_t w_cb = vzipq_s16(s_cb, s_cb);
            int16x8x2_t w_cr = vzipq_s16(s_cr, s_cr);

            convert_ycbcr_8x1_neon(dest +  0, s_y0, w_cb.val[0], w_cr.val[0], s0, s1, s2, s3);
            convert_ycbcr_8x1_neon(dest + 32, s_y1, w_cb.val[1], w_cr.val[1], s0, s1, s2, s3);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }

    void process_YCbCr_16x16_neon(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        u8 result[64 * 6];

        state->idct(result +   0, data +   0, state->block[0].qt); // Y0
        state->idct(result + 128, data +  64, state->block[1].qt); // Y1
        state->idct(result +  64, data + 128, state->block[2].qt); // Y2
        state->idct(result + 192, data + 192, state->block[3].qt); // Y3
        state->idct(result + 256, data + 256, state->block[4].qt); // Cb
        state->idct(result + 320, data + 320, state->block[5].qt); // Cr

        const uint8x8_t tosigned = vdup_n_u8(0x80);
        const int16x8_t s0 = vdupq_n_s16(JPEG_FIXED( 1.40200));
        const int16x8_t s1 = vdupq_n_s16(JPEG_FIXED(-0.71414));
        const int16x8_t s2 = vdupq_n_s16(JPEG_FIXED(-0.34414));
        const int16x8_t s3 = vdupq_n_s16(JPEG_FIXED( 1.77200));

        for (int y = 0; y < 8; ++y)
        {
            uint8x8_t u_y0 = vld1_u8(result + y * 16 + 0);
            uint8x8_t u_y1 = vld1_u8(result + y * 16 + 128);
            uint8x8_t u_y2 = vld1_u8(result + y * 16 + 8);
            uint8x8_t u_y3 = vld1_u8(result + y * 16 + 136);
            uint8x8_t u_cb = vld1_u8(result + y * 8 + 256);
            uint8x8_t u_cr = vld1_u8(result + y * 8 + 320);

            int16x8_t s_y0 = vreinterpretq_s16_u16(vshll_n_u8(u_y0, 4));
            int16x8_t s_y1 = vreinterpretq_s16_u16(vshll_n_u8(u_y1, 4));
            int16x8_t s_y2 = vreinterpretq_s16_u16(vshll_n_u8(u_y2, 4));
            int16x8_t s_y3 = vreinterpretq_s16_u16(vshll_n_u8(u_y3, 4));
            int16x8_t s_cb = vshll_n_s8(vreinterpret_s8_u8(vsub_u8(u_cb, tosigned)), 7);
            int16x8_t s_cr = vshll_n_s8(vreinterpret_s8_u8(vsub_u8(u_cr, tosigned)), 7);

            int16x8x2_t w_cb = vzipq_s16(s_cb, s_cb);
            int16x8x2_t w_cr = vzipq_s16(s_cr, s_cr);

            convert_ycbcr_8x1_neon(dest +  0, s_y0, w_cb.val[0], w_cr.val[0], s0, s1, s2, s3);
            convert_ycbcr_8x1_neon(dest + 32, s_y1, w_cb.val[1], w_cr.val[1], s0, s1, s2, s3);
            dest += stride;

            convert_ycbcr_8x1_neon(dest +  0, s_y2, w_cb.val[0], w_cr.val[0], s0, s1, s2, s3);
            convert_ycbcr_8x1_neon(dest + 32, s_y3, w_cb.val[1], w_cr.val[1], s0, s1, s2, s3);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }

#endif

#if defined(JPEG_ENABLE_SSE2)

    // ------------------------------------------------------------------------------------------------
    // SSE2 implementation
    // ------------------------------------------------------------------------------------------------

    // The original code is by Petr Kobalicek ; WE HAVE TAKEN LIBERTIES TO ADAPT IT TO OUR USE!!!
    // https://github.com/kobalicek/simdtests
    // [License]
    // Public Domain <unlicense.org>

    constexpr int JPEG_PREC = 12;
    constexpr int JPEG_SCALE(int x) { return x << JPEG_PREC; }
    constexpr int JPEG_FIXED(double x) { return int((x * double(1 << JPEG_PREC) + 0.5)); }

#define JPEG_CONST_SSE2(x, y)  _mm_setr_epi16(x, y, x, y, x, y, x, y)

    static inline
    void convert_ycbcr_8x1_sse2(u8* dest, __m128i y, __m128i cb, __m128i cr, __m128i s0, __m128i s1, __m128i s2, __m128i rounding)
    {
        __m128i zero = _mm_setzero_si128();

        __m128i r_l = _mm_madd_epi16(_mm_unpacklo_epi16(y, cr), s0);
        __m128i r_h = _mm_madd_epi16(_mm_unpackhi_epi16(y, cr), s0);

        __m128i b_l = _mm_madd_epi16(_mm_unpacklo_epi16(y, cb), s1);
        __m128i b_h = _mm_madd_epi16(_mm_unpackhi_epi16(y, cb), s1);

        __m128i g_l = _mm_madd_epi16(_mm_unpacklo_epi16(cb, cr), s2);
        __m128i g_h = _mm_madd_epi16(_mm_unpackhi_epi16(cb, cr), s2);

        g_l = _mm_add_epi32(g_l, _mm_slli_epi32(_mm_unpacklo_epi16(y, zero), JPEG_PREC));
        g_h = _mm_add_epi32(g_h, _mm_slli_epi32(_mm_unpackhi_epi16(y, zero), JPEG_PREC));

        r_l = _mm_add_epi32(r_l, rounding);
        r_h = _mm_add_epi32(r_h, rounding);

        b_l = _mm_add_epi32(b_l, rounding);
        b_h = _mm_add_epi32(b_h, rounding);

        g_l = _mm_add_epi32(g_l, rounding);
        g_h = _mm_add_epi32(g_h, rounding);

        r_l = _mm_srai_epi32(r_l, JPEG_PREC);
        r_h = _mm_srai_epi32(r_h, JPEG_PREC);

        b_l = _mm_srai_epi32(b_l, JPEG_PREC);
        b_h = _mm_srai_epi32(b_h, JPEG_PREC);

        g_l = _mm_srai_epi32(g_l, JPEG_PREC);
        g_h = _mm_srai_epi32(g_h, JPEG_PREC);

        __m128i r = _mm_packs_epi32(r_l, r_h);
        __m128i g = _mm_packs_epi32(g_l, g_h);
        __m128i b = _mm_packs_epi32(b_l, b_h);

        r = _mm_packus_epi16(r, r);
        g = _mm_packus_epi16(g, g);
        b = _mm_packus_epi16(b, b);

        __m128i ra = _mm_unpacklo_epi8(r, _mm_cmpeq_epi8(r, r));
        __m128i bg = _mm_unpacklo_epi8(b, g);

        __m128i bgra0 = _mm_unpacklo_epi16(bg, ra);
        __m128i bgra1 = _mm_unpackhi_epi16(bg, ra);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest +  0), bgra0);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 16), bgra1);
    }

    void process_YCbCr_8x8_sse2(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        u8 result[64 * 3];

        state->idct(result +   0, data +   0, state->block[0].qt); // Y
        state->idct(result +  64, data +  64, state->block[1].qt); // Cb
        state->idct(result + 128, data + 128, state->block[2].qt); // Cr

        // color conversion
        const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
        const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
        const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
        const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
        const __m128i tosigned = _mm_set1_epi16(-128);

        for (int y = 0; y < 4; ++y)
        {
            __m128i yy = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 0));
            __m128i cb = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 64));
            __m128i cr = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 128));

            __m128i zero = _mm_setzero_si128();

            __m128i cb0 = _mm_unpacklo_epi8(cb, zero);
            __m128i cr0 = _mm_unpacklo_epi8(cr, zero);
            __m128i cb1 = _mm_unpackhi_epi8(cb, zero);
            __m128i cr1 = _mm_unpackhi_epi8(cr, zero);

            cb0 = _mm_add_epi16(cb0, tosigned);
            cr0 = _mm_add_epi16(cr0, tosigned);
            cb1 = _mm_add_epi16(cb1, tosigned);
            cr1 = _mm_add_epi16(cr1, tosigned);

            convert_ycbcr_8x1_sse2(dest, _mm_unpacklo_epi8(yy, zero), cb0, cr0, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_sse2(dest, _mm_unpackhi_epi8(yy, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }

    void process_YCbCr_8x16_sse2(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        u8 result[64 * 4];

        state->idct(result +   0, data +   0, state->block[0].qt); // Y0
        state->idct(result +  64, data +  64, state->block[1].qt); // Y1
        state->idct(result + 128, data + 128, state->block[2].qt); // Cb
        state->idct(result + 192, data + 192, state->block[3].qt); // Cr

        // color conversion
        const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
        const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
        const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
        const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
        const __m128i tosigned = _mm_set1_epi16(-128);

        for (int y = 0; y < 4; ++y)
        {
            __m128i y0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 0));
            __m128i y1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 16));
            __m128i cb = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 128));
            __m128i cr = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 192));

            __m128i zero = _mm_setzero_si128();

            __m128i cb0 = _mm_unpacklo_epi8(cb, zero);
            __m128i cr0 = _mm_unpacklo_epi8(cr, zero);
            __m128i cb1 = _mm_unpackhi_epi8(cb, zero);
            __m128i cr1 = _mm_unpackhi_epi8(cr, zero);

            cb0 = _mm_add_epi16(cb0, tosigned);
            cr0 = _mm_add_epi16(cr0, tosigned);
            cb1 = _mm_add_epi16(cb1, tosigned);
            cr1 = _mm_add_epi16(cr1, tosigned);

            convert_ycbcr_8x1_sse2(dest, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_sse2(dest, _mm_unpackhi_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_sse2(dest, _mm_unpacklo_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_sse2(dest, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }

    void process_YCbCr_16x8_sse2(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        u8 result[64 * 4];

        state->idct(result +   0, data +   0, state->block[0].qt); // Y0
        state->idct(result +  64, data +  64, state->block[1].qt); // Y1
        state->idct(result + 128, data + 128, state->block[2].qt); // Cb
        state->idct(result + 192, data + 192, state->block[3].qt); // Cr

        // color conversion
        const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
        const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
        const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
        const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
        const __m128i tosigned = _mm_set1_epi16(-128);

        for (int y = 0; y < 4; ++y)
        {
            __m128i y0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 0));
            __m128i y1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 64));
            __m128i cb = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 128));
            __m128i cr = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 192));

            __m128i zero = _mm_setzero_si128();
            __m128i cb0;
            __m128i cb1;
            __m128i cr0;
            __m128i cr1;

            cb0 = _mm_unpacklo_epi8(cb, cb);
            cr0 = _mm_unpacklo_epi8(cr, cr);

            cb1 = _mm_unpackhi_epi8(cb0, zero);
            cr1 = _mm_unpackhi_epi8(cr0, zero);
            cb0 = _mm_unpacklo_epi8(cb0, zero);
            cr0 = _mm_unpacklo_epi8(cr0, zero);

            cb0 = _mm_add_epi16(cb0, tosigned);
            cr0 = _mm_add_epi16(cr0, tosigned);
            cb1 = _mm_add_epi16(cb1, tosigned);
            cr1 = _mm_add_epi16(cr1, tosigned);

            convert_ycbcr_8x1_sse2(dest +  0, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_sse2(dest + 32, _mm_unpacklo_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            cb0 = _mm_unpackhi_epi8(cb, cb);
            cr0 = _mm_unpackhi_epi8(cr, cr);

            cb1 = _mm_unpackhi_epi8(cb0, zero);
            cr1 = _mm_unpackhi_epi8(cr0, zero);
            cb0 = _mm_unpacklo_epi8(cb0, zero);
            cr0 = _mm_unpacklo_epi8(cr0, zero);

            cb0 = _mm_add_epi16(cb0, tosigned);
            cr0 = _mm_add_epi16(cr0, tosigned);
            cb1 = _mm_add_epi16(cb1, tosigned);
            cr1 = _mm_add_epi16(cr1, tosigned);

            convert_ycbcr_8x1_sse2(dest +  0, _mm_unpackhi_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_sse2(dest + 32, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }

    void process_YCbCr_16x16_sse2(u8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        u8 result[64 * 6];

        state->idct(result +   0, data +   0, state->block[0].qt); // Y0
        state->idct(result + 128, data +  64, state->block[1].qt); // Y1
        state->idct(result +  64, data + 128, state->block[2].qt); // Y2
        state->idct(result + 192, data + 192, state->block[3].qt); // Y3
        state->idct(result + 256, data + 256, state->block[4].qt); // Cb
        state->idct(result + 320, data + 320, state->block[5].qt); // Cr

        // color conversion
        const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
        const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
        const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
        const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
        const __m128i tosigned = _mm_set1_epi16(-128);

        for (int y = 0; y < 4; ++y)
        {
            __m128i y0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 0));
            __m128i y1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 128));
            __m128i y2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 16));
            __m128i y3 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 144));
            __m128i cb = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 256));
            __m128i cr = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 320));

            __m128i zero = _mm_setzero_si128();
            __m128i cb0;
            __m128i cb1;
            __m128i cr0;
            __m128i cr1;

            cb0 = _mm_unpacklo_epi8(cb, cb);
            cr0 = _mm_unpacklo_epi8(cr, cr);

            cb1 = _mm_unpackhi_epi8(cb0, zero);
            cr1 = _mm_unpackhi_epi8(cr0, zero);
            cb0 = _mm_unpacklo_epi8(cb0, zero);
            cr0 = _mm_unpacklo_epi8(cr0, zero);

            cb0 = _mm_add_epi16(cb0, tosigned);
            cr0 = _mm_add_epi16(cr0, tosigned);
            cb1 = _mm_add_epi16(cb1, tosigned);
            cr1 = _mm_add_epi16(cr1, tosigned);

            convert_ycbcr_8x1_sse2(dest +  0, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_sse2(dest + 32, _mm_unpacklo_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_sse2(dest +  0, _mm_unpackhi_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_sse2(dest + 32, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            cb0 = _mm_unpackhi_epi8(cb, cb);
            cr0 = _mm_unpackhi_epi8(cr, cr);

            cb1 = _mm_unpackhi_epi8(cb0, zero);
            cr1 = _mm_unpackhi_epi8(cr0, zero);
            cb0 = _mm_unpacklo_epi8(cb0, zero);
            cr0 = _mm_unpacklo_epi8(cr0, zero);

            cb0 = _mm_add_epi16(cb0, tosigned);
            cr0 = _mm_add_epi16(cr0, tosigned);
            cb1 = _mm_add_epi16(cb1, tosigned);
            cr1 = _mm_add_epi16(cr1, tosigned);

            convert_ycbcr_8x1_sse2(dest +  0, _mm_unpacklo_epi8(y2, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_sse2(dest + 32, _mm_unpacklo_epi8(y3, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_sse2(dest +  0, _mm_unpackhi_epi8(y2, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_sse2(dest + 32, _mm_unpackhi_epi8(y3, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }

#endif // JPEG_ENABLE_SSE2

} // namespace jpeg
