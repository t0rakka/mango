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
// Generic C++ implementation
// ----------------------------------------------------------------------------

void process_Y(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
	if (width == 8 && height == 8)
	{
        // Optimization: FULL block can be directly decoded into the target surface
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
    
    state->idct(result + 64 * 0, 8, data + 64 * 0, state->block[0].qt->table); // Y
    state->idct(result + 64 * 1, 8, data + 64 * 1, state->block[1].qt->table); // Cb
    state->idct(result + 64 * 2, 8, data + 64 * 2, state->block[2].qt->table); // Cr
    
    // color conversion
    const uint8* src = result;
    
    for (int y = 0; y < 8; ++y)
    {
        const uint8* s = src + y * 8;
        uint32* d = reinterpret_cast<uint32*>(dest);

        for (int x = 0; x < 8; ++x)
        {
            int cb = s[x + 64];
            int cr = s[x + 128];
            COMPUTE_CBCR(cb, cr);
            PACK_ARGB(d[x], s[x]);
        }
        
        dest += stride;
    }
    
    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void process_YCbCr_8x16(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    uint8 result[64 * 4];

    state->idct(result +   0, 8, data +   0, state->block[0].qt->table); // Y0
    state->idct(result +  64, 8, data +  64, state->block[1].qt->table); // Y1
    state->idct(result + 128, 8, data + 128, state->block[2].qt->table); // Cb
    state->idct(result + 192, 8, data + 192, state->block[3].qt->table); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        uint32* d0 = reinterpret_cast<uint32*>(dest);
        uint32* d1 = reinterpret_cast<uint32*>(dest + stride);
        const uint8* s = result + y * 16;
        const uint8* c = result + y * 8 + 128;

        for (int x = 0; x < 8; ++x)
        {
            int cb = c[x + 0];
            int cr = c[x + 64];
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

    state->idct(result +   0, 8, data +   0, state->block[0].qt->table); // Y0
    state->idct(result +  64, 8, data +  64, state->block[1].qt->table); // Y1
    state->idct(result + 128, 8, data + 128, state->block[2].qt->table); // Cb
    state->idct(result + 192, 8, data + 192, state->block[3].qt->table); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        uint32* d = reinterpret_cast<uint32*>(dest);
        uint8* s = result + y * 8;
        uint8* c = result + y * 8 + 128;

        for (int x = 0; x < 4; ++x)
        {
            int cb = c[x + 0];
            int cr = c[x + 64];
            COMPUTE_CBCR(cb, cr);
            PACK_ARGB(d[x * 2 + 0], s[x * 2 + 0]);
            PACK_ARGB(d[x * 2 + 1], s[x * 2 + 1]);
        }

        for (int x = 0; x < 4; ++x)
        {
            int cb = c[x + 4];
            int cr = c[x + 68];
            COMPUTE_CBCR(cb, cr);
            PACK_ARGB(d[x * 2 + 8], s[x * 2 + 64]);
            PACK_ARGB(d[x * 2 + 9], s[x * 2 + 65]);
        }

        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void process_YCbCr_16x16(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
{
    uint8 result[64 * 6];

    state->idct(result +   0, 8, data +   0, state->block[0].qt->table); // Y0
    state->idct(result + 128, 8, data +  64, state->block[1].qt->table); // Y1
    state->idct(result +  64, 8, data + 128, state->block[2].qt->table); // Y2
    state->idct(result + 192, 8, data + 192, state->block[3].qt->table); // Y3
    state->idct(result + 256, 8, data + 256, state->block[4].qt->table); // Cb
    state->idct(result + 320, 8, data + 320, state->block[5].qt->table); // Cr

    // color conversion
    for (int y = 0; y < 8; ++y)
    {
        uint32* d0 = reinterpret_cast<uint32*>(dest);
        uint32* d1 = reinterpret_cast<uint32*>(dest + stride);
        const uint8* s = result + y * 16;
        const uint8* c = result + y * 8 + 256;

        for (int x = 0; x < 4; ++x)
        {
            int cb = c[x + 0];
            int cr = c[x + 64];
            COMPUTE_CBCR(cb, cr);
            PACK_ARGB(d0[x * 2 + 0], s[x * 2 + 0]);
            PACK_ARGB(d0[x * 2 + 1], s[x * 2 + 1]);
            PACK_ARGB(d1[x * 2 + 0], s[x * 2 + 8]);
            PACK_ARGB(d1[x * 2 + 1], s[x * 2 + 9]);
        }

        for (int x = 0; x < 4; ++x)
        {
            int cb = c[x + 4];
            int cr = c[x + 68];
            COMPUTE_CBCR(cb, cr);
            PACK_ARGB(d0[x * 2 + 8], s[x * 2 + 128]);
            PACK_ARGB(d0[x * 2 + 9], s[x * 2 + 129]);
            PACK_ARGB(d1[x * 2 + 8], s[x * 2 + 136]);
            PACK_ARGB(d1[x * 2 + 9], s[x * 2 + 137]);
        }

        dest += stride * 2;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

#undef COMPUTE_CBCR
#undef PACK_ARGB
#undef PACK_CMYK

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
    void convert_ycbcr_8x1_sse2(uint8* dest, __m128i y, __m128i cb, __m128i cr, __m128i s0, __m128i s1, __m128i s2, __m128i rounding)
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

    void process_YCbCr_8x8_sse2(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        uint8 result[64 * 3];

        state->idct(result +   0, 8, data +   0, state->block[0].qt->table); // Y
        state->idct(result +  64, 8, data +  64, state->block[1].qt->table); // Cb
        state->idct(result + 128, 8, data + 128, state->block[2].qt->table); // Cr

        // color conversion
        const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
        const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
        const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
        const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
        const __m128i tosigned = _mm_set1_epi16(-128);

        for (int y = 0; y < 4; ++y)
        {
            __m128i yy = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 0));
            __m128i cb = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 64));
            __m128i cr = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 128));

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
    
    void process_YCbCr_8x16_sse2(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        uint8 result[64 * 4];
        
        state->idct(result +   0, 8, data +   0, state->block[0].qt->table); // Y0
        state->idct(result +  64, 8, data +  64, state->block[1].qt->table); // Y1
        state->idct(result + 128, 8, data + 128, state->block[2].qt->table); // Cb
        state->idct(result + 192, 8, data + 192, state->block[3].qt->table); // Cr

        // color conversion
        const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
        const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
        const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
        const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
        const __m128i tosigned = _mm_set1_epi16(-128);

        for (int y = 0; y < 4; ++y)
        {
            __m128i y0 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 0));
            __m128i y1 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 16));
            __m128i cb = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 128));
            __m128i cr = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 192));

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
    
    void process_YCbCr_16x8_sse2(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        uint8 result[64 * 4];
        
        state->idct(result +   0, 16, data +   0, state->block[0].qt->table); // Y0
        state->idct(result +   8, 16, data +  64, state->block[1].qt->table); // Y1
        state->idct(result + 128,  8, data + 128, state->block[2].qt->table); // Cb
        state->idct(result + 192,  8, data + 192, state->block[3].qt->table); // Cr
        
        // color conversion
        const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
        const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
        const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
        const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
        const __m128i tosigned = _mm_set1_epi16(-128);

        for (int y = 0; y < 4; ++y)
        {
            __m128i y0 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 0));
            __m128i y1 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 16));
            __m128i cb = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 128));
            __m128i cr = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 192));

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
            convert_ycbcr_8x1_sse2(dest + 32, _mm_unpackhi_epi8(y0, zero), cb1, cr1, s0, s1, s2, rounding);
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

            convert_ycbcr_8x1_sse2(dest +  0, _mm_unpacklo_epi8(y1, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_sse2(dest + 32, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }
    
    void process_YCbCr_16x16_sse2(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        uint8 result[64 * 6];

        state->idct(result +   0, 16, data +   0, state->block[0].qt->table); // Y0
        state->idct(result +   8, 16, data +  64, state->block[1].qt->table); // Y1
        state->idct(result + 128, 16, data + 128, state->block[2].qt->table); // Y2
        state->idct(result + 136, 16, data + 192, state->block[3].qt->table); // Y3
        state->idct(result + 256,  8, data + 256, state->block[4].qt->table); // Cb
        state->idct(result + 320,  8, data + 320, state->block[5].qt->table); // Cr

        // color conversion
        const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
        const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
        const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
        const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
        const __m128i tosigned = _mm_set1_epi16(-128);

        for (int y = 0; y < 4; ++y)
        {
            __m128i y0 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 64 + 0));
            __m128i y1 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 64 + 16));
            __m128i y2 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 64 + 32));
            __m128i y3 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 64 + 48));
            __m128i cb = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 256));
            __m128i cr = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 320));

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
            convert_ycbcr_8x1_sse2(dest + 32, _mm_unpackhi_epi8(y0, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_sse2(dest +  0, _mm_unpacklo_epi8(y1, zero), cb0, cr0, s0, s1, s2, rounding);
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
            convert_ycbcr_8x1_sse2(dest + 32, _mm_unpackhi_epi8(y2, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_sse2(dest +  0, _mm_unpacklo_epi8(y3, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_sse2(dest + 32, _mm_unpackhi_epi8(y3, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }

#endif // JPEG_ENABLE_SSE2

#if defined(JPEG_ENABLE_AVX2)
    
    // ------------------------------------------------------------------------------------------------
    // AVX2 implementation
    // ------------------------------------------------------------------------------------------------

#define JPEG_CONST_AVX2(x, y)  _mm256_setr_epi16(x, y, x, y, x, y, x, y, x, y, x, y, x, y, x, y)

    static inline
    void convert_ycbcr_8x1_avx2(uint8* dest, __m128i y, __m128i cb, __m128i cr, __m256i s0, __m256i s1, __m256i s2, __m256i rounding)
    {
        __m128i zero = _mm_setzero_si128();

        // TODO: optimize
        __m256i yy00 = _mm256_setr_m128i(_mm_unpacklo_epi16(y, zero), _mm_unpackhi_epi16(y, zero));
        __m256i yycr = _mm256_setr_m128i(_mm_unpacklo_epi16(y, cr), _mm_unpackhi_epi16(y, cr));
        __m256i yycb = _mm256_setr_m128i(_mm_unpacklo_epi16(y, cb), _mm_unpackhi_epi16(y, cb));
        __m256i cbcr2 = _mm256_setr_m128i(_mm_unpacklo_epi16(cb, cr), _mm_unpackhi_epi16(cb, cr));

        __m256i r = _mm256_madd_epi16(yycr, s0);
        __m256i b = _mm256_madd_epi16(yycb, s1);
        __m256i g = _mm256_madd_epi16(cbcr2, s2);
        __m256i a = _mm256_cmpeq_epi8(r, r);

        g = _mm256_add_epi32(g, _mm256_slli_epi32(yy00, JPEG_PREC));
        r = _mm256_add_epi32(r, rounding);
        b = _mm256_add_epi32(b, rounding);
        g = _mm256_add_epi32(g, rounding);
        r = _mm256_srai_epi32(r, JPEG_PREC);
        b = _mm256_srai_epi32(b, JPEG_PREC);
        g = _mm256_srai_epi32(g, JPEG_PREC);

        __m256i bg0 = _mm256_unpacklo_epi32(b, g);
        __m256i ra0 = _mm256_unpacklo_epi32(r, a);
        __m256i bg1 = _mm256_unpackhi_epi32(b, g);
        __m256i ra1 = _mm256_unpackhi_epi32(r, a);

        __m256i bg = _mm256_packs_epi32(bg0, bg1);
        __m256i ra = _mm256_packs_epi32(ra0, ra1);
        bg = _mm256_packus_epi16(bg, bg);
        ra = _mm256_packus_epi16(ra, ra);

        __m256i bgra = _mm256_unpacklo_epi16(bg, ra);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest + 0), bgra);
    }

    void process_YCbCr_8x8_avx2(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        uint8 result[64 * 3];
        
        state->idct(result +   0, 8, data +   0, state->block[0].qt->table); // Y
        state->idct(result +  64, 8, data +  64, state->block[1].qt->table); // Cb
        state->idct(result + 128, 8, data + 128, state->block[2].qt->table); // Cr

        // color conversion
        const __m256i s0 = JPEG_CONST_AVX2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
        const __m256i s1 = JPEG_CONST_AVX2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
        const __m256i s2 = JPEG_CONST_AVX2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
        const __m256i rounding = _mm256_set1_epi32(1 << (JPEG_PREC - 1));
        const __m128i tosigned = _mm_set1_epi16(-128);

        for (int y = 0; y < 4; ++y)
        {
            __m128i yy = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 0));
            __m128i cb = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 64));
            __m128i cr = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 128));

            __m128i zero = _mm_setzero_si128();

            __m128i cb0 = _mm_unpacklo_epi8(cb, zero);
            __m128i cr0 = _mm_unpacklo_epi8(cr, zero);
            __m128i cb1 = _mm_unpackhi_epi8(cb, zero);
            __m128i cr1 = _mm_unpackhi_epi8(cr, zero);

            cb0 = _mm_add_epi16(cb0, tosigned);
            cr0 = _mm_add_epi16(cr0, tosigned);
            cb1 = _mm_add_epi16(cb1, tosigned);
            cr1 = _mm_add_epi16(cr1, tosigned);

            convert_ycbcr_8x1_avx2(dest, _mm_unpacklo_epi8(yy, zero), cb0, cr0, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_avx2(dest, _mm_unpackhi_epi8(yy, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }
    
    void process_YCbCr_8x16_avx2(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        uint8 result[64 * 4];
        
        state->idct(result +   0, 8, data +   0, state->block[0].qt->table); // Y0
        state->idct(result +  64, 8, data +  64, state->block[1].qt->table); // Y1
        state->idct(result + 128, 8, data + 128, state->block[2].qt->table); // Cb
        state->idct(result + 192, 8, data + 192, state->block[3].qt->table); // Cr

        // color conversion
        const __m256i s0 = JPEG_CONST_AVX2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
        const __m256i s1 = JPEG_CONST_AVX2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
        const __m256i s2 = JPEG_CONST_AVX2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
        const __m256i rounding = _mm256_set1_epi32(1 << (JPEG_PREC - 1));
        const __m128i tosigned = _mm_set1_epi16(-128);

        for (int y = 0; y < 4; ++y)
        {
            __m128i y0 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 0));
            __m128i y1 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 16));
            __m128i cb = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 128));
            __m128i cr = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 192));
            
            __m128i zero = _mm_setzero_si128();

            __m128i cb0 = _mm_unpacklo_epi8(cb, zero);
            __m128i cr0 = _mm_unpacklo_epi8(cr, zero);
            __m128i cb1 = _mm_unpackhi_epi8(cb, zero);
            __m128i cr1 = _mm_unpackhi_epi8(cr, zero);

            cb0 = _mm_add_epi16(cb0, tosigned);
            cr0 = _mm_add_epi16(cr0, tosigned);
            cb1 = _mm_add_epi16(cb1, tosigned);
            cr1 = _mm_add_epi16(cr1, tosigned);

            convert_ycbcr_8x1_avx2(dest, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_avx2(dest, _mm_unpackhi_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_avx2(dest, _mm_unpacklo_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_avx2(dest, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }
    
    void process_YCbCr_16x8_avx2(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        uint8 result[64 * 4];
        
        state->idct(result +   0, 16, data +   0, state->block[0].qt->table); // Y0
        state->idct(result +   8, 16, data +  64, state->block[1].qt->table); // Y1
        state->idct(result + 128,  8, data + 128, state->block[2].qt->table); // Cb
        state->idct(result + 192,  8, data + 192, state->block[3].qt->table); // Cr

        // color conversion
        const __m256i s0 = JPEG_CONST_AVX2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
        const __m256i s1 = JPEG_CONST_AVX2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
        const __m256i s2 = JPEG_CONST_AVX2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
        const __m256i rounding = _mm256_set1_epi32(1 << (JPEG_PREC - 1));
        const __m128i tosigned = _mm_set1_epi16(-128);

        for (int y = 0; y < 4; ++y)
        {
            __m128i y0 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 0));
            __m128i y1 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 16));
            __m128i cb = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 128));
            __m128i cr = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 192));
            
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

            convert_ycbcr_8x1_avx2(dest +  0, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_avx2(dest + 32, _mm_unpackhi_epi8(y0, zero), cb1, cr1, s0, s1, s2, rounding);
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

            convert_ycbcr_8x1_avx2(dest +  0, _mm_unpacklo_epi8(y1, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_avx2(dest + 32, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }
    
    void process_YCbCr_16x16_avx2(uint8* dest, int stride, const BlockType* data, ProcessState* state, int width, int height)
    {
        uint8 result[64 * 6];
        
        state->idct(result +   0, 16, data +   0, state->block[0].qt->table); // Y0
        state->idct(result +   8, 16, data +  64, state->block[1].qt->table); // Y1
        state->idct(result + 128, 16, data + 128, state->block[2].qt->table); // Y2
        state->idct(result + 136, 16, data + 192, state->block[3].qt->table); // Y3
        state->idct(result + 256,  8, data + 256, state->block[4].qt->table); // Cb
        state->idct(result + 320,  8, data + 320, state->block[5].qt->table); // Cr

        // color conversion
        const __m256i s0 = JPEG_CONST_AVX2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
        const __m256i s1 = JPEG_CONST_AVX2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
        const __m256i s2 = JPEG_CONST_AVX2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
        const __m256i rounding = _mm256_set1_epi32(1 << (JPEG_PREC - 1));
        const __m128i tosigned = _mm_set1_epi16(-128);

        for (int y = 0; y < 4; ++y)
        {
            __m128i y0 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 64 + 0));
            __m128i y1 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 64 + 16));
            __m128i y2 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 64 + 32));
            __m128i y3 = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 64 + 48));
            __m128i cb = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 256));
            __m128i cr = _mm_load_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 320));

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

            convert_ycbcr_8x1_avx2(dest +  0, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_avx2(dest + 32, _mm_unpackhi_epi8(y0, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_avx2(dest +  0, _mm_unpacklo_epi8(y1, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_avx2(dest + 32, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
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

            convert_ycbcr_8x1_avx2(dest +  0, _mm_unpacklo_epi8(y2, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_avx2(dest + 32, _mm_unpackhi_epi8(y2, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            convert_ycbcr_8x1_avx2(dest +  0, _mm_unpacklo_epi8(y3, zero), cb0, cr0, s0, s1, s2, rounding);
            convert_ycbcr_8x1_avx2(dest + 32, _mm_unpackhi_epi8(y3, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;
        }

        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }

#endif // JPEG_ENABLE_AVX2

} // namespace jpeg
