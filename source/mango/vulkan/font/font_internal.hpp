/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <mango/core/buffer.hpp>
#include <mango/filesystem/file.hpp>
#include <mango/filesystem/path.hpp>
#include <mango/math/math.hpp>

#ifdef MANGO_ENABLE_FREETYPE
struct FT_Outline_;
#endif

namespace mango::font
{

    using math::float32x2;

    struct QuadraticCurve
    {
        float32x2 p0;
        float32x2 p1;
        float32x2 p2;
    };

    struct Contour
    {
        std::vector<QuadraticCurve> curves;
    };

    struct GlyphOutline
    {
        float advance = 0.0f;
        float32x2 bounds_min { 0.0f, 0.0f };
        float32x2 bounds_max { 0.0f, 0.0f };
        std::vector<Contour> contours;
    };

    struct GpuContourHeader
    {
        u32 curve_offset = 0;
        u32 curve_count = 0;
    };

    struct GlyphGpuData
    {
        float advance = 0.0f;
        float32x2 bounds_min { 0.0f, 0.0f };
        float32x2 bounds_max { 0.0f, 0.0f };
        std::vector<GpuContourHeader> contours;
        std::vector<float> curve_data;
        std::vector<u8> contour_bytes;
        std::vector<u8> curve_bytes;
    };

    // CPU typeface: owns TTF bytes, no graphics API dependencies.
    class Face
    {
    public:
        float scale = 1.0f;
        int ascent = 0;
        int descent = 0;
        int line_gap = 0;

        Face();
        ~Face();

        Face(const Face&) = delete;
        Face& operator=(const Face&) = delete;

        bool load(ConstMemory memory);
        bool load(const filesystem::File& file);
        bool load(const std::string& path);
        bool load(const filesystem::Path& path, const std::string& filename);

        void setPixelHeight(float pixel_height);
        float pixelScale() const;
        float emPerPixel() const;

        float advanceWidth(u32 codepoint) const;

        GlyphOutline loadOutline(u32 codepoint) const;
        GlyphGpuData loadGpuGlyph(u32 codepoint) const;
        int kerning(u32 codepoint1, u32 codepoint2) const;

        explicit operator bool () const;

    private:
        Buffer m_data;
        struct FontInfo;
        std::unique_ptr<FontInfo> m_info;
    };

    void packForGpu(const GlyphOutline& outline, GlyphGpuData& gpu);

    struct StbVertex
    {
        s16 x, y, cx, cy;
        s16 cx1, cy1;
        u8 type, padding;
    };

    void processStbShape(const StbVertex* vertices, int count, GlyphOutline& outline);

#ifdef MANGO_ENABLE_FREETYPE
    void processFreeTypeOutline(const FT_Outline_& outline, GlyphOutline& result);
#endif

} // namespace mango::font
