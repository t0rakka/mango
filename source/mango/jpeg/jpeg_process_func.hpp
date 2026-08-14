/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#ifdef FUNCTION_GENERIC

void FUNCTION_GENERIC(u8* dest, size_t stride, const u8* spatial, ProcessState* state, int width, int height, int count, size_t xstride)
{
    int hmax = std::max(std::max(state->frame[0].hsf, state->frame[1].hsf), state->frame[2].hsf);
    int vmax = std::max(std::max(state->frame[0].vsf, state->frame[1].vsf), state->frame[2].vsf);

    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * JPEG_MAX_SAMPLES_IN_MCU;

        u8 temp[JPEG_MAX_SAMPLES_IN_MCU * 3];

        // first pass: expand channel data
        for (int channel = 0; channel < 3; ++channel)
        {
            int offset = state->frame[channel].offset * 64;
            int hsf = state->frame[channel].hsf;
            int vsf = state->frame[channel].vsf;

            for (int yblock = 0; yblock < vsf; ++yblock)
            {
                for (int xblock = 0; xblock < hsf; ++xblock)
                {
                    const u8* source = result + offset + (yblock * hsf + xblock) * 64;
                    u8* d = temp + channel * JPEG_MAX_SAMPLES_IN_MCU + yblock * 8 * (hmax * 8) + xblock * 8;

                    if (hmax != hsf || vmax != vsf)
                    {
                        int xscale = hmax / hsf;
                        int yscale = vmax / vsf;

                        for (int y = 0; y < 8; ++y)
                        {
                            for (int x = 0; x < 8; ++x)
                            {
                                u8 sample = *source++;
                                std::memset(d + x * xscale, sample, xscale);
                            }

                            d += hmax * 8;

                            for (int s = 1; s < yscale; ++s)
                            {
                                std::memcpy(d, d - hmax * 8, xscale * 8);
                                d += hmax * 8;
                            }
                        }
                    }
                    else
                    {
                        for (int y = 0; y < 8; ++y)
                        {
                            std::memcpy(d, source, 8);
                            source += 8;
                            d += hmax * 8;
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
}

#endif

#ifdef FUNCTION_YCBCR_8x8

void JPEG_COLOR_FUNC(FUNCTION_YCBCR_8x8)(u8* dest, size_t stride, const u8* spatial, ProcessState* state, int width, int height, int count, size_t xstride)
{
    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * JPEG_MAX_SAMPLES_IN_MCU;

        for (int y = 0; y < 8; ++y)
        {
            const u8* s = result + y * 8;
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
    }

    MANGO_UNREFERENCED(state);
    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif

#ifdef FUNCTION_YCBCR_8x16

void JPEG_COLOR_FUNC(FUNCTION_YCBCR_8x16)(u8* dest, size_t stride, const u8* spatial, ProcessState* state, int width, int height, int count, size_t xstride)
{
    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * JPEG_MAX_SAMPLES_IN_MCU;

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
    }

    MANGO_UNREFERENCED(state);
    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif

#ifdef FUNCTION_YCBCR_16x8

void JPEG_COLOR_FUNC(FUNCTION_YCBCR_16x8)(u8* dest, size_t stride, const u8* spatial, ProcessState* state, int width, int height, int count, size_t xstride)
{
    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * JPEG_MAX_SAMPLES_IN_MCU;

        for (int y = 0; y < 8; ++y)
        {
            u8* d = dest;
            const u8* s = result + y * 8;
            const u8* c = result + y * 8 + 128;

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
    }

    MANGO_UNREFERENCED(state);
    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif

#ifdef FUNCTION_YCBCR_16x16

void JPEG_COLOR_FUNC(FUNCTION_YCBCR_16x16)(u8* dest, size_t stride, const u8* spatial, ProcessState* state, int width, int height, int count, size_t xstride)
{
    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * JPEG_MAX_SAMPLES_IN_MCU;

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
    }

    MANGO_UNREFERENCED(state);
    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif
