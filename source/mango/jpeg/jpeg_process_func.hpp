/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#ifdef FUNCTION_GENERIC

void FUNCTION_GENERIC(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[JPEG_MAX_SAMPLES_IN_MCU];

    for (int i = 0; i < state->blocks; ++i)
    {
        Block& block = state->block[i];
        state->idct(result + i * 64, data, block.qt);
        data += 64;
    }

    // MCU dimension in blocks
    int hmax = std::max(std::max(state->frame[0].hsf, state->frame[1].hsf), state->frame[2].hsf);
    int vmax = std::max(std::max(state->frame[0].vsf, state->frame[1].vsf), state->frame[2].vsf);

    u8 temp[JPEG_MAX_SAMPLES_IN_MCU * 3];

    // first pass: expand channel data
    for (int i = 0; i < 3; ++i)
    {
        int offset = state->frame[i].offset * 64;
        int hsf = state->frame[i].hsf;
        int vsf = state->frame[i].vsf;

        for (int yblock = 0; yblock < vsf; ++yblock)
        {
            for (int xblock = 0; xblock < hsf; ++xblock)
            {
                u8* source = result + offset + (yblock * hsf + xblock) * 64;
                u8* dest = temp + i * JPEG_MAX_SAMPLES_IN_MCU + yblock * 8 * (hmax * 8) + xblock * 8;

                if (hmax != hsf || vmax != vsf)
                {
                    int xscale = hmax / hsf;
                    int yscale = vmax / vsf;

                    for (int y = 0; y < 8; ++y)
                    {
                        for (int x = 0; x < 8; ++x)
                        {
                            u8 sample = *source++;
                            std::memset(dest + x * xscale, sample, xscale);
                        }

                        dest += hmax * 8;

                        for (int s = 1; s < yscale; ++s)
                        {
                            std::memcpy(dest, dest - hmax * 8, xscale * 8);
                            dest += hmax * 8;
                        }
                    }
                }
                else
                {
                    for (int y = 0; y < 8; ++y)
                    {
                        std::memcpy(dest, source, 8);
                        source += 8;
                        dest += hmax * 8;
                    }
                }
            }
        }
    }

    // second pass: resolve color
    for (int y = 0; y < height; ++y)
    {
        u8* source0 = temp + 0 * JPEG_MAX_SAMPLES_IN_MCU + (y * hmax * 8);
        u8* source1 = temp + 1 * JPEG_MAX_SAMPLES_IN_MCU + (y * hmax * 8);
        u8* source2 = temp + 2 * JPEG_MAX_SAMPLES_IN_MCU + (y * hmax * 8);
        u8* d = dest + y * stride;

        for (int x = 0; x < width; ++x)
        {
            u8 y0 = source0[x];
            u8 cb = source1[x];
            u8 cr = source2[x];
            int r, g, b;
            COMPUTE_CBCR(cb, cr);
            WRITE_COLOR(d, y0, r, g, b);
            d += XSTEP;
        }
    }
}

#endif

#ifdef FUNCTION_YCBCR_8x8

void FUNCTION_YCBCR_8x8(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
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
            int r, g, b;
            COMPUTE_CBCR(cb, cr);
            WRITE_COLOR(d, y0, r, g, b);
            d += XSTEP;
        }

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
            int r, g, b;
            COMPUTE_CBCR(cb, cr);
            WRITE_COLOR(d0, y0, r, g, b);
            WRITE_COLOR(d1, y1, r, g, b);
            d0 += XSTEP;
            d1 += XSTEP;
        }

        dest += stride * 2;
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
            int r, g, b;
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
            int r, g, b;
            COMPUTE_CBCR(cb, cr);
            WRITE_COLOR(d + 0 * XSTEP, y0, r, g, b);
            WRITE_COLOR(d + 1 * XSTEP, y1, r, g, b);
            d += 2 * XSTEP;
        }

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
        size_t dest_offset = (i >> 1) * 8 * stride + (i & 1) * 8 * XSTEP;
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

                int r, g, b;
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

    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif
