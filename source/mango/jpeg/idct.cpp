/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "jpeg.hpp"

namespace
{

    using namespace mango;

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

} // namespace

namespace jpeg
{

    // ------------------------------------------------------------------------------------------------
    // Generic C++ implementation
    // ------------------------------------------------------------------------------------------------

    void idct(uint8* dest, int stride, const BlockType* data, const uint16* qt)
    {
        int temp[64];
        int* v;

        const int16_t *s = data;

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

        for (int i = 0; i < 8; ++i)
        {
            IDCT idct;
            idct.compute(v[0], v[8], v[16], v[24], v[32], v[40], v[48], v[56]);
            ++v;
            const int bias = 0x10000 + (128 << 17);
            idct.x0 += bias;
            idct.x1 += bias;
            idct.x2 += bias;
            idct.x3 += bias;
            dest[0] = byteclamp((idct.x0 + idct.y3) >> 17);
            dest[1] = byteclamp((idct.x1 + idct.y2) >> 17);
            dest[2] = byteclamp((idct.x2 + idct.y1) >> 17);
            dest[3] = byteclamp((idct.x3 + idct.y0) >> 17);
            dest[4] = byteclamp((idct.x3 - idct.y0) >> 17);
            dest[5] = byteclamp((idct.x2 - idct.y1) >> 17);
            dest[6] = byteclamp((idct.x1 - idct.y2) >> 17);
            dest[7] = byteclamp((idct.x0 - idct.y3) >> 17);
            dest += stride;
        }
    }

#if defined(JPEG_ENABLE_SIMD)

    // ------------------------------------------------------------------------------------------------
    // SIMD implementation
    // ------------------------------------------------------------------------------------------------

    static inline uint8x16 packRow(float32x4 x, float32x4 y)
    {
        float32x4 a = x - y;
        float32x4 b = x + y;
        b = b.wzyx;
        int16x8 c0 = simd::narrow(convert<int32x4>(a).m, convert<int32x4>(b).m);
        uint16x8 c1 = reinterpret<uint16x8>(c0);
        return simd::narrow(c1.m, c1.m);
    }

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

    void idct_simd(uint8* dest, int stride, const BlockType* data, const uint16* qt)
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

            if (stride == 8)
            {
                uint8x16* d = reinterpret_cast<uint8x16 *>(dest);
                d[0] = packRow2(x3, y3, x2, y2);
                d[1] = packRow2(x1, y1, x0, y0);
            }
            else
            {
                store_low(dest + stride * 0, packRow(x3, y3));
                store_low(dest + stride * 1, packRow(x2, y2));
                store_low(dest + stride * 2, packRow(x1, y1));
                store_low(dest + stride * 3, packRow(x0, y0));
            }

            dest += stride * 4;
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

#if defined(_MSC_VER)
    #define SIMD_ALIGN_VAR(type, name, alignment) __declspec(align(alignment)) type name
#else
    #define SIMD_ALIGN_VAR(type, name, alignment) type __attribute__((__aligned__(alignment))) name
#endif

    // Derived from jidctint's `jpeg_idct_islow`
    constexpr int JPEG_IDCT_PREC = 12;

    constexpr int JPEG_IDCT_HALF(int precision)
    {
        return (1 << ((precision) - 1));
    }

    constexpr int JPEG_IDCT_FIXED(double x)
    {
        return static_cast<int>(((double)(x) * (double)(1 << JPEG_IDCT_PREC) + 0.5));
    }

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

