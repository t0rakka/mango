/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <vector>
#include <algorithm>
#include <mango/core/system.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/math/math.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::math;
    using namespace mango::image;

    // ------------------------------------------------------------
    // linear/sRGB conversion tables
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

    // ------------------------------------------------------------
    // linear/sRGB conversion functions
    // ------------------------------------------------------------

    void srgbToLinear(const Surface& surface)
    {
        for (int y = 0; y < surface.height; ++y)
        {
            u8* scan = surface.address(0, y);
            int count = surface.width;

#if MANGO_SIMD_VECTOR_SIZE >= 256
            while (count >= 8)
            {
                int32x8 color = int32x8::uload(scan);

                float32x8 r = convert<float32x8>((color      ) & int32x8(0xff)) * (1.0f / 255.0f);
                float32x8 g = convert<float32x8>((color >>  8) & int32x8(0xff)) * (1.0f / 255.0f);
                float32x8 b = convert<float32x8>((color >> 16) & int32x8(0xff)) * (1.0f / 255.0f);
                r = srgb_to_linear(r) * 255.0f;
                g = srgb_to_linear(g) * 255.0f;
                b = srgb_to_linear(b) * 255.0f;

                int32x8 red   = convert<int32x8>(r);
                int32x8 green = convert<int32x8>(g);
                int32x8 blue  = convert<int32x8>(b);
                int32x8 alpha = color & int32x8(0xff000000);

                color = alpha | (blue << 16) | (green << 8) | red;

                int32x8::ustore(scan, color);
                scan += 32;
                count -= 8;
            }
#endif

            for (int x = 0; x < count; ++x)
            {
                u8 r = scan[x * 4 + 0];
                u8 g = scan[x * 4 + 1];
                u8 b = scan[x * 4 + 2];
                //u8 a = scan[x * 4 + 3];
                scan[x * 4 + 0] = decode_srgb_table[r];
                scan[x * 4 + 1] = decode_srgb_table[g];
                scan[x * 4 + 2] = decode_srgb_table[b];
                //scan[x * 4 + 3] = a;
            }
        }
    }

    void linearToSRGB(const Surface& surface)
    {
        for (int y = 0; y < surface.height; ++y)
        {
            u8* scan = surface.address(0, y);
            int count = surface.width;

#if MANGO_SIMD_VECTOR_SIZE >= 256
            while (count >= 8)
            {
                int32x8 color = int32x8::uload(scan);

                float32x8 r = convert<float32x8>((color      ) & int32x8(0xff)) * (1.0f / 255.0f);
                float32x8 g = convert<float32x8>((color >>  8) & int32x8(0xff)) * (1.0f / 255.0f);
                float32x8 b = convert<float32x8>((color >> 16) & int32x8(0xff)) * (1.0f / 255.0f);
                r = linear_to_srgb(r) * 255.0f;
                g = linear_to_srgb(g) * 255.0f;
                b = linear_to_srgb(b) * 255.0f;

                int32x8 red   = convert<int32x8>(r);
                int32x8 green = convert<int32x8>(g);
                int32x8 blue  = convert<int32x8>(b);
                int32x8 alpha = color & int32x8(0xff000000);

                color = alpha | (blue << 16) | (green << 8) | red;

                int32x8::ustore(scan, color);
                scan += 32;
                count -= 8;
            }
#endif

            for (int x = 0; x < count; ++x)
            {
                u8 r = scan[x * 4 + 0];
                u8 g = scan[x * 4 + 1];
                u8 b = scan[x * 4 + 2];
                //u8 a = scan[x * 4 + 3];
                scan[x * 4 + 0] = encode_srgb_table[r];
                scan[x * 4 + 1] = encode_srgb_table[g];
                scan[x * 4 + 2] = encode_srgb_table[b];
                //scan[x * 4 + 3] = a;
            }
        }
    }

    // ------------------------------------------------------------
    // grayscale conversion functions
    // ------------------------------------------------------------

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

