/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <cstring>

#include "font_internal.hpp"

#ifdef MANGO_ENABLE_FREETYPE
#include <ft2build.h>
#include FT_OUTLINE_H
#endif

namespace mango::font
{

    namespace
    {
        constexpr float kEpsilon = 1e-4f;

        float32x2 evaluateCubic(const float32x2& p0, const float32x2& p1, const float32x2& p2, const float32x2& p3, float t)
        {
            float s = 1.0f - t;
            float s2 = s * s;
            float s3 = s2 * s;
            float t2 = t * t;
            float t3 = t2 * t;
            return p0 * s3 + p1 * (3.0f * s2 * t) + p2 * (3.0f * s * t2) + p3 * t3;
        }

        bool isMonotonicAxis(float c0, float c1, float c2)
        {
            float qa = c0 + c2 - 2.0f * c1;
            if (std::fabs(qa) < 1e-3f)
            {
                return true;
            }

            float t = (c0 - c1) / qa;
            return t <= kEpsilon || t >= 1.0f - kEpsilon;
        }

        bool isMonotonic(const QuadraticCurve& curve)
        {
            return isMonotonicAxis(curve.p0.x, curve.p1.x, curve.p2.x) &&
                   isMonotonicAxis(curve.p0.y, curve.p1.y, curve.p2.y);
        }

        void subdivideQuadratic(const QuadraticCurve& curve, QuadraticCurve& left, QuadraticCurve& right)
        {
            float32x2 p01 = (curve.p0 + curve.p1) * 0.5f;
            float32x2 p12 = (curve.p1 + curve.p2) * 0.5f;
            float32x2 p012 = (p01 + p12) * 0.5f;

            left = { curve.p0, p01, p012 };
            right = { p012, p12, curve.p2 };
        }

        void makeMonotonic(QuadraticCurve curve, std::vector<QuadraticCurve>& output)
        {
            if (isMonotonic(curve))
            {
                output.push_back(curve);
                return;
            }

            QuadraticCurve left, right;
            subdivideQuadratic(curve, left, right);
            makeMonotonic(left, output);
            makeMonotonic(right, output);
        }

        void appendCurve(Contour& contour, const QuadraticCurve& curve)
        {
            makeMonotonic(curve, contour.curves);
        }

        QuadraticCurve lineToQuadratic(const float32x2& p0, const float32x2& p1)
        {
            return { p0, (p0 + p1) * 0.5f, p1 };
        }

        bool isHorizontalLine(const float32x2& p0, const float32x2& p1)
        {
            return std::fabs(p0.y - p1.y) < kEpsilon && std::fabs(p0.x - p1.x) > kEpsilon;
        }

        void cubicToQuadratics(const float32x2& p0, const float32x2& p1, const float32x2& p2, const float32x2& p3,
                               std::vector<QuadraticCurve>& output, float tolerance = 0.5f, int depth = 0)
        {
            float32x2 mid = evaluateCubic(p0, p1, p2, p3, 0.5f);
            float32x2 chord_mid = (p0 + p3) * 0.5f;
            float error = length(mid - chord_mid);

            if (error <= tolerance || depth >= 6)
            {
                float32x2 control = (4.0f * mid - p0 - p3) * 0.5f;
                output.push_back({ p0, control, p3 });
                return;
            }

            float32x2 p01 = (p0 + p1) * 0.5f;
            float32x2 p12 = (p1 + p2) * 0.5f;
            float32x2 p23 = (p2 + p3) * 0.5f;
            float32x2 p012 = (p01 + p12) * 0.5f;
            float32x2 p123 = (p12 + p23) * 0.5f;
            float32x2 p0123 = (p012 + p123) * 0.5f;

            cubicToQuadratics(p0, p01, p012, p0123, output, tolerance, depth + 1);
            cubicToQuadratics(p0123, p123, p23, p3, output, tolerance, depth + 1);
        }

