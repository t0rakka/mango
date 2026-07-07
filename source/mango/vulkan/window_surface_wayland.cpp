/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#if defined(MANGO_HAS_WAYLAND_WINDOW)

#include <mango/core/exception.hpp>
#include "window_surface.hpp"

#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

#include "../../window/wayland/wayland_window.hpp"

namespace mango::vulkan
{

    VkSurfaceKHR createVulkanSurfaceWayland(WindowBackend* backend, VkInstance instance)
    {
        auto* wayland = static_cast<WaylandBackend*>(backend);

        VkWaylandSurfaceCreateInfoKHR createInfo =
        {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .display = wayland->display,
            .surface = wayland->surface,
        };

        VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
        VkResult result = vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &vk_surface);
        if (result != VK_SUCCESS)
        {
            MANGO_EXCEPTION("[WaylandBackend] vkCreateWaylandSurfaceKHR() failed.");
        }

        return vk_surface;
    }

    bool getVulkanPresentationSupportWayland(WindowBackend* backend, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex)
    {
        auto* wayland = static_cast<WaylandBackend*>(backend);
        VkBool32 support = vkGetPhysicalDeviceWaylandPresentationSupportKHR(
            physicalDevice, queueFamilyIndex, wayland->display);
        return support == VK_TRUE;
    }

} // namespace mango::vulkan

#endif // defined(MANGO_HAS_WAYLAND_WINDOW)
