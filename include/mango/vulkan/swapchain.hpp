/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <mango/vulkan/vulkan.hpp>

namespace mango::vulkan
{

    class Swapchain
    {
    private:
        VkDevice m_device;
        VkPhysicalDevice m_physicalDevice;
        VkSurfaceKHR m_surface;
        VkQueue m_presentQueue;

        VkFormat m_format;
        VkColorSpaceKHR m_colorSpace;
        VkExtent2D m_extent { 0, 0 };
        VkSwapchainKHR m_swapchain;

        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;

        static constexpr u32 m_maxImagesInFlight = 5;

        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_fences;
        u32 m_currentFrame = 0;

        void cleanup();
        void configure();
        void createSwapchain(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
        void createSyncObjects();

    public:
        Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkQueue presentQueue);
        ~Swapchain();

        u32 getImageCount() const;
        VkFormat getFormat() const;
        VkColorSpaceKHR getColorSpace() const;
        VkExtent2D getExtent() const;
        VkImageView getImageView(u32 imageIndex) const;
        VkSemaphore getImageAvailableSemaphore() const;
        VkSemaphore getRenderFinishedSemaphore() const;
        VkFence getFence() const;

        bool recreateSwapchain();
        VkResult acquireNextImage(u32& imageIndex);
        VkResult present(u32 imageIndex);
    };

} // namespace mango::vulkan
