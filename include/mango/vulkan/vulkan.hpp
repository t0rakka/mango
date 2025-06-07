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
        VkInstance m_instance { VK_NULL_HANDLE };
        VkSurfaceKHR m_surface { VK_NULL_HANDLE };

        bool m_fullscreen { false };

    public:
        VulkanWindow(int width, int height, u32 flags);
        ~VulkanWindow();
    };

    const char* getString(VkResult result);
    const char* getString(VkPhysicalDeviceType deviceType);
    const char* getString(VkFormat format);
    const char* getString(VkColorSpaceKHR colorSpace);

} // namespace mango::vulkan
