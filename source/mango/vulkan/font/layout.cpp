/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <cmath>

#include "layout.hpp"

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

        float glyphAdvance(const Face& face, u32 codepoint, u32 next_codepoint)
        {
            float advance = face.advanceWidth(codepoint);
            if (next_codepoint)
            {
                advance += float(face.kerning(codepoint, next_codepoint));
            }
            return advance * face.pixelScale();
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

                float width = measureTextWidth(face, trial);
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
        float width = 0.0f;
        const float scale = face.pixelScale();

        for (size_t i = 0; i < codepoints.size(); ++i)
        {
            u32 cp = u32(codepoints[i]);
            u32 next = (i + 1 < codepoints.size()) ? u32(codepoints[i + 1]) : 0;

            width += face.advanceWidth(cp) * scale;
            if (next)
            {
                width += float(face.kerning(cp, next)) * scale;
            }
        }

        return width;
    }

    LayoutResult layoutParagraph(const Face& face, const math::Rectangle& bounds,
                                 const std::u32string& codepoints,
                                 const vulkan::ParagraphStyle& style)
    {
        LayoutResult result;

        const float scale = face.pixelScale();
        const float line_height = float(face.ascent - face.descent + face.line_gap) * scale * style.lineSpacing;

        auto lines = breakLines(face, codepoints, bounds.size.x, style.wordWrap);

        float max_width = 0.0f;
        float y = bounds.position.y + float(face.ascent) * scale;

        for (const std::u32string& line_cps : lines)
        {
            if (style.overflow == vulkan::TextOverflow::Clip && y > bounds.position.y + bounds.size.y)
            {
                result.metrics.truncated = true;
                break;
            }

            LayoutLine line;
            float pen_x = bounds.position.x;
            line.width = measureTextWidth(face, line_cps);

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

            for (size_t i = 0; i < line_cps.size(); ++i)
            {
                u32 cp = u32(line_cps[i]);
                u32 next = (i + 1 < line_cps.size()) ? u32(line_cps[i + 1]) : 0;

                PositionedGlyph pg;
                pg.codepoint = cp;
                pg.x = pen_x;
                pg.y = y;
                line.glyphs.push_back(pg);

                pen_x += face.advanceWidth(cp) * scale;
                if (next)
                {
                    pen_x += float(face.kerning(cp, next)) * scale;
                }
            }

            max_width = std::max(max_width, line.width);
            result.lines.push_back(std::move(line));
            y += line_height;
        }

        result.metrics.width = max_width;
        result.metrics.lineCount = u32(result.lines.size());
        result.metrics.height = result.lines.empty() ? 0.0f : (y - bounds.position.y - float(face.ascent) * scale + line_height);
        return result;
    }

} // namespace mango::font
