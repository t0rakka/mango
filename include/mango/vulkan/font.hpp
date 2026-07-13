/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.

    Scanline Sweeper vector font rendering (Vulkan).
    https://rookandpossum.com/papers/scanline_sweeper_preprint.pdf

    FontRenderer is not thread-safe.

    Typical frame:
        renderer.beginFrame();
        auto cursor = renderer.cursorTopLeft(font, 8, 32, style);
        renderer.drawLine(cursor, font, "Hello", style);
        // Application clears/renders its framebuffer first, then:
        renderer.encode(cmd, { .image = targetImage, .imageView = targetView, .extent = extent });
*/
#pragma once

#include <string>
#include <string_view>

#include <vulkan/vulkan.h>

#include <mango/core/configure.hpp>
#include <mango/core/memory.hpp>
#include <mango/math/math.hpp>
#include <mango/filesystem/file.hpp>
#include <mango/filesystem/path.hpp>
#include <mango/math/geometry.hpp>
#include <mango/math/vector.hpp>
#include <mango/vulkan/allocator.hpp>

namespace mango::vulkan
{

    struct Font
    {
        u32 index = ~0u;
        u32 generation = 0;

        operator bool () const noexcept
        {
            return index != ~0u;
        }

        friend bool operator == (const Font& a, const Font& b) noexcept
        {
            return a.index == b.index && a.generation == b.generation;
        }

        friend bool operator != (const Font& a, const Font& b) noexcept
        {
            return !(a == b);
        }
    };

    struct TextStyle
    {
        math::float32x4 color { 1.0f, 1.0f, 1.0f, 1.0f };
        float lineSpacing = 1.0f;
        // 0 = use the size from setSize(); otherwise rasterize at this pixel height.
        float pixelHeight = 0.0f;
        // Boost sub-pixel stem coverage at small sizes. 0 = off, 1 = default.
        float stemDarkening = 1.0f;
    };

    enum class TextAlign
    {
        Left,
        Center,
        Right,
    };

    enum class TextOverflow
    {
        Clip,
        Ellipsis,
    };

    struct ParagraphStyle : TextStyle
    {
        TextAlign align = TextAlign::Left;
        bool wordWrap = true;
        TextOverflow overflow = TextOverflow::Clip;
    };

    struct TextMetrics
    {
        float width = 0.0f;
        float height = 0.0f;
        u32 lineCount = 0;
        bool truncated = false;
    };

    // Baseline position for sequential text layout.
    struct TextCursor
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct EncodeTarget
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkExtent2D extent {};
    };

    class FontRenderer : public NonCopyable
    {
    public:
        struct CreateInfo
        {
            VkDevice device = VK_NULL_HANDLE;
            VkQueue queue = VK_NULL_HANDLE;
            u32 queueFamily = 0;
            Allocator* allocator = nullptr;
            VkFormat targetFormat = VK_FORMAT_B8G8R8A8_UNORM;
            // Ring of GPU batch buffers; must be >= concurrent in-flight encode() calls.
            u32 maxFramesInFlight = 3;
        };

        explicit FontRenderer(const CreateInfo& info);
        ~FontRenderer();

        Font load(const std::string& path);
        Font load(ConstMemory memory);
        Font load(const filesystem::File& file);
        Font load(const filesystem::Path& path, const std::string& filename);

        void unload(Font font);

        void setSize(Font font, float pixelHeight);
        float size(Font font) const;

        void warmup(Font font, std::string_view utf8);

        TextMetrics measure(Font font, std::string_view utf8, const TextStyle& style = {}) const;
        TextMetrics measureParagraph(Font font, const math::Rectangle& bounds,
                                     std::string_view utf8,
                                     const ParagraphStyle& style = {}) const;
        float textWidth(Font font, std::string_view utf8, const TextStyle& style = {}) const;
        float lineHeight(Font font, const TextStyle& style = {}) const;
        float ascender(Font font, const TextStyle& style = {}) const;
        float descender(Font font, const TextStyle& style = {}) const;

        void resize(VkExtent2D extent);

        // Phase A: queue text for this frame (CPU only, no VkCommandBuffer).
        void beginFrame();

        TextCursor cursor(Font font, float x, float baseline_y) const;
        TextCursor cursorTopLeft(Font font, float x, float top, const TextStyle& style = {}) const;

        void draw(Font font, float x, float y, std::string_view utf8, const TextStyle& style = {});
        void draw(TextCursor& cursor, Font font, std::string_view utf8, const TextStyle& style = {});
        void drawLine(TextCursor& cursor, Font font, std::string_view utf8, const TextStyle& style = {});
        void newline(TextCursor& cursor, Font font, const TextStyle& style = {});

        void drawParagraph(Font font, const math::Rectangle& bounds,
                           std::string_view utf8, const ParagraphStyle& style = {});
        void drawParagraph(TextCursor& cursor, Font font, float width,
                           std::string_view utf8, const ParagraphStyle& style = {});

        // Phase B: composite queued text into target.
        // Target image must be in VK_IMAGE_LAYOUT_GENERAL with storage access.
        void encode(VkCommandBuffer cmd, const EncodeTarget& target);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace mango::vulkan
