/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "egl_surface.hpp"

#include <algorithm>
#include <cstdint>

#include "../../window/window_backend.hpp"
#include "../../window/wayland/wayland_window.hpp"

#define MESA_EGL_NO_X11_HEADERS
#define EGL_NO_X11
#define USE_OZONE

#include <wayland-egl.h>

namespace mango::opengl::egl
{

    namespace
    {

        struct WaylandEGLState
        {
            WaylandBackend* backend = nullptr;
            struct wl_egl_window* egl_window = nullptr;
            int32_t synced_size[2] = { 0, 0 };
        };

        void syncWaylandState(void* cookie)
        {
            auto* state = static_cast<WaylandEGLState*>(cookie);
            if (!state || !state->backend || !state->egl_window)
            {
                return;
            }

            state->backend->syncSurfaceScale();

            const int32_t width = state->backend->size[0];
            const int32_t height = state->backend->size[1];
            if (width <= 0 || height <= 0)
            {
                return;
            }

            if (state->synced_size[0] == width && state->synced_size[1] == height)
            {
                return;
            }

            wl_egl_window_resize(state->egl_window, width, height, 0, 0);
            state->synced_size[0] = width;
            state->synced_size[1] = height;
        }

        void presentWaylandState(void* cookie)
        {
            auto* state = static_cast<WaylandEGLState*>(cookie);
            if (!state || !state->backend)
            {
                return;
            }

            const bool first = (state->synced_size[0] == 0 && state->synced_size[1] == 0);
            if (first && state->backend->display)
            {
                wl_display_roundtrip(state->backend->display);
            }

            syncWaylandState(cookie);
            state->backend->beforePresent();
        }

        void destroyWaylandState(void* cookie)
        {
            auto* state = static_cast<WaylandEGLState*>(cookie);
            if (!state)
            {
                return;
            }

            if (state->egl_window)
            {
                wl_egl_window_destroy(state->egl_window);
            }

            delete state;
        }

    } // namespace

    NativeWindowBinding createWaylandNativeWindow(WindowBackend* backend, int width, int height, u32 flags)
    {
        NativeWindowBinding result;
        MANGO_UNREFERENCED(flags);

        auto* wayland = static_cast<WaylandBackend*>(backend);
        if (!wayland || !wayland->surface)
        {
            return result;
        }

        const int egl_width = std::max(1, wayland->size[0] > 0 ? wayland->size[0] : width);
        const int egl_height = std::max(1, wayland->size[1] > 0 ? wayland->size[1] : height);

        auto* state = new WaylandEGLState();
        state->backend = wayland;
        state->egl_window = wl_egl_window_create(wayland->surface, egl_width, egl_height);
        if (!state->egl_window)
        {
            delete state;
            return result;
        }

        state->synced_size[0] = egl_width;
        state->synced_size[1] = egl_height;

        wayland->graphics_hooks.cookie = state;
        wayland->graphics_hooks.sync = syncWaylandState;
        wayland->graphics_hooks.present = presentWaylandState;
        wayland->graphics_hooks.destroy = destroyWaylandState;

        result.native_window = state->egl_window;
        result.present_opaque = true;
        result.cookie = state;
        result.sync = [](WindowBackend* window_backend, void* cookie) {
            MANGO_UNREFERENCED(window_backend);
            syncWaylandState(cookie);
        };
        result.destroy = destroyWaylandState;

        return result;
    }

} // namespace mango::opengl::egl
