/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "jpeg.hpp"

// ----------------------------------------------------------------------------------------------------
// Generic C++ implementation
// ----------------------------------------------------------------------------------------------------

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

} // namespace jpeg

// ----------------------------------------------------------------------------------------------------
// SSE2 implementation
// ----------------------------------------------------------------------------------------------------

#if defined(JPEG_ENABLE_SSE4)

#define SHUFFLE(reg,x,y,z,w) \
    _mm_shuffle_ps(reg, reg, _MM_SHUFFLE(x, y, z, w))

#define DATAQT(x) \
    _mm_cvtepi32_ps(_mm_cvtepi16_epi32(_mm_mullo_epi16( \
    _mm_loadl_epi64((__m128i const*)(data + x)), \
    _mm_loadl_epi64((__m128i const*)(qt + x)))));

#define SH(reg, comp) \
    _mm_shuffle_ps(reg, reg, _MM_SHUFFLE(comp,comp,comp,comp))

#define _mm_madd_ps(s, a, b) \
    _mm_add_ps(s, _mm_mul_ps(a, b))

namespace jpeg
{

    static inline __m128i packRow(__m128 x, __m128 y)
    {
        __m128 a = _mm_sub_ps(x, y);
        __m128 b = _mm_add_ps(x, y);
        b = _mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 1, 2, 3));
        __m128i c = _mm_packs_epi32 (_mm_cvtps_epi32(a), _mm_cvtps_epi32(b));
        return _mm_packus_epi16(c, c);
    }
    
    void idct_sse41(uint8* dest, int stride, const BlockType* data, const uint16* qt)
    {
        __m128 temp[16];
        float* v;

        v = reinterpret_cast<float*>(temp);

        const __m128 f0 = _mm_setr_ps(-0.3535533905f, 0.3535533905f, 128.0f,        128.0f);
        const __m128 f1 = _mm_setr_ps( 0.4619397662f, 0.1913417161f,-0.4619397662f,-0.1913417161f);
        const __m128 c0 = SHUFFLE(f0, 1, 1, 1, 1);
        const __m128 c1 = SHUFFLE(f1, 2, 3, 1, 0);
        const __m128 c2 = SHUFFLE(f0, 1, 0, 0, 1);
        const __m128 c3 = SHUFFLE(f1, 3, 0, 2, 1);
        const __m128 c4 = _mm_setr_ps(-0.4903926402f,-0.4157348061f,-0.2777851165f,-0.0975451610f);
        const __m128 c5 = _mm_setr_ps(-0.4157348061f, 0.0975451610f, 0.4903926402f, 0.2777851165f);
        const __m128 c6 = _mm_setr_ps(-0.2777851165f, 0.4903926402f,-0.0975451610f,-0.4157348061f);
        const __m128 c7 = _mm_setr_ps(-0.0975451610f, 0.2777851165f,-0.4157348061f, 0.4903926402f);
        const __m128 c8 = SHUFFLE(f0, 3, 3, 3, 3);

        for (int i = 0; i < 8; ++i)
        {
            __m128 s0 = DATAQT(0);
            __m128 s1 = DATAQT(4);

            __m128 v0 = SH(s0, 0);
            __m128 v2 = SH(s0, 2);
            __m128 v4 = SH(s1, 0);
            __m128 v6 = SH(s1, 2);
            __m128 x = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_mul_ps(v0, c0), v2, c1), v4, c2), v6, c3);

            __m128 v1 = SH(s0, 1);
            __m128 v3 = SH(s0, 3);
            __m128 v5 = SH(s1, 1);
            __m128 v7 = SH(s1, 3);
            __m128 y = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_mul_ps(v1, c4), v3, c5), v5, c6), v7, c7);

            __m128 a = _mm_add_ps(x, y);
            __m128 b = _mm_sub_ps(x, y);
            b = _mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 1, 2, 3));

            _mm_store_ps(v +  0, a);
            _mm_store_ps(v +  4, b);

            data += 8;
            qt += 8;
            v += 8;
        }

        v = reinterpret_cast<float*>(temp) + 4;

        for (int i = 0; i < 2; ++i)
        {
            __m128 v0 = _mm_load_ps(v + 0 * 8);
            __m128 v2 = _mm_load_ps(v + 2 * 8);
            __m128 v4 = _mm_load_ps(v + 4 * 8);
            __m128 v6 = _mm_load_ps(v + 6 * 8);
            __m128 x0 = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_mul_ps(SH(v0, 0), c0), SH(v2, 0), c1), SH(v4, 0), c2), SH(v6, 0), c3);
            __m128 x1 = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_mul_ps(SH(v0, 1), c0), SH(v2, 1), c1), SH(v4, 1), c2), SH(v6, 1), c3);
            __m128 x2 = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_mul_ps(SH(v0, 2), c0), SH(v2, 2), c1), SH(v4, 2), c2), SH(v6, 2), c3);
            __m128 x3 = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_mul_ps(SH(v0, 3), c0), SH(v2, 3), c1), SH(v4, 3), c2), SH(v6, 3), c3);

            x0 = _mm_add_ps(x0, c8);
            x1 = _mm_add_ps(x1, c8);
            x2 = _mm_add_ps(x2, c8);
            x3 = _mm_add_ps(x3, c8);

            __m128 v1 = _mm_load_ps(v + 1 * 8);
            __m128 v3 = _mm_load_ps(v + 3 * 8);
            __m128 v5 = _mm_load_ps(v + 5 * 8);
            __m128 v7 = _mm_load_ps(v + 7 * 8);
            __m128 y0 = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_mul_ps(SH(v1, 0), c4), SH(v3, 0), c5), SH(v5, 0), c6), SH(v7, 0), c7);
            __m128 y1 = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_mul_ps(SH(v1, 1), c4), SH(v3, 1), c5), SH(v5, 1), c6), SH(v7, 1), c7);
            __m128 y2 = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_mul_ps(SH(v1, 2), c4), SH(v3, 2), c5), SH(v5, 2), c6), SH(v7, 2), c7);
            __m128 y3 = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_mul_ps(SH(v1, 3), c4), SH(v3, 3), c5), SH(v5, 3), c6), SH(v7, 3), c7);

            // NOTE: We could pack and store two rows simultaneously for cases where the stride is 8.
            //       However; this scenario is statistically very inpropable so we go with writing only 64 bits (one row) at a time.
            _mm_storel_epi64((__m128i*)(dest + stride * 0), packRow(x3, y3));
            _mm_storel_epi64((__m128i*)(dest + stride * 1), packRow(x2, y2));
            _mm_storel_epi64((__m128i*)(dest + stride * 2), packRow(x1, y1));
            _mm_storel_epi64((__m128i*)(dest + stride * 3), packRow(x0, y0));

            dest += stride * 4;
            v -= 4;
        }
    }

} // namespace jpeg

#endif
