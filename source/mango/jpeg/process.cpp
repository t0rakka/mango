/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
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

    NOTE: we use alternate conversion formula in PACK_CMYK()
*/

#define COMPUTE_CBCR(cb, cr) \
    int r = (cr * 91750 - 11711232) >> 16; \
    int g = (cb * -22479 + cr * -46596 + 8874368) >> 16; \
    int b = (cb * 115671 - 14773120) >> 16;

#define PACK_ARGB(dest, y) \
    dest = 0xff000000 | (byteclamp(y + r) << 16) | (byteclamp(y + g) << 8) | byteclamp(y + b);

#define PACK_CMYK(dest, y, k) \
    r = ((255 - r - y) * k) / 255; \
    g = ((255 - g - y) * k) / 255; \
    b = ((255 - b - y) * k) / 255; \
    dest = 0xff000000 | (byteclamp(r) << 16) | (byteclamp(g) << 8) | byteclamp(b);

// ----------------------------------------------------------------------------
// xxx
// ----------------------------------------------------------------------------

void process_Y(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
	if (width == 8 && height == 8)
	{
	    state->idct(dest, stride, data, state->block[0].qt->table); // Y
	}
	else
	{
		uint8 result[64];
	    state->idct(result, 8, data, state->block[0].qt->table); // Y

	    for (int y = 0; y < height; ++y)
		{
			std::memcpy(dest, result + y * 8, width);
			dest += stride;
		}
	}
}

void process_YCbCr(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    uint8 result[64 * JPEG_MAX_BLOCKS_IN_MCU];

    for (int i = 0; i < state->blocks; ++i)
    {
        Block& block = state->block[i];
        state->idct(result + block.offset, block.stride, data, block.qt->table);
        data += 64;
    }

    const int offset0 = state->frame[0].offset;
    const int offset1 = state->frame[1].offset;
    const int offset2 = state->frame[2].offset;

    const int stride0 = state->block[offset0].stride;
    const int stride1 = state->block[offset1].stride;
    const int stride2 = state->block[offset2].stride;

    const int h0 = state->frame[0].Hsf;
    const int v0 = state->frame[0].Vsf;
    const int h1 = state->frame[1].Hsf;
    const int v1 = state->frame[1].Vsf;
    const int h2 = state->frame[2].Hsf;
    const int v2 = state->frame[2].Vsf;

    const uint8* p0 = result + offset0 * 64;
    const uint8* p1 = result + offset1 * 64;
    const uint8* p2 = result + offset2 * 64;

    for (int y = 0; y < height; ++y)
    {
        uint32* d = reinterpret_cast<uint32*>(dest);
        const uint8* s0 = p0 + (y >> v0) * stride0;
        const uint8* s1 = p1 + (y >> v1) * stride1;
        const uint8* s2 = p2 + (y >> v2) * stride2;

        for (int x = 0; x < width; ++x)
        {
            int cb = s1[x >> h1];
            int cr = s2[x >> h2];
            int Y = s0[x >> h0];
            COMPUTE_CBCR(cb, cr);
            PACK_ARGB(d[x], Y);
        }

        dest += stride;
    }
}

void process_CMYK(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    uint8 result[64 * JPEG_MAX_BLOCKS_IN_MCU];

    for (int i = 0; i < state->blocks; ++i)
    {
        Block& block = state->block[i];
        state->idct(result + block.offset, block.stride, data, block.qt->table);
        data += 64;
    }

    const uint8* p0 = result + state->frame[0].offset * 64;
    const uint8* p1 = result + state->frame[1].offset * 64;
    const uint8* p2 = result + state->frame[2].offset * 64;
    const uint8* p3 = result + state->frame[3].offset * 64;

    const int stride0 = state->block[state->frame[0].offset].stride;
    const int stride1 = state->block[state->frame[1].offset].stride;
    const int stride2 = state->block[state->frame[2].offset].stride;
    const int stride3 = state->block[state->frame[3].offset].stride;

    const int h0 = state->frame[0].Hsf;
    const int v0 = state->frame[0].Vsf;
    const int h1 = state->frame[1].Hsf;
    const int v1 = state->frame[1].Vsf;
    const int h2 = state->frame[2].Hsf;
    const int v2 = state->frame[2].Vsf;
    const int h3 = state->frame[3].Hsf;
    const int v3 = state->frame[3].Vsf;

    for (int y = 0; y < height; ++y)
    {
        uint32* d = reinterpret_cast<uint32*>(dest);
        const uint8* s0 = p0 + (y >> v0) * stride0;
        const uint8* s1 = p1 + (y >> v1) * stride1;
        const uint8* s2 = p2 + (y >> v2) * stride2;
        const uint8* s3 = p3 + (y >> v3) * stride3;

        for (int x = 0; x < width; ++x)
        {
            int cb = s1[x >> h1];
            int cr = s2[x >> h2];
            int K = s3[x >> h3];
            int Y = s0[x >> h0];
            COMPUTE_CBCR(cb, cr);
            PACK_CMYK(d[x], Y, K);
        }

        dest += stride;
    }
}

