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

    const char* getString(VkResult result);
    const char* getString(VkPhysicalDeviceType deviceType);
    const char* getString(VkFormat format);
    const char* getString(VkColorSpaceKHR colorSpace);

} // namespace mango::vulkan
