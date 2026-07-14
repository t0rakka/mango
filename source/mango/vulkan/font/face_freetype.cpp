/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mutex>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H

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

    } // namespace

    struct Face::FontInfo
    {
        FT_Face face = nullptr;
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
        scale = 32.0f / lineScale(m_info->face);
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

    void Face::setPixelHeight(float pixel_height)
    {
        if (!m_info || !m_info->face)
        {
            return;
        }

        scale = pixel_height / lineScale(m_info->face);
    }

    float Face::pixelScale() const
    {
        return scale;
    }

    float Face::emPerPixel() const
    {
        return scale > 0.0f ? 1.0f / scale : 1.0f;
    }

    int Face::kerning(u32 codepoint1, u32 codepoint2) const
    {
        if (!m_info || !m_info->face)
        {
            return 0;
        }

        const FT_UInt index1 = FT_Get_Char_Index(m_info->face, codepoint1);
        const FT_UInt index2 = FT_Get_Char_Index(m_info->face, codepoint2);
        FT_Vector delta {};
        if (FT_Get_Kerning(m_info->face, index1, index2, FT_KERNING_UNSCALED, &delta) != 0)
        {
            return 0;
        }

        return int(delta.x);
    }

    float Face::advanceWidth(u32 codepoint) const
    {
        if (!m_info || !m_info->face)
        {
            return 0.0f;
        }

        const FT_UInt glyph_index = FT_Get_Char_Index(m_info->face, codepoint);
        FT_Fixed advance = 0;
        if (FT_Get_Advance(m_info->face, glyph_index, FT_LOAD_NO_SCALE, &advance) != 0)
        {
            return 0.0f;
        }

        return float(advance) / 64.0f;
    }

    GlyphOutline Face::loadOutline(u32 codepoint) const
    {
        GlyphOutline outline;

        if (!m_info || !m_info->face)
        {
            return outline;
        }

        const FT_UInt glyph_index = FT_Get_Char_Index(m_info->face, codepoint);
        if (FT_Load_Glyph(m_info->face, glyph_index, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE) != 0)
        {
            return outline;
        }

        outline.advance = float(m_info->face->glyph->metrics.horiAdvance);

        if (m_info->face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
        {
            processFreeTypeOutline(m_info->face->glyph->outline, outline);
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
