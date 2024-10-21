/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    Original NeuQuant implementation (C) 1994 Anthony Becker
    Based on Self Organizing Map (SOM) neural network algorithm by Kohonen
*/
#include <mango/core/bits.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/math/math.hpp>
#include <mango/core/exception.hpp>
#include <mango/image/quantize.hpp>

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // constants
    // ------------------------------------------------------------

    enum
    {
        NETSIZE = 256,
        INITRAD = NETSIZE >> 3
    };

    #define prime1          499
    #define prime2          491
    #define prime3          487
    #define prime4          503

    #define netbiasshift	4
    #define ncycles         100

    #define intbiasshift    16
    #define intbias         ((int(1)) << intbiasshift)
    #define gammashift      10
    #define betashift       10
    #define beta            (intbias >> betashift)
    #define betagamma       (intbias << (gammashift - betashift))

    #define radiusbiasshift	6
    #define radiusbias      ((int(1)) << radiusbiasshift)
    #define initradius      (INITRAD * radiusbias)
    #define radiusdec       30

    #define alphabiasshift  10
    #define radbiasshift    8

    // ------------------------------------------------------------
    // grayscale conversion functions
    // ------------------------------------------------------------

    // linear to sRGB table
    const u8 encode_srgb_table [] =
    {
        0x00, 0x0c, 0x15, 0x1c, 0x21, 0x26, 0x2a, 0x2e, 0x31, 0x34, 0x37, 0x3a, 0x3d, 0x3f, 0x42, 0x44,
        0x46, 0x49, 0x4b, 0x4d, 0x4f, 0x51, 0x52, 0x54, 0x56, 0x58, 0x59, 0x5b, 0x5d, 0x5e, 0x60, 0x61,
        0x63, 0x64, 0x66, 0x67, 0x68, 0x6a, 0x6b, 0x6c, 0x6e, 0x6f, 0x70, 0x72, 0x73, 0x74, 0x75, 0x76,
        0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
        0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
        0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa1, 0xa2, 0xa3, 0xa4,
        0xa5, 0xa5, 0xa6, 0xa7, 0xa8, 0xa8, 0xa9, 0xaa, 0xab, 0xab, 0xac, 0xad, 0xae, 0xae, 0xaf, 0xb0,
        0xb0, 0xb1, 0xb2, 0xb3, 0xb3, 0xb4, 0xb5, 0xb5, 0xb6, 0xb7, 0xb7, 0xb8, 0xb9, 0xb9, 0xba, 0xbb,
        0xbb, 0xbc, 0xbd, 0xbd, 0xbe, 0xbf, 0xbf, 0xc0, 0xc1, 0xc1, 0xc2, 0xc2, 0xc3, 0xc4, 0xc4, 0xc5,
        0xc6, 0xc6, 0xc7, 0xc7, 0xc8, 0xc9, 0xc9, 0xca, 0xca, 0xcb, 0xcc, 0xcc, 0xcd, 0xcd, 0xce, 0xce,
        0xcf, 0xd0, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2, 0xd3, 0xd4, 0xd4, 0xd5, 0xd5, 0xd6, 0xd6, 0xd7, 0xd7,
        0xd8, 0xd9, 0xd9, 0xda, 0xda, 0xdb, 0xdb, 0xdc, 0xdc, 0xdd, 0xdd, 0xde, 0xde, 0xdf, 0xdf, 0xe0,
        0xe1, 0xe1, 0xe2, 0xe2, 0xe3, 0xe3, 0xe4, 0xe4, 0xe5, 0xe5, 0xe6, 0xe6, 0xe7, 0xe7, 0xe8, 0xe8,
        0xe9, 0xe9, 0xea, 0xea, 0xeb, 0xeb, 0xec, 0xec, 0xed, 0xed, 0xee, 0xee, 0xee, 0xef, 0xef, 0xf0,
        0xf0, 0xf1, 0xf1, 0xf2, 0xf2, 0xf3, 0xf3, 0xf4, 0xf4, 0xf5, 0xf5, 0xf6, 0xf6, 0xf6, 0xf7, 0xf7,
        0xf8, 0xf8, 0xf9, 0xf9, 0xfa, 0xfa, 0xfb, 0xfb, 0xfb, 0xfc, 0xfc, 0xfd, 0xfd, 0xfe, 0xfe, 0xff
    };

    // sRGB to linear table
    const u8 decode_srgb_table [] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
        0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b, 0x0c, 0x0c,
        0x0c, 0x0d, 0x0d, 0x0e, 0x0e, 0x0f, 0x0f, 0x10, 0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13, 0x14,
        0x14, 0x15, 0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 0x19, 0x19, 0x1a, 0x1a, 0x1b, 0x1c, 0x1c, 0x1d,
        0x1e, 0x1e, 0x1f, 0x20, 0x20, 0x21, 0x22, 0x22, 0x23, 0x24, 0x24, 0x25, 0x26, 0x27, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
        0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3d, 0x3e, 0x3f, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
        0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x5c, 0x5d, 0x5e, 0x5f, 0x61, 0x62, 0x63, 0x65, 0x66, 0x67, 0x69, 0x6a, 0x6b, 0x6d,
        0x6e, 0x70, 0x71, 0x72, 0x74, 0x75, 0x77, 0x78, 0x7a, 0x7b, 0x7d, 0x7e, 0x80, 0x81, 0x83, 0x84,
        0x86, 0x87, 0x89, 0x8a, 0x8c, 0x8e, 0x8f, 0x91, 0x93, 0x94, 0x96, 0x98, 0x99, 0x9b, 0x9d, 0x9e,
        0xa0, 0xa2, 0xa4, 0xa5, 0xa7, 0xa9, 0xab, 0xad, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xb9, 0xbb,
        0xbd, 0xbf, 0xc1, 0xc3, 0xc5, 0xc7, 0xc9, 0xcb, 0xcd, 0xcf, 0xd1, 0xd3, 0xd5, 0xd7, 0xd9, 0xdb,
        0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xed, 0xef, 0xf1, 0xf3, 0xf5, 0xf8, 0xfa, 0xfc, 0xff 
    };

    void grayscale_linear(u8* d, const u8* s, int width)
    {
        for (int x = 0; x < width; ++x)
        {
            u32 r = s[x * 4 + 0];
            u32 g = s[x * 4 + 1];
            u32 b = s[x * 4 + 2];
            d[x] = u8((r * 77 + g * 150 + b * 29) >> 8);
        }
    }

    void grayscale_linear_alpha(u8* d, const u8* s, int width)
    {
        for (int x = 0; x < width; ++x)
        {
            u32 r = s[x * 4 + 0];
            u32 g = s[x * 4 + 1];
            u32 b = s[x * 4 + 2];
            d[x * 2 + 0] = u8((r * 77 + g * 150 + b * 29) >> 8);
            d[x * 2 + 1] = s[x * 4 + 3];
        }
    }

    void grayscale_srgb(u8* d, const u8* s, int width)
    {
        for (int x = 0; x < width; ++x)
        {
            u32 r = decode_srgb_table[s[x * 4 + 0]];
            u32 g = decode_srgb_table[s[x * 4 + 1]];
            u32 b = decode_srgb_table[s[x * 4 + 2]];
            u8 luminance = u8((r * 77 + g * 150 + b * 29) >> 8);
            d[x] = encode_srgb_table[luminance];
        }
    }

    void grayscale_srgb_alpha(u8* d, const u8* s, int width)
    {
        for (int x = 0; x < width; ++x)
        {
            u32 r = decode_srgb_table[s[x * 4 + 0]];
            u32 g = decode_srgb_table[s[x * 4 + 1]];
            u32 b = decode_srgb_table[s[x * 4 + 2]];
            u8 luminance = u8((r * 77 + g * 150 + b * 29) >> 8);
            d[x * 2 + 0] = encode_srgb_table[luminance];
            d[x * 2 + 1] = s[x * 4 + 3];
        }
    }

