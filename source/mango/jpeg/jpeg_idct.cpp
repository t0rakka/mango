/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "jpeg.hpp"

namespace
{

    using namespace mango;
    using namespace jpeg;

    struct IDCT
    {
        int x0, x1, x2, x3;
        int y0, y1, y2, y3;

        void compute(int s0, int s1, int s2, int s3, int s4, int s5, int s6, int s7)
        {
            const int n0 = (s2 + s6) * 2217;
            const int t2 = n0 + s6 * -7567;
            const int t3 = n0 + s2 * 3135;
            const int t0 = (s0 + s4) << 12;
            const int t1 = (s0 - s4) << 12;
            x0 = t0 + t3;
            x3 = t0 - t3;
            x1 = t1 + t2;
            x2 = t1 - t2;

            int p1 = s7 + s1;
            int p2 = s5 + s3;
            int p3 = s7 + s3;
            int p4 = s5 + s1;
            int p5 = (p3 + p4) * 4816;
            p1 = p1 * -3685 + p5;
            p2 = p2 * -10497 + p5;
            p3 = p3 * -8034;
            p4 = p4 * -1597;
            y0 = p1 + p3 + s7 * 1223;
            y1 = p2 + p4 + s5 * 8410;
            y2 = p2 + p3 + s3 * 12586;
            y3 = p1 + p4 + s1 * 6149;
        }
    };

    template <int PRECISION>
    void idct(u8* dest, const s16* data, const u16* qt)
    {
        int temp[64];
        int* v;

        const s16* s = data;

        v = temp;

        for (int i = 0; i < 8; ++i)
        {
            if (s[1] || s[2] || s[3] || s[4] || s[5] || s[6] || s[7])
            {
                // dequantize
                const int s0 = s[0] * qt[0];
                const int s1 = s[1] * qt[1];
                const int s2 = s[2] * qt[2];
                const int s3 = s[3] * qt[3];
                const int s4 = s[4] * qt[4];
                const int s5 = s[5] * qt[5];
                const int s6 = s[6] * qt[6];
                const int s7 = s[7] * qt[7];

                IDCT idct;
                idct.compute(s0, s1, s2, s3, s4, s5, s6, s7);
                const int bias = 0x200;
                idct.x0 += bias;
                idct.x1 += bias;
                idct.x2 += bias;
                idct.x3 += bias;
                v[0] = (idct.x0 + idct.y3) >> 10;
                v[1] = (idct.x1 + idct.y2) >> 10;
                v[2] = (idct.x2 + idct.y1) >> 10;
                v[3] = (idct.x3 + idct.y0) >> 10;
                v[4] = (idct.x3 - idct.y0) >> 10;
                v[5] = (idct.x2 - idct.y1) >> 10;
                v[6] = (idct.x1 - idct.y2) >> 10;
                v[7] = (idct.x0 - idct.y3) >> 10;
            }
            else
            {
                int dc = (s[0] * qt[0]) << 2;
                v[0] = dc;
                v[1] = dc;
                v[2] = dc;
                v[3] = dc;
                v[4] = dc;
                v[5] = dc;
                v[6] = dc;
                v[7] = dc;
            }

            v += 8;
            s += 8;
            qt += 8;
        }

        v = temp;
        const int shift = PRECISION + 9;

        for (int i = 0; i < 8; ++i)
        {
            IDCT idct;
            idct.compute(v[0], v[8], v[16], v[24], v[32], v[40], v[48], v[56]);
            ++v;
            const int bias = 0x10000 + (128 << shift);
            idct.x0 += bias;
            idct.x1 += bias;
            idct.x2 += bias;
            idct.x3 += bias;
            dest[0] = byteclamp((idct.x0 + idct.y3) >> shift);
            dest[1] = byteclamp((idct.x1 + idct.y2) >> shift);
            dest[2] = byteclamp((idct.x2 + idct.y1) >> shift);
            dest[3] = byteclamp((idct.x3 + idct.y0) >> shift);
            dest[4] = byteclamp((idct.x3 - idct.y0) >> shift);
            dest[5] = byteclamp((idct.x2 - idct.y1) >> shift);
            dest[6] = byteclamp((idct.x1 - idct.y2) >> shift);
            dest[7] = byteclamp((idct.x0 - idct.y3) >> shift);
            dest += 8;
        }
    }

} // namespace

namespace jpeg
{

