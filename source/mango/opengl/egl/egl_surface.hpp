/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/configure.hpp>

namespace mango
{

    struct WindowBackend;

    namespace opengl::egl
    {

        struct NativeWindowBinding
        {
            void* native_window = nullptr;
            void* cookie = nullptr;
            void (*sync)(WindowBackend* backend, void* cookie) = nullptr;
            void (*destroy)(void* cookie) = nullptr;
            bool present_opaque = false;
        };

        void* getNativeDisplay(WindowBackend* backend);
        NativeWindowBinding createNativeWindow(WindowBackend* backend, int width, int height, u32 flags);
        void syncNativeWindow(WindowBackend* backend, NativeWindowBinding& binding);
        void destroyNativeWindow(NativeWindowBinding& binding);

#if defined(MANGO_OPENGL_CONTEXT_EGL)
#if defined(MANGO_HAS_WAYLAND_WINDOW)
        NativeWindowBinding createWaylandNativeWindow(WindowBackend* backend, int width, int height, u32 flags);
#endif
#if defined(MANGO_HAS_XLIB_WINDOW)
        NativeWindowBinding createXlibNativeWindow(WindowBackend* backend, int width, int height, u32 flags);
#endif
#if defined(MANGO_HAS_XCB_WINDOW)
        NativeWindowBinding createXcbNativeWindow(WindowBackend* backend, int width, int height, u32 flags);
#endif
#endif

    } // namespace opengl::egl

} // namespace mango
