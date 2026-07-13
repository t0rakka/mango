/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/print.hpp>
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

        u32 selectGraphicsPresentQueueFamily(VulkanWindow& window, VkPhysicalDevice physicalDevice)
        {
            std::vector<VkQueueFamilyProperties> queueFamilies = getPhysicalDeviceQueueFamilyProperties(physicalDevice);

            for (size_t i = 0; i < queueFamilies.size(); ++i)
            {
                const VkQueueFamilyProperties& properties = queueFamilies[i];
                if ((properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                    window.getPresentationSupport(physicalDevice, u32(i)))
                {
                    return u32(i);
                }
            }

            return UINT32_MAX;
        }

    } // namespace

    // ------------------------------------------------------------------------------
    // VulkanWindow
    // ------------------------------------------------------------------------------

    VulkanWindow::VulkanWindow(VkInstance instance, int width, int height, u32 flags,
                               const VulkanDeviceConfig* config)
        : Window(width, height, flags | Window::API_VULKAN)
        , m_instance(instance)
        , m_surface(VK_NULL_HANDLE)
    {
        ensureVulkanWindowContent(this, backend(), width, height);
        m_surface = createVulkanSurface(backend(), m_instance);
        if (!m_surface)
        {
            MANGO_EXCEPTION("[VulkanWindow] Creating surface failed.");
        }

        initDevice(config);
    }

    VulkanWindow::~VulkanWindow()
    {
        destroyDevice();

        if (m_surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        }
    }

    bool VulkanWindow::getPresentationSupport(VkPhysicalDevice physicalDevice, u32 queueFamilyIndex)
    {
        return getVulkanPresentationSupport(backend(), physicalDevice, queueFamilyIndex);
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

    u32 VulkanWindow::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

        for (u32 i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        MANGO_EXCEPTION("[VulkanWindow] Failed to find suitable memory type.");
        return 0;
    }

    void VulkanWindow::initDevice(const VulkanDeviceConfig* config)
    {
        VulkanDeviceConfig defaults;
        const VulkanDeviceConfig& settings = config ? *config : defaults;

        m_physicalDevice = settings.physicalDevice;
        if (!m_physicalDevice)
        {
            m_physicalDevice = selectPhysicalDevice(m_instance);
        }

        if (!m_physicalDevice)
        {
            printLine(Print::Error, "[VulkanWindow] No suitable physical device.");
            return;
        }

        m_graphicsQueueFamilyIndex = selectGraphicsPresentQueueFamily(*this, m_physicalDevice);
        if (m_graphicsQueueFamilyIndex == UINT32_MAX)
        {
            printLine(Print::Error, "[VulkanWindow] No graphics + present queue family.");
            return;
        }

        float queuePriority = 1.0f;

        VkDeviceQueueCreateInfo queueCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = m_graphicsQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };

        std::vector<const char*> deviceExtensions = requiredDeviceExtensions();
        for (const char* extension : settings.deviceExtensions)
        {
            deviceExtensions.push_back(extension);
        }

        VkPhysicalDeviceFeatures supportedFeatures {};
        vkGetPhysicalDeviceFeatures(m_physicalDevice, &supportedFeatures);

        VkPhysicalDeviceFeatures deviceFeatures {};
        if (supportedFeatures.shaderStorageImageReadWithoutFormat)
        {
            deviceFeatures.shaderStorageImageReadWithoutFormat = VK_TRUE;
        }
        if (supportedFeatures.shaderStorageImageWriteWithoutFormat)
        {
            deviceFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;
        }

        VkPhysicalDeviceDescriptorIndexingFeatures supportedIndexing
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        };

        VkPhysicalDeviceFeatures2 supportedFeatures2
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &supportedIndexing,
        };

        vkGetPhysicalDeviceFeatures2(m_physicalDevice, &supportedFeatures2);

        VkPhysicalDeviceVulkan12Features features12 =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .descriptorBindingStorageBufferUpdateAfterBind =
                supportedIndexing.descriptorBindingStorageBufferUpdateAfterBind,
            .timelineSemaphore = VK_TRUE,
        };

        VkPhysicalDeviceVulkan13Features features13 =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &features12,
            .dynamicRendering = VK_TRUE,
        };

        VkDeviceCreateInfo deviceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = settings.deviceCreateInfoPNext ? settings.deviceCreateInfoPNext : &features13,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = u32(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
            .pEnabledFeatures = &deviceFeatures,
        };

        VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "[VulkanWindow] vkCreateDevice: {}", getString(result));
            return;
        }

        vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);

        m_surfaceFormat = selectSurfaceFormat(m_physicalDevice, m_surface, settings);

        m_swapchain = std::make_unique<Swapchain>(m_device, m_physicalDevice, m_surface,
            m_surfaceFormat, m_graphicsQueue, this);

        VkCommandPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_graphicsQueueFamilyIndex,
        };

        result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "[VulkanWindow] vkCreateCommandPool: {}", getString(result));
            return;
        }

        m_swapchainExtent = m_swapchain->getExtent();
        allocateCommandBuffers();
    }

    void VulkanWindow::destroyDevice()
    {
        if (m_device == VK_NULL_HANDLE)
        {
            return;
        }

        vkDeviceWaitIdle(m_device);

        if (m_commandPool != VK_NULL_HANDLE && !m_commandBuffers.empty())
        {
            vkFreeCommandBuffers(m_device, m_commandPool, u32(m_commandBuffers.size()), m_commandBuffers.data());
            m_commandBuffers.clear();
        }

        m_swapchain.reset();

        if (m_commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
            m_commandPool = VK_NULL_HANDLE;
        }

        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
        m_graphicsQueue = VK_NULL_HANDLE;
    }

    void VulkanWindow::allocateCommandBuffers()
    {
        if (!m_swapchain || m_commandPool == VK_NULL_HANDLE)
        {
            return;
        }

        if (!m_commandBuffers.empty())
        {
            vkFreeCommandBuffers(m_device, m_commandPool, u32(m_commandBuffers.size()), m_commandBuffers.data());
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

        vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data());
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

        Swapchain::Frame frame = m_swapchain->beginFrame();
        if (!frame)
        {
            return frame;
        }

        VkExtent2D extent = m_swapchain->getExtent();
        ensureCommandBuffers();

        if (extent.width != prevExtent.width || extent.height != prevExtent.height)
        {
            m_swapchainExtent = extent;
            onSwapchainResize(extent);
        }

        return frame;
    }

    void VulkanWindow::enterEventLoop()
    {
        if (!m_on_device_ready_called)
        {
            m_on_device_ready_called = true;
            if (isDeviceReady())
            {
                onDeviceReady();
                setVisible(true);
            }
        }

        Window::enterEventLoop();
    }

    void VulkanWindow::enterEventLoop(const EventLoopConfig& config)
    {
        if (!m_on_device_ready_called)
        {
            m_on_device_ready_called = true;
            if (isDeviceReady())
            {
                onDeviceReady();
                setVisible(true);
            }
        }

        Window::enterEventLoop(config);
    }

    void VulkanWindow::onDeviceReady()
    {
    }

    void VulkanWindow::onSwapchainResize(VkExtent2D extent)
    {
        MANGO_UNREFERENCED(extent);
    }

} // namespace mango::vulkan