#if defined(MANGO_ENABLE_SSE2)

    void sse2_grayscale_linear(u8* d, const u8* s, int width)
    {
        const __m128i scale_rg = _mm_setr_epi16(77, 150, 77, 150, 77, 150, 77, 150);
        const __m128i scale_b0 = _mm_setr_epi16(29, 0, 29, 0, 29, 0, 29, 0);
        const __m128i mask = _mm_set1_epi32(0xff);

        while (width >= 16)
        {
            const __m128i* ptr = reinterpret_cast<const __m128i*>(s);

            __m128i rgba0 = _mm_loadu_si128(ptr + 0);
            __m128i rgba1 = _mm_loadu_si128(ptr + 1);
            __m128i rgba2 = _mm_loadu_si128(ptr + 2);
            __m128i rgba3 = _mm_loadu_si128(ptr + 3);

            __m128i r0 = rgba0;
            __m128i r1 = rgba1;
            __m128i r2 = rgba2;
            __m128i r3 = rgba3;

            __m128i g0 = _mm_srli_epi32(rgba0, 8);
            __m128i g1 = _mm_srli_epi32(rgba1, 8);
            __m128i g2 = _mm_srli_epi32(rgba2, 8);
            __m128i g3 = _mm_srli_epi32(rgba3, 8);

            __m128i b0 = _mm_srli_epi32(rgba0, 16);
            __m128i b1 = _mm_srli_epi32(rgba1, 16);
            __m128i b2 = _mm_srli_epi32(rgba2, 16);
            __m128i b3 = _mm_srli_epi32(rgba3, 16);

            r0 = _mm_and_si128(r0, mask);
            r1 = _mm_and_si128(r1, mask);
            r2 = _mm_and_si128(r2, mask);
            r3 = _mm_and_si128(r3, mask);

            g0 = _mm_and_si128(g0, mask);
            g1 = _mm_and_si128(g1, mask);
            g2 = _mm_and_si128(g2, mask);
            g3 = _mm_and_si128(g3, mask);

            b0 = _mm_and_si128(b0, mask);
            b1 = _mm_and_si128(b1, mask);
            b2 = _mm_and_si128(b2, mask);
            b3 = _mm_and_si128(b3, mask);

            __m128i r01 = _mm_packus_epi32(r0, r1);
            __m128i g01 = _mm_packus_epi32(g0, g1);
            __m128i b01 = _mm_packus_epi32(b0, b1);

            __m128i r23 = _mm_packus_epi32(r2, r3);
            __m128i g23 = _mm_packus_epi32(g2, g3);
            __m128i b23 = _mm_packus_epi32(b2, b3);

            __m128i rg0 = _mm_madd_epi16(_mm_unpacklo_epi16(r01, g01), scale_rg);
            __m128i rg1 = _mm_madd_epi16(_mm_unpackhi_epi16(r01, g01), scale_rg);
            __m128i rg2 = _mm_madd_epi16(_mm_unpacklo_epi16(r23, g23), scale_rg);
            __m128i rg3 = _mm_madd_epi16(_mm_unpackhi_epi16(r23, g23), scale_rg);

            __m128i bx0 = _mm_madd_epi16(_mm_unpacklo_epi16(b01, b01), scale_b0);
            __m128i bx1 = _mm_madd_epi16(_mm_unpackhi_epi16(b01, b01), scale_b0);
            __m128i bx2 = _mm_madd_epi16(_mm_unpacklo_epi16(b23, b23), scale_b0);
            __m128i bx3 = _mm_madd_epi16(_mm_unpackhi_epi16(b23, b23), scale_b0);

            __m128i sum0 = _mm_add_epi32(rg0, bx0);
            __m128i sum1 = _mm_add_epi32(rg1, bx1);
            __m128i sum2 = _mm_add_epi32(rg2, bx2);
            __m128i sum3 = _mm_add_epi32(rg3, bx3);

            __m128i temp0 = _mm_srli_epi16(_mm_packus_epi32(sum0, sum1), 8);
            __m128i temp1 = _mm_srli_epi16(_mm_packus_epi32(sum2, sum3), 8);
            __m128i temp = _mm_packus_epi16(temp0, temp1);

            _mm_storeu_si128(reinterpret_cast<__m128i *>(d), temp);

            s += 64;
            d += 16;
            width -= 16;
        }

        grayscale_linear(d, s, width);
    }

