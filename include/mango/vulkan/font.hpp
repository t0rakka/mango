/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.

    Scanline Sweeper vector font rendering (Vulkan).
    https://rookandpossum.com/papers/scanline_sweeper_preprint.pdf

    FontRenderer is not thread-safe: load(), unload(), setSize(), and warmup() must not run
    concurrently with clear(), draw(), drawParagraph(), or resolve() on the same FontRenderer
    instance. Use the thread that records the VkCommandBuffers passed to the draw/resolve methods.
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

    // Handle to a loaded typeface. Invalid when default-constructed or when load() fails.
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

    // How resolve() composites the internal canvas onto the target image.
    enum class ResolveMode
    {
        // Alpha-blend text onto existing target contents (LOAD + blend). Clear the canvas
        // transparent before drawing. Typical use: HUD/text over a rendered scene.
        Overlay,

        // Copy the full canvas over the target (DONT_CARE + replace). Clear the canvas
        // with your background color before drawing. Typical use: text-only applications.
        Replace,
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
        float lineHeight(Font font) const;
        float ascender(Font font) const;
        float descender(Font font) const;

        void warmup(Font font, std::string_view utf8);

        TextMetrics measure(Font font, std::string_view utf8) const;
        TextMetrics measureParagraph(Font font, const math::Rectangle& bounds,
                                     std::string_view utf8,
                                     const ParagraphStyle& style = {}) const;
        float textWidth(Font font, std::string_view utf8) const;

        void resize(VkExtent2D extent);

        void clear(VkCommandBuffer cmd, math::float32x4 color = math::float32x4(0.0f, 0.0f, 0.0f, 0.0f));

        void draw(VkCommandBuffer cmd, Font font, float x, float y,
                  std::string_view utf8, const TextStyle& style = {});

        void drawParagraph(VkCommandBuffer cmd, Font font, const math::Rectangle& bounds,
                           std::string_view utf8, const ParagraphStyle& style = {});

        void resolve(VkCommandBuffer cmd, VkImageView target, VkExtent2D extent,
                     ResolveMode mode = ResolveMode::Overlay);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace mango::vulkan
