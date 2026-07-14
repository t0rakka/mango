/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <mango/filesystem/filesystem.hpp>

#include "font_internal.hpp"

namespace mango::font
{

    struct Face::FontInfo
    {
        stbtt_fontinfo info {};
    };

    static_assert(sizeof(StbVertex) == sizeof(stbtt_vertex));

    Face::Face() = default;
    Face::~Face() = default;

    bool Face::load(ConstMemory memory)
    {
        m_info.reset();
        m_data.reset();
        m_data.append(memory);
        if (m_data.size() < 4)
        {
            return false;
        }

        m_info = std::make_unique<FontInfo>();
        if (!stbtt_InitFont(&m_info->info, m_data.data(), stbtt_GetFontOffsetForIndex(m_data.data(), 0)))
        {
            m_info.reset();
            return false;
        }

        stbtt_GetFontVMetrics(&m_info->info, &ascent, &descent, &line_gap);
        scale = stbtt_ScaleForPixelHeight(&m_info->info, 32.0f);
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
        return m_info != nullptr;
    }

    void Face::setPixelHeight(float pixel_height)
    {
        if (!m_info)
        {
            return;
        }

        scale = stbtt_ScaleForPixelHeight(&m_info->info, pixel_height);
    }

    void Face::setHinting(Hinting)
    {
    }

    Hinting Face::hinting() const
    {
        return Hinting::None;
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
        if (!m_info)
        {
            return 0.0f;
        }

        return float(stbtt_GetCodepointKernAdvance(&m_info->info, int(codepoint1), int(codepoint2)));
    }

    float Face::advanceWidth(u32 codepoint) const
    {
        if (!m_info)
        {
            return 0.0f;
        }

        int advance = 0;
        stbtt_GetCodepointHMetrics(&m_info->info, int(codepoint), &advance, nullptr);
        return float(advance);
    }

    float Face::ascenderPixels() const
    {
        return float(ascent) * scale;
    }

    float Face::descenderPixels() const
    {
        return float(descent) * scale;
    }

    float Face::lineHeightPixels() const
    {
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

    u32 Face::glyphIndex(u32 codepoint) const
    {
        if (!m_info)
        {
            return 0;
        }

        return u32(stbtt_FindGlyphIndex(&m_info->info, int(codepoint)));
    }

    GlyphOutline Face::loadOutlineByIndex(u32 glyph_index) const
    {
        GlyphOutline outline;

        if (!m_info)
        {
            return outline;
        }

        int advance = 0;
        stbtt_GetGlyphHMetrics(&m_info->info, int(glyph_index), &advance, nullptr);
        outline.advance = float(advance);

        stbtt_vertex* vertices = nullptr;
        int count = stbtt_GetGlyphShape(&m_info->info, int(glyph_index), &vertices);
        if (count > 0 && vertices)
        {
            processStbShape(reinterpret_cast<const StbVertex*>(vertices), count, outline);
            stbtt_FreeShape(&m_info->info, vertices);
        }

        return outline;
    }

    GlyphOutline Face::loadOutline(u32 codepoint) const
    {
        return loadOutlineByIndex(glyphIndex(codepoint));
    }

    GlyphGpuData Face::loadGpuGlyphByIndex(u32 glyph_index) const
    {
        GlyphOutline outline = loadOutlineByIndex(glyph_index);
        GlyphGpuData gpu;
        packForGpu(outline, gpu);
        return gpu;
    }

    GlyphGpuData Face::loadGpuGlyph(u32 codepoint) const
    {
        return loadGpuGlyphByIndex(glyphIndex(codepoint));
    }

} // namespace mango::font
