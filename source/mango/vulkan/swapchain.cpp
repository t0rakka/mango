/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <mango/core/configure.hpp>
#include <mango/core/print.hpp>
#include <mango/vulkan/vulkan.hpp>

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
        , m_format(format.format)
        , m_colorSpace(format.colorSpace)
        , m_presentQueue(presentQueue)
        , m_window(window)
    {
        recreateSwapchain();
    }

    Swapchain::~Swapchain()
    {
        cleanup();
    }

    void Swapchain::cleanup()
    {
        vkDeviceWaitIdle(m_device);

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
        m_imageAvailableSemaphores.clear();
        m_renderFinishedSemaphores.clear();
        m_imagesInFlight.clear();

        for (auto imageView : m_imageViews)
        {
            vkDestroyImageView(m_device, imageView, nullptr);
        }

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

    void Swapchain::createSwapchain(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, VkExtent2D extent)
    {
        // create swapchain
        VkSwapchainCreateInfoKHR createInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_surface,
            .minImageCount = surfaceCapabilities.minImageCount,
            .imageFormat = m_format,
            .imageColorSpace = m_colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        };

        VkResult result = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkCreateSwapchainKHR: {}", getString(result));
            return;
        }

        // get swapchain images
        u32 imageCount = 0;
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
        m_images.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_images.data());

        // create image views
        m_imageViews.resize(imageCount);

        for (u32 i = 0; i < imageCount; ++i)
        {
            VkImageViewCreateInfo viewInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = m_images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = m_format,
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

            result = vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageViews[i]);
            if (result != VK_SUCCESS)
            {
                printLine(Print::Error, "vkCreateImageView: {}", getString(result));
                return;
            }
        }
    }

    void Swapchain::createSyncObjects()
    {
        const u32 imageCount = u32(m_images.size());

        m_imageAvailableSemaphores.resize(m_maxImagesInFlight);
        m_fences.resize(m_maxImagesInFlight);
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

    VkFormat Swapchain::getFormat() const
    {
        return m_format;
    }

    VkColorSpaceKHR Swapchain::getColorSpace() const
    {
        return m_colorSpace;
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
        if (!m_swapchain)
        {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        VkSemaphore imageAvailableSemaphore = m_swapchain->m_imageAvailableSemaphores[m_swapchain->m_currentFrame];
        VkSemaphore renderFinishedSemaphore = m_swapchain->m_renderFinishedSemaphores[m_imageIndex];
        VkFence fence = m_swapchain->m_fences[m_swapchain->m_currentFrame];

        VkPipelineStageFlags waitStages [] =
        {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };

        VkSubmitInfo submitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &imageAvailableSemaphore,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &renderFinishedSemaphore,
        };

        vkResetFences(m_swapchain->m_device, 1, &fence);

        VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkQueueSubmit failed: {}", getString(result));
            return result;
        }

        return m_swapchain->present(m_imageIndex);
    }

    Swapchain::Frame Swapchain::beginFrame()
    {
        if (m_extent.width <= 1 || m_extent.height <= 1)
        {
            return Frame();
        }

        u32 imageIndex = 0;
        VkResult result = acquireNextImage(imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_recreateRequired = true;
            return Frame();
        }

        if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
        {
            if (result == VK_SUBOPTIMAL_KHR)
            {
                m_recreateRequired = true;
            }
            return Frame(this, imageIndex, result);
        }

        return Frame();
    }

    bool Swapchain::recreateSwapchain()
    {
        bool recreate = false;

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCapabilities);

        VkExtent2D extent = resolveExtent(surfaceCapabilities);

        if (extent.width > 0 && extent.height > 0)
        {
            bool extentChanged = extent.width != m_extent.width || extent.height != m_extent.height;
            if (m_recreateRequired || extentChanged)
            {
                cleanup();
                createSwapchain(surfaceCapabilities, extent);
                createSyncObjects();
                m_currentFrame = 0;
                m_recreateRequired = false;
                recreate = true;
            }
        }

        m_extent = extent;

        return recreate;
    }

    VkResult Swapchain::acquireNextImage(u32& imageIndex)
    {
        VkFence fence = m_fences[m_currentFrame];

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
