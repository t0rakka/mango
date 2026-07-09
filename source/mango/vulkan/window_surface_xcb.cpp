/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#if defined(MANGO_HAS_XCB_WINDOW)

#define VK_USE_PLATFORM_XCB_KHR

#include <mango/core/exception.hpp>
#include "window_surface.hpp"

#include "../window/xcb/xcb_window.hpp"

namespace mango::vulkan
{

    VkSurfaceKHR createVulkanSurfaceXcb(WindowBackend* backend, VkInstance instance)
    {
        auto* xcb = static_cast<XcbBackend*>(backend);

        VkXcbSurfaceCreateInfoKHR createInfo =
        {
            .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
            .connection = xcb->connection,
            .window = xcb->window,
        };

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkResult result = vkCreateXcbSurfaceKHR(instance, &createInfo, nullptr, &surface);
        if (result != VK_SUCCESS)
        {
            MANGO_EXCEPTION("[XcbBackend] vkCreateXcbSurfaceKHR() failed.");
        }

        return surface;
    }

    bool getVulkanPresentationSupportXcb(WindowBackend* backend, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex)
    {
        auto* xcb = static_cast<XcbBackend*>(backend);
        VkBool32 support = vkGetPhysicalDeviceXcbPresentationSupportKHR(
            physicalDevice, queueFamilyIndex, xcb->connection, xcb->visualid);
        return support == VK_TRUE;
    }

} // namespace mango::vulkan

#endif // defined(MANGO_HAS_XCB_WINDOW)
