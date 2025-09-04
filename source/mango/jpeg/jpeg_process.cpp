/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "jpeg.hpp"

namespace mango::image::jpeg
{

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

#define COMPUTE_CBCR(cb, cr) \
    r = (cr * 91750 - 11711232) >> 16; \
    g = (cb * -22479 + cr * -46596 + 8874368) >> 16; \
    b = (cb * 115671 - 14773120) >> 16;

// ----------------------------------------------------------------------------
// Generic C++ implementation
// ----------------------------------------------------------------------------

void process_y_8bit(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[64];
    state->idct(result, data, state->block[0].qt); // Y

    for (int y = 0; y < height; ++y)
    {
        std::memcpy(dest, result + y * 8, width);
        dest += stride;
    }
}

void process_y_24bit(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[64];
    state->idct(result, data, state->block[0].qt); // Y

    stride -= width * 3;

    for (int y = 0; y < height; ++y)
    {
        const u8* s = result + y * 8;
        for (int x = 0; x < width; ++x)
        {
            u8 v = s[x];
            dest[0] = v;
            dest[1] = v;
            dest[2] = v;
            dest += 3;
        }
        dest += stride;
    }
}

void process_y_32bit(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[64];
    state->idct(result, data, state->block[0].qt); // Y

    for (int y = 0; y < height; ++y)
    {
        const u8* s = result + y * 8;
        u32* d = reinterpret_cast<u32*>(dest);
        for (int x = 0; x < width; ++x)
        {
            u32 v = s[x];
            d[x] = 0xff000000 | (v << 16) | (v << 8) | v;
        }
        dest += stride;
    }
}

void process_cmyk_rgba(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
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

    u8 temp[JPEG_MAX_SAMPLES_IN_MCU * 4];

    // first pass: expand channel data
    for (int channel = 0; channel < 4; ++channel)
    {
        int offset = state->frame[channel].offset * 64;
        int hsf = state->frame[channel].hsf;
        int vsf = state->frame[channel].vsf;

        for (int yblock = 0; yblock < vsf; ++yblock)
        {
            for (int xblock = 0; xblock < hsf; ++xblock)
            {
                u8* source = result + offset + (yblock * hsf + xblock) * 64;
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

    const ColorSpace colorspace = state->colorspace;
    const u8* lookup = math::get_linear_to_srgb_table();

    // second pass: resolve color
    for (int y = 0; y < height; ++y)
    {
        u8* source0 = temp + 0 * JPEG_MAX_SAMPLES_IN_MCU + (y * hmax * 8);
        u8* source1 = temp + 1 * JPEG_MAX_SAMPLES_IN_MCU + (y * hmax * 8);
        u8* source2 = temp + 2 * JPEG_MAX_SAMPLES_IN_MCU + (y * hmax * 8);
        u8* source3 = temp + 3 * JPEG_MAX_SAMPLES_IN_MCU + (y * hmax * 8);
        u32* d = reinterpret_cast<u32*>(dest + y * stride);

        for (int x = 0; x < width; ++x)
        {
            u8 y0 = source0[x];
            u8 cb = source1[x];
            u8 cr = source2[x];
            u8 ck = source3[x];

            int C;
            int M;
            int Y;
            int K;

            switch (colorspace)
            {
                case ColorSpace::CMYK:
                    C = y0;
                    M = cb;
                    Y = cr;
                    K = ck;
                    break;
                case ColorSpace::YCCK:
                    // convert YCCK to CMYK
                    C = 255 - (y0 + ((5734 * cr - 735052) >> 12));
                    M = 255 - (y0 + ((-1410 * cb - 2925 * cr + 554844) >> 12));
                    Y = 255 - (y0 + ((7258 * cb - 929038) >> 12));
                    K = ck;
                    break;
                default:
                case ColorSpace::YCBCR:
                    C = 0;
                    M = 0;
                    Y = 0;
                    K = 0;
                    break;
            }

            int r = (C * K + 127) / 255;
            int g = (M * K + 127) / 255;
            int b = (Y * K + 127) / 255;

            r = byteclamp(r);
            g = byteclamp(g);
            b = byteclamp(b);
            r = lookup[r];
            g = lookup[g];
            b = lookup[b];
            d[x] = image::makeRGBA(r, g, b, 0xff);
        }
    }
}

void process_ycbcr_8bit(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[JPEG_MAX_SAMPLES_IN_MCU];

    const int luma_blocks = state->blocks - 2; // don't idct two last blocks (Cb, Cr)
    for (int i = 0; i < luma_blocks; ++i)
    {
        Block& block = state->block[i];
        state->idct(result + i * 64, data, block.qt);
        data += 64;
    }

    // MCU size in blocks
    int xsize = (width + 7) / 8;
    int ysize = (height + 7) / 8;

    // process MCU
    for (int yb = 0; yb < ysize; ++yb)
    {
        // vertical clipping limit for current block
        const int ymax = std::min(8, height - yb * 8);

        for (int xb = 0; xb < xsize; ++xb)
        {
            u8* dest_block = dest + yb * 8 * stride + xb * 8 * sizeof(u8);
            u8* y_block = result + (yb * xsize + xb) * 64;

            // horizontal clipping limit for current block
            const int xmax = std::min(8, width - xb * 8);

            // process 8x8 block
            for (int y = 0; y < ymax; ++y)
            {
                std::memcpy(dest_block, y_block, xmax);
                dest_block += stride;
                y_block += 8;
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// RGB
// ------------------------------------------------------------------------------------------------

void process_rgb_bgr(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[JPEG_MAX_SAMPLES_IN_MCU];

    for (int i = 0; i < state->blocks; ++i)
    {
        Block& block = state->block[i];
        state->idct(result + i * 64, data, block.qt);
        data += 64;
    }

    for (int y = 0; y < 8; ++y)
    {
        u8* d = dest + y * stride;
        const u8* s = result + y * 8;

        for (int x = 0; x < 8; ++x)
        {
            u8 r = s[x];
            u8 g = s[x + 64];
            u8 b = s[x + 128];
            d[x * 3 + 0] = b;
            d[x * 3 + 1] = g;
            d[x * 3 + 2] = r;
        }
    }
}

void process_rgb_rgb(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[JPEG_MAX_SAMPLES_IN_MCU];

    for (int i = 0; i < state->blocks; ++i)
    {
        Block& block = state->block[i];
        state->idct(result + i * 64, data, block.qt);
        data += 64;
    }

    for (int y = 0; y < 8; ++y)
    {
        u8* d = dest + y * stride;
        const u8* s = result + y * 8;

        for (int x = 0; x < 8; ++x)
        {
            u8 r = s[x];
            u8 g = s[x + 64];
            u8 b = s[x + 128];
            d[x * 3 + 0] = r;
            d[x * 3 + 1] = g;
            d[x * 3 + 2] = b;
        }
    }
}

void process_rgb_bgra(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[JPEG_MAX_SAMPLES_IN_MCU];

    for (int i = 0; i < state->blocks; ++i)
    {
        Block& block = state->block[i];
        state->idct(result + i * 64, data, block.qt);
        data += 64;
    }

    for (int y = 0; y < 8; ++y)
    {
        u8* d = dest + y * stride;
        const u8* s = result + y * 8;

        for (int x = 0; x < 8; ++x)
        {
            u8 r = s[x];
            u8 g = s[x + 64];
            u8 b = s[x + 128];
            d[x * 4 + 0] = b;
            d[x * 4 + 1] = g;
            d[x * 4 + 2] = r;
            d[x * 4 + 3] = 0xff;
        }
    }
}

void process_rgb_rgba(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[JPEG_MAX_SAMPLES_IN_MCU];

    for (int i = 0; i < state->blocks; ++i)
    {
        Block& block = state->block[i];
        state->idct(result + i * 64, data, block.qt);
        data += 64;
    }

    for (int y = 0; y < 8; ++y)
    {
        u8* d = dest + y * stride;
        const u8* s = result + y * 8;

        for (int x = 0; x < 8; ++x)
        {
            u8 r = s[x];
            u8 g = s[x + 64];
            u8 b = s[x + 128];
            d[x * 4 + 0] = r;
            d[x * 4 + 1] = g;
            d[x * 4 + 2] = b;
            d[x * 4 + 3] = 0xff;
        }
    }
}

// ------------------------------------------------------------------------------------------------
// YCbCr
// ------------------------------------------------------------------------------------------------

static inline
void write_color_bgra(u8* dest, int y, int r, int g, int b)
{
    dest[0] = u8_clamp(b + y);
    dest[1] = u8_clamp(g + y);
    dest[2] = u8_clamp(r + y);
    dest[3] = 0xff;
}

static inline
void write_color_rgba(u8* dest, int y, int r, int g, int b)
{
    dest[0] = u8_clamp(r + y);
    dest[1] = u8_clamp(g + y);
    dest[2] = u8_clamp(b + y);
    dest[3] = 0xff;
}

static inline
void write_color_bgr(u8* dest, int y, int r, int g, int b)
{
    dest[0] = u8_clamp(b + y);
    dest[1] = u8_clamp(g + y);
    dest[2] = u8_clamp(r + y);
}

static inline
void write_color_rgb(u8* dest, int y, int r, int g, int b)
{
    dest[0] = u8_clamp(r + y);
    dest[1] = u8_clamp(g + y);
    dest[2] = u8_clamp(b + y);
}

// Generate YCBCR to BGRA functions
#define WRITE_COLOR            write_color_bgra
#define XSTEP                  4
#define FUNCTION_GENERIC       process_ycbcr_bgra
#define FUNCTION_YCBCR_8x8     process_ycbcr_bgra_8x8
#define FUNCTION_YCBCR_8x16    process_ycbcr_bgra_8x16
#define FUNCTION_YCBCR_16x8    process_ycbcr_bgra_16x8
#define FUNCTION_YCBCR_16x16   process_ycbcr_bgra_16x16
#include "jpeg_process_func.hpp"
#undef WRITE_COLOR
#undef XSTEP
#undef FUNCTION_GENERIC
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

// Generate YCBCR to RGBA functions
#define WRITE_COLOR            write_color_rgba
#define XSTEP                  4
#define FUNCTION_GENERIC       process_ycbcr_rgba
#define FUNCTION_YCBCR_8x8     process_ycbcr_rgba_8x8
#define FUNCTION_YCBCR_8x16    process_ycbcr_rgba_8x16
#define FUNCTION_YCBCR_16x8    process_ycbcr_rgba_16x8
#define FUNCTION_YCBCR_16x16   process_ycbcr_rgba_16x16
#include "jpeg_process_func.hpp"
#undef WRITE_COLOR
#undef XSTEP
#undef FUNCTION_GENERIC
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

// Generate YCBCR to BGR functions
#define WRITE_COLOR            write_color_bgr
#define XSTEP                  3
#define FUNCTION_GENERIC       process_ycbcr_bgr
#define FUNCTION_YCBCR_8x8     process_ycbcr_bgr_8x8
#define FUNCTION_YCBCR_8x16    process_ycbcr_bgr_8x16
#define FUNCTION_YCBCR_16x8    process_ycbcr_bgr_16x8
#define FUNCTION_YCBCR_16x16   process_ycbcr_bgr_16x16
#include "jpeg_process_func.hpp"
#undef WRITE_COLOR
#undef XSTEP
#undef FUNCTION_GENERIC
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

// Generate YCBCR to RGB functions
#define WRITE_COLOR            write_color_rgb
#define XSTEP                  3
#define FUNCTION_GENERIC       process_ycbcr_rgb
#define FUNCTION_YCBCR_8x8     process_ycbcr_rgb_8x8
#define FUNCTION_YCBCR_8x16    process_ycbcr_rgb_8x16
#define FUNCTION_YCBCR_16x8    process_ycbcr_rgb_16x8
#define FUNCTION_YCBCR_16x16   process_ycbcr_rgb_16x16
#include "jpeg_process_func.hpp"
#undef WRITE_COLOR
#undef XSTEP
#undef FUNCTION_GENERIC
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

#undef COMPUTE_CBCR

// ------------------------------------------------------------------------------------------------
// NEON implementation
// ------------------------------------------------------------------------------------------------

#if defined(MANGO_ENABLE_NEON)

static constexpr s16 JPEG_PREC = 12;
static constexpr s16 JPEG_FIXED(double x) { return s16((x * double(1 << JPEG_PREC) + 0.5)); }

static inline
void convert_ycbcr_bgra_8x1_neon(u8* dest, int16x8_t y, int16x8_t cb, int16x8_t cr, int16x8_t s0, int16x8_t s1, int16x8_t s2, int16x8_t s3)
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

static inline
void convert_ycbcr_rgba_8x1_neon(u8* dest, int16x8_t y, int16x8_t cb, int16x8_t cr, int16x8_t s0, int16x8_t s1, int16x8_t s2, int16x8_t s3)
{
    int16x8_t cb0 = vqdmulhq_s16(cb, s2);
    int16x8_t cr0 = vqdmulhq_s16(cr, s0);
    int16x8_t cb1 = vqdmulhq_s16(cb, s3);
    int16x8_t cr1 = vqdmulhq_s16(cr, s1);
    int16x8_t r = vaddq_s16(y, cr0);
    int16x8_t g = vaddq_s16(vaddq_s16(y, cb0), cr1);
    int16x8_t b = vaddq_s16(y, cb1);

    uint8x8x4_t packed;
    packed.val[0] = vqrshrun_n_s16(r, 4);
    packed.val[1] = vqrshrun_n_s16(g, 4);
    packed.val[2] = vqrshrun_n_s16(b, 4);
    packed.val[3] = vdup_n_u8(255);
    vst4_u8(dest, packed);
}

static inline
void convert_ycbcr_bgr_8x1_neon(u8* dest, int16x8_t y, int16x8_t cb, int16x8_t cr, int16x8_t s0, int16x8_t s1, int16x8_t s2, int16x8_t s3)
{
    int16x8_t cb0 = vqdmulhq_s16(cb, s2);
    int16x8_t cr0 = vqdmulhq_s16(cr, s0);
    int16x8_t cb1 = vqdmulhq_s16(cb, s3);
    int16x8_t cr1 = vqdmulhq_s16(cr, s1);
    int16x8_t r = vaddq_s16(y, cr0);
    int16x8_t g = vaddq_s16(vaddq_s16(y, cb0), cr1);
    int16x8_t b = vaddq_s16(y, cb1);

    uint8x8x3_t packed;
    packed.val[0] = vqrshrun_n_s16(b, 4);
    packed.val[1] = vqrshrun_n_s16(g, 4);
    packed.val[2] = vqrshrun_n_s16(r, 4);
    vst3_u8(dest, packed);
}

static inline
void convert_ycbcr_rgb_8x1_neon(u8* dest, int16x8_t y, int16x8_t cb, int16x8_t cr, int16x8_t s0, int16x8_t s1, int16x8_t s2, int16x8_t s3)
{
    int16x8_t cb0 = vqdmulhq_s16(cb, s2);
    int16x8_t cr0 = vqdmulhq_s16(cr, s0);
    int16x8_t cb1 = vqdmulhq_s16(cb, s3);
    int16x8_t cr1 = vqdmulhq_s16(cr, s1);
    int16x8_t r = vaddq_s16(y, cr0);
    int16x8_t g = vaddq_s16(vaddq_s16(y, cb0), cr1);
    int16x8_t b = vaddq_s16(y, cb1);

    uint8x8x3_t packed;
    packed.val[0] = vqrshrun_n_s16(r, 4);
    packed.val[1] = vqrshrun_n_s16(g, 4);
    packed.val[2] = vqrshrun_n_s16(b, 4);
    vst3_u8(dest, packed);
}

// Generate YCBCR to BGRA functions
#define INNERLOOP_YCBCR      convert_ycbcr_bgra_8x1_neon
#define XSTEP                32
#define FUNCTION_YCBCR_8x8   process_ycbcr_bgra_8x8_neon
#define FUNCTION_YCBCR_8x16  process_ycbcr_bgra_8x16_neon
#define FUNCTION_YCBCR_16x8  process_ycbcr_bgra_16x8_neon
#define FUNCTION_YCBCR_16x16 process_ycbcr_bgra_16x16_neon
#include "jpeg_process_neon.hpp"
#undef INNERLOOP_YCBCR
#undef XSTEP
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

// Generate YCBCR to RGBA functions
#define INNERLOOP_YCBCR      convert_ycbcr_rgba_8x1_neon
#define XSTEP                32
#define FUNCTION_YCBCR_8x8   process_ycbcr_rgba_8x8_neon
#define FUNCTION_YCBCR_8x16  process_ycbcr_rgba_8x16_neon
#define FUNCTION_YCBCR_16x8  process_ycbcr_rgba_16x8_neon
#define FUNCTION_YCBCR_16x16 process_ycbcr_rgba_16x16_neon
#include "jpeg_process_neon.hpp"
#undef INNERLOOP_YCBCR
#undef XSTEP
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

// Generate YCBCR to BGR functions
#define INNERLOOP_YCBCR      convert_ycbcr_bgr_8x1_neon
#define XSTEP                24
#define FUNCTION_YCBCR_8x8   process_ycbcr_bgr_8x8_neon
#define FUNCTION_YCBCR_8x16  process_ycbcr_bgr_8x16_neon
#define FUNCTION_YCBCR_16x8  process_ycbcr_bgr_16x8_neon
#define FUNCTION_YCBCR_16x16 process_ycbcr_bgr_16x16_neon
#include "jpeg_process_neon.hpp"
#undef INNERLOOP_YCBCR
#undef XSTEP
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

// Generate YCBCR to RGB functions
#define INNERLOOP_YCBCR      convert_ycbcr_rgb_8x1_neon
#define XSTEP                24
#define FUNCTION_YCBCR_8x8   process_ycbcr_rgb_8x8_neon
#define FUNCTION_YCBCR_8x16  process_ycbcr_rgb_8x16_neon
#define FUNCTION_YCBCR_16x8  process_ycbcr_rgb_16x8_neon
#define FUNCTION_YCBCR_16x16 process_ycbcr_rgb_16x16_neon
#include "jpeg_process_neon.hpp"
#undef INNERLOOP_YCBCR
#undef XSTEP
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

#endif // MANGO_ENABLE_NEON

// ------------------------------------------------------------------------------------------------
// SSE2 implementation
// ------------------------------------------------------------------------------------------------

#if defined(MANGO_ENABLE_SSE2)

// The original code is by Petr Kobalicek ; WE HAVE TAKEN LIBERTIES TO ADAPT IT TO OUR USE!!!
// https://github.com/kobalicek/simdtests
// [License]
// Public Domain <unlicense.org>

static constexpr s16 JPEG_PREC = 12;
static constexpr s16 JPEG_FIXED(double x) { return s16((x * double(1 << JPEG_PREC) + 0.5)); }

#define JPEG_CONST_SSE2(x, y)  _mm_setr_epi16(x, y, x, y, x, y, x, y)

static inline
void convert_ycbcr_bgra_8x1_sse2(u8* dest, __m128i y, __m128i cb, __m128i cr, __m128i s0, __m128i s1, __m128i s2, __m128i rounding)
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
    __m128i a = _mm_cmpeq_epi8(r, r);

    r = _mm_packus_epi16(r, r);
    g = _mm_packus_epi16(g, g);
    b = _mm_packus_epi16(b, b);

    __m128i bg = _mm_unpacklo_epi8(b, g);
    __m128i ra = _mm_unpacklo_epi8(r, a);
    __m128i bgra0 = _mm_unpacklo_epi16(bg, ra);
    __m128i bgra1 = _mm_unpackhi_epi16(bg, ra);

    _mm_storeu_si128(reinterpret_cast<__m128i *>(dest +  0), bgra0);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 16), bgra1);
}

static inline
void convert_ycbcr_rgba_8x1_sse2(u8* dest, __m128i y, __m128i cb, __m128i cr, __m128i s0, __m128i s1, __m128i s2, __m128i rounding)
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
    __m128i a = _mm_cmpeq_epi8(r, r);

    r = _mm_packus_epi16(r, r);
    g = _mm_packus_epi16(g, g);
    b = _mm_packus_epi16(b, b);

    __m128i rg = _mm_unpacklo_epi8(r, g);
    __m128i ba = _mm_unpacklo_epi8(b, a);
    __m128i rgba0 = _mm_unpacklo_epi16(rg, ba);
    __m128i rgba1 = _mm_unpackhi_epi16(rg, ba);

    _mm_storeu_si128(reinterpret_cast<__m128i *>(dest +  0), rgba0);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 16), rgba1);
}

// Generate YCBCR to BGRA functions
#define INNERLOOP_YCBCR      convert_ycbcr_bgra_8x1_sse2
#define XSTEP                32
#define FUNCTION_YCBCR_8x8   process_ycbcr_bgra_8x8_sse2
#define FUNCTION_YCBCR_8x16  process_ycbcr_bgra_8x16_sse2
#define FUNCTION_YCBCR_16x8  process_ycbcr_bgra_16x8_sse2
#define FUNCTION_YCBCR_16x16 process_ycbcr_bgra_16x16_sse2
#include "jpeg_process_sse2.hpp"
#undef INNERLOOP_YCBCR
#undef XSTEP
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

// Generate YCBCR to RGBA functions
#define INNERLOOP_YCBCR      convert_ycbcr_rgba_8x1_sse2
#define XSTEP                32
#define FUNCTION_YCBCR_8x8   process_ycbcr_rgba_8x8_sse2
#define FUNCTION_YCBCR_8x16  process_ycbcr_rgba_8x16_sse2
#define FUNCTION_YCBCR_16x8  process_ycbcr_rgba_16x8_sse2
#define FUNCTION_YCBCR_16x16 process_ycbcr_rgba_16x16_sse2
#include "jpeg_process_sse2.hpp"
#undef INNERLOOP_YCBCR
#undef XSTEP
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

#endif // MANGO_ENABLE_SSE2

// ------------------------------------------------------------------------------------------------
// SSE4.1 implementation
// ------------------------------------------------------------------------------------------------

#if defined(MANGO_ENABLE_SSE4_1)

static inline
void convert_ycbcr_bgr_8x1_sse41(u8* dest, __m128i y, __m128i cb, __m128i cr, __m128i s0, __m128i s1, __m128i s2, __m128i rounding)
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

    __m128i bg = _mm_unpacklo_epi64(b, g);

    constexpr u8 n = 0x80;

    __m128i bg0 = _mm_shuffle_epi8(bg, _mm_setr_epi8(0, 8, n, 1, 9, n, 2, 10, n, 3, 11, n, 4, 12, n, 5));
    __m128i bg1 = _mm_shuffle_epi8(bg, _mm_setr_epi8(13, n, 6, 14, n, 7, 15, n, n, n, n, n, n, n, n, n));
    __m128i r0 = _mm_shuffle_epi8(r, _mm_setr_epi8(n, n, 0, n, n, 1, n, n, 2, n, n, 3, n, n, 4, n));
    __m128i r1 = _mm_shuffle_epi8(r, _mm_setr_epi8(n, 5, n, n, 6, n, n, 7, n, n, n, n, n, n, n, n));
    __m128i bgr0 = _mm_or_si128(bg0, r0);
    __m128i bgr1 = _mm_or_si128(bg1, r1);

    _mm_storeu_si128(reinterpret_cast<__m128i *>(dest +  0), bgr0);
    _mm_storel_epi64(reinterpret_cast<__m128i *>(dest + 16), bgr1);
}

static inline
void convert_ycbcr_rgb_8x1_sse41(u8* dest, __m128i y, __m128i cb, __m128i cr, __m128i s0, __m128i s1, __m128i s2, __m128i rounding)
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

