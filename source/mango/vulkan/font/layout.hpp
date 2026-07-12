/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>

#include <mango/math/geometry.hpp>
#include <mango/vulkan/font.hpp>

#include "font_internal.hpp"

namespace mango::font
{

    struct PositionedGlyph
    {
        u32 codepoint = 0;
        float x = 0.0f;
        float y = 0.0f; // baseline
    };

    struct LayoutLine
    {
        std::vector<PositionedGlyph> glyphs;
        float width = 0.0f;
    };

    struct LayoutResult
    {
        std::vector<LayoutLine> lines;
        vulkan::TextMetrics metrics;
    };

    float measureTextWidth(const Face& face, const std::u32string& codepoints);
    LayoutResult layoutParagraph(const Face& face, const math::Rectangle& bounds,
                                 const std::u32string& codepoints,
                                 const vulkan::ParagraphStyle& style);

} // namespace mango::font
