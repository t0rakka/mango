/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>

namespace
{

    using namespace mango;
    using namespace mango::math;
    using namespace mango::image;

    template <u32 bits>
    class CubicTable
    {
    public:
        enum : u32
        {
            size = 1 << bits,
            mask = (1 << bits) - 1
        };

        CubicTable()
        {
            const float32x4 delta(1.0f / -float(size));
            const float32x4 scale(-4.0f / 6.0f, 1.0f, -4.0f / 6.0f, 1.0f / 6.0f);

            float32x4 series0(-2.0f, -1.0f, 0.0f, 1.0f);
            float32x4 series1(-1.0f,  0.0f, 1.0f, 2.0f);
            float32x4 series2( 0.0f,  1.0f, 2.0f, 3.0f);
            float32x4 series3( 1.0f,  2.0f, 3.0f, 4.0f);

            for (u32 i = 0; i < size; ++i)
            {
                const float32x4 p0 = cubed(series0) * scale.x;
                const float32x4 p1 = cubed(series1) * scale.y;
                const float32x4 p2 = cubed(series2) * scale.z;
                const float32x4 p3 = cubed(series3) * scale.w;
                series0 += delta;
                series1 += delta;
                series2 += delta;
                series3 += delta;
                m_table[i] = p0 + p1 + p2 + p3;

                int32x4 scale = convert<int32x4>(m_table[i] * 256.0f);
                m_s16_table[i] = simd::narrow(scale, scale);            
            }
        }

        ~CubicTable()
        {
        }

        const float32x4 cubicfv(int index) const
        {
            return m_table[index & mask];
        }

        const int16x8 cubiciv(int index) const
        {
            return m_s16_table[index & mask];
        }

    protected:
        float32x4 m_table[size];
        int16x8 m_s16_table[size];

        float32x4 cubed(const float32x4& value) const
        {
            const float32x4 zero(0.0f);
            const float32x4 s = max(zero, value);
            return s * s * s;
        }
    };

    static inline
    float32x4 unpack(const u32* scan, int x)
    {
        int32x4 s = simd::unpack(scan[x]);
        return convert<float32x4>(s);
    }

    static inline
    u32 pack(float32x4 s)
    {
        return simd::pack(convert<int32x4>(s));
    }

    void u32_bicubic_xmin_ymin(const Surface& dest, const Surface& source, float xpos, float ypos, float xsize, float ysize)
    {
        const float inv_area = 1.0f / ((xsize / dest.width) * (ysize / dest.height));

        const float dx = xsize / dest.width;
        const float dy = ysize / dest.height;

        ConcurrentQueue q;

        const int block = 32;

        for (int y0 = 0; y0 < dest.height; y0 += block)
        {
            const int y1 = std::min(dest.height, y0 + block);

            q.enqueue([=]
            {
                float fy = ypos + y0 * dy;

                for (int y = y0; y < y1; ++y)
                {
                    const int py0 = int(fy * 256.0f); fy += dy;
                    const int py1 = int(fy * 256.0f);

                    const float fy0 = (py0 & 0xff) / 256.0f * inv_area;
                    const float fy1 = (1.0f - ((py1 & 0xff) / 256.0f)) * inv_area;

                    const int iy0 = py0 >> 8;
                    const int iy1 = std::min(source.height - 1, py1 >> 8);
            
                    u32* buffer = dest.address<u32>(0, y);
            
                    float fx = xpos;

                    for (int x = 0; x < dest.width; ++x)
                    {
                        const int px0 = int(fx * 256.0f); fx += dx;
                        const int px1 = int(fx * 256.0f);

                        const float fx0 = (px0 & 0xff) / 256.0f;
                        const float fx1 = 1.0f - ((px1 & 0xff) / 256.0f);

                        const int ix0 = px0 >> 8;
                        const int ix1 = std::min(source.width - 1, px1 >> 8);
        
                        float32x4 v = 0.0f;
        
                        for (int j = iy0; j <= iy1; ++j)
                        {
                            u32* scan = source.address<u32>(0, j);
        
                            float yfactor = inv_area;
                            if (j == iy0) yfactor -= fy0;
                            if (j == iy1) yfactor -= fy1;
        
                            for (int i = ix0; i <= ix1; ++i)
                            {
                                float xfactor = yfactor;
                                if (i == ix0) xfactor -= fx0 * yfactor;
                                if (i == ix1) xfactor -= fx1 * yfactor;
        
                                v = madd(v, unpack(scan, i), xfactor);
                            }
                        }
        
                        buffer[x] = pack(v);
                    }
                }
            });
        }
    }

    void u32_bicubic_xmin_ymag(const Surface& dest, const Surface& source, float xpos, float ypos, float xsize, float ysize)
    {
        CubicTable<8> table;

        const float inv_area = dest.width / xsize;

        const int ymax = source.height - 1;

        const float dx = xsize / dest.width;
        const float dy = ysize / dest.height;

        ConcurrentQueue q;

        const int block = 32;

        for (int y0 = 0; y0 < dest.height; y0 += block)
        {
            const int y1 = std::min(dest.height, y0 + block);

            q.enqueue([=]
            {
                for (int y = y0; y < y1; ++y)
                {
                    int py = int((ypos + y * dy)  * 256.0f);
                    const float32x4 yscale = table.cubicfv(py);
                    const int iy = py >> 8;

                    const int ny0 = std::max(0, iy - 1);
                    const int ny1 = iy;
                    const int ny2 = std::min(ymax, iy + 1);
                    const int ny3 = std::min(ymax, iy + 2);
    
                    const u32* scan0 = source.address<u32>(0, ny0);
                    const u32* scan1 = source.address<u32>(0, ny1);
                    const u32* scan2 = source.address<u32>(0, ny2);
                    const u32* scan3 = source.address<u32>(0, ny3);

                    u32* buffer = dest.address<u32>(0, y);

                    float fx = xpos;

                    for (int x = 0; x < dest.width; ++x)
                    {
                        const int px0 = int(fx * 256.0f); fx += dx;
                        const int px1 = int(fx * 256.0f);

                        const float fx0 = (px0 & 0xff) / 256.0f;
                        const float fx1 = 1.0f - ((px1 & 0xff) / 256.0f);

                        const int ix0 = px0 >> 8;
                        const int ix1 = std::min(ymax, px1 >> 8);

                        float32x4 v0 = 0.0f;
                        float32x4 v1 = v0;
                        float32x4 v2 = v0;
                        float32x4 v3 = v0;

                        for (int i = ix0; i <= ix1; ++i)
                        {
                            float xfactor = inv_area;
                            if (i == ix0) xfactor -= fx0 * inv_area;
                            if (i == ix1) xfactor -= fx1 * inv_area;

                            const float32x4 xxxx(xfactor);

                            v0 = madd(v0, unpack(scan0, i), xxxx);
                            v1 = madd(v1, unpack(scan1, i), xxxx);
                            v2 = madd(v2, unpack(scan2, i), xxxx);
                            v3 = madd(v3, unpack(scan3, i), xxxx);
                        }

                        float32x4 s = v0 * yscale.xxxx;
                        s = madd(s, v1, yscale.yyyy);
                        s = madd(s, v2, yscale.zzzz);
                        s = madd(s, v3, yscale.wwww);

                        buffer[x] = pack(s);
                    }
                }
            });
        }
    }

    void u32_bicubic_xmag_ymin(const Surface& dest, const Surface& source, float xpos, float ypos, float xsize, float ysize)
    {
        CubicTable<8> table;

        const float inv_area = dest.height / ysize;

        const int xmax = source.width - 1;
        const int ymax = source.height - 1;

        const float dx = xsize / dest.width;
        const float dy = ysize / dest.height;

        ConcurrentQueue q;

        const int block = 32;

        for (int y0 = 0; y0 < dest.height; y0 += block)
        {
            const int y1 = std::min(dest.height, y0 + block);

            q.enqueue([=]
            {
                float fy = ypos + y0 * dy;

                for (int y = y0; y < y1; ++y)
                {
                    const int py0 = int(fy * 256.0f); fy += dy;
                    const int py1 = int(fy * 256.0f);

                    const float fy0 = (py0 & 0xff) / 256.0f * inv_area;
                    const float fy1 = (1.0f - ((py1 & 0xff) / 256.0f)) * inv_area;

                    const int iy0 = py0 >> 8;
                    const int iy1 = std::min(ymax, py1 >> 8);

                    u32* buffer = dest.address<u32>(0, y);

                    for (int x = 0; x < dest.width; ++x)
                    {
                        int px = int((xpos + x * dx) * 256.0f);
                        const float32x4 xscale = table.cubicfv(px);
                        const int ix = px >> 8;

                        const int nx0 = std::max(0, ix - 1);
                        const int nx1 = ix;
                        const int nx2 = std::min(xmax, ix + 1);
                        const int nx3 = std::min(xmax, ix + 2);

                        float32x4 v0 = 0.0f;
                        float32x4 v1 = 0.0f;
                        float32x4 v2 = 0.0f;
                        float32x4 v3 = 0.0f;

                        for (int j = iy0; j <= iy1; ++j)
                        {
                            u32* scan = source.address<u32>(0, j);

                            float yfactor = inv_area;
                            if (j == iy0) yfactor -= fy0;
                            if (j == iy1) yfactor -= fy1;

                            const float32x4 xxxx(yfactor);

                            v0 = madd(v0, unpack(scan, nx0), xxxx);
                            v1 = madd(v1, unpack(scan, nx1), xxxx);
                            v2 = madd(v2, unpack(scan, nx2), xxxx);
                            v3 = madd(v3, unpack(scan, nx3), xxxx);
                        }

                        float32x4 s = v0 * xscale.xxxx;
                        s = madd(s, v1, xscale.yyyy);
                        s = madd(s, v2, xscale.zzzz);
                        s = madd(s, v3, xscale.wwww);

                        buffer[x] = pack(s);
                    }
                }
            });
        }
    }

#if 0