    __m128i rg = _mm_unpacklo_epi64(r, g);

    constexpr u8 n = 0x80;

    __m128i rg0 = _mm_shuffle_epi8(rg, _mm_setr_epi8(0, 8, n, 1, 9, n, 2, 10, n, 3, 11, n, 4, 12, n, 5));
    __m128i rg1 = _mm_shuffle_epi8(rg, _mm_setr_epi8(13, n, 6, 14, n, 7, 15, n, n, n, n, n, n, n, n, n));
    __m128i b0 = _mm_shuffle_epi8(b, _mm_setr_epi8(n, n, 0, n, n, 1, n, n, 2, n, n, 3, n, n, 4, n));
    __m128i b1 = _mm_shuffle_epi8(b, _mm_setr_epi8(n, 5, n, n, 6, n, n, 7, n, n, n, n, n, n, n, n));
    __m128i rgb0 = _mm_or_si128(rg0, b0);
    __m128i rgb1 = _mm_or_si128(rg1, b1);

    _mm_storeu_si128(reinterpret_cast<__m128i *>(dest +  0), rgb0);
    _mm_storel_epi64(reinterpret_cast<__m128i *>(dest + 16), rgb1);
}

// Generate YCBCR to BGR functions
#define INNERLOOP_YCBCR      convert_ycbcr_bgr_8x1_sse41
#define XSTEP                24
#define FUNCTION_YCBCR_8x8   process_ycbcr_bgr_8x8_sse41
#define FUNCTION_YCBCR_8x16  process_ycbcr_bgr_8x16_sse41
#define FUNCTION_YCBCR_16x8  process_ycbcr_bgr_16x8_sse41
#define FUNCTION_YCBCR_16x16 process_ycbcr_bgr_16x16_sse41
#include "jpeg_process_sse2.hpp"
#undef INNERLOOP_YCBCR
#undef XSTEP
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