    struct DeJPEG_SSE2Consts
    {
        int16_t rot0_0[8], rot0_1[8];
        int16_t rot1_0[8], rot1_1[8];
        int16_t rot2_0[8], rot2_1[8];
        int16_t rot3_0[8], rot3_1[8];
        int32_t colBias[4];
        int32_t rowBias[4];
    };

#define DATA_4X(...) { __VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__ }
    SIMD_ALIGN_VAR(static const DeJPEG_SSE2Consts, dejpeg_sse2_consts, 16) = {
        DATA_4X(JPEG_IDCT_P_0_541196100                          , JPEG_IDCT_P_0_541196100 + JPEG_IDCT_M_1_847759065),
        DATA_4X(JPEG_IDCT_P_0_541196100 + JPEG_IDCT_P_0_765366865, JPEG_IDCT_P_0_541196100                          ),
        DATA_4X(JPEG_IDCT_P_1_175875602 + JPEG_IDCT_M_0_899976223, JPEG_IDCT_P_1_175875602                          ),
        DATA_4X(JPEG_IDCT_P_1_175875602                          , JPEG_IDCT_P_1_175875602 + JPEG_IDCT_M_2_562915447),
        DATA_4X(JPEG_IDCT_M_1_961570560 + JPEG_IDCT_P_0_298631336, JPEG_IDCT_M_1_961570560                          ),
        DATA_4X(JPEG_IDCT_M_1_961570560                          , JPEG_IDCT_M_1_961570560 + JPEG_IDCT_P_3_072711026),
        DATA_4X(JPEG_IDCT_M_0_390180644 + JPEG_IDCT_P_2_053119869, JPEG_IDCT_M_0_390180644                          ),
        DATA_4X(JPEG_IDCT_M_0_390180644                          , JPEG_IDCT_M_0_390180644 + JPEG_IDCT_P_1_501321110),
        DATA_4X(JPEG_IDCT_COL_BIAS),
        DATA_4X(JPEG_IDCT_ROW_BIAS),
    };
#undef DATA_4X

#define JPEG_CONST_XMM(x) (*(const __m128i*)(dejpeg_sse2_consts.x))

#if 1

#define JPEG_IDCT_ROTATE_XMM(dst0, dst1, x, y, c0, c1) \
    __m128i c0##_l = _mm_unpacklo_epi16(x, y); \
    __m128i c0##_h = _mm_unpackhi_epi16(x, y); \
    __m128i dst0##_l = _mm_madd_epi16(c0##_l, JPEG_CONST_XMM(c0)); \
    __m128i dst0##_h = _mm_madd_epi16(c0##_h, JPEG_CONST_XMM(c0)); \
    __m128i dst1##_l = _mm_madd_epi16(c0##_l, JPEG_CONST_XMM(c1)); \
    __m128i dst1##_h = _mm_madd_epi16(c0##_h, JPEG_CONST_XMM(c1));

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

#else

    // WIP: AVX2 idct
    
