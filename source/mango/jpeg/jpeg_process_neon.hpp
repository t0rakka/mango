/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#ifdef FUNCTION_YCBCR_8x8

void FUNCTION_YCBCR_8x8(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
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

        INNERLOOP_YCBCR(dest, s_y, s_cb, s_cr, s0, s1, s2, s3);
        dest += stride;
    }

    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif

#ifdef FUNCTION_YCBCR_8x16

void FUNCTION_YCBCR_8x16(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
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

        INNERLOOP_YCBCR(dest, s_y0, s_cb, s_cr, s0, s1, s2, s3);
        dest += stride;

        INNERLOOP_YCBCR(dest, s_y1, s_cb, s_cr, s0, s1, s2, s3);
        dest += stride;
    }

    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif

#ifdef FUNCTION_YCBCR_16x8

void FUNCTION_YCBCR_16x8(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
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

        INNERLOOP_YCBCR(dest + 0 * XSTEP, s_y0, w_cb.val[0], w_cr.val[0], s0, s1, s2, s3);
        INNERLOOP_YCBCR(dest + 1 * XSTEP, s_y1, w_cb.val[1], w_cr.val[1], s0, s1, s2, s3);
        dest += stride;
    }

    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif

#ifdef FUNCTION_YCBCR_16x16

void FUNCTION_YCBCR_16x16(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
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

        INNERLOOP_YCBCR(dest + 0 * XSTEP, s_y0, w_cb.val[0], w_cr.val[0], s0, s1, s2, s3);
        INNERLOOP_YCBCR(dest + 1 * XSTEP, s_y1, w_cb.val[1], w_cr.val[1], s0, s1, s2, s3);
        dest += stride;

        INNERLOOP_YCBCR(dest + 0 * XSTEP, s_y2, w_cb.val[0], w_cr.val[0], s0, s1, s2, s3);
        INNERLOOP_YCBCR(dest + 1 * XSTEP, s_y3, w_cb.val[1], w_cr.val[1], s0, s1, s2, s3);
        dest += stride;
    }

    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif
