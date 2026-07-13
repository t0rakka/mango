/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <memory>
#include <vulkan/vulkan.h>

#include <mango/core/configure.hpp>
#include <mango/math/vector.hpp>
#include <mango/vulkan/allocator.hpp>
#include <mango/vulkan/colorspace.hpp>

namespace mango::vulkan
{

    class Swapchain;

    enum class RenderTargetFormat
    {
        UNORM8,   // R8G8B8A8_UNORM  — display-referred SDR
        SRGB8,    // R8G8B8A8_SRGB   — sRGB-encoded storage
        Float16,  // R16G16B16A16_SFLOAT — scene-linear HDR
    };

    // Swapchain-sized color image. The application fills it each frame, then calls
    // resolve() to copy/encode into the acquired swapchain image.
    //
    // Two supported write paths — pick one per frame; do not mix layout helpers from
    // both paths on the same image in one command buffer:
    //
    //   (A) General / compute / clear — image stays in GENERAL until resolve()
    //       clear(cmd, color);
    //       // compute or transfer writes using view() in GENERAL layout
    //       resolve(cmd, swapchain, imageIndex);
    //
    //   (B) Graphics color attachment — dynamic rendering into view()
    //       beginColorAttachment(cmd);
    //       vkCmdBeginRendering(cmd, ... view(), COLOR_ATTACHMENT_OPTIMAL ...);
    //       // draw
    //       vkCmdEndRendering(cmd);
    //       endColorAttachment(cmd);
    //       resolve(cmd, swapchain, imageIndex);
    //
    // Path (B) requires beginColorAttachment/endColorAttachment (or the equivalent
    // beginRendering/endRendering) so internal layout tracking stays in sync with
    // resolve(). Never issue your own layout barriers on image(); use these hooks.
    //
    // For a single graphics pass in one function, ColorAttachmentScope (below) wraps
    // the layout transitions; you still record vkCmdBeginRendering/EndRendering inside.
    //
    // Resize when the swapchain extent changes (see examples/vulkan/font.cpp).
    class RenderTarget : public NonCopyable
    {
    public:
        struct CreateInfo
        {
            VkDevice device = VK_NULL_HANDLE;
            Allocator* allocator = nullptr;
            VkQueue queue = VK_NULL_HANDLE;
            u32 queueFamily = 0;
            RenderTargetFormat format = RenderTargetFormat::UNORM8;
            VkExtent2D extent {};
        };

        // RAII layout transitions for path (B). Created at pass start (before
        // vkCmdBeginRendering); destroyed after vkCmdEndRendering.
        class ColorAttachmentScope : public NonCopyable
        {
        public:
            ColorAttachmentScope(RenderTarget& target, VkCommandBuffer commandBuffer);
            ~ColorAttachmentScope();

            ColorAttachmentScope(ColorAttachmentScope&& other) noexcept;
            ColorAttachmentScope& operator = (ColorAttachmentScope&& other) = delete;

        private:
            RenderTarget* m_target = nullptr;
            VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
        };

        explicit RenderTarget(const CreateInfo& info);
        ~RenderTarget();

        void resize(VkExtent2D extent);

        operator bool () const noexcept;

        RenderTargetFormat targetFormat() const;
        VkFormat format() const;
        VkExtent2D extent() const;
        VkImage image() const;
        VkImageView view() const;

        // Path (A): clear in GENERAL layout (storage / transfer / compute ready).
        void clear(VkCommandBuffer commandBuffer, math::float32x4 color);

        // Path (B): layout transitions around an external vkCmdBeginRendering pass.
        void beginColorAttachment(VkCommandBuffer commandBuffer);
        void endColorAttachment(VkCommandBuffer commandBuffer);

        ColorAttachmentScope beginColorAttachmentScope(VkCommandBuffer commandBuffer);

        // Synonyms for beginColorAttachment / endColorAttachment.
        void beginRendering(VkCommandBuffer commandBuffer) { beginColorAttachment(commandBuffer); }
        void endRendering(VkCommandBuffer commandBuffer) { endColorAttachment(commandBuffer); }

        // 1:1 resolve to the swapchain image; handles swapchain layout transitions.
        // Float16: color nullptr uses defaultOutputOptions(swapchain surface format).
        void resolve(VkCommandBuffer commandBuffer, Swapchain& swapchain, u32 imageIndex,
                     const OutputTransformOptions* color = nullptr);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace mango::vulkan
