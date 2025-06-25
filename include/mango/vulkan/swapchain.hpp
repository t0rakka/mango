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
        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;

        VkFormat m_format;
        VkExtent2D m_extent;
        VkSwapchainKHR m_swapchain;

        u32 m_imageCount = 0;

        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;

        static constexpr u32 m_maxImagesInFlight = 2;

        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_fences;
        std::vector<bool> m_framesInFlight;
        u32 m_currentFrame = 0;

        void cleanup();
        void configure();
        void createSwapchain();
        void createSyncObjects();

    public:
        Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkQueue graphicsQueue, VkQueue presentQueue);
        ~Swapchain();

        u32 getImageCount() const;
        VkFormat getImageFormat() const;
        VkExtent2D getExtent() const;
        VkImageView getImageView(u32 imageIndex) const;
        VkSemaphore getImageAvailableSemaphore() const;
        VkSemaphore getRenderFinishedSemaphore() const;

        void recreate(VkExtent2D extent);
        VkResult acquireNextImage(u32& imageIndex);
        VkResult present(u32 imageIndex);
    };

} // namespace mango::vulkan
