/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "egl_surface.hpp"

#include <cstdint>

#include "../../window/window_backend.hpp"
#include "../../window/xlib/xlib_window.hpp"

namespace mango::opengl::egl
{

    NativeWindowBinding createXlibNativeWindow(WindowBackend* backend, int width, int height, u32 flags)
    {
        NativeWindowBinding result;

        auto* xlib = static_cast<XlibBackend*>(backend);
        if (!xlib)
        {
            return result;
        }

        void* native = xlib->createNativeWindowForGraphics(width, height, flags);
        if (!native)
        {
            return result;
        }

        result.native_window = native;
        return result;
    }

} // namespace mango::opengl::egl