    // ------------------------------------------------------------------------------------------------
    // Generic C++ implementation
    // ------------------------------------------------------------------------------------------------

    void idct8(u8* dest, const s16* data, const u16* qt)
    {
        idct<8>(dest, data, qt);
    }

    void idct12(u8* dest, const s16* data, const u16* qt)
    {
        idct<12>(dest, data, qt);
    }

#if defined(JPEG_ENABLE_SIMD)

    // ------------------------------------------------------------------------------------------------
    // SIMD implementation
    // ------------------------------------------------------------------------------------------------

#if 0
    static inline uint8x16 packRow(float32x4 x, float32x4 y)
    {
        float32x4 a = x - y;
        float32x4 b = x + y;
        b = b.wzyx;
        int16x8 c0 = simd::narrow(convert<int32x4>(a).m, convert<int32x4>(b).m);
        uint16x8 c1 = reinterpret<uint16x8>(c0);
        return simd::narrow(c1.m, c1.m);
    }
#endif

    static inline uint8x16 packRow2(float32x4 x0, float32x4 y0, float32x4 x1, float32x4 y1)
    {
        float32x4 a0 = x0 - y0;
        float32x4 b0 = x0 + y0;
        b0 = b0.wzyx;

        float32x4 a1 = x1 - y1;
        float32x4 b1 = x1 + y1;
        b1 = b1.wzyx;

        int16x8 c0 = simd::narrow(convert<int32x4>(a0).m, convert<int32x4>(b0).m);
        uint16x8 d0 = reinterpret<uint16x8>(c0);

        int16x8 c1 = simd::narrow(convert<int32x4>(a1).m, convert<int32x4>(b1).m);
        uint16x8 d1 = reinterpret<uint16x8>(c1);

        return simd::narrow(d0.m, d1.m);
    }

    void idct_simd(u8* dest, const s16* data, const u16* qt)
    {
        float32x4 temp[16];
        float32x4* v = temp;

        const float32x4 f0(-0.3535533905f, 0.3535533905f, 128.0f,        128.0f);
        const float32x4 f1( 0.4619397662f, 0.1913417161f,-0.4619397662f,-0.1913417161f);
        const float32x4 c0 = f0.yyyy;
        const float32x4 c1 = f1.xywz;
        const float32x4 c2 = f0.yxxy;
        const float32x4 c3 = f1.yzxw;
        const float32x4 c4(-0.4903926402f,-0.4157348061f,-0.2777851165f,-0.0975451610f);
        const float32x4 c5(-0.4157348061f, 0.0975451610f, 0.4903926402f, 0.2777851165f);
        const float32x4 c6(-0.2777851165f, 0.4903926402f,-0.0975451610f,-0.4157348061f);
        const float32x4 c7(-0.0975451610f, 0.2777851165f,-0.4157348061f, 0.4903926402f);
        const float32x4 c8 = f0.wwww;

        for (int i = 0; i < 8; ++i)
        {
            int16x8 d = *reinterpret_cast<const int16x8 *>(data);
            int16x8 q = *reinterpret_cast<const int16x8 *>(qt);
            int16x8 dq = simd::mullo(d, q);
            float32x8 s = convert<float32x8>(int32x8(simd::extend32x8(dq)));
            float32x4 s0 = s.low;
            float32x4 s1 = s.high;
            float32x4 x = madd(madd(madd(s0.xxxx * c0, s0.zzzz, c1), s1.xxxx, c2), s1.zzzz, c3);
            float32x4 y = madd(madd(madd(s0.yyyy * c4, s0.wwww, c5), s1.yyyy, c6), s1.wwww, c7);
            float32x4 a = x + y;
            float32x4 b = x - y;
            b = b.wzyx;

            v[0] = a;
            v[1] = b;

            data += 8;
            qt += 8;
            v += 2;
        }

        v = temp + 1;

        for (int i = 0; i < 2; ++i)
        {
            float32x4 v0 = v[0];
            float32x4 v2 = v[4];
            float32x4 v4 = v[8];
            float32x4 v6 = v[12];
            float32x4 x0 = madd(madd(madd(v0.xxxx * c0, v2.xxxx, c1), v4.xxxx, c2), v6.xxxx, c3) + c8;
            float32x4 x1 = madd(madd(madd(v0.yyyy * c0, v2.yyyy, c1), v4.yyyy, c2), v6.yyyy, c3) + c8;
            float32x4 x2 = madd(madd(madd(v0.zzzz * c0, v2.zzzz, c1), v4.zzzz, c2), v6.zzzz, c3) + c8;
            float32x4 x3 = madd(madd(madd(v0.wwww * c0, v2.wwww, c1), v4.wwww, c2), v6.wwww, c3) + c8;

            float32x4 v1 = v[2];
            float32x4 v3 = v[6];
            float32x4 v5 = v[10];
            float32x4 v7 = v[14];
            float32x4 y0 = madd(madd(madd(v1.xxxx * c4, v3.xxxx, c5), v5.xxxx, c6), v7.xxxx, c7);
            float32x4 y1 = madd(madd(madd(v1.yyyy * c4, v3.yyyy, c5), v5.yyyy, c6), v7.yyyy, c7);
            float32x4 y2 = madd(madd(madd(v1.zzzz * c4, v3.zzzz, c5), v5.zzzz, c6), v7.zzzz, c7);
            float32x4 y3 = madd(madd(madd(v1.wwww * c4, v3.wwww, c5), v5.wwww, c6), v7.wwww, c7);

            uint8x16* d = reinterpret_cast<uint8x16 *>(dest);
            d[0] = packRow2(x3, y3, x2, y2);
            d[1] = packRow2(x1, y1, x0, y0);
            dest += 8 * 4;
            --v;
        }
    }

#endif // JPEG_ENABLE_SIMD

#if defined(JPEG_ENABLE_SSE2)