#if defined(MANGO_ENABLE_SSE4_1)

    void sse41_grayscale_linear(u8* d, const u8* s, int width)
    {
        const __m128i index_r0g0 = _mm_setr_epi8(0, -128, 1, -128, 4, -128, 5, -128, 8, -128, 9, -128, 12, -128, 13, -128);
        const __m128i index_b0a0 = _mm_setr_epi8(2, -128, 3, -128, 6, -128, 7, -128, 10, -128, 11, -128, 14, -128, 15, -128);
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

#endif // defined(MANGO_ENABLE_SSE4_1)

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

        while (width >= 32)
        {
            const __m256i* ptr = reinterpret_cast<const __m256i*>(s);

            __m256i rgba0 = _mm256_loadu_si256(ptr + 0); // RGBA RGBA RGBA RGBA | RGBA RGBA RGBA RGBA
            __m256i rgba1 = _mm256_loadu_si256(ptr + 1); // RGBA RGBA RGBA RGBA | RGBA RGBA RGBA RGBA
            __m256i rgba2 = _mm256_loadu_si256(ptr + 2); // RGBA RGBA RGBA RGBA | RGBA RGBA RGBA RGBA
            __m256i rgba3 = _mm256_loadu_si256(ptr + 3); // RGBA RGBA RGBA RGBA | RGBA RGBA RGBA RGBA

            rgba0 = _mm256_shuffle_epi8(rgba0, index_rgba); // RG[4] BA[4] | RG[4] BA[4]
            rgba1 = _mm256_shuffle_epi8(rgba1, index_rgba); // RG[4] BA[4] | RG[4] BA[4]
            rgba2 = _mm256_shuffle_epi8(rgba2, index_rgba); // RG[4] BA[4] | RG[4] BA[4]
            rgba3 = _mm256_shuffle_epi8(rgba3, index_rgba); // RG[4] BA[4] | RG[4] BA[4]

            rgba0 = _mm256_permute4x64_epi64(rgba0, 0xd8); // RG[8] | BA[8]
            rgba1 = _mm256_permute4x64_epi64(rgba1, 0xd8); // RG[8] | BA[8]
            rgba2 = _mm256_permute4x64_epi64(rgba2, 0xd8); // RG[8] | BA[8]
            rgba3 = _mm256_permute4x64_epi64(rgba3, 0xd8); // RG[8] | BA[8]

            __m256i rg01 = _mm256_permute2x128_si256(rgba0, rgba1, 0x20); // RG[8] | RG[8]
            __m256i rg23 = _mm256_permute2x128_si256(rgba2, rgba3, 0x20); // RG[8] | RG[8]
            __m256i ba01 = _mm256_permute2x128_si256(rgba0, rgba1, 0x31); // BA[8] | BA[8]
            __m256i ba23 = _mm256_permute2x128_si256(rgba2, rgba3, 0x31); // BA[8] | BA[8]

            // maddubs is u8 x s8 -> s16
            __m256i rg0 = _mm256_maddubs_epi16(rg01, scale_rg);
            __m256i rg1 = _mm256_maddubs_epi16(rg23, scale_rg);
            __m256i ba0 = _mm256_maddubs_epi16(ba01, scale_ba);
            __m256i ba1 = _mm256_maddubs_epi16(ba23, scale_ba);
            __m256i sum0 = _mm256_add_epi16(rg0, ba0);
            __m256i sum1 = _mm256_add_epi16(rg1, ba1);
            sum0 = _mm256_srai_epi16(sum0, 7);
            sum1 = _mm256_srai_epi16(sum1, 7);
            sum0 = _mm256_abs_epi16(sum0);
            sum1 = _mm256_abs_epi16(sum1);

            __m256i temp = _mm256_packus_epi16(sum0, sum1);
            temp = _mm256_permute4x64_epi64(temp, 0xd8);
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(d), temp);

            s += 128;
            d += 32;
            width -= 32;
        }

        grayscale_linear(d, s, width);
    }

    void avx2_grayscale_srgb(u8* d, const u8* s, int width)
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

        while (width >= 32)
        {
            int32x8 color0 = int32x8::uload(s +  0);
            int32x8 color1 = int32x8::uload(s + 32);
            int32x8 color2 = int32x8::uload(s + 64);
            int32x8 color3 = int32x8::uload(s + 96);
            float32x8 r0 = convert<float32x8>((color0      ) & int32x8(0xff)) * (1.0f / 255.0f);
            float32x8 r1 = convert<float32x8>((color1      ) & int32x8(0xff)) * (1.0f / 255.0f);
            float32x8 r2 = convert<float32x8>((color2      ) & int32x8(0xff)) * (1.0f / 255.0f);
            float32x8 r3 = convert<float32x8>((color3      ) & int32x8(0xff)) * (1.0f / 255.0f);
            float32x8 g0 = convert<float32x8>((color0 >>  8) & int32x8(0xff)) * (1.0f / 255.0f);
            float32x8 g1 = convert<float32x8>((color1 >>  8) & int32x8(0xff)) * (1.0f / 255.0f);
            float32x8 g2 = convert<float32x8>((color2 >>  8) & int32x8(0xff)) * (1.0f / 255.0f);
            float32x8 g3 = convert<float32x8>((color3 >>  8) & int32x8(0xff)) * (1.0f / 255.0f);
            float32x8 b0 = convert<float32x8>((color0 >> 16) & int32x8(0xff)) * (1.0f / 255.0f);
            float32x8 b1 = convert<float32x8>((color1 >> 16) & int32x8(0xff)) * (1.0f / 255.0f);
            float32x8 b2 = convert<float32x8>((color2 >> 16) & int32x8(0xff)) * (1.0f / 255.0f);
            float32x8 b3 = convert<float32x8>((color3 >> 16) & int32x8(0xff)) * (1.0f / 255.0f);
            r0 = srgb_to_linear(r0) * 255.0f;
            r1 = srgb_to_linear(r1) * 255.0f;
            r2 = srgb_to_linear(r2) * 255.0f;
            r3 = srgb_to_linear(r3) * 255.0f;
            g0 = srgb_to_linear(g0) * 255.0f;
            g1 = srgb_to_linear(g1) * 255.0f;
            g2 = srgb_to_linear(g2) * 255.0f;
            g3 = srgb_to_linear(g3) * 255.0f;
            b0 = srgb_to_linear(b0) * 255.0f;
            b1 = srgb_to_linear(b1) * 255.0f;
            b2 = srgb_to_linear(b2) * 255.0f;
            b3 = srgb_to_linear(b3) * 255.0f;

            int32x8 red0   = convert<int32x8>(r0);
            int32x8 red1   = convert<int32x8>(r1);
            int32x8 red2   = convert<int32x8>(r2);
            int32x8 red3   = convert<int32x8>(r3);
            int32x8 green0 = convert<int32x8>(g0);
            int32x8 green1 = convert<int32x8>(g1);
            int32x8 green2 = convert<int32x8>(g2);
            int32x8 green3 = convert<int32x8>(g3);
            int32x8 blue0  = convert<int32x8>(b0);
            int32x8 blue1  = convert<int32x8>(b1);
            int32x8 blue2  = convert<int32x8>(b2);
            int32x8 blue3  = convert<int32x8>(b3);
            int32x8 alpha0 = color0 & int32x8(0xff000000);
            int32x8 alpha1 = color1 & int32x8(0xff000000);
            int32x8 alpha2 = color2 & int32x8(0xff000000);
            int32x8 alpha3 = color3 & int32x8(0xff000000);

            __m256i rgba0 = alpha0 | (blue0 << 16) | (green0 << 8) | red0;
            __m256i rgba1 = alpha1 | (blue1 << 16) | (green1 << 8) | red1;
            __m256i rgba2 = alpha2 | (blue2 << 16) | (green2 << 8) | red2;
            __m256i rgba3 = alpha3 | (blue3 << 16) | (green3 << 8) | red3;

            rgba0 = _mm256_shuffle_epi8(rgba0, index_rgba); // RG[4] BA[4] | RG[4] BA[4]
            rgba1 = _mm256_shuffle_epi8(rgba1, index_rgba); // RG[4] BA[4] | RG[4] BA[4]
            rgba2 = _mm256_shuffle_epi8(rgba2, index_rgba); // RG[4] BA[4] | RG[4] BA[4]
            rgba3 = _mm256_shuffle_epi8(rgba3, index_rgba); // RG[4] BA[4] | RG[4] BA[4]

            rgba0 = _mm256_permute4x64_epi64(rgba0, 0xd8); // RG[8] | BA[8]
            rgba1 = _mm256_permute4x64_epi64(rgba1, 0xd8); // RG[8] | BA[8]
            rgba2 = _mm256_permute4x64_epi64(rgba2, 0xd8); // RG[8] | BA[8]
            rgba3 = _mm256_permute4x64_epi64(rgba3, 0xd8); // RG[8] | BA[8]

            __m256i rg01 = _mm256_permute2x128_si256(rgba0, rgba1, 0x20); // RG[8] | RG[8]
            __m256i rg23 = _mm256_permute2x128_si256(rgba2, rgba3, 0x20); // RG[8] | RG[8]
            __m256i ba01 = _mm256_permute2x128_si256(rgba0, rgba1, 0x31); // BA[8] | BA[8]
            __m256i ba23 = _mm256_permute2x128_si256(rgba2, rgba3, 0x31); // BA[8] | BA[8]

            // maddubs is u8 x s8 -> s16
            __m256i rg0 = _mm256_maddubs_epi16(rg01, scale_rg);
            __m256i rg1 = _mm256_maddubs_epi16(rg23, scale_rg);
            __m256i ba0 = _mm256_maddubs_epi16(ba01, scale_ba);
            __m256i ba1 = _mm256_maddubs_epi16(ba23, scale_ba);
            __m256i sum0 = _mm256_add_epi16(rg0, ba0);
            __m256i sum1 = _mm256_add_epi16(rg1, ba1);
            sum0 = _mm256_srai_epi16(sum0, 7);
            sum1 = _mm256_srai_epi16(sum1, 7);
            sum0 = _mm256_abs_epi16(sum0);
            sum1 = _mm256_abs_epi16(sum1);

            __m256i temp = _mm256_packus_epi16(sum0, sum1);
            temp = _mm256_permute4x64_epi64(temp, 0xd8);

            uint32x8 color(temp);
            float32x8 r = convert<float32x8>((color      ) & uint32x8(0xff)) * (1.0f / 255.0f);
            float32x8 g = convert<float32x8>((color >>  8) & uint32x8(0xff)) * (1.0f / 255.0f);
            float32x8 b = convert<float32x8>((color >> 16) & uint32x8(0xff)) * (1.0f / 255.0f);
            float32x8 a = convert<float32x8>((color >> 24) & uint32x8(0xff)) * (1.0f / 255.0f);
            r = linear_to_srgb(r) * 255.0f;
            g = linear_to_srgb(g) * 255.0f;
            b = linear_to_srgb(b) * 255.0f;
            a = linear_to_srgb(a) * 255.0f;
            uint32x8 red   = convert<uint32x8>(r);
            uint32x8 green = convert<uint32x8>(g);
            uint32x8 blue  = convert<uint32x8>(b);
            uint32x8 alpha = convert<uint32x8>(a);
            color = (alpha << 24) | (blue << 16) | (green << 8) | red;
            uint32x8::ustore(d, color);

            s += 128;
            d += 32;
            width -= 32;
        }

        grayscale_srgb(d, s, width);
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

