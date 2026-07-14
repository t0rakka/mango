/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string_view>
#include <vector>

#include <mango/core/configure.hpp>

#include "font_internal.hpp"

namespace mango::font
{

    struct ShapedGlyph
    {
        u32 glyph_index = 0;
        u32 cluster = 0;
        float x_offset = 0.0f;
        float y_offset = 0.0f;
        float x_advance = 0.0f;
        float y_advance = 0.0f;
    };

    struct ShapedRun
    {
        std::vector<ShapedGlyph> glyphs;
        float width = 0.0f;
    };

    bool textShapingEnabled();

    ShapedRun shapeText(const Face& face, std::u32string_view text);

} // namespace mango::font