    // ------------------------------------------------------------------------------------------------
    // SSE2 implementation
    // ------------------------------------------------------------------------------------------------

    // The original code is by Petr Kobalicek ; WE HAVE TAKEN LIBERTIES TO ADAPT IT TO OUR USE!!!
    // https://github.com/kobalicek/simdtests
    // [License]
    // Public Domain <unlicense.org>

    // Derived from jidctint's `jpeg_idct_islow`
    constexpr int JPEG_IDCT_PREC = 12;
    constexpr int JPEG_IDCT_HALF(int precision) { return (1 << ((precision) - 1)); }
    constexpr int JPEG_IDCT_FIXED(double x) { return int((x * double(1 << JPEG_IDCT_PREC) + 0.5)); }

    constexpr int JPEG_IDCT_M_2_562915447 = JPEG_IDCT_FIXED(-2.562915447);
    constexpr int JPEG_IDCT_M_1_961570560 = JPEG_IDCT_FIXED(-1.961570560);
    constexpr int JPEG_IDCT_M_1_847759065 = JPEG_IDCT_FIXED(-1.847759065);
    constexpr int JPEG_IDCT_M_0_899976223 = JPEG_IDCT_FIXED(-0.899976223);
    constexpr int JPEG_IDCT_M_0_390180644 = JPEG_IDCT_FIXED(-0.390180644);
    constexpr int JPEG_IDCT_P_0_298631336 = JPEG_IDCT_FIXED(0.298631336);
    constexpr int JPEG_IDCT_P_0_541196100 = JPEG_IDCT_FIXED(0.541196100);
    constexpr int JPEG_IDCT_P_0_765366865 = JPEG_IDCT_FIXED(0.765366865);
    constexpr int JPEG_IDCT_P_1_175875602 = JPEG_IDCT_FIXED(1.175875602);
    constexpr int JPEG_IDCT_P_1_501321110 = JPEG_IDCT_FIXED(1.501321110);
    constexpr int JPEG_IDCT_P_2_053119869 = JPEG_IDCT_FIXED(2.053119869);
    constexpr int JPEG_IDCT_P_3_072711026 = JPEG_IDCT_FIXED(3.072711026);

    // Keep 2 bits of extra precision for the intermediate results
    constexpr int JPEG_IDCT_COL_NORM = (JPEG_IDCT_PREC - 2);
    constexpr int JPEG_IDCT_COL_BIAS = JPEG_IDCT_HALF(JPEG_IDCT_COL_NORM);

    // Consume 2 bits of an intermediate results precision and 3 bits that were
    // produced by `2 * sqrt(8)`. Also normalize to from `-128..127` to `0..255`
    constexpr int JPEG_IDCT_ROW_NORM = (JPEG_IDCT_PREC + 2 + 3);
    constexpr int JPEG_IDCT_ROW_BIAS = (JPEG_IDCT_HALF(JPEG_IDCT_ROW_NORM) + (128 << JPEG_IDCT_ROW_NORM));

#define JPEG_CONST16_SSE2(x, y)  _mm_setr_epi16(x, y, x, y, x, y, x, y)
#define JPEG_CONST32_SSE2(x)     _mm_setr_epi32(x, x, x, x)