// Generate YCBCR to RGB functions
#define INNERLOOP_YCBCR      convert_ycbcr_rgb_8x1_sse41
#define XSTEP                24
#define FUNCTION_YCBCR_8x8   process_ycbcr_rgb_8x8_sse41
#define FUNCTION_YCBCR_8x16  process_ycbcr_rgb_8x16_sse41
#define FUNCTION_YCBCR_16x8  process_ycbcr_rgb_16x8_sse41
#define FUNCTION_YCBCR_16x16 process_ycbcr_rgb_16x16_sse41
#include "jpeg_process_sse2.hpp"
#undef INNERLOOP_YCBCR
#undef XSTEP
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

#endif // MANGO_ENABLE_SSE4_1

// ------------------------------------------------------------------------------------------------
// AVX2 implementation
// ------------------------------------------------------------------------------------------------

#if defined(MANGO_ENABLE_AVX2)

#define JPEG_CONST_AVX2(x, y) _mm256_setr_epi16(x, y, x, y, x, y, x, y, x, y, x, y, x, y, x, y)

static inline
u8* convert_ycbcr_rgba_8x2_avx2(u8* dest, size_t stride, __m256i y, __m256i cb, __m256i cr, __m256i alpha, __m256i s0, __m256i s1, __m256i s2, __m256i rounding)
{
    __m256i zero = _mm256_setzero_si256();

    __m256i r_l = _mm256_madd_epi16(_mm256_unpacklo_epi16(y, cr), s0);
    __m256i r_h = _mm256_madd_epi16(_mm256_unpackhi_epi16(y, cr), s0);

    __m256i b_l = _mm256_madd_epi16(_mm256_unpacklo_epi16(y, cb), s1);
    __m256i b_h = _mm256_madd_epi16(_mm256_unpackhi_epi16(y, cb), s1);

    __m256i g_l = _mm256_madd_epi16(_mm256_unpacklo_epi16(cb, cr), s2);
    __m256i g_h = _mm256_madd_epi16(_mm256_unpackhi_epi16(cb, cr), s2);

    g_l = _mm256_add_epi32(g_l, _mm256_slli_epi32(_mm256_unpacklo_epi16(y, zero), JPEG_PREC));
    g_h = _mm256_add_epi32(g_h, _mm256_slli_epi32(_mm256_unpackhi_epi16(y, zero), JPEG_PREC));

    r_l = _mm256_add_epi32(r_l, rounding);
    r_h = _mm256_add_epi32(r_h, rounding);

    b_l = _mm256_add_epi32(b_l, rounding);
    b_h = _mm256_add_epi32(b_h, rounding);

    g_l = _mm256_add_epi32(g_l, rounding);
    g_h = _mm256_add_epi32(g_h, rounding);

    r_l = _mm256_srai_epi32(r_l, JPEG_PREC);
    r_h = _mm256_srai_epi32(r_h, JPEG_PREC);

    b_l = _mm256_srai_epi32(b_l, JPEG_PREC);
    b_h = _mm256_srai_epi32(b_h, JPEG_PREC);

    g_l = _mm256_srai_epi32(g_l, JPEG_PREC);
    g_h = _mm256_srai_epi32(g_h, JPEG_PREC);

    __m256i r = _mm256_packs_epi32(r_l, r_h);
    __m256i g = _mm256_packs_epi32(g_l, g_h);
    __m256i b = _mm256_packs_epi32(b_l, b_h);
    __m256i a = alpha; // can't be generated: 0xffff (-1) will become 0 with packus (signed saturation)

    __m256i rb = _mm256_packus_epi16(r, b);        // RRRRRRRRBBBBBBBB
    __m256i ga = _mm256_packus_epi16(g, a);        // GGGGGGGGAAAAAAAA
    __m256i rg = _mm256_unpacklo_epi8(rb, ga);     // RGRGRGRGRGRGRGRG
    __m256i ba = _mm256_unpackhi_epi8(rb, ga);     // BABABABABABABABA
    __m256i rgba0 = _mm256_unpacklo_epi16(rg, ba); // RGBARGBARGBARGBA
    __m256i rgba1 = _mm256_unpackhi_epi16(rg, ba); // RGBARGBARGBARGBA

    __m256i color0 = _mm256_permute2x128_si256(rgba0, rgba1, 0x20);
    __m256i color1 = _mm256_permute2x128_si256(rgba0, rgba1, 0x31);

    _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest +  0), color0);
    dest += stride;

    _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest +  0), color1);
    dest += stride;

    return dest;
}

