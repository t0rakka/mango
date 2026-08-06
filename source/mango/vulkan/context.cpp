/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/vulkan/vulkan.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/print.hpp>
#include "window_surface.hpp"

namespace mango::vulkan
{

    namespace
    {

        u32 selectGraphicsPresentQueueFamily(Window& window, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
        {
            std::vector<VkQueueFamilyProperties> queueFamilies = getPhysicalDeviceQueueFamilyProperties(physicalDevice);

            for (size_t i = 0; i < queueFamilies.size(); ++i)
            {
                const VkQueueFamilyProperties& properties = queueFamilies[i];
                if (!(properties.queueFlags & VK_QUEUE_GRAPHICS_BIT))
                {
                    continue;
                }

                VkBool32 presentSupported = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, u32(i), surface, &presentSupported);
                if (!presentSupported)
                {
                    continue;
                }

                if (getVulkanPresentationSupport(window.backend(), physicalDevice, u32(i)))
                {
                    return u32(i);
                }
            }

            return UINT32_MAX;
        }

    } // namespace

    VulkanContext::VulkanContext(VkInstance instance)
        : m_instance(instance)
    {
    }

    VulkanContext::VulkanContext(Instance& instance)
        : m_instance(instance)
    {
    }

    VulkanContext::~VulkanContext()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;
            m_graphicsQueue = VK_NULL_HANDLE;
        }
    }

    bool VulkanContext::getPresentationSupport(Window& window, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex) const
    {
        return getVulkanPresentationSupport(window.backend(), physicalDevice, queueFamilyIndex);
    }

    u32 VulkanContext::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const
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

        MANGO_EXCEPTION("[VulkanContext] Failed to find suitable memory type.");
        return 0;
    }

    bool VulkanContext::verifyPresentation(VkSurfaceKHR surface) const
    {
        if (!m_device)
        {
            return false;
        }

        VkBool32 presentSupported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, m_graphicsQueueFamilyIndex,
                                             surface, &presentSupported);
        return presentSupported == VK_TRUE;
    }

    void VulkanContext::initialize(Window& window, VkSurfaceKHR surface, const VulkanDeviceConfig* config)
    {
        if (m_device != VK_NULL_HANDLE)
        {
            if (!verifyPresentation(surface))
            {
                printLine(Print::Error,
                    "[VulkanContext] Graphics queue cannot present to the new surface.");
            }
            return;
        }

        VulkanDeviceConfig defaults;
        const VulkanDeviceConfig& settings = config ? *config : defaults;
        createDevice(window, surface, settings);
    }

    void VulkanContext::createDevice(Window& window, VkSurfaceKHR surface, const VulkanDeviceConfig& settings)
    {
        m_physicalDevice = settings.physicalDevice;
        if (!m_physicalDevice)
        {
            m_physicalDevice = selectPhysicalDevice(m_instance);
        }

        if (!m_physicalDevice)
        {
            printLine(Print::Error, "[VulkanContext] No suitable physical device.");
            return;
        }

        m_graphicsQueueFamilyIndex = selectGraphicsPresentQueueFamily(window, m_physicalDevice, surface);
        if (m_graphicsQueueFamilyIndex == UINT32_MAX)
        {
            printLine(Print::Error, "[VulkanContext] No graphics + present queue family.");
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
            printLine(Print::Error, "[VulkanContext] vkCreateDevice: {}", getString(result));
            return;
        }

        vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
    }

} // namespace mango::vulkan