    // reference floating-point implementation

    void u32_bicubic_xmag_ymag(const Surface& dest, const Surface& source, float xpos, float ypos, float xsize, float ysize)
    {
        CubicTable<8> table;

        const int xmin = 0;
        const int ymin = 0;
        const int xmax = source.width - 1;
        const int ymax = source.height - 1;

        const float dx = xsize / dest.width;
        const float dy = ysize / dest.height;

        ConcurrentQueue q;

        const int block = 32;

        for (int y0 = 0; y0 < dest.height; y0 += block)
        {
            const int y1 = std::min(dest.height, y0 + block);

            q.enqueue([=]
            {
                for (int y = y0; y < y1; ++y)
                {
                    int py = int((ypos + dy * y) * 256.0f);
                    const float32x4 yscale = table.cubicfv(py);
                    int iy = py >> 8;

                    const u32* scan0 = source.address<u32>(0, std::max(ymin, iy - 1));
                    const u32* scan1 = source.address<u32>(0, std::max(ymin, iy - 0));
                    const u32* scan2 = source.address<u32>(0, std::min(ymax, iy + 1));
                    const u32* scan3 = source.address<u32>(0, std::min(ymax, iy + 2));

                    u32* buffer = dest.address<u32>(0, y);

                    const int32x4 v_yscale = convert<int32x4>(yscale * 256.0f);

                    for (int x = 0; x < dest.width; ++x)
                    {
                        int px = int((xpos + dx * x) * 256.0f);
                        const float32x4 xscale = table.cubicfv(px);
                        int ix = px >> 8;

                        const int x0 = std::max(xmin, ix - 1);
                        const int x1 = std::max(xmin, ix - 0);
                        const int x2 = std::min(xmax, ix + 1);
                        const int x3 = std::min(xmax, ix + 2);

                        float32x4 s00 = unpack(scan0, x0);
                        float32x4 s01 = unpack(scan0, x1);
                        float32x4 s02 = unpack(scan0, x2);
                        float32x4 s03 = unpack(scan0, x3);
        
                        float32x4 s10 = unpack(scan1, x0);
                        float32x4 s11 = unpack(scan1, x1);
                        float32x4 s12 = unpack(scan1, x2);
                        float32x4 s13 = unpack(scan1, x3);
        
                        float32x4 s20 = unpack(scan2, x0);
                        float32x4 s21 = unpack(scan2, x1);
                        float32x4 s22 = unpack(scan2, x2);
                        float32x4 s23 = unpack(scan2, x3);
        
                        float32x4 s30 = unpack(scan3, x0);
                        float32x4 s31 = unpack(scan3, x1);
                        float32x4 s32 = unpack(scan3, x2);
                        float32x4 s33 = unpack(scan3, x3);
        
                        float32x4 v0 = s00 * xscale.x;
                        v0 = madd(v0, s01, xscale.y);
                        v0 = madd(v0, s02, xscale.z);
                        v0 = madd(v0, s03, xscale.w);

                        float32x4 v1 = s10 * xscale.x;
                        v1 = madd(v1, s11, xscale.y);
                        v1 = madd(v1, s12, xscale.z);
                        v1 = madd(v1, s13, xscale.w);

                        float32x4 v2 = s20 * xscale.x;
                        v2 = madd(v2, s21, xscale.y);
                        v2 = madd(v2, s22, xscale.z);
                        v2 = madd(v2, s23, xscale.w);

                        float32x4 v3 = s30 * xscale.x;
                        v3 = madd(v3, s31, xscale.y);
                        v3 = madd(v3, s32, xscale.z);
                        v3 = madd(v3, s33, xscale.w);

                        float32x4 s = v0 * yscale.x;
                        s = madd(s, v1, yscale.y);
                        s = madd(s, v2, yscale.z);
                        s = madd(s, v3, yscale.w);

                        buffer[x] = pack(s);
                    }
                }
            });
        }
    }

#else