    static const __m128i rot0_0 = JPEG_CONST16_SSE2(JPEG_IDCT_P_0_541196100                          , JPEG_IDCT_P_0_541196100 + JPEG_IDCT_M_1_847759065);
    static const __m128i rot0_1 = JPEG_CONST16_SSE2(JPEG_IDCT_P_0_541196100 + JPEG_IDCT_P_0_765366865, JPEG_IDCT_P_0_541196100                          );
    static const __m128i rot1_0 = JPEG_CONST16_SSE2(JPEG_IDCT_P_1_175875602 + JPEG_IDCT_M_0_899976223, JPEG_IDCT_P_1_175875602                          );
    static const __m128i rot1_1 = JPEG_CONST16_SSE2(JPEG_IDCT_P_1_175875602                          , JPEG_IDCT_P_1_175875602 + JPEG_IDCT_M_2_562915447);
    static const __m128i rot2_0 = JPEG_CONST16_SSE2(JPEG_IDCT_M_1_961570560 + JPEG_IDCT_P_0_298631336, JPEG_IDCT_M_1_961570560                          );
    static const __m128i rot2_1 = JPEG_CONST16_SSE2(JPEG_IDCT_M_1_961570560                          , JPEG_IDCT_M_1_961570560 + JPEG_IDCT_P_3_072711026);
    static const __m128i rot3_0 = JPEG_CONST16_SSE2(JPEG_IDCT_M_0_390180644 + JPEG_IDCT_P_2_053119869, JPEG_IDCT_M_0_390180644                          );
    static const __m128i rot3_1 = JPEG_CONST16_SSE2(JPEG_IDCT_M_0_390180644                          , JPEG_IDCT_M_0_390180644 + JPEG_IDCT_P_1_501321110);
    static const __m128i colBias = JPEG_CONST32_SSE2(JPEG_IDCT_COL_BIAS);
    static const __m128i rowBias = JPEG_CONST32_SSE2(JPEG_IDCT_ROW_BIAS);

#define JPEG_IDCT_ROTATE_XMM(dst0, dst1, x, y, c0, c1) \
    __m128i c0##_l = _mm_unpacklo_epi16(x, y); \
    __m128i c0##_h = _mm_unpackhi_epi16(x, y); \
    __m128i dst0##_l = _mm_madd_epi16(c0##_l, c0); \
    __m128i dst0##_h = _mm_madd_epi16(c0##_h, c0); \
    __m128i dst1##_l = _mm_madd_epi16(c0##_l, c1); \
    __m128i dst1##_h = _mm_madd_epi16(c0##_h, c1);

    // out = in << 12  (in 16-bit, out 32-bit)
#define JPEG_IDCT_WIDEN_XMM(dst, in) \
    __m128i dst##_l = _mm_srai_epi32(_mm_unpacklo_epi16(_mm_setzero_si128(), (in)), 4); \
    __m128i dst##_h = _mm_srai_epi32(_mm_unpackhi_epi16(_mm_setzero_si128(), (in)), 4);

    // wide add
#define JPEG_IDCT_WADD_XMM(dst, a, b) \
    __m128i dst##_l = _mm_add_epi32(a##_l, b##_l); \
    __m128i dst##_h = _mm_add_epi32(a##_h, b##_h);

    // wide sub
#define JPEG_IDCT_WSUB_XMM(dst, a, b) \
    __m128i dst##_l = _mm_sub_epi32(a##_l, b##_l); \
    __m128i dst##_h = _mm_sub_epi32(a##_h, b##_h);

