/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/configure.hpp>
#include <mango/core/print.hpp>
#include <mango/vulkan/vulkan.hpp>

#if 0

namespace mango::vulkan
{

    // ------------------------------------------------------------------------------
    // Swapchain
    // ------------------------------------------------------------------------------

    Swapchain::Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkQueue graphicsQueue, VkQueue presentQueue)
        : m_device(device)
        , m_physicalDevice(physicalDevice)
        , m_surface(surface)
        , m_graphicsQueue(graphicsQueue)
        , m_presentQueue(presentQueue)
    {
        configure();
        createSwapchain();
        createSyncObjects();
    }

    Swapchain::~Swapchain()
    {
        cleanup();
    }

    void Swapchain::configure()
    {
        //VkBool32 supported = VK_FALSE;
        //vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsQueueFamilyIndex, m_surface, &supported);
        //printLine("vkGetPhysicalDeviceSurfaceSupportKHR: {}", supported);

        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);
        printLine("PhysicalDeviceSurface.Extent: {} x {}", caps.currentExtent.width, caps.currentExtent.height);

        m_extent = caps.currentExtent;

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);

        if (formatCount == 0)
        {
            return;
        }

        printLine("PhysicalDeviceSurfaceFormats:");

        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

        size_t selectedFormatIndex = 0;
        for (size_t i = 0; i < formats.size(); ++i)
        {
            if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                selectedFormatIndex = i;
                break;
            }
        }

        for (size_t i = 0; i < formats.size(); ++i)
        {
            std::string_view prefix = i == selectedFormatIndex ? ">" : " ";
            printLine("  {} {} | {}", prefix, getString(formats[i].format), getString(formats[i].colorSpace));
        }

        m_format = formats[selectedFormatIndex].format;
    }

    void Swapchain::cleanup()
    {
        // Wait for device to be idle to ensure all operations are complete
        vkDeviceWaitIdle(m_device);

        // Wait for fences that are currently in use
        for (u32 i = 0; i < m_maxImagesInFlight; ++i)
        {
            if (m_framesInFlight[i])
            {
                vkWaitForFences(m_device, 1, &m_fences[i], VK_TRUE, UINT64_MAX);
            }
        }

        // Destroy fences
        for (auto fence : m_fences)
        {
            vkDestroyFence(m_device, fence, nullptr);
        }

        // Destroy semaphores
        for (auto semaphore : m_imageAvailableSemaphores)
        {
            vkDestroySemaphore(m_device, semaphore, nullptr);
        }

        for (auto semaphore : m_renderFinishedSemaphores)
        {
            vkDestroySemaphore(m_device, semaphore, nullptr);
        }

        // Destroy image views
        for (auto imageView : m_imageViews)
        {
            vkDestroyImageView(m_device, imageView, nullptr);
        }
        //m_imageViews.clear();

        // Destroy swapchain
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    }

    void Swapchain::createSwapchain()
    {
        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);

        // create swapchain
        VkSwapchainCreateInfoKHR createInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_surface,
            .minImageCount = caps.minImageCount,
            .imageFormat = m_format,
            //.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            .imageExtent = m_extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            //.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            //.queueFamilyIndexCount = 0,
            //.pQueueFamilyIndices = nullptr,
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
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, nullptr);
        m_images.resize(m_imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, m_images.data());
        
        // create image views
        m_imageViews.resize(m_imageCount);

        for (size_t i = 0; i < m_imageCount; ++i)
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
        m_imageAvailableSemaphores.resize(m_maxImagesInFlight);
        m_renderFinishedSemaphores.resize(m_maxImagesInFlight);
        m_fences.resize(m_maxImagesInFlight);
        m_framesInFlight.resize(m_maxImagesInFlight, false);

        VkSemaphoreCreateInfo semaphoreInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        VkFenceCreateInfo fenceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        for (u32 i = 0; i < m_maxImagesInFlight; i++)
        {
            VkResult result;

            result = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
            if (result != VK_SUCCESS)
            {
                printLine(Print::Error, "vkCreateSemaphore: {}", getString(result));
                return;
            }

            result = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
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
    }

    u32 Swapchain::getImageCount() const
    {
        return m_imageCount;
    }

    VkFormat Swapchain::getImageFormat() const
    {
        return m_format;
    }

    VkExtent2D Swapchain::getExtent() const
    {
        return m_extent;
    }

    VkImageView Swapchain::getImageView(u32 imageIndex) const
    {
        return m_imageViews[imageIndex];
    }

    VkSemaphore Swapchain::getImageAvailableSemaphore() const
    {
        return m_imageAvailableSemaphores[m_currentFrame];
    }

    VkSemaphore Swapchain::getRenderFinishedSemaphore() const
    {
        return m_renderFinishedSemaphores[m_currentFrame];
    }

    VkExtent2D Swapchain::update()
    {
        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);

        VkExtent2D extent = caps.currentExtent;

        bool changed = extent.width != m_extent.width || extent.height != m_extent.height;
        bool valid = extent.width > 0 && extent.height > 0;

        m_extent = extent;

        if (changed && valid)
        {
            cleanup();
            createSwapchain();
            createSyncObjects();
            m_currentFrame = 0;
        }

        return m_extent;
    }

    VkResult Swapchain::acquireNextImage(u32& imageIndex)
    {
        // Wait for the fence of the current frame to be signaled (if it was previously used)
        if (m_framesInFlight[m_currentFrame])
        {
            VkResult waitResult = vkWaitForFences(m_device, 1, &m_fences[m_currentFrame], VK_TRUE, 250000000); // 0.25 seconds timeout
            if (waitResult == VK_TIMEOUT)
            {
                printLine(Print::Warning, "Fence wait timeout, resetting frame state");
                // Reset the fence and mark frame as not in flight to recover
                vkResetFences(m_device, 1, &m_fences[m_currentFrame]);
                m_framesInFlight[m_currentFrame] = false;
            }
            else if (waitResult != VK_SUCCESS)
            {
                printLine(Print::Error, "vkWaitForFences failed: {}", getString(waitResult));
                return waitResult;
            }
        }
        
        VkSemaphore imageAvailableSemaphore = m_imageAvailableSemaphores[m_currentFrame];
        VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        
        if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
        {
            // Reset the fence for the current frame
            vkResetFences(m_device, 1, &m_fences[m_currentFrame]);
            m_framesInFlight[m_currentFrame] = true;
        }
        else
        {
            // If acquire failed, don't mark the frame as in flight
            // This prevents the fence from being waited on in future calls
            printLine(Print::Warning, "vkAcquireNextImageKHR failed: {}", getString(result));
        }
        
        return result;
    }

    VkResult Swapchain::present(u32 imageIndex)
    {
        VkSemaphore renderFinishedSemaphore = m_renderFinishedSemaphores[m_currentFrame];

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
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            printLine(Print::Error, "vkQueuePresentKHR failed: {}", getString(result));
        }
        
        // Always mark the frame as not in flight and advance to the next frame
        m_framesInFlight[m_currentFrame] = false;
        m_currentFrame = (m_currentFrame + 1) % m_maxImagesInFlight;

        return result;
    }

    void Swapchain::skipFrame()
    {
        // If the current frame was marked as in flight but we're not going to submit work,
        // we need to reset the fence and mark it as not in flight
        if (m_framesInFlight[m_currentFrame])
        {
            vkResetFences(m_device, 1, &m_fences[m_currentFrame]);
            m_framesInFlight[m_currentFrame] = false;
        }
        
        // Advance to the next frame
        m_currentFrame = (m_currentFrame + 1) % m_maxImagesInFlight;
    }

    void Swapchain::resetSyncState()
    {
        // Wait for all fences with timeout
        for (u32 i = 0; i < m_maxImagesInFlight; ++i)
        {
            if (m_framesInFlight[i])
            {
                VkResult waitResult = vkWaitForFences(m_device, 1, &m_fences[i], VK_TRUE, 250000000);
                if (waitResult == VK_TIMEOUT)
                {
                    printLine(Print::Warning, "Fence wait timeout during reset; forcing reset.");
                    vkResetFences(m_device, 1, &m_fences[i]);
                }
                m_framesInFlight[i] = false;
            }
        }
        
        m_currentFrame = 0;
    }

} // namespace mango::vulkan

#endif // 0
