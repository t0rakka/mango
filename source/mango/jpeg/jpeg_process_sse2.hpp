/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#ifdef INNERLOOP

void FUNCTION_8x8(u8* dest, int stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[64 * 3];

    state->idct(result +   0, data +   0, state->block[0].qt); // Y
    state->idct(result +  64, data +  64, state->block[1].qt); // Cb
    state->idct(result + 128, data + 128, state->block[2].qt); // Cr

    // color conversion
    const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
    const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
    const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
    const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
    const __m128i tosigned = _mm_set1_epi16(-128);

    for (int y = 0; y < 4; ++y)
    {
        __m128i yy = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 0));
        __m128i cb = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 64));
        __m128i cr = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 16 + 128));

        __m128i zero = _mm_setzero_si128();

        __m128i cb0 = _mm_unpacklo_epi8(cb, zero);
        __m128i cr0 = _mm_unpacklo_epi8(cr, zero);
        __m128i cb1 = _mm_unpackhi_epi8(cb, zero);
        __m128i cr1 = _mm_unpackhi_epi8(cr, zero);

        cb0 = _mm_add_epi16(cb0, tosigned);
        cr0 = _mm_add_epi16(cr0, tosigned);
        cb1 = _mm_add_epi16(cb1, tosigned);
        cr1 = _mm_add_epi16(cr1, tosigned);

        INNERLOOP(dest, _mm_unpacklo_epi8(yy, zero), cb0, cr0, s0, s1, s2, rounding);
        dest += stride;

        INNERLOOP(dest, _mm_unpackhi_epi8(yy, zero), cb1, cr1, s0, s1, s2, rounding);
        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void FUNCTION_8x16(u8* dest, int stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[64 * 4];

    state->idct(result +   0, data +   0, state->block[0].qt); // Y0
    state->idct(result +  64, data +  64, state->block[1].qt); // Y1
    state->idct(result + 128, data + 128, state->block[2].qt); // Cb
    state->idct(result + 192, data + 192, state->block[3].qt); // Cr

    // color conversion
    const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
    const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
    const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
    const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
    const __m128i tosigned = _mm_set1_epi16(-128);

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

        cb0 = _mm_add_epi16(cb0, tosigned);
        cr0 = _mm_add_epi16(cr0, tosigned);
        cb1 = _mm_add_epi16(cb1, tosigned);
        cr1 = _mm_add_epi16(cr1, tosigned);

        INNERLOOP(dest, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
        dest += stride;

        INNERLOOP(dest, _mm_unpackhi_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
        dest += stride;

        INNERLOOP(dest, _mm_unpacklo_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
        dest += stride;

        INNERLOOP(dest, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void FUNCTION_16x8(u8* dest, int stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[64 * 4];

    state->idct(result +   0, data +   0, state->block[0].qt); // Y0
    state->idct(result +  64, data +  64, state->block[1].qt); // Y1
    state->idct(result + 128, data + 128, state->block[2].qt); // Cb
    state->idct(result + 192, data + 192, state->block[3].qt); // Cr

    // color conversion
    const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
    const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
    const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
    const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
    const __m128i tosigned = _mm_set1_epi16(-128);

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

        cb0 = _mm_add_epi16(cb0, tosigned);
        cr0 = _mm_add_epi16(cr0, tosigned);
        cb1 = _mm_add_epi16(cb1, tosigned);
        cr1 = _mm_add_epi16(cr1, tosigned);

        INNERLOOP(dest +  0, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
        INNERLOOP(dest + 32, _mm_unpacklo_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
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

        INNERLOOP(dest +  0, _mm_unpackhi_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
        INNERLOOP(dest + 32, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

void FUNCTION_16x16(u8* dest, int stride, const s16* data, ProcessState* state, int width, int height)
{
    u8 result[64 * 6];

    state->idct(result +   0, data +   0, state->block[0].qt); // Y0
    state->idct(result + 128, data +  64, state->block[1].qt); // Y1
    state->idct(result +  64, data + 128, state->block[2].qt); // Y2
    state->idct(result + 192, data + 192, state->block[3].qt); // Y3
    state->idct(result + 256, data + 256, state->block[4].qt); // Cb
    state->idct(result + 320, data + 320, state->block[5].qt); // Cr

    // color conversion
    const __m128i s0 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.40200));
    const __m128i s1 = JPEG_CONST_SSE2(JPEG_FIXED( 1.00000), JPEG_FIXED( 1.77200));
    const __m128i s2 = JPEG_CONST_SSE2(JPEG_FIXED(-0.34414), JPEG_FIXED(-0.71414));
    const __m128i rounding = _mm_set1_epi32(1 << (JPEG_PREC - 1));
    const __m128i tosigned = _mm_set1_epi16(-128);

    for (int y = 0; y < 4; ++y)
    {
        __m128i y0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 0));
        __m128i y1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 128));
        __m128i y2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 16));
        __m128i y3 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(result + y * 32 + 144));
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

        cb0 = _mm_add_epi16(cb0, tosigned);
        cr0 = _mm_add_epi16(cr0, tosigned);
        cb1 = _mm_add_epi16(cb1, tosigned);
        cr1 = _mm_add_epi16(cr1, tosigned);

        INNERLOOP(dest +  0, _mm_unpacklo_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
        INNERLOOP(dest + 32, _mm_unpacklo_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
        dest += stride;

        INNERLOOP(dest +  0, _mm_unpackhi_epi8(y0, zero), cb0, cr0, s0, s1, s2, rounding);
        INNERLOOP(dest + 32, _mm_unpackhi_epi8(y1, zero), cb1, cr1, s0, s1, s2, rounding);
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

        INNERLOOP(dest +  0, _mm_unpacklo_epi8(y2, zero), cb0, cr0, s0, s1, s2, rounding);
        INNERLOOP(dest + 32, _mm_unpacklo_epi8(y3, zero), cb1, cr1, s0, s1, s2, rounding);
        dest += stride;

        INNERLOOP(dest +  0, _mm_unpackhi_epi8(y2, zero), cb0, cr0, s0, s1, s2, rounding);
        INNERLOOP(dest + 32, _mm_unpackhi_epi8(y3, zero), cb1, cr1, s0, s1, s2, rounding);
        dest += stride;
    }

    MANGO_UNREFERENCED_PARAMETER(width);
    MANGO_UNREFERENCED_PARAMETER(height);
}

#endif // INNERLOOP
