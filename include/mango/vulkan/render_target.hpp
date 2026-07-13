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

    // GPU color image sized 1:1 with the swapchain. Cleared and rendered by the
    // application (e.g. FontRenderer::encode), then resolved to the swapchain via
    // resolve(). Resize explicitly when the window/swapchain extent changes.
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

        explicit RenderTarget(const CreateInfo& info);
        ~RenderTarget();

        void resize(VkExtent2D extent);

        operator bool () const noexcept;

        RenderTargetFormat targetFormat() const;
        VkFormat format() const;
        VkExtent2D extent() const;
        VkImage image() const;
        VkImageView view() const;

        // Leaves the image in GENERAL layout (storage / transfer / compute ready).
        void clear(VkCommandBuffer commandBuffer, math::float32x4 color);

        // 1:1 texel copy to the swapchain image; handles swapchain layout transitions.
        // Float16: color nullptr uses defaultOutputOptions(swapchain surface format).
        void resolve(VkCommandBuffer commandBuffer, Swapchain& swapchain, u32 imageIndex,
                     const OutputTransformOptions* color = nullptr);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace mango::vulkan