#endif // defined(MANGO_ENABLE_SSE2)

#if defined(MANGO_ENABLE_SSSE3)

    void ssse3_grayscale_linear(u8* d, const u8* s, int width)
    {
        const __m128i index_r0g0 = _mm_setr_epi8(0, 0x80, 1, 0x80, 4, 0x80, 5, 0x80, 8, 0x80, 9, 0x80, 12, 0x80, 13, 0x80);
        const __m128i index_b0a0 = _mm_setr_epi8(0, 0x80, 1, 0x80, 4, 0x80, 5, 0x80, 8, 0x80, 9, 0x80, 12, 0x80, 13, 0x80);
        const __m128i scale_rg = _mm_setr_epi16(77, 150, 77, 150, 77, 150, 77, 150);
        const __m128i scale_b0 = _mm_setr_epi16(29, 0, 29, 0, 29, 0, 29, 0);

        while (width >= 16)
        {
            const __m128i* ptr = reinterpret_cast<const __m128i*>(s);

            __m128i rgba0 = _mm_loadu_si128(ptr + 0); // RGBA RGBA RGBA RGBA
            __m128i rgba1 = _mm_loadu_si128(ptr + 1); // RGBA RGBA RGBA RGBA
            __m128i rgba2 = _mm_loadu_si128(ptr + 2); // RGBA RGBA RGBA RGBA
            __m128i rgba3 = _mm_loadu_si128(ptr + 3); // RGBA RGBA RGBA RGBA

            __m128i rg0 = _mm_shuffle_epi8(rgba0, index_r0g0); // R0G0 R0G0 R0G0 R0G0
            __m128i rg1 = _mm_shuffle_epi8(rgba1, index_r0g0); // R0G0 R0G0 R0G0 R0G0
            __m128i rg2 = _mm_shuffle_epi8(rgba2, index_r0g0); // R0G0 R0G0 R0G0 R0G0
            __m128i rg3 = _mm_shuffle_epi8(rgba3, index_r0g0); // R0G0 R0G0 R0G0 R0G0

            __m128i ba0 = _mm_shuffle_epi8(rgba0, index_b0a0); // B0A0 B0A0 B0A0 B0A0
            __m128i ba1 = _mm_shuffle_epi8(rgba1, index_b0a0); // B0A0 B0A0 B0A0 B0A0
            __m128i ba2 = _mm_shuffle_epi8(rgba2, index_b0a0); // B0A0 B0A0 B0A0 B0A0
            __m128i ba3 = _mm_shuffle_epi8(rgba3, index_b0a0); // B0A0 B0A0 B0A0 B0A0

            rg0 = _mm_madd_epi16(rg0, scale_rg);
            rg1 = _mm_madd_epi16(rg1, scale_rg);
            rg2 = _mm_madd_epi16(rg2, scale_rg);
            rg3 = _mm_madd_epi16(rg3, scale_rg);

            ba0 = _mm_madd_epi16(ba0, scale_b0);
            ba1 = _mm_madd_epi16(ba1, scale_b0);
            ba2 = _mm_madd_epi16(ba2, scale_b0);
            ba3 = _mm_madd_epi16(ba3, scale_b0);

            __m128i sum0 = _mm_add_epi32(rg0, ba0);
            __m128i sum1 = _mm_add_epi32(rg1, ba1);
            __m128i sum2 = _mm_add_epi32(rg2, ba2);
            __m128i sum3 = _mm_add_epi32(rg3, ba3);

            __m128i temp0 = _mm_srli_epi16(_mm_packus_epi32(sum0, sum1), 8);
            __m128i temp1 = _mm_srli_epi16(_mm_packus_epi32(sum2, sum3), 8);
            __m128i temp = _mm_packus_epi16(temp0, temp1);

            _mm_storeu_si128(reinterpret_cast<__m128i *>(d), temp);

            s += 64;
            d += 16;
            width -= 16;
        }

        grayscale_linear(d, s, width);
    }