#if defined(MANGO_ENABLE_SSE4_1)
        if (features & INTEL_SSE4_1)
        {
            table[0] = sse41_grayscale_linear;
        }
#endif
#if defined(MANGO_ENABLE_AVX2)
        if (features & INTEL_AVX2)
        {
            table[0] = avx2_grayscale_linear;
            table[2] = avx2_grayscale_srgb;
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

    // ----------------------------------------------------------------------------
    // clear
    // ----------------------------------------------------------------------------

    void clear_u8_scan(u8* dest, int count, u32 color)
    {
        std::memset(dest, u8(color), count);
    }

    void clear_u16_scan(u8* dest, int count, u32 color)
    {
        std::fill_n(reinterpret_cast<u16*>(dest), count, u16(color));
    }

    void clear_u24_scan(u8* dest, int count, u32 color)
    {
        std::fill_n(reinterpret_cast<u24*>(dest), count, u24(color));
    }

    void clear_u32_scan(u8* dest, int count, u32 color)
    {
        std::fill_n(reinterpret_cast<u32*>(dest), count, color);
    }

    template <typename FloatType>
    void clear_float_scan(u8* dest, int count, const FloatType* color, const int size)
    {
        FloatType* d = reinterpret_cast<FloatType*>(dest);
        for (int i = 0; i < count; ++i)
        {
            switch (size)
            {
                case 4: d[3] = color[3];
                case 3: d[2] = color[2];
                case 2: d[1] = color[1];
                case 1: d[0] = color[0];
            }
            d += size;
        }
    }

    template <typename FloatType>
    int config_clear_color(FloatType* color, const Format& format, float red, float green, float blue, float alpha)
    {
        int size = 0;

        // make the parameters indexable
        const float temp[] = { red, green, blue, alpha };

        for (int i = 0; i < 4; ++i)
        {
            if (format.size[i])
            {
                int index = format.offset[i] / (sizeof(FloatType) * 8);
                color[index] = FloatType(temp[i]);
                size = std::max(size, index + 1);
            }
        }

        return size;
    }

    // ----------------------------------------------------------------------------
    // create_surface()
    // ----------------------------------------------------------------------------

    Surface create_surface(ConstMemory memory, const std::string& filename, const Format* ptr_format, const ImageDecodeOptions& options)
    {
        Surface surface;

        ImageDecoder decoder(memory, filename);
        if (decoder.isDecoder())
        {
            ImageHeader header = decoder.header();
            if (!header)
            {
                printLine(Print::Info, header.info);
                return surface;
            }

            if (ptr_format)
            {
                surface.format = *ptr_format;
                surface.format.flags = u16_select(Format::MASK, header.format.flags, surface.format.flags);
            }
            else
            {
                surface.format = header.format;
            }

            if (options.palette)
            {
                // the decoder will set palette if present
                options.palette->size = 0;

                if (header.palette)
                {
                    surface.format = IndexedFormat(8);
                }
            }

            surface.width  = header.width;
            surface.height = header.height;
            surface.stride = header.width * surface.format.bytes();
            surface.image  = new u8[header.height * surface.stride];

            // decode
            ImageDecodeStatus status = decoder.decode(surface, options, 0, 0, 0);
            MANGO_UNREFERENCED(status);
        }

        return surface;
    }

} // namespace

