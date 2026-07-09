/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#if defined(MANGO_HAS_XLIB_WINDOW)

#define VK_USE_PLATFORM_XLIB_KHR

#include <mango/core/exception.hpp>
#include "window_surface.hpp"

#include "../window/xlib/xlib_window.hpp"

namespace mango::vulkan
{

    VkSurfaceKHR createVulkanSurfaceXlib(WindowBackend* backend, VkInstance instance)
    {
        auto* xlib = static_cast<XlibBackend*>(backend);

        VkXlibSurfaceCreateInfoKHR createInfo =
        {
            .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
            .dpy = xlib->x11Display(),
            .window = xlib->x11Window(),
        };

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkResult result = vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface);
        if (result != VK_SUCCESS)
        {
            MANGO_EXCEPTION("[XlibBackend] vkCreateXlibSurfaceKHR() failed.");
        }

        return surface;
    }

    bool getVulkanPresentationSupportXlib(WindowBackend* backend, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex)
    {
        auto* xlib = static_cast<XlibBackend*>(backend);
        VkBool32 support = vkGetPhysicalDeviceXlibPresentationSupportKHR(
            physicalDevice, queueFamilyIndex, xlib->x11Display(), xlib->x11VisualID());
        return support == VK_TRUE;
    }

} // namespace mango::vulkan

#endif // defined(MANGO_HAS_XLIB_WINDOW)
