/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "egl_surface.hpp"

#if defined(MANGO_HAS_WAYLAND_WINDOW)

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

        void recreateWaylandNativeWindow(NativeWindowBinding& binding)
        {
            auto* state = static_cast<WaylandEGLState*>(binding.cookie);
            if (!state || !state->backend || !state->backend->surface)
            {
                return;
            }

            const int32_t width = std::max(1, state->backend->size[0] > 0 ? state->backend->size[0] : state->synced_size[0]);
            const int32_t height = std::max(1, state->backend->size[1] > 0 ? state->backend->size[1] : state->synced_size[1]);

            if (state->egl_window)
            {
                wl_egl_window_destroy(state->egl_window);
                state->egl_window = nullptr;
            }

            state->egl_window = wl_egl_window_create(state->backend->surface, width, height);
            if (!state->egl_window)
            {
                binding.native_window = nullptr;
                return;
            }

            if (state->backend->display)
            {
                wl_display_flush(state->backend->display);
            }

            state->synced_size[0] = width;
            state->synced_size[1] = height;
            binding.native_window = state->egl_window;
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

        // Prefer the size requested by the GL layer; fall back to the window
        // backend size if the caller passed 0 (e.g. some recreate paths).
        const int egl_width = std::max(1, width > 0 ? width : wayland->size[0]);
        const int egl_height = std::max(1, height > 0 ? height : wayland->size[1]);

        auto* state = new WaylandEGLState();
        state->backend = wayland;
        state->egl_window = wl_egl_window_create(wayland->surface, egl_width, egl_height);
        if (!state->egl_window)
        {
            delete state;
            return result;
        }

        if (wayland->display)
        {
            wl_display_flush(wayland->display);
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
        result.recreate = recreateWaylandNativeWindow;

        return result;
    }

} // namespace mango::opengl::egl

#endif // defined(MANGO_HAS_WAYLAND_WINDOW)