void process_YCbCr_8x8(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    uint8 result[64 * 3];

    state->idct(result +  0, 24, data +   0, state->block[0].qt->table); // Y
    state->idct(result +  8, 24, data +  64, state->block[1].qt->table); // Cb
    state->idct(result + 16, 24, data + 128, state->block[2].qt->table); // Cr

    // color conversion
    const uint8* src = result;

    for (int y = 0; y < 8; ++y)
    {
        const uint8* s = src;
        uint32* d = reinterpret_cast<uint32*>(dest);

        for (int x = 0; x < 8; ++x)
        {
            int cb = s[x + 8];
            int cr = s[x + 16];
            COMPUTE_CBCR(cb, cr);
            PACK_ARGB(d[x], s[x]);
        }

        src += 24;
        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void process_YCbCr_8x16(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    uint8 result[64 * 4];

    state->idct(result +   0,  8, data +   0, state->block[0].qt->table); // Y0
    state->idct(result +  64,  8, data +  64, state->block[1].qt->table); // Y1
    state->idct(result + 128, 16, data + 128, state->block[2].qt->table); // Cb
    state->idct(result + 136, 16, data + 192, state->block[3].qt->table); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        uint32* d0 = reinterpret_cast<uint32*>(dest);
        uint32* d1 = reinterpret_cast<uint32*>(dest + stride);
        const uint8* s = result + y * 16;

        for (int x = 0; x < 8; ++x)
        {
            int cb = s[x + 128];
            int cr = s[x + 136];
            COMPUTE_CBCR(cb, cr);
            PACK_ARGB(d0[x], s[x + 0]);
            PACK_ARGB(d1[x], s[x + 8]);
        }

        dest += stride * 2;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void process_YCbCr_16x8(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    uint8 result[64 * 4];

    state->idct(result +   0, 16, data +   0, state->block[0].qt->table); // Y0
    state->idct(result +   8, 16, data +  64, state->block[1].qt->table); // Y1
    state->idct(result + 128, 16, data + 128, state->block[2].qt->table); // Cb
    state->idct(result + 136, 16, data + 192, state->block[3].qt->table); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        uint32* d = reinterpret_cast<uint32*>(dest);
        uint8* s = result + y * 16;
        uint8* c = result + y * 16 + 128;

        for (int x = 0; x < 8; ++x)
        {
            int cb = c[x + 0];
            int cr = c[x + 8];
            COMPUTE_CBCR(cb, cr);
            PACK_ARGB(d[0], s[0]);
            PACK_ARGB(d[1], s[1]);
            s += 2;
            d += 2;
        }
        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void process_YCbCr_16x16(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    uint8 result[64 * 6];

    state->idct(result +   0, 16, data +   0, state->block[0].qt->table); // Y0
    state->idct(result +   8, 16, data +  64, state->block[1].qt->table); // Y1
    state->idct(result + 128, 16, data + 128, state->block[2].qt->table); // Y2
    state->idct(result + 136, 16, data + 192, state->block[3].qt->table); // Y3
    state->idct(result + 256, 16, data + 256, state->block[4].qt->table); // Cb
    state->idct(result + 264, 16, data + 320, state->block[5].qt->table); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        uint32* d0 = reinterpret_cast<uint32*>(dest);
        uint32* d1 = reinterpret_cast<uint32*>(dest + stride);
        const uint8* s = result + y * 32;
        const uint8* c = result + y * 16 + 256;

        for (int x = 0; x < 8; ++x)
        {
            int cb = c[x + 0];
            int cr = c[x + 8];
            COMPUTE_CBCR(cb, cr);
            PACK_ARGB(d0[0], s[0]);
            PACK_ARGB(d0[1], s[1]);
            PACK_ARGB(d1[0], s[16]);
            PACK_ARGB(d1[1], s[17]);
            s += 2;
            d0 += 2;
            d1 += 2;
        }

        dest += stride * 2;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

#undef COMPUTE_CBCR
#undef PACK_ARGB
#undef PACK_CMYK

#if defined(JPEG_ENABLE_SSE4)

// ----------------------------------------------------------------------------
// xxx
// ----------------------------------------------------------------------------

    // SSE TODO:
    // - support for different SSE ISA (SSE2, SSE4.1 being the most critical ones)
    // - support for aligned / unaligned stores, especially in the pack4_YCbCr()
    // - color component shuffling is free, so do it
    // - do NOT set up __m128i registers before CPU support is determined
    // - mm_mullo_epi32() replacement and profiling

/*
     // SSE2 variation
     static inline __m128i x_mm_mullo_epi32(const __m128i& a, const __m128i& b)
     {
        __m128i tmp1 = _mm_mul_epu32(a,b);
        __m128i tmp2 = _mm_mul_epu32( _mm_srli_si128(a,4), _mm_srli_si128(b,4));
        return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE (0,0,2,0)), _mm_shuffle_epi32(tmp2, _MM_SHUFFLE (0,0,2,0)));
     }
*/

#if 0
    // R,G,B,A
    static const __m128i weight_cb = _mm_set_epi32(0, 115671, -22479, 0);
    static const __m128i weight_cr = _mm_set_epi32(0, 0, -46596, 91750);
    static const __m128i weight_half = _mm_set_epi32(0, -14773120, 8874368, -11711232);
    static const __m128i g_alphamask = _mm_set1_epi32(0xff000000);
#else
    // B,G,R,A
    static const __m128i weight_cb = _mm_set_epi32(0, 0, -22479, 115671);
    static const __m128i weight_cr = _mm_set_epi32(0, 91750, -46596, 0);
    static const __m128i weight_half = _mm_set_epi32(0, -11711232, 8874368, -14773120);
    static const __m128i g_alphamask = _mm_set1_epi32(0xff000000);
#endif

#define SH(c, i) _mm_shuffle_epi32(c, 0x55 * i)
#define YCBCR(i) _mm_add_epi32( \
    _mm_mullo_epi32(weight_cb, _mm_shuffle_epi32(cb8, 0x55 * i)), \
    _mm_mullo_epi32(weight_cr, _mm_shuffle_epi32(cr8, 0x55 * i)))

static inline void compute_YCbCr_sse41(__m128i* dest, __m128i cb, __m128i cr)
{
    __m128i cb8 = _mm_cvtepu8_epi32(cb);
    __m128i cr8 = _mm_cvtepu8_epi32(cr);
    dest[0] = _mm_srai_epi32(_mm_add_epi32(YCBCR(0), weight_half), 16);
    dest[1] = _mm_srai_epi32(_mm_add_epi32(YCBCR(1), weight_half), 16);
    dest[2] = _mm_srai_epi32(_mm_add_epi32(YCBCR(2), weight_half), 16);
    dest[3] = _mm_srai_epi32(_mm_add_epi32(YCBCR(3), weight_half), 16);
}

static inline void pack4_YCbCr_sse41(uint8* dest, __m128i Y, __m128i c0, __m128i c1, __m128i c2, __m128i c3)
{
    __m128i a = _mm_packus_epi32(_mm_add_epi32(SH(Y, 0), c0), _mm_add_epi32(SH(Y, 1), c1));
    __m128i b = _mm_packus_epi32(_mm_add_epi32(SH(Y, 2), c2), _mm_add_epi32(SH(Y, 3), c3));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dest), _mm_or_si128(_mm_packus_epi16(a, b), g_alphamask));
}

// ----------------------------------------------------------------------------
// xxx
// ----------------------------------------------------------------------------

void process_YCbCr_8x8_sse41(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    uint8 result[64 * 3];

    state->idct(result +   0, 8, data +   0, state->block[0].qt->table); // Y
    state->idct(result +  64, 8, data +  64, state->block[1].qt->table); // Cb
    state->idct(result + 128, 8, data + 128, state->block[2].qt->table); // Cr

    // color conversion
    for (int y = 0; y < 8; y += 2)
    {
        const uint8* s = result + y * 8;

        __m128i y_row = _mm_load_si128(reinterpret_cast<const __m128i*>(s + 0));
        __m128i cb_row = _mm_load_si128(reinterpret_cast<const __m128i*>(s + 64));
        __m128i cr_row = _mm_load_si128(reinterpret_cast<const __m128i*>(s + 128));

        __m128i cs[4];

        compute_YCbCr_sse41(cs + 0, SH(cb_row, 0), SH(cr_row, 0));
        pack4_YCbCr_sse41(dest +  0, _mm_cvtepu8_epi32(SH(y_row, 0)), cs[0], cs[1], cs[2], cs[3]);

        compute_YCbCr_sse41(cs + 0, SH(cb_row, 1), SH(cr_row, 1));
        pack4_YCbCr_sse41(dest + 16, _mm_cvtepu8_epi32(SH(y_row, 1)), cs[0], cs[1], cs[2], cs[3]);

        dest += stride;

        compute_YCbCr_sse41(cs + 0, SH(cb_row, 2), SH(cr_row, 2));
        pack4_YCbCr_sse41(dest +  0, _mm_cvtepu8_epi32(SH(y_row, 2)), cs[0], cs[1], cs[2], cs[3]);

        compute_YCbCr_sse41(cs + 0, SH(cb_row, 3), SH(cr_row, 3));
        pack4_YCbCr_sse41(dest + 16, _mm_cvtepu8_epi32(SH(y_row, 3)), cs[0], cs[1], cs[2], cs[3]);

        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void process_YCbCr_8x16_sse41(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    uint8 result[64 * 4];

    state->idct(result +   0,  8, data +   0, state->block[0].qt->table); // Y0
    state->idct(result +  64,  8, data +  64, state->block[1].qt->table); // Y1
    state->idct(result + 128, 16, data + 128, state->block[2].qt->table); // Cb
    state->idct(result + 136, 16, data + 192, state->block[3].qt->table); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        const uint8* s = result + y * 16;

        __m128i y_row = _mm_load_si128(reinterpret_cast<const __m128i*>(s + 0));
        __m128i cbcr_row = _mm_load_si128(reinterpret_cast<const __m128i*>(s + 128));

        __m128i cs[8];

        compute_YCbCr_sse41(cs + 0, SH(cbcr_row, 0), SH(cbcr_row, 2));
        compute_YCbCr_sse41(cs + 4, SH(cbcr_row, 1), SH(cbcr_row, 3));

        pack4_YCbCr_sse41(dest +  0, _mm_cvtepu8_epi32(SH(y_row, 0)), cs[0], cs[1], cs[2], cs[3]);
        pack4_YCbCr_sse41(dest + 16, _mm_cvtepu8_epi32(SH(y_row, 1)), cs[4], cs[5], cs[6], cs[7]);

        dest += stride;

        pack4_YCbCr_sse41(dest +  0, _mm_cvtepu8_epi32(SH(y_row, 2)), cs[0], cs[1], cs[2], cs[3]);
        pack4_YCbCr_sse41(dest + 16, _mm_cvtepu8_epi32(SH(y_row, 3)), cs[4], cs[5], cs[6], cs[7]);

        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void process_YCbCr_16x8_sse41(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    uint8 result[64 * 4];

    state->idct(result +   0, 16, data +   0, state->block[0].qt->table); // Y0
    state->idct(result +   8, 16, data +  64, state->block[1].qt->table); // Y1
    state->idct(result + 128, 16, data + 128, state->block[2].qt->table); // Cb
    state->idct(result + 136, 16, data + 192, state->block[3].qt->table); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        uint8* s = result + y * 16;

        __m128i y_row = _mm_load_si128(reinterpret_cast<const __m128i*>(s));
        __m128i cbcr_row = _mm_load_si128(reinterpret_cast<const __m128i*>(s + 128));

        __m128i cs[4];

        compute_YCbCr_sse41(cs + 0, SH(cbcr_row, 0), SH(cbcr_row, 2));
        pack4_YCbCr_sse41(dest +  0, _mm_cvtepu8_epi32(SH(y_row, 0)), cs[0], cs[0], cs[1], cs[1]);
        pack4_YCbCr_sse41(dest + 16, _mm_cvtepu8_epi32(SH(y_row, 1)), cs[2], cs[2], cs[3], cs[3]);

        compute_YCbCr_sse41(cs + 0, SH(cbcr_row, 1), SH(cbcr_row, 3));
        pack4_YCbCr_sse41(dest + 32, _mm_cvtepu8_epi32(SH(y_row, 2)), cs[0], cs[0], cs[1], cs[1]);
        pack4_YCbCr_sse41(dest + 48, _mm_cvtepu8_epi32(SH(y_row, 3)), cs[2], cs[2], cs[3], cs[3]);

        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void process_YCbCr_16x16_sse41(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    uint8 result[64 * 6];

    state->idct(result +   0, 16, data +   0, state->block[0].qt->table); // Y0
    state->idct(result +   8, 16, data +  64, state->block[1].qt->table); // Y1
    state->idct(result + 128, 16, data + 128, state->block[2].qt->table); // Y2
    state->idct(result + 136, 16, data + 192, state->block[3].qt->table); // Y3
    state->idct(result + 256, 16, data + 256, state->block[4].qt->table); // Cb
    state->idct(result + 264, 16, data + 320, state->block[5].qt->table); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        const uint8* s = result + y * 32;
        const uint8* c = result + y * 16 + 256;

        __m128i y_row0 = _mm_load_si128(reinterpret_cast<const __m128i*>(s + 0));
        __m128i y_row1 = _mm_load_si128(reinterpret_cast<const __m128i*>(s + 16));
        __m128i cbcr_row = _mm_load_si128(reinterpret_cast<const __m128i*>(c));

        __m128i cs[8];

        compute_YCbCr_sse41(cs + 0, SH(cbcr_row, 0), SH(cbcr_row, 2));
        compute_YCbCr_sse41(cs + 4, SH(cbcr_row, 1), SH(cbcr_row, 3));

        pack4_YCbCr_sse41(dest +  0, _mm_cvtepu8_epi32(SH(y_row0, 0)), cs[0], cs[0], cs[1], cs[1]);
        pack4_YCbCr_sse41(dest + 16, _mm_cvtepu8_epi32(SH(y_row0, 1)), cs[2], cs[2], cs[3], cs[3]);
        pack4_YCbCr_sse41(dest + 32, _mm_cvtepu8_epi32(SH(y_row0, 2)), cs[4], cs[4], cs[5], cs[5]);
        pack4_YCbCr_sse41(dest + 48, _mm_cvtepu8_epi32(SH(y_row0, 3)), cs[6], cs[6], cs[7], cs[7]);

        dest += stride;

        pack4_YCbCr_sse41(dest +  0, _mm_cvtepu8_epi32(SH(y_row1, 0)), cs[0], cs[0], cs[1], cs[1]);
        pack4_YCbCr_sse41(dest + 16, _mm_cvtepu8_epi32(SH(y_row1, 1)), cs[2], cs[2], cs[3], cs[3]);
        pack4_YCbCr_sse41(dest + 32, _mm_cvtepu8_epi32(SH(y_row1, 2)), cs[4], cs[4], cs[5], cs[5]);
        pack4_YCbCr_sse41(dest + 48, _mm_cvtepu8_epi32(SH(y_row1, 3)), cs[6], cs[6], cs[7], cs[7]);

        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

#endif // JPEG_ENABLE_SSE4

} // namespace jpeg
