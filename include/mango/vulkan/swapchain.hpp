/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <mango/vulkan/vulkan.hpp>

namespace mango::vulkan
{

    class Swapchain
    {
    public:
        class Frame
        {
        public:
            Frame() = default;

            u32 imageIndex() const
            {
                return m_imageIndex;
            }

            VkResult acquireResult() const
            {
                return m_acquireResult;
            }

            explicit operator bool() const
            {
                return m_swapchain != nullptr;
            }

            VkImage image() const;
            VkImageView imageView() const;

            VkResult submitAndPresent(VkQueue graphicsQueue, VkCommandBuffer commandBuffer);

        private:
            friend class Swapchain;

            Swapchain* m_swapchain = nullptr;
            u32 m_imageIndex = 0;
            VkResult m_acquireResult = VK_NOT_READY;

            Frame(Swapchain* swapchain, u32 imageIndex, VkResult acquireResult);
        };

    private:
        VkDevice m_device;
        VkPhysicalDevice m_physicalDevice;
        VkSurfaceKHR m_surface;
        VkQueue m_presentQueue;

        VkFormat m_format;
        VkColorSpaceKHR m_colorSpace;
        VkExtent2D m_extent { 0, 0 };
        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

        const Window* m_window = nullptr;

        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;

        static constexpr u32 m_maxImagesInFlight = 5;

        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_fences;
        std::vector<VkFence> m_imagesInFlight;
        u32 m_currentFrame = 0;

        void cleanup();
        void configure();
        VkExtent2D resolveExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) const;
        void createSwapchain(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, VkExtent2D extent);
        void createSyncObjects();
        VkResult acquireNextImage(u32& imageIndex);
        VkResult present(u32 imageIndex);

    public:
        Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceFormatKHR format, VkQueue presentQueue,
                  const Window* window);
        ~Swapchain();

        u32 getImageCount() const;
        VkFormat getFormat() const;
        VkColorSpaceKHR getColorSpace() const;
        VkExtent2D getExtent() const;
        VkImage getImage(u32 imageIndex) const;
        VkImageView getImageView(u32 imageIndex) const;

        void cmdTransitionImageToColorAttachment(VkCommandBuffer commandBuffer, u32 imageIndex) const;
        void cmdTransitionImageToPresent(VkCommandBuffer commandBuffer, u32 imageIndex) const;

        bool recreateSwapchain();
        Frame beginFrame();
    };

} // namespace mango::vulkan