namespace mango::image
{

    // ----------------------------------------------------------------------------
    // Surface
    // ----------------------------------------------------------------------------

    Surface::Surface()
        : format()
        , image(nullptr)
        , stride(0)
        , width(0)
        , height(0)
    {
    }

    Surface::Surface(const Surface& surface, bool yflip)
        : format(surface.format)
        , image(surface.image)
        , stride(surface.stride)
        , width(surface.width)
        , height(surface.height)
    {
        if (yflip)
        {
            image += (height - 1) * stride;
            stride = 0 - stride;
        }
    }

    Surface::Surface(int width, int height, const Format& format, size_t stride, const void* image)
        : format(format)
        , image(const_cast<u8*>(reinterpret_cast<const u8*>(image)))
        , stride(stride)
        , width(width)
        , height(height)
    {
    }

    Surface::Surface(const Surface& surface, int x, int y, int w, int h)
        : format(surface.format)
        , stride(surface.stride)
    {
        if (x < 0)
        {
            w = std::max(0, w + x);
            x = 0;
        }

        if (y < 0)
        {
            h = std::max(0, h + y);
            y = 0;
        }

        if (x >= surface.width || y >= surface.height)
        {
            image = nullptr;
            width = 0;
            height = 0;
        }
        else
        {
            image  = surface.address(x, y);
            width  = std::max(0, std::min(surface.width, x + w) - x);
            height = std::max(0, std::min(surface.height, y + h) - y);
        }
    }

