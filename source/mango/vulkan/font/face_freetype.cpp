/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <cmath>
#include <mutex>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <mango/filesystem/filesystem.hpp>

#include "font_internal.hpp"

namespace mango::font
{

    namespace
    {

        FT_Library freetypeLibrary()
        {
            static FT_Library library = nullptr;
            static std::once_flag once;
            std::call_once(once, []()
            {
                if (FT_Init_FreeType(&library) != 0)
                {
                    library = nullptr;
                }
            });
            return library;
        }

        float lineScale(const FT_Face face)
        {
            const float height = float(face->ascender - face->descender);
            return height > 0.0f ? height : float(face->units_per_EM);
        }

        float smoothPixelScale(FT_Face face, float pixel_height)
        {
            const float denom = lineScale(face);
            return denom > 0.0f ? pixel_height / denom : pixel_height;
        }

        float pixelScaleFromSize(FT_Face face)
        {
            if (!face || !face->size)
            {
                return 0.0f;
            }

            // Match FreeType's font-unit → pixel mapping for the active size.
            return float(face->size->metrics.y_scale) / 65536.0f / 64.0f;
        }

        void requestPixelHeight(FT_Face face, float pixel_height)
        {
            FT_Size_RequestRec req {};
            req.type = FT_SIZE_REQUEST_TYPE_NOMINAL;
            req.width = 0;
            req.height = (FT_Long)(std::max(1.0f, pixel_height) * 64.0f);
            req.horiResolution = 0;
            req.vertResolution = 0;
            FT_Request_Size(face, &req);
        }

        void updatePixelScale(FT_Face face, float pixel_height, float& scale_out)
        {
            requestPixelHeight(face, pixel_height);
            scale_out = pixelScaleFromSize(face);
            if (scale_out <= 0.0f)
            {
                scale_out = smoothPixelScale(face, pixel_height);
            }
        }

        float toFontUnits(float value_26_6, float scale)
        {
            return scale > 0.0f ? value_26_6 / (64.0f * scale) : value_26_6 / 64.0f;
        }

        float kerningFontUnits(float delta_x_26_6)
        {
            return delta_x_26_6 / 64.0f;
        }

        constexpr FT_Int32 kNoneOutlineLoadFlags = FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;
        constexpr FT_Int32 kNoneAdvanceLoadFlags = FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE | FT_LOAD_ADVANCE_ONLY;
        constexpr FT_Int32 kLightOutlineLoadFlags = FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_LIGHT;
        constexpr FT_Int32 kLightAdvanceLoadFlags = FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_LIGHT | FT_LOAD_ADVANCE_ONLY;

    } // namespace

    struct Face::FontInfo
    {
        FT_Face face = nullptr;
        float pixel_height = 32.0f;
        Hinting hinting = Hinting::None;
    };

    Face::Face() = default;

    Face::~Face()
    {
        if (m_info && m_info->face)
        {
            FT_Done_Face(m_info->face);
            m_info->face = nullptr;
        }
    }

    bool Face::load(ConstMemory memory)
    {
        m_info.reset();
        m_data.reset();
        m_data.append(memory);
        if (m_data.size() < 4)
        {
            return false;
        }

        FT_Library library = freetypeLibrary();
        if (!library)
        {
            return false;
        }

        m_info = std::make_unique<FontInfo>();
        if (FT_New_Memory_Face(library, m_data.data(), FT_Long(m_data.size()), 0, &m_info->face) != 0)
        {
            m_info.reset();
            return false;
        }

        ascent = m_info->face->ascender;
        descent = m_info->face->descender;
        line_gap = m_info->face->height - (ascent - descent);

        m_info->pixel_height = 32.0f;
        updatePixelScale(m_info->face, m_info->pixel_height, scale);
        return true;
    }

    bool Face::load(const filesystem::File& file)
    {
        if (!file.size())
        {
            return false;
        }

        return load(ConstMemory(file));
    }

    bool Face::load(const std::string& path)
    {
        filesystem::File file(path);
        return load(file);
    }

    bool Face::load(const filesystem::Path& path, const std::string& filename)
    {
        filesystem::File file(path, filename);
        return load(file);
    }

    Face::operator bool () const
    {
        return m_info && m_info->face;
    }

    void Face::setHinting(Hinting hinting)
    {
        if (!m_info)
        {
            return;
        }

        m_info->hinting = hinting;
        setPixelHeight(m_info->pixel_height);
    }

    Hinting Face::hinting() const
    {
        return m_info ? m_info->hinting : Hinting::None;
    }