    struct DeJPEG_AVXConsts
    {
        int16 rot0_0[16], rot0_1[16];
        int16 rot1_0[16], rot1_1[16];
        int16 rot2_0[16], rot2_1[16];
        int16 rot3_0[16], rot3_1[16];
        int32 colBias[8];
        int32 rowBias[8];
    };

#define DATA_8X(...) { __VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__ }
    SIMD_ALIGN_VAR(static const DeJPEG_AVXConsts, dejpeg_avx_consts, 32) = {
        DATA_8X(JPEG_IDCT_P_0_541196100                          , JPEG_IDCT_P_0_541196100 + JPEG_IDCT_M_1_847759065),
        DATA_8X(JPEG_IDCT_P_0_541196100 + JPEG_IDCT_P_0_765366865, JPEG_IDCT_P_0_541196100                          ),
        DATA_8X(JPEG_IDCT_P_1_175875602 + JPEG_IDCT_M_0_899976223, JPEG_IDCT_P_1_175875602                          ),
        DATA_8X(JPEG_IDCT_P_1_175875602                          , JPEG_IDCT_P_1_175875602 + JPEG_IDCT_M_2_562915447),
        DATA_8X(JPEG_IDCT_M_1_961570560 + JPEG_IDCT_P_0_298631336, JPEG_IDCT_M_1_961570560                          ),
        DATA_8X(JPEG_IDCT_M_1_961570560                          , JPEG_IDCT_M_1_961570560 + JPEG_IDCT_P_3_072711026),
        DATA_8X(JPEG_IDCT_M_0_390180644 + JPEG_IDCT_P_2_053119869, JPEG_IDCT_M_0_390180644                          ),
        DATA_8X(JPEG_IDCT_M_0_390180644                          , JPEG_IDCT_M_0_390180644 + JPEG_IDCT_P_1_501321110),
        DATA_8X(JPEG_IDCT_COL_BIAS),
        DATA_8X(JPEG_IDCT_ROW_BIAS),
    };
#undef DATA_8X
    
#define JPEG_CONST_XMM(x) (*(const __m256i*)(dejpeg_avx_consts.x))
    
#define JPEG_IDCT_ROTATE_XMM(dst0, dst1, x, y, c0, c1) \
    __m256i c0##x = _mm256_setr_m128i(_mm_unpacklo_epi16(x, y), \
    _mm_unpackhi_epi16(x, y)); \
    __m256i dst0 = _mm256_madd_epi16(c0##x, JPEG_CONST_XMM(c0)); \
    __m256i dst1 = _mm256_madd_epi16(c0##x, JPEG_CONST_XMM(c1));
    
    // out = in << 12  (in 16-bit, out 32-bit)
#define JPEG_IDCT_WIDEN_XMM(dst, in) \
    __m256i dst = _mm256_setr_m128i(_mm_unpacklo_epi16(_mm_setzero_si128(), in), \
    _mm_unpackhi_epi16(_mm_setzero_si128(), in)); \
    dst = _mm256_srai_epi32(dst, 4);
    
    // wide add
#define JPEG_IDCT_WADD_XMM(dst, a, b) \
    __m256i dst = _mm256_add_epi32(a, b);
    
    // wide sub
#define JPEG_IDCT_WSUB_XMM(dst, a, b) \
    __m256i dst = _mm256_sub_epi32(a, b);
    
    // butterfly a/b, add bias, then shift by `norm` and pack to 16-bit
#define JPEG_IDCT_BFLY_XMM(dst0, dst1, a, b, bias, norm) { \
    __m256i a_biased = _mm256_add_epi32(a, bias); \
    JPEG_IDCT_WADD_XMM(sum, a_biased, b) \
    JPEG_IDCT_WSUB_XMM(diff, a_biased, b) \
    sum = _mm256_srai_epi32(sum, norm); \
    diff = _mm256_srai_epi32(diff, norm); \
    dst0 = _mm_packs_epi32(_mm256_extractf128_si256(sum, 0), _mm256_extractf128_si256(sum, 1)); \
    dst1 = _mm_packs_epi32(_mm256_extractf128_si256(diff, 0), _mm256_extractf128_si256(diff, 1)); \
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

#endif

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

    void idct_sse2(uint8* dest, int stride, const BlockType* src, const uint16* qt)
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
        JPEG_IDCT_IDCT_PASS_XMM(JPEG_CONST_XMM(colBias), 10)

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
        JPEG_IDCT_IDCT_PASS_XMM(JPEG_CONST_XMM(rowBias), 17)

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
        if (stride == 8)
        {
            __m128i* d = reinterpret_cast<__m128i *>(dest);
            d[0] = s0;
            d[1] = s2;
            d[2] = s1;
            d[3] = s3;
        }
        else
        {
            uint8* dst0 = dest;
            uint8* dst1 = dest + stride;
            int dstStride2 = stride * 2;

            _mm_storel_pi((__m64 *)dst0, _mm_castsi128_ps(s0)); dst0 += dstStride2;
            _mm_storeh_pi((__m64 *)dst1, _mm_castsi128_ps(s0)); dst1 += dstStride2;

            _mm_storel_pi((__m64 *)dst0, _mm_castsi128_ps(s2)); dst0 += dstStride2;
            _mm_storeh_pi((__m64 *)dst1, _mm_castsi128_ps(s2)); dst1 += dstStride2;

            _mm_storel_pi((__m64 *)dst0, _mm_castsi128_ps(s1)); dst0 += dstStride2;
            _mm_storeh_pi((__m64 *)dst1, _mm_castsi128_ps(s1)); dst1 += dstStride2;

            _mm_storel_pi((__m64 *)dst0, _mm_castsi128_ps(s3));
            _mm_storeh_pi((__m64 *)dst1, _mm_castsi128_ps(s3));
        }
    }

#endif // JPEG_ENABLE_SSE2

} // namespace jpeg
