/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vulkan/vulkan.h>
#include <mango/core/configure.hpp>
#include <mango/window/window.hpp>

namespace mango::vulkan
{

    class Instance;
    struct VulkanDeviceConfig;

    class VulkanContext : private NonCopyable
    {
    protected:
        VkInstance m_instance = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        u32 m_graphicsQueueFamilyIndex = 0;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;

        void createDevice(Window& window, VkSurfaceKHR surface, const VulkanDeviceConfig& settings);
        bool verifyPresentation(VkSurfaceKHR surface) const;

    public:
        explicit VulkanContext(VkInstance instance);
        explicit VulkanContext(Instance& instance);
        ~VulkanContext();

        void initialize(Window& window, VkSurfaceKHR surface, const VulkanDeviceConfig* config = nullptr);

        bool isDeviceReady() const
        {
            return m_device != VK_NULL_HANDLE;
        }

        VkInstance instance() const { return m_instance; }
        VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
        VkDevice device() const { return m_device; }
        VkQueue graphicsQueue() const { return m_graphicsQueue; }
        u32 graphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }

        bool getPresentationSupport(Window& window, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex) const;
        u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const;
    };

} // namespace mango::vulkan
