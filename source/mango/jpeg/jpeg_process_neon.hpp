/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#ifdef FUNCTION_YCBCR_8x8

void JPEG_COLOR_FUNC(FUNCTION_YCBCR_8x8)(u8* dest, size_t stride, const u8* spatial, ProcessState* state, int width, int height, int count, size_t xstride)
{
    const uint8x8_t tosigned = vdup_n_u8(0x80);
    const int16x8_t s0 = vdupq_n_s16(JPEG_FIXED( 1.40200));
    const int16x8_t s1 = vdupq_n_s16(JPEG_FIXED(-0.71414));
    const int16x8_t s2 = vdupq_n_s16(JPEG_FIXED(-0.34414));
    const int16x8_t s3 = vdupq_n_s16(JPEG_FIXED( 1.77200));

    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * state->spatialMCUBytes();

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
    }

    MANGO_UNREFERENCED(state);
    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif

#ifdef FUNCTION_YCBCR_8x16

void JPEG_COLOR_FUNC(FUNCTION_YCBCR_8x16)(u8* dest, size_t stride, const u8* spatial, ProcessState* state, int width, int height, int count, size_t xstride)
{
    const uint8x8_t tosigned = vdup_n_u8(0x80);
    const int16x8_t s0 = vdupq_n_s16(JPEG_FIXED( 1.40200));
    const int16x8_t s1 = vdupq_n_s16(JPEG_FIXED(-0.71414));
    const int16x8_t s2 = vdupq_n_s16(JPEG_FIXED(-0.34414));
    const int16x8_t s3 = vdupq_n_s16(JPEG_FIXED( 1.77200));

    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * state->spatialMCUBytes();

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
    }

    MANGO_UNREFERENCED(state);
    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif

#ifdef FUNCTION_YCBCR_16x8

void JPEG_COLOR_FUNC(FUNCTION_YCBCR_16x8)(u8* dest, size_t stride, const u8* spatial, ProcessState* state, int width, int height, int count, size_t xstride)
{
    const uint8x8_t tosigned = vdup_n_u8(0x80);
    const int16x8_t s0 = vdupq_n_s16(JPEG_FIXED( 1.40200));
    const int16x8_t s1 = vdupq_n_s16(JPEG_FIXED(-0.71414));
    const int16x8_t s2 = vdupq_n_s16(JPEG_FIXED(-0.34414));
    const int16x8_t s3 = vdupq_n_s16(JPEG_FIXED( 1.77200));

    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * state->spatialMCUBytes();

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
    }

    MANGO_UNREFERENCED(state);
    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif

#ifdef FUNCTION_YCBCR_16x16

void JPEG_COLOR_FUNC(FUNCTION_YCBCR_16x16)(u8* dest, size_t stride, const u8* spatial, ProcessState* state, int width, int height, int count, size_t xstride)
{
    const uint8x8_t tosigned = vdup_n_u8(0x80);
    const int16x8_t s0 = vdupq_n_s16(JPEG_FIXED( 1.40200));
    const int16x8_t s1 = vdupq_n_s16(JPEG_FIXED(-0.71414));
    const int16x8_t s2 = vdupq_n_s16(JPEG_FIXED(-0.34414));
    const int16x8_t s3 = vdupq_n_s16(JPEG_FIXED( 1.77200));

    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * state->spatialMCUBytes();

        for (int y = 0; y < 8; ++y)
        {
            const int y_base = (y >> 2) * 128 + (y & 3) * 16;
            uint8x8_t u_y0 = vld1_u8(result + y_base + 0);
            uint8x8_t u_y1 = vld1_u8(result + y_base + 64);
            uint8x8_t u_y2 = vld1_u8(result + y_base + 8);
            uint8x8_t u_y3 = vld1_u8(result + y_base + 72);
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
    }

    MANGO_UNREFERENCED(state);
    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif
