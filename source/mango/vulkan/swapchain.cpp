/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <mango/core/configure.hpp>
#include <mango/core/print.hpp>
#include <mango/vulkan/vulkan.hpp>
#include "../window/window_backend.hpp"

namespace mango::vulkan
{

    // ------------------------------------------------------------------------------
    // Swapchain
    // ------------------------------------------------------------------------------

    Swapchain::Swapchain(VkDevice device, VkPhysicalDevice physicalDevice,
                         VkSurfaceKHR surface, VkSurfaceFormatKHR format,
                         VkQueue presentQueue, const Window* window)
        : m_device(device)
        , m_physicalDevice(physicalDevice)
        , m_surface(surface)
        , m_surfaceFormat(format)
        , m_presentQueue(presentQueue)
        , m_window(window)
    {
        updateSwapchain();
    }

    Swapchain::~Swapchain()
    {
        cleanup();
    }

    void Swapchain::destroyImageViews()
    {
        for (auto imageView : m_imageViews)
        {
            vkDestroyImageView(m_device, imageView, nullptr);
        }

        m_imageViews.clear();
        m_images.clear();
    }

    void Swapchain::destroySyncObjects()
    {
        for (auto fence : m_fences)
        {
            vkDestroyFence(m_device, fence, nullptr);
        }

        for (auto semaphore : m_imageAvailableSemaphores)
        {
            vkDestroySemaphore(m_device, semaphore, nullptr);
        }

        for (auto semaphore : m_renderFinishedSemaphores)
        {
            vkDestroySemaphore(m_device, semaphore, nullptr);
        }

        m_fences.clear();
        m_submitFences.clear();
        m_imageAvailableSemaphores.clear();
        m_renderFinishedSemaphores.clear();
        m_imagesInFlight.clear();
    }

    void Swapchain::cleanup()
    {
        vkDeviceWaitIdle(m_device);

        destroySyncObjects();
        destroyImageViews();

        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
            m_swapchain = VK_NULL_HANDLE;
        }
    }

    VkExtent2D Swapchain::resolveExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) const
    {
        VkExtent2D extent = surfaceCapabilities.currentExtent;

        if (extent.width == UINT32_MAX || extent.height == UINT32_MAX || extent.width == 0 || extent.height == 0)
        {
            if (m_window)
            {
                math::int32x2 size = m_window->getWindowSize();
                extent.width = u32(std::max(size.x, 0));
                extent.height = u32(std::max(size.y, 0));
            }
        }

        if (extent.width == 0 || extent.height == 0)
        {
            return { 0, 0 };
        }

        extent.width = std::max(extent.width, surfaceCapabilities.minImageExtent.width);
        extent.width = std::min(extent.width, surfaceCapabilities.maxImageExtent.width);
        extent.height = std::max(extent.height, surfaceCapabilities.minImageExtent.height);
        extent.height = std::min(extent.height, surfaceCapabilities.maxImageExtent.height);

        return extent;
    }

    bool Swapchain::createImageViews()
    {
        u32 swapchainImageCount = 0;
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, nullptr);
        m_images.resize(swapchainImageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, m_images.data());

        m_imageViews.resize(swapchainImageCount);

        for (u32 i = 0; i < swapchainImageCount; ++i)
        {
            VkImageViewCreateInfo viewInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = m_images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = m_surfaceFormat.format,
                .components =
                {
                    VK_COMPONENT_SWIZZLE_IDENTITY,
                    VK_COMPONENT_SWIZZLE_IDENTITY,
                    VK_COMPONENT_SWIZZLE_IDENTITY,
                    VK_COMPONENT_SWIZZLE_IDENTITY
                },
                .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                }
            };

            VkResult result = vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageViews[i]);
            if (result != VK_SUCCESS)
            {
                printLine(Print::Error, "vkCreateImageView: {}", getString(result));
                return false;
            }
        }

        return true;
    }

    VkPresentModeKHR Swapchain::choosePresentMode() const
    {
        // FIFO is always supported and is the guaranteed fallback.
        //
        // On compositor-driven windowing systems (Win32 DWM, macOS CoreAnimation /
        // WindowServer) we deliberately prefer FIFO. With non-blocking MAILBOX the
        // swapchain is recreated on nearly every resize step while a queued present
        // is still in flight; recreate destroys that swapchain before the compositor
        // has shown it, so blank/clear-only frames slip through and the content
        // flickers (and trails the live window geometry). FIFO blocks present until
        // the image is actually displayed, so each rendered frame is shown before the
        // next recreate and resize stays smooth.
        //
        // Everywhere else (notably Wayland) MAILBOX is preferred: vkQueuePresentKHR
        // does not block on vsync, so the event loop keeps processing window
        // configure events immediately and interactive resize tracks in real time
        // while staying tear-free. FIFO is blocking there and causes problems.
#if defined(MANGO_PLATFORM_WINDOWS) || defined(MANGO_PLATFORM_MACOS)
        return VK_PRESENT_MODE_FIFO_KHR;
#else
        u32 count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &count, nullptr);

        std::vector<VkPresentModeKHR> presentModes(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &count, presentModes.data());

        for (VkPresentModeKHR mode : presentModes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return VK_PRESENT_MODE_MAILBOX_KHR;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
#endif
    }

    bool Swapchain::createSwapchain(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, VkExtent2D extent, VkSwapchainKHR oldSwapchain)
    {
        const VkPresentModeKHR presentMode = choosePresentMode();

        u32 imageCount = surfaceCapabilities.minImageCount;

        // MAILBOX's non-blocking vkAcquireNextImageKHR guarantee requires
        // minImageCount + 1 images (one held by the presentation engine, one
        // queued, one for the application to render into).
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            imageCount = surfaceCapabilities.minImageCount + 1;
        }

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            u32 oldImageCount = 0;
            vkGetSwapchainImagesKHR(m_device, oldSwapchain, &oldImageCount, nullptr);
            imageCount = std::max(imageCount, oldImageCount);
        }

        if (surfaceCapabilities.maxImageCount > 0)
        {
            imageCount = std::min(imageCount, surfaceCapabilities.maxImageCount);
        }

        VkSwapchainCreateInfoKHR createInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_surface,
            .minImageCount = imageCount,
            .imageFormat = m_surfaceFormat.format,
            .imageColorSpace = m_surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) |
                (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT),
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .preTransform = surfaceCapabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = oldSwapchain,
        };

        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkResult result = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &swapchain);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkCreateSwapchainKHR: {}", getString(result));
            return false;
        }

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);
        }

        m_swapchain = swapchain;
        m_preTransform = surfaceCapabilities.currentTransform;

        return createImageViews();
    }

    void Swapchain::createSyncObjects()
    {
        const u32 imageCount = u32(m_images.size());

        m_imageAvailableSemaphores.resize(m_maxImagesInFlight);
        m_fences.resize(m_maxImagesInFlight);
        m_submitFences.resize(m_maxImagesInFlight);
        m_renderFinishedSemaphores.resize(imageCount);
        m_imagesInFlight.assign(imageCount, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        VkFenceCreateInfo fenceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        for (u32 i = 0; i < m_maxImagesInFlight; ++i)
        {
            VkResult result;

            result = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
            if (result != VK_SUCCESS)
            {
                printLine(Print::Error, "vkCreateSemaphore: {}", getString(result));
                return;
            }

            result = vkCreateFence(m_device, &fenceInfo, nullptr, &m_fences[i]);
            if (result != VK_SUCCESS)
            {
                printLine(Print::Error, "vkCreateFence: {}", getString(result));
                return;
            }

            m_submitFences[i] = m_fences[i];
        }

        for (u32 i = 0; i < imageCount; ++i)
        {
            VkResult result = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
            if (result != VK_SUCCESS)
            {
                printLine(Print::Error, "vkCreateSemaphore: {}", getString(result));
                return;
            }
        }
    }

    u32 Swapchain::getImageCount() const
    {
        return u32(m_images.size());
    }

    VkSurfaceFormatKHR Swapchain::getSurfaceFormat() const
    {
        return m_surfaceFormat;
    }

    VkFormat Swapchain::getFormat() const
    {
        return m_surfaceFormat.format;
    }

    VkColorSpaceKHR Swapchain::getColorSpace() const
    {
        return m_surfaceFormat.colorSpace;
    }

    std::string Swapchain::getOutputTransformGLSL() const
    {
        return mango::vulkan::getOutputTransformGLSL(m_surfaceFormat);
    }

    std::string Swapchain::getOutputTransformGLSL(const OutputTransformOptions& options) const
    {
        return mango::vulkan::getOutputTransformGLSL(m_surfaceFormat, options);
    }

    std::string Swapchain::getDisplayOutputTransformGLSL() const
    {
        return mango::vulkan::getDisplayOutputTransformGLSL(m_surfaceFormat);
    }

    VkExtent2D Swapchain::getExtent() const
    {
        return m_extent;
    }

    VkImage Swapchain::getImage(u32 imageIndex) const
    {
        return m_images[imageIndex];
    }

    VkImageView Swapchain::getImageView(u32 imageIndex) const
    {
        return m_imageViews[imageIndex];
    }

    void Swapchain::cmdTransitionImageToColorAttachment(VkCommandBuffer commandBuffer, u32 imageIndex) const
    {
        VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_images[imageIndex],
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void Swapchain::cmdTransitionImageToGeneral(VkCommandBuffer commandBuffer, u32 imageIndex) const
    {
        VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_images[imageIndex],
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void Swapchain::cmdTransitionImageToPresent(VkCommandBuffer commandBuffer, u32 imageIndex) const
    {
        VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_images[imageIndex],
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void Swapchain::cmdTransitionImageFromGeneralToPresent(VkCommandBuffer commandBuffer, u32 imageIndex) const
    {
        VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_images[imageIndex],
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    VkImage Swapchain::Frame::image() const
    {
        return m_swapchain ? m_swapchain->getImage(m_imageIndex) : VK_NULL_HANDLE;
    }

    VkImageView Swapchain::Frame::imageView() const
    {
        return m_swapchain ? m_swapchain->getImageView(m_imageIndex) : VK_NULL_HANDLE;
    }

    Swapchain::Frame::Frame(Swapchain* swapchain, u32 imageIndex, VkResult acquireResult)
        : m_swapchain(swapchain)
        , m_imageIndex(imageIndex)
        , m_acquireResult(acquireResult)
    {
    }

    VkResult Swapchain::Frame::submitAndPresent(VkQueue graphicsQueue, VkCommandBuffer commandBuffer)
    {
        return submitAndPresent(graphicsQueue, commandBuffer, VK_NULL_HANDLE, 0);
    }

    VkResult Swapchain::Frame::submit(VkQueue graphicsQueue, VkCommandBuffer commandBuffer, VkFence signalFence)
    {
        return submit(graphicsQueue, commandBuffer, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, signalFence);
    }

    VkResult Swapchain::Frame::present()
    {
        if (!m_swapchain)
        {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        return m_swapchain->present(m_imageIndex);
    }

    VkResult Swapchain::Frame::submit(VkQueue graphicsQueue, VkCommandBuffer commandBuffer,
                                      VkSemaphore signalTimeline, u64 signalValue,
                                      VkSemaphore waitTimeline, u64 waitValue,
                                      VkPipelineStageFlags waitStage,
                                      VkFence signalFence)
    {
        if (!m_swapchain)
        {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        const u32 frame_index = m_swapchain->m_currentFrame;

        VkSemaphore imageAvailableSemaphore = m_swapchain->m_imageAvailableSemaphores[frame_index];
        VkSemaphore renderFinishedSemaphore = m_swapchain->m_renderFinishedSemaphores[m_imageIndex];
        VkFence fence = signalFence != VK_NULL_HANDLE ? signalFence : m_swapchain->m_fences[frame_index];

        // The binary image-available semaphore is always waited; an optional caller
        // timeline wait (e.g. a transfer-queue upload) is appended. Likewise the binary
        // render-finished semaphore is always signalled for present, with an optional
        // caller timeline signal appended for frame-completion tracking. Per-semaphore
        // values are ignored for binary semaphores but the arrays must stay parallel.
        VkSemaphore waitSemaphores [2] = { imageAvailableSemaphore, waitTimeline };
        u64 waitValues [2] = { 0, waitValue };
        VkPipelineStageFlags waitStages [2] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, waitStage };
        const u32 waitCount = (waitTimeline != VK_NULL_HANDLE) ? 2 : 1;

        VkSemaphore signalSemaphores [2] = { renderFinishedSemaphore, signalTimeline };
        u64 signalValues [2] = { 0, signalValue };
        const u32 signalCount = (signalTimeline != VK_NULL_HANDLE) ? 2 : 1;

        const bool useTimeline = (signalTimeline != VK_NULL_HANDLE) || (waitTimeline != VK_NULL_HANDLE);

        VkTimelineSemaphoreSubmitInfo timelineInfo =
        {
            .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
            .waitSemaphoreValueCount = waitCount,
            .pWaitSemaphoreValues = waitValues,
            .signalSemaphoreValueCount = signalCount,
            .pSignalSemaphoreValues = signalValues,
        };

        VkSubmitInfo submitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = useTimeline ? &timelineInfo : nullptr,
            .waitSemaphoreCount = waitCount,
            .pWaitSemaphores = waitSemaphores,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
            .signalSemaphoreCount = signalCount,
            .pSignalSemaphores = signalSemaphores,
        };

        vkResetFences(m_swapchain->m_device, 1, &fence);

        VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkQueueSubmit failed: {}", getString(result));
            return result;
        }

        m_swapchain->m_submitFences[frame_index] = fence;
        m_swapchain->m_imagesInFlight[m_imageIndex] = fence;

        return result;
    }

    VkResult Swapchain::Frame::submitAndPresent(VkQueue graphicsQueue, VkCommandBuffer commandBuffer,
                                                VkSemaphore signalTimeline, u64 signalValue,
                                                VkSemaphore waitTimeline, u64 waitValue,
                                                VkPipelineStageFlags waitStage)
    {
        VkResult result = submit(graphicsQueue, commandBuffer,
            signalTimeline, signalValue, waitTimeline, waitValue, waitStage);
        if (result != VK_SUCCESS)
        {
            return result;
        }

        return present();
    }

    Swapchain::Frame Swapchain::beginFrame()
    {
        // Acquire a correctly-sized image, hiding the recreate/retry dance from callers.
        // updateSwapchain() recreates when the surface extent changed (or a previous
        // frame flagged it). On VK_SUBOPTIMAL_KHR / VK_ERROR_OUT_OF_DATE_KHR the acquired
        // image no longer matches the surface (the window resized mid-acquire), so we
        // recreate and re-acquire rather than hand back a wrong-sized drawable.
        static constexpr int kMaxAttempts = 8;

        for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
        {
            updateSwapchain();

            if (m_extent.width <= 1 || m_extent.height <= 1)
            {
                if (attempt + 1 < kMaxAttempts)
                {
                    continue;
                }
                return Frame();
            }

            u32 imageIndex = 0;
            VkResult result = acquireNextImage(imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                m_recreateRequired = true;
                continue;
            }

            if (result == VK_SUBOPTIMAL_KHR)
            {
                m_recreateRequired = true;
                if (attempt + 1 < kMaxAttempts)
                {
                    continue;
                }
                return Frame(this, imageIndex, result);
            }

            if (result == VK_SUCCESS)
            {
                return Frame(this, imageIndex, result);
            }

            if (attempt + 1 < kMaxAttempts)
            {
                continue;
            }

            return Frame();
        }

        return Frame();
    }

    bool Swapchain::updateSwapchain()
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCapabilities);

        VkExtent2D extent = resolveExtent(surfaceCapabilities);

        bool recreated = false;

        if (extent.width > 1 && extent.height > 1)
        {
            const bool extentChanged = extent.width != m_extent.width || extent.height != m_extent.height;
            const bool transformChanged = m_swapchain != VK_NULL_HANDLE &&
                surfaceCapabilities.currentTransform != m_preTransform;
            const bool needsRecreate = m_swapchain == VK_NULL_HANDLE ||
                m_recreateRequired || extentChanged || transformChanged;

            if (needsRecreate)
            {
                if (!recreateSwapchain(surfaceCapabilities, extent))
                {
                    // Recreation failed; the previous swapchain has been restored.
                    // Leave m_extent unchanged so the next update retries.
                    return false;
                }

                recreated = true;
            }

            m_extent = extent;
        }

        return recreated;
    }

    bool Swapchain::recreateSwapchain(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, VkExtent2D extent)
    {
        if (!m_submitFences.empty())
        {
            vkWaitForFences(m_device, u32(m_submitFences.size()), m_submitFences.data(), VK_TRUE, UINT64_MAX);
        }

        // Fences only retire the graphics-queue submit; vkQueuePresentKHR may still be
        // waiting on the per-image render-finished semaphores on the present queue.
        // Destroying those semaphores here (before the old swapchain is retired) trips
        // VUID-vkDestroySemaphore-semaphore-05149 on the next resize.
        vkDeviceWaitIdle(m_device);

        VkSwapchainKHR oldSwapchain = m_swapchain;

        destroyImageViews();
        destroySyncObjects();

        if (!createSwapchain(surfaceCapabilities, extent, oldSwapchain))
        {
            if (oldSwapchain != VK_NULL_HANDLE)
            {
                m_swapchain = oldSwapchain;
                createImageViews();
                createSyncObjects();
            }
            return false;
        }

        createSyncObjects();
        m_currentFrame = 0;
        m_recreateRequired = false;
        ++m_generation;

        return true;
    }

    VkResult Swapchain::acquireNextImage(u32& imageIndex)
    {
        VkFence fence = m_submitFences[m_currentFrame];

        VkResult waitResult = vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);
        if (waitResult != VK_SUCCESS)
        {
            printLine(Print::Error, "vkWaitForFences failed: {}", getString(waitResult));
            return waitResult;
        }

        VkSemaphore imageAvailableSemaphore = m_imageAvailableSemaphores[m_currentFrame];

        VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return result;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            printLine(Print::Warning, "vkAcquireNextImageKHR failed: {}", getString(result));
            return result;
        }

        if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        {
            waitResult = vkWaitForFences(m_device, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
            if (waitResult != VK_SUCCESS)
            {
                printLine(Print::Error, "vkWaitForFences failed: {}", getString(waitResult));
                return waitResult;
            }
        }

        m_imagesInFlight[imageIndex] = fence;

        return result;
    }

    VkResult Swapchain::present(u32 imageIndex)
    {
        if (m_window)
        {
            if (WindowBackend* backend = m_window->backend())
            {
                backend->beforePresent();
            }
        }

        VkSemaphore renderFinishedSemaphore = m_renderFinishedSemaphores[imageIndex];

        VkPresentInfoKHR presentInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &renderFinishedSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &m_swapchain,
            .pImageIndices = &imageIndex,
        };

        VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            m_recreateRequired = true;
        }
        else if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkQueuePresentKHR failed: {}", getString(result));
        }
        
        m_currentFrame = (m_currentFrame + 1) % m_maxImagesInFlight;

        return result;
    }

} // namespace mango::vulkan
