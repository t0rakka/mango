/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/core.hpp>
#include <mango/window/window.hpp>

#include <vulkan/vulkan.h>

namespace mango::vulkan
{

    template <typename T>
    class VulkanHandle : public NonCopyable
    {
    protected:
        T m_handle = VK_NULL_HANDLE;

    public:
        operator T () const
        {
            return m_handle;
        }
    };

    class Instance : public VulkanHandle<VkInstance>
    {
    public:
        Instance(const VkApplicationInfo& applicationInfo,
                 const std::vector<const char*> layers,
                 const std::vector<const char*> extensions);
        ~Instance();

    };

    class Device : public VulkanHandle<VkDevice>
    {
    public:
        Device(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& deviceCreateInfo);
        ~Device();
    };

    class VulkanWindow : public Window
    {
    protected:
        VkInstance m_instance;
        VkSurfaceKHR m_surface;

    public:
        VulkanWindow(VkInstance instance, int width, int height, u32 flags);
        ~VulkanWindow();

        bool getPresentationSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) const;
        void toggleFullscreen();
        bool isFullscreen() const;
    };

    std::vector<const char*> getSurfaceExtensions();

    std::vector<VkExtensionProperties> enumerateInstanceExtensionProperties(const char* layerName = nullptr);
    std::vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance);
    VkPhysicalDevice selectPhysicalDevice(VkInstance instance);
    std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice);

    const char* getString(VkResult result);
    const char* getString(VkPhysicalDeviceType deviceType);
    const char* getString(VkFormat format);
    const char* getString(VkColorSpaceKHR colorSpace);

} // namespace mango::vulkan
