/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#ifdef FUNCTION_GENERIC
void FUNCTION_GENERIC(u8* dest, int stride, const s16* data, ProcessState* state, int width, int height)
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
            u8* dest_block = dest + yb * 8 * stride + xb * 8 * XSTEP;
            u8* y_block = result + (yb * xsize + xb) * 64;
            u8* cb_block = cb_data + yb * (8 >> cb_yshift) * 8 + xb * (8 >> cb_xshift);
            u8* cr_block = cr_data + yb * (8 >> cr_yshift) * 8 + xb * (8 >> cr_xshift);

            // horizontal clipping limit for current block
            const int xmax = std::min(8, width - xb * 8);

            // process 8x8 block
            for (int y = 0; y < ymax; ++y)
            {
                u8* d = dest_block;
                u8* cb_scan = cb_block + (y >> cb_yshift) * 8;
                u8* cr_scan = cr_block + (y >> cr_yshift) * 8;

                for (int x = 0; x < xmax; ++x)
                {
                    u8 y0 = y_block[x];
                    u8 cb = cb_scan[x >> cb_xshift];
                    u8 cr = cr_scan[x >> cr_xshift];
                    COMPUTE_CBCR(cb, cr);
                    WRITE_COLOR(d, y0, r, g, b);
                    d += XSTEP;
                }

                dest_block += stride;
                y_block += 8;
            }
        }
    }
}
#endif

#ifdef FUNCTION_YCBCR_8x8
void FUNCTION_YCBCR_8x8(u8* dest, int stride, const s16* data, ProcessState* state, int width, int height)
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
        u8* d = dest;

        for (int x = 0; x < 8; ++x)
        {
            int y0 = s[x];
            int cb = s[x + 64];
            int cr = s[x + 128];
            COMPUTE_CBCR(cb, cr);
            WRITE_COLOR(d, y0, r, g, b);
            d += XSTEP;
        }

        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}
#endif

#ifdef FUNCTION_YCBCR_8x16
void FUNCTION_YCBCR_8x16(u8* dest, int stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[64 * 4];

    state->idct(result +   0, data +   0, state->block[0].qt); // Y0
    state->idct(result +  64, data +  64, state->block[1].qt); // Y1
    state->idct(result + 128, data + 128, state->block[2].qt); // Cb
    state->idct(result + 192, data + 192, state->block[3].qt); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        u8* d0 = dest;
        u8* d1 = dest + stride;
        const u8* s = result + y * 16;
        const u8* c = result + y * 8 + 128;

        for (int x = 0; x < 8; ++x)
        {
            int y0 = s[x + 0];
            int y1 = s[x + 8];
            int cb = c[x + 0];
            int cr = c[x + 64];
            COMPUTE_CBCR(cb, cr);
            WRITE_COLOR(d0, y0, r, g, b);
            WRITE_COLOR(d1, y1, r, g, b);
            d0 += XSTEP;
            d1 += XSTEP;
        }

        dest += stride * 2;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}
#endif

#ifdef FUNCTION_YCBCR_16x8
void FUNCTION_YCBCR_16x8(u8* dest, int stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[64 * 4];

    state->idct(result +   0, data +   0, state->block[0].qt); // Y0
    state->idct(result +  64, data +  64, state->block[1].qt); // Y1
    state->idct(result + 128, data + 128, state->block[2].qt); // Cb
    state->idct(result + 192, data + 192, state->block[3].qt); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        u8* d = dest;
        u8* s = result + y * 8;
        u8* c = result + y * 8 + 128;

        for (int x = 0; x < 4; ++x)
        {
            int y0 = s[x * 2 + 0];
            int y1 = s[x * 2 + 1];
            int cb = c[x + 0];
            int cr = c[x + 64];
            COMPUTE_CBCR(cb, cr);
            WRITE_COLOR(d + 0 * XSTEP, y0, r, g, b);
            WRITE_COLOR(d + 1 * XSTEP, y1, r, g, b);
            d += 2 * XSTEP;
        }

        for (int x = 0; x < 4; ++x)
        {
            int y0 = s[x * 2 + 64];
            int y1 = s[x * 2 + 65];
            int cb = c[x + 4];
            int cr = c[x + 68];
            COMPUTE_CBCR(cb, cr);
            WRITE_COLOR(d + 0 * XSTEP, y0, r, g, b);
            WRITE_COLOR(d + 1 * XSTEP, y1, r, g, b);
            d += 2 * XSTEP;
        }

        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}
#endif

#ifdef FUNCTION_YCBCR_16x16
void FUNCTION_YCBCR_16x16(u8* dest, int stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[64 * 6];

    state->idct(result +   0, data +   0, state->block[0].qt); // Y0
    state->idct(result +  64, data +  64, state->block[1].qt); // Y1
    state->idct(result + 128, data + 128, state->block[2].qt); // Y2
    state->idct(result + 192, data + 192, state->block[3].qt); // Y3
    state->idct(result + 256, data + 256, state->block[4].qt); // Cb
    state->idct(result + 320, data + 320, state->block[5].qt); // Cr

    // color conversion
    for (int i = 0; i < 4; ++i)
    {
        int cbcr_offset = (i & 1) * 4 + (i >> 1) * 32;
        int y_offset = i * 64;
        int dest_offset = (i >> 1) * 8 * stride + (i & 1) * 8 * XSTEP;
        const u8* ptr_cbcr = result + 256 + cbcr_offset;
        const u8* ptr_y = result + y_offset;
        u8* ptr_dest = dest + dest_offset;

        for (int y = 0; y < 4; ++y)
        {
            u8* scan = ptr_dest;

            for (int x = 0; x < 4; ++x)
            {
                u8 y0 = ptr_y[x * 2 + 0];
                u8 y1 = ptr_y[x * 2 + 1];
                u8 y2 = ptr_y[x * 2 + 8];
                u8 y3 = ptr_y[x * 2 + 9];
                u8 cb = ptr_cbcr[x + 0];
                u8 cr = ptr_cbcr[x + 64];

                COMPUTE_CBCR(cb, cr);
                WRITE_COLOR(scan + 0 * XSTEP, y0, r, g, b);
                WRITE_COLOR(scan + 1 * XSTEP, y1, r, g, b);

                u8* next = scan + stride;
                scan += 2 * XSTEP;
                WRITE_COLOR(next + 0 * XSTEP, y2, r, g, b);
                WRITE_COLOR(next + 1 * XSTEP, y3, r, g, b);
            }

            ptr_dest += stride * 2;
            ptr_y += 8 * 2;
            ptr_cbcr += 8;
        }
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}
#endif