    Surface::~Surface()
    {
    }

    Surface& Surface::operator = (const Surface& surface)
    {
        format = surface.format;
        image = surface.image;
        stride = surface.stride;
        width = surface.width;
        height = surface.height;
        return *this;
    }

    ImageEncodeStatus Surface::save(Stream& stream, const std::string& extension, const ImageEncodeOptions& options) const
    {
        ImageEncodeStatus status;
        ImageEncoder encoder(extension);
        if (encoder.isEncoder())
        {
            status = encoder.encode(stream, *this, options);
            if (!status)
            {
                printLine(Print::Info, status.info);
            }
        }
        else
        {
            status.setError("Incorrect encoder: {}", extension);
        }

        return status;
    }

    ImageEncodeStatus Surface::save(const std::string& filename, const ImageEncodeOptions& options) const
    {
        filesystem::OutputFileStream file(filename);
        ImageEncodeStatus status = save(file, filesystem::getExtension(filename), options);
        return status;
    }

    void Surface::clear(float red, float green, float blue, float alpha) const
    {
        switch (format.type)
        {
            case Format::UNORM:
            {
                void (*func)(u8 *, int, u32) = nullptr;

                switch (format.bits)
                {
                    case 8: func = clear_u8_scan; break;
                    case 16: func = clear_u16_scan; break;
                    case 24: func = clear_u24_scan; break;
                    case 32: func = clear_u32_scan; break;
                }

                if (func)
                {
                    u32 color = format.pack(red, green, blue, alpha);

                    for (int y = 0; y < height; ++y)
                    {
                        func(image + y * stride, width, color);
                    }
                }

                break;
            }

            case Format::FLOAT16:
            {
                float16 color[4];
                int size = config_clear_color<float16>(color, format, red, green, blue, alpha);

                for (int y = 0; y < height; ++y)
                {
                    clear_float_scan<float16>(image + y * stride, width, color, size);
                }

                break;
            }

            case Format::FLOAT32:
            {
                float color[4];
                int size = config_clear_color<float>(color, format, red, green, blue, alpha);

                for (int y = 0; y < height; ++y)
                {
                    clear_float_scan<float>(image + y * stride, width, color, size);
                }

                break;
            }

            default:
                break;
        }
    }

