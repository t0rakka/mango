/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/window/window.hpp>
#include "window_surface.hpp"

namespace mango::vulkan
{

#if defined(MANGO_PLATFORM_WINDOWS)
    VkSurfaceKHR createVulkanSurfaceWin32(WindowBackend* backend, VkInstance instance);
    bool getVulkanPresentationSupportWin32(WindowBackend* backend, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex);
#elif defined(MANGO_PLATFORM_MACOS)
    void ensureCocoaVulkanContent(Window* window, WindowBackend* backend, int width, int height);
    VkSurfaceKHR createVulkanSurfaceCocoa(WindowBackend* backend, VkInstance instance);
    bool getVulkanPresentationSupportCocoa(WindowBackend* backend, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex);
#elif defined(MANGO_PLATFORM_LINUX)
    #if defined(MANGO_HAS_XCB_WINDOW)
        VkSurfaceKHR createVulkanSurfaceXcb(WindowBackend* backend, VkInstance instance);
        bool getVulkanPresentationSupportXcb(WindowBackend* backend, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex);
    #endif
    #if defined(MANGO_HAS_XLIB_WINDOW)
        VkSurfaceKHR createVulkanSurfaceXlib(WindowBackend* backend, VkInstance instance);
        bool getVulkanPresentationSupportXlib(WindowBackend* backend, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex);
    #endif
    #if defined(MANGO_HAS_WAYLAND_WINDOW)
        VkSurfaceKHR createVulkanSurfaceWayland(WindowBackend* backend, VkInstance instance);
        bool getVulkanPresentationSupportWayland(WindowBackend* backend, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex);
    #endif
#endif

    void ensureVulkanWindowContent(Window* window, WindowBackend* backend, int width, int height)
    {
#if defined(MANGO_PLATFORM_MACOS)
        ensureCocoaVulkanContent(window, backend, width, height);
#else
        MANGO_UNREFERENCED(window);
        MANGO_UNREFERENCED(backend);
        MANGO_UNREFERENCED(width);
        MANGO_UNREFERENCED(height);
#endif
    }

    VkSurfaceKHR createVulkanSurface(WindowBackend* backend, VkInstance instance)
    {
        switch (Window::getWindowSystem())
        {
#if defined(MANGO_PLATFORM_WINDOWS)
            case WindowSystem::Win32:
                return createVulkanSurfaceWin32(backend, instance);
#elif defined(MANGO_PLATFORM_MACOS)
            case WindowSystem::Cocoa:
                return createVulkanSurfaceCocoa(backend, instance);
#elif defined(MANGO_PLATFORM_LINUX)
    #if defined(MANGO_HAS_WAYLAND_WINDOW)
            case WindowSystem::Wayland:
                return createVulkanSurfaceWayland(backend, instance);
    #endif
    #if defined(MANGO_HAS_XLIB_WINDOW)
            case WindowSystem::Xlib:
                return createVulkanSurfaceXlib(backend, instance);
    #endif
    #if defined(MANGO_HAS_XCB_WINDOW)
            case WindowSystem::Xcb:
                return createVulkanSurfaceXcb(backend, instance);
    #endif
#endif
            default:
                break;
        }

        MANGO_EXCEPTION("[Vulkan] Unsupported window system for surface creation.");
    }

    bool getVulkanPresentationSupport(WindowBackend* backend, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex)
    {
        switch (Window::getWindowSystem())
        {
#if defined(MANGO_PLATFORM_WINDOWS)
            case WindowSystem::Win32:
                return getVulkanPresentationSupportWin32(backend, physicalDevice, queueFamilyIndex);
#elif defined(MANGO_PLATFORM_MACOS)
            case WindowSystem::Cocoa:
                return getVulkanPresentationSupportCocoa(backend, physicalDevice, queueFamilyIndex);
#elif defined(MANGO_PLATFORM_LINUX)
    #if defined(MANGO_HAS_WAYLAND_WINDOW)
            case WindowSystem::Wayland:
                return getVulkanPresentationSupportWayland(backend, physicalDevice, queueFamilyIndex);
    #endif
    #if defined(MANGO_HAS_XLIB_WINDOW)
            case WindowSystem::Xlib:
                return getVulkanPresentationSupportXlib(backend, physicalDevice, queueFamilyIndex);
    #endif
    #if defined(MANGO_HAS_XCB_WINDOW)
            case WindowSystem::Xcb:
                return getVulkanPresentationSupportXcb(backend, physicalDevice, queueFamilyIndex);
    #endif
#endif
            default:
                break;
        }

        return false;
    }

} // namespace mango::vulkan