#endif // defined(MANGO_ENABLE_SSSE3)

#if defined(MANGO_ENABLE_AVX2)

    void avx2_grayscale_linear(u8* d, const u8* s, int width)
    {
        // The scales are negative because signed range is [-128, 127] and we want a power of two
        // so that we can shift the result; so we choose 7 bit weights. The abs() after scaling
        // brings the values back to positive so that they can be stored with unsigned saturation.
        const __m256i index_rgba = _mm256_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15,
                                                    0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15);
        const __m256i scale_rg = _mm256_setr_epi8(-38, -75, -38, -75, -38, -75, -38, -75, -38, -75, -38, -75, -38, -75, -38, -75, 
                                                  -38, -75, -38, -75, -38, -75, -38, -75, -38, -75, -38, -75, -38, -75, -38, -75);
        const __m256i scale_ba = _mm256_setr_epi8(-15, 0, -15, 0, -15, 0, -15, 0, -15, 0, -15, 0, -15, 0, -15, 0,
                                                  -15, 0, -15, 0, -15, 0, -15, 0, -15, 0, -15, 0, -15, 0, -15, 0);

        while (width >= 16)
        {
            const __m256i* ptr = reinterpret_cast<const __m256i*>(s);

            __m256i rgba0 = _mm256_loadu_si256(ptr + 0); // RGBA RGBA RGBA RGBA | RGBA RGBA RGBA RGBA
            __m256i rgba1 = _mm256_loadu_si256(ptr + 1); // RGBA RGBA RGBA RGBA | RGBA RGBA RGBA RGBA

            rgba0 = _mm256_shuffle_epi8(rgba0, index_rgba); // RG[4] BA[4] | RG[4] BA[4]
            rgba1 = _mm256_shuffle_epi8(rgba1, index_rgba); // RG[4] BA[4] | RG[4] BA[4]

            rgba0 = _mm256_permute4x64_epi64(rgba0, 0xd8); // RG[8] | BA[8]
            rgba1 = _mm256_permute4x64_epi64(rgba1, 0xd8); // RG[8] | BA[8]

            __m256i rg01 = _mm256_permute2x128_si256(rgba0, rgba1, 0x20); // RG[8] | RG[8]
            __m256i ba01 = _mm256_permute2x128_si256(rgba0, rgba1, 0x31); // BA[8] | BA[8]

            // maddubs is u8 x s8 -> s16
            __m256i rg = _mm256_maddubs_epi16(rg01, scale_rg);
            __m256i ba = _mm256_maddubs_epi16(ba01, scale_ba);
            __m256i sum = _mm256_add_epi16(rg, ba);
            sum = _mm256_srai_epi16(sum, 7);
            sum = _mm256_abs_epi16(sum);

            __m128i temp0 = _mm256_extracti128_si256(sum, 0);
            __m128i temp1 = _mm256_extracti128_si256(sum, 1);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(d), _mm_packus_epi16(temp0, temp1));

            s += 64;
            d += 16;
            width -= 16;
        }

        grayscale_linear(d, s, width);
    }

