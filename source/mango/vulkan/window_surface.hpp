/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vulkan/vulkan.h>
#include <mango/core/configure.hpp>

namespace mango
{
    class Window;
    struct WindowBackend;
}

namespace mango::vulkan
{

    void ensureVulkanWindowContent(Window* window, WindowBackend* backend, int width, int height);
    VkSurfaceKHR createVulkanSurface(WindowBackend* backend, VkInstance instance);
    bool getVulkanPresentationSupport(WindowBackend* backend, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex);

} // namespace mango::vulkan
