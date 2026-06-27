/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>

#if defined(MANGO_ENABLE_WINDOW)

#if defined(MANGO_ENABLE_VULKAN)
#include <vulkan/vulkan.h>
#endif

namespace mango
{

    // -----------------------------------------------------------------------
    // WindowBackend
    // -----------------------------------------------------------------------

    // Abstract per-window-system implementation. One concrete subclass exists
    // per backend (Win32Backend, CocoaBackend, XlibBackend, XcbBackend,
    // WaylandBackend). The owning Window forwards every platform call here, and
    // the backend's event loop calls back into the Window (dispatchFrame, the
    // on* handlers) via the owner pointer. Native types are confined to each
    // backend's own translation unit, so they never need to coexist.
    struct WindowBackend
    {
        Window* owner = nullptr;   // owning Window, for event dispatch and callbacks

        // Suppress frame dispatch while a graphics layer reconfigures the surface
        // (e.g. a fullscreen toggle). Shared storage so the GL/Vulkan layers can
        // set it through the abstract interface while each backend's event loop
        // reads it directly.
        bool busy = false;

        WindowBackend() = default;
        virtual ~WindowBackend() = default;

        virtual void setWindowPosition(int x, int y) = 0;
        virtual void setWindowSize(int width, int height) = 0;
        virtual void setTitle(const std::string& title) = 0;
        virtual void setVisible(bool enable) = 0;

        virtual math::int32x2 getWindowSize() const = 0;
        virtual math::int32x2 getCursorPosition() const = 0;
        virtual double getDisplayRefreshRate() const = 0;

        virtual void toggleFullscreen() = 0;
        virtual bool isFullscreen() const = 0;

        virtual bool isKeyPressed(Keycode code) const = 0;

        virtual void runEventLoop() = 0;
        virtual void wakeEventLoop() = 0;

#if defined(MANGO_ENABLE_VULKAN)
        // Native Vulkan surface creation. Each backend defines the matching
        // VK_USE_PLATFORM_*_KHR in its own translation unit.
        virtual VkSurfaceKHR createVulkanSurface(VkInstance instance) = 0;
        virtual bool getPresentationSupport(VkPhysicalDevice physicalDevice, u32 queueFamilyIndex) = 0;
#endif

#if defined(MANGO_ENABLE_EGL)
        // EGL integration. Lets the single, shared EGL translation unit obtain the
        // native display/window handles without including any backend-specific
        // native header, so the xlib / xcb / wayland headers never share a TU.
        //
        // eglNativeDisplay():  EGLNativeDisplayType as void* (nullptr selects
        //                      EGL_DEFAULT_DISPLAY; Wayland returns its wl_display).
        // eglNativeWindow():   creates the OS window (X11) or wl_egl_window
        //                      (Wayland) and returns the EGLNativeWindowType as a
        //                      void* (X11 packs the XID via uintptr_t).
        // eglPresent():        per-frame hook before eglSwapBuffers (Wayland sync).
        virtual void* eglNativeDisplay() { return nullptr; }
        virtual void* eglNativeWindow(int /*width*/, int /*height*/, u32 /*flags*/) { return nullptr; }
        virtual void eglPresent() {}
#endif
    };

    // -----------------------------------------------------------------------
    // Backend factory
    // -----------------------------------------------------------------------

    // Per-backend creators are defined only in their own (guarded) translation
    // unit, so the linker pulls in exactly the backends compiled for the target.
    std::unique_ptr<WindowBackend> createWin32Backend(Window* window, int width, int height, u32 flags, const char* title);
    std::unique_ptr<WindowBackend> createCocoaBackend(Window* window, int width, int height, u32 flags, const char* title);
    std::unique_ptr<WindowBackend> createXlibBackend(Window* window, int width, int height, u32 flags, const char* title);
    std::unique_ptr<WindowBackend> createXcbBackend(Window* window, int width, int height, u32 flags, const char* title);
    std::unique_ptr<WindowBackend> createWaylandBackend(Window* window, int width, int height, u32 flags, const char* title);

    // Resolves WindowSystem (including Default) to a concrete backend, dispatching
    // only to creators that exist in this build.
    std::unique_ptr<WindowBackend> createWindowBackend(WindowSystem ws, Window* window, int width, int height, u32 flags, const char* title);

    // Resolve WindowSystem::Default and validate a requested system against what
    // this platform/build actually provides.
    WindowSystem resolveWindowSystem(WindowSystem ws);

} // namespace mango

#endif // defined(MANGO_ENABLE_WINDOW)