    // butterfly a/b, add bias, then shift by `norm` and pack to 16-bit
#define JPEG_IDCT_BFLY_XMM(dst0, dst1, a, b, bias, norm) { \
    __m128i abiased_l = _mm_add_epi32(a##_l, bias); \
    __m128i abiased_h = _mm_add_epi32(a##_h, bias); \
    JPEG_IDCT_WADD_XMM(sum, abiased, b) \
    JPEG_IDCT_WSUB_XMM(diff, abiased, b) \
    dst0 = _mm_packs_epi32(_mm_srai_epi32(sum_l, norm), _mm_srai_epi32(sum_h, norm)); \
    dst1 = _mm_packs_epi32(_mm_srai_epi32(diff_l, norm), _mm_srai_epi32(diff_h, norm)); \
    }

#define JPEG_IDCT_IDCT_PASS_XMM(bias, norm) { \
    JPEG_IDCT_ROTATE_XMM(t2e, t3e, v2, v6, rot0_0, rot0_1) \
    __m128i sum04 = _mm_add_epi16(v0, v4); \
    __m128i dif04 = _mm_sub_epi16(v0, v4); \
    JPEG_IDCT_WIDEN_XMM(t0e, sum04) \
    JPEG_IDCT_WIDEN_XMM(t1e, dif04) \
    JPEG_IDCT_WADD_XMM(x0, t0e, t3e) \
    JPEG_IDCT_WSUB_XMM(x3, t0e, t3e) \
    JPEG_IDCT_WADD_XMM(x1, t1e, t2e) \
    JPEG_IDCT_WSUB_XMM(x2, t1e, t2e) \
    JPEG_IDCT_ROTATE_XMM(y0o, y2o, v7, v3, rot2_0, rot2_1) \
    JPEG_IDCT_ROTATE_XMM(y1o, y3o, v5, v1, rot3_0, rot3_1) \
    __m128i sum17 = _mm_add_epi16(v1, v7); \
    __m128i sum35 = _mm_add_epi16(v3, v5); \
    JPEG_IDCT_ROTATE_XMM(y4o,y5o, sum17, sum35, rot1_0, rot1_1) \
    JPEG_IDCT_WADD_XMM(x4, y0o, y4o) \
    JPEG_IDCT_WADD_XMM(x5, y1o, y5o) \
    JPEG_IDCT_WADD_XMM(x6, y2o, y5o) \
    JPEG_IDCT_WADD_XMM(x7, y3o, y4o) \
    JPEG_IDCT_BFLY_XMM(v0, v7, x0, x7, bias, norm) \
    JPEG_IDCT_BFLY_XMM(v1, v6, x1, x6, bias, norm) \
    JPEG_IDCT_BFLY_XMM(v2, v5, x2, x5, bias, norm) \
    JPEG_IDCT_BFLY_XMM(v3, v4, x3, x4, bias, norm) \
    }

    static inline void interleave8(__m128i &a, __m128i &b)
    {
        __m128i c = a;
        a = _mm_unpacklo_epi8(a, b);
        b = _mm_unpackhi_epi8(c, b);
    }

    static inline void interleave16(__m128i &a, __m128i &b)
    {
        __m128i c = a;
        a = _mm_unpacklo_epi16(a, b);
        b = _mm_unpackhi_epi16(c, b);
    }

    void idct_sse2(u8* dest, const s16* src, const u16* qt)
    {
        const __m128i* data = reinterpret_cast<const __m128i *>(src);
        const __m128i* qtable = reinterpret_cast<const __m128i *>(qt);

        // Load and dequantize
        __m128i v0 = _mm_mullo_epi16(data[0], qtable[0]);
        __m128i v1 = _mm_mullo_epi16(data[1], qtable[1]);
        __m128i v2 = _mm_mullo_epi16(data[2], qtable[2]);
        __m128i v3 = _mm_mullo_epi16(data[3], qtable[3]);
        __m128i v4 = _mm_mullo_epi16(data[4], qtable[4]);
        __m128i v5 = _mm_mullo_epi16(data[5], qtable[5]);
        __m128i v6 = _mm_mullo_epi16(data[6], qtable[6]);
        __m128i v7 = _mm_mullo_epi16(data[7], qtable[7]);

        // IDCT columns
        JPEG_IDCT_IDCT_PASS_XMM(colBias, 10)

        // Transpose
        interleave16(v0, v4);
        interleave16(v2, v6);
        interleave16(v1, v5);
        interleave16(v3, v7);

        interleave16(v0, v2);
        interleave16(v1, v3);
        interleave16(v4, v6);
        interleave16(v5, v7);

        interleave16(v0, v1);
        interleave16(v2, v3);
        interleave16(v4, v5);
        interleave16(v6, v7);

        // IDCT rows
        JPEG_IDCT_IDCT_PASS_XMM(rowBias, 17)

        // Pack to 8-bit integers, also saturates the result to 0..255
        __m128i s0 = _mm_packus_epi16(v0, v1);
        __m128i s1 = _mm_packus_epi16(v2, v3);
        __m128i s2 = _mm_packus_epi16(v4, v5);
        __m128i s3 = _mm_packus_epi16(v6, v7);

        // Transpose
        interleave8(s0, s2);
        interleave8(s1, s3);
        interleave8(s0, s1);
        interleave8(s2, s3);
        interleave8(s0, s2);
        interleave8(s1, s3);

        // Store
        __m128i* d = reinterpret_cast<__m128i *>(dest);
        _mm_storeu_si128(d + 0, s0);
        _mm_storeu_si128(d + 1, s2);
        _mm_storeu_si128(d + 2, s1);
        _mm_storeu_si128(d + 3, s3);
    }

#endif // JPEG_ENABLE_SSE2

#if defined(JPEG_ENABLE_NEON)

