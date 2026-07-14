/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "shape.hpp"

#ifdef MANGO_HAS_HARFBUZZ
#include <hb.h>
#include <hb-ft.h>
#endif

namespace mango::font
{

    bool textShapingEnabled()
    {
#ifdef MANGO_HAS_HARFBUZZ
        return true;
#else
        return false;
#endif
    }

    namespace
    {

        ShapedRun shapeTextFallback(const Face& face, std::u32string_view text)
        {
            ShapedRun run;
            if (text.empty())
            {
                return run;
            }

            run.glyphs.reserve(text.size());
            float pen_x = 0.0f;

            for (size_t i = 0; i < text.size(); ++i)
            {
                const u32 cp = u32(text[i]);
                const u32 next = (i + 1 < text.size()) ? u32(text[i + 1]) : 0;

                ShapedGlyph glyph;
                glyph.glyph_index = face.glyphIndex(cp);
                glyph.cluster = u32(i);
                glyph.x_advance = face.advancePixels(cp);
                if (next)
                {
                    glyph.x_advance += face.kerningPixels(cp, next);
                }

                run.glyphs.push_back(glyph);
                pen_x += glyph.x_advance;
            }

            run.width = pen_x;
            return run;
        }

#ifdef MANGO_HAS_HARFBUZZ

        ShapedRun shapeTextHarfbuzz(Face& face, std::u32string_view text)
        {
            ShapedRun run;
            if (text.empty())
            {
                return run;
            }

            face.syncHarfbuzzFont();
            hb_font_t* hb_font = face.harfbuzzFont();
            if (!hb_font)
            {
                return shapeTextFallback(face, text);
            }

            hb_buffer_t* buffer = hb_buffer_create();
            hb_buffer_add_utf32(buffer, reinterpret_cast<const uint32_t*>(text.data()),
                int(text.size()), 0, int(text.size()));
            hb_buffer_guess_segment_properties(buffer);

            const hb_feature_t features[] =
            {
                { HB_TAG('l', 'i', 'g', 'a'), 1, 0, HB_FEATURE_GLOBAL_END },
                { HB_TAG('c', 'l', 'i', 'g'), 1, 0, HB_FEATURE_GLOBAL_END },
                { HB_TAG('k', 'e', 'r', 'n'), 1, 0, HB_FEATURE_GLOBAL_END },
            };

            hb_shape(hb_font, buffer, features, std::size(features));

            const unsigned count = hb_buffer_get_length(buffer);
            const hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, nullptr);
            const hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer, nullptr);

            run.glyphs.reserve(count);
            float pen_x = 0.0f;

            for (unsigned i = 0; i < count; ++i)
            {
                ShapedGlyph glyph;
                glyph.glyph_index = u32(info[i].codepoint);
                glyph.cluster = info[i].cluster;
                glyph.x_offset = float(pos[i].x_offset) / 64.0f;
                glyph.y_offset = float(pos[i].y_offset) / 64.0f;
                glyph.x_advance = float(pos[i].x_advance) / 64.0f;
                glyph.y_advance = float(pos[i].y_advance) / 64.0f;

                run.glyphs.push_back(glyph);
                pen_x += glyph.x_advance;
            }

            hb_buffer_destroy(buffer);

            run.width = pen_x;
            return run;
        }

#endif

    } // namespace

    ShapedRun shapeText(const Face& face, std::u32string_view text)
    {
        if (!face)
        {
            return {};
        }

#ifdef MANGO_HAS_HARFBUZZ
        return shapeTextHarfbuzz(const_cast<Face&>(face), text);
#else
        return shapeTextFallback(face, text);
#endif
    }

} // namespace mango::font
