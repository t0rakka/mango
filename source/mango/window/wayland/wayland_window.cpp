/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include <mango/core/timer.hpp>

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

#include <unistd.h>
#include "wayland_window.hpp"
#include <wayland-client-protocol.h>

// TODO: The build scripts must be able to build wayland window code w/o OpenGL build for Vulkan
// TODO: The build scripts must switch to EGL mode when building OpenGL + Wayland
// TODO: This implementaiton is not yet complete it is WIP with a lot of missing features

// TODO: Need to generate and include xdg-shell protocol headers
//#include "xdg-shell-client-protocol.h"

namespace
{
    using namespace mango;

    void shell_surface_ping(void* data, struct wl_shell_surface* shell_surface, uint32_t serial)
    {
        wl_shell_surface_pong(shell_surface, serial);
    }

    void shell_surface_configure(void* data, struct wl_shell_surface* shell_surface,
                               uint32_t edges, int32_t width, int32_t height)
    {
        WindowHandle* handle = static_cast<WindowHandle*>(data);
        if (width > 0 && height > 0) {
            handle->size[0] = width;
            handle->size[1] = height;
        }
    }

    void shell_surface_popup_done(void* data, struct wl_shell_surface* shell_surface)
    {
    }

    static const struct wl_shell_surface_listener shell_surface_listener = {
        shell_surface_ping,
        shell_surface_configure,
        shell_surface_popup_done
    };

    // Registry handling
    void registry_global(void* data, struct wl_registry* registry,
                        uint32_t name, const char* interface, uint32_t version)
    {
        WindowHandle* handle = static_cast<WindowHandle*>(data);

        if (strcmp(interface, "wl_compositor") == 0) {
            handle->compositor = static_cast<struct wl_compositor*>(
                wl_registry_bind(registry, name, &wl_compositor_interface, 1));
        }
        else if (strcmp(interface, "wl_shell") == 0) {
            handle->shell = static_cast<struct wl_shell*>(
                wl_registry_bind(registry, name, &wl_shell_interface, 1));
        }
        else if (strcmp(interface, "wl_seat") == 0) {
            handle->seat = static_cast<struct wl_seat*>(
                wl_registry_bind(registry, name, &wl_seat_interface, 1));
        }
    }

    void registry_global_remove(void* data, struct wl_registry* registry, uint32_t name)
    {
    }

    static const struct wl_registry_listener registry_listener = {
        registry_global,
        registry_global_remove
    };

} // namespace

namespace mango
{
    using namespace mango::math;
    using namespace mango::image;
    using namespace mango::filesystem;

    // -----------------------------------------------------------------------
    // WindowHandle
    // -----------------------------------------------------------------------

    WindowHandle::WindowHandle(int width, int height, u32 flags)
        : display(nullptr)
        , registry(nullptr)
        , compositor(nullptr)
        , surface(nullptr)
        , shell(nullptr)
        , shell_surface(nullptr)
        , seat(nullptr)
        , pointer(nullptr)
        , keyboard(nullptr)
        , output(nullptr)
        , xkb_context(nullptr)
        , xkb_keymap(nullptr)
        , xkb_state(nullptr)
        , is_looping(false)
        , busy(false)
    {
        size[0] = width;
        size[1] = height;

        // Connect to Wayland display
        display = wl_display_connect(nullptr);
        if (!display) {
            MANGO_EXCEPTION("[Window] Failed to connect to Wayland display.");
        }

        // Store native handle for EGL/Vulkan
        native.display = display;

        // Get registry
        registry = wl_display_get_registry(display);
        if (!registry) {
            MANGO_EXCEPTION("[Window] Failed to get Wayland registry.");
        }

        // Add registry listener
        wl_registry_add_listener(registry, &registry_listener, this);

        // Wait for registry events
        wl_display_roundtrip(display);

        // Create window
        if (!createWaylandWindow(width, height, "Mango Window")) {
            MANGO_EXCEPTION("[Window] Failed to create Wayland window.");
        }

        // Initialize mouse time array
        for (int i = 0; i < 6; ++i) {
            mouse_time[i] = 0;
        }
    }