    void Surface::clear(Color color) const
    {
        const float s = 1.0f / 255.0f;
        clear(color.r * s, color.g * s, color.b * s, color.a * s);
    }

    void Surface::blit(int x, int y, const Surface& source) const
    {
        if (!source.width || !source.height || !source.format.bits || !format.bits)
            return;

        Surface dest(*this, x, y, source.width, source.height);

        if (!dest.width || !dest.height)
            return;

        BlitRect rect;

        rect.width = dest.width;
        rect.height = dest.height;
        rect.source.address = source.image;
        rect.source.stride = source.stride;
        rect.dest.address = dest.image;
        rect.dest.stride = dest.stride;

        if (x < 0)
        {
            rect.source.address -= x * source.format.bytes();
        }

        if (y < 0)
        {
            rect.source.address -= y * source.stride;
        }

        Blitter blitter(dest.format, source.format);

#if 0 // enabling this creates a wind-tunnel for laptops
        const int slice = 128;

        if (ThreadPool::getHardwareConcurrency() > 2 && rect.height >= slice * 2)
        {
            ConcurrentQueue queue("blitter");

            for (int y = 0; y < rect.height; y += slice)
            {
                queue.enqueue([=, &blitter]
                {
                    int y0 = y;
                    int y1 = std::min(y + slice, rect.height);

                    BlitRect temp = rect;

                    temp.dest.address += y0 * rect.dest.stride;
                    temp.source.address += y0 * rect.source.stride;
                    temp.height = y1 - y0;

                    blitter.convert(temp);
                });
            }
        }
        else
#endif
        {
            blitter.convert(rect);
        }
    }

    void Surface::xflip() const
    {
        if (!image || !stride)
            return;

        const int bytes_per_pixel = format.bytes();
        const int half_width = width / 2;

        u8* left = image;
        u8* right = image + (width - 1) * bytes_per_pixel;

        for (int y = 0; y < height; ++y)
        {
            u8* a = left;
            u8* b = right;

            for (int x = 0; x < half_width; ++x)
            {
                // swap pixels
                for (int i = 0; i < bytes_per_pixel; ++i)
                {
                    std::swap(a[i], b[i]);
                }

                // next pixel
                a += bytes_per_pixel;
                b -= bytes_per_pixel;
            }

            // next scanline
            left += stride;
            right += stride;
        }
    }

    void Surface::yflip() const
    {
        if (!image || !stride)
            return;

        // NOTE: We can't do logical yflip here because image could be a storage pointer.

        const int bytes_per_pixel = format.bytes();
        const int bytes_per_scan = width * bytes_per_pixel;
        const int half_height = height / 2;

        u8* top = image;
        u8* bottom = image + (height - 1) * stride;

        for (int y = 0; y < half_height; ++y)
        {
            // swap pixels
            for (int i = 0; i < bytes_per_scan; ++i)
            {
                std::swap(top[i], bottom[i]);
            }

            // next scanline
            top += stride;
            bottom -= stride;
        }
    }

    // ----------------------------------------------------------------------------
    // Bitmap
    // ----------------------------------------------------------------------------

