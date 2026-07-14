/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <cmath>

#include "layout.hpp"
#include "shape.hpp"

namespace mango::font
{

    namespace
    {
        bool isWhitespace(char32_t cp)
        {
            return cp == ' ' || cp == '\t' || cp == '\r';
        }

        bool isBreakable(char32_t cp)
        {
            return isWhitespace(cp) || cp >= 0x3000;
        }

        float measureShapedWidth(const ShapedRun& run)
        {
            return run.width;
        }

        ShapedRun shapeLine(const Face& face, const std::u32string& text)
        {
            return shapeText(face, text);
        }

        std::vector<std::u32string> breakLines(const Face& face, const std::u32string& codepoints,
                                                      float max_width, bool word_wrap)
        {
            std::vector<std::u32string> lines;
            std::u32string current_line;

            auto flush = [&]()
            {
                if (!current_line.empty())
                {
                    lines.push_back(current_line);
                    current_line.clear();
                }
            };

            size_t i = 0;
            while (i < codepoints.size())
            {
                char32_t cp = codepoints[i];

                if (cp == U'\n')
                {
                    flush();
                    ++i;
                    continue;
                }

                if (!word_wrap || max_width <= 0.0f)
                {
                    current_line.push_back(cp);
                    ++i;
                    continue;
                }

                std::u32string trial = current_line;
                trial.push_back(cp);

                float width = measureShapedWidth(shapeLine(face, trial));
                if (width <= max_width || trial.size() <= 1)
                {
                    current_line = std::move(trial);
                    ++i;
                    continue;
                }

                if (current_line.empty())
                {
                    current_line.push_back(cp);
                    flush();
                    ++i;
                    continue;
                }

                size_t break_at = current_line.size();
                for (size_t j = current_line.size(); j > 0; --j)
                {
                    if (isBreakable(current_line[j - 1]))
                    {
                        break_at = j;
                        break;
                    }
                }

                if (break_at == current_line.size())
                {
                    break_at = current_line.size() > 1 ? current_line.size() - 1 : 1;
                }

                std::u32string next_line(current_line.begin() + ptrdiff_t(break_at), current_line.end());
                current_line.erase(current_line.begin() + ptrdiff_t(break_at), current_line.end());

                while (!next_line.empty() && isWhitespace(next_line.front()))
                {
                    next_line.erase(next_line.begin());
                }

                flush();
                current_line = std::move(next_line);
            }

            flush();
            return lines;
        }

    } // namespace

    float measureTextWidth(const Face& face, const std::u32string& codepoints)
    {
        return measureShapedWidth(shapeText(face, codepoints));
    }

    LayoutResult layoutParagraph(const Face& face, const math::Rectangle& bounds,
                                 const std::u32string& codepoints,
                                 const vulkan::ParagraphStyle& style)
    {
        LayoutResult result;

        const float line_height = face.lineHeightPixels() * style.lineSpacing;

        auto lines = breakLines(face, codepoints, bounds.size.x, style.wordWrap);

        float max_width = 0.0f;
        float y = bounds.position.y + face.ascenderPixels();

        for (const std::u32string& line_cps : lines)
        {
            if (style.overflow == vulkan::TextOverflow::Clip && y > bounds.position.y + bounds.size.y)
            {
                result.metrics.truncated = true;
                break;
            }

            LayoutLine line;
            float pen_x = bounds.position.x;
            const ShapedRun shaped = shapeLine(face, line_cps);
            line.width = shaped.width;

            switch (style.align)
            {
                case vulkan::TextAlign::Center:
                    pen_x = bounds.position.x + (bounds.size.x - line.width) * 0.5f;
                    break;
                case vulkan::TextAlign::Right:
                    pen_x = bounds.position.x + bounds.size.x - line.width;
                    break;
                case vulkan::TextAlign::Left:
                default:
                    break;
            }

            float cursor_x = pen_x;
            for (const ShapedGlyph& glyph : shaped.glyphs)
            {
                PositionedGlyph pg;
                pg.glyph_index = glyph.glyph_index;
                pg.codepoint = glyph.cluster < line_cps.size() ? u32(line_cps[glyph.cluster]) : 0;
                pg.x = cursor_x + glyph.x_offset;
                pg.y = y + glyph.y_offset;
                line.glyphs.push_back(pg);

                cursor_x += glyph.x_advance;
            }

            max_width = std::max(max_width, line.width);
            result.lines.push_back(std::move(line));
            y += line_height;
        }

        result.metrics.width = max_width;
        result.metrics.lineCount = u32(result.lines.size());
        result.metrics.height = result.lines.empty() ? 0.0f : (y - bounds.position.y - face.ascenderPixels() + line_height);
        return result;
    }

} // namespace mango::font