    void u32_bicubic_xmag_ymag(const Surface& dest, const Surface& source, float xpos, float ypos, float xsize, float ysize)
    {
        CubicTable<8> table;

        const int xmin = 0;
        const int ymin = 0;
        const int xmax = source.width - 1;
        const int ymax = source.height - 1;

        const float dx = xsize / dest.width;
        const float dy = ysize / dest.height;

        ConcurrentQueue q;

        const int block = 32;

        for (int y0 = 0; y0 < dest.height; y0 += block)
        {
            const int y1 = std::min(dest.height, y0 + block);

            q.enqueue([=]
            {
                for (int y = y0; y < y1; ++y)
                {
                    int py = int((ypos + dy * y) * 256.0f);
                    const float32x4 yscale = table.cubicfv(py);
                    const int32x4 v_yscale = convert<int32x4>(yscale * 256.0f);
                    int iy = py >> 8;

                    const u32* scan0 = source.address<u32>(0, std::max(ymin, iy - 1));
                    const u32* scan1 = source.address<u32>(0, std::max(ymin, iy - 0));
                    const u32* scan2 = source.address<u32>(0, std::min(ymax, iy + 1));
                    const u32* scan3 = source.address<u32>(0, std::min(ymax, iy + 2));

                    u32* buffer = dest.address<u32>(0, y);

                    for (int x = 0; x < dest.width; ++x)
                    {
                        int px = int((xpos + dx * x) * 256.0f);
                        const int16x8 v_xscale = table.cubiciv(px);
                        int ix = px >> 8;

                        // s0: r0 g0 b0 a0 r1 g1 b1 a1 | r2 g2 b2 a2 r3 g3 b3 a3
                        // s1: r4 g4 b4 a4 r5 g5 b5 a5 | r6 g6 b6 a6 r7 g7 b7 a7
                        // s2: ...
                        // s3: ...
                        uint8x16 s0;
                        uint8x16 s1;
                        uint8x16 s2;
                        uint8x16 s3;

                        if (x >= 1 && x < dest.width - 2)
                        {
                            // no clipping required for interior
                            const int x0 = ix - 1;
                            s0 = uint8x16::uload(scan0 + x0);
                            s1 = uint8x16::uload(scan1 + x0);
                            s2 = uint8x16::uload(scan2 + x0);
                            s3 = uint8x16::uload(scan3 + x0);
                        }
                        else
                        {
                            // clipping needed for edge pixels
                            const int x0 = std::max(xmin, ix - 1);
                            const int x1 = std::max(xmin, ix - 0);
                            const int x2 = std::min(xmax, ix + 1);
                            const int x3 = std::min(xmax, ix + 2);
                            s0 = reinterpret<uint8x16>(uint32x4(scan0[x0], scan0[x1], scan0[x2], scan0[x3]));
                            s1 = reinterpret<uint8x16>(uint32x4(scan1[x0], scan1[x1], scan1[x2], scan1[x3]));
                            s2 = reinterpret<uint8x16>(uint32x4(scan2[x0], scan2[x1], scan2[x2], scan2[x3]));
                            s3 = reinterpret<uint8x16>(uint32x4(scan3[x0], scan3[x1], scan3[x2], scan3[x3]));
                        }

                        uint8x16 t0 = unpacklo(s0, s1); // r0, r4, g0, g4, b0, b4, a0, a4, r1, r5, g1, g5, b1, b5, a1, a5
                        uint8x16 t1 = unpackhi(s0, s1); // r2, r6, g2, g6, b2, b6, a2, a6, r3, r7, g3, g7, b3, b7, a3, a7
                        uint8x16 t2 = unpacklo(t0, t1); // r0, r2, r4, r6, g0, g2, g4, g6, b0, b2, b4, b6, a0, a2, a4, a6
                        uint8x16 t3 = unpackhi(t0, t1); // r1, r3, r5, r7, g1, g3, g5, g7, b1, b3, b5, b7, a1, a3, a5, a7

                        uint8x16 t4 = unpacklo(s2, s3);
                        uint8x16 t5 = unpackhi(s2, s3);
                        uint8x16 t6 = unpacklo(t4, t5);
                        uint8x16 t7 = unpackhi(t4, t5);

                        uint8x16 rg0 = unpacklo(t2, t3); // r0, r1, r2, r3, r4, r5, r6, r7, g0, g1, g2, g3, g4, g5, g6, g7
                        uint8x16 rg1 = unpacklo(t6, t7);

                        uint8x16 ba0 = unpackhi(t2, t3); // b0, b1, b2, b3, b4, b5, b6, b7, a0, a1, a2, a3, a4, a5, a6, a7
                        uint8x16 ba1 = unpackhi(t6, t7);

                        uint8x16 zero(0);

                        int16x8 r0001 = reinterpret<int16x8>(unpacklo(rg0, zero));
                        int16x8 r0203 = reinterpret<int16x8>(unpacklo(rg1, zero));

                        int16x8 g0001 = reinterpret<int16x8>(unpackhi(rg0, zero));
                        int16x8 g0203 = reinterpret<int16x8>(unpackhi(rg1, zero));

                        int16x8 b0001 = reinterpret<int16x8>(unpacklo(ba0, zero));
                        int16x8 b0203 = reinterpret<int16x8>(unpacklo(ba1, zero));

                        int32x4 r_0 = simd::madd(r0001, v_xscale);
                        int32x4 r_1 = simd::madd(r0203, v_xscale);
                        int32x4 r = mullo(hadd(r_0, r_1), v_yscale) >> 16;

                        int32x4 g_0 = simd::madd(g0001, v_xscale);
                        int32x4 g_1 = simd::madd(g0203, v_xscale);
                        int32x4 g = mullo(hadd(g_0, g_1), v_yscale) >> 16;

                        int32x4 b_0 = simd::madd(b0001, v_xscale);
                        int32x4 b_1 = simd::madd(b0203, v_xscale);
                        int32x4 b = mullo(hadd(b_0, b_1), v_yscale) >> 16;

                        int32x4 a(0xff);

                        int32x4 rb0 = unpacklo(r, b);
                        int32x4 rb1 = unpackhi(r, b);
                        int32x4 rb = rb0 + rb1;

                        int32x4 ga0 = unpacklo(g, a);
                        int32x4 ga1 = unpackhi(g, a);
                        int32x4 ga = ga0 + ga1;

                        int32x4 rgba0 = unpacklo(rb, ga);
                        int32x4 rgba1 = unpackhi(rb, ga);
                        u32 rgba = simd::pack(rgba0 + rgba1);

                        buffer[x] = rgba;
                    }
                }
            });
        }
    }

#endif

