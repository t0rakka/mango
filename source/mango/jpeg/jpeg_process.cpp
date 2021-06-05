/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "jpeg.hpp"

namespace mango::jpeg
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

void process_cmyk_bgra(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[JPEG_MAX_SAMPLES_IN_MCU];

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

    const ColorSpace colorspace = state->colorspace;

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
                    u8 y0 = y_block[x];
                    u8 cb = cb_scan[x >> cb_xshift];
                    u8 cr = cr_scan[x >> cr_xshift];
                    u8 ck = ck_scan[x >> ck_xshift];

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

                    // NOTE: We should output "raw" CMYK here so that it can be mapped into
                    //       RGB with correct ICC color profile. It's mot JPEG encoder/decoder's
                    //       responsibility to handle color management.
                    //
                    // We don't have API to expose the CMYK color data so we do the worst possible
                    // thing and approximate the RGB colors. THIS IS VERY BAD!!!!!
                    //
                    // TODO: Proposed API is to expose CMYK as "packed pixels" compressed image format,
                    //       we DO have a mechanism for that. Alternatively, we could add CMYK
                    //       color type in the mango::Format. We already expose sRGB-U8 this way.
                    int r = (C * K) / 255;
                    int g = (M * K) / 255;
                    int b = (Y * K) / 255;

                    r = byteclamp(r);
                    g = byteclamp(g);
                    b = byteclamp(b);
                    d[x] = image::makeBGRA(r, g, b, 0xff);
                }
                dest_block += stride;
                y_block += 8;
            }
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

static inline void write_color_bgra(u8* dest, int y, int r, int g, int b)
{
    dest[0] = byteclamp(b + y);
    dest[1] = byteclamp(g + y);
    dest[2] = byteclamp(r + y);
    dest[3] = 0xff;
}

static inline void write_color_rgba(u8* dest, int y, int r, int g, int b)
{
    dest[0] = byteclamp(r + y);
    dest[1] = byteclamp(g + y);
    dest[2] = byteclamp(b + y);
    dest[3] = 0xff;
}

static inline void write_color_bgr(u8* dest, int y, int r, int g, int b)
{
    dest[0] = byteclamp(b + y);
    dest[1] = byteclamp(g + y);
    dest[2] = byteclamp(r + y);
}

static inline void write_color_rgb(u8* dest, int y, int r, int g, int b)
{
    dest[0] = byteclamp(r + y);
    dest[1] = byteclamp(g + y);
    dest[2] = byteclamp(b + y);
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

#if defined(MANGO_ENABLE_NEON)

// ------------------------------------------------------------------------------------------------
// NEON implementation
// ------------------------------------------------------------------------------------------------

constexpr s16 JPEG_PREC = 12;
constexpr s16 JPEG_FIXED(double x) { return s16((x * double(1 << JPEG_PREC) + 0.5)); }

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

#if defined(MANGO_ENABLE_SSE2)

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

    r = _mm_packus_epi16(r, r);
    g = _mm_packus_epi16(g, g);
    b = _mm_packus_epi16(b, b);
    __m128i a = _mm_cmpeq_epi8(r, r);

    __m128i ra = _mm_unpacklo_epi8(r, a);
    __m128i bg = _mm_unpacklo_epi8(b, g);

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

    r = _mm_packus_epi16(r, r);
    g = _mm_packus_epi16(g, g);
    b = _mm_packus_epi16(b, b);
    __m128i a = _mm_cmpeq_epi8(r, r);

    __m128i ba = _mm_unpacklo_epi8(b, a);
    __m128i rg = _mm_unpacklo_epi8(r, g);

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

#if defined(MANGO_ENABLE_SSE4_1)

static inline
void convert_ycbcr_bgr_8x1_ssse3(u8* dest, __m128i y, __m128i cb, __m128i cr, __m128i s0, __m128i s1, __m128i s2, __m128i rounding)
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
void convert_ycbcr_rgb_8x1_ssse3(u8* dest, __m128i y, __m128i cb, __m128i cr, __m128i s0, __m128i s1, __m128i s2, __m128i rounding)
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
#define INNERLOOP_YCBCR      convert_ycbcr_bgr_8x1_ssse3
#define XSTEP                24
#define FUNCTION_YCBCR_8x8   process_ycbcr_bgr_8x8_ssse3
#define FUNCTION_YCBCR_8x16  process_ycbcr_bgr_8x16_ssse3
#define FUNCTION_YCBCR_16x8  process_ycbcr_bgr_16x8_ssse3
#define FUNCTION_YCBCR_16x16 process_ycbcr_bgr_16x16_ssse3
#include "jpeg_process_sse2.hpp"
#undef INNERLOOP_YCBCR
#undef XSTEP
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

// Generate YCBCR to RGB functions
#define INNERLOOP_YCBCR      convert_ycbcr_rgb_8x1_ssse3
#define XSTEP                24
#define FUNCTION_YCBCR_8x8   process_ycbcr_rgb_8x8_ssse3
#define FUNCTION_YCBCR_8x16  process_ycbcr_rgb_8x16_ssse3
#define FUNCTION_YCBCR_16x8  process_ycbcr_rgb_16x8_ssse3
#define FUNCTION_YCBCR_16x16 process_ycbcr_rgb_16x16_ssse3
#include "jpeg_process_sse2.hpp"
#undef INNERLOOP_YCBCR
#undef XSTEP
#undef FUNCTION_YCBCR_8x8
#undef FUNCTION_YCBCR_8x16
#undef FUNCTION_YCBCR_16x8
#undef FUNCTION_YCBCR_16x16

#endif // MANGO_ENABLE_SSE4_1

} // namespace mango::jpeg