#endif // defined(MANGO_ENABLE_AVX2)

#if defined(MANGO_ENABLE_NEON)

    void neon_grayscale_linear(u8* d, const u8* s, int width)
    {
        const uint8x8_t scale_r = vdup_n_u8(77);
        const uint8x8_t scale_g = vdup_n_u8(150);
        const uint8x8_t scale_b = vdup_n_u8(29);

        while (width >= 8)
        {
            uint8x8x4_t rgba = vld4_u8(s);
            uint16x8_t luminance = vmull_u8(rgba.val[0], scale_r);
            luminance = vmlal_u8(luminance, rgba.val[1], scale_g);
            luminance = vmlal_u8(luminance, rgba.val[2], scale_b);
            vst1_u8(d, vshrn_n_u16(luminance, 8));
            s += 32;
            d += 8;
            width -= 8;
        }

        grayscale_linear(d, s, width);
    }

#endif // defined(MANGO_ENABLE_NEON)

    using GrayConversionFunc = void (*)(u8*, const u8*, int);

    GrayConversionFunc select_conversion_function(bool alpha, bool linear)
    {
        GrayConversionFunc table [] =
        {
            grayscale_linear,
            grayscale_linear_alpha,
            grayscale_srgb,
            grayscale_srgb_alpha,
        };

        u64 features = getCPUFlags();
        MANGO_UNREFERENCED(features);

#if defined(MANGO_ENABLE_SSE2)
        if (features & INTEL_SSE2)
        {
            table[0] = sse2_grayscale_linear;
        }
#endif
#if defined(MANGO_ENABLE_SSSE3)
        if (features & INTEL_SSSE3)
        {
            table[0] = ssse3_grayscale_linear;
        }
#endif
#if defined(MANGO_ENABLE_AVX2)
        if (features & INTEL_AVX2)
        {
            table[0] = avx2_grayscale_linear;
        }
#endif
#if defined(MANGO_ENABLE_NEON)
        if (features & ARM_NEON)
        {
            table[0] = neon_grayscale_linear;
        }
#endif

        return table[alpha + (!linear) * 2];
    }

    // ------------------------------------------------------------
    // NeuQuant
    // ------------------------------------------------------------

    struct NeuQuant
    {
        u8* m_image;
        int m_length_count;
        int m_sample_factor; // 1..30

        int network[NETSIZE][4];
        int bias[NETSIZE];
        int freq[NETSIZE];
        int radpower[INITRAD];

        NeuQuant(u8* image, int length, int sample_factor);
        ~NeuQuant();

        int contest(int r, int g, int b);
        void alterSingle(int alpha, int i, int r, int g, int b);
        void alterNeigh(int rad, int i, int r, int g, int b);
        void learn();
        void unbias();
    };

    NeuQuant::NeuQuant(u8* image, int length, int sample_factor)
    {
        m_image = image;
        m_length_count = length;
        m_sample_factor = sample_factor;

        for (int i = 0; i < NETSIZE; ++i)
        {
            int* p = network[i];
            p[0] = p[1] = p[2] = (i << (netbiasshift + 8)) / NETSIZE;
            freq[i] = intbias / NETSIZE;
            bias[i] = 0;
        }

        learn();
        unbias();
    }

    NeuQuant::~NeuQuant()
    {
    }

    int NeuQuant::contest(int r, int g, int b)
    {
        int	bestd = ~(((int)1) << 31);
        int	bestbiasd = bestd;
        int	bestpos = -1;
        int	bestbiaspos = bestpos;
        int* p = bias;
        int* f = freq;

        for (int i = 0; i < NETSIZE; ++i)
        {
            const int* n = network[i];
            int dist = std::abs(n[0] - r) +
                       std::abs(n[1] - g) +
                       std::abs(n[2] - b);

            if (dist < bestd)
            {
                bestd = dist;
                bestpos = i;
            }

            int biasdist = dist - ((*p) >> (intbiasshift - netbiasshift));
            if (biasdist < bestbiasd)
            {
                bestbiasd = biasdist;
                bestbiaspos = i;
            }

            int betafreq = (*f >> betashift);
            *f++ -= betafreq;
            *p++ +=(betafreq << gammashift);
        }

        freq[bestpos] += beta;
        bias[bestpos] -= betagamma;

        return bestbiaspos;
    }

    void NeuQuant::alterSingle(int alpha, int i, int r, int g, int b)
    {
        int* n = network[i];
        n[0] -= (alpha * (n[0] - r)) >> alphabiasshift;
        n[1] -= (alpha * (n[1] - g)) >> alphabiasshift;
        n[2] -= (alpha * (n[2] - b)) >> alphabiasshift;
    }

    void NeuQuant::alterNeigh(int rad, int i, int r, int g, int b)
    {
        const int lo = std::max(i - rad, -1);
        const int hi = std::min(i + rad, int(NETSIZE));
        int	j = i + 1;
        int	k = i - 1;

        const int* q = radpower;
        while ((j < hi) || (k > lo))
        {
            int* p;
            int	a = *(++q);

            if (j < hi)
            {
                p = network[j++];
                p[0] -= (a * (p[0] - r)) >> (alphabiasshift + radbiasshift);
                p[1] -= (a * (p[1] - g)) >> (alphabiasshift + radbiasshift);
                p[2] -= (a * (p[2] - b)) >> (alphabiasshift + radbiasshift);
            }
            if (k > lo)
            {
                p = network[k--];
                p[0] -= (a * (p[0] - r)) >> (alphabiasshift + radbiasshift);
                p[1] -= (a * (p[1] - g)) >> (alphabiasshift + radbiasshift);
                p[2] -= (a * (p[2] - b)) >> (alphabiasshift + radbiasshift);
            }
        }
    }

    void NeuQuant::learn()
    {
        const int alphadec = 30 + ((m_sample_factor - 1) / 3);

        u8*	p = m_image;
        u8*	lim = m_image + m_length_count;
        int samplepixels = m_length_count / (4 * m_sample_factor);
        int delta = samplepixels / ncycles;
        int alpha = int(1) << alphabiasshift;
        int radius = initradius;

        int rad = radius >> radiusbiasshift;
        if (rad > 1)
        {
            int	radrad = rad * rad;
            int	adder = 1;
            int	bigalpha = (alpha << radbiasshift) / radrad;

            for (int i = 0; i < rad; ++i)
            {
                radpower[i] = bigalpha * radrad;
                radrad -= adder;
                adder += 2;
            }
        }
        else
        {
            rad = 0;
        }

        int	step;
        if (m_length_count % prime1)
        {
            step = 4 * prime1;
        }
        else
        {
            if (m_length_count % prime2)
            {
                step = 4 * prime2;
            }
            else
            {
                if (m_length_count % prime3)
                {
                    step = 4 * prime3;
                }
                else
                {
                    step = 4 * prime4;
                }
            }
        }

        int j, r, g, b;
        int i = 0;
        int	phase = 0;
        while (i++ < samplepixels)
        {
            r = p[0] << netbiasshift;
            g = p[1] << netbiasshift;
            b = p[2] << netbiasshift;
            j = contest(r, g, b);

            alterSingle(alpha, j, r, g, b);
            if (rad)
            {
                alterNeigh(rad, j, r, g, b);
            }

            p += step;
            while (p >= lim)
            {
                p -= m_length_count;
            }

            if (++phase == delta)
            {
                phase = 0;

                alpha -= alpha / alphadec;
                radius -= radius / radiusdec;
                rad = radius >> radiusbiasshift;

                if (rad > 1)
                {
                    int	radrad = rad * rad;
                    int	adder = 1;
                    int	bigalpha = (alpha << radbiasshift) / radrad;

                    for (int k = 0; k < rad; ++k)
                    {
                        radpower[k] = bigalpha * radrad;
                        radrad -= adder;
                        adder += 2;
                    }
                }
                else
                {
                    rad = 0;
                }
            }
        }
    }

    void NeuQuant::unbias()
    {
        for (int i = 0; i < NETSIZE; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                constexpr int bias = 1 << (netbiasshift - 1);
                network[i][j] = math::clamp((network[i][j] + bias) >> netbiasshift, 0, 255);
            }

            network[i][3] = i;
        }
    }

} // namespace