    // ------------------------------------------------------------------------------------------------
    // NEON implementation
    // ------------------------------------------------------------------------------------------------

    // The original code is by Sean Barrett ; WE HAVE TAKEN LIBERTIES TO ADAPT IT TO OUR USE!!!
    // https://github.com/nothings/stb
    // [License]
    // Public Domain / MIT

#define stbi__f2f(x)  ((int) (((x) * 4096 + 0.5)))

static void stbi__idct_simd(u8 *out, const short data[64], const u16* qt)
{
   int16x8_t row0, row1, row2, row3, row4, row5, row6, row7;

   int16x4_t rot0_0 = vdup_n_s16(stbi__f2f(0.5411961f));
   int16x4_t rot0_1 = vdup_n_s16(stbi__f2f(-1.847759065f));
   int16x4_t rot0_2 = vdup_n_s16(stbi__f2f( 0.765366865f));
   int16x4_t rot1_0 = vdup_n_s16(stbi__f2f( 1.175875602f));
   int16x4_t rot1_1 = vdup_n_s16(stbi__f2f(-0.899976223f));
   int16x4_t rot1_2 = vdup_n_s16(stbi__f2f(-2.562915447f));
   int16x4_t rot2_0 = vdup_n_s16(stbi__f2f(-1.961570560f));
   int16x4_t rot2_1 = vdup_n_s16(stbi__f2f(-0.390180644f));
   int16x4_t rot3_0 = vdup_n_s16(stbi__f2f( 0.298631336f));
   int16x4_t rot3_1 = vdup_n_s16(stbi__f2f( 2.053119869f));
   int16x4_t rot3_2 = vdup_n_s16(stbi__f2f( 3.072711026f));
   int16x4_t rot3_3 = vdup_n_s16(stbi__f2f( 1.501321110f));

#define dct_long_mul(out, inq, coeff) \
   int32x4_t out##_l = vmull_s16(vget_low_s16(inq), coeff); \
   int32x4_t out##_h = vmull_s16(vget_high_s16(inq), coeff)

#define dct_long_mac(out, acc, inq, coeff) \
   int32x4_t out##_l = vmlal_s16(acc##_l, vget_low_s16(inq), coeff); \
   int32x4_t out##_h = vmlal_s16(acc##_h, vget_high_s16(inq), coeff)

#define dct_widen(out, inq) \
   int32x4_t out##_l = vshll_n_s16(vget_low_s16(inq), 12); \
   int32x4_t out##_h = vshll_n_s16(vget_high_s16(inq), 12)

// wide add
#define dct_wadd(out, a, b) \
   int32x4_t out##_l = vaddq_s32(a##_l, b##_l); \
   int32x4_t out##_h = vaddq_s32(a##_h, b##_h)

// wide sub
#define dct_wsub(out, a, b) \
   int32x4_t out##_l = vsubq_s32(a##_l, b##_l); \
   int32x4_t out##_h = vsubq_s32(a##_h, b##_h)

// butterfly a/b, then shift using "shiftop" by "s" and pack
#define dct_bfly32o(out0,out1, a,b,shiftop,s) \
   { \
      dct_wadd(sum, a, b); \
      dct_wsub(dif, a, b); \
      out0 = vcombine_s16(shiftop(sum_l, s), shiftop(sum_h, s)); \
      out1 = vcombine_s16(shiftop(dif_l, s), shiftop(dif_h, s)); \
   }

#define dct_pass(shiftop, shift) \
   { \
      /* even part */ \
      int16x8_t sum26 = vaddq_s16(row2, row6); \
      dct_long_mul(p1e, sum26, rot0_0); \
      dct_long_mac(t2e, p1e, row6, rot0_1); \
      dct_long_mac(t3e, p1e, row2, rot0_2); \
      int16x8_t sum04 = vaddq_s16(row0, row4); \
      int16x8_t dif04 = vsubq_s16(row0, row4); \
      dct_widen(t0e, sum04); \
      dct_widen(t1e, dif04); \
      dct_wadd(x0, t0e, t3e); \
      dct_wsub(x3, t0e, t3e); \
      dct_wadd(x1, t1e, t2e); \
      dct_wsub(x2, t1e, t2e); \
      /* odd part */ \
      int16x8_t sum15 = vaddq_s16(row1, row5); \
      int16x8_t sum17 = vaddq_s16(row1, row7); \
      int16x8_t sum35 = vaddq_s16(row3, row5); \
      int16x8_t sum37 = vaddq_s16(row3, row7); \
      int16x8_t sumodd = vaddq_s16(sum17, sum35); \
      dct_long_mul(p5o, sumodd, rot1_0); \
      dct_long_mac(p1o, p5o, sum17, rot1_1); \
      dct_long_mac(p2o, p5o, sum35, rot1_2); \
      dct_long_mul(p3o, sum37, rot2_0); \
      dct_long_mul(p4o, sum15, rot2_1); \
      dct_wadd(sump13o, p1o, p3o); \
      dct_wadd(sump24o, p2o, p4o); \
      dct_wadd(sump23o, p2o, p3o); \
      dct_wadd(sump14o, p1o, p4o); \
      dct_long_mac(x4, sump13o, row7, rot3_0); \
      dct_long_mac(x5, sump24o, row5, rot3_1); \
      dct_long_mac(x6, sump23o, row3, rot3_2); \
      dct_long_mac(x7, sump14o, row1, rot3_3); \
      dct_bfly32o(row0,row7, x0,x7,shiftop,shift); \
      dct_bfly32o(row1,row6, x1,x6,shiftop,shift); \
      dct_bfly32o(row2,row5, x2,x5,shiftop,shift); \
      dct_bfly32o(row3,row4, x3,x4,shiftop,shift); \
   }

   // load
   row0 = vld1q_s16(data + 0*8);
   row1 = vld1q_s16(data + 1*8);
   row2 = vld1q_s16(data + 2*8);
   row3 = vld1q_s16(data + 3*8);
   row4 = vld1q_s16(data + 4*8);
   row5 = vld1q_s16(data + 5*8);
   row6 = vld1q_s16(data + 6*8);
   row7 = vld1q_s16(data + 7*8);

   int16x8_t qt0 = vreinterpretq_s16_u16(vld1q_u16(qt + 0*8));
   int16x8_t qt1 = vreinterpretq_s16_u16(vld1q_u16(qt + 1*8));
   int16x8_t qt2 = vreinterpretq_s16_u16(vld1q_u16(qt + 2*8));
   int16x8_t qt3 = vreinterpretq_s16_u16(vld1q_u16(qt + 3*8));
   int16x8_t qt4 = vreinterpretq_s16_u16(vld1q_u16(qt + 4*8));
   int16x8_t qt5 = vreinterpretq_s16_u16(vld1q_u16(qt + 5*8));
   int16x8_t qt6 = vreinterpretq_s16_u16(vld1q_u16(qt + 6*8));
   int16x8_t qt7 = vreinterpretq_s16_u16(vld1q_u16(qt + 7*8));

   row0 = vmulq_s16(row0, qt0);
   row1 = vmulq_s16(row1, qt1);
   row2 = vmulq_s16(row2, qt2);
   row3 = vmulq_s16(row3, qt3);
   row4 = vmulq_s16(row4, qt4);
   row5 = vmulq_s16(row5, qt5);
   row6 = vmulq_s16(row6, qt6);
   row7 = vmulq_s16(row7, qt7);

   // add DC bias
   row0 = vaddq_s16(row0, vsetq_lane_s16(1024, vdupq_n_s16(0), 0));

   // column pass
   dct_pass(vrshrn_n_s32, 10);

   // 16bit 8x8 transpose
   {
// these three map to a single VTRN.16, VTRN.32, and VSWP, respectively.
// whether compilers actually get this is another story, sadly.
#define dct_trn16(x, y) { int16x8x2_t t = vtrnq_s16(x, y); x = t.val[0]; y = t.val[1]; }
#define dct_trn32(x, y) { int32x4x2_t t = vtrnq_s32(vreinterpretq_s32_s16(x), vreinterpretq_s32_s16(y)); x = vreinterpretq_s16_s32(t.val[0]); y = vreinterpretq_s16_s32(t.val[1]); }
#define dct_trn64(x, y) { int16x8_t x0 = x; int16x8_t y0 = y; x = vcombine_s16(vget_low_s16(x0), vget_low_s16(y0)); y = vcombine_s16(vget_high_s16(x0), vget_high_s16(y0)); }

      // pass 1
      dct_trn16(row0, row1); // a0b0a2b2a4b4a6b6
      dct_trn16(row2, row3);
      dct_trn16(row4, row5);
      dct_trn16(row6, row7);

      // pass 2
      dct_trn32(row0, row2); // a0b0c0d0a4b4c4d4
      dct_trn32(row1, row3);
      dct_trn32(row4, row6);
      dct_trn32(row5, row7);

      // pass 3
      dct_trn64(row0, row4); // a0b0c0d0e0f0g0h0
      dct_trn64(row1, row5);
      dct_trn64(row2, row6);
      dct_trn64(row3, row7);

#undef dct_trn16
#undef dct_trn32
#undef dct_trn64
   }

   // row pass
   // vrshrn_n_s32 only supports shifts up to 16, we need
   // 17. so do a non-rounding shift of 16 first then follow
   // up with a rounding shift by 1.
   dct_pass(vshrn_n_s32, 16);

   {
      // pack and round
      uint8x8_t p0 = vqrshrun_n_s16(row0, 1);
      uint8x8_t p1 = vqrshrun_n_s16(row1, 1);
      uint8x8_t p2 = vqrshrun_n_s16(row2, 1);
      uint8x8_t p3 = vqrshrun_n_s16(row3, 1);
      uint8x8_t p4 = vqrshrun_n_s16(row4, 1);
      uint8x8_t p5 = vqrshrun_n_s16(row5, 1);
      uint8x8_t p6 = vqrshrun_n_s16(row6, 1);
      uint8x8_t p7 = vqrshrun_n_s16(row7, 1);

      // again, these can translate into one instruction, but often don't.
#define dct_trn8_8(x, y) { uint8x8x2_t t = vtrn_u8(x, y); x = t.val[0]; y = t.val[1]; }
#define dct_trn8_16(x, y) { uint16x4x2_t t = vtrn_u16(vreinterpret_u16_u8(x), vreinterpret_u16_u8(y)); x = vreinterpret_u8_u16(t.val[0]); y = vreinterpret_u8_u16(t.val[1]); }
#define dct_trn8_32(x, y) { uint32x2x2_t t = vtrn_u32(vreinterpret_u32_u8(x), vreinterpret_u32_u8(y)); x = vreinterpret_u8_u32(t.val[0]); y = vreinterpret_u8_u32(t.val[1]); }

      // sadly can't use interleaved stores here since we only write
      // 8 bytes to each scan line!

      // 8x8 8-bit transpose pass 1
      dct_trn8_8(p0, p1);
      dct_trn8_8(p2, p3);
      dct_trn8_8(p4, p5);
      dct_trn8_8(p6, p7);

      // pass 2
      dct_trn8_16(p0, p2);
      dct_trn8_16(p1, p3);
      dct_trn8_16(p4, p6);
      dct_trn8_16(p5, p7);

      // pass 3
      dct_trn8_32(p0, p4);
      dct_trn8_32(p1, p5);
      dct_trn8_32(p2, p6);
      dct_trn8_32(p3, p7);

      // store
      vst1_u8(out, p0); out += 8;
      vst1_u8(out, p1); out += 8;
      vst1_u8(out, p2); out += 8;
      vst1_u8(out, p3); out += 8;
      vst1_u8(out, p4); out += 8;
      vst1_u8(out, p5); out += 8;
      vst1_u8(out, p6); out += 8;
      vst1_u8(out, p7);

#undef dct_trn8_8
#undef dct_trn8_16
#undef dct_trn8_32
   }

#undef dct_long_mul
#undef dct_long_mac
#undef dct_widen
#undef dct_wadd
#undef dct_wsub
#undef dct_bfly32o
#undef dct_pass
}


    void idct_neon(u8* dest, const s16* data, const u16* qt)
    {
        stbi__idct_simd(dest, data, qt);
    }

#endif // JPEG_ENABLE_NEON

} // namespace jpeg
