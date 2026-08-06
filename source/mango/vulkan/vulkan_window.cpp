/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/print.hpp>
#include <mango/core/timer.hpp>
#include <mango/vulkan/vulkan.hpp>
#include "window_surface.hpp"

namespace mango::vulkan
{

    namespace
    {

        bool surfaceFormatMatches(const VkSurfaceFormatKHR& a, const VkSurfaceFormatKHR& b)
        {
            return a.format == b.format && a.colorSpace == b.colorSpace;
        }

        VkSurfaceFormatKHR selectSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                               const VulkanDeviceConfig& settings)
        {
            const std::vector<VkSurfaceFormatKHR> surfaceFormats =
                getSurfaceFormats(physicalDevice, surface);

            if (settings.preferredFormats.empty())
            {
                return mango::vulkan::selectSurfaceFormat(surfaceFormats, settings.surfaceFormatIntent).format;
            }

            for (const VkSurfaceFormatKHR& preferred : settings.preferredFormats)
            {
                for (const VkSurfaceFormatKHR& format : surfaceFormats)
                {
                    if (surfaceFormatMatches(format, preferred))
                    {
                        return format;
                    }
                }
            }

            return mango::vulkan::selectSurfaceFormat(surfaceFormats, settings.surfaceFormatIntent).format;
        }

    } // namespace

    VulkanWindow::VulkanWindow(VulkanContext& context, int width, int height, u32 flags,
                               const VulkanDeviceConfig* config)
        : Window(width, height, flags | Window::API_VULKAN)
        , m_context(context)
        , m_surface(VK_NULL_HANDLE)
    {
        ensureVulkanWindowContent(this, backend(), width, height);
        m_surface = createVulkanSurface(backend(), m_context.instance());
        if (!m_surface)
        {
            MANGO_EXCEPTION("[VulkanWindow] Creating surface failed.");
        }

        m_context.initialize(*this, m_surface, config);

        if (isDeviceReady())
        {
            createSwapchainAndPool(config);
        }
    }

    VulkanWindow::~VulkanWindow()
    {
        destroyWindowResources();

        if (m_surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_context.instance(), m_surface, nullptr);
            m_surface = VK_NULL_HANDLE;
        }
    }

    bool VulkanWindow::getPresentationSupport(VkPhysicalDevice physicalDevice, u32 queueFamilyIndex)
    {
        return m_context.getPresentationSupport(*this, physicalDevice, queueFamilyIndex);
    }

    Swapchain& VulkanWindow::swapchain()
    {
        return *m_swapchain;
    }

    const Swapchain& VulkanWindow::swapchain() const
    {
        return *m_swapchain;
    }

    VkCommandBuffer VulkanWindow::commandBuffer(u32 imageIndex) const
    {
        if (imageIndex < m_commandBuffers.size())
        {
            return m_commandBuffers[imageIndex];
        }

        return VK_NULL_HANDLE;
    }

    void VulkanWindow::createSwapchainAndPool(const VulkanDeviceConfig* config)
    {
        VulkanDeviceConfig defaults;
        const VulkanDeviceConfig& settings = config ? *config : defaults;

        m_surfaceFormat = selectSurfaceFormat(m_context.physicalDevice(), m_surface, settings);

        m_swapchain = std::make_unique<Swapchain>(m_context.device(), m_context.physicalDevice(), m_surface,
            m_surfaceFormat, m_context.graphicsQueue(), this);

        VkCommandPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_context.graphicsQueueFamilyIndex(),
        };

        VkResult result = vkCreateCommandPool(m_context.device(), &poolInfo, nullptr, &m_commandPool);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "[VulkanWindow] vkCreateCommandPool: {}", getString(result));
            return;
        }

        m_swapchainExtent = m_swapchain->getExtent();
        m_swapchainGeneration = m_swapchain->generation();
        allocateCommandBuffers();
    }

    void VulkanWindow::destroyWindowResources()
    {
        if (!isDeviceReady())
        {
            return;
        }

        vkDeviceWaitIdle(m_context.device());

        if (m_commandPool != VK_NULL_HANDLE && !m_commandBuffers.empty())
        {
            vkFreeCommandBuffers(m_context.device(), m_commandPool, u32(m_commandBuffers.size()), m_commandBuffers.data());
            m_commandBuffers.clear();
        }

        m_swapchain.reset();

        if (m_commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_context.device(), m_commandPool, nullptr);
            m_commandPool = VK_NULL_HANDLE;
        }
    }

    void VulkanWindow::allocateCommandBuffers()
    {
        if (!m_swapchain || m_commandPool == VK_NULL_HANDLE)
        {
            return;
        }

        if (!m_commandBuffers.empty())
        {
            vkFreeCommandBuffers(m_context.device(), m_commandPool, u32(m_commandBuffers.size()), m_commandBuffers.data());
            m_commandBuffers.clear();
        }

        u32 imageCount = m_swapchain->getImageCount();
        m_commandBuffers.resize(imageCount);

        VkCommandBufferAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = imageCount,
        };

        vkAllocateCommandBuffers(m_context.device(), &allocInfo, m_commandBuffers.data());
    }

    void VulkanWindow::ensureCommandBuffers()
    {
        if (!m_swapchain)
        {
            return;
        }

        if (m_commandBuffers.size() != m_swapchain->getImageCount())
        {
            allocateCommandBuffers();
        }
    }

    Swapchain::Frame VulkanWindow::beginDraw()
    {
        if (!m_swapchain)
        {
            return Swapchain::Frame();
        }

        VkExtent2D prevExtent = m_swapchainExtent;
        const u32 prevGeneration = m_swapchainGeneration;

        Swapchain::Frame frame = m_swapchain->beginFrame();
        if (!frame)
        {
            return frame;
        }

        VkExtent2D extent = m_swapchain->getExtent();
        ensureCommandBuffers();

        const u32 generation = m_swapchain->generation();
        const bool swapchainRecreated = generation != prevGeneration;
        const bool extentChanged = extent.width > 1 && extent.height > 1 &&
            (extent.width != prevExtent.width || extent.height != prevExtent.height);

        if (swapchainRecreated || extentChanged)
        {
            m_swapchainGeneration = generation;
            m_swapchainExtent = extent;
            onSwapchainResize(extent);
        }

        return frame;
    }

    void VulkanWindow::presentInitialFrame()
    {
        if (!m_swapchain)
        {
            return;
        }

        m_swapchain->requestRecreate();

        FrameInfo info {};
        info.time_us = Time::us();
        info.trigger = FrameTrigger::Invalidate;
        onFrame(info);

        vkDeviceWaitIdle(m_context.device());
    }

    void VulkanWindow::onEventLoopStarting()
    {
        if (!m_on_device_ready_called)
        {
            m_on_device_ready_called = true;
            if (isDeviceReady())
            {
                onDeviceReady();
                presentInitialFrame();
                setVisible(true);
            }
        }
    }

    void VulkanWindow::onDeviceReady()
    {
    }

    void VulkanWindow::onSwapchainResize(VkExtent2D extent)
    {
        MANGO_UNREFERENCED(extent);
    }

    void VulkanWindow::onResize(int width, int height)
    {
        Window::onResize(width, height);

        if (m_swapchain && width > 1 && height > 1)
        {
            m_swapchain->requestRecreate();
        }
    }

} // namespace mango::vulkan