    Bitmap::Bitmap(int w, int h, const Format& f, size_t s)
        : Surface(w, h, f, s, nullptr)
    {
        if (!stride)
        {
            stride = width * format.bytes();
        }

        size_t bytes = stride * height;
        image = new u8[bytes];
    }

    Bitmap::Bitmap(const Surface& source, const Format& format)
        : Surface(source.width, source.height, format, 0, nullptr)
    {
        stride = width * format.bytes();

        size_t bytes = stride * height;
        image = new u8[bytes];

        blit(0, 0, source);
    }

    Bitmap::Bitmap(ConstMemory memory, const std::string& extension, const ImageDecodeOptions& options)
        : Surface(create_surface(memory, extension, nullptr, options))
    {
    }

    Bitmap::Bitmap(ConstMemory memory, const std::string& extension, const Format& format, const ImageDecodeOptions& options)
        : Surface(create_surface(memory, extension, &format, options))
    {
    }

    Bitmap::Bitmap(const filesystem::File& file, const ImageDecodeOptions& options)
        : Surface(create_surface(file, file.filename(), nullptr, options))
    {
    }

    Bitmap::Bitmap(const filesystem::File& file, const Format& format, const ImageDecodeOptions& options)
        : Surface(create_surface(file, file.filename(), &format, options))
    {
    }

    Bitmap::Bitmap(const std::string& filename, const ImageDecodeOptions& options)
        : Surface(create_surface(filesystem::File(filename), filename, nullptr, options))
    {
    }

    Bitmap::Bitmap(const std::string& filename, const Format& format, const ImageDecodeOptions& options)
        : Surface(create_surface(filesystem::File(filename), filename, &format, options))
    {
    }

    Bitmap::Bitmap(Bitmap&& bitmap)
        : Surface(bitmap)
    {
        bitmap.image = nullptr;
    }

    Bitmap::~Bitmap()
    {
        delete[] image;
    }

    Bitmap& Bitmap::operator = (Bitmap&& bitmap)
    {
        // copy surface
        format = bitmap.format;
        image = bitmap.image;
        stride = bitmap.stride;
        width = bitmap.width;
        height = bitmap.height;

        // move image ownership
        bitmap.image = nullptr;

        return *this;
    }

    // ----------------------------------------------------------------------------
    // TemporaryBitmap
    // ----------------------------------------------------------------------------

    TemporaryBitmap::TemporaryBitmap(const Surface& surface, const Format& format, bool yflip)
        : Surface(surface)
    {
        if (surface.format != format)
        {
            m_bitmap = std::make_unique<Bitmap>(surface, format);
            static_cast<Surface&>(*this) = *m_bitmap;
        }

        if (yflip)
        {
            image += (height - 1) * stride;
            stride = 0 - stride;
        }
    }

    TemporaryBitmap::TemporaryBitmap(const Surface& surface, int width, int height, const Format& format, bool yflip)
        : Surface(surface)
    {
        if (surface.format != format || surface.width != width || surface.height != height)
        {
            m_bitmap = std::make_unique<Bitmap>(width, height, format);
            m_bitmap->blit(0, 0, surface);
            static_cast<Surface&>(*this) = *m_bitmap;
        }

        if (yflip)
        {
            image += (height - 1) * stride;
            stride = 0 - stride;
        }
    }

    // ----------------------------------------------------------------------------
    // LuminanceBitmap
    // ----------------------------------------------------------------------------

    LuminanceBitmap::LuminanceBitmap(const Surface& source, bool alpha, bool force_linear)
        : Bitmap(source.width, source.height, alpha ? LuminanceFormat(16, 0x00ff, 0xff00) : LuminanceFormat(8, 0xff, 0))
    {
        // convert to correct format when required
        TemporaryBitmap temp(source, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        // select conversion function
        bool linear = source.format.isLinear() || force_linear;
        auto func = select_conversion_function(alpha, linear);

        // resolve conversion
        for (int y = 0; y < temp.height; ++y)
        {
            const u8* s = temp.address(0, y);
            u8* d = address(0, y);
            func(d, s, temp.width);
        }
    }

    // ----------------------------------------------------------------------------
    // misc
    // ----------------------------------------------------------------------------

    void srgbToLinear(const Surface& surface)
    {
        ::srgbToLinear(surface);
    }

    void linearToSRGB(const Surface& surface)
    {
        ::linearToSRGB(surface);
    }

} // namespace mango::image