    WindowHandle::~WindowHandle()
    {
        if (shell_surface) wl_shell_surface_destroy(shell_surface);
        if (surface) wl_surface_destroy(surface);
        if (shell) wl_shell_destroy(shell);
        if (compositor) wl_compositor_destroy(compositor);
        if (pointer) wl_pointer_destroy(pointer);
        if (keyboard) wl_keyboard_destroy(keyboard);
        if (seat) wl_seat_destroy(seat);
        if (output) wl_output_destroy(output);
        if (registry) wl_registry_destroy(registry);
        if (display) {
            wl_display_flush(display);
            wl_display_disconnect(display);
        }
        if (xkb_state) xkb_state_unref(xkb_state);
        if (xkb_keymap) xkb_keymap_unref(xkb_keymap);
        if (xkb_context) xkb_context_unref(xkb_context);
    }

    math::int32x2 WindowHandle::getWindowSize() const
    {
        // TODO
        return int32x2(0, 0);
    }

    bool WindowHandle::createWaylandWindow(int width, int height, const char* title)
    {
        if (!compositor || !shell) return false;

        // Create surface
        surface = wl_compositor_create_surface(compositor);
        if (!surface) return false;

        // Store surface in native handle
        native.surface = surface;

        // Create shell surface
        shell_surface = wl_shell_get_shell_surface(shell, surface);
        if (!shell_surface) {
            wl_surface_destroy(surface);
            return false;
        }

        wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, this);
        wl_shell_surface_set_toplevel(shell_surface);
        wl_shell_surface_set_title(shell_surface, title);

        // Commit the surface
        wl_surface_commit(surface);

        return true;
    }

    void WindowHandle::processEvents()
    {
        while (wl_display_prepare_read(display) != 0) {
            wl_display_dispatch_pending(display);
        }
        
        wl_display_flush(display);
        
        if (wl_display_read_events(display) >= 0) {
            wl_display_dispatch_pending(display);
        }
    }

    // -----------------------------------------------------------------------
    // Window
    // -----------------------------------------------------------------------

    int Window::getScreenCount()
    {
        // TODO
        return 0;
    }

    int32x2 Window::getScreenSize(int index)
    {
        // TODO
        return int32x2(0, 0);
    }

    Window::Window(int width, int height, u32 flags)
    {
        m_handle = std::make_unique<WindowHandle>(width, height, flags);
    }

    Window::~Window()
    {
    }

    void Window::setWindowPosition(int x, int y)
    {
        // TODO
    }

    void Window::setWindowSize(int width, int height)
    {
        // TODO
    }

    void Window::setTitle(const std::string& title)
    {
        // TODO
    }

    void Window::setIcon(const Surface& surface)
    {
        // TODO
    }

    void Window::setVisible(bool enable)
    {
        // TODO
    }

    int32x2 Window::getWindowSize() const
    {
        return m_handle->getWindowSize();
    }

    int32x2 Window::getCursorPosition() const
    {
        // TODO
        return int32x2(0, 0);
    }

    bool Window::isKeyPressed(Keycode code) const
    {
        // TODO
        return false;
    }

    Window::operator NativeWindowHandle () const
    {
        return m_handle->native;
    }

    Window::operator WindowHandle* () const
    {
        return m_handle.get();
    }

    void Window::enterEventLoop()
    {
        // TODO
    }

    void Window::breakEventLoop()
    {
        // TODO
    }

    void Window::onIdle()
    {
    }

    void Window::onDraw()
    {
    }

    void Window::onResize(int width, int height)
    {
        MANGO_UNREFERENCED(width);
        MANGO_UNREFERENCED(height);
    }

    void Window::onMinimize()
    {
    }

    void Window::onMaximize()
    {
    }

    void Window::onKeyPress(Keycode code, u32 mask)
    {
        MANGO_UNREFERENCED(code);
        MANGO_UNREFERENCED(mask);
    }

    void Window::onKeyRelease(Keycode code)
    {
        MANGO_UNREFERENCED(code);
    }

    void Window::onMouseMove(int x, int y)
    {
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
    }

    void Window::onMouseClick(int x, int y, MouseButton button, int count)
    {
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
        MANGO_UNREFERENCED(button);
        MANGO_UNREFERENCED(count);
    }

    void Window::onDropFiles(const FileIndex& index)
    {
        MANGO_UNREFERENCED(index);
    }

    void Window::onClose()
    {
    }

    void Window::onShow()
    {
    }

    void Window::onHide()
    {
    }

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_WAYLAND)