    void Face::setPixelHeight(float pixel_height)
    {
        if (!m_info || !m_info->face)
        {
            return;
        }

        m_info->pixel_height = std::max(1.0f, pixel_height);
        updatePixelScale(m_info->face, m_info->pixel_height, scale);
    }

    float Face::pixelScale() const
    {
        return scale;
    }

    float Face::emPerPixel() const
    {
        return scale > 0.0f ? 1.0f / scale : 1.0f;
    }

    float Face::kerning(u32 codepoint1, u32 codepoint2) const
    {
        if (!m_info || !m_info->face)
        {
            return 0.0f;
        }

        const FT_UInt index1 = FT_Get_Char_Index(m_info->face, codepoint1);
        const FT_UInt index2 = FT_Get_Char_Index(m_info->face, codepoint2);
        FT_Vector delta {};
        const FT_UInt mode = m_info->hinting == Hinting::Light ? FT_KERNING_DEFAULT : FT_KERNING_UNSCALED;
        if (FT_Get_Kerning(m_info->face, index1, index2, mode, &delta) != 0)
        {
            return 0.0f;
        }

        if (m_info->hinting == Hinting::Light)
        {
            return toFontUnits(float(delta.x), scale);
        }

        return kerningFontUnits(float(delta.x));
    }

    float Face::advanceWidth(u32 codepoint) const
    {
        if (!m_info || !m_info->face)
        {
            return 0.0f;
        }

        const FT_UInt glyph_index = FT_Get_Char_Index(m_info->face, codepoint);
        const FT_Int32 flags = m_info->hinting == Hinting::Light ? kLightAdvanceLoadFlags : kNoneAdvanceLoadFlags;
        if (FT_Load_Glyph(m_info->face, glyph_index, flags) != 0)
        {
            return 0.0f;
        }

        const float advance = float(m_info->face->glyph->metrics.horiAdvance);
        if (m_info->hinting == Hinting::Light)
        {
            return toFontUnits(advance, scale);
        }

        return advance;
    }

    float Face::ascenderPixels() const
    {
        if (m_info && m_info->hinting == Hinting::Light && m_info->face && m_info->face->size)
        {
            return float(m_info->face->size->metrics.ascender) / 64.0f;
        }

        return float(ascent) * scale;
    }

    float Face::descenderPixels() const
    {
        if (m_info && m_info->hinting == Hinting::Light && m_info->face && m_info->face->size)
        {
            return float(m_info->face->size->metrics.descender) / 64.0f;
        }

        return float(descent) * scale;
    }

    float Face::lineHeightPixels() const
    {
        if (m_info && m_info->hinting == Hinting::Light && m_info->face && m_info->face->size)
        {
            return float(m_info->face->size->metrics.height) / 64.0f;
        }

        return float(ascent - descent + line_gap) * scale;
    }

    float Face::advancePixels(u32 codepoint) const
    {
        return advanceWidth(codepoint) * pixelScale();
    }

    float Face::kerningPixels(u32 codepoint1, u32 codepoint2) const
    {
        return kerning(codepoint1, codepoint2) * pixelScale();
    }

    GlyphOutline Face::loadOutline(u32 codepoint) const
    {
        GlyphOutline outline;

        if (!m_info || !m_info->face)
        {
            return outline;
        }

        const FT_UInt glyph_index = FT_Get_Char_Index(m_info->face, codepoint);
        const FT_Int32 flags = m_info->hinting == Hinting::Light ? kLightOutlineLoadFlags : kNoneOutlineLoadFlags;
        if (FT_Load_Glyph(m_info->face, glyph_index, flags) != 0)
        {
            return outline;
        }

        const float advance = float(m_info->face->glyph->metrics.horiAdvance);
        if (m_info->hinting == Hinting::Light)
        {
            outline.advance = toFontUnits(advance, scale);

            if (m_info->face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
            {
                processFreeTypeOutline(m_info->face->glyph->outline, outline, 1.0f / (64.0f * scale));
            }
        }
        else
        {
            outline.advance = advance;

            if (m_info->face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
            {
                processFreeTypeOutline(m_info->face->glyph->outline, outline, 1.0f);
            }
        }

        return outline;
    }

    GlyphGpuData Face::loadGpuGlyph(u32 codepoint) const
    {
        GlyphOutline outline = loadOutline(codepoint);
        GlyphGpuData gpu;
        packForGpu(outline, gpu);
        return gpu;
    }

} // namespace mango::font