        void updateBounds(GlyphOutline& outline, const float32x2& point, bool& initialized)
        {
            if (!initialized)
            {
                outline.bounds_min = point;
                outline.bounds_max = point;
                initialized = true;
                return;
            }

            outline.bounds_min = min(outline.bounds_min, point);
            outline.bounds_max = max(outline.bounds_max, point);
        }

    } // namespace

    void packForGpu(const GlyphOutline& outline, GlyphGpuData& gpu)
    {
        gpu.advance = outline.advance;
        gpu.bounds_min = outline.bounds_min;
        gpu.bounds_max = outline.bounds_max;
        gpu.contours.clear();
        gpu.curve_data.clear();
        gpu.contour_bytes.clear();
        gpu.curve_bytes.clear();

        u32 curve_offset = 0;
        for (const Contour& contour : outline.contours)
        {
            GpuContourHeader header;
            header.curve_offset = curve_offset;
            header.curve_count = u32(contour.curves.size());
            gpu.contours.push_back(header);

            for (const QuadraticCurve& curve : contour.curves)
            {
                gpu.curve_data.push_back(curve.p0.x);
                gpu.curve_data.push_back(curve.p0.y);
                gpu.curve_data.push_back(curve.p1.x);
                gpu.curve_data.push_back(curve.p1.y);
                gpu.curve_data.push_back(curve.p2.x);
                gpu.curve_data.push_back(curve.p2.y);
            }

            curve_offset += header.curve_count;
        }

        gpu.contour_bytes.resize(gpu.contours.size() * sizeof(GpuContourHeader));
        if (!gpu.contour_bytes.empty())
        {
            std::memcpy(gpu.contour_bytes.data(), gpu.contours.data(), gpu.contour_bytes.size());
        }

        gpu.curve_bytes.resize(gpu.curve_data.size() * sizeof(float));
        if (!gpu.curve_bytes.empty())
        {
            std::memcpy(gpu.curve_bytes.data(), gpu.curve_data.data(), gpu.curve_bytes.size());
        }
    }

    void processStbShape(const StbVertex* vertices, int count, GlyphOutline& outline)
    {
        Contour contour;
        float32x2 current { 0.0f, 0.0f };
        bool has_current = false;
        bool bounds_initialized = false;

        auto flush_contour = [&]()
        {
            if (!contour.curves.empty())
            {
                outline.contours.push_back(std::move(contour));
                contour = Contour {};
            }
        };

        for (int i = 0; i < count; ++i)
        {
            const StbVertex& v = vertices[i];
            float32x2 next(float(v.x), float(v.y));

            switch (v.type)
            {
                case 1: // STBTT_vmove
                    flush_contour();
                    current = next;
                    has_current = true;
                    updateBounds(outline, current, bounds_initialized);
                    break;

                case 2: // STBTT_vline
                {
                    if (!has_current)
                    {
                        break;
                    }

                    if (!isHorizontalLine(current, next))
                    {
                        appendCurve(contour, lineToQuadratic(current, next));
                    }

                    current = next;
                    updateBounds(outline, current, bounds_initialized);
                    break;
                }

                case 3: // STBTT_vcurve
                {
                    if (!has_current)
                    {
                        break;
                    }

                    float32x2 control(float(v.cx), float(v.cy));
                    appendCurve(contour, { current, control, next });

                    current = next;
                    updateBounds(outline, current, bounds_initialized);
                    updateBounds(outline, control, bounds_initialized);
                    break;
                }

                case 4: // STBTT_vcubic
                {
                    if (!has_current)
                    {
                        break;
                    }

                    float32x2 c1(float(v.cx), float(v.cy));
                    float32x2 c2(float(v.cx1), float(v.cy1));
                    std::vector<QuadraticCurve> quadratics;
                    cubicToQuadratics(current, c1, c2, next, quadratics);
                    for (const QuadraticCurve& q : quadratics)
                    {
                        appendCurve(contour, q);
                    }

                    current = next;
                    updateBounds(outline, current, bounds_initialized);
                    updateBounds(outline, c1, bounds_initialized);
                    updateBounds(outline, c2, bounds_initialized);
                    break;
                }
            }
        }

        flush_contour();
    }

#ifdef MANGO_ENABLE_FREETYPE

