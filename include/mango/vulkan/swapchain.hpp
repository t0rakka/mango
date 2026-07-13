/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include <mango/vulkan/colorspace.hpp>
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

            VkResult submit(VkQueue graphicsQueue, VkCommandBuffer commandBuffer);

            // Same as submit(), but the queue submit additionally signals a caller-owned
            // timeline semaphore at signalValue, and optionally waits on a timeline value
            // first at waitStage. Pass VK_NULL_HANDLE to skip either.
            VkResult submit(VkQueue graphicsQueue, VkCommandBuffer commandBuffer,
                            VkSemaphore signalTimeline, u64 signalValue,
                            VkSemaphore waitTimeline = VK_NULL_HANDLE, u64 waitValue = 0,
                            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            VkResult present();

            // Same as submitAndPresent above, but the queue submit additionally signals a caller-owned
            // timeline semaphore at signalValue (so the caller can track frame
            // completion on a unified timeline), and optionally waits on a timeline
            // value first at waitStage (e.g. an upload the frame's sampling depends on,
            // such as work done on a separate transfer queue). Pass VK_NULL_HANDLE to
            // skip either. The internal binary image-available / render-finished
            // semaphores and the per-frame fence are used exactly as in the 2-arg form.
            VkResult submitAndPresent(VkQueue graphicsQueue, VkCommandBuffer commandBuffer,
                                      VkSemaphore signalTimeline, u64 signalValue,
                                      VkSemaphore waitTimeline = VK_NULL_HANDLE, u64 waitValue = 0,
                                      VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

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

        VkSurfaceFormatKHR m_surfaceFormat {};
        VkExtent2D m_extent { 0, 0 };
        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

        const Window* m_window = nullptr;

        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;

        // Keep the CPU-ahead present queue shallow. With FIFO (vsync-locked)
        // present, a deep queue means the displayed image trails the current
        // window geometry by that many vsyncs, which reads as resize lag/stretch.
        static constexpr u32 m_maxImagesInFlight = 2;

        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_fences;
        std::vector<VkFence> m_imagesInFlight;
        u32 m_currentFrame = 0;
        bool m_recreateRequired = false;

        void cleanup();
        void destroySyncObjects();
        void destroyImageViews();
        void configure();
        VkExtent2D resolveExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) const;
        VkPresentModeKHR choosePresentMode() const;
        bool createSwapchain(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, VkExtent2D extent, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
        bool recreateSwapchain(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, VkExtent2D extent);
        bool createImageViews();
        void createSyncObjects();
        VkResult acquireNextImage(u32& imageIndex);
        VkResult present(u32 imageIndex);

    public:
        Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceFormatKHR format, VkQueue presentQueue,
                  const Window* window);
        ~Swapchain();

        u32 getImageCount() const;
        VkSurfaceFormatKHR getSurfaceFormat() const;
        VkFormat getFormat() const;
        VkColorSpaceKHR getColorSpace() const;
        VkExtent2D getExtent() const;

        std::string getOutputTransformGLSL() const;
        std::string getOutputTransformGLSL(const OutputTransformOptions& options) const;
        std::string getDisplayOutputTransformGLSL() const;
        VkImage getImage(u32 imageIndex) const;
        VkImageView getImageView(u32 imageIndex) const;

        void cmdTransitionImageToColorAttachment(VkCommandBuffer commandBuffer, u32 imageIndex) const;
        void cmdTransitionImageToGeneral(VkCommandBuffer commandBuffer, u32 imageIndex) const;
        void cmdTransitionImageToPresent(VkCommandBuffer commandBuffer, u32 imageIndex) const;
        void cmdTransitionImageFromGeneralToPresent(VkCommandBuffer commandBuffer, u32 imageIndex) const;

        // Refreshes the swapchain to match the current surface. Recreation is
        // triggered only when the surface extent changed or a previous frame flagged
        // VK_SUBOPTIMAL_KHR / VK_ERROR_OUT_OF_DATE_KHR; returns true when a recreation
        // actually happened (so the caller can rebuild size-dependent resources).
        bool updateSwapchain();
        Frame beginFrame();
    };

} // namespace mango::vulkan
