/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>
#include "../window_backend.hpp"

#include <mango/math/math.hpp>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;

namespace mango
{

    struct WaylandBackend : WindowBackend
    {
        struct wl_display* display = nullptr;
        struct wl_surface* surface = nullptr;

        struct wl_registry* registry = nullptr;
        struct wl_compositor* compositor = nullptr;
        struct xdg_wm_base* xdg_wm_base = nullptr;
        struct xdg_surface* xdg_surface = nullptr;
        struct xdg_toplevel* xdg_toplevel = nullptr;

        struct wl_seat* seat = nullptr;
        struct wl_pointer* pointer = nullptr;
        struct wl_keyboard* keyboard = nullptr;

        struct xkb_context* xkb_context = nullptr;
        struct xkb_keymap* xkb_keymap = nullptr;
        struct xkb_state* xkb_state = nullptr;

        bool keyboard_focused = false;
        bool key_pressed[256] = {};

        bool pointer_focused = false;

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
        int32_t buffer_scale = 1;

        WaylandBackend(int width, int height, u32 flags);
        ~WaylandBackend() override;

        void setWindowPosition(int x, int y) override;
        void setWindowSize(int width, int height) override;
        void setTitle(const std::string& title) override;
        void setVisible(bool enable) override;
        math::int32x2 getWindowSize() const override;
        math::int32x2 getClientSize() const override;
        float getContentScale() const override;
        math::int32x2 getCursorPosition() const override;
        double getDisplayRefreshRate() const override;
        void toggleFullscreen() override;
        bool isFullscreen() const override;
        bool isKeyPressed(Keycode code) const override;
        void runEventLoop() override;
        void wakeEventLoop() override;
        void beforePresent() override;

        void* nativeDisplay() override { return display; }
        void* nativeSurface() override { return surface; }

        bool createWaylandWindow(int width, int height, const char* title);
        void syncSurfaceScale();
        void syncOpaqueRegion();
        void syncGraphicsSurfaceFromResize();
        void requestRefresh();
        void processEvents();
        void dispatchPendingResize();
    };

} // namespace mango