namespace mango::image
{

    // ----------------------------------------------------------------------------
    // ColorQuantizer
    // ----------------------------------------------------------------------------

    ColorQuantizer::ColorQuantizer(const Surface& source, float quality)
    {
        quality = math::clamp(quality, 0.0f, 1.0f);
        int sample_factor = std::max(1, 30 - int(quality * 29.0f + 1.0f));

        // convert to correct format when required
        TemporaryBitmap temp(source, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        NeuQuant nq(temp.image, temp.width * temp.height * 4, sample_factor);

        m_palette.size = NETSIZE;

        for (int i = 0; i < NETSIZE; ++i)
        {
            m_palette.color[i].r = nq.network[i][0];
            m_palette.color[i].g = nq.network[i][1];
            m_palette.color[i].b = nq.network[i][2];
            m_palette.color[i].a = 0xff;

            m_network[i][0] = nq.network[i][0];
            m_network[i][1] = nq.network[i][1];
            m_network[i][2] = nq.network[i][2];
            m_network[i][3] = nq.network[i][3];
        }

        buildIndex();
    }

    ColorQuantizer::ColorQuantizer(const Palette& palette)
    {
        if (palette.size != 256)
        {
            MANGO_EXCEPTION("[ColorQuantizer] The palette size must be 256.");
        }

        m_palette = palette;

        for (int i = 0; i < NETSIZE; ++i)
        {
            m_network[i][0] = palette[i].r;
            m_network[i][1] = palette[i].g;
            m_network[i][2] = palette[i].b;
            m_network[i][3] = i;
        }

        buildIndex();
    }

    ColorQuantizer::~ColorQuantizer()
    {
    }

    Palette ColorQuantizer::getPalette() const
    {
        return m_palette;
    }

    void ColorQuantizer::quantize(const Surface& dest, const Surface& source, bool dithering)
    {
        if (!dest.format.isIndexed())
        {
            MANGO_EXCEPTION("[ColorQuantizer] The destination surface must have indexed format.");
        }

        if (dest.width != source.width || dest.height != source.height)
        {
            MANGO_EXCEPTION("[ColorQuantizer] The destination and source dimensions must be identical.");
        }

        // convert to correct format when required
        Bitmap temp(source, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        const int width = temp.width;
        const int height = temp.height;

        for (int y = 0; y < height; ++y)
        {
            Color* s = temp.address<Color>(0, y + 0);
            Color* n = temp.address<Color>(0, y + 1); // clipped in the loop
            u8* d = dest.address<u8>(0, y);

            for (int x = 0; x < width; ++x)
            {
                int r = s[x].r;
                int g = s[x].g;
                int b = s[x].b;
                u8 index = getIndex(r, g, b);
                d[x] = index;

                if (dithering)
                {
                    // quantization error
                    r -= m_palette[index].r;
                    g -= m_palette[index].g;
                    b -= m_palette[index].b;

                    const auto distribute = [] (Color& color, int r, int g, int b, int scale)
                    {
                        color.r = math::clamp(color.r + (r * scale / 16), 0, 255);
                        color.g = math::clamp(color.g + (g * scale / 16), 0, 255);
                        color.b = math::clamp(color.b + (b * scale / 16), 0, 255);
                    };

                    // distribute the error to neighbouring pixels with Floyd-Steinberg weights
                    if (x < width - 1)
                    {
                        distribute(s[x + 1], r, g, b, 7);

                        // clipping
                        if (y < height - 1)
                        {
                            // clipping
                            if (x > 0)
                            {
                                distribute(n[x - 1], r, g, b, 3);
                            }

                            distribute(n[x + 0], r, g, b, 5);
                            distribute(n[x + 1], r, g, b, 1);
                        }
                    }
                }
            }
        }
    }

    void ColorQuantizer::buildIndex()
    {
        int previouscol = 0;
        int startpos = 0;

        for (int i = 0; i < NETSIZE; ++i)
        {
            int* p = m_network[i];
            int smallpos = i;
            int smallval = p[1];

            int* q;
            for (int j = i + 1; j < NETSIZE; ++j)
            {
                q = m_network[j];
                if (q[1] < smallval)
                {
                    smallpos = j;
                    smallval = q[1];
                }
            }
            q = m_network[smallpos];

            if (i != smallpos)
            {
                std::swap(q[0], p[0]);
                std::swap(q[1], p[1]);
                std::swap(q[2], p[2]);
                std::swap(q[3], p[3]);
            }

            if (smallval != previouscol)
            {
                m_netindex[previouscol] = (startpos + i) >> 1;
                for (int j = previouscol + 1; j < smallval; ++j)
                {
                    m_netindex[j] = i;
                }
                previouscol = smallval;
                startpos = i;
            }
        }

        m_netindex[previouscol] = (startpos + (NETSIZE - 1)) >> 1;
        for (int j = previouscol + 1; j < 256; ++j)
        {
            m_netindex[j] = NETSIZE - 1;
        }
    }

    int ColorQuantizer::getIndex(int r, int g, int b) const
    {
        int	bestd = 1000;
        int	best = -1;
        int	i = m_netindex[g];
        int	j = i - 1;

        while ((i < NETSIZE) || (j >= 0))
        {
            if (i < NETSIZE)
            {
                const int* p = m_network[i];
                int dist = p[1] - g;
                if (dist >= bestd)
                {
                    i = NETSIZE;
                }
                else
                {
                    ++i;
                    if (dist < 0) dist = -dist;
                    dist += std::abs(p[0] - r);
                    if (dist < bestd)
                    {
                        dist += std::abs(p[2] - b);
                        if (dist < bestd)
                        {
                            bestd = dist;
                            best = p[3];
                        }
                    }
                }
            }

            if (j >= 0)
            {
                const int* p = m_network[j];
                int dist = g - p[1];
                if (dist >= bestd)
                {
                    j = -1;
                }
                else
                {
                    --j;
                    if (dist < 0) dist = -dist;
                    dist += std::abs(p[0] - r);
                    if (dist < bestd)
                    {
                        dist += std::abs(p[2] - b);
                        if (dist < bestd)
                        {
                            bestd = dist;
                            best = p[3];
                        }
                    }
                }
            }
        }

        return best;
    }

    // ----------------------------------------------------------------------------
    // QuantizedBitmap
    // ----------------------------------------------------------------------------

    QuantizedBitmap::QuantizedBitmap(const Surface& source, float quality, bool dithering)
        : Bitmap(source.width, source.height, IndexedFormat(8))
    {
        // convert to correct format when required
        TemporaryBitmap temp(source, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        ColorQuantizer qt(temp, quality);
        qt.quantize(*this, temp, dithering);

        m_palette = qt.getPalette();
    }

    QuantizedBitmap::QuantizedBitmap(const Surface& source, const Palette& palette, bool dithering)
        : Bitmap(source.width, source.height, IndexedFormat(8))
    {
        ColorQuantizer qt(palette);
        qt.quantize(*this, source, dithering);

        m_palette = palette;
    }

    QuantizedBitmap::~QuantizedBitmap()
    {
    }

    Palette QuantizedBitmap::getPalette() const
    {
        return m_palette;
    }

    // ----------------------------------------------------------------------------
    // LuminanceBitmap
    // ----------------------------------------------------------------------------

    LuminanceBitmap::LuminanceBitmap(const Surface& source, bool alpha, bool linear)
        : Bitmap(source.width, source.height, alpha ? LuminanceFormat(16, 0x00ff, 0xff00) : LuminanceFormat(8, 0xff, 0))
    {
        // convert to correct format when required
        TemporaryBitmap temp(source, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        // select conversion function
        auto func = select_conversion_function(alpha, linear);

        // resolve conversion
        for (int y = 0; y < temp.height; ++y)
        {
            const u8* s = temp.address(0, y);
            u8* d = address(0, y);
            func(d, s, temp.width);
        }
    }

} // namespace mango::image
