/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "egl_surface.hpp"

#include <mango/window/window.hpp>
#include "../../window/window_backend.hpp"

#if defined(MANGO_OPENGL_CONTEXT_EGL)

namespace mango::opengl::egl
{

    void* getNativeDisplay(WindowBackend* backend)
    {
        return backend ? backend->nativeDisplay() : nullptr;
    }

    NativeWindowBinding createNativeWindow(WindowBackend* backend, int width, int height, u32 flags)
    {
        NativeWindowBinding result;

        if (!backend)
        {
            return result;
        }

        switch (Window::getWindowSystem())
        {
#if defined(MANGO_HAS_WAYLAND_WINDOW)
            case WindowSystem::Wayland:
                return createWaylandNativeWindow(backend, width, height, flags);
#endif
#if defined(MANGO_HAS_XLIB_WINDOW)
            case WindowSystem::Xlib:
                return createXlibNativeWindow(backend, width, height, flags);
#endif
#if defined(MANGO_HAS_XCB_WINDOW)
            case WindowSystem::Xcb:
                return createXcbNativeWindow(backend, width, height, flags);
#endif
            default:
                break;
        }

        return result;
    }

    void syncNativeWindow(WindowBackend* backend, NativeWindowBinding& binding)
    {
        if (binding.sync)
        {
            binding.sync(backend, binding.native_window);
        }
    }

    void destroyNativeWindow(NativeWindowBinding& binding)
    {
        if (binding.destroy)
        {
            binding.destroy(binding.cookie);
        }

        binding = {};
    }

} // namespace mango::opengl::egl

#endif // defined(MANGO_OPENGL_CONTEXT_EGL)
