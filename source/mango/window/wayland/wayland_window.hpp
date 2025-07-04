/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/window/window.hpp>

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

#include <mango/math/math.hpp>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

namespace mango
{

    struct WindowContext : WindowHandle
    {
        WindowHandle handle;

        // Wayland core objects
        struct wl_registry* registry;
        struct wl_compositor* compositor;
        struct wl_shell* shell;
        struct wl_shell_surface* shell_surface;

        // Input handling
        struct wl_seat* seat;
        struct wl_pointer* pointer;
        struct wl_keyboard* keyboard;
        struct wl_output* output;

        // XKB handling
        struct xkb_context* xkb_context;
        struct xkb_keymap* xkb_keymap;
        struct xkb_state* xkb_state;

        // Window state
        bool is_looping;
        bool busy;
        bool configured;
        int32_t size[2];
        uint32_t mouse_time[6];
        bool fullscreen { false };

        WindowContext(int width, int height, u32 flags);
        ~WindowContext();

        void toggleFullscreen();
        math::int32x2 getWindowSize() const;

        bool createWaylandWindow(int width, int height, const char* title);
        void processEvents();
    };

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_WAYLAND)