void process_ycbcr_rgba_8x8_avx2(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[64 * 3];

    state->idct(result +   0, data +   0, state->block[0].qt); // Y
    state->idct(result +  64, data +  64, state->block[1].qt); // Cb
    state->idct(result + 128, data + 128, state->block[2].qt); // Cr

    // color conversion
    const __m256i s0 = JPEG_CONST_AVX2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
    const __m256i s1 = JPEG_CONST_AVX2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
    const __m256i s2 = JPEG_CONST_AVX2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
    const __m256i rounding = _mm256_set1_epi32(1 << (JPEG_PREC - 1));
    const __m256i tosigned = _mm256_set1_epi16(128);
    const __m256i alpha = _mm256_set1_epi16(0x00ff);

    for (int y = 0; y < 2; ++y)
    {
        __m256i y0 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(result + y * 32 + 0));
        __m256i cb = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(result + y * 32 + 64));
        __m256i cr = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(result + y * 32 + 128));

        y0 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(3, 1, 2, 0));
        cb = _mm256_permute4x64_epi64(cb, _MM_SHUFFLE(3, 1, 2, 0));
        cr = _mm256_permute4x64_epi64(cr, _MM_SHUFFLE(3, 1, 2, 0));

        __m256i zero = _mm256_setzero_si256();

        __m256i cb0 = _mm256_unpacklo_epi8(cb, zero);
        __m256i cr0 = _mm256_unpacklo_epi8(cr, zero);
        __m256i cb1 = _mm256_unpackhi_epi8(cb, zero);
        __m256i cr1 = _mm256_unpackhi_epi8(cr, zero);

        cb0 = _mm256_sub_epi16(cb0, tosigned);
        cr0 = _mm256_sub_epi16(cr0, tosigned);
        cb1 = _mm256_sub_epi16(cb1, tosigned);
        cr1 = _mm256_sub_epi16(cr1, tosigned);

        dest = convert_ycbcr_rgba_8x2_avx2(dest, stride, _mm256_unpacklo_epi8(y0, zero), cb0, cr0, alpha, s0, s1, s2, rounding);
        dest = convert_ycbcr_rgba_8x2_avx2(dest, stride, _mm256_unpackhi_epi8(y0, zero), cb1, cr1, alpha, s0, s1, s2, rounding);
    }

    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif // MANGO_ENABLE_AVX2

} // namespace mango::image::jpeg