    bool bicubicCheckFormat(const Format& format)
    {
        if (format.bits != 32 || format.type != Format::UNORM)
            return false;

        return true;
    }

} // namespace

namespace mango::image
{

    void u32_bicubic_blit(const Surface& dest, const Surface& source, float x, float y, float xsize, float ysize)
    {
        if (dest.width < 1 || dest.height < 1)
        {
            printLine(Print::Error, "Incorrect destination surface dimensions: {} x {}", dest.width, dest.height);
            return;
        }

        if (!bicubicCheckFormat(dest.format))
        {
            printLine(Print::Error, "Incorrect dest format.");
            return;
        }

        if (!bicubicCheckFormat(source.format))
        {
            printLine(Print::Error, "Incorrect source format.");
            return;
        }

        if (dest.format != source.format)
        {
            printLine(Print::Error, "Must use matching formats.");
            return;
        }

        const float xscale = dest.width / xsize;
        const float yscale = dest.height / ysize;

        static decltype(u32_bicubic_xmag_ymag)* functionTable [] =
        {
            u32_bicubic_xmin_ymin,
            u32_bicubic_xmag_ymin,
            u32_bicubic_xmin_ymag,
            u32_bicubic_xmag_ymag
        };

        const int index = (xscale >= 1.0f) + (yscale >= 1.0f) * 2;
        functionTable[index](dest, source, x, y, xsize, ysize);
    }

} // namespace mango::image
