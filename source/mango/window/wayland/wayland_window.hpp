/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/window/window.hpp>
#include "../window_backend.hpp"

#if defined(MANGO_ENABLE_WAYLAND)

#include <mango/math/math.hpp>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;

namespace mango
{

    class Window;

    // Concrete Wayland backend. Distinct class name (vs XlibBackend / XcbBackend)
    // so all three Linux backends can be linked into one binary without ODR clash;
    // the native wayland-client headers live only in this translation unit (the
    // shared EGL TU reaches the wl_egl_window via the eglNative* hooks).
    struct WaylandBackend : WindowBackend
    {
        // native handle (formerly in the public WindowHandle)
        struct wl_display* display = nullptr;
        struct wl_surface* surface = nullptr;

        // Wayland core objects
        struct wl_registry* registry = nullptr;
        struct wl_compositor* compositor = nullptr;
        struct xdg_wm_base* xdg_wm_base = nullptr;
        struct xdg_surface* xdg_surface = nullptr;
        struct xdg_toplevel* xdg_toplevel = nullptr;

        // Input handling
        struct wl_seat* seat = nullptr;
        struct wl_pointer* pointer = nullptr;
        struct wl_keyboard* keyboard = nullptr;

        // EGL native window (wl_egl_window*, stored as void* to avoid wayland-egl in this header)
        void* egl_window = nullptr;
        int32_t egl_synced_size[2] = { 0, 0 };
        int32_t buffer_scale = 1;

        // XKB handling
        struct xkb_context* xkb_context = nullptr;
        struct xkb_keymap* xkb_keymap = nullptr;
        struct xkb_state* xkb_state = nullptr;

        // Keyboard state
        bool keyboard_focused = false;
        bool key_pressed[256] = {};

        // Pointer state
        bool pointer_focused = false;

        // Window state
        bool is_looping = false;
        bool configured = false;
        bool needs_redraw = false;
        bool pending_resize = false;
        bool maximized = false;
        bool activated = false;
        int32_t size[2] = { 0, 0 };
        int32_t cursor[2] = { 0, 0 };
        uint32_t mouse_time[6] = {};
        bool fullscreen = false;

        WaylandBackend(int width, int height, u32 flags);
        ~WaylandBackend() override;

        // WindowBackend interface
        void setWindowPosition(int x, int y) override;
        void setWindowSize(int width, int height) override;
        void setTitle(const std::string& title) override;
        void setVisible(bool enable) override;
        math::int32x2 getWindowSize() const override;
        math::int32x2 getCursorPosition() const override;
        double getDisplayRefreshRate() const override;
        void toggleFullscreen() override;
        bool isFullscreen() const override;
        bool isKeyPressed(Keycode code) const override;
        void runEventLoop() override;
        void wakeEventLoop() override;

#if defined(MANGO_ENABLE_VULKAN)
        VkSurfaceKHR createVulkanSurface(VkInstance instance) override;
        bool getPresentationSupport(VkPhysicalDevice physicalDevice, u32 queueFamilyIndex) override;
#endif

#if defined(MANGO_ENABLE_EGL)
        void* eglNativeDisplay() override;
        void* eglNativeWindow(int width, int height, u32 flags) override;
        void eglPresent() override;
#endif

        bool createWaylandWindow(int width, int height, const char* title);
        void syncSurfaceScale();
        void syncEGLWindow();
        void requestRefresh();
        void processEvents();
        void dispatchPendingResize();
    };

} // namespace mango

#endif // defined(MANGO_ENABLE_WAYLAND)
