/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/configure.hpp>

#if defined(MANGO_PLATFORM_WINDOWS)

#define VK_USE_PLATFORM_WIN32_KHR

#include <mango/core/exception.hpp>
#include <mango/window/window.hpp>
#include "window_surface.hpp"

#include "../window/win32/win32_window.hpp"

namespace mango::vulkan
{

    VkSurfaceKHR createVulkanSurfaceWin32(WindowBackend* backend, VkInstance instance)
    {
        auto* context = static_cast<WindowContext*>(backend);

        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = ::GetModuleHandle(NULL),
            .hwnd = context->hwnd,
        };

        VkResult result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
        if (result != VK_SUCCESS)
        {
            MANGO_EXCEPTION("[WindowContext] vkCreateWin32SurfaceKHR failed.");
        }

        return surface;
    }

    bool getVulkanPresentationSupportWin32(WindowBackend* backend, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex)
    {
        MANGO_UNREFERENCED(backend);
        return vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
    }

} // namespace mango::vulkan

#endif // defined(MANGO_PLATFORM_WINDOWS)
