/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#ifdef FUNCTION_YCBCR_8x8

void JPEG_COLOR_FUNC(FUNCTION_YCBCR_8x8)(u8* dest, size_t stride, const u8* spatial, ProcessState* state, int width, int height, int count, size_t xstride)
{
    // color conversion
    const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
    const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
    const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
    const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
    const __m128i tosigned = _mm_set1_epi16(128);

    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * state->spatialMCUBytes();

        for (int y = 0; y < 4; ++y)
        {
            __m128i y0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 0));
            __m128i cb = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 64));
            __m128i cr = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 128));

            __m128i zero = _mm_setzero_si128();

            __m128i cb0 = _mm_unpacklo_epi8(cb, zero);
            __m128i cr0 = _mm_unpacklo_epi8(cr, zero);
            __m128i cb1 = _mm_unpackhi_epi8(cb, zero);
            __m128i cr1 = _mm_unpackhi_epi8(cr, zero);

            cb0 = _mm_sub_epi16(cb0, tosigned);
            cr0 = _mm_sub_epi16(cr0, tosigned);
            cb1 = _mm_sub_epi16(cb1, tosigned);
            cr1 = _mm_sub_epi16(cr1, tosigned);

            INNERLOOP_YCBCR(dest, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            dest += stride;

            INNERLOOP_YCBCR(dest, _mm_unpackhi_epi8(y0, zero), cb1, cr1, s0, s1, s2, rounding);
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
    // color conversion
    const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
    const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
    const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
    const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
    const __m128i tosigned = _mm_set1_epi16(128);

    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * state->spatialMCUBytes();

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

            cb0 = _mm_sub_epi16(cb0, tosigned);
            cr0 = _mm_sub_epi16(cr0, tosigned);
            cb1 = _mm_sub_epi16(cb1, tosigned);
            cr1 = _mm_sub_epi16(cr1, tosigned);

            INNERLOOP_YCBCR(dest, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            dest += stride;

            INNERLOOP_YCBCR(dest, _mm_unpackhi_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            dest += stride;

            INNERLOOP_YCBCR(dest, _mm_unpacklo_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            INNERLOOP_YCBCR(dest, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
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
    // color conversion
    const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
    const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
    const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
    const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
    const __m128i tosigned = _mm_set1_epi16(128);

    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * state->spatialMCUBytes();

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

            cb0 = _mm_sub_epi16(cb0, tosigned);
            cr0 = _mm_sub_epi16(cr0, tosigned);
            cb1 = _mm_sub_epi16(cb1, tosigned);
            cr1 = _mm_sub_epi16(cr1, tosigned);

            INNERLOOP_YCBCR(dest + 0 * XSTEP, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            INNERLOOP_YCBCR(dest + 1 * XSTEP, _mm_unpacklo_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            cb0 = _mm_unpackhi_epi8(cb, cb);
            cr0 = _mm_unpackhi_epi8(cr, cr);

            cb1 = _mm_unpackhi_epi8(cb0, zero);
            cr1 = _mm_unpackhi_epi8(cr0, zero);
            cb0 = _mm_unpacklo_epi8(cb0, zero);
            cr0 = _mm_unpacklo_epi8(cr0, zero);

            cb0 = _mm_sub_epi16(cb0, tosigned);
            cr0 = _mm_sub_epi16(cr0, tosigned);
            cb1 = _mm_sub_epi16(cb1, tosigned);
            cr1 = _mm_sub_epi16(cr1, tosigned);

            INNERLOOP_YCBCR(dest + 0 * XSTEP, _mm_unpackhi_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            INNERLOOP_YCBCR(dest + 1 * XSTEP, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
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
    // color conversion
    const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
    const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
    const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
    const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
    const __m128i tosigned = _mm_set1_epi16(128);

    u8* origin = dest;

    for (int m = 0; m < count; ++m)
    {
        dest = origin + m * xstride;
        const u8* result = spatial + m * state->spatialMCUBytes();

        for (int y = 0; y < 4; ++y)
        {
            const int y_base = (y >> 1) * 128 + (y & 1) * 32;
            __m128i y0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y_base + 0));
            __m128i y1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y_base + 64));
            __m128i y2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y_base + 16));
            __m128i y3 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y_base + 80));
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

            cb0 = _mm_sub_epi16(cb0, tosigned);
            cr0 = _mm_sub_epi16(cr0, tosigned);
            cb1 = _mm_sub_epi16(cb1, tosigned);
            cr1 = _mm_sub_epi16(cr1, tosigned);

            INNERLOOP_YCBCR(dest + 0 * XSTEP, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            INNERLOOP_YCBCR(dest + 1 * XSTEP, _mm_unpacklo_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            INNERLOOP_YCBCR(dest + 0 * XSTEP, _mm_unpackhi_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
            INNERLOOP_YCBCR(dest + 1 * XSTEP, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            cb0 = _mm_unpackhi_epi8(cb, cb);
            cr0 = _mm_unpackhi_epi8(cr, cr);

            cb1 = _mm_unpackhi_epi8(cb0, zero);
            cr1 = _mm_unpackhi_epi8(cr0, zero);
            cb0 = _mm_unpacklo_epi8(cb0, zero);
            cr0 = _mm_unpacklo_epi8(cr0, zero);

            cb0 = _mm_sub_epi16(cb0, tosigned);
            cr0 = _mm_sub_epi16(cr0, tosigned);
            cb1 = _mm_sub_epi16(cb1, tosigned);
            cr1 = _mm_sub_epi16(cr1, tosigned);

            INNERLOOP_YCBCR(dest + 0 * XSTEP, _mm_unpacklo_epi8(y2, zero), cb0, cr0, s0, s1, s2, rounding);
            INNERLOOP_YCBCR(dest + 1 * XSTEP, _mm_unpacklo_epi8(y3, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;

            INNERLOOP_YCBCR(dest + 0 * XSTEP, _mm_unpackhi_epi8(y2, zero), cb0, cr0, s0, s1, s2, rounding);
            INNERLOOP_YCBCR(dest + 1 * XSTEP, _mm_unpackhi_epi8(y3, zero), cb1, cr1, s0, s1, s2, rounding);
            dest += stride;
        }
    }

    MANGO_UNREFERENCED(state);
    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
}

#endif