    float32x2 ftPoint(const FT_Vector& v)
    {
        // FT_LOAD_NO_SCALE: outline coordinates are integer font units.
        return float32x2(float(v.x), float(v.y));
    }

    struct FtDecomposeContext
    {
        GlyphOutline* outline = nullptr;
        Contour contour {};
        float32x2 current { 0.0f, 0.0f };
        bool has_current = false;
        bool bounds_initialized = false;

        void flush_contour()
        {
            if (!contour.curves.empty())
            {
                outline->contours.push_back(std::move(contour));
                contour = Contour {};
            }
        }
    };

    int ftMoveTo(const FT_Vector* to, void* user)
    {
        auto* ctx = static_cast<FtDecomposeContext*>(user);
        ctx->flush_contour();
        ctx->current = ftPoint(*to);
        ctx->has_current = true;
        updateBounds(*ctx->outline, ctx->current, ctx->bounds_initialized);
        return 0;
    }

    int ftLineTo(const FT_Vector* to, void* user)
    {
        auto* ctx = static_cast<FtDecomposeContext*>(user);
        if (!ctx->has_current)
        {
            return 0;
        }

        float32x2 next = ftPoint(*to);
        if (!isHorizontalLine(ctx->current, next))
        {
            appendCurve(ctx->contour, lineToQuadratic(ctx->current, next));
        }

        ctx->current = next;
        updateBounds(*ctx->outline, ctx->current, ctx->bounds_initialized);
        return 0;
    }

    int ftConicTo(const FT_Vector* control, const FT_Vector* to, void* user)
    {
        auto* ctx = static_cast<FtDecomposeContext*>(user);
        if (!ctx->has_current)
        {
            return 0;
        }

        float32x2 c = ftPoint(*control);
        float32x2 next = ftPoint(*to);
        appendCurve(ctx->contour, { ctx->current, c, next });

        ctx->current = next;
        updateBounds(*ctx->outline, ctx->current, ctx->bounds_initialized);
        updateBounds(*ctx->outline, c, ctx->bounds_initialized);
        return 0;
    }

    int ftCubicTo(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* user)
    {
        auto* ctx = static_cast<FtDecomposeContext*>(user);
        if (!ctx->has_current)
        {
            return 0;
        }

        float32x2 p1 = ftPoint(*c1);
        float32x2 p2 = ftPoint(*c2);
        float32x2 next = ftPoint(*to);

        std::vector<QuadraticCurve> quadratics;
        cubicToQuadratics(ctx->current, p1, p2, next, quadratics);
        for (const QuadraticCurve& q : quadratics)
        {
            appendCurve(ctx->contour, q);
        }

        ctx->current = next;
        updateBounds(*ctx->outline, ctx->current, ctx->bounds_initialized);
        updateBounds(*ctx->outline, p1, ctx->bounds_initialized);
        updateBounds(*ctx->outline, p2, ctx->bounds_initialized);
        return 0;
    }

    void processFreeTypeOutlineImpl(const FT_Outline& ft_outline, GlyphOutline& outline)
    {
        FT_Outline_Funcs funcs {};
        funcs.move_to = ftMoveTo;
        funcs.line_to = ftLineTo;
        funcs.conic_to = ftConicTo;
        funcs.cubic_to = ftCubicTo;
        funcs.shift = 0;
        funcs.delta = 0;

        FtDecomposeContext ctx;
        ctx.outline = &outline;
        FT_Outline_Decompose(const_cast<FT_Outline*>(&ft_outline), &funcs, &ctx);
        ctx.flush_contour();
    }

#endif // MANGO_ENABLE_FREETYPE

#ifdef MANGO_ENABLE_FREETYPE
    void processFreeTypeOutline(const FT_Outline& ft_outline, GlyphOutline& outline)
    {
        processFreeTypeOutlineImpl(ft_outline, outline);
    }
#endif

} // namespace mango::font
